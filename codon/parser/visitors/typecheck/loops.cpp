// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;

/// Nothing to typecheck; just call setDone
void TypecheckVisitor::visit(BreakStmt *stmt) {
  stmt->setDone();
  if (!ctx->staticLoops.back().empty()) {
    auto a = N<AssignStmt>(N<IdExpr>(ctx->staticLoops.back()), N<BoolExpr>(false));
    a->setUpdate();
    resultStmt = transform(N<SuiteStmt>(a, stmt->clone()));
  }
}

/// Nothing to typecheck; just call setDone
void TypecheckVisitor::visit(ContinueStmt *stmt) {
  stmt->setDone();
  if (!ctx->staticLoops.back().empty()) {
    resultStmt = N<BreakStmt>();
    resultStmt->setDone();
  }
}

/// Typecheck while statements.
void TypecheckVisitor::visit(WhileStmt *stmt) {
  ctx->staticLoops.push_back(stmt->gotoVar.empty() ? "" : stmt->gotoVar);
  transform(stmt->cond);
  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  if (stmt->cond->isDone() && stmt->suite->isDone())
    stmt->setDone();
}

/// Typecheck for statements. Wrap the iterator expression with `__iter__` if needed.
/// See @c transformHeterogenousTupleFor for iterating heterogenous tuples.
void TypecheckVisitor::visit(ForStmt *stmt) {
  transform(stmt->decorator);
  transform(stmt->iter);

  // Extract the iterator type of the for
  auto iterType = stmt->iter->getType()->getClass();
  if (!iterType)
    return; // wait until the iterator is known

  if ((resultStmt = transformStaticForLoop(stmt)))
    return;

  bool maybeHeterogenous = startswith(iterType->name, TYPE_TUPLE) ||
                           startswith(iterType->name, TYPE_KWTUPLE);
  if (maybeHeterogenous && !iterType->canRealize()) {
    return; // wait until the tuple is fully realizable
  } else if (maybeHeterogenous && iterType->getHeterogenousTuple()) {
    // Case: iterating a heterogenous tuple
    resultStmt = transformHeterogenousTupleFor(stmt);
    return;
  }

  // Case: iterating a non-generator. Wrap with `__iter__`
  if (iterType->name != "Generator" && !stmt->wrapped) {
    stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->iter, "__iter__")));
    iterType = stmt->iter->getType()->getClass();
    stmt->wrapped = true;
  }

  auto var = stmt->var->getId();
  seqassert(var, "corrupt for variable: {}", stmt->var);

  // Handle dominated for bindings
  auto changed = in(ctx->cache->replacements, var->value);
  while (auto s = in(ctx->cache->replacements, var->value))
    var->value = s->first, changed = s;
  if (changed && changed->second) {
    auto u =
        N<AssignStmt>(N<IdExpr>(format("{}.__used__", var->value)), N<BoolExpr>(true));
    u->setUpdate();
    stmt->suite = N<SuiteStmt>(u, stmt->suite);
  }
  if (changed)
    var->setAttr(ExprAttr::Dominated);

  // Unify iterator variable and the iterator type
  auto val = ctx->find(var->value);
  if (!changed)
    val = ctx->add(TypecheckItem::Var, var->value,
                   ctx->getUnbound(stmt->var->getSrcInfo()));
  if (iterType && iterType->name != "Generator")
    E(Error::EXPECTED_GENERATOR, stmt->iter);
  unify(stmt->var->type,
        iterType ? unify(val->type, iterType->generics[0].type) : val->type);

  ctx->staticLoops.push_back("");
  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  if (stmt->iter->isDone() && stmt->suite->isDone())
    stmt->setDone();
}

/// Handle heterogeneous tuple iteration.
/// @example
///   `for i in tuple_expr: <suite>` ->
///   ```tuple = tuple_expr
///      for cnt in range(<tuple length>):
///        if cnt == 0:
///          i = t[0]; <suite>
///        if cnt == 1:
///          i = t[1]; <suite> ...```
/// A separate suite is generated  for each tuple member.
StmtPtr TypecheckVisitor::transformHeterogenousTupleFor(ForStmt *stmt) {
  auto block = N<SuiteStmt>();
  // `tuple = <tuple expression>`
  auto tupleVar = ctx->cache->getTemporaryVar("tuple");
  block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), stmt->iter));

  auto tupleArgs = stmt->iter->getType()->getClass()->getHeterogenousTuple()->args;
  auto cntVar = ctx->cache->getTemporaryVar("idx");
  std::vector<StmtPtr> forBlock;
  for (size_t ai = 0; ai < tupleArgs.size(); ai++) {
    // `if cnt == ai: (var = tuple[ai]; <suite>)`
    forBlock.push_back(N<IfStmt>(
        N<BinaryExpr>(N<IdExpr>(cntVar), "==", N<IntExpr>(ai)),
        N<SuiteStmt>(N<AssignStmt>(clone(stmt->var),
                                   N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))),
                     clone(stmt->suite))));
  }
  // `for cnt in range(tuple_size): ...`
  block->stmts.push_back(
      N<ForStmt>(N<IdExpr>(cntVar),
                 N<CallExpr>(N<IdExpr>("std.internal.types.range.range"),
                             N<IntExpr>(tupleArgs.size())),
                 N<SuiteStmt>(forBlock)));

  ctx->blockLevel++;
  transform(block);
  ctx->blockLevel--;

  return block;
}

