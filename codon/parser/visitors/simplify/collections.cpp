// Copyright (C) 2022 Exaloop Inc. <https://exaloop.io>

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

  std::string optimizeVar;
  if (expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
      loops[0].conds.empty()) {
    // List comprehension optimization:
    // Use `iter.__len__()` when creating list if there is a single for loop
    // without any if conditions in the comprehension
    optimizeVar = ctx->cache->getTemporaryVar("i");
    stmts.push_back(transform(N<AssignStmt>(N<IdExpr>(optimizeVar), loops[0].gen)));
    loops[0].gen = N<IdExpr>(optimizeVar);
  }

  SuiteStmt *prev = nullptr;
  auto avoidDomination = true;
  std::swap(avoidDomination, ctx->avoidDomination);
  auto suite = transformGeneratorBody(loops, prev);
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    // List comprehensions
    std::vector<ExprPtr> args;
    if (!optimizeVar.empty()) {
      // Use special List.__init__(bool, [optimizeVar]) constructor
      args = {N<BoolExpr>(true), N<IdExpr>(optimizeVar)};
    }
    stmts.push_back(
        transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("List"), args))));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
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
    resultExpr = N<CallExpr>(N<DotExpr>(N<CallExpr>(makeAnonFn(stmts)), "__iter__"));
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
