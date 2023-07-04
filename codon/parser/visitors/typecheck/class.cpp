// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Parse a class (type) declaration and add a (generic) type to the context.
void TypecheckVisitor::visit(ClassStmt *stmt) {
  // Get root name
  std::string name = stmt->name;

  // Generate/find class' canonical name (unique ID) and AST
  std::string canonicalName;
  std::vector<Param> &argsToParse = stmt->args;

  // classItem will be added later when the scope is different
  auto classItem = std::make_shared<TypecheckItem>("", "", ctx->getModule(), nullptr,
                                                   ctx->getScope());
  classItem->setSrcInfo(stmt->getSrcInfo());
  types::ClassTypePtr typ = nullptr;
  if (!stmt->attributes.has(Attr::Extend)) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !stmt->attributes.has(Attr::Internal),
                                   /* noSuffix*/ stmt->attributes.has(Attr::Internal));

    typ = Type::makeType(ctx->cache, canonicalName, name, stmt->isRecord())->getClass();
    if (stmt->isRecord() && stmt->hasAttr("__notuple__"))
      typ->getRecord()->noTuple = true;
    if (stmt->isRecord() && startswith(stmt->name, TYPE_PARTIAL)) {
      // Special handling of partial types (e.g., `Partial.0001.foo`)
      if (auto p = in(ctx->cache->partials, stmt->name))
        typ = std::make_shared<PartialType>(typ->getRecord(), p->first, p->second);
    }
    typ->setSrcInfo(stmt->getSrcInfo());
    classItem->type = typ;

    // Reference types are added to the context here.
    // Tuple types are added after class contents are parsed to prevent
    // recursive record types (note: these are allowed for reference types)
    if (!stmt->attributes.has(Attr::Tuple)) {
      auto v = ctx->find(name);
      if (v && !v->canShadow)
        E(Error::ID_INVALID_BIND, getSrcInfo(), name);
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
    }
  } else {
    // Find the canonical name and AST of the class that is to be extended
    if (!ctx->isGlobal() || ctx->isConditional())
      E(Error::EXPECTED_TOPLEVEL, getSrcInfo(), "class extension");
    auto val = ctx->find(name);
    if (!val || !val->isType())
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), name);
    canonicalName = val->canonicalName;
    typ = val->type->getClass();
    const auto &astIter = ctx->cache->classes.find(canonicalName);
    if (astIter == ctx->cache->classes.end()) {
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), name);
    } else {
      argsToParse = astIter->second.ast->args;
    }
  }

  std::vector<StmtPtr> clsStmts; // Will be filled later!
  std::vector<StmtPtr> varStmts; // Will be filled later!
  std::vector<StmtPtr> fnStmts;  // Will be filled later!
  std::vector<TypeContext::Item> addLater;
  try {
    // Add the class base
    TypeContext::BaseGuard br(ctx.get(), canonicalName);
    ctx->getBase()->type = typ;

    // Parse and add class generics
    std::vector<Param> args;
    std::pair<StmtPtr, FunctionStmt *> autoDeducedInit{nullptr, nullptr};
    if (stmt->attributes.has("deduce") && args.empty()) {
      // todo)) do this
      // Auto-detect generics and fields
      // autoDeducedInit = autoDeduceMembers(stmt, args);
    } else if (stmt->attributes.has(Attr::Extend)) {
      for (auto &a : argsToParse) {
        if (a.status != Param::Generic)
          continue;
        auto val = ctx->forceFind(a.name);
        auto generic = ctx->instantiate(val->type);
        generic->getUnbound()->id = val->type->getLink()->id;
        ctx->addType(ctx->cache->rev(val->canonicalName), val->canonicalName, generic)
            ->generic = true;
      }
    } else {
      // Add all generics before parent classes, fields and methods
      for (auto &a : argsToParse) {
        if (a.status != Param::Generic)
          continue;

        auto varName = ctx->generateCanonicalName(a.name), genName = a.name;
        auto generic = ctx->getUnbound();
        auto typId = generic->id;
        generic->getLink()->genericName = genName;
        if (a.defaultValue) {
          auto defType = transformType(clone(a.defaultValue));
          generic->defaultType = defType->type;
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
        if (auto st = getStaticGeneric(a.type.get())) {
          generic->isStatic = st;
          auto val = ctx->addVar(genName, varName, generic);
          val->generic = true;
          val->staticType = st;
        } else {
          ctx->addType(genName, varName, generic)->generic = true;
        }
        ClassType::Generic g{varName, genName, generic->generalize(ctx->typecheckLevel),
                             typId};
        if (a.status == Param::Generic) {
          typ->generics.push_back(g);
        } else {
          typ->hiddenGenerics.push_back(g);
        }
        args.emplace_back(varName, transformType(clone(a.type), false),
                          transformType(clone(a.defaultValue), false), a.status);
      }
    }

    // Form class type node (e.g. `Foo`, or `Foo[T, U]` for generic classes)
    ExprPtr typeAst = N<IdExpr>(name), transformedTypeAst = NT<IdExpr>(canonicalName);
    for (auto &a : args) {
      if (a.status == Param::Generic) {
        if (!typeAst->getIndex()) {
          typeAst = N<IndexExpr>(N<IdExpr>(name), N<TupleExpr>());
          transformedTypeAst =
              NT<InstantiateExpr>(NT<IdExpr>(canonicalName), std::vector<ExprPtr>{});
        }
        typeAst->getIndex()->index->getTuple()->items.push_back(N<IdExpr>(a.name));
        CAST(transformedTypeAst, InstantiateExpr)
            ->typeParams.push_back(transform(N<IdExpr>(a.name), true));
      }
    }

    // Collect classes (and their fields) that are to be statically inherited
    std::vector<ClassStmt *> staticBaseASTs, baseASTs;
    if (!stmt->attributes.has(Attr::Extend)) {
      staticBaseASTs = parseBaseClasses(stmt->staticBaseClasses, args, stmt->attributes,
                                        canonicalName, nullptr, typ);
      if (ctx->cache->isJit && !stmt->baseClasses.empty())
        E(Error::CUSTOM, stmt->baseClasses[0],
          "inheritance is not yet supported in JIT mode");
      parseBaseClasses(stmt->baseClasses, args, stmt->attributes, canonicalName,
                       transformedTypeAst, typ);
    }

    // A ClassStmt will be separated into class variable assignments, method-free
    // ClassStmts (that include nested classes) and method FunctionStmts
    transformNestedClasses(stmt, clsStmts, varStmts, fnStmts);

    // Collect class fields
    for (auto &a : argsToParse) {
      if (a.status == Param::Normal) {
        if (ClassStmt::isClassVar(a)) {
          // Handle class variables. Transform them later to allow self-references
          auto name = format("{}.{}", canonicalName, a.name);
          // prependStmts->push_back(N<AssignStmt>(N<IdExpr>(name), nullptr, nullptr));
          // ctx->cache->addGlobal(name);
          auto assign = N<AssignStmt>(N<IdExpr>(name), a.defaultValue,
                                      a.type ? a.type->getIndex()->index : nullptr);
          varStmts.push_back(assign);
          ctx->cache->classes[canonicalName].classVars[a.name] = name;
        } else if (!stmt->attributes.has(Attr::Extend)) {
          std::string varName = a.name;
          // stmt->attributes.has(Attr::Extend)
          //                           ? a.name
          //                           : ctx->generateCanonicalName(a.name);
          args.emplace_back(varName, transformType(clone(a.type), false),
                            transform(clone(a.defaultValue), true));
          ctx->cache->classes[canonicalName].fields.push_back(Cache::Class::ClassField{
              varName, args.back().type->getType(), canonicalName});
        }
      }
    }

    // ASTs for member arguments to be used for populating magic methods
    std::vector<Param> memberArgs;
    for (auto &a : args) {
      if (a.status == Param::Normal) {
        memberArgs.push_back(a.clone());
      }
    }

    // Handle class members
    if (!stmt->attributes.has(Attr::Extend)) {
      ctx->typecheckLevel++; // to avoid unifying generics early
      auto &fields = ctx->cache->classes[canonicalName].fields;
      for (auto ai = 0, aj = 0; ai < stmt->args.size(); ai++)
        if (stmt->args[ai].status == Param::Normal &&
            !ClassStmt::isClassVar(stmt->args[ai])) {
          fields[aj].type = transformType(stmt->args[ai].type)
                                ->getType()
                                ->generalize(ctx->typecheckLevel - 1);
          fields[aj].type->setSrcInfo(stmt->args[ai].type->getSrcInfo());
          if (stmt->isRecord())
            typ->getRecord()->args.push_back(fields[aj].type);
          aj++;
        }
      ctx->typecheckLevel--;
    }

    // Parse class members (arguments) and methods
    if (!stmt->attributes.has(Attr::Extend)) {
      // Now that we are done with arguments, add record type to the context
      if (stmt->attributes.has(Attr::Tuple)) {
        // Ensure that class binding does not shadow anything.
        // Class bindings cannot be dominated either
        auto v = ctx->find(name);
        if (v && !v->canShadow)
          E(Error::CLASS_INVALID_BIND, stmt, name);
        ctx->add(name, classItem);
        ctx->addAlwaysVisible(classItem);
      }
      // Create a cached AST.
      stmt->attributes.module = ctx->moduleName.status == ImportFile::STDLIB
                                    ? STDLIB_IMPORT
                                    : ctx->moduleName.path;
      ctx->cache->classes[canonicalName].ast =
          N<ClassStmt>(canonicalName, args, N<SuiteStmt>(), stmt->attributes);
      ctx->cache->classes[canonicalName].ast->baseClasses = stmt->baseClasses;
      for (auto &b : staticBaseASTs)
        ctx->cache->classes[canonicalName].staticParentClasses.emplace_back(b->name);
      ctx->cache->classes[canonicalName].ast->validate();
      ctx->cache->classes[canonicalName].module = ctx->getModule();

      // Handle MRO
      for (auto &m : ctx->cache->classes[canonicalName].mro) {
        m = transformType(m);
      }

      // Codegen default magic methods
      for (auto &m : stmt->attributes.magics) {
        fnStmts.push_back(transform(
            codegenMagic(m, typeAst, memberArgs, stmt->attributes.has(Attr::Tuple))));
      }
      // Add inherited methods
      for (auto &base : staticBaseASTs) {
        for (auto &mm : ctx->cache->classes[base->name].methods)
          for (auto &mf : ctx->cache->overloads[mm.second]) {
            auto f = ctx->cache->functions[mf].ast;
            if (!f->attributes.has("autogenerated")) {
              std::string rootName;
              auto &mts = ctx->cache->classes[ctx->getBase()->name].methods;
              auto it = mts.find(ctx->cache->rev(f->name));
              if (it != mts.end())
                rootName = it->second;
              else
                rootName = ctx->generateCanonicalName(ctx->cache->rev(f->name), true);
              auto newCanonicalName =
                  format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
              ctx->cache->overloads[rootName].push_back(newCanonicalName);
              ctx->cache->reverseIdentifierLookup[newCanonicalName] =
                  ctx->cache->rev(f->name);
              auto nf = std::dynamic_pointer_cast<FunctionStmt>(f->clone());
              nf->name = newCanonicalName;
              nf->attributes.parentClass = ctx->getBase()->name;
              ctx->cache->functions[newCanonicalName].ast = nf;
              ctx->cache->classes[ctx->getBase()->name]
                  .methods[ctx->cache->rev(f->name)] = rootName;
              fnStmts.push_back(nf);
            }
          }
      }
      // Add auto-deduced __init__ (if available)
      if (autoDeducedInit.first)
        fnStmts.push_back(autoDeducedInit.first);
    }
    // Add class methods
    for (const auto &sp : getClassMethods(stmt->suite))
      if (sp && sp->getFunction()) {
        if (sp.get() != autoDeducedInit.second)
          fnStmts.push_back(transform(sp));
      }

    // After popping context block, record types and nested classes will disappear.
    // Store their references and re-add them to the context after popping
    addLater.reserve(clsStmts.size() + 1);
    for (auto &c : clsStmts)
      addLater.push_back(ctx->find(c->getClass()->name));
    if (stmt->attributes.has(Attr::Tuple))
      addLater.push_back(ctx->forceFind(name));

    // Mark functions as virtual:
    auto banned =
        std::set<std::string>{"__init__", "__new__", "__raw__", "__tuplesize__"};
    for (auto &m : ctx->cache->classes[canonicalName].methods) {
      auto method = m.first;
      for (size_t mi = 1; mi < ctx->cache->classes[canonicalName].mro.size(); mi++) {
        // ... in the current class
        auto b = ctx->cache->classes[canonicalName].mro[mi]->getTypeName();
        if (in(ctx->cache->classes[b].methods, method) && !in(banned, method)) {
          ctx->cache->classes[canonicalName].virtuals.insert(method);
        }
      }
      for (auto &v : ctx->cache->classes[canonicalName].virtuals) {
        for (size_t mi = 1; mi < ctx->cache->classes[canonicalName].mro.size(); mi++) {
          // ... and in parent classes
          auto b = ctx->cache->classes[canonicalName].mro[mi]->getTypeName();
          ctx->cache->classes[b].virtuals.insert(v);
        }
      }
    }

    // Generalize generics and remove them from the context
    for (const auto &g : args)
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
    LOG("[class] {} -> {:D} / {}", canonicalName, typ,
        ctx->cache->classes[canonicalName].fields.size());
    for (auto &m : ctx->cache->classes[canonicalName].fields)
      LOG("       - member: {}: {:D}", m.name, m.type);
    for (auto &m : ctx->cache->classes[canonicalName].methods)
      LOG("       - method: {}: {}", m.first, m.second);
  } catch (const exc::ParserException &) {
    if (!stmt->attributes.has(Attr::Tuple))
      ctx->remove(name);
    ctx->cache->classes.erase(name);
    throw;
  }
  for (auto &i : addLater)
    ctx->add(ctx->cache->rev(i->canonicalName), i);

  // Extensions are not needed as the cache is already populated
  if (!stmt->attributes.has(Attr::Extend)) {
    auto c = ctx->cache->classes[canonicalName].ast;
    seqassert(c, "not a class AST for {}", canonicalName);
    c->setDone();
    clsStmts.push_back(c);
  }

  clsStmts.insert(clsStmts.end(), fnStmts.begin(), fnStmts.end());
  for (auto &a : varStmts) {
    // Transform class variables here to allow self-references
    transform(a);
    // if (auto assign = a->getAssign()) {
    //   transform(assign->rhs);
    //   transformType(assign->type);
    // }
    clsStmts.push_back(a);
  }
  resultStmt = N<SuiteStmt>(clsStmts);
}

