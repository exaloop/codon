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
    if (auto es = i->getStar()) {
      items.emplace_back(N<StarExpr>(transform(es->what)));
    } else {
      items.emplace_back(transform(i));
    }
  }
  resultExpr = N<TupleExpr>(items);
}

void SimplifyVisitor::visit(ListExpr *expr) {
  ctx->addBlock(); // prevent tmp vars from being toplevel vars
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("list"));
  stmts.push_back(transform(N<AssignStmt>(
      clone(var),
      N<CallExpr>(N<IdExpr>("List"),
                  !expr->items.empty() ? N<IntExpr>(expr->items.size()) : nullptr))));
  for (const auto &it : expr->items) {
    if (auto star = it->getStar()) {
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      auto st = star->what->clone();
      st->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), st,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), clone(forVar))))));
    } else {
      auto st = clone(it);
      st->setAttr(ExprAttr::SequenceItem);
      stmts.push_back(
          transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "append"), st))));
    }
  }
  auto e = N<StmtExpr>(stmts, transform(var));
  e->setAttr(ExprAttr::List);
  resultExpr = e;
  ctx->popBlock();
}

void SimplifyVisitor::visit(SetExpr *expr) {
  ctx->addBlock(); // prevent tmp vars from being toplevel vars
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("set"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Set")))));
  for (auto &it : expr->items)
    if (auto star = it->getStar()) {
      ExprPtr forVar = N<IdExpr>(ctx->cache->getTemporaryVar("it"));
      auto st = star->what->clone();
      st->setAttr(ExprAttr::StarSequenceItem);
      stmts.push_back(transform(N<ForStmt>(
          clone(forVar), st,
          N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), clone(forVar))))));
    } else {
      auto st = clone(it);
      st->setAttr(ExprAttr::SequenceItem);
      stmts.push_back(
          transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "add"), st))));
    }
  auto e = N<StmtExpr>(stmts, transform(var));
  e->setAttr(ExprAttr::Set);
  resultExpr = e;
  ctx->popBlock();
}

void SimplifyVisitor::visit(DictExpr *expr) {
  ctx->addBlock(); // prevent tmp vars from being toplevel vars
  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("dict"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  for (auto &it : expr->items)
    if (auto star = CAST(it.value, KeywordStarExpr)) {
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
  auto e = N<StmtExpr>(stmts, transform(var));
  e->setAttr(ExprAttr::Dict);
  resultExpr = e;
  ctx->popBlock();
}

void SimplifyVisitor::visit(GeneratorExpr *expr) {
  ctx->addBlock(); // prevent tmp vars from being toplevel vars
  SuiteStmt *prev;
  std::vector<StmtPtr> stmts;

  auto loops = clone_nop(expr->loops);
  // List comprehension optimization: pass iter.__len__() if we only have a single for
  // loop without any conditions.
  std::string optimizeVar;
  if (expr->kind == GeneratorExpr::ListGenerator && loops.size() == 1 &&
      loops[0].conds.empty()) {
    optimizeVar = ctx->cache->getTemporaryVar("iter");
    stmts.push_back(transform(N<AssignStmt>(N<IdExpr>(optimizeVar), loops[0].gen)));
    loops[0].gen = N<IdExpr>(optimizeVar);
  }

  auto suite = transformGeneratorBody(loops, prev);
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  if (expr->kind == GeneratorExpr::ListGenerator) {
    std::vector<ExprPtr> args;
    // Use special List.__init__(bool, T) constructor.
    if (!optimizeVar.empty())
      args = {N<BoolExpr>(true), N<IdExpr>(optimizeVar)};
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
    prev->stmts.push_back(N<YieldStmt>(clone(expr->expr)));
    stmts.push_back(suite);
    resultExpr = N<CallExpr>(N<DotExpr>(N<CallExpr>(makeAnonFn(stmts)), "__iter__"));
  }
  ctx->popBlock();
}

void SimplifyVisitor::visit(DictGeneratorExpr *expr) {
  ctx->addBlock(); // prevent tmp vars from being toplevel vars
  SuiteStmt *prev;
  auto suite = transformGeneratorBody(expr->loops, prev);

  std::vector<StmtPtr> stmts;
  ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("gen"));
  stmts.push_back(transform(N<AssignStmt>(clone(var), N<CallExpr>(N<IdExpr>("Dict")))));
  prev->stmts.push_back(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__setitem__"),
                                                clone(expr->key), clone(expr->expr))));
  stmts.push_back(transform(suite));
  resultExpr = N<StmtExpr>(stmts, transform(var));
  ctx->popBlock();
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