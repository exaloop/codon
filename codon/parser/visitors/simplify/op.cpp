// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

void SimplifyVisitor::visit(UnaryExpr *expr) { transform(expr->expr); }

/// Transform binary expressions with a few special considerations.
/// The real stuff happens during the type checking.
void SimplifyVisitor::visit(BinaryExpr *expr) {
  // Special case: `is` can take type as well
  transform(expr->lexpr, startswith(expr->op, "is"));
  auto tmp = ctx->isConditionalExpr;
  // The second operand of the and/or expression is conditional
  ctx->isConditionalExpr = expr->op == "&&" || expr->op == "||";
  transform(expr->rexpr, startswith(expr->op, "is"));
  ctx->isConditionalExpr = tmp;
}

/// Transform chain binary expression.
/// @example
///   `a <= b <= c` -> `(a <= (chain := b)) and (chain <= c)`
/// The assignment above ensures that all expressions are executed only once.
void SimplifyVisitor::visit(ChainBinaryExpr *expr) {
  seqassert(expr->exprs.size() >= 2, "not enough expressions in ChainBinaryExpr");
  std::vector<ExprPtr> items;
  std::string prev;
  for (int i = 1; i < expr->exprs.size(); i++) {
    auto l = prev.empty() ? clone(expr->exprs[i - 1].second) : N<IdExpr>(prev);
    prev = ctx->generateCanonicalName("chain");
    auto r =
        (i + 1 == expr->exprs.size())
            ? clone(expr->exprs[i].second)
            : N<StmtExpr>(N<AssignStmt>(N<IdExpr>(prev), clone(expr->exprs[i].second)),
                          N<IdExpr>(prev));
    items.emplace_back(N<BinaryExpr>(l, expr->exprs[i].first, r));
  }

  ExprPtr final = items.back();
  for (auto i = items.size() - 1; i-- > 0;)
    final = N<BinaryExpr>(items[i], "&&", final);
  resultExpr = transform(final);
}

/// Transform index into an instantiation @c InstantiateExpr if possible.
/// Generate tuple class `Tuple` for `Tuple[T1, ... TN]` (and `tuple[...]`).
/// The rest is handled during the type checking.
void SimplifyVisitor::visit(IndexExpr *expr) {
  if (expr->expr->isId("tuple") || expr->expr->isId(TYPE_TUPLE)) {
    auto t = expr->index->getTuple();
    expr->expr = NT<IdExpr>(TYPE_TUPLE);
  } else if (expr->expr->isId("Static")) {
    // Special case: static types. Ensure that static is supported
    if (!expr->index->isId("int") && !expr->index->isId("str"))
      E(Error::BAD_STATIC_TYPE, expr->index);
    expr->markType();
    return;
  } else {
    transform(expr->expr, true);
  }

  // IndexExpr[i1, ..., iN] is internally represented as
  // IndexExpr[TupleExpr[i1, ..., iN]] for N > 1
  std::vector<ExprPtr> items;
  bool isTuple = expr->index->getTuple();
  if (auto t = expr->index->getTuple()) {
    items = t->items;
  } else {
    items.push_back(expr->index);
  }
  for (auto &i : items) {
    if (i->getList() && expr->expr->isType()) {
      // Special case: `A[[A, B], C]` -> `A[Tuple[A, B], C]` (e.g., in
      // `Function[...]`)
      i = N<IndexExpr>(N<IdExpr>(TYPE_TUPLE), N<TupleExpr>(i->getList()->items));
    }
    transform(i, true);
  }
  if (expr->expr->isType()) {
    resultExpr = N<InstantiateExpr>(expr->expr, items);
    resultExpr->markType();
  } else {
    expr->index = (!isTuple && items.size() == 1) ? items[0] : N<TupleExpr>(items);
  }
}

/// Already transformed. Sometimes needed again
/// for identifier analysis.
void SimplifyVisitor::visit(InstantiateExpr *expr) {
  transformType(expr->typeExpr);
  for (auto &tp : expr->typeParams)
    transform(tp, true);
}

} // namespace codon::ast