/// Parse statically inherited classes.
/// Returns a list of their ASTs. Also updates the class fields.
/// @param args Class fields that are to be updated with base classes' fields.
/// @param typeAst Transformed AST for base class type (e.g., `A[T]`).
///                Only set when dealing with dynamic polymorphism.
std::vector<ClassStmt *>
TypecheckVisitor::parseBaseClasses(std::vector<ExprPtr> &baseClasses,
                                   std::vector<Param> &args, const Attr &attr,
                                   const std::string &canonicalName,
                                   const ExprPtr &typeAst, types::ClassTypePtr &typ) {
  std::vector<ClassStmt *> asts;

  // MAJOR TODO: fix MRO it to work with generic classes (maybe replacements? IDK...)
  std::vector<std::vector<ExprPtr>> mro{{typeAst}};
  std::vector<ExprPtr> parentClasses;
  for (auto &cls : baseClasses) {
    std::string name;
    std::vector<ExprPtr> subs;

    // Get the base class and generic replacements (e.g., if there is Bar[T],
    // Bar in Foo(Bar[int]) will have `T = int`)
    transformType(cls);
    if (auto i = cls->getId()) {
      name = i->value;
    } else if (auto e = CAST(cls, InstantiateExpr)) {
      if (auto ei = e->typeExpr->getId()) {
        name = ei->value;
        subs = e->typeParams;
      }
    }

    Cache::Class *cachedCls = in(ctx->cache->classes, name);
    if (!cachedCls)
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), ctx->cache->rev(name));
    asts.push_back(cachedCls->ast.get());
    parentClasses.push_back(clone(cls));
    mro.push_back(cachedCls->mro);

    // Sanity checks
    if (attr.has(Attr::Tuple) && typeAst)
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "tuple");
    if (!attr.has(Attr::Tuple) && asts.back()->attributes.has(Attr::Tuple))
      E(Error::CLASS_TUPLE_INHERIT, getSrcInfo());
    if (asts.back()->attributes.has(Attr::Internal))
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "internal");

    // Mark parent classes as polymorphic as well.
    if (typeAst) {
      cachedCls->rtti = true;
    }

    // Add generics first
    int nGenerics = 0;
    for (auto &a : asts.back()->args)
      nGenerics += a.status == Param::Generic;
    int si = 0;
    for (auto &a : asts.back()->args) {
      if (a.status == Param::Normal)
        continue;

      if (a.status == Param::Generic) {
        if (si == subs.size())
          E(Error::GENERICS_MISMATCH, cls, ctx->cache->rev(asts.back()->name),
            nGenerics, subs.size());
        args.emplace_back(a.name, a.type, transformType(subs[si++], false),
                          Param::HiddenGeneric);
      } else if (a.status == Param::HiddenGeneric) {
        args.emplace_back(a);
      }

      auto generic = ctx->getUnbound();
      auto typId = generic->id;
      generic->getLink()->genericName = ctx->cache->rev(a.name);
      if (args.back().defaultValue) {
        auto defType = transformType(clone(args.back().defaultValue));
        // Hidden generics can be outright replaced (e.g., `T=int`).
        // Unify them immediately.
        unify(defType->type, generic);
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

      if (auto st = getStaticGeneric(a.type.get())) {
        generic->isStatic = st;
        auto val = ctx->addVar(a.name, a.name, generic);
        val->generic = true;
        val->staticType = st;
      } else {
        ctx->addType(a.name, a.name, generic)->generic = true;
      }
      ClassType::Generic g{a.name, a.name, generic->generalize(ctx->typecheckLevel),
                           typId};
      if (a.status == Param::Generic) {
        typ->generics.push_back(g);
      } else {
        typ->hiddenGenerics.push_back(g);
      }
    }
    if (si != subs.size())
      E(Error::GENERICS_MISMATCH, cls, ctx->cache->rev(asts.back()->name), nGenerics,
        subs.size());
  }
  // Add normal fields
  for (auto &ast : asts) {
    int ai = 0;
    for (auto &a : ast->args) {
      if (a.status == Param::Normal && !ClassStmt::isClassVar(a)) {
        auto name = a.name;
        int i = 0;
        for (auto &aa : args)
          i += aa.name == a.name || startswith(aa.name, a.name + "#");
        if (i)
          name = format("{}#{}", name, i);
        seqassert(ctx->cache->classes[ast->name].fields[ai].name == a.name,
                  "bad class fields: {} vs {}",
                  ctx->cache->classes[ast->name].fields[ai].name, a.name);
        args.emplace_back(name, transformType(a.type), transform(a.defaultValue));
        ctx->cache->classes[canonicalName].fields.push_back(Cache::Class::ClassField{
            name, args.back().type->getType(),
            ctx->cache->classes[ast->name].fields[ai].baseClass});
        ai++;
      }
    }
  }
  if (typeAst) {
    if (!parentClasses.empty()) {
      mro.push_back(parentClasses);
      ctx->cache->classes[canonicalName].rtti = true;
    }
    ctx->cache->classes[canonicalName].mro = Cache::mergeC3(mro);
    if (ctx->cache->classes[canonicalName].mro.empty()) {
      E(Error::CLASS_BAD_MRO, getSrcInfo());
    } else if (ctx->cache->classes[canonicalName].mro.size() > 1) {
      // LOG("[mro] {} -> {}", canonicalName, ctx->cache->classes[canonicalName].mro);
    }
  }
  return asts;
}

