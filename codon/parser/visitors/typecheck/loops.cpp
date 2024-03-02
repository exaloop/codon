// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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

  bool maybeHeterogenous =
      iterType->name == TYPE_TUPLE || startswith(iterType->name, TYPE_KWTUPLE);
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

  ctx->staticLoops.emplace_back();
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
/// If a loop is flat, while wrappers are removed.
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
  auto [ok, items] = transformStaticLoopCall(vars, stmt->iter, [&](StmtPtr assigns) {
    StmtPtr ret = nullptr;
    if (!stmt->flat) {
      auto brk = N<BreakStmt>();
      brk->setDone(); // Avoid transforming this one to continue
      // var [: Static] := expr; suite...
      auto loop = N<WhileStmt>(N<IdExpr>(loopVar),
                               N<SuiteStmt>(assigns, clone(stmt->suite), brk));
      loop->gotoVar = loopVar;
      ret = loop;
    } else {
      ret = N<SuiteStmt>(assigns, clone(stmt->suite));
    }
    return ret;
  });
  if (!ok) {
    if (oldSuite)
      stmt->suite = oldSuite;
    return nullptr;
  }

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
  return loop;
}

std::pair<bool, std::vector<std::shared_ptr<codon::SrcObject>>>
TypecheckVisitor::transformStaticLoopCall(
    const std::vector<std::string> &vars, ExprPtr iter,
    std::function<std::shared_ptr<codon::SrcObject>(StmtPtr)> wrap) {
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
    for (size_t i = 0; i < args.size(); i++) {
      stmt->rhs = args[i].value;
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
    int st =
        fn->type->getFunc()->funcGenerics[0].type->getStatic()->evaluate().getInt();
    int ed =
        fn->type->getFunc()->funcGenerics[1].type->getStatic()->evaluate().getInt();
    int step =
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
    int ed =
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
          if (endswith(method.name, ":dispatch") ||
              !ctx->cache->functions[method.name].type)
            continue;
          if (method.age <= ctx->age) {
            if (typ->getHeterogenousTuple()) {
              auto &ast = ctx->cache->functions[method.name].ast;
              if (ast->hasAttr("autogenerated") &&
                  (endswith(ast->name, ".__iter__:0") ||
                   endswith(ast->name, ".__getitem__:0"))) {
                // ignore __getitem__ and other heterogenuous methods
                continue;
              }
            }
            stmt->rhs = N<IdExpr>(method.name);
            block.push_back(wrap(stmt->clone()));
          }
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
      if (auto typ = args[0]->getRecord()) {
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
        error("staticenumerate needs a tuple");
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
      for (auto &f : getClassFields(typ.get())) {
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

      if (auto utyp = typ->getUnion()) {
        for (size_t i = 0; i < utyp->getRealizationTypes().size(); i++) {
          std::vector<StmtPtr> stmts;
          if (withIdx) {
            stmts.push_back(
                N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                              NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"))));
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
                              NT<IndexExpr>(NT<IdExpr>("Static"), NT<IdExpr>("int"))));
          }
          stmts.push_back(
              N<AssignStmt>(N<IdExpr>(vars[withIdx]), NT<IdExpr>(ta->realizedName())));
          auto b = N<SuiteStmt>(stmts);
          block.push_back(wrap(b));
          idx++;
        }
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
