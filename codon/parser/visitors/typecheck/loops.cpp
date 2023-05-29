// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;

/// Ensure that `break` is in a loop.
/// Transform if a loop break variable is available
/// (e.g., a break within loop-else block).
/// @example
///   `break` -> `no_break = False; break`

void TypecheckVisitor::visit(BreakStmt *stmt) {
  if (!ctx->getBase()->getLoop())
    E(Error::EXPECTED_LOOP, stmt, "break");
  if (!ctx->getBase()->getLoop()->breakVar.empty()) {
    resultStmt = N<SuiteStmt>(
        transform(N<AssignStmt>(N<IdExpr>(ctx->getBase()->getLoop()->breakVar),
                                N<BoolExpr>(false))),
        N<BreakStmt>());
  } else {
    stmt->setDone();
    if (!ctx->staticLoops.back().empty()) {
      auto a = N<AssignStmt>(N<IdExpr>(ctx->staticLoops.back()), N<BoolExpr>(false));
      a->setUpdate();
      resultStmt = transform(N<SuiteStmt>(a, stmt->clone()));
    }
  }
}

/// Ensure that `continue` is in a loop
void TypecheckVisitor::visit(ContinueStmt *stmt) {
  if (!ctx->getBase()->getLoop())
    E(Error::EXPECTED_LOOP, stmt, "continue");

  stmt->setDone();
  if (!ctx->staticLoops.back().empty()) {
    resultStmt = N<BreakStmt>();
    resultStmt->setDone();
  }
}

/// Transform a while loop.
/// @example
///   `while cond: ...`           ->  `while cond.__bool__(): ...`
///   `while cond: ... else: ...` -> ```no_break = True
///                                     while cond.__bool__():
///                                       ...
///                                     if no_break: ...```
void TypecheckVisitor::visit(WhileStmt *stmt) {
  // Check for while-else clause
  std::string breakVar;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    // no_break = True
    breakVar = ctx->cache->getTemporaryVar("no_break");
    prependStmts->push_back(
        transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true))));
  }

  ctx->enterConditionalBlock();
  ctx->staticLoops.push_back(stmt->gotoVar.empty() ? "" : stmt->gotoVar);
  ctx->getBase()->loops.push_back({breakVar, ctx->scope.blocks, {}});
  stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));

  ctx->blockLevel++;
  transformConditionalScope(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  // Complete while-else clause
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt = N<SuiteStmt>(N<WhileStmt>(*stmt),
                              N<IfStmt>(transform(N<IdExpr>(breakVar)),
                                        transformConditionalScope(stmt->elseSuite)));
  }

  ctx->leaveConditionalBlock();
  // Dominate loop variables
  for (auto &var : ctx->getBase()->getLoop()->seenVars) {
    ctx->findDominatingBinding(var, this);
  }
  ctx->getBase()->loops.pop_back();

  if (stmt->cond->isDone() && stmt->suite->isDone())
    stmt->setDone();
}