/// Find the first __init__ with self parameter and use it to deduce class members.
/// Each deduced member will be treated as generic.
/// @example
///   ```@deduce
///      class Foo:
///        def __init__(self):
///          self.x, self.y = 1, 2```
///   will result in
///   ```class Foo[T1, T2]:
///        x: T1
///        y: T2```
/// @return the transformed init and the pointer to the original function.
std::pair<StmtPtr, FunctionStmt *>
TypecheckVisitor::autoDeduceMembers(ClassStmt *stmt, std::vector<Param> &args) {
  std::pair<StmtPtr, FunctionStmt *> init{nullptr, nullptr};
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getFunction()) {
      auto f = sp->getFunction();
      // todo)) do this
      // if (f->name == "__init__" && !f->args.empty() && f->args[0].name == "self") {
      //   // Set up deducedMembers that will be populated during AssignStmt evaluation
      //   ctx->getBase()->deducedMembers =
      //   std::make_shared<std::vector<std::string>>(); auto transformed =
      //   transform(sp);
      //   transformed->getFunction()->attributes.set(Attr::RealizeWithoutSelf);
      //   ctx->cache->functions[transformed->getFunction()->name].ast->attributes.set(
      //       Attr::RealizeWithoutSelf);
      //   int i = 0;
      //   // Once done, add arguments
      //   for (auto &m : *(ctx->getBase()->deducedMembers)) {
      //     auto varName = ctx->generateCanonicalName(format("T{}", ++i));
      //     auto memberName = ctx->cache->rev(varName);
      //     ctx->addType(memberName, varName, stmt->getSrcInfo())->generic = true;
      //     args.emplace_back(varName, N<IdExpr>("type"), nullptr, Param::Generic);
      //     args.emplace_back(m, N<IdExpr>(varName));
      //     ctx->cache->classes[canonicalName].fields.push_back(
      //         Cache::Class::ClassField{m, nullptr, canonicalName});
      //   }
      //   ctx->getBase()->deducedMembers = nullptr;
      //   return {transformed, f};
      // }
    }
  return {nullptr, nullptr};
}

