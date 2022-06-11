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
  std::vector<ExprPtr> items;
  for (auto &i : expr->items) {
    items.emplace_back(transform(i));
  }
  resultExpr = N<TupleExpr>(items);
}

/// Transform a list `[a1, ..., aN]` to the corresponding statement expression.
/// See @c transformComprehension
void SimplifyVisitor::visit(ListExpr *expr) {
  resultExpr = transformComprehension("List", "append", expr->items);
  resultExpr->setAttr(ExprAttr::List);
}

/// Transform a set `{a1, ..., aN}` to the corresponding statement expression.
/// See @c transformComprehension
void SimplifyVisitor::visit(SetExpr *expr) {
  resultExpr = transformComprehension("Set", "add", expr->items);
  resultExpr->setAttr(ExprAttr::Set);
}

/// Transform a collection of type `type` to a statement expression:
///   `[a1, ..., aN]` -> `cont = [type](); (cont.[fn](a1); ...); cont`
/// Any star-expression within the collection will be expanded:
///   `[a, *b]` -> `cont.[fn](a); for i in b: cont.[fn](i)`.
/// @example
///   `[a, *b, c]` -> ```cont = List(3)
///                      cont.append(a)
///                      for i in b: cont.append(i)
///                      cont.append(c)```
///   `{a, *b, c}` -> ```cont = Set()
///                      cont.add(a)
///                      for i in b: cont.add(i)
///                      cont.add(c)```
ExprPtr SimplifyVisitor::transformComprehension(const std::string &type,
                                                const std::string &fn,
                                                const std::vector<ExprPtr> &items) {
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));

  std::vector<ExprPtr> constructorArgs{};
  if (type == "List" && !items.empty()) {
    // Optimization: pre-allocate the list with the exact number of elements
    constructorArgs.push_back(N<IntExpr>(items.size()));
  }
  stmts.push_back(transform(
      N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>(type), constructorArgs))));
  for (const auto &it : items) {
    if (auto star = it->getStar()) {
      // Unpack star-expression by iterating over it
      // `*star` -> `for i in star: cont.[fn](i)`
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("i"));
      auto st = star->what->clone();
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), st,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar))))));
      // Set the proper attributes
      st->setAttr(ExprAttr::StarSequenceItem);
    } else {
      auto st = clone(it);
      stmts.push_back(
          transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), st))));
      // Set the proper attributes
      st->setAttr(ExprAttr::SequenceItem);
    }
  }
  return N<StmtExpr>(stmts, transform(var));
}

/// Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement expression.
/// Any kwstar-expression within the collection will be expanded.
/// @example
///   `{a: 1, **d}` -> ```cont = Dict()
///                       cont.__setitem__(a, 1)
///                       for i in b.items(): cont.__setitem__(i[0], i[i])```
void SimplifyVisitor::visit(DictExpr *expr) {
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  for (auto &it : expr->items) {
    if (auto star = CAST(it.value, KeywordStarExpr)) {
      // Expand kwstar-expression by iterating over it: see the example above
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      auto st = star->what->clone();
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), N<CallExpr>(N<DotExpr>(st, "items")),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1)))))));
      // Set the proper attributes
      st->setAttr(ExprAttr::StarSequenceItem);
    } else {
      auto k = clone(it.key);
      auto v = clone(it.value);
      stmts.push_back(transform(
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"), k, v))));
      // Set the proper attributes
      k->setAttr(ExprAttr::SequenceItem);
      v->setAttr(ExprAttr::SequenceItem);
    }
  }
  resultExpr = N<StmtExpr>(stmts, transform(var));
  resultExpr->setAttr(ExprAttr::Dict);
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

  SuiteStmt *prev;
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
}

/// Transform a dictionary comprehension to the corresponding statement expression.
/// @example
///   `{i+a: j+1 for i in j if a}` -> ```gen = Dict()
///                                      for i in j: if a: gen.__setitem__(i+a, j+1)```
void SimplifyVisitor::visit(DictGeneratorExpr *expr) {
  SuiteStmt *prev;
  auto suite = transformGeneratorBody(expr->loops, prev);

  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  prev->stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                                clone(expr->key), clone(expr->expr))));
  stmts.push_back(transform(suite));
  resultExpr = N<StmtExpr>(stmts, transform(var));
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
  prev = (SuiteStmt *)suite.get();
  for (auto &l : loops) {
    newSuite = N<SuiteStmt>();
    auto nextPrev = (SuiteStmt *)newSuite.get();

    prev->stmts.push_back(N<ForStmt>(l.vars->clone(), l.gen->clone(), newSuite));
    prev = nextPrev;
    for (auto &cond : l.conds) {
      newSuite = N<SuiteStmt>();
      nextPrev = (SuiteStmt *)newSuite.get();
      prev->stmts.push_back(N<IfStmt>(cond->clone(), newSuite));
      prev = nextPrev;
    }
  }
  return suite;
}

} // namespace codon::ast