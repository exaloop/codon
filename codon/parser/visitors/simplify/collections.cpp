#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(TupleExpr *expr) {
  std::vector<ExprPtr> items;
  for (auto &i : expr->items) {
    items.emplace_back(transform(i));
  }
  resultExpr = N<TupleExpr>(items);
}

void SimplifyVisitor::visit(ListExpr *expr) {
  resultExpr = transformComprehension("List", "append", expr->items);
  resultExpr->setAttr(ExprAttr::List);
}

void SimplifyVisitor::visit(SetExpr *expr) {
  resultExpr = transformComprehension("Set", "add", expr->items);
  resultExpr->setAttr(ExprAttr::Set);
}

ExprPtr SimplifyVisitor::transformComprehension(const std::string &type,
                                                const std::string &fn,
                                                const std::vector<ExprPtr> &items) {
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));

  std::vector<ExprPtr> constructorArgs{};
  if (type == "List" && !items.empty())
    constructorArgs.push_back(N<IntExpr>(items.size()));
  stmts.push_back(transform(
      N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>(type), constructorArgs))));
  for (const auto &it : items) {
    if (auto star = it->getStar()) {
      // unpack star-expression by iterating it
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("i"));
      auto st = star->what->clone();
      st->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), st,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), clone(forVar))))));
    } else {
      auto st = clone(it);
      st->setAttr(ExprAttr::SequenceItem);
      stmts.push_back(
          transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), fn), st))));
    }
  }
  return N<StmtExpr>(stmts, transform(var));
}

void SimplifyVisitor::visit(DictExpr *expr) {
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("cont"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  for (auto &it : expr->items) {
    if (auto star = CAST(it.value, KeywordStarExpr)) {
      // unpack kwstar-expression by iterating it
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      auto st = star->what->clone();
      st->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), N<CallExpr>(N<DotExpr>(st, "items")),
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(0)),
                                  N<IndexExpr>(clone(forVar), N<IntExpr>(1)))))));
    } else {
      auto k = clone(it.key);
      k->setAttr(ExprAttr::SequenceItem);
      auto v = clone(it.value);
      v->setAttr(ExprAttr::SequenceItem);
      stmts.push_back(transform(
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"), k, v))));
    }
  }
  resultExpr = N<StmtExpr>(stmts, transform(var));
  resultExpr->setAttr(ExprAttr::Dict);
}

void SimplifyVisitor::visit(GeneratorExpr *expr) {
  std::vector<StmtPtr> stmts;

  auto loops = clone_nop(expr->loops);
  // List comprehension optimization:
  // pass iter.__len__() if we only have a single for loop without any conditions.
  std::string optimizeVar;
  if (expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
      loops[0].conds.empty()) {
    optimizeVar = ctx->cache->getTemporaryVar("i");
    stmts.push_back(transform(N<AssignStmt>(N<IdExpr>(optimizeVar), loops[0].gen)));
    loops[0].gen = N<IdExpr>(optimizeVar);
  }

  SuiteStmt *prev;
  auto suite = transformGeneratorBody(loops, prev);
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    std::vector<ExprPtr> args;
    if (!optimizeVar.empty()) {
      // Use special List.__init__(bool, T) constructor.
      args = {N<BoolExpr>(true), N<IdExpr>(optimizeVar)};
    }
    stmts.push_back(
        transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("List"), args))));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
  } else if (expr->kind == GeneratorExpr::SetGenerator) {
    stmts.push_back(
        transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")))));
    prev->stmts.push_back(
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(expr->expr))));
    stmts.push_back(transform(suite));
    resultExpr = N<StmtExpr>(stmts, transform(var));
  } else {
    // Standard generators are converted to lambda functions that yield the target expression.
    prev->stmts.push_back(N<YieldStmt>(clone(expr->expr)));
    stmts.push_back(suite);
    resultExpr = N<CallExpr>(N<DotExpr>(N<CallExpr>(makeAnonFn(stmts)), "__iter__"));
  }
}

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