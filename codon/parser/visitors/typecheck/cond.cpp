// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Call `ready` and `notReady` depending whether the provided static expression can be
/// evaluated or not.
template <typename TT, typename TF>
auto evaluateStaticCondition(const ExprPtr &cond, TT ready, TF notReady) {
  seqassertn(cond->isStatic(), "not a static condition");
  if (cond->staticValue.evaluated) {
    bool isTrue = false;
    if (cond->staticValue.type == StaticValue::STRING)
      isTrue = !cond->staticValue.getString().empty();
    else
      isTrue = cond->staticValue.getInt();
    return ready(isTrue);
  } else {
    return notReady();
  }
}

/// Typecheck if expressions. Evaluate static if blocks if possible.
/// Also wrap the condition with `__bool__()` if needed and wrap both conditional
/// expressions. See @c wrapExpr for more details.
void TypecheckVisitor::visit(IfExpr *expr) {
  transform(expr->cond);

  // Static if evaluation
  if (expr->cond->isStatic()) {
    resultExpr = evaluateStaticCondition(
        expr->cond,
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          return transform(isTrue ? expr->ifexpr : expr->elsexpr);
        },
        [&]() -> ExprPtr {
          // Check if both subexpressions are static; if so, this if expression is also
          // static and should be marked as such
          auto i = transform(clone(expr->ifexpr));
          auto e = transform(clone(expr->elsexpr));
          if (i->isStatic() && e->isStatic()) {
            expr->staticValue.type = i->staticValue.type;
            unify(expr->type,
                  ctx->getType(expr->staticValue.type == StaticValue::INT ? "int"
                                                                          : "str"));
          }
          return nullptr;
        });
    if (resultExpr)
      unify(expr->type, resultExpr->getType());
    else
      unify(expr->type, ctx->getUnbound());
    return;
  }

  transform(expr->ifexpr);
  transform(expr->elsexpr);
  // Add __bool__ wrapper
  while (expr->cond->type->getClass() && !expr->cond->type->is("bool"))
    expr->cond = transform(N<CallExpr>(N<DotExpr>(expr->cond, "__bool__")));
  // Add wrappers and unify both sides
  wrapExpr(expr->elsexpr, expr->ifexpr->getType(), nullptr, /*allowUnwrap*/ false);
  wrapExpr(expr->ifexpr, expr->elsexpr->getType(), nullptr, /*allowUnwrap*/ false);
  unify(expr->type, expr->ifexpr->getType());
  unify(expr->type, expr->elsexpr->getType());

  if (expr->cond->isDone() && expr->ifexpr->isDone() && expr->elsexpr->isDone())
    expr->setDone();
}

/// Typecheck if statements. Evaluate static if blocks if possible.
/// Also wrap the condition with `__bool__()` if needed.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(IfStmt *stmt) {
  transform(stmt->cond);

  // Static if evaluation
  if (stmt->cond->isStatic()) {
    resultStmt = evaluateStaticCondition(
        stmt->cond,
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          auto t = transform(isTrue ? stmt->ifSuite : stmt->elseSuite);
          return t ? t : transform(N<SuiteStmt>());
        },
        [&]() -> StmtPtr { return nullptr; });
    return;
  }

  while (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
    stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));
  ctx->blockLevel++;
  transform(stmt->ifSuite);
  transform(stmt->elseSuite);
  ctx->blockLevel--;

  if (stmt->cond->isDone() && (!stmt->ifSuite || stmt->ifSuite->isDone()) &&
      (!stmt->elseSuite || stmt->elseSuite->isDone()))
    stmt->setDone();
}

} // namespace codon::ast
