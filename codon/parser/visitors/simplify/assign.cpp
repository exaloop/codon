// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

/// Transform walrus (assignment) expression.
/// @example
///   `(expr := var)` -> `var = expr; var`
void SimplifyVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");
  StmtPtr s = N<AssignStmt>(clone(expr->var), expr->expr);
  auto avoidDomination = false; // walruses always leak
  std::swap(avoidDomination, ctx->avoidDomination);
  if (ctx->isConditionalExpr) {
    // Make sure to transform both suite _AND_ the expression in the same scope
    ctx->enterConditionalBlock();
    transform(s);
    transform(expr->var);
    SuiteStmt *suite = s->getSuite();
    if (!suite) {
      s = N<SuiteStmt>(s);
      suite = s->getSuite();
    }
    ctx->leaveConditionalBlock(&suite->stmts);
  } else {
    s = transform(s);
    transform(expr->var);
  }
  std::swap(avoidDomination, ctx->avoidDomination);
  resultExpr = N<StmtExpr>(std::vector<StmtPtr>{s}, expr->var);
}

/// Unpack assignments.
/// See @c transformAssignment and @c unpackAssignments for more details.
void SimplifyVisitor::visit(AssignStmt *stmt) {
  std::vector<StmtPtr> stmts;
  if (stmt->rhs && stmt->rhs->getBinary() && stmt->rhs->getBinary()->inPlace) {
    // Update case: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString());
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, nullptr, true));
  } else if (stmt->type) {
    // Type case: `a: T = b, c` (no unpacking)
    stmts.push_back(transformAssignment(stmt->lhs, stmt->rhs, stmt->type));
  } else {
    // Normal case
    unpackAssignments(stmt->lhs, stmt->rhs, stmts);
  }
  resultStmt = stmts.size() == 1 ? stmts[0] : N<SuiteStmt>(stmts);
}

/// Transform deletions.
/// @example
///   `del a`    -> `a = type(a)()` and remove `a` from the context
///   `del a[x]` -> `a.__delitem__(x)`
void SimplifyVisitor::visit(DelStmt *stmt) {
  if (auto idx = stmt->expr->getIndex()) {
    resultStmt = N<ExprStmt>(
        transform(N<CallExpr>(N<DotExpr>(idx->expr, "__delitem__"), idx->index)));
  } else if (auto ei = stmt->expr->getId()) {
    // Assign `a` to `type(a)()` to mark it for deletion
    resultStmt = N<AssignStmt>(
        transform(clone(stmt->expr)),
        transform(N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)))));
    resultStmt->getAssign()->setUpdate();

    // Allow deletion *only* if the binding is dominated
    auto val = ctx->find(ei->value);
    if (!val)
      E(Error::ID_NOT_FOUND, ei, ei->value);
    if (ctx->scope.blocks != val->scope)
      E(Error::DEL_NOT_ALLOWED, ei, ei->value);
    ctx->remove(ei->value);
  } else {
    E(Error::DEL_INVALID, stmt);
  }
}

/// Transform simple assignments.
/// @example
///   `a[x] = b`    -> `a.__setitem__(x, b)`
///   `a.x = b`     -> @c AssignMemberStmt
///   `a: type` = b -> @c AssignStmt
///   `a = b`       -> @c AssignStmt or @c UpdateStmt (see below)
StmtPtr SimplifyVisitor::transformAssignment(ExprPtr lhs, ExprPtr rhs, ExprPtr type,
                                             bool mustExist) {
  if (auto idx = lhs->getIndex()) {
    // Case: a[x] = b
    seqassert(!type, "unexpected type annotation");
    if (auto b = rhs->getBinary()) {
      if (mustExist && b->inPlace && !b->rexpr->getId()) {
        auto var = ctx->cache->getTemporaryVar("assign");
        seqassert(rhs->getBinary(), "not a bin");
        return transform(N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(var), idx->index),
            N<ExprStmt>(N<CallExpr>(
                N<DotExpr>(idx->expr, "__setitem__"), N<IdExpr>(var),
                N<BinaryExpr>(N<IndexExpr>(idx->expr->clone(), N<IdExpr>(var)), b->op,
                              b->rexpr, true)))));
      }
    }
    return transform(N<ExprStmt>(
        N<CallExpr>(N<DotExpr>(idx->expr, "__setitem__"), idx->index, rhs)));
  }

  if (auto dot = lhs->getDot()) {
    // Case: a.x = b
    seqassert(!type, "unexpected type annotation");
    transform(dot->expr, true);
    // If we are deducing class members, check if we can deduce a member from this
    // assignment
    auto deduced = ctx->getClassBase() ? ctx->getClassBase()->deducedMembers : nullptr;
    if (deduced && dot->expr->isId(ctx->getBase()->selfName) &&
        !in(*deduced, dot->member))
      deduced->push_back(dot->member);
    return N<AssignMemberStmt>(dot->expr, dot->member, transform(rhs));
  }

  // Case: a (: t) = b
  auto e = lhs->getId();
  if (!e)
    E(Error::ASSIGN_INVALID, lhs);

  // Disable creation of local variables that share the name with some global if such
  // global was already accessed within the current scope. Example:
  // x = 1
  // def foo():
  //   print(x)  # x is seen here
  //   x = 2     # this should error
  if (in(ctx->seenGlobalIdentifiers[ctx->getBaseName()], e->value))
    E(Error::ASSIGN_LOCAL_REFERENCE,
      ctx->seenGlobalIdentifiers[ctx->getBaseName()][e->value], e->value);

  auto val = ctx->find(e->value);
  // Make sure that existing values that cannot be shadowed (e.g. imported globals) are
  // only updated
  mustExist |= val && val->noShadow && !ctx->isOuter(val);
  if (mustExist) {
    val = ctx->findDominatingBinding(e->value);
    if (val && val->isVar() && !ctx->isOuter(val)) {
      auto s = N<AssignStmt>(transform(lhs, false), transform(rhs));
      if (ctx->getBase()->attributes && ctx->getBase()->attributes->has(Attr::Atomic))
        s->setAtomicUpdate();
      else
        s->setUpdate();
      return s;
    } else {
      E(Error::ASSIGN_LOCAL_REFERENCE, e, e->value);
    }
  }

  transform(rhs, true);
  transformType(type, false);

  // Generate new canonical variable name for this assignment and add it to the context
  auto canonical = ctx->generateCanonicalName(e->value);
  auto assign = N<AssignStmt>(N<IdExpr>(canonical), rhs, type);
  val = nullptr;
  if (rhs && rhs->isType()) {
    val = ctx->addType(e->value, canonical, lhs->getSrcInfo());
  } else {
    val = ctx->addVar(e->value, canonical, lhs->getSrcInfo());
    if (auto st = getStaticGeneric(type.get()))
      val->staticType = st;
    if (ctx->avoidDomination)
      val->avoidDomination = true;
  }
  // Clean up seen tags if shadowing a name
  ctx->seenGlobalIdentifiers[ctx->getBaseName()].erase(e->value);

  // Register all toplevel variables as global in JIT mode
  bool isGlobal = (ctx->cache->isJit && val->isGlobal() && !val->isGeneric()) ||
                  (canonical == VAR_ARGV);
  if (isGlobal && !val->isGeneric())
    ctx->cache->addGlobal(canonical);

  return assign;
}

