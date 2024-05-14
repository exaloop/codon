// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Call `ready` and `notReady` depending whether the provided static expression can be
/// evaluated or not.
template <typename TT, typename TF>
auto evaluateStaticCondition(Expr *cond, TT ready, TF notReady) {
  seqassertn(cond->type->isStaticType(), "not a static condition");
  if (cond->type->canRealize()) {
    bool isTrue = false;
    if (cond->type->getStrStatic())
      isTrue = !cond->type->getStrStatic()->value.empty();
    else if (cond->type->getIntStatic())
      isTrue = cond->type->getIntStatic()->value;
    else if (cond->type->getBoolStatic())
      isTrue = cond->type->getBoolStatic()->value;
    return ready(isTrue);
  } else {
    return notReady();
  }
}

/// Only allowed in @c MatchStmt
void TypecheckVisitor::visit(RangeExpr *expr) {
  E(Error::UNEXPECTED_TYPE, expr, "range");
}

/// Typecheck if expressions. Evaluate static if blocks if possible.
/// Also wrap the condition with `__bool__()` if needed and wrap both conditional
/// expressions. See @c wrapExpr for more details.
void TypecheckVisitor::visit(IfExpr *expr) {
  // C++ call order is not defined; make sure to transform the conditional first
  expr->cond = transform(expr->cond);
  // Static if evaluation
  if (expr->cond->type->isStaticType()) {
    resultExpr = evaluateStaticCondition(
        expr->cond,
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          return transform(isTrue ? expr->ifexpr : expr->elsexpr);
        },
        [&]() -> Expr * { return nullptr; });
    if (resultExpr)
      unify(expr->type, resultExpr->getType());
    else
      expr->type->getUnbound()->isStatic = 1; // TODO: determine later!
    return;
  }

  expr->ifexpr = transform(expr->ifexpr);
  expr->elsexpr = transform(expr->elsexpr);

  // Add __bool__ wrapper
  while (expr->cond->type->getClass() && !expr->cond->type->is("bool"))
    expr->cond = transform(N<CallExpr>(N<DotExpr>(expr->cond, "__bool__")));
  // Add wrappers and unify both sides
  if (expr->ifexpr->type->getStatic())
    expr->ifexpr->type = expr->ifexpr->type->getStatic()->getNonStaticType();
  if (expr->elsexpr->type->getStatic())
    expr->elsexpr->type = expr->elsexpr->type->getStatic()->getNonStaticType();
  wrapExpr(&expr->elsexpr, expr->ifexpr->getType(), nullptr, /*allowUnwrap*/ false);
  wrapExpr(&expr->ifexpr, expr->elsexpr->getType(), nullptr, /*allowUnwrap*/ false);

  unify(expr->type, expr->ifexpr->getType());
  unify(expr->type, expr->elsexpr->getType());
  if (expr->cond->isDone() && expr->ifexpr->isDone() && expr->elsexpr->isDone())
    expr->setDone();
}

/// Typecheck if statements. Evaluate static if blocks if possible.
/// Also wrap the condition with `__bool__()` if needed.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(IfStmt *stmt) {
  stmt->cond = transform(stmt->cond);

  // Static if evaluation
  if (stmt->cond->type->isStaticType()) {
    resultStmt = evaluateStaticCondition(
        stmt->cond,
        [&](bool isTrue) {
          LOG_TYPECHECK("[static::cond] {}: {}", getSrcInfo(), isTrue);
          auto t = transform(isTrue ? stmt->ifSuite : stmt->elseSuite);
          return t ? t : transform(N<SuiteStmt>());
        },
        [&]() -> Stmt * { return nullptr; });
    return;
  }

  while (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
    stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));
  ctx->blockLevel++;
  stmt->ifSuite = transform(stmt->ifSuite);
  stmt->elseSuite = transform(stmt->elseSuite);
  ctx->blockLevel--;

  if (stmt->cond->isDone() && (!stmt->ifSuite || stmt->ifSuite->isDone()) &&
      (!stmt->elseSuite || stmt->elseSuite->isDone()))
    stmt->setDone();
}

/// Simplify match statement by transforming it into a series of conditional statements.
/// @example
///   ```match e:
///        case pattern1: ...
///        case pattern2 if guard: ...
///        ...``` ->
///   ```_match = e
///      while True:  # used to simulate goto statement with break
///        [pattern1 transformation]: (...; break)
///        [pattern2 transformation]: if guard: (...; break)
///        ...
///        break  # exit the loop no matter what```
/// The first pattern that matches the given expression will be used; other patterns
/// will not be used (i.e., there is no fall-through). See @c transformPattern for
/// pattern transformations
void TypecheckVisitor::visit(MatchStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("match");
  auto result = N<SuiteStmt>();
  result->stmts.push_back(transform(N<AssignStmt>(N<IdExpr>(var), clone(stmt->what))));
  for (auto &c : stmt->cases) {
    Stmt *suite = N<SuiteStmt>(clone(c.suite), N<BreakStmt>());
    if (c.guard)
      suite = N<IfStmt>(clone(c.guard), suite);
    result->stmts.push_back(transformPattern(N<IdExpr>(var), clone(c.pattern), suite));
  }
  // Make sure to break even if there is no case _ to prevent infinite loop
  result->stmts.push_back(N<BreakStmt>());
  resultStmt = transform(N<WhileStmt>(N<BoolExpr>(true), result));
}

