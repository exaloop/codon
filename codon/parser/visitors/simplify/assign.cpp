#include <memory>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(AssignStmt *stmt) {
  std::vector<StmtPtr> stmts;
  if (stmt->rhs && stmt->rhs->getBinary() && stmt->rhs->getBinary()->inPlace) {
    /// Case 1: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString());
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, nullptr, false, true));
  } else if (stmt->type) {
    /// Case 2:
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, stmt->type, true, false));
  } else {
    unpackAssignments(stmt->lhs, stmt->rhs, stmts, stmt->shadow, false);
  }
  resultStmt = stmts.size() == 1 ? stmts[0] : N<SuiteStmt>(stmts);
}

void SimplifyVisitor::visit(DelStmt *stmt) {
  if (auto eix = stmt->expr->getIndex()) {
    resultStmt = N<ExprStmt>(transform(
        N<CallExpr>(N<DotExpr>(clone(eix->expr), "__delitem__"), clone(eix->index))));
  } else if (auto ei = stmt->expr->getId()) {
    resultStmt = N<AssignStmt>(
        transform(stmt->expr),
        transform(N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)))));
    // Allow deletion _only_ if the variable is dominated!
    auto val = ctx->find(ei->value);
    if (!val || ctx->scope != val->scope)
      error("cannot delete '{}'", ei->value);
    ctx->remove(ei->value);
  } else {
    error("invalid del statement");
  }
}

StmtPtr SimplifyVisitor::transformAssignment(const ExprPtr &lhs, const ExprPtr &rhs,
                                             const ExprPtr &type, bool shadow,
                                             bool mustExist) {
  if (auto ei = lhs->getIndex()) {
    seqassert(!type, "unexpected type annotation");
    return transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(ei->expr), "__setitem__"),
                                             clone(ei->index), rhs->clone())));
  } else if (auto ed = lhs->getDot()) {
    seqassert(!type, "unexpected type annotation");
    auto l = transform(ed->expr);
    if (!ctx->bases.empty() && ctx->bases.back().deducedMembers &&
        l->isId(ctx->bases.back().selfName)) {
      if (std::find(ctx->bases.back().deducedMembers->begin(),
                    ctx->bases.back().deducedMembers->end(),
                    ed->member) == ctx->bases.back().deducedMembers->end())
        ctx->bases.back().deducedMembers->push_back(ed->member);
    }
    return N<AssignMemberStmt>(l, ed->member, transform(rhs, false));
  } else if (auto e = lhs->getId()) {
    if (in(ctx->seenGlobalIdentifiers[ctx->getBase()], e->value))
      error(ctx->seenGlobalIdentifiers[ctx->getBase()][e->value],
            "local variable '{}' referenced before assignment", e->value);

    ExprPtr t = transformType(type, false);
    auto r = transform(rhs, true);

    auto val = ctx->find(e->value);
    mustExist |= val && val->noShadow;
    if (mustExist) {
      val = ctx->findDominatingBinding(e->value);
      if (val && val->isVar() && val->getBase() == ctx->getBase())
        return N<UpdateStmt>(transform(lhs, false), transform(rhs, true),
                             !ctx->bases.empty() &&
                                 ctx->bases.back().attributes & FLAG_ATOMIC);
      else
        error("variable '{}' cannot be updated", e->value);
    }

    // Generate new canonical variable name for this assignment and use it afterwards.
    auto canonical = ctx->generateCanonicalName(e->value);
    if (r && r->isType()) {
      ctx->addType(e->value, canonical, lhs->getSrcInfo());
    } else {
      // bool isStatic = t && t->getIndex() && t->getIndex()->expr->isId("Static");
      // if (ctx->scope.size() == 1 && !in(ctx->cache->globals, canonical) && !isStatic)
      // {
      //   ctx->cache->globals[canonical] = nullptr;
      //   preamble->globals.push_back(N<AssignStmt>(N<IdExpr>(canonical), nullptr, t));
      //   return N<UpdateStmt>(N<IdExpr>(canonical), r);
      // } else {
      // }
      ctx->addVar(e->value, canonical, lhs->getSrcInfo());
    }
    return N<AssignStmt>(N<IdExpr>(canonical), r, t);
  } else {
    error("invalid assignment");
    return nullptr;
  }
}

void SimplifyVisitor::unpackAssignments(ExprPtr lhs, ExprPtr rhs,
                                        std::vector<StmtPtr> &stmts, bool shadow,
                                        bool mustExist) {
  std::vector<ExprPtr> leftSide;
  if (auto et = lhs->getTuple()) { // (a, b) = ...
    for (auto &i : et->items)
      leftSide.push_back(i);
  } else if (auto el = lhs->getList()) { // [a, b] = ...
    for (auto &i : el->items)
      leftSide.push_back(i);
  } else { // A simple assignment.
    stmts.push_back(transformAssignment(lhs, rhs, nullptr, shadow, mustExist));
    return;
  }

  // Prepare the right-side expression
  auto srcPos = rhs.get();
  if (!rhs->getId()) { // Store any non-trivial right-side expression (assign = rhs).
    auto var = ctx->cache->getTemporaryVar("assign");
    ExprPtr newRhs = Nx<IdExpr>(srcPos, var);
    stmts.push_back(transformAssignment(newRhs, rhs, nullptr, shadow, mustExist));
    rhs = newRhs;
  }

  // Process each assignment until the fist StarExpr (if any).
  int st;
  for (st = 0; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: leftSide_st = rhs[st]
    auto rightSide = Nx<IndexExpr>(srcPos, rhs->clone(), Nx<IntExpr>(srcPos, st));
    // Recursively process the assignment (as we can have cases like (a, (b, c)) = d).
    unpackAssignments(leftSide[st], rightSide, stmts, shadow, mustExist);
  }
  // If there is a StarExpr, process it and the remaining assignments after it (if
  // any).
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr: in (a, *b, c) = d, b is d[1:-2]
    auto rightSide = Nx<IndexExpr>(
        srcPos, rhs->clone(),
        Nx<SliceExpr>(srcPos, Nx<IntExpr>(srcPos, st),
                      // This slice is either [st:] or [st:-lhs_len + st + 1]
                      leftSide.size() == st + 1
                          ? nullptr
                          : Nx<IntExpr>(srcPos, -leftSide.size() + st + 1),
                      nullptr));
    unpackAssignments(leftSide[st]->getStar()->what, rightSide, stmts, shadow,
                      mustExist);
    st += 1;
    // Keep going till the very end. Remaining assignments use negative indices (-1,
    // -2 etc) as we are not sure how big is StarExpr.
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        error(leftSide[st], "multiple unpack expressions");
      rightSide = Nx<IndexExpr>(srcPos, rhs->clone(),
                                Nx<IntExpr>(srcPos, -leftSide.size() + st));
      unpackAssignments(leftSide[st], rightSide, stmts, shadow, mustExist);
    }
  }
}

} // namespace codon::ast