/// Unpack an assignment expression `lhs = rhs` into a list of simple assignment
/// expressions (e.g., `a = b`, `a.x = b`, or `a[x] = b`).
/// Handle Python unpacking rules.
/// @example
///   `(a, b) = c`     -> `a = c[0]; b = c[1]`
///   `a, b = c`       -> `a = c[0]; b = c[1]`
///   `[a, *x, b] = c` -> `a = c[0]; x = c[1:-1]; b = c[-1]`.
/// Non-trivial right-hand expressions are first stored in a temporary variable.
/// @example
///   `a, b = c, d + foo()` -> `assign = (c, d + foo); a = assign[0]; b = assign[1]`.
/// Each assignment is unpacked recursively to allow cases like `a, (b, c) = d`.
void SimplifyVisitor::unpackAssignments(const ExprPtr &lhs, ExprPtr rhs,
                                        std::vector<StmtPtr> &stmts) {
  std::vector<ExprPtr> leftSide;
  if (auto et = lhs->getTuple()) {
    // Case: (a, b) = ...
    for (auto &i : et->items)
      leftSide.push_back(i);
  } else if (auto el = lhs->getList()) {
    // Case: [a, b] = ...
    for (auto &i : el->items)
      leftSide.push_back(i);
  } else {
    // Case: simple assignment (a = b, a.x = b, or a[x] = b)
    stmts.push_back(transformAssignment(clone(lhs), clone(rhs)));
    return;
  }

  // Prepare the right-side expression
  auto srcPos = rhs->getSrcInfo();
  if (!rhs->getId()) {
    // Store any non-trivial right-side expression into a variable
    auto var = ctx->cache->getTemporaryVar("assign");
    ExprPtr newRhs = N<IdExpr>(srcPos, var);
    stmts.push_back(transformAssignment(newRhs, clone(rhs)));
    rhs = newRhs;
  }

  // Process assignments until the fist StarExpr (if any)
  size_t st = 0;
  for (; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: `leftSide_st = rhs[st]` where `st` is static integer
    auto rightSide = N<IndexExpr>(srcPos, clone(rhs), N<IntExpr>(srcPos, st));
    // Recursively process the assignment because of cases like `(a, (b, c)) = d)`
    unpackAssignments(leftSide[st], rightSide, stmts);
  }
  // Process StarExpr (if any) and the assignments that follow it
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes `d[1:-2]`)
    auto rightSide = N<IndexExpr>(
        srcPos, clone(rhs),
        N<SliceExpr>(srcPos, N<IntExpr>(srcPos, st),
                     // this slice is either [st:] or [st:-lhs_len + st + 1]
                     leftSide.size() == st + 1
                         ? nullptr
                         : N<IntExpr>(srcPos, -leftSide.size() + st + 1),
                     nullptr));
    unpackAssignments(leftSide[st]->getStar()->what, rightSide, stmts);
    st += 1;
    // Process remaining assignments. They will use negative indices (-1, -2 etc.)
    // because we do not know how big is StarExpr
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        E(Error::ASSIGN_MULTI_STAR, leftSide[st]);
      rightSide = N<IndexExpr>(srcPos, clone(rhs),
                               N<IntExpr>(srcPos, -int(leftSide.size() - st)));
      unpackAssignments(leftSide[st], rightSide, stmts);
    }
  }
}

} // namespace codon::ast