/// Typecheck for statements. Wrap the iterator expression with `__iter__` if needed.
/// See @c transformHeterogenousTupleFor for iterating heterogenous tuples.
void TypecheckVisitor::visit(ForStmt *stmt) {
  stmt->decorator = transformForDecorator(stmt->decorator);
  // transform(stmt->decorator);

  std::string breakVar;
  // Needs in-advance transformation to prevent name clashes with the iterator variable
  stmt->iter = transform(stmt->iter);

  // Check for for-else clause
  StmtPtr assign = nullptr;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    breakVar = ctx->cache->getTemporaryVar("no_break");
    assign = transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true)));
  }

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

  ctx->enterConditionalBlock();
  ctx->getBase()->loops.push_back({breakVar, ctx->scope.blocks, {}});
  std::string varName;
  if (auto i = stmt->var->getId()) {
    auto val = ctx->addVar(i->value, varName = ctx->generateCanonicalName(i->value),
                           stmt->var->getSrcInfo());
    val->avoidDomination = ctx->avoidDomination;
    transform(stmt->var);
    stmt->suite = N<SuiteStmt>(stmt->suite);
  } else {
    varName = ctx->cache->getTemporaryVar("for");
    auto val = ctx->addVar(varName, varName, stmt->var->getSrcInfo());
    auto var = N<IdExpr>(varName);
    std::vector<StmtPtr> stmts;
    // Add for_var = [for variables]
    stmts.push_back(N<AssignStmt>(stmt->var, clone(var)));
    stmt->var = var;
    stmts.push_back(stmt->suite);
    stmt->suite = N<SuiteStmt>(stmts);
  }

  auto var = stmt->var->getId();
  seqassert(var, "corrupt for variable: {}", stmt->var);

  // Unify iterator variable and the iterator type
  auto val = ctx->addVar(var->value, var->value, getSrcInfo(),
                         ctx->getUnbound(stmt->var->getSrcInfo()));
  val->root = stmt;
  if (iterType && iterType->name != "Generator")
    E(Error::EXPECTED_GENERATOR, stmt->iter);
  unify(stmt->var->type,
        iterType ? unify(val->type, iterType->generics[0].type) : val->type);

  ctx->staticLoops.emplace_back("");
  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  // Complete while-else clause
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt = N<SuiteStmt>(assign, N<ForStmt>(*stmt),
                              N<IfStmt>(transform(N<IdExpr>(breakVar)),
                                        transformConditionalScope(stmt->elseSuite)));
    val->root = resultStmt->getSuite()->stmts[1].get();
  }

  ctx->leaveConditionalBlock(&(stmt->suite->getSuite()->stmts));
  // Dominate loop variables
  for (auto &var : ctx->getBase()->getLoop()->seenVars)
    ctx->findDominatingBinding(var, this);
  ctx->getBase()->loops.pop_back();

  if (stmt->iter->isDone() && stmt->suite->isDone())
    stmt->setDone();
}

/// Transform and check for OpenMP decorator.
/// @example
///   `@par(num_threads=2, openmp="schedule(static)")` ->
///   `for_par(num_threads=2, schedule="static")`
ExprPtr TypecheckVisitor::transformForDecorator(const ExprPtr &decorator) {
  if (!decorator)
    return nullptr;
  ExprPtr callee = decorator;
  if (auto c = callee->getCall())
    callee = c->expr;
  if (!callee || !callee->isId("par"))
    E(Error::LOOP_DECORATOR, decorator);
  std::vector<CallExpr::Arg> args;
  std::string openmp;
  std::vector<CallExpr::Arg> omp;
  if (auto c = decorator->getCall())
    for (auto &a : c->args) {
      if (a.name == "openmp" ||
          (a.name.empty() && openmp.empty() && a.value->getString())) {
        omp = parseOpenMP(ctx->cache, a.value->getString()->getValue(),
                          a.value->getSrcInfo());
      } else {
        args.emplace_back(a.name, transform(a.value));
      }
    }
  for (auto &a : omp)
    args.emplace_back(a.name, transform(a.value));
  return N<CallExpr>(transform(N<IdExpr>("for_par")), args);
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
  auto var = stmt->var->getId()->value;
  if (!stmt->iter->getCall() || !stmt->iter->getCall()->expr->getId())
    return nullptr;
  auto iter = stmt->iter->getCall()->expr->getId();
  auto loopVar = ctx->cache->getTemporaryVar("loop");

  std::vector<std::string> vars{var};
  auto suiteVec = stmt->suite->getSuite();
  auto oldSuite = suiteVec ? suiteVec->clone() : nullptr;
  for (int validI = 0; suiteVec && validI < suiteVec->stmts.size(); validI++) {
    if (auto a = suiteVec->stmts[validI]->getAssign())
      if (a->rhs && a->rhs->getIndex())
        if (a->rhs->getIndex()->expr->isId(var)) {
          vars.push_back(a->lhs->getId()->value);
          suiteVec->stmts[validI] = nullptr;
          continue;
        }
    break;
  }
  if (vars.size() > 1)
    vars.erase(vars.begin());
  auto [ok,
        items] = transformStaticLoopCall(vars, stmt->iter, [&](const StmtPtr &assigns) {
    auto brk = N<BreakStmt>();
    brk->setDone(); // Avoid transforming this one to continue
    // var [: Static] := expr; suite...
    auto loop = N<WhileStmt>(N<IdExpr>(loopVar),
                             N<SuiteStmt>(assigns, clone(stmt->suite), brk));
    loop->gotoVar = loopVar;
    return loop;
  });
  if (!ok) {
    if (oldSuite)
      stmt->suite = oldSuite;
    return nullptr;
  }

  // Close the loop
  ctx->blockLevel++;
  auto a = N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(false));
  a->setUpdate();
  auto block = N<SuiteStmt>();
  for (auto &i : items)
    block->stmts.push_back(std::dynamic_pointer_cast<Stmt>(i));
  block->stmts.push_back(a);
  auto loop =
      transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(true)),
                             N<WhileStmt>(N<IdExpr>(loopVar), block)));
  ctx->blockLevel--;
  return loop;
}

