#include <memory>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(AssignExpr *expr) {
  seqassert(expr->var->getId(), "only simple assignment expression are supported");
  StmtPtr s = N<AssignStmt>(clone(expr->var), clone(expr->expr));
  if (ctx->isConditionalExpr)
    s = transformInScope(s);
  else
    s = transform(s);
  resultExpr = N<StmtExpr>(std::vector<StmtPtr>{s}, transform(expr->var));
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
  if (auto eix = stmt->expr->getIndex()) {
    resultStmt = N<ExprStmt>(transform(
        N<CallExpr>(N<DotExpr>(clone(eix->expr), "__delitem__"), clone(eix->index))));
  } else if (auto ei = stmt->expr->getId()) {
    // Assign `a` to `type(a)()` to mark it for deletion
    resultStmt = N<AssignStmt>(
        transform(stmt->expr),
        transform(N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)))));
    // Allow deletion *only* if the binding is dominated
    auto val = ctx->find(ei->value);
    if (!val || ctx->scope != val->scope)
      error("cannot delete '{}'", ei->value);
    ctx->remove(ei->value);
  } else {
    error("invalid del statement");
  }
}

/// Transform simple assignments.
/// @example
///   `a[x] = b`    -> `a.__setitem__(x, b)`
///   `a.x = b`     -> @c AssignMemberStmt
///   `a: type` = b -> @c AssignStmt
///   `a = b`       -> @c AssignStmt or @c UpdateStmt (see below)
StmtPtr SimplifyVisitor::transformAssignment(const ExprPtr &lhs, const ExprPtr &rhs,
                                             const ExprPtr &type, bool mustExist) {
  if (auto idx = lhs->getIndex()) {
    // Case: a[x] = b
    seqassert(!type, "unexpected type annotation");
    return transform(N<ExprStmt>(N<CallExpr>(
        N<DotExpr>(clone(idx->expr), "__setitem__"), clone(idx->index), rhs->clone())));
  }

  if (auto dot = lhs->getDot()) {
    // Case: a.x = b
    seqassert(!type, "unexpected type annotation");
    auto l = transform(dot->expr);
    // If we are deducing class members, check if we can deduce a member from this
    // assignment
    auto deduced = ctx->getClassBase() ? ctx->getClassBase()->deducedMembers : nullptr;
    if (deduced && l->isId(ctx->getBase()->selfName) && !in(*deduced, dot->member))
      deduced->push_back(dot->member);
    return N<AssignMemberStmt>(l, dot->member, transform(rhs, false));
  }

  // Case: a (: t) = b
  auto e = lhs->getId();
  if (!e) {
    error("invalid assignment");
  }

  // Disable creation of local variables that share the name with some global if such
  // global was already accessed within the current scope. Example:
  // x = 1
  // def foo():
  //   print(x)  # x is seen here
  //   x = 2     # this should error
  if (in(ctx->seenGlobalIdentifiers[ctx->getBaseName()], e->value))
    error(ctx->seenGlobalIdentifiers[ctx->getBaseName()][e->value],
          "local variable '{}' referenced before assignment", e->value);

  ExprPtr t = transformType(type, false);
  auto r = transform(rhs, true);

  auto val = ctx->find(e->value);
  // Make sure that existing values that cannot be shadowed (e.g. imported globals) are
  // only updated
  mustExist |= val && val->noShadow && !ctx->isOuter(val);
  if (mustExist) {
    val = ctx->findDominatingBinding(e->value);
    if (val && val->isVar() && !ctx->isOuter(val)) {
      return N<UpdateStmt>(transform(lhs, false), transform(rhs, true),
                           ctx->getBase()->attributes &&
                               ctx->getBase()->attributes->has(Attr::Atomic));
    } else {
      error("variable '{}' cannot be updated", e->value);
    }
  }

  // Generate new canonical variable name for this assignment and add it to the context
  auto canonical = ctx->generateCanonicalName(e->value);
  if (r && r->isType()) {
    ctx->addType(e->value, canonical, lhs->getSrcInfo());
  } else {
    ctx->addVar(e->value, canonical, lhs->getSrcInfo());
  }
  return N<AssignStmt>(N<IdExpr>(canonical), r, t);
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
void SimplifyVisitor::unpackAssignments(ExprPtr lhs, ExprPtr rhs,
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
    stmts.push_back(transformAssignment(lhs, rhs));
    return;
  }

  // Prepare the right-side expression
  auto srcPos = rhs.get();
  if (!rhs->getId()) {
    // Store any non-trivial right-side expression into a variable
    auto var = ctx->cache->getTemporaryVar("assign");
    ExprPtr newRhs = Nx<IdExpr>(srcPos, var);
    stmts.push_back(transformAssignment(newRhs, rhs));
    rhs = newRhs;
  }

  // Process assignments until the fist StarExpr (if any)
  size_t st;
  for (st = 0; st < leftSide.size(); st++) {
    if (leftSide[st]->getStar())
      break;
    // Transformation: `leftSide_st = rhs[st]` where `st` is static integer
    auto rightSide = Nx<IndexExpr>(srcPos, rhs->clone(), Nx<IntExpr>(srcPos, st));
    // Recursively process the assignment because of cases like `(a, (b, c)) = d)`
    unpackAssignments(leftSide[st], rightSide, stmts);
  }
  // Process StarExpr (if any) and the assignments that follow it
  if (st < leftSide.size() && leftSide[st]->getStar()) {
    // StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes `d[1:-2]`)
    auto rightSide = Nx<IndexExpr>(
        srcPos, rhs->clone(),
        Nx<SliceExpr>(srcPos, Nx<IntExpr>(srcPos, st),
                      // this slice is either [st:] or [st:-lhs_len + st + 1]
                      leftSide.size() == st + 1
                          ? nullptr
                          : Nx<IntExpr>(srcPos, -leftSide.size() + st + 1),
                      nullptr));
    unpackAssignments(leftSide[st]->getStar()->what, rightSide, stmts);
    st += 1;
    // Process remaining assignments. They will use negative indices (-1, -2 etc.)
    // because we do not know how big is StarExpr
    for (; st < leftSide.size(); st++) {
      if (leftSide[st]->getStar())
        error(leftSide[st], "multiple unpack expressions");
      rightSide = Nx<IndexExpr>(srcPos, rhs->clone(),
                                Nx<IntExpr>(srcPos, -int(leftSide.size() - st)));
      unpackAssignments(leftSide[st], rightSide, stmts);
    }
  }
}

} // namespace codon::ast