/// Return a list of all statements within a given class suite.
/// Checks each suite recursively, and assumes that each statement is either
/// a function, a class or a docstring.
std::vector<StmtPtr> TypecheckVisitor::getClassMethods(const StmtPtr &s) {
  std::vector<StmtPtr> v;
  if (!s)
    return v;
  if (auto sp = s->getSuite()) {
    for (const auto &ss : sp->stmts)
      for (const auto &u : getClassMethods(ss))
        v.push_back(u);
  } else if (s->getExpr() && s->getExpr()->expr->getString()) {
    /// Those are doc-strings, ignore them.
  } else if (!s->getFunction() && !s->getClass()) {
    E(Error::CLASS_BAD_ATTR, s);
  } else {
    v.push_back(s);
  }
  return v;
}

/// Extract nested classes and transform them before the main class.
void TypecheckVisitor::transformNestedClasses(ClassStmt *stmt,
                                              std::vector<StmtPtr> &clsStmts,
                                              std::vector<StmtPtr> &varStmts,
                                              std::vector<StmtPtr> &fnStmts) {
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getClass()) {
      auto origName = sp->getClass()->name;
      // If class B is nested within A, it's name is always A.B, never B itself.
      // Ensure that parent class name is appended
      auto parentName = stmt->name;
      sp->getClass()->name = fmt::format("{}.{}", parentName, origName);
      auto tsp = transform(sp);
      std::string name;
      if (tsp->getSuite()) {
        for (auto &s : tsp->getSuite()->stmts)
          if (auto c = s->getClass()) {
            clsStmts.push_back(s);
            name = c->name;
          } else if (auto a = s->getAssign()) {
            varStmts.push_back(s);
          } else {
            fnStmts.push_back(s);
          }
        ctx->add(origName, ctx->forceFind(name));
      }
    }
}

