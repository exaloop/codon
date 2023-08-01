// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Transform tuples.
/// Generate tuple classes (e.g., `Tuple.N`) if not available.
/// @example
///   `(a1, ..., aN)` -> `Tuple.N.__new__(a1, ..., aN)`
void TypecheckVisitor::visit(TupleExpr *expr) {
  expr->setType(ctx->getUnbound());
  for (int ai = 0; ai < expr->items.size(); ai++)
    if (auto star = expr->items[ai]->getStar()) {
      // Case: unpack star expressions (e.g., `*arg` -> `arg.item1, arg.item2, ...`)
      transform(star->what);
      auto typ = star->what->type->getClass();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        star->what = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), star->what));
        typ = star->what->type->getClass();
      }
      if (!typ)
        return; // continue later when the type becomes known
      if (!typ->getRecord())
        E(Error::CALL_BAD_UNPACK, star, typ->prettyString());
      auto &ff = ctx->cache->classes[typ->name].fields;
      for (int i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
        expr->items.insert(expr->items.begin() + ai,
                           transform(N<DotExpr>(clone(star->what), ff[i].name)));
      }
      // Remove the star
      expr->items.erase(expr->items.begin() + ai);
      ai--;
    } else {
      expr->items[ai] = transform(expr->items[ai]);
    }
  auto tupleName = generateTuple(expr->items.size());
  resultExpr = transform(N<CallExpr>(N<DotExpr>(tupleName, "__new__"), expr->items));
  unify(expr->type, resultExpr->type);
}

/// Transform a list `[a1, ..., aN]` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(ListExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->forceFind("List");
  if ((resultExpr =
           transformComprehension(name->canonicalName, "append", expr->items))) {
    resultExpr->setAttr(ExprAttr::List);
  }
}

/// Transform a set `{a1, ..., aN}` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(SetExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->forceFind("Set");
  if ((resultExpr = transformComprehension(name->canonicalName, "add", expr->items))) {
    resultExpr->setAttr(ExprAttr::Set);
  }
}

/// Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement
/// expression. See @c transformComprehension
void TypecheckVisitor::visit(DictExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->forceFind("Dict");
  if ((resultExpr =
           transformComprehension(name->canonicalName, "__setitem__", expr->items))) {
    resultExpr->setAttr(ExprAttr::Dict);
  }
}

