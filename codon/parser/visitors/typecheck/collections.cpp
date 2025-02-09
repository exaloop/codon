// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Transform tuples.
/// @example
///   `(a1, ..., aN)` -> `Tuple.__new__(a1, ..., aN)`
void TypecheckVisitor::visit(TupleExpr *expr) {
  resultExpr =
      transform(N<CallExpr>(N<DotExpr>(N<IdExpr>(TYPE_TUPLE), "__new__"), expr->items));
}

/// Transform a list `[a1, ..., aN]` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(ListExpr *expr) {
  expr->setType(instantiateUnbound());
  auto name = getStdLibType("List")->name;
  if ((resultExpr = transformComprehension(name, "append", expr->items))) {
    resultExpr->setAttribute(Attr::ExprList);
  }
}

/// Transform a set `{a1, ..., aN}` to the corresponding statement expression.
/// See @c transformComprehension
void TypecheckVisitor::visit(SetExpr *expr) {
  expr->setType(instantiateUnbound());
  auto name = getStdLibType("Set")->name;
  if ((resultExpr = transformComprehension(name, "add", expr->items))) {
    resultExpr->setAttribute(Attr::ExprSet);
  }
}

/// Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement
/// expression. See @c transformComprehension
void TypecheckVisitor::visit(DictExpr *expr) {
  expr->setType(instantiateUnbound());
  auto name = getStdLibType("Dict")->name;
  if ((resultExpr = transformComprehension(name, "__setitem__", expr->items))) {
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
    auto iter = transform(clone(cast<ForStmt>(expr->getFinalSuite())->getIter()));
    auto ce = cast<CallExpr>(iter);
    IdExpr *id = nullptr;
    if (ce && (id = cast<IdExpr>(ce->getExpr()))) {
      // Turn off this optimization for static items
      canOptimize &=
          !startswith(id->getValue(), "std.internal.types.range.staticrange");
      canOptimize &= !startswith(id->getValue(), "statictuple");
    }
  }

  Expr *var = N<IdExpr>(getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    // List comprehensions
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "append"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    auto noOptStmt =
        N<SuiteStmt>(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("List"))), suite);

    if (canOptimize) {
      auto optimizeVar = getTemporaryVar("i");
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
    resultExpr = transform(resultExpr);
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    // Set comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")));
    expr->setFinalExpr(
        N<CallExpr>(N<DotExpr>(clone(var), "add"), expr->getFinalExpr()));
    auto suite = expr->getFinalSuite();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
  } else if (expr->kind == GeneratorExpr::DictGenerator) {
    // Dictionary comprehensions
    auto head = N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")));
    expr->setFinalExpr(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                   N<StarExpr>(expr->getFinalExpr())));
    auto suite = expr->getFinalSuite();
    resultExpr = transform(N<StmtExpr>(N<SuiteStmt>(head, suite), var));
  } else if (expr->kind == GeneratorExpr::TupleGenerator) {
    seqassert(expr->loopCount() == 1, "invalid tuple generator");
    auto gen = transform(cast<ForStmt>(expr->getFinalSuite())->getIter());
    if (!gen->getType()->canRealize())
      return; // Wait until the iterator can be realized

    auto block = N<SuiteStmt>();
    // `tuple = tuple_generator`
    auto tupleVar = getTemporaryVar("tuple");
    block->addStmt(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

    auto forStmt = clone(cast<ForStmt>(expr->getFinalSuite()));
    auto finalExpr = expr->getFinalExpr();
    auto [ok, delay, preamble, staticItems] = transformStaticLoopCall(
        cast<ForStmt>(expr->getFinalSuite())->getVar(), &forStmt->suite, gen,
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
    resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(tupleItems)));
  } else {
    expr->loops =
        transform(expr->getFinalSuite()); // assume: internal data will be changed
    unify(expr->getType(), instantiateType(getStdLibType("Generator"),
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
  auto superTyp = [&](ClassType *collectionCls, ClassType *ti) -> TypePtr {
    if (!collectionCls)
      return ti->shared_from_this();
    if (collectionCls->is("int") && ti->is("float")) {
      // Rule: int derives from float
      return ti->shared_from_this();
    } else if (collectionCls->name != TYPE_OPTIONAL && ti->name == TYPE_OPTIONAL) {
      // Rule: T derives from Optional[T]
      return instantiateType(getStdLibType("Optional"),
                             std::vector<types::Type *>{collectionCls});
    } else if (collectionCls->name == TYPE_OPTIONAL && ti->name != TYPE_OPTIONAL) {
      return instantiateType(getStdLibType("Optional"), std::vector<types::Type *>{ti});
    } else if (!collectionCls->is("pyobj") && ti->is("pyobj")) {
      // Rule: anything derives from pyobj
      return ti->shared_from_this();
    } else if (collectionCls->name != ti->name) {
      // Rule: subclass derives from superclass
      const auto &mros = getClass(collectionCls)->mro;
      for (size_t i = 1; i < mros.size(); i++) {
        auto t = instantiateType(mros[i].get(), collectionCls);
        if (t->unify(ti, nullptr) >= 0) {
          return ti->shared_from_this();
          break;
        }
      }
    }
    return nullptr;
  };

  TypePtr collectionTyp = instantiateUnbound();
  bool done = true;
  bool isDict = type == getStdLibType("Dict")->name;
  for (auto &i : items) {
    ClassType *typ = nullptr;
    if (!isDict && cast<StarExpr>(i)) {
      auto star = cast<StarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "__iter__")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = extractClassGeneric(star->getExpr()->getType())->getClass();
    } else if (isDict && cast<KeywordStarExpr>(i)) {
      auto star = cast<KeywordStarExpr>(i);
      star->expr = transform(N<CallExpr>(N<DotExpr>(star->getExpr(), "items")));
      if (star->getExpr()->getType()->is("Generator"))
        typ = extractClassGeneric(star->getExpr()->getType())->getClass();
    } else {
      i = transform(i);
      typ = i->getClassType();
    }
    if (!typ) {
      done = false;
      continue;
    }
    if (!collectionTyp->getClass()) {
      unify(collectionTyp.get(), typ);
    } else if (!isDict) {
      if (auto t = superTyp(collectionTyp->getClass(), typ))
        collectionTyp = t;
    } else {
      auto tt = unify(typ, instantiateType(generateTuple(2)))->getClass();
      seqassert(collectionTyp->getClass() &&
                    collectionTyp->getClass()->generics.size() == 2 &&
                    tt->generics.size() == 2,
                "bad dict");
      std::vector<types::TypePtr> nt;
      for (int di = 0; di < 2; di++) {
        nt.push_back(extractClassGeneric(collectionTyp.get(), di)->shared_from_this());
        if (!nt[di]->getClass())
          unify(nt[di].get(), extractClassGeneric(tt, di));
        else if (auto dt = superTyp(nt[di]->getClass(),
                                    extractClassGeneric(tt, di)->getClass()))
          nt[di] = dt;
      }
      collectionTyp =
          instantiateType(generateTuple(nt.size()), ctx->cache->castVectorPtr(nt));
    }
  }
  if (!done)
    return nullptr;
  std::vector<Stmt *> stmts;
  Expr *var = N<IdExpr>(getTemporaryVar("cont"));

  std::vector<Expr *> constructorArgs{};
  if (type == getStdLibType("List")->name && !items.empty()) {
    // Optimization: pre-allocate the list with the exact number of elements
    constructorArgs.push_back(N<IntExpr>(items.size()));
  }
  auto t = N<IdExpr>(type);
  auto ta = instantiateType(getStdLibType(type));
  if (isDict && collectionTyp->getClass()) {
    seqassert(collectionTyp->getClass()->isRecord(), "bad dict");
    std::vector<types::Type *> nt;
    for (auto &g : collectionTyp->getClass()->generics)
      nt.push_back(g.getType());
    ta = instantiateType(getStdLibType(type), nt);
  } else if (!isDict) {
    ta = instantiateType(getStdLibType(type), {collectionTyp.get()});
  }
  t->setType(instantiateTypeVar(ta.get()));
  stmts.push_back(N<AssignStmt>(clone(var), N<CallExpr>(t, constructorArgs)));
  for (const auto &it : items) {
    if (!isDict && cast<StarExpr>(it)) {
      // Unpack star-expression by iterating over it
      // `*star` -> `for i in star: cont.[fn](i)`
      auto star = cast<StarExpr>(it);
      Expr *forVar = N<IdExpr>(getTemporaryVar("i"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar)))));
    } else if (isDict && cast<KeywordStarExpr>(it)) {
      // Expand kwstar-expression by iterating over it: see the example above
      auto star = cast<KeywordStarExpr>(it);
      Expr *forVar = N<IdExpr>(getTemporaryVar("it"));
      star->getExpr()->setAttribute(Attr::ExprStarSequenceItem);
      stmts.push_back(N<ForStmt>(
          clone(forVar), star->getExpr(),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1))))));
    } else {
      it->setAttribute(Attr::ExprSequenceItem);
      if (isDict) {
        stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn),
                                                N<IndexExpr>(it, N<IntExpr>(0)),
                                                N<IndexExpr>(it, N<IntExpr>(1)))));
      } else {
        stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), it)));
      }
    }
  }
  return transform(N<StmtExpr>(stmts, var));
}

} // namespace codon::ast
