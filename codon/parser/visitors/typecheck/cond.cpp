#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

void TypecheckVisitor::visit(IfExpr *expr) {
  expr->cond = transform(expr->cond);
  if (expr->cond->isStatic()) {
    if (expr->cond->staticValue.evaluated) {
      bool isTrue = false;
      if (expr->cond->staticValue.type == StaticValue::STRING)
        isTrue = !expr->cond->staticValue.getString().empty();
      else
        isTrue = expr->cond->staticValue.getInt();
      resultExpr =
          transform(isTrue ? expr->ifexpr : expr->elsexpr, false);
      unify(expr->type, resultExpr->getType());
    } else {
      auto i = clone(expr->ifexpr), e = clone(expr->elsexpr);
      i = transform(i, false);
      e = transform(e, false);
      unify(expr->type, ctx->addUnbound(expr, ctx->typecheckLevel));
      if (i->isStatic() && e->isStatic()) {
        expr->staticValue.type = i->staticValue.type;
        unify(expr->type,
              ctx->getType(expr->staticValue.type == StaticValue::INT ? "int"
                                                                           : "str"));
      }
      expr->done = false; // do not typecheck this suite yet
    }
    return;
  }

  expr->ifexpr = transform(expr->ifexpr, false);
  expr->elsexpr = transform(expr->elsexpr, false);
  if (expr->cond->type->getClass() && !expr->cond->type->is("bool"))
    expr->cond = transform(N<CallExpr>(N<DotExpr>(expr->cond, "__bool__")));
  wrapOptionalIfNeeded(expr->ifexpr->getType(), expr->elsexpr);
  wrapOptionalIfNeeded(expr->elsexpr->getType(), expr->ifexpr);
  unify(expr->type, expr->ifexpr->getType());
  unify(expr->type, expr->elsexpr->getType());
  expr->ifexpr = transform(expr->ifexpr);
  expr->elsexpr = transform(expr->elsexpr);
  expr->done = expr->cond->done && expr->ifexpr->done && expr->elsexpr->done;
}

void TypecheckVisitor::visit(IfStmt *stmt) {
  stmt->cond = transform(stmt->cond);
  if (stmt->cond->isStatic()) {
    if (!stmt->cond->staticValue.evaluated) {
      stmt->done = false; // do not typecheck this suite yet
      return;
    } else {
      bool isTrue = false;
      if (stmt->cond->staticValue.type == StaticValue::STRING)
        isTrue = !stmt->cond->staticValue.getString().empty();
      else
        isTrue = stmt->cond->staticValue.getInt();
      resultStmt = isTrue ? stmt->ifSuite : stmt->elseSuite;
      resultStmt = transform(resultStmt);
      if (!resultStmt)
        resultStmt = transform(N<SuiteStmt>());
      return;
    }
  } else {
    if (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
      stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));
    ctx->blockLevel++;
    stmt->ifSuite = transform(stmt->ifSuite);
    stmt->elseSuite = transform(stmt->elseSuite);
    ctx->blockLevel--;
    stmt->done = stmt->cond->done && (!stmt->ifSuite || stmt->ifSuite->done) &&
                 (!stmt->elseSuite || stmt->elseSuite->done);
  }
}

}