/// Handle static for constructs.
/// @example
///   `for i in statictuple(1, x): <suite>` ->
///   ```loop = True
///      while loop:
///        while loop:
///          i: Static[int] = 1; <suite>; break
///        while loop:
///          i = x; <suite>; break
///        loop = False   # also set to False on break
/// A separate suite is generated for each static iteration.
StmtPtr TypecheckVisitor::transformStaticForLoop(ForStmt *stmt) {
  auto loopVar = ctx->cache->getTemporaryVar("loop");
  auto fn = [&](const std::string &var, const ExprPtr &expr) {
    auto brk = N<BreakStmt>();
    brk->setDone(); // Avoid transforming this one to continue
    // var [: Static] := expr; suite...
    auto loop = N<WhileStmt>(
        N<IdExpr>(loopVar),
        N<SuiteStmt>(
            expr ? N<AssignStmt>(N<IdExpr>(var), expr->clone(),
                                 expr->isStatic()
                                     ? NT<IndexExpr>(N<IdExpr>("Static"),
                                                     N<IdExpr>(expr->staticValue.type ==
                                                                       StaticValue::INT
                                                                   ? "int"
                                                                   : "str"))
                                     : nullptr)
                 : nullptr,
            clone(stmt->suite), brk));
    loop->gotoVar = loopVar;
    return loop;
  };

  auto var = stmt->var->getId()->value;
  if (!stmt->iter->getCall() || !stmt->iter->getCall()->expr->getId())
    return nullptr;
  auto iter = stmt->iter->getCall()->expr->getId();
  auto block = N<SuiteStmt>();
  if (iter && startswith(iter->value, "statictuple:0")) {
    auto &args = stmt->iter->getCall()->args[0].value->getCall()->args;
    for (size_t i = 0; i < args.size(); i++)
      block->stmts.push_back(fn(var, args[i].value));
  } else if (iter &&
             startswith(iter->value, "std.internal.types.range.staticrange:0")) {
    int st =
        iter->type->getFunc()->funcGenerics[0].type->getStatic()->evaluate().getInt();
    int ed =
        iter->type->getFunc()->funcGenerics[1].type->getStatic()->evaluate().getInt();
    int step =
        iter->type->getFunc()->funcGenerics[2].type->getStatic()->evaluate().getInt();
    if (abs(st - ed) / abs(step) > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, iter, MAX_STATIC_ITER, abs(st - ed) / abs(step));
    for (int i = st; step > 0 ? i < ed : i > ed; i += step)
      block->stmts.push_back(fn(var, N<IntExpr>(i)));
  } else if (iter &&
             startswith(iter->value, "std.internal.types.range.staticrange:1")) {
    int ed =
        iter->type->getFunc()->funcGenerics[0].type->getStatic()->evaluate().getInt();
    if (ed > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, iter, MAX_STATIC_ITER, ed);
    for (int i = 0; i < ed; i++)
      block->stmts.push_back(fn(var, N<IntExpr>(i)));
  } else if (iter && startswith(iter->value, "std.internal.static.fn_overloads")) {
    if (auto fna = ctx->getFunctionArgs(iter->type)) {
      auto [generics, args] = *fna;
      auto typ = generics[0]->getClass();
      auto name = ctx->getStaticString(generics[1]);
      seqassert(name, "bad static string");
      if (auto n = in(ctx->cache->classes[typ->name].methods, *name)) {
        auto &mt = ctx->cache->overloads[*n];
        for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
          auto &method = mt[mti];
          if (endswith(method.name, ":dispatch") ||
              !ctx->cache->functions[method.name].type)
            continue;
          if (method.age <= ctx->age)
            block->stmts.push_back(fn(var, N<IdExpr>(method.name)));
        }
      }
    } else {
      error("bad call to fn_overloads");
    }
  } else if (iter && startswith(iter->value, "std.internal.builtin.staticenumerate")) {
    auto &suiteVec = stmt->suite->getSuite()->stmts;
    int validI = 0;
    for (; validI < suiteVec.size(); validI++) {
      if (auto a = suiteVec[validI]->getAssign())
        if (auto i = suiteVec[0]->getAssign()->rhs->getIndex())
          if (i->expr->isId(var))
            continue;
      break;
    }
    if (validI != 2)
      error("fn_args needs two variables in for loop");

    if (auto fna = ctx->getFunctionArgs(iter->type)) {
      auto [generics, args] = *fna;
      auto typ = args[0]->getRecord();
      if (!typ)
        error("fn_args needs a tuple");
      for (size_t i = 0; i < typ->args.size(); i++) {
        suiteVec[0]->getAssign()->rhs = N<IntExpr>(i);
        suiteVec[0]->getAssign()->type =
            NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"));
        suiteVec[1]->getAssign()->rhs =
            N<IndexExpr>(stmt->iter->getCall()->args[0].value->clone(), N<IntExpr>(i));
        block->stmts.push_back(fn("", nullptr));
      }
    } else {
      error("bad call to fn_args");
    }
  } else {
    return nullptr;
  }
  ctx->blockLevel++;

  // Close the loop
  auto a = N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(false));
  a->setUpdate();
  block->stmts.push_back(a);

  auto loop =
      transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(true)),
                             N<WhileStmt>(N<IdExpr>(loopVar), block)));
  ctx->blockLevel--;
  return loop;
}

} // namespace codon::ast
