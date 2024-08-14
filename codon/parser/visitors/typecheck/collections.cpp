// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
    if (auto star = cast<StarExpr>(expr->items[ai])) {
      // Case: unpack star expressions (e.g., `*arg` -> `arg.item1, arg.item2, ...`)
      star->expr = transform(star->getExpr());
      auto typ = star->getExpr()->getClassType();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        star->expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), star->getExpr()));
        typ = star->getExpr()->getClassType();
      }
      if (!typ)
        return; // continue later when the type becomes known
      if (!typ->isRecord())
        E(Error::CALL_BAD_UNPACK, star, typ->prettyString());
      auto ff = getClassFields(typ.get());
      for (int i = 0; i < ff.size(); i++, ai++) {
        expr->items.insert(expr->items.begin() + ai,
                           transform(N<DotExpr>(star->getExpr(), ff[i].name)));
      }
      // Remove the star
      expr->items.erase(expr->items.begin() + ai);
      ai--;
    }
  generateTuple(expr->items.size());
  resultExpr = transform(N<CallExpr>(N<IdExpr>(TYPE_TUPLE), clone(expr->items)));
  unify(expr->getType(), resultExpr->getType());
}

/// Transform a list `[a1, ..., aN]` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(ListExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->getType("List")->getClass();
  if ((resultExpr = transformComprehension(name->name, "append", expr->items))) {
    resultExpr->setAttribute(Attr::ExprList);
  }
}

/// Transform a set `{a1, ..., aN}` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(SetExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->getType("Set")->getClass();
  if ((resultExpr = transformComprehension(name->name, "add", expr->items))) {
    resultExpr->setAttribute(Attr::ExprSet);
  }
}

/// Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement
/// expression. See @c transformComprehension
void TypecheckVisitor::visit(DictExpr *expr) {
  expr->setType(ctx->getUnbound());
  auto name = ctx->cache->imports[STDLIB_IMPORT].ctx->getType("Dict")->getClass();
  if ((resultExpr = transformComprehension(name->name, "__setitem__", expr->items))) {
    resultExpr->setAttribute(Attr::ExprDict);
  }
}

