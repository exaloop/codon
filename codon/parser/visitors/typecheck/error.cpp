#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Typecheck try-except statements.
void TypecheckVisitor::visit(TryStmt *stmt) {
  ctx->blockLevel++;
  transform(stmt->suite);
  ctx->blockLevel--;

  auto done = stmt->suite->isDone();
  for (auto &c : stmt->catches) {
    transformType(c.exc);
    if (!c.var.empty()) {
      // Handle dominated except bindings
      auto changed = in(ctx->cache->replacements, c.var);
      while (auto s = in(ctx->cache->replacements, c.var))
        c.var = s->first, changed = s;
      if (changed && changed->second) {
        auto update =
            N<AssignStmt>(N<IdExpr>(format("{}.__used__", c.var)), N<BoolExpr>(true));
        update->setUpdate();
        c.suite = N<SuiteStmt>(update, c.suite);
      }
      if (changed)
        c.exc->setAttr(ExprAttr::Dominated);
      auto val = ctx->find(c.var);
      if (!changed)
        val = ctx->add(TypecheckItem::Var, c.var, c.exc->getType());
      unify(val->type, c.exc->getType());
    }
    ctx->blockLevel++;
    transform(c.suite);
    ctx->blockLevel--;
    done &= (!c.exc || c.exc->isDone()) && c.suite->isDone();
  }
  if (stmt->finally) {
    ctx->blockLevel++;
    transform(stmt->finally);
    ctx->blockLevel--;
    done &= stmt->finally->isDone();
  }

  if (done)
    stmt->setDone();
}

/// Transform `raise` statements.
/// @example
///   `raise exc` -> ```raise __internal__.set_header(exc, "fn", "file", line, col)```
void TypecheckVisitor::visit(ThrowStmt *stmt) {
  transform(stmt->expr);

  if (!(stmt->expr->getCall() &&
        stmt->expr->getCall()->expr->isId("__internal__.set_header:0"))) {
    stmt->expr = transform(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("__internal__"), "set_header"), stmt->expr,
        N<StringExpr>(ctx->getRealizationBase()->name),
        N<StringExpr>(stmt->getSrcInfo().file), N<IntExpr>(stmt->getSrcInfo().line),
        N<IntExpr>(stmt->getSrcInfo().col)));
  }
  if (stmt->expr->isDone())
    stmt->setDone();
}

} // namespace codon::ast