#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(SliceExpr *expr) {
  resultExpr = N<SliceExpr>(transform(expr->start), transform(expr->stop),
                            transform(expr->step));
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

} // namespace codon::ast