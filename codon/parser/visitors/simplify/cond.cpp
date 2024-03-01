// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

void SimplifyVisitor::visit(IfExpr *expr) {
  // C++ call order is not defined; make sure to transform the conditional first
  transform(expr->cond);
  auto tmp = ctx->isConditionalExpr;
  // Ensure that ifexpr and elsexpr are set as a potential short-circuit expressions.
  // Needed to ensure that variables defined within these expressions are properly
  // checked for their existence afterwards
  // (e.g., `x` will be created within `a if cond else (x := b)`
  // only if `cond` is not true)
  ctx->isConditionalExpr = true;
  transform(expr->ifexpr);
  transform(expr->elsexpr);
  ctx->isConditionalExpr = tmp;
}

void SimplifyVisitor::visit(IfStmt *stmt) {
  seqassert(stmt->cond, "invalid if statement");
  transform(stmt->cond);
  // Ensure that conditional suites are marked and transformed in their own scope
  transformConditionalScope(stmt->ifSuite);
  transformConditionalScope(stmt->elseSuite);
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
void SimplifyVisitor::visit(MatchStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("match");
  auto result = N<SuiteStmt>();
  result->stmts.push_back(N<AssignStmt>(N<IdExpr>(var), clone(stmt->what)));
  for (auto &c : stmt->cases) {
    ctx->enterConditionalBlock();
    StmtPtr suite = N<SuiteStmt>(clone(c.suite), N<BreakStmt>());
    if (c.guard)
      suite = N<IfStmt>(clone(c.guard), suite);
    result->stmts.push_back(transformPattern(N<IdExpr>(var), clone(c.pattern), suite));
    ctx->leaveConditionalBlock();
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
///   `case (x := pat)`    -> `(x = var; if match(var, pat))`
///   `case x`             -> `(x := var)`
///                           (only when `x` is not '_')
///   `case expr`          -> `if hasattr(typeof(var), "__match__"): if
///   var.__match__(foo())`
///                           (any expression that does not fit above patterns)
StmtPtr SimplifyVisitor::transformPattern(const ExprPtr &var, ExprPtr pattern,
                                          StmtPtr suite) {
  // Convenience function to generate `isinstance(e, typ)` calls
  auto isinstance = [&](const ExprPtr &e, const std::string &typ) -> ExprPtr {
    return N<CallExpr>(N<IdExpr>("isinstance"), e->clone(), N<IdExpr>(typ));
  };
  // Convenience function to find the index of an ellipsis within a list pattern
  auto findEllipsis = [&](const std::vector<ExprPtr> &items) {
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
                     N<IfStmt>(N<BinaryExpr>(var->clone(), "==", pattern), suite));
  } else if (auto er = CAST(pattern, RangeExpr)) {
    // Range pattern
    return N<IfStmt>(
        isinstance(var, "int"),
        N<IfStmt>(
            N<BinaryExpr>(var->clone(), ">=", clone(er->start)),
            N<IfStmt>(N<BinaryExpr>(var->clone(), "<=", clone(er->stop)), suite)));
  } else if (auto et = pattern->getTuple()) {
    // Tuple pattern
    for (auto it = et->items.size(); it-- > 0;) {
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
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
      suite = transformPattern(
          N<IndexExpr>(var->clone(), N<IntExpr>(it - el->items.size())),
          clone(el->items[it]), suite);
    }
    for (auto it = ellipsis; it-- > 0;) {
      suite = transformPattern(N<IndexExpr>(var->clone(), N<IntExpr>(it)),
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
  } else if (auto ea = CAST(pattern, AssignExpr)) {
    // Bound pattern
    seqassert(ea->var->getId(), "only simple assignment expressions are supported");
    return N<SuiteStmt>(N<AssignStmt>(clone(ea->var), clone(var)),
                        transformPattern(clone(var), clone(ea->expr), clone(suite)));
  } else if (auto ei = pattern->getId()) {
    // Wildcard pattern
    if (ei->value != "_") {
      return N<SuiteStmt>(N<AssignStmt>(clone(pattern), clone(var)), suite);
    } else {
      return suite;
    }
  }
  pattern = transform(pattern); // transform to check for pattern errors
  if (pattern->getEllipsis())
    pattern = N<CallExpr>(N<IdExpr>("ellipsis"));
  // Fallback (`__match__`) pattern
  return N<IfStmt>(
      N<CallExpr>(N<IdExpr>("hasattr"), var->clone(), N<StringExpr>("__match__"),
                  N<CallExpr>(N<IdExpr>("type"), pattern->clone())),
      N<IfStmt>(N<CallExpr>(N<DotExpr>(var->clone(), "__match__"), pattern), suite));
}

} // namespace codon::ast
