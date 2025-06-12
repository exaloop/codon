// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;
using namespace matcher;

/// Ensure that `break` is in a loop.
/// Transform if a loop break variable is available
/// (e.g., a break within loop-else block).
/// @example
///   `break` -> `no_break = False; break`

void TypecheckVisitor::visit(BreakStmt *stmt) {
  if (!ctx->getBase()->getLoop())
    E(Error::EXPECTED_LOOP, stmt, "break");
  ctx->getBase()->getLoop()->flat = false;
  if (!ctx->getBase()->getLoop()->breakVar.empty()) {
    resultStmt =
        N<SuiteStmt>(transform(N<AssignStmt>(
                         N<IdExpr>(ctx->getBase()->getLoop()->breakVar),
                         N<BoolExpr>(false), nullptr, AssignStmt::UpdateMode::Update)),
                     N<BreakStmt>());
  } else {
    stmt->setDone();
    if (!ctx->staticLoops.back().empty()) {
      auto a = N<AssignStmt>(N<IdExpr>(ctx->staticLoops.back()), N<BoolExpr>(false));
      a->setUpdate();
      resultStmt = transform(N<SuiteStmt>(a, stmt));
    }
  }
}

/// Ensure that `continue` is in a loop
void TypecheckVisitor::visit(ContinueStmt *stmt) {
  if (!ctx->getBase()->getLoop())
    E(Error::EXPECTED_LOOP, stmt, "continue");
  ctx->getBase()->getLoop()->flat = false;

  stmt->setDone();
  if (!ctx->staticLoops.back().empty()) {
    resultStmt = N<BreakStmt>();
    resultStmt->setDone();
  }
}

/// Transform a while loop.
/// @example
///   `while cond: ...`           ->  `while cond: ...`
///   `while cond: ... else: ...` -> ```no_break = True
///                                     while cond:
///                                       ...
///                                     if no_break: ...```
void TypecheckVisitor::visit(WhileStmt *stmt) {
  // Check for while-else clause
  std::string breakVar;
  if (stmt->getElse() && stmt->getElse()->firstInBlock()) {
    // no_break = True
    breakVar = getTemporaryVar("no_break");
    prependStmts->push_back(
        transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true))));
  }

  ctx->staticLoops.push_back(stmt->gotoVar.empty() ? "" : stmt->gotoVar);
  ctx->getBase()->loops.emplace_back(breakVar);

  auto oldExpectedType = getStdLibType("bool")->shared_from_this();
  std::swap(ctx->expectedType, oldExpectedType);
  stmt->cond = transform(stmt->getCond());
  std::swap(ctx->expectedType, oldExpectedType);
  wrapExpr(&stmt->cond, getStdLibType("bool"));

  ctx->blockLevel++;
  stmt->suite = SuiteStmt::wrap(transform(stmt->getSuite()));
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  // Complete while-else clause
  if (stmt->getElse() && stmt->getElse()->firstInBlock()) {
    auto es = stmt->getElse();
    stmt->elseSuite = nullptr;
    resultStmt = transform(N<SuiteStmt>(stmt, N<IfStmt>(N<IdExpr>(breakVar), es)));
  }
  ctx->getBase()->loops.pop_back();

  if (stmt->getCond()->isDone() && stmt->getSuite()->isDone())
    stmt->setDone();
}