std::pair<bool, std::vector<std::shared_ptr<codon::SrcObject>>>
TypecheckVisitor::transformStaticLoopCall(
    const std::vector<std::string> &vars, const ExprPtr &iter,
    const std::function<std::shared_ptr<codon::SrcObject>(StmtPtr)> &wrap) {
  if (!iter->getCall())
    return {false, {}};
  auto fn = iter->getCall()->expr->getId();
  if (!fn || vars.empty())
    return {false, {}};

  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  std::vector<std::shared_ptr<codon::SrcObject>> block;
  if (startswith(fn->value, "statictuple:0")) {
    auto &args = iter->getCall()->args[0].value->getCall()->args;
    if (vars.size() != 1)
      error("expected one item");
    for (auto &a : args) {
      stmt->rhs = a.value;
      if (stmt->rhs->isStatic()) {
        stmt->type = NT<IndexExpr>(
            N<IdExpr>("Static"),
            N<IdExpr>(stmt->rhs->staticValue.type == StaticValue::INT ? "int" : "str"));
      } else {
        stmt->type = nullptr;
      }
      block.push_back(wrap(stmt->clone()));
    }
  } else if (fn && startswith(fn->value, "std.internal.types.range.staticrange:0")) {
    if (vars.size() != 1)
      error("expected one item");
    auto st =
        fn->type->getFunc()->funcGenerics[0].type->getStatic()->evaluate().getInt();
    auto ed =
        fn->type->getFunc()->funcGenerics[1].type->getStatic()->evaluate().getInt();
    auto step =
        fn->type->getFunc()->funcGenerics[2].type->getStatic()->evaluate().getInt();
    if (abs(st - ed) / abs(step) > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER, abs(st - ed) / abs(step));
    for (int i = st; step > 0 ? i < ed : i > ed; i += step) {
      stmt->rhs = N<IntExpr>(i);
      stmt->type = NT<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"));
      block.push_back(wrap(stmt->clone()));
    }
  } else if (fn && startswith(fn->value, "std.internal.types.range.staticrange:1")) {
    if (vars.size() != 1)
      error("expected one item");
    auto ed =
        fn->type->getFunc()->funcGenerics[0].type->getStatic()->evaluate().getInt();
    if (ed > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER, ed);
    for (int i = 0; i < ed; i++) {
      stmt->rhs = N<IntExpr>(i);
      stmt->type = NT<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"));
      block.push_back(wrap(stmt->clone()));
    }
  } else if (fn && startswith(fn->value, "std.internal.static.fn_overloads")) {
    if (vars.size() != 1)
      error("expected one item");
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;
      auto typ = generics[0]->getClass();
      auto name = ctx->getStaticString(generics[1]);
      seqassert(name, "bad static string");
      if (auto n = in(ctx->cache->classes[typ->name].methods, *name)) {
        auto &mt = ctx->cache->overloads[*n];
        for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
          auto &method = mt[mti];
          if (endswith(method, ":dispatch") || !ctx->cache->functions[method].type)
            continue;
          // if (method.age <= ctx->age) {
          if (typ->getHeterogenousTuple()) {
            auto &ast = ctx->cache->functions[method].ast;
            if (ast->hasAttr("autogenerated") &&
                (endswith(ast->name, ".__iter__:0") ||
                 endswith(ast->name, ".__getitem__:0"))) {
              // ignore __getitem__ and other heterogenuous methods
              continue;
            }
          }
            stmt->rhs = N<IdExpr>(method);
            block.push_back(wrap(stmt->clone()));
            // }
        }
      }
    } else {
      error("bad call to fn_overloads");
    }
  } else if (fn && startswith(fn->value, "std.internal.builtin.staticenumerate")) {
    if (vars.size() != 2)
      error("expected two items");
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;
      auto typ = args[0]->getRecord();
      if (!typ)
        error("staticenumerate needs a tuple");
      for (size_t i = 0; i < typ->args.size(); i++) {
        auto b = N<SuiteStmt>(
            {N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                           NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"))),
             N<AssignStmt>(N<IdExpr>(vars[1]),
                           N<IndexExpr>(iter->getCall()->args[0].value->clone(),
                                        N<IntExpr>(i)))});
        block.push_back(wrap(b));
      }
    } else {
      error("bad call to staticenumerate");
    }
  } else if (fn && startswith(fn->value, "std.internal.internal.vars:0")) {
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;

      auto withIdx = generics[0]->getStatic()->evaluate().getInt() != 0 ? 1 : 0;
      if (!withIdx && vars.size() != 2)
        error("expected two items");
      else if (withIdx && vars.size() != 3)
        error("expected three items");
      auto typ = args[0]->getClass();
      size_t idx = 0;
      for (auto &f : ctx->cache->classes[typ->name].fields) {
        std::vector<StmtPtr> stmts;
        if (withIdx) {
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                            NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"))));
        }
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<StringExpr>(f.name),
                          NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("str"))));
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[withIdx + 1]),
                          N<DotExpr>(iter->getCall()->args[0].value->clone(), f.name)));
        auto b = N<SuiteStmt>(stmts);
        block.push_back(wrap(b));
        idx++;
      }
    } else {
      error("bad call to vars");
    }
  } else if (fn && startswith(fn->value, "std.internal.static.vars_types:0")) {
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;

      auto typ = realize(generics[0]->getClass());
      auto withIdx = generics[1]->getStatic()->evaluate().getInt() != 0 ? 1 : 0;
      if (!withIdx && vars.size() != 1)
        error("expected one item");
      else if (withIdx && vars.size() != 2)
        error("expected two items");

      seqassert(typ, "vars_types expects a realizable type, got '{}' instead",
                generics[0]);
      size_t idx = 0;
      for (auto &f : ctx->cache->classes[typ->getClass()->name].fields) {
        auto ta = realize(ctx->instantiate(f.type, typ->getClass()));
        seqassert(ta, "cannot realize '{}'", f.type->debugString(1));
        std::vector<StmtPtr> stmts;
        if (withIdx) {
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                            NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"))));
        }
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[withIdx]), NT<IdExpr>(ta->realizedName())));
        auto b = N<SuiteStmt>(stmts);
        block.push_back(wrap(b));
        idx++;
      }
    } else {
      error("bad call to vars");
    }
  } else {
    return {false, {}};
  }
  return {true, block};
}

} // namespace codon::ast
