#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(LambdaExpr *expr) {
  resultExpr =
      makeAnonFn(std::vector<StmtPtr>{N<ReturnStmt>(clone(expr->expr))}, expr->vars);
}

void SimplifyVisitor::visit(SliceExpr *expr) {
  resultExpr = N<SliceExpr>(transform(expr->start), transform(expr->stop),
                            transform(expr->step));
}

void SimplifyVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");
  if (ctx->shortCircuit)
    error("assignment expression in a short-circuiting subexpression");
  resultExpr = transform(N<StmtExpr>(
      std::vector<StmtPtr>{N<AssignStmt>(clone(expr->var), clone(expr->expr))},
      clone(expr->var)));
}

void SimplifyVisitor::visit(StmtExpr *expr) {
  std::vector<StmtPtr> stmts;
  for (auto &s : expr->stmts)
    stmts.emplace_back(transform(s));
  auto e = transform(expr->expr);
  auto s = N<StmtExpr>(stmts, e);
  s->attributes = expr->attributes;
  resultExpr = s;
}

void SimplifyVisitor::visit(ExprStmt *stmt) {
  resultStmt = N<ExprStmt>(transform(stmt->expr, true));
}

void SimplifyVisitor::visit(PrintStmt *stmt) {
  std::vector<CallExpr::Arg> args;
  for (auto &i : stmt->items)
    args.emplace_back(CallExpr::Arg{"", transform(i)});
  if (stmt->isInline)
    args.emplace_back(CallExpr::Arg{"end", N<StringExpr>(" ")});
  resultStmt = N<ExprStmt>(N<CallExpr>(transform(N<IdExpr>("print")), args));
}

void SimplifyVisitor::visit(YieldFromStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("yield");
  resultStmt = transform(
      N<ForStmt>(N<IdExpr>(var), clone(stmt->expr), N<YieldStmt>(N<IdExpr>(var))));
}

void SimplifyVisitor::visit(AssertStmt *stmt) {
  ExprPtr msg = N<StringExpr>("");
  if (stmt->message)
    msg = N<CallExpr>(N<IdExpr>("str"), clone(stmt->message));
  if (ctx->getLevel() && (ctx->bases.back().attributes & FLAG_TEST))
    resultStmt = transform(
        N<IfStmt>(N<UnaryExpr>("!", clone(stmt->expr)),
                  N<ExprStmt>(N<CallExpr>(N<DotExpr>("__internal__", "seq_assert_test"),
                                          N<StringExpr>(stmt->getSrcInfo().file),
                                          N<IntExpr>(stmt->getSrcInfo().line), msg))));
  else
    resultStmt = transform(
        N<IfStmt>(N<UnaryExpr>("!", clone(stmt->expr)),
                  N<ThrowStmt>(N<CallExpr>(N<DotExpr>("__internal__", "seq_assert"),
                                           N<StringExpr>(stmt->getSrcInfo().file),
                                           N<IntExpr>(stmt->getSrcInfo().line), msg))));
}

void SimplifyVisitor::visit(CustomStmt *stmt) {
  if (stmt->suite) {
    auto fn = ctx->cache->customBlockStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customBlockStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second.second(this, stmt);
  } else {
    auto fn = ctx->cache->customExprStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customExprStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second(this, stmt);
  }
}

ExprPtr SimplifyVisitor::makeAnonFn(std::vector<StmtPtr> stmts,
                                    const std::vector<std::string> &argNames) {
  std::vector<Param> params;
  std::string name = ctx->cache->getTemporaryVar("lambda");
  for (auto &s : argNames)
    params.emplace_back(Param{s, nullptr, nullptr});
  auto s = transform(N<FunctionStmt>(name, nullptr, params, N<SuiteStmt>(move(stmts)),
                                     Attr({Attr::Capture})));
  if (s)
    return N<StmtExpr>(s, transform(N<IdExpr>(name)));
  return transform(N<IdExpr>(name));
}

} // namespace codon::ast