/// Typecheck for statements. Wrap the iterator expression with `__iter__` if needed.
/// See @c transformHeterogenousTupleFor for iterating heterogenous tuples.
void TypecheckVisitor::visit(ForStmt *stmt) {
  if (stmt->isAsync())
    E(Error::CUSTOM, stmt, "async not yet supported");

  stmt->decorator = transformForDecorator(stmt->getDecorator());

  std::string breakVar;
  // Needs in-advance transformation to prevent name clashes with the iterator variable
  stmt->getIter()->setAttribute(
      Attr::ExprNoSpecial); // do not expand special calls here,
                            // might be needed for statis loops!
  stmt->iter = transform(stmt->getIter());

  // Check for for-else clause
  Stmt *assign = nullptr;
  if (stmt->getElse() && stmt->getElse()->firstInBlock()) {
    breakVar = getTemporaryVar("no_break");
    assign = transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true)));
  }

  // Extract the iterator type of the for
  auto iterType = extractClassType(stmt->getIter());
  if (!iterType)
    return; // wait until the iterator is known

  auto [delay, staticLoop] = transformStaticForLoop(stmt);
  if (delay)
    return;
  if (staticLoop) {
    resultStmt = staticLoop;
    return;
  }

  // Replace for (i, j) in ... { ... } with for tmp in ...: { i, j = tmp ; ... }
  if (!cast<IdExpr>(stmt->getVar())) {
    auto var = N<IdExpr>(ctx->cache->getTemporaryVar("for"));
    auto ns = unpackAssignment(stmt->getVar(), var);
    stmt->suite = N<SuiteStmt>(ns, stmt->getSuite());
    stmt->var = var;
  }

  // Case: iterating a non-generator. Wrap with `__iter__`
  if (iterType->name != "Generator" && !stmt->isWrapped()) {
    stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->getIter(), "__iter__")));
    iterType = extractClassType(stmt->getIter());
    stmt->wrapped = true;
  }

  ctx->getBase()->loops.emplace_back(breakVar);
  auto var = cast<IdExpr>(stmt->getVar());
  seqassert(var, "corrupt for variable: {}", *(stmt->getVar()));

  if (!var->hasAttribute(Attr::ExprDominated) &&
      !var->hasAttribute(Attr::ExprDominatedUsed)) {
    ctx->addVar(
        getUnmangledName(var->getValue()), ctx->generateCanonicalName(var->getValue()),
        stmt->getVar()->getType() ? stmt->getVar()->getType()->shared_from_this()
                                  : instantiateUnbound(),
        getTime());
  } else if (var->hasAttribute(Attr::ExprDominatedUsed)) {
    var->eraseAttribute(Attr::ExprDominatedUsed);
    var->setAttribute(Attr::ExprDominated);
    stmt->suite = N<SuiteStmt>(
        N<AssignStmt>(N<IdExpr>(format("{}{}", var->getValue(), VAR_USED_SUFFIX)),
                      N<BoolExpr>(true), nullptr, AssignStmt::UpdateMode::Update),
        stmt->getSuite());
  }
  stmt->var = transform(stmt->getVar());

  // Unify iterator variable and the iterator type
  if (iterType && iterType->name != "Generator")
    E(Error::EXPECTED_GENERATOR, stmt->getIter());
  if (iterType)
    unify(stmt->getVar()->getType(), extractClassGeneric(iterType));

  ctx->staticLoops.emplace_back();
  ctx->blockLevel++;
  stmt->suite = SuiteStmt::wrap(transform(stmt->getSuite()));
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  if (ctx->getBase()->getLoop()->flat)
    stmt->flat = true;

  // Complete for-else clause
  if (stmt->getElse() && stmt->getElse()->firstInBlock()) {
    auto es = stmt->getElse();
    stmt->elseSuite = nullptr;
    resultStmt =
        transform(N<SuiteStmt>(assign, stmt, N<IfStmt>(N<IdExpr>(breakVar), es)));
    stmt->elseSuite = nullptr;
  }

  ctx->getBase()->loops.pop_back();

  if (stmt->getIter()->isDone() && stmt->getSuite()->isDone())
    stmt->setDone();
}

/// Transform and check for OpenMP decorator.
/// @example
///   `@par(num_threads=2, openmp="schedule(static)")` ->
///   `for_par(num_threads=2, schedule="static")`
Expr *TypecheckVisitor::transformForDecorator(Expr *decorator) {
  if (!decorator)
    return nullptr;
  Expr *callee = decorator;
  if (auto c = cast<CallExpr>(callee))
    callee = c->getExpr();
  auto ci = cast<IdExpr>(transform(callee));
  if (!ci || !startswith(ci->getValue(), "std.openmp.for_par.0")) {
    E(Error::LOOP_DECORATOR, decorator);
  }

  std::vector<CallArg> args;
  std::string openmp;
  std::vector<CallArg> omp;
  if (auto c = cast<CallExpr>(decorator))
    for (auto &a : *c) {
      if (a.getName() == "openmp" ||
          (a.getName().empty() && openmp.empty() && cast<StringExpr>(a.getExpr()))) {
        auto ompOrErr =
            parseOpenMP(ctx->cache, cast<StringExpr>(a.getExpr())->getValue(),
                        a.value->getSrcInfo());
        if (!ompOrErr)
          throw exc::ParserException(ompOrErr.takeError());
        omp = *ompOrErr;
      } else {
        args.emplace_back(a.getName(), transform(a.getExpr()));
      }
    }
  for (auto &a : omp)
    args.emplace_back(a.getName(), transform(a.getExpr()));
  return transform(N<CallExpr>(transform(N<IdExpr>("for_par")), args));
}

