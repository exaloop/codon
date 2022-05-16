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

void SimplifyVisitor::visit(BinaryExpr *expr) {
  auto lhs = (startswith(expr->op, "is") && expr->lexpr->getNone())
                 ? clone(expr->lexpr)
                 : transform(expr->lexpr, startswith(expr->op, "is"));
  auto rhs = (startswith(expr->op, "is") && expr->rexpr->getNone())
                 ? clone(expr->rexpr)
                 : transform(expr->rexpr, startswith(expr->op, "is"),
                             /*allowAssign*/ expr->op != "&&" && expr->op != "||");
  resultExpr = N<BinaryExpr>(lhs, expr->op, rhs, expr->inPlace);
}

void SimplifyVisitor::visit(ChainBinaryExpr *expr) {
  seqassert(expr->exprs.size() >= 2, "not enough expressions in ChainBinaryExpr");
  std::vector<ExprPtr> e;
  std::string prev;
  for (int i = 1; i < expr->exprs.size(); i++) {
    auto l = prev.empty() ? clone(expr->exprs[i - 1].second) : N<IdExpr>(prev);
    prev = ctx->generateCanonicalName("chain");
    auto r =
        (i + 1 == expr->exprs.size())
            ? clone(expr->exprs[i].second)
            : N<StmtExpr>(N<AssignStmt>(N<IdExpr>(prev), clone(expr->exprs[i].second)),
                          N<IdExpr>(prev));
    e.emplace_back(N<BinaryExpr>(l, expr->exprs[i].first, r));
  }

  int i = int(e.size()) - 1;
  ExprPtr b = e[i];
  for (i -= 1; i >= 0; i--)
    b = N<BinaryExpr>(e[i], "&&", b);
  resultExpr = transform(b);
}

void SimplifyVisitor::visit(PipeExpr *expr) {
  std::vector<PipeExpr::Pipe> p;
  for (auto &i : expr->items) {
    auto e = clone(i.expr);
    if (auto ec = const_cast<CallExpr *>(e->getCall())) {
      for (auto &a : ec->args)
        if (auto ee = const_cast<EllipsisExpr *>(a.value->getEllipsis()))
          ee->isPipeArg = true;
    }
    p.push_back({i.op, transform(e)});
  }
  resultExpr = N<PipeExpr>(p);
}

void SimplifyVisitor::visit(IndexExpr *expr) {
  ExprPtr e = nullptr;
  auto index = expr->index->clone();
  // First handle the tuple[] and function[] cases.
  if (expr->expr->isId("tuple") || expr->expr->isId("Tuple")) {
    auto t = index->getTuple();
    e = N<IdExpr>(format(TYPE_TUPLE "{}", t ? t->items.size() : 1));
    e->markType();
  } else if (expr->expr->isId("function") || expr->expr->isId("Function") ||
             expr->expr->isId("Callable")) {
    auto t = const_cast<TupleExpr *>(index->getTuple());
    if (!t || t->items.size() != 2 || !t->items[0]->getList())
      error("invalid {} type declaration", expr->expr->getId()->value);
    for (auto &i : const_cast<ListExpr *>(t->items[0]->getList())->items)
      t->items.emplace_back(i);
    t->items.erase(t->items.begin());
    e = N<IdExpr>(
        format(expr->expr->isId("Callable") ? TYPE_CALLABLE "{}" : TYPE_FUNCTION "{}",
               int(t->items.size()) - 1));
    e->markType();
  } else if (expr->expr->isId("Static")) {
    if (!expr->index->isId("int") && !expr->index->isId("str"))
      error("only static integers and strings are supported");
    resultExpr = expr->clone();
    resultExpr->markType();
    return;
  } else {
    e = transform(expr->expr, true);
  }
  // IndexExpr[i1, ..., iN] is internally stored as IndexExpr[TupleExpr[i1, ..., iN]]
  // for N > 1, so make sure to check that case.

  std::vector<ExprPtr> it;
  if (auto t = index->getTuple())
    for (auto &i : t->items)
      it.push_back(i);
  else
    it.push_back(index);
  for (auto &i : it) {
    if (auto es = i->getStar())
      i = N<StarExpr>(transform(es->what));
    else if (auto ek = CAST(i, KeywordStarExpr))
      i = N<KeywordStarExpr>(transform(ek->what));
    else
      i = transform(i, true);
  }
  if (e->isType()) {
    resultExpr = N<InstantiateExpr>(e, it);
    resultExpr->markType();
  } else {
    resultExpr = N<IndexExpr>(e, it.size() == 1 ? it[0] : N<TupleExpr>(it));
  }
}

} // namespace codon::ast