/// Transform a tuple generator expression.
/// @example
///   `tuple(expr for i in tuple_generator)` -> `Tuple.N.__new__(expr...)`
void TypecheckVisitor::visit(GeneratorExpr *expr) {
  // List comprehension optimization:
  // Use `iter.__len__()` when creating list if there is a single for loop
  // without any if conditions in the comprehension
  bool canOptimize =
      expr->kind == GeneratorExpr::ListGenerator && expr->loopCount() == 1;
  if (canOptimize) {
    auto iter = transform(clone(cast<ForStmt>(expr->getFinalSuite())->iter));
    auto ce = cast<CallExpr>(iter);
    IdExpr *id = nullptr;
    if (ce && (id = cast<IdExpr>(ce->getExpr()))) {
      // Turn off this optimization for static items
      canOptimize &= !startswith(id->value, "std.internal.types.range.staticrange");
      canOptimize &= !startswith(id->value, "statictuple");
    }
  }

  Expr *var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    // List comprehensions
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "append"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    auto noOptStmt =
        N<SuiteStmt>(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("List"))), suite);

    if (canOptimize) {
      auto optimizeVar = ctx->cache->getTemporaryVar("i");
      auto origIter = cast<ForStmt>(expr->getFinalSuite())->getIter();

      auto optStmt = clone(noOptStmt);
      cast<ForStmt>((*cast<SuiteStmt>(optStmt))[1])->iter = N<IdExpr>(optimizeVar);
      optStmt = N<SuiteStmt>(
          N<AssignStmt>(N<IdExpr>(optimizeVar), clone(origIter)),
          N<AssignStmt>(
              clone(var),
              N<CallExpr>(N<IdExpr>("List"),
                          N<CallExpr>(N<DotExpr>(N<IdExpr>(optimizeVar), "__len__")))),
          (*cast<SuiteStmt>(optStmt))[1]);
      resultExpr = N<IfExpr>(
          N<CallExpr>(N<IdExpr>("hasattr"), clone(origIter), N<StringExpr>("__len__")),
          N<StmtExpr>(optStmt, clone(var)), N<StmtExpr>(noOptStmt, var));
    } else {
      resultExpr = N<StmtExpr>(noOptStmt, var);
    }
    // ctx->addBlock();
    resultExpr = transform(resultExpr);
    // ctx->popBlock();
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    // Set comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")));
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "add"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    // ctx->addBlock();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
    // ctx->popBlock();
  } else if (expr->kind == GeneratorExpr::DictGenerator) {
    // Set comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")));
    expr->setFinalExpr(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                   N<StarExpr>(expr->getFinalExpr())));
    auto suite = expr->getFinalSuite();
    // ctx->addBlock();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
    // ctx->popBlock();
  } else if (expr->kind == GeneratorExpr::TupleGenerator) {
    seqassert(expr->loopCount() == 1, "invalid tuple generator");
    auto gen = transform(cast<ForStmt>(expr->getFinalSuite())->iter);
    if (!gen->getType()->canRealize())
      return; // Wait until the iterator can be realized

    auto block = N<SuiteStmt>();
    // `tuple = tuple_generator`
    auto tupleVar = ctx->cache->getTemporaryVar("tuple");
    block->addStmt(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

    auto forStmt = clone(cast<ForStmt>(expr->getFinalSuite()));
    seqassert(cast<IdExpr>(forStmt->var), "tuple() not simplified");
    auto finalExpr = expr->getFinalExpr();
    auto suiteVec =
        cast<StmtExpr>(finalExpr) ? cast<StmtExpr>(finalExpr)->stmts[0] : nullptr;
    auto [ok, delay, preamble, staticItems] = transformStaticLoopCall(
        cast<ForStmt>(expr->getFinalSuite())->var, &forStmt->suite,
        cast<ForStmt>(expr->getFinalSuite())->iter,
        [&](Stmt *wrap) { return N<StmtExpr>(clone(wrap), clone(finalExpr)); }, true);
    if (!ok)
      E(Error::CALL_BAD_ITER, gen, gen->getType()->prettyString());
    if (delay)
      return;

    std::vector<Expr *> tupleItems;
    for (auto &i : staticItems)
      tupleItems.push_back(cast<Expr>(i));
    if (preamble)
      block->addStmt(preamble);
    // ctx->addBlock();
    resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(tupleItems)));
    // ctx->popBlock();
  } else {
    // ctx->addBlock();
    expr->loops =
        transform(expr->getFinalSuite()); // assume: internal data will be changed
    // ctx->popBlock();
    unify(expr->getType(), ctx->instantiateGeneric(ctx->getType("Generator"),
                                              {expr->getFinalExpr()->getType()}));
    if (realize(expr->getType()))
      expr->setDone();
  }
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
Expr *TypecheckVisitor::transformComprehension(const std::string &type,
                                               const std::string &fn,
                                               std::vector<Expr *> &items) {
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
      return ctx->instantiateGeneric(ctx->getType("Optional"), {collectionCls})
          ->getClass();
    } else if (collectionCls->name == TYPE_OPTIONAL && ti->name != TYPE_OPTIONAL) {
      return ctx->instantiateGeneric(ctx->getType("Optional"), {ti})->getClass();
    } else if (!collectionCls->is("pyobj") && ti->is("pyobj")) {
      // Rule: anything derives from pyobj
      return ti;
    } else if (collectionCls->name != ti->name) {
      // Rule: subclass derives from superclass
      const auto &mros = ctx->cache->getClass(collectionCls)->mro;
      for (size_t i = 1; i < mros.size(); i++) {
        auto t = ctx->instantiate(mros[i], collectionCls);
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
  bool isDict = endswith(type, "Dict.0");
  for (auto &i : items) {
    ClassTypePtr typ = nullptr;
    if (!isDict && cast<StarExpr>(i)) {
      auto star = cast<StarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "__iter__")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = star->getExpr()->getClassType()->generics[0].type->getClass();
    } else if (isDict && cast<KeywordStarExpr>(i)) {
      auto star = cast<KeywordStarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "items")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = star->getExpr()->getClassType()->generics[0].type->getClass();
    } else {
      i = transform(i);
      typ = i->getClassType();
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
      auto tt = unify(typ, ctx->instantiate(generateTuple(2)))->getClass();
      seqassert(collectionTyp->getClass() &&
                    collectionTyp->getClass()->generics.size() == 2 &&
                    tt->generics.size() == 2,
                "bad dict");
      std::vector<types::TypePtr> nt;
      for (int di = 0; di < 2; di++) {
        nt.push_back(collectionTyp->getClass()->generics[di].type);
        if (!nt[di]->getClass())
          unify(nt[di], tt->generics[di].type);
        else if (auto dt =
                     superTyp(nt[di]->getClass(), tt->generics[di].type->getClass()))
          nt[di] = dt;
      }
      collectionTyp = ctx->instantiateGeneric(generateTuple(nt.size()), nt);
    }
  }
  if (!done)
    return nullptr;
  std::vector<Stmt *> stmts;
  Expr *var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));

  std::vector<Expr *> constructorArgs{};
  if (endswith(type, "List") && !items.empty()) {
    // Optimization: pre-allocate the list with the exact number of elements
    constructorArgs.push_back(N<IntExpr>(items.size()));
  }
  auto t = N<IdExpr>(type);
  if (isDict && collectionTyp->getClass()) {
    seqassert(collectionTyp->getClass()->isRecord(), "bad dict");
    std::vector<types::TypePtr> nt;
    for (auto &g : collectionTyp->getClass()->generics)
      nt.push_back(g.type);
    t->setType(ctx->instantiateGeneric(
        ctx->getType("type"), {ctx->instantiateGeneric(ctx->getType(type), nt)}));
  } else if (isDict) {
    t->setType(ctx->instantiateGeneric(ctx->getType("type"),
                                       {ctx->instantiate(ctx->getType(type))}));
  } else {
    t->setType(ctx->instantiateGeneric(
        ctx->getType("type"),
        {ctx->instantiateGeneric(ctx->getType(type), {collectionTyp})}));
  }
  stmts.push_back(
      transform(N<AssignStmt>(clone(var), N<CallExpr>(t, constructorArgs))));
  for (const auto &it : items) {
    if (!isDict && cast<StarExpr>(it)) {
      // Unpack star-expression by iterating over it
      // `*star` -> `for i in star: cont.[fn](i)`
      auto star = cast<StarExpr>(it);
      Expr *forVar = N<IdExpr>(ctx->cache->getTemporaryVar("i"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar))))));
    } else if (isDict && cast<KeywordStarExpr>(it)) {
      // Expand kwstar-expression by iterating over it: see the example above
      auto star = cast<KeywordStarExpr>(it);
      Expr *forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1)))))));
    } else {
      it->setAttribute(Attr::ExprSequenceItem);
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

} // namespace codon::ast
