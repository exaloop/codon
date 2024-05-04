// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
      resultStmt = transform(N<SuiteStmt>(a, stmt->shared_from_this()));
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

  ctx->staticLoops.push_back(stmt->gotoVar.empty() ? "" : stmt->gotoVar);
  ctx->getBase()->loops.emplace_back(breakVar);
  transform(stmt->cond);
  if (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
    stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));

  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  // Complete while-else clause
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    auto es = stmt->elseSuite;
    stmt->elseSuite = nullptr;
    resultStmt = transform(
        N<SuiteStmt>(stmt->shared_from_this(), N<IfStmt>(N<IdExpr>(breakVar), es)));
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

  auto [delay, staticLoop] = transformStaticForLoop(stmt);
  if (delay)
    return;
  if (staticLoop) {
    resultStmt = staticLoop;
    return;
  }

  // Case: iterating a non-generator. Wrap with `__iter__`
  if (iterType->name != "Generator" && !stmt->wrapped) {
    stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->iter, "__iter__")));
    iterType = stmt->iter->getType()->getClass();
    stmt->wrapped = true;
  }

  ctx->getBase()->loops.emplace_back(breakVar);
  auto var = stmt->var->getId();
  seqassert(var, "corrupt for variable: {}", stmt->var);

  if (!stmt->var->hasAttr(ExprAttr::Dominated)) {
    ctx->addVar(var->value, ctx->generateCanonicalName(var->value), ctx->getUnbound());
  }
  transform(stmt->var);

  // Unify iterator variable and the iterator type
  if (iterType && iterType->name != "Generator")
    E(Error::EXPECTED_GENERATOR, stmt->iter);
  if (iterType)
    unify(stmt->var->type, iterType->generics[0].type);

  ctx->staticLoops.emplace_back();
  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;
  ctx->staticLoops.pop_back();

  if (ctx->getBase()->getLoop()->flat)
    stmt->flat = true;

  // Complete for-else clause
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    auto es = stmt->elseSuite;
    stmt->elseSuite = nullptr;
    resultStmt = transform(N<SuiteStmt>(assign, stmt->shared_from_this(),
                                        N<IfStmt>(N<IdExpr>(breakVar), es)));
  }

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
  transform(callee);
  if (!callee || !(callee->getId() &&
                   startswith(callee->getId()->value, "std.openmp.for_par.0"))) {
    E(Error::LOOP_DECORATOR, decorator);
  }
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
  return transform(N<CallExpr>(transform(N<IdExpr>("for_par")), args));
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
/// If a loop is flat, while wrappers are removed.
/// A separate suite is generated for each static iteration.
std::pair<bool, StmtPtr> TypecheckVisitor::transformStaticForLoop(ForStmt *stmt) {
  auto loopVar = ctx->cache->getTemporaryVar("loop");
  auto suite = clean_clone(stmt->suite);
  auto [ok, delay, preamble, items] = transformStaticLoopCall(
      stmt->var, suite, stmt->iter, [&](const StmtPtr &assigns) {
        StmtPtr ret = nullptr;
        if (!stmt->flat) {
          auto brk = N<BreakStmt>();
          brk->setDone(); // Avoid transforming this one to continue
          // var [: Static] := expr; suite...
          auto loop = N<WhileStmt>(N<IdExpr>(loopVar),
                                   N<SuiteStmt>(assigns, clone(suite), brk));
          loop->gotoVar = loopVar;
          ret = loop;
        } else {
          ret = N<SuiteStmt>(assigns, clone(stmt->suite));
        }
        return ret;
      });
  if (!ok)
    return {false, nullptr};
  if (delay)
    return {true, nullptr};

  // Close the loop
  auto block = N<SuiteStmt>();
  for (auto &i : items)
    block->stmts.push_back(std::dynamic_pointer_cast<Stmt>(i));
  StmtPtr loop = nullptr;
  if (!stmt->flat) {
    ctx->blockLevel++;
    auto a = N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(false));
    a->setUpdate();
    block->stmts.push_back(a);
    loop = transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(loopVar), N<BoolExpr>(true)),
                                  N<WhileStmt>(N<IdExpr>(loopVar), block)));
    ctx->blockLevel--;
  } else {
    loop = transform(block);
  }
  return {false, loop};
}

