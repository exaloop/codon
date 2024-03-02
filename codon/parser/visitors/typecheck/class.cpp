// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Parse a class (type) declaration and add a (generic) type to the context.
void TypecheckVisitor::visit(ClassStmt *stmt) {
  // Extensions are not possible after the simplification
  seqassert(!stmt->attributes.has(Attr::Extend), "invalid extension '{}'", stmt->name);
  // Type should be constructed only once
  stmt->setDone();

  // Generate the type and add it to the context
  auto typ = Type::makeType(ctx->cache, stmt->name, ctx->cache->rev(stmt->name),
                            stmt->isRecord())
                 ->getClass();
  if (stmt->isRecord() && stmt->hasAttr("__notuple__"))
    typ->getRecord()->noTuple = true;
  if (stmt->isRecord() && startswith(stmt->name, TYPE_PARTIAL)) {
    // Special handling of partial types (e.g., `Partial.0001.foo`)
    if (auto p = in(ctx->cache->partials, stmt->name))
      typ = std::make_shared<PartialType>(typ->getRecord(), p->first, p->second);
  }
  typ->setSrcInfo(stmt->getSrcInfo());
  // Classes should always be visible, so add them to the toplevel
  ctx->addToplevel(stmt->name,
                   std::make_shared<TypecheckItem>(TypecheckItem::Type, typ));

  // Handle generics
  for (const auto &a : stmt->args) {
    if (a.status != Param::Normal) {
      // Generic and static types
      auto generic = ctx->getUnbound();
      generic->isStatic = getStaticGeneric(a.type.get());
      auto typId = generic->id;
      generic->getLink()->genericName = ctx->cache->rev(a.name);
      if (a.defaultValue) {
        auto defType = transformType(clone(a.defaultValue));
        if (a.status == Param::Generic) {
          generic->defaultType = defType->type;
        } else {
          // Hidden generics can be outright replaced (e.g., `T=int`).
          // Unify them immediately.
          unify(defType->type, generic);
        }
      }
      if (auto ti = CAST(a.type, InstantiateExpr)) {
        // Parse TraitVar
        seqassert(ti->typeExpr->isId(TYPE_TYPEVAR), "not a TypeVar instantiation");
        auto l = transformType(ti->typeParams[0])->type;
        if (l->getLink() && l->getLink()->trait)
          generic->getLink()->trait = l->getLink()->trait;
        else
          generic->getLink()->trait = std::make_shared<types::TypeTrait>(l);
      }
      ctx->add(TypecheckItem::Type, a.name, generic);
      ClassType::Generic g{a.name, ctx->cache->rev(a.name),
                           generic->generalize(ctx->typecheckLevel), typId};
      if (a.status == Param::Generic) {
        typ->generics.push_back(g);
      } else {
        typ->hiddenGenerics.push_back(g);
      }
    }
  }

  // Handle class members
  ctx->typecheckLevel++; // to avoid unifying generics early
  auto &fields = ctx->cache->classes[stmt->name].fields;
  for (auto ai = 0, aj = 0; ai < stmt->args.size(); ai++)
    if (stmt->args[ai].status == Param::Normal) {
      fields[aj].type = transformType(stmt->args[ai].type)
                            ->getType()
                            ->generalize(ctx->typecheckLevel - 1);
      fields[aj].type->setSrcInfo(stmt->args[ai].type->getSrcInfo());
      if (stmt->isRecord())
        typ->getRecord()->args.push_back(fields[aj].type);
      aj++;
    }
  ctx->typecheckLevel--;

  // Handle MRO
  for (auto &m : ctx->cache->classes[stmt->name].mro) {
    m = transformType(m);
  }

  // Generalize generics and remove them from the context
  for (const auto &g : stmt->args)
    if (g.status != Param::Normal) {
      auto generic = ctx->forceFind(g.name)->type;
      if (g.status == Param::Generic) {
        // Generalize generics. Hidden generics are linked to the class generics so
        // ignore them
        seqassert(generic && generic->getLink() &&
                      generic->getLink()->kind != types::LinkType::Link,
                  "generic has been unified");
        generic->getLink()->kind = LinkType::Generic;
      }
      ctx->remove(g.name);
    }

  // Debug information
  LOG_REALIZE("[class] {} -> {}", stmt->name, typ);
  for (auto &m : ctx->cache->classes[stmt->name].fields)
    LOG_REALIZE("       - member: {}: {}", m.name, m.type);
}