/// Transform a tuple generator expression.
/// @example
///   `tuple(expr for i in tuple_generator)` -> `Tuple.N.__new__(expr...)`
void TypecheckVisitor::visit(GeneratorExpr *expr) {
  std::vector<StmtPtr> stmts;

  auto loops = clone_nop(expr->loops); // Clone as loops will be modified

  // List comprehension optimization:
  // Use `iter.__len__()` when creating list if there is a single for loop
  // without any if conditions in the comprehension
  bool canOptimize = expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
                     loops[0].conds.empty();
  if (canOptimize) {
    auto iter = transform(loops[0].gen);
    IdExpr *id = nullptr;
    if (iter->getCall() && (id = iter->getCall()->expr->getId())) {
      // Turn off this optimization for static items
      canOptimize &= !startswith(id->value, "std.internal.types.range.staticrange");
      canOptimize &= !startswith(id->value, "statictuple");
    }
  }

  SuiteStmt *prev = nullptr;
  auto suite = transformGeneratorBody(loops, prev);
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    // List comprehensions
    std::vector<ExprPtr> args;
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(expr->expr))));
    auto noOptStmt =
        N<SuiteStmt>(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("List"))), suite);
    if (canOptimize) {
      seqassert(suite->getSuite() && !suite->getSuite()->stmts.empty() &&
                    CAST(suite->getSuite()->stmts[0], ForStmt),
                "bad comprehension transformation");
      auto optimizeVar = ctx->cache->getTemporaryVar("i");
      auto optSuite = clone(suite);
      CAST(optSuite->getSuite()->stmts[0], ForStmt)->iter = N<IdExpr>(optimizeVar);

      auto optStmt = N<SuiteStmt>(
          N<AssignStmt>(N<IdExpr>(optimizeVar), clone(expr->loops[0].gen)),
          N<AssignStmt>(
              clone(var),
              N<CallExpr>(N<IdExpr>("List"),
                          N<CallExpr>(N<DotExpr>(N<IdExpr>(optimizeVar), "__len__")))),
          optSuite);
      resultExpr = transform(
          N<IfExpr>(N<CallExpr>(N<IdExpr>("hasattr"), clone(expr->loops[0].gen),
                                N<StringExpr>("__len__")),
                    N<StmtExpr>(optStmt, clone(var)), N<StmtExpr>(noOptStmt, var)));
    } else {
      resultExpr = transform(N<StmtExpr>(noOptStmt, var));
    }
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    // Set comprehensions
    stmts.push_back(
        transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")))));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
  } else if (expr->kind == GeneratorExpr::TupleGenerator) {
    seqassert(expr->loops.size() == 1 && expr->loops[0].conds.empty(),
              "invalid tuple generator");
    unify(expr->type, ctx->getUnbound());
    auto gen = transform(expr->loops[0].gen);
    if (!gen->type->canRealize())
      return; // Wait until the iterator can be realized

    auto block = N<SuiteStmt>();
    // `tuple = tuple_generator`
    auto tupleVar = ctx->cache->getTemporaryVar("tuple");
    block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

    seqassert(expr->loops[0].vars->getId(), "tuple() not simplified");
    std::vector<std::string> vars{expr->loops[0].vars->getId()->value};
    auto suiteVec = expr->expr->getStmtExpr()
                        ? expr->expr->getStmtExpr()->stmts[0]->getSuite()
                        : nullptr;
    auto oldSuite = suiteVec ? suiteVec->clone() : nullptr;
    for (int validI = 0; suiteVec && validI < suiteVec->stmts.size(); validI++) {
      if (auto a = suiteVec->stmts[validI]->getAssign())
        if (a->rhs && a->rhs->getIndex())
          if (a->rhs->getIndex()->expr->isId(vars[0])) {
            vars.push_back(a->lhs->getId()->value);
            suiteVec->stmts[validI] = nullptr;
            continue;
          }
      break;
    }
    if (vars.size() > 1)
      vars.erase(vars.begin());
    auto [ok, staticItems] =
        transformStaticLoopCall(vars, expr->loops[0].gen, [&](const StmtPtr &wrap) {
          return N<StmtExpr>(wrap, clone(expr->expr));
        });
    if (ok) {
      std::vector<ExprPtr> tupleItems;
      for (auto &i : staticItems)
        tupleItems.push_back(std::dynamic_pointer_cast<Expr>(i));
      resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(tupleItems)));
      return;
    } else if (oldSuite) {
      expr->expr->getStmtExpr()->stmts[0] = oldSuite;
    }

    auto tuple = gen->type->getRecord();
    if (!tuple || !startswith(tuple->name, TYPE_TUPLE))
      E(Error::CALL_BAD_ITER, gen, gen->type->prettyString());

    // `a := tuple[i]; expr...` for each i
    std::vector<ExprPtr> items;
    items.reserve(tuple->args.size());
    for (int ai = 0; ai < tuple->args.size(); ai++) {
      items.emplace_back(
          N<StmtExpr>(N<AssignStmt>(clone(expr->loops[0].vars),
                                    N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))),
                      clone(expr->expr)));
    }

    // `((a := tuple[0]; expr), (a := tuple[1]; expr), ...)`
    resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(items)));
  } else {
    // Generators: converted to lambda functions that yield the target expression
    prev->stmts.push_back(N<YieldStmt>(clone(expr->expr)));
    stmts.push_back(suite);

    auto anon = makeAnonFn(stmts);
    if (auto call = anon->getCall()) {
      seqassert(!call->args.empty() && call->args.back().value->getEllipsis(),
                "bad lambda: {}", *call);
      call->args.pop_back();
    } else {
      anon = N<CallExpr>(anon);
    }
    resultExpr = anon;
  }
}