/// Generate a magic method `__op__` for each magic `op`
/// described by @param typExpr and its arguments.
/// Currently generate:
/// @li Constructors: __new__, __init__
/// @li Utilities: __raw__, __hash__, __repr__, __tuplesize__, __add__, __mul__, __len__
/// @li Iteration: __iter__, __getitem__, __len__, __contains__
/// @li Comparisons: __eq__, __ne__, __lt__, __le__, __gt__, __ge__
/// @li Pickling: __pickle__, __unpickle__
/// @li Python: __to_py__, __from_py__
/// @li GPU: __to_gpu__, __from_gpu__, __from_gpu_new__
/// TODO: move to Codon as much as possible
StmtPtr TypecheckVisitor::codegenMagic(const std::string &op, const ExprPtr &typExpr,
                                       const std::vector<Param> &allArgs,
                                       bool isRecord) {
#define I(s) N<IdExpr>(s)
#define NS(x) N<DotExpr>(N<IdExpr>("__magic__"), (x))
  seqassert(typExpr, "typExpr is null");
  ExprPtr ret;
  std::vector<Param> fargs;
  std::vector<StmtPtr> stmts;
  Attr attr;
  attr.set("autogenerated");

  std::vector<Param> args;
  args.reserve(allArgs.size());
  for (auto &a : allArgs)
    args.push_back(a.clone());

  if (op == "new") {
    ret = typExpr->clone();
    if (isRecord) {
      // Tuples: def __new__() -> T (internal)
      for (auto &a : args)
        fargs.emplace_back(a.name, clone(a.type),
                           a.defaultValue ? clone(a.defaultValue)
                                          : N<CallExpr>(clone(a.type)));
      attr.set(Attr::Internal);
    } else {
      // Classes: def __new__() -> T
      stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), typExpr->clone())));
    }
  } else if (op == "init") {
    // Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> None:
    //            self.aI = aI ...
    ret = I("NoneType");
    fargs.emplace_back("self", typExpr->clone());
    for (auto &a : args) {
      stmts.push_back(N<AssignStmt>(N<DotExpr>(I("self"), a.name), I(a.name)));
      fargs.emplace_back(a.name, clone(a.type),
                         a.defaultValue ? clone(a.defaultValue)
                                        : N<CallExpr>(clone(a.type)));
    }
  } else if (op == "raw" || op == "dict") {
    // Classes: def __raw__(self: T)
    fargs.emplace_back("self", typExpr->clone());
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "tuplesize") {
    // def __tuplesize__() -> int
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op))));
  } else if (op == "getitem") {
    // Tuples: def __getitem__(self: T, index: int)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("index", I("int"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("index"))));
  } else if (op == "iter") {
    // Tuples: def __iter__(self: T)
    fargs.emplace_back("self", typExpr->clone());
    stmts.emplace_back(N<YieldFromStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "contains") {
    // Tuples: def __contains__(self: T, what) -> bool
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("what", nullptr);
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("what"))));
  } else if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" ||
             op == "ge") {
    // def __op__(self: T, obj: T) -> bool
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("obj", typExpr->clone());
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "hash" || op == "len") {
    // def __hash__(self: T) -> int
    fargs.emplace_back("self", typExpr->clone());
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "pickle") {
    // def __pickle__(self: T, dest: Ptr[byte])
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("dest", N<IndexExpr>(I("Ptr"), I("byte")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("dest"))));
  } else if (op == "unpickle" || op == "from_py") {
    // def __unpickle__(src: Ptr[byte]) -> T
    fargs.emplace_back("src", N<IndexExpr>(I("Ptr"), I("byte")));
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("src"), typExpr->clone())));
  } else if (op == "to_py") {
    // def __to_py__(self: T) -> Ptr[byte]
    fargs.emplace_back("self", typExpr->clone());
    ret = N<IndexExpr>(I("Ptr"), I("byte"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "to_gpu") {
    // def __to_gpu__(self: T, cache) -> T
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("cache");
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("cache"))));
  } else if (op == "from_gpu") {
    // def __from_gpu__(self: T, other: T)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("other", typExpr->clone());
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("other"))));
  } else if (op == "from_gpu_new") {
    // def __from_gpu_new__(other: T) -> T
    fargs.emplace_back("other", typExpr->clone());
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("other"))));
  } else if (op == "repr") {
    // def __repr__(self: T) -> str
    fargs.emplace_back("self", typExpr->clone());
    ret = I("str");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "add") {
    // def __add__(self, obj)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("obj", nullptr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "mul") {
    // def __mul__(self, i: Static[int])
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("i", N<IndexExpr>(I("Static"), I("int")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("i"))));
  } else {
    seqassert(false, "invalid magic {}", op);
  }