/// Transform a match pattern into a series of if statements.
/// @example
///   `case True`          -> `if isinstance(var, "bool"): if var == True`
///   `case 1`             -> `if isinstance(var, "int"): if var == 1`
///   `case 1...3`         -> ```if isinstance(var, "int"):
///                                if var >= 1: if var <= 3```
///   `case (1, pat)`      -> ```if isinstance(var, "Tuple"): if staticlen(var) == 2:
///                                 if match(var[0], 1): if match(var[1], pat)```
///   `case [1, ..., pat]` -> ```if isinstance(var, "List"): if len(var) >= 2:
///                                 if match(var[0], 1): if match(var[-1], pat)```
///   `case 1 or pat`      -> `if match(var, 1): if match(var, pat)`
///                           (note: pattern suite is cloned for each `or`)
///   `case (x := pat)`    -> `(x := var; if match(var, pat))`
///   `case x`             -> `(x := var)`
///                           (only when `x` is not '_')
///   `case expr`          -> `if hasattr(typeof(var), "__match__"): if
///   var.__match__(foo())`
///                           (any expression that does not fit above patterns)
Stmt *TypecheckVisitor::transformPattern(Expr *var, Expr *pattern, Stmt *suite) {
  // Convenience function to generate `isinstance(e, typ)` calls
  auto isinstance = [&](Expr *e, const std::string &typ) -> Expr * {
    return N<CallExpr>(N<IdExpr>("isinstance"), clone(e), N<IdExpr>(typ));
  };
  // Convenience function to find the index of an ellipsis within a list pattern
  auto findEllipsis = [&](const std::vector<Expr *> &items) {
    size_t i = items.size();
    for (auto it = 0; it < items.size(); it++)
      if (items[it]->getEllipsis()) {
        if (i != items.size())
          E(Error::MATCH_MULTI_ELLIPSIS, items[it], "multiple ellipses in pattern");
        i = it;
      }
    return i;
  };

  // See the above examples for transformation details
  if (pattern->getInt() || CAST(pattern, BoolExpr)) {
    // Bool and int patterns
    return N<IfStmt>(isinstance(var, CAST(pattern, BoolExpr) ? "bool" : "int"),
                     N<IfStmt>(N<BinaryExpr>(clone(var), "==", pattern), suite));
  } else if (auto er = CAST(pattern, RangeExpr)) {
    // Range pattern
    return N<IfStmt>(
        isinstance(var, "int"),
        N<IfStmt>(N<BinaryExpr>(clone(var), ">=", clone(er->start)),
                  N<IfStmt>(N<BinaryExpr>(clone(var), "<=", clone(er->stop)), suite)));
  } else if (auto et = pattern->getTuple()) {
    // Tuple pattern
    for (auto it = et->items.size(); it-- > 0;) {
      suite = transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it)),
                               clone(et->items[it]), suite);
    }
    return N<IfStmt>(
        isinstance(var, "Tuple"),
        N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("staticlen"), clone(var)),
                                "==", N<IntExpr>(et->items.size())),
                  suite));
  } else if (auto el = pattern->getList()) {
    // List pattern
    auto ellipsis = findEllipsis(el->items), sz = el->items.size();
    std::string op;
    if (ellipsis == el->items.size()) {
      op = "==";
    } else {
      op = ">=", sz -= 1;
    }
    for (auto it = el->items.size(); it-- > ellipsis + 1;) {
      suite =
          transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it - el->items.size())),
                           clone(el->items[it]), suite);
    }
    for (auto it = ellipsis; it-- > 0;) {
      suite = transformPattern(N<IndexExpr>(clone(var), N<IntExpr>(it)),
                               clone(el->items[it]), suite);
    }
    return N<IfStmt>(isinstance(var, "List"),
                     N<IfStmt>(N<BinaryExpr>(N<CallExpr>(N<IdExpr>("len"), clone(var)),
                                             op, N<IntExpr>(sz)),
                               suite));
  } else if (auto eb = pattern->getBinary()) {
    // Or pattern
    if (eb->op == "|" || eb->op == "||") {
      return N<SuiteStmt>(transformPattern(clone(var), clone(eb->lexpr), clone(suite)),
                          transformPattern(clone(var), clone(eb->rexpr), suite));
    }
  } else if (auto ei = pattern->getId()) {
    // Wildcard pattern
    if (ei->value != "_") {
      return N<SuiteStmt>(N<AssignStmt>(clone(pattern), clone(var)), suite);
    } else {
      return suite;
    }
  } else {
    auto se = pattern->getStmtExpr();
    if (se && se->stmts.size() == 1) {
      AssignStmt *a = nullptr;
      if (se->stmts[0]->getSuite() && se->stmts[0]->getSuite()->stmts.size() >= 1 &&
          (a = se->stmts[0]->getSuite()->stmts[0]->getAssign())) {
        seqassert(a->lhs->getId(), "only simple assignment expressions are supported");
        auto stmts = se->stmts[0]->getSuite()->stmts;
        stmts.push_back(transformPattern(clone(var), clone(a->rhs), clone(suite)));
        a->rhs = clone(var);
        return N<SuiteStmt>(stmts);
      }
    }
  }
  pattern = transform(pattern); // transform to check for pattern errors
  if (pattern->getEllipsis())
    pattern = N<CallExpr>(N<IdExpr>("ellipsis"));
  // Fallback (`__match__`) pattern
  auto p = N<IfStmt>(
      N<CallExpr>(N<IdExpr>("hasattr"), clone(var), N<StringExpr>("__match__"),
                  clone(pattern)),
      N<IfStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__match__"), pattern), suite));
  return p;
}

} // namespace codon::ast