/// Transform a dictionary comprehension to the corresponding statement expression.
/// @example
///   `{i+a: j+1 for i in j if a}` -> ```gen = Dict()
///                                      for i in j: if a: gen.__setitem__(i+a, j+1)```
void TypecheckVisitor::visit(DictGeneratorExpr *expr) {
  SuiteStmt *prev = nullptr;
  auto suite = transformGeneratorBody(expr->loops, prev);

  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  prev->stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                                clone(expr->key), clone(expr->expr))));
  stmts.push_back(transform(suite));
  resultExpr = N<StmtExpr>(stmts, transform(var));
}

/// Transform a collection of type `type` to a statement expression:
///   `[a1, ..., aN]` -> `cont = [type](); (cont.[fn](a1); ...); cont`
/// Any star-expression within the collection will be expanded:
///   `[a, *b]` -> `cont.[fn](a); for i in b: cont.[fn](i)`.
/// @example
///   `[a, *b, c]`  -> ```cont = List(3)
///                       cont.append(a)
///                       for i in b: cont.append(i)
///                       cont.append(c)```
///   `{a, *b, c}`  -> ```cont = Set()
///                       cont.add(a)
///                       for i in b: cont.add(i)
///                       cont.add(c)```
///   `{a: 1, **d}` -> ```cont = Dict()
///                       cont.__setitem__((a, 1))
///                       for i in b.items(): cont.__setitem__((i[0], i[i]))```
ExprPtr TypecheckVisitor::transformComprehension(const std::string &type,
                                                 const std::string &fn,
                                                 std::vector<ExprPtr> &items) {
  // Deduce the super type of the collection--- in other words, the least common
  // ancestor of all types in the collection. For example, `type([1, 1.2]) == type([1.2,
  // 1]) == float` because float is an "ancestor" of int.
  auto superTyp = [&](const ClassTypePtr &collectionCls,
                      const ClassTypePtr &ti) -> ClassTypePtr {
    if (!collectionCls)
      return ti;
    if (collectionCls->is("int") && ti->is("float")) {
      // Rule: int derives from float
      return ti;
    } else if (collectionCls->name != TYPE_OPTIONAL && ti->name == TYPE_OPTIONAL) {
      // Rule: T derives from Optional[T]
      return ctx->instantiateGeneric(ctx->forceFind("Optional")->type, {collectionCls})
          ->getClass();
    } else if (!collectionCls->is("pyobj") && ti->is("pyobj")) {
      // Rule: anything derives from pyobj
      return ti;
    } else if (collectionCls->name != ti->name) {
      // Rule: subclass derives from superclass
      auto &mros = ctx->cache->classes[collectionCls->name].mro;
      for (size_t i = 1; i < mros.size(); i++) {
        auto t = ctx->instantiate(mros[i]->type, collectionCls);
        if (t->unify(ti.get(), nullptr) >= 0) {
          return ti;
          break;
        }
      }
    }
    return nullptr;
  };

  TypePtr collectionTyp = ctx->getUnbound();
  bool done = true;
  bool isDict = endswith(type, "Dict");
  for (auto &i : items) {
    ClassTypePtr typ = nullptr;
    if (!isDict && i->getStar()) {
      auto star = i->getStar();
      star->what = transform(N<CallExpr>(N<DotExpr>(star->what, "__iter__")));
      if (star->what->type->is("Generator"))
        typ = star->what->type->getClass()->generics[0].type->getClass();
    } else if (isDict && CAST(i, KeywordStarExpr)) {
      auto star = CAST(i, KeywordStarExpr);
      star->what = transform(N<CallExpr>(N<DotExpr>(star->what, "items")));
      if (star->what->type->is("Generator"))
        typ = star->what->type->getClass()->generics[0].type->getClass();
    } else {
      i = transform(i);
      typ = i->type->getClass();
    }
    if (!typ) {
      done = false;
      continue;
    }
    if (!collectionTyp->getClass()) {
      unify(collectionTyp, typ);
    } else if (!isDict) {
      if (auto t = superTyp(collectionTyp->getClass(), typ))
        collectionTyp = t;
    } else {
      seqassert(collectionTyp->getRecord() &&
                    collectionTyp->getRecord()->args.size() == 2,
                "bad dict");
      auto tname = generateTuple(2);
      auto tt = unify(typ, ctx->instantiate(ctx->forceFind(tname)->type))->getRecord();
      auto nt = collectionTyp->getRecord()->args;
      for (int di = 0; di < 2; di++) {
        if (!nt[di]->getClass())
          unify(nt[di], tt->args[di]);
        else if (auto dt = superTyp(nt[di]->getClass(), tt->args[di]->getClass()))
          nt[di] = dt;
      }
      collectionTyp = ctx->instantiateGeneric(ctx->forceFind(tname)->type, nt);
    }
  }
  if (!done)
    return nullptr;
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));

  std::vector<ExprPtr> constructorArgs{};
  if (endswith(type, "List") && !items.empty()) {
    // Optimization: pre-allocate the list with the exact number of elements
    constructorArgs.push_back(N<IntExpr>(items.size()));
  }
  auto t = NT<IdExpr>(type);
  if (isDict && collectionTyp->getRecord()) {
    t->setType(ctx->instantiateGeneric(ctx->forceFind(type)->type,
                                       collectionTyp->getRecord()->args));
  } else if (isDict) {
    t->setType(ctx->instantiate(ctx->forceFind(type)->type));
  } else {
    t->setType(ctx->instantiateGeneric(ctx->forceFind(type)->type, {collectionTyp}));
  }
  stmts.push_back(
      transform(N<AssignStmt>(clone(var), N<CallExpr>(t, constructorArgs))));
  for (const auto &it : items) {
    if (!isDict && it->getStar()) {
      // Unpack star-expression by iterating over it
      // `*star` -> `for i in star: cont.[fn](i)`
      auto star = it->getStar();
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("i"));
      star->what->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->what,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar))))));
    } else if (isDict && CAST(it, KeywordStarExpr)) {
      // Expand kwstar-expression by iterating over it: see the example above
      auto star = CAST(it, KeywordStarExpr);
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      star->what->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->what,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1)))))));
    } else {
      it->setAttr(ExprAttr::SequenceItem);
      if (isDict) {
        stmts.push_back(transform(N<ExprStmt>(
            N<CallExpr>(N<DotExpr>(clone(var), fn), N<IndexExpr>(it, N<IntExpr>(0)),
                        N<IndexExpr>(it, N<IntExpr>(1))))));
      } else {
        stmts.push_back(
            transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), it))));
      }
    }
  }
  return transform(N<StmtExpr>(stmts, var));
}

