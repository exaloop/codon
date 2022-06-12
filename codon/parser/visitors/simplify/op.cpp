#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(UnaryExpr *expr) {
  resultExpr = N<UnaryExpr>(expr->op, transform(expr->expr));
}

/// Transform binary expressions with a few special considerations.
/// The real stuff happens during the type checking.
void SimplifyVisitor::visit(BinaryExpr *expr) {
  // Keep None in `is None` for typechecker.
  // Special case: `is` can take type as well
  auto lhs = (startswith(expr->op, "is") && expr->lexpr->getNone())
                 ? clone(expr->lexpr)
                 : transform(expr->lexpr, startswith(expr->op, "is"));

  auto tmp = ctx->isConditionalExpr;
  // The second operand of the and/or expression is conditional
  ctx->isConditionalExpr = expr->op == "&&" || expr->op == "||";
  auto rhs = (startswith(expr->op, "is") && expr->rexpr->getNone())
                 ? clone(expr->rexpr)
                 : transform(expr->rexpr, startswith(expr->op, "is"));
  ctx->isConditionalExpr = tmp;
  resultExpr = N<BinaryExpr>(lhs, expr->op, rhs, expr->inPlace);
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
  for (auto i = items.size() - 1; i-- > 0; )
    final = N<BinaryExpr>(items[i], "&&", final);
  resultExpr = transform(final);
}

/// Check for ellipses within pipes and mark them with `isPipeArg`.
/// The rest is handled during the type checking.
void SimplifyVisitor::visit(PipeExpr *expr) {
  std::vector<PipeExpr::Pipe> p;
  for (auto &i : expr->items) {
    auto e = clone(i.expr);
    if (auto ec = e->getCall()) {
      for (auto &a : ec->args)
        if (auto ee = a.value->getEllipsis())
          ee->isPipeArg = true;
    }
    p.push_back({i.op, transform(e)});
  }
  resultExpr = N<PipeExpr>(p);
}

/// Transform index into an instantiation @c InstantiateExpr if possible.
/// Generate tuple class `Tuple.N` for `Tuple[T1, ... TN]` (and `tuple[...]`).
/// The rest is handled during the type checking.
void SimplifyVisitor::visit(IndexExpr *expr) {
  ExprPtr ex = nullptr;
  if (expr->expr->isId("tuple") || expr->expr->isId("Tuple")) {
    // Special case: tuples. Change to Tuple.N and generate tuple stub
    auto t = expr->index->getTuple();
    ex = N<IdExpr>(format(TYPE_TUPLE "{}", t ? t->items.size() : 1));
    ex->markType();
  } else if (expr->expr->isId("Static")) {
    // Special case: static types. Ensure that static is supported
    if (!expr->index->isId("int") && !expr->index->isId("str"))
      error("only static integers and strings are supported");
    resultExpr = expr->clone();
    resultExpr->markType();
    return;
  } else {
    ex = transform(expr->expr, true);
  }

  // IndexExpr[i1, ..., iN] is internally represented as
  // IndexExpr[TupleExpr[i1, ..., iN]] for N > 1
  std::vector<ExprPtr> items;
  if (auto t = expr->index->getTuple()) {
    for (auto &i : t->items)
      items.push_back(i);
  } else {
    items.push_back(expr->index);
  }
  for (auto &i : items) {
    if (i->getList() && ex->isType()) {
      // Special case: `A[[A, B], C]` -> `A[Tuple[A, B], C]` (e.g., in `Function[...]`)
      i = N<IndexExpr>(N<IdExpr>("Tuple"), N<TupleExpr>(clone(i)->getList()->items));
    }
    i = transform(clone(i), true);
  }
  if (ex->isType()) {
    resultExpr = N<InstantiateExpr>(ex, items);
    resultExpr->markType();
  } else {
    resultExpr = N<IndexExpr>(ex, items.size() == 1 ? items[0] : N<TupleExpr>(items));
  }
}

} // namespace codon::ast