/// Handle static for constructs.
/// @example
///   `for i in statictuple(1, x): <suite>` ->
///   ```loop = True
///      while loop:
///        while loop:
///          i: Literal[int] = 1; <suite>; break
///        while loop:
///          i = x; <suite>; break
///        loop = False   # also set to False on break
/// If a loop is flat, while wrappers are removed.
/// A separate suite is generated for each static iteration.
std::pair<bool, Stmt *> TypecheckVisitor::transformStaticForLoop(ForStmt *stmt) {
  auto loopVar = getTemporaryVar("loop");
  auto suite = clean_clone(stmt->getSuite());
  auto [ok, delay, preamble, items] = transformStaticLoopCall(
      stmt->getVar(), &suite, stmt->getIter(), [&](Stmt *assigns) {
        Stmt *ret = nullptr;
        if (!stmt->flat) {
          auto brk = N<BreakStmt>();
          brk->setDone(); // Avoid transforming this one to skip extra checks
          // var [: Static] := expr; suite...
          auto loop = N<WhileStmt>(N<IdExpr>(loopVar),
                                   N<SuiteStmt>(assigns, clone(suite), brk));
          loop->gotoVar = loopVar;
          ret = loop;
        } else {
          ret = N<SuiteStmt>(assigns, clone(stmt->getSuite()));
        }
        return ret;
      });
  if (!ok)
    return {false, nullptr};
  if (delay)
    return {true, nullptr};

  // Close the loop
  auto block = N<SuiteStmt>();
  block->addStmt(preamble);
  for (auto &i : items)
    block->addStmt(cast<Stmt>(i));
  Stmt *loop = nullptr;
  if (!stmt->flat) {
    ctx->blockLevel++;
    auto a = N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(false));
    a->setUpdate();
    block->addStmt(a);
    loop = transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(true)),
                                  N<WhileStmt>(N<IdExpr>(loopVar), block)));
    ctx->blockLevel--;
  } else {
    loop = transform(block);
  }
  return {false, loop};
}

std::tuple<bool, bool, Stmt *, std::vector<ASTNode *>>
TypecheckVisitor::transformStaticLoopCall(Expr *varExpr, SuiteStmt **varSuite,
                                          Expr *iter,
                                          const std::function<ASTNode *(Stmt *)> &wrap,
                                          bool allowNonHeterogenous) {
  if (!iter->getClassType())
    return {true, true, nullptr, {}};

  std::vector<std::string> vars{};
  if (auto ei = cast<IdExpr>(varExpr)) {
    vars.push_back(ei->getValue());
  } else {
    Items<Expr *> *list = nullptr;
    if (auto el = cast<ListExpr>(varExpr))
      list = el;
    else if (auto et = cast<TupleExpr>(varExpr))
      list = et;
    if (list) {
      for (const auto &it : *list)
        if (auto ei = cast<IdExpr>(it)) {
          vars.push_back(ei->getValue());
        } else {
          return {false, false, nullptr, {}};
        }
    } else {
      return {false, false, nullptr, {}};
    }
  }

  Stmt *preamble = nullptr;
  iter = getHeadExpr(iter);
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  std::vector<Stmt *> block;
  if (fn && startswith(fn->getValue(), "std.internal.static.tuple.0")) {
    block = populateStaticTupleLoop(iter, vars);
  } else if (fn && startswith(fn->getValue(), "std.internal.static.range.0:1")) {
    block = populateSimpleStaticRangeLoop(iter, vars);
  } else if (fn && startswith(fn->getValue(), "std.internal.static.range.0")) {
    block = populateStaticRangeLoop(iter, vars);
  } else if (fn &&
             startswith(fn->getValue(), "std.internal.static.function.0.overloads.0")) {
    block = populateStaticFnOverloadsLoop(iter, vars);
  } else if (fn && startswith(fn->getValue(), "std.internal.static.enumerate.0")) {
    block = populateStaticEnumerateLoop(iter, vars);
  } else if (fn && startswith(fn->getValue(), "std.internal.static.vars.0")) {
    block = populateStaticVarsLoop(iter, vars);
  } else if (fn && startswith(fn->getValue(), "std.internal.static.vars_types.0")) {
    block = populateStaticVarTypesLoop(iter, vars);
  } else {
    bool maybeHeterogenous = iter->getType()->is(TYPE_TUPLE);
    if (maybeHeterogenous) {
      if (!iter->getType()->canRealize())
        return {true, true, nullptr, {}}; // wait until the tuple is fully realizable
      if (!iter->getClassType()->getHeterogenousTuple() && !allowNonHeterogenous)
        return {false, false, nullptr, {}};
      block = populateStaticHeterogenousTupleLoop(iter, vars);
      preamble = block.back();
      block.pop_back();
    } else {
      return {false, false, nullptr, {}};
    }
  }
  std::vector<ASTNode *> wrapBlock;
  wrapBlock.reserve(block.size());
  for (auto b : block) {
    wrapBlock.push_back(wrap(b));
  }
  return {true, false, preamble, wrapBlock};
}

} // namespace codon::ast