/// Transforms a list of @c GeneratorBody loops to the corresponding set of for loops.
/// @example
///   `for i in j if a for k in i if a if b` ->
///   `for i in j: if a: for k in i: if a: if b: [prev]`
/// @param prev (out-argument): A pointer to the innermost block (suite) where the
///                             comprehension (or generator) expression should reside
StmtPtr
TypecheckVisitor::transformGeneratorBody(const std::vector<GeneratorBody> &loops,
                                         SuiteStmt *&prev) {
  StmtPtr suite = N<SuiteStmt>(), newSuite = nullptr;
  prev = dynamic_cast<SuiteStmt *>(suite.get());
  for (auto &l : loops) {
    newSuite = N<SuiteStmt>();
    auto nextPrev = dynamic_cast<SuiteStmt *>(newSuite.get());

    auto forStmt = N<ForStmt>(l.vars->clone(), l.gen->clone(), newSuite);
    prev->stmts.push_back(forStmt);
    prev = nextPrev;
    for (auto &cond : l.conds) {
      newSuite = N<SuiteStmt>();
      nextPrev = dynamic_cast<SuiteStmt *>(newSuite.get());
      prev->stmts.push_back(N<IfStmt>(cond->clone(), newSuite));
      prev = nextPrev;
    }
  }
  return suite;
}

} // namespace codon::ast
