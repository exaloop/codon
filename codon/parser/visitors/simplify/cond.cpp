#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(IfExpr *expr) {
  // C++ call order is not defined; make sure to transform the conditional first
  auto cond = transform(expr->cond);
  auto tmp = ctx->shortCircuit;
  ctx->shortCircuit = true;
  auto ifexpr = transform(expr->ifexpr);
  auto elsexpr = transform(expr->elsexpr);
  ctx->shortCircuit = tmp;
  resultExpr = N<IfExpr>(cond, ifexpr, elsexpr);
}

void SimplifyVisitor::visit(IfStmt *stmt) {
  seqassert(stmt->cond, "invalid if statement");

  // C++ call order is not defined; make sure to transform the conditional first
  auto cond = transform(stmt->cond);
  resultStmt = N<IfStmt>(cond, transformInScope(stmt->ifSuite),
                         transformInScope(stmt->elseSuite));
}

void SimplifyVisitor::visit(MatchStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("match");
  auto result = N<SuiteStmt>();
  result->stmts.push_back(N<AssignStmt>(N<IdExpr>(var), clone(stmt->what)));
  for (auto &c : stmt->cases) {
    ctx->addScope();
    StmtPtr suite = N<SuiteStmt>(clone(c.suite), N<BreakStmt>());
    if (c.guard)
      suite = N<IfStmt>(clone(c.guard), suite);
    result->stmts.push_back(transformPattern(N<IdExpr>(var), clone(c.pattern), suite));
    ctx->popScope();
  }
  result->stmts.push_back(N<BreakStmt>()); // break even if there is no case _.
  resultStmt = transform(N<WhileStmt>(N<BoolExpr>(true), result));
}

StmtPtr SimplifyVisitor::transformPattern(ExprPtr var, ExprPtr pattern, StmtPtr suite) {
  auto isinstance = [&](const ExprPtr &e, const std::string &typ) -> ExprPtr {
    return N<CallExpr>(N<IdExpr>("isinstance"), e->clone(), N<IdExpr>(typ));
  };
  auto findEllipsis = [&](const std::vector<ExprPtr> &items) {
    size_t i = items.size();
    for (auto it = 0; it < items.size(); it++)
      if (items[it]->getEllipsis()) {
        if (i != items.size())
          error("cannot have multiple ranges in a pattern");
        i = it;
      }
    return i;
  };

  if (pattern->getInt() || CAST(pattern, BoolExpr)) {
    return N<IfStmt>(isinstance(var, CAST(pattern, BoolExpr) ? "bool" : "int"),
                     N<IfStmt>(N<BinaryExpr>(var->clone(), "==", pattern), suite));
  } else if (auto er = CAST(pattern, RangeExpr)) {
    return N<IfStmt>(
        isinstance(var, "int"),
        N<IfStmt>(
            N<BinaryExpr>(var->clone(), ">=", clone(er->start)),
            N<IfStmt>(N<BinaryExpr>(var->clone(), "<=", clone(er->stop)), suite)));
  } else if (auto et = pattern->getTuple()) {
    for (auto it = et->items.size(); it-- > 0; ) {
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
                               clone(et->items[it]), suite);
    }
    return N<IfStmt>(
        isinstance(var, "Tuple"),
        N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("staticlen"), clone(var)),
                                "==", N<IntExpr>(et->items.size())),
                  suite));
  } else if (auto el = pattern->getList()) {
    auto ellipsis = findEllipsis(el->items), sz = el->items.size();
    std::string op;
    if (ellipsis == el->items.size()) {
      op = "==";
    } else {
      op = ">=", sz -= 1;
    }
    for (auto it = el->items.size(); it-- > ellipsis + 1; ) {
      suite = transformPattern(
          N<IndexExpr>(var->clone(), N<IntExpr>(it - el->items.size())),
          clone(el->items[it]), suite);
    }
    for (auto it = ellipsis; it-- > 0; ) {
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
                               clone(el->items[it]), suite);
    }
    return N<IfStmt>(isinstance(var, "List"),
                     N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("len"), clone(var)),
                                             op, N<IntExpr>(sz)),
                               suite));
  } else if (auto eb = pattern->getBinary()) {
    if (eb->op == "|") {
      return N<SuiteStmt>(transformPattern(clone(var), clone(eb->lexpr), clone(suite)),
                          transformPattern(clone(var), clone(eb->rexpr), suite));
    }
  } else if (auto ea = CAST(pattern, AssignExpr)) {
    seqassert(ea->var->getId(), "only simple assignment expressions are supported");
    return N<SuiteStmt>(
        std::vector<StmtPtr>{
            N<AssignStmt>(clone(ea->var), clone(var)),
            transformPattern(clone(var), clone(ea->expr), clone(suite))},
        true);
  } else if (auto ei = pattern->getId()) {
    if (ei->value != "_") {
      return N<SuiteStmt>(
          std::vector<StmtPtr>{N<AssignStmt>(clone(pattern), clone(var)), suite}, true);
    } else {
      return suite;
    }
  }
  pattern = transform(pattern); // call transform to check for errors
  return N<IfStmt>(
      N<CallExpr>(N<IdExpr>("hasattr"), var->clone(), N<StringExpr>("__match__"),
                  N<CallExpr>(N<IdExpr>("type"), pattern->clone())),
      N<IfStmt>(N<CallExpr>(N<DotExpr>(var->clone(), "__match__"), pattern), suite));
}

} // namespace codon::ast