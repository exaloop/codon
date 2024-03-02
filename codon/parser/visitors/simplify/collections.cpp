// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

/// Simple transformation.
/// The rest will be handled during the type-checking stage.
void SimplifyVisitor::visit(TupleExpr *expr) {
  for (auto &i : expr->items)
    transform(i, true); // types needed for some constructs (e.g., isinstance)
}

/// Simple transformation.
/// The rest will be handled during the type-checking stage.
void SimplifyVisitor::visit(ListExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}

/// Simple transformation.
/// The rest will be handled during the type-checking stage.
void SimplifyVisitor::visit(SetExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}

/// Simple transformation.
/// The rest will be handled during the type-checking stage.
void SimplifyVisitor::visit(DictExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}

/// Transform a collection comprehension to the corresponding statement expression.
/// @example (lists and sets):
///   `[i+a for i in j if a]` -> ```gen = List()
///                                 for i in j: if a: gen.append(i+a)```
/// Generators are transformed to lambda calls.
/// @example
///   `(i+a for i in j if a)` -> ```def _lambda(j, a):
///                                   for i in j: yield i+a
///                                 _lambda(j, a).__iter__()```
void SimplifyVisitor::visit(GeneratorExpr *expr) {
  std::vector<StmtPtr> stmts;

  auto loops = clone_nop(expr->loops); // Clone as loops will be modified

  // List comprehension optimization:
  // Use `iter.__len__()` when creating list if there is a single for loop
  // without any if conditions in the comprehension
  bool canOptimize = expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
                     loops[0].conds.empty();
  if (canOptimize) {
    auto iter = transform(clone(loops[0].gen));
    IdExpr *id;
    if (iter->getCall() && (id = iter->getCall()->expr->getId())) {
      // Turn off this optimization for static items
      canOptimize &= !startswith(id->value, "std.internal.types.range.staticrange");
      canOptimize &= !startswith(id->value, "statictuple");
    }
  }

  SuiteStmt *prev = nullptr;
  auto avoidDomination = true;
  std::swap(avoidDomination, ctx->avoidDomination);
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
  std::swap(avoidDomination, ctx->avoidDomination);
}

/// Transform a dictionary comprehension to the corresponding statement expression.
/// @example
///   `{i+a: j+1 for i in j if a}` -> ```gen = Dict()
///                                      for i in j: if a: gen.__setitem__(i+a, j+1)```
void SimplifyVisitor::visit(DictGeneratorExpr *expr) {
  SuiteStmt *prev = nullptr;
  auto avoidDomination = true;
  std::swap(avoidDomination, ctx->avoidDomination);
  auto suite = transformGeneratorBody(expr->loops, prev);

  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  prev->stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                                clone(expr->key), clone(expr->expr))));
  stmts.push_back(transform(suite));
  resultExpr = N<StmtExpr>(stmts, transform(var));
  std::swap(avoidDomination, ctx->avoidDomination);
}

/// Transforms a list of @c GeneratorBody loops to the corresponding set of for loops.
/// @example
///   `for i in j if a for k in i if a if b` ->
///   `for i in j: if a: for k in i: if a: if b: [prev]`
/// @param prev (out-argument): A pointer to the innermost block (suite) where the
///                             comprehension (or generator) expression should reside
StmtPtr SimplifyVisitor::transformGeneratorBody(const std::vector<GeneratorBody> &loops,
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