/// Generate a tuple class `Tuple[T1,...,TN]`.
/// @param len       Tuple length (`N`)
/// @param name      Tuple name. `Tuple` by default.
///                  Can be something else (e.g., `KwTuple`)
/// @param names     Member names. By default `item1`...`itemN`.
/// @param hasSuffix Set if the tuple name should have `.N` suffix.
std::string TypecheckVisitor::generateTuple(size_t len, const std::string &name,
                                            std::vector<std::string> names,
                                            bool hasSuffix) {
  auto key = join(names, ";");
  std::string suffix;
  if (!names.empty()) {
    // Each set of names generates different tuple (i.e., `KwArgs[foo, bar]` is not the
    // same as `KwArgs[bar, baz]`). Cache the names and use an integer for each name
    // combination.
    if (!in(ctx->cache->generatedTuples, key))
      ctx->cache->generatedTuples[key] = int(ctx->cache->generatedTuples.size());
    suffix = format("_{}", ctx->cache->generatedTuples[key]);
  } else {
    for (size_t i = 1; i <= len; i++)
      names.push_back(format("item{}", i));
  }

  auto typeName = format("{}{}", name, hasSuffix ? format("{}{}", len, suffix) : "");
  if (!ctx->find(typeName)) {
    // Generate the appropriate ClassStmt
    std::vector<Param> args;
    for (size_t i = 0; i < len; i++)
      args.emplace_back(Param(names[i], N<IdExpr>(format("T{}", i + 1)), nullptr));
    for (size_t i = 0; i < len; i++)
      args.emplace_back(Param(format("T{}", i + 1), N<IdExpr>("type"), nullptr, true));
    StmtPtr stmt = N<ClassStmt>(ctx->cache->generateSrcInfo(), typeName, args, nullptr,
                                std::vector<ExprPtr>{N<IdExpr>("tuple")});

    // Add helpers for KwArgs:
    //   `def __getitem__(self, key: Static[str]): return getattr(self, key)`
    //   `def __contains__(self, key: Static[str]): return hasattr(self, key)`
    auto getItem = N<FunctionStmt>(
        "__getitem__", nullptr,
        std::vector<Param>{Param{"self"}, Param{"key", N<IndexExpr>(N<IdExpr>("Static"),
                                                                    N<IdExpr>("str"))}},
        N<SuiteStmt>(N<ReturnStmt>(
            N<CallExpr>(N<IdExpr>("getattr"), N<IdExpr>("self"), N<IdExpr>("key")))));
    auto contains = N<FunctionStmt>(
        "__contains__", nullptr,
        std::vector<Param>{Param{"self"}, Param{"key", N<IndexExpr>(N<IdExpr>("Static"),
                                                                    N<IdExpr>("str"))}},
        N<SuiteStmt>(N<ReturnStmt>(
            N<CallExpr>(N<IdExpr>("hasattr"), N<IdExpr>("self"), N<IdExpr>("key")))));
    auto getDef = N<FunctionStmt>(
        "get", nullptr,
        std::vector<Param>{
            Param{"self"},
            Param{"key", N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("str"))},
            Param{"default", nullptr, N<CallExpr>(N<IdExpr>("NoneType"))}},
        N<SuiteStmt>(N<ReturnStmt>(
            N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "kwargs_get"),
                        N<IdExpr>("self"), N<IdExpr>("key"), N<IdExpr>("default")))));
    if (startswith(typeName, TYPE_KWTUPLE))
      stmt->getClass()->suite = N<SuiteStmt>(getItem, contains, getDef);

    // Add repr and call for partials:
    //   `def __repr__(self): return __magic__.repr_partial(self)`
    auto repr = N<FunctionStmt>(
        "__repr__", nullptr, std::vector<Param>{Param{"self"}},
        N<SuiteStmt>(N<ReturnStmt>(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("__magic__"), "repr_partial"), N<IdExpr>("self")))));
    auto pcall = N<FunctionStmt>(
        "__call__", nullptr,
        std::vector<Param>{Param{"self"}, Param{"*args"}, Param{"**kwargs"}},
        N<SuiteStmt>(
            N<ReturnStmt>(N<CallExpr>(N<IdExpr>("self"), N<StarExpr>(N<IdExpr>("args")),
                                      N<KeywordStarExpr>(N<IdExpr>("kwargs"))))));
    if (startswith(typeName, TYPE_PARTIAL))
      stmt->getClass()->suite = N<SuiteStmt>(repr, pcall);

    // Simplify in the standard library context and type check
    stmt = SimplifyVisitor::apply(ctx->cache->imports[STDLIB_IMPORT].ctx, stmt,
                                  FILE_GENERATED, 0);
    stmt = TypecheckVisitor(ctx).transform(stmt);
    prependStmts->push_back(stmt);
  }
  return typeName;
}

} // namespace codon::ast