#undef I
#undef NS
  auto t = std::make_shared<FunctionStmt>(format("__{}__", op), ret, fargs,
                                          N<SuiteStmt>(stmts), attr);
  t->setSrcInfo(ctx->cache->generateSrcInfo());
  return t;
}

/// Generate a tuple class `Tuple.N[T1,...,TN]`.
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
      args.emplace_back(names[i], N<IdExpr>(format("T{}", i + 1)), nullptr);
    for (size_t i = 0; i < len; i++)
      args.emplace_back(format("T{}", i + 1), N<IdExpr>("type"), nullptr, true);
    StmtPtr stmt = N<ClassStmt>(ctx->cache->generateSrcInfo(), typeName, args, nullptr,
                                std::vector<ExprPtr>{N<IdExpr>("tuple")});

    // Add getItem for KwArgs:
    //   `def __getitem__(self, key: Static[str]): return getattr(self, key)`
    auto getItem = N<FunctionStmt>(
        "__getitem__", nullptr,
        std::vector<Param>{Param{"self"}, Param{"key", N<IndexExpr>(N<IdExpr>("Static"),
                                                                    N<IdExpr>("str"))}},
        N<SuiteStmt>(N<ReturnStmt>(
            N<CallExpr>(N<IdExpr>("getattr"), N<IdExpr>("self"), N<IdExpr>("key")))));
    if (startswith(typeName, TYPE_KWTUPLE))
      stmt->getClass()->suite = getItem;

    // Add getItem for KwArgs:
    //   `def __repr__(self,): return __magic__.repr_partial(self)`
    auto repr = N<FunctionStmt>(
        "__repr__", nullptr, std::vector<Param>{Param{"self"}},
        N<SuiteStmt>(N<ReturnStmt>(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("__magic__"), "repr_partial"), N<IdExpr>("self")))));
    if (startswith(typeName, TYPE_PARTIAL))
      stmt->getClass()->suite = repr;

    // Simplify in the standard library context and type check
    stmt = TypecheckVisitor::apply(ctx->cache->imports[STDLIB_IMPORT].ctx, stmt,
                                   FILE_GENERATED);
    prependStmts->push_back(stmt);
  }
  return typeName;
}

} // namespace codon::ast