std::tuple<bool, bool, StmtPtr, std::vector<std::shared_ptr<codon::SrcObject>>>
TypecheckVisitor::transformStaticLoopCall(
    const ExprPtr &varExpr, StmtPtr &varSuite, const ExprPtr &iter,
    const std::function<std::shared_ptr<codon::SrcObject>(StmtPtr)> &wrap,
    bool allowNonHeterogenous) {
  if (!iter->type->getClass())
    return {true, true, nullptr, {}};

  seqassert(varExpr->getId(), "bad varExpr");
  std::function<int(StmtPtr &, const std::function<void(StmtPtr &)> &)> iterFn;
  iterFn = [&iterFn](StmtPtr &s, const std::function<void(StmtPtr &)> &fn) -> int {
    // if (n <= 0)
    // return 0;
    if (!s)
      return 0;
    if (auto su = s->getSuite()) {
      int i = 0;
      for (auto &si : su->stmts) {
        i += iterFn(si, fn);
        // if (i >= n)
        // break;
      }
      return i;
    } else {
      fn(s);
      return 1;
    }
  };
  std::vector<std::string> vars{varExpr->getId()->value};
  iterFn(varSuite, [&](StmtPtr &s) {
    if (auto a = s->getAssign()) {
      if (a->rhs && a->rhs->getIndex())
        if (a->rhs->getIndex()->expr->isId(vars[0])) {
          vars.push_back(a->lhs->getId()->value);
          s = nullptr;
        }
    }
  });
  if (vars.size() > 1)
    vars.erase(vars.begin());
  if (vars.empty())
    return {false, false, nullptr, {}};

  StmtPtr preamble = nullptr;
  auto fn = iter->getCall() ? iter->getCall()->expr->getId() : nullptr;
  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  std::vector<std::shared_ptr<codon::SrcObject>> block;
  if (fn && startswith(fn->value, "statictuple")) {
    auto &args = iter->getCall()->args[0].value->getCall()->args;
    if (vars.size() != 1)
      error("expected one item");
    for (auto &a : args) {
      stmt->rhs = transform(clean_clone(a.value));
      if (auto st = stmt->rhs->type->getStatic()) {
        stmt->type = N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>(st->name));
      } else {
        stmt->type = nullptr;
      }
      block.push_back(wrap(clone(stmt)));
    }
  } else if (fn && startswith(fn->value, "std.internal.types.range.staticrange.0:1")) {
    if (vars.size() != 1)
      error("expected one item");
    auto ed = fn->type->getFunc()->funcGenerics[0].type->getIntStatic()->value;
    if (ed > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER, ed);
    for (int64_t i = 0; i < ed; i++) {
      stmt->rhs = N<IntExpr>(i);
      stmt->type = N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"));
      block.push_back(wrap(clone(stmt)));
    }
  } else if (fn && startswith(fn->value, "std.internal.types.range.staticrange.0")) {
    if (vars.size() != 1)
      error("expected one item");
    auto st = fn->type->getFunc()->funcGenerics[0].type->getIntStatic()->value;
    auto ed = fn->type->getFunc()->funcGenerics[1].type->getIntStatic()->value;
    auto step = fn->type->getFunc()->funcGenerics[2].type->getIntStatic()->value;
    if (abs(st - ed) / abs(step) > MAX_STATIC_ITER)
      E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER, abs(st - ed) / abs(step));
    for (int64_t i = st; step > 0 ? i < ed : i > ed; i += step) {
      stmt->rhs = N<IntExpr>(i);
      stmt->type = N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"));
      block.push_back(wrap(clone(stmt)));
    }
  } else if (fn && startswith(fn->value, "std.internal.static.fn_overloads.0")) {
    if (vars.size() != 1)
      error("expected one item");
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;
      auto typ = generics[0]->getClass();
      seqassert(generics[1]->getStrStatic(), "bad static string");
      auto name = generics[1]->getStrStatic()->value;
      if (auto n = in(ctx->cache->getClass(typ)->methods, name)) {
        auto &mt = ctx->cache->overloads[*n];
        for (int mti = int(mt.size()) - 1; mti >= 0; mti--) {
          auto &method = mt[mti];
          if (endswith(method, ":dispatch") || !ctx->cache->functions[method].type)
            continue;
          // if (method.age <= ctx->age) {
          if (typ->getHeterogenousTuple()) {
            auto &ast = ctx->cache->functions[method].ast;
            if (ast->hasAttr("autogenerated") &&
                (endswith(ast->name, ".__iter__") ||
                 endswith(ast->name, ".__getitem__"))) {
              // ignore __getitem__ and other heterogenuous methods
              continue;
            }
          }
          stmt->rhs = N<IdExpr>(method);
          block.push_back(wrap(clone(stmt)));
          // }
        }
      }
    } else {
      error("bad call to fn_overloads");
    }
  } else if (fn && startswith(fn->value, "std.internal.builtin.staticenumerate.0")) {
    if (vars.size() != 2)
      error("expected two items");
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;
      auto typ = args[0]->getClass();
      if (typ && typ->isRecord()) {
        for (size_t i = 0; i < getClassFields(typ.get()).size(); i++) {
          auto b = N<SuiteStmt>(
              {N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                             N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"))),
               N<AssignStmt>(N<IdExpr>(vars[1]),
                             N<IndexExpr>(clone(iter->getCall()->args[0].value),
                                          N<IntExpr>(i)))});
          block.push_back(wrap(b));
        }
      } else {
        error("staticenumerate needs a tuple");
      }
    } else {
      error("bad call to staticenumerate");
    }
  } else if (fn && startswith(fn->value, "std.internal.internal.vars.0")) {
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;

      bool withIdx = generics[0]->getBoolStatic()->value;
      if (!withIdx && vars.size() != 2)
        error("expected two items");
      else if (withIdx && vars.size() != 3)
        error("expected three items");
      auto typ = args[0]->getClass();
      size_t idx = 0;
      for (auto &f : getClassFields(typ.get())) {
        std::vector<StmtPtr> stmts;
        if (withIdx) {
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                            N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"))));
        }
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<StringExpr>(f.name),
                          N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("str"))));
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[withIdx + 1]),
                          N<DotExpr>(clone(iter->getCall()->args[0].value), f.name)));
        auto b = N<SuiteStmt>(stmts);
        block.push_back(wrap(b));
        idx++;
      }
    } else {
      error("bad call to vars");
    }
  } else if (fn && startswith(fn->value, "std.internal.static.vars_types.0")) {
    if (auto fna = ctx->getFunctionArgs(fn->type)) {
      auto [generics, args] = *fna;

      auto typ = realize(generics[0]->getClass());
      bool withIdx = generics[1]->getBoolStatic()->value;
      if (!withIdx && vars.size() != 1)
        error("expected one item");
      else if (withIdx && vars.size() != 2)
        error("expected two items");

      seqassert(typ, "vars_types expects a realizable type, got '{}' instead",
                generics[0]);

      if (auto utyp = typ->getUnion()) {
        for (size_t i = 0; i < utyp->getRealizationTypes().size(); i++) {
          std::vector<StmtPtr> stmts;
          if (withIdx) {
            stmts.push_back(
                N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                              N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"))));
          }
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[1]),
                            N<IdExpr>(utyp->getRealizationTypes()[i]->realizedName())));
          auto b = N<SuiteStmt>(stmts);
          block.push_back(wrap(b));
        }
      } else {
        size_t idx = 0;
        for (auto &f : getClassFields(typ->getClass().get())) {
          auto ta = realize(ctx->instantiate(f.type, typ->getClass()));
          seqassert(ta, "cannot realize '{}'", f.type->debugString(1));
          std::vector<StmtPtr> stmts;
          if (withIdx) {
            stmts.push_back(
                N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                              N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"))));
          }
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<IdExpr>(ta->realizedName())));
          auto b = N<SuiteStmt>(stmts);
          block.push_back(wrap(b));
          idx++;
        }
      }
    } else {
      error("bad call to vars");
    }
  } else {
    bool maybeHeterogenous = iter->type->is(TYPE_TUPLE);
    if (maybeHeterogenous) {
      if (!iter->type->canRealize())
        return {true, true, nullptr, {}}; // wait until the tuple is fully realizable
      if (!iter->type->getClass()->getHeterogenousTuple() && !allowNonHeterogenous)
        return {false, false, nullptr, {}};

      std::string tupleVar;
      if (!iter->getId()) {
        tupleVar = ctx->cache->getTemporaryVar("tuple");
        preamble = N<AssignStmt>(N<IdExpr>(tupleVar), iter);
      } else {
        tupleVar = iter->getId()->value;
      }
      for (size_t i = 0; i < iter->type->getClass()->generics.size(); i++) {
        auto s = N<SuiteStmt>();
        if (vars.size() > 1) {
          for (size_t j = 0; j < vars.size(); j++) {
            s->stmts.push_back(N<AssignStmt>(
                N<IdExpr>(vars[j]),
                N<IndexExpr>(N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(i)),
                             N<IntExpr>(j))));
          }
        } else {
          s->stmts.push_back(N<AssignStmt>(
              N<IdExpr>(vars[0]), N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(i))));
        }
        block.push_back(wrap(s));
      }
      return {true, false, preamble, block};
    }
    return {false, false, nullptr, {}};
  }
  return {true, false, preamble, block};
}

} // namespace codon::ast
