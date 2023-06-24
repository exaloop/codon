// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Transform walrus (assignment) expression.
/// @example
///   `(expr := var)` -> `var = expr; var`
void TypecheckVisitor::visit(AssignExpr *expr) {
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
  resultExpr = transform(N<StmtExpr>(std::vector<StmtPtr>{s}, expr->var));
}

/// Transform assignments. Handle dominated assignments, forward declarations, static
/// assignments and type/function aliases.
/// See @c transformAssignment and @c unpackAssignments for more details.
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignStmt *stmt) {
  if (stmt->rhs && stmt->rhs->getBinary() && stmt->rhs->getBinary()->inPlace) {
    // Update case: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString());
    resultStmt = transformAssignment(stmt->lhs, stmt->rhs, nullptr, true);
  } else if (!stmt->type && !stmt->lhs->getId()) {
    // Normal case
    std::vector<StmtPtr> stmts;
    unpackAssignments(stmt->lhs, stmt->rhs, stmts);
    resultStmt = transform(N<SuiteStmt>(stmts));
  } else {
    // Type case: `a: T = b, c` (no unpacking); all other (invalid) cases
    resultStmt = transformAssignment(stmt->lhs, stmt->rhs, stmt->type);
  }
}

/// Transform deletions.
/// @example
///   `del a`    -> `a = type(a)()` and remove `a` from the context
///   `del a[x]` -> `a.__delitem__(x)`
void TypecheckVisitor::visit(DelStmt *stmt) {
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
StmtPtr TypecheckVisitor::transformAssignment(ExprPtr lhs, ExprPtr rhs, ExprPtr type,
                                              bool mustExist) {
  if (auto idx = lhs->getIndex()) {
    // Case: a[x] = b
    seqassert(!type, "unexpected type annotation");
    return transform(N<ExprStmt>(
        N<CallExpr>(N<DotExpr>(idx->expr, "__setitem__"), idx->index, rhs)));
  }

  if (auto dot = lhs->getDot()) {
    // Case: a.x = b
    seqassert(!type, "unexpected type annotation");
    transform(dot->expr, true);
    // If we are deducing class members, check if we can deduce a member from this
    // assignment
    // todo)) deduction!
    // auto deduced = ctx->getClassBase() ? ctx->getClassBase()->deducedMembers :
    // nullptr; if (deduced && dot->expr->isId(ctx->getBase()->selfName) &&
    //     !in(*deduced, dot->member))
    //   deduced->push_back(dot->member);
    return transform(N<AssignMemberStmt>(dot->expr, dot->member, transform(rhs)));
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
  if (in(ctx->getBase()->seenGlobalIdentifiers, e->value))
    E(Error::ASSIGN_LOCAL_REFERENCE, ctx->getBase()->seenGlobalIdentifiers[e->value],
      e->value);

  auto val = ctx->find(e->value);
  // Make sure that existing values that cannot be shadowed (e.g. imported globals) are
  // only updated
  mustExist |= val && !val->canShadow && !ctx->isOuter(val);
  if (mustExist) {
    val = findDominatingBinding(e->value, ctx.get());
    if (val && val->isVar() && !ctx->isOuter(val)) {
      auto s = N<AssignStmt>(lhs, rhs);
      if (ctx->getBase()->attributes && ctx->getBase()->attributes->has(Attr::Atomic))
        s->setAtomicUpdate();
      else
        s->setUpdate();
      transformUpdate(s.get());
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
  unify(assign->lhs->type, ctx->getUnbound(assign->lhs->getSrcInfo()));
  if (assign->type) {
    unify(assign->lhs->type,
          ctx->instantiate(assign->type->getSrcInfo(), assign->type->getType()));
  }
  val = std::make_shared<TypecheckItem>(canonical, ctx->getBaseName(), ctx->getModule(),
                                        assign->lhs->type, ctx->scope.blocks);
  val->setSrcInfo(getSrcInfo());
  if (auto st = getStaticGeneric(assign->type.get()))
    val->staticType = st;
  if (ctx->avoidDomination)
    val->avoidDomination = true;
  ctx->Context<TypecheckItem>::add(e->value, val);
  ctx->addAlwaysVisible(val);
  LOG("added ass/{}: {}", val->isVar() ? "v" : (val->isFunc() ? "f" : "t"),
      val->canonicalName);

  if (assign->rhs && assign->type && assign->type->getType()->isStaticType()) {
    // Static assignments (e.g., `x: Static[int] = 5`)
    if (!assign->rhs->isStatic())
      E(Error::EXPECTED_STATIC, assign->rhs);
    seqassert(assign->rhs->staticValue.evaluated, "static not evaluated");
    unify(assign->lhs->type,
          unify(assign->type->type, Type::makeStatic(ctx->cache, assign->rhs)));
  } else if (assign->rhs) {
    // Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
    if (wrapExpr(assign->rhs, assign->lhs->getType()))
      unify(assign->lhs->type, assign->rhs->type);
    if (rhs->isType())
      val->type = val->type->getClass();
    auto type = assign->lhs->getType();
    // Generalize non-variable types. That way we can support cases like:
    // `a = foo(x, ...); a(1); a('s')`
    if (!val->isVar())
      val->type = val->type->generalize(ctx->typecheckLevel - 1);

    // todo)) if (in(ctx->cache->globals, lhs)) {
  }

  if ((!assign->rhs || assign->rhs->isDone()) && realize(assign->lhs->type)) {
    assign->setDone();
  }

  // Clean up seen tags if shadowing a name
  ctx->getBase()->seenGlobalIdentifiers.erase(e->value);
  // Register all toplevel variables as global in JIT mode
  bool isGlobal = (ctx->cache->isJit && val->isGlobal() && !val->isGeneric()) ||
                  (canonical == VAR_ARGV);
  if (isGlobal && val->isVar())
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
void TypecheckVisitor::unpackAssignments(const ExprPtr &lhs, ExprPtr rhs,
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

/// Transform binding updates. Special handling is done for atomic or in-place
/// statements (e.g., `a += b`).
/// See @c transformInplaceUpdate and @c wrapExpr for details.
void TypecheckVisitor::transformUpdate(AssignStmt *stmt) {
  transform(stmt->lhs);
  if (stmt->lhs->isStatic())
    E(Error::ASSIGN_UNEXPECTED_STATIC, stmt->lhs);

  // Check inplace updates
  auto [inPlace, inPlaceExpr] = transformInplaceUpdate(stmt);
  if (inPlace) {
    if (inPlaceExpr) {
      resultStmt = N<ExprStmt>(inPlaceExpr);
      if (inPlaceExpr->isDone())
        resultStmt->setDone();
    }
    return;
  }

  transform(stmt->rhs);
  // Case: wrap expressions if needed (e.g. floats or optionals)
  if (wrapExpr(stmt->rhs, stmt->lhs->getType()))
    unify(stmt->rhs->type, stmt->lhs->type);
  if (stmt->rhs->done && realize(stmt->lhs->type))
    stmt->setDone();
}

/// Typecheck instance member assignments (e.g., `a.b = c`) and handle optional
/// instances. Disallow tuple updates.
/// @example
///   `opt.foo = bar` -> `unwrap(opt).foo = wrap(bar)`
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignMemberStmt *stmt) {
  transform(stmt->lhs);

  if (auto lhsClass = stmt->lhs->getType()->getClass()) {
    auto member = ctx->findMember(lhsClass->name, stmt->member);

    if (!member && stmt->lhs->isType()) {
      // Case: class variables
      if (auto cls = in(ctx->cache->classes, lhsClass->name))
        if (auto var = in(cls->classVars, stmt->member)) {
          auto a = N<AssignStmt>(N<IdExpr>(*var), transform(stmt->rhs));
          a->setUpdate();
          resultStmt = transform(a);
          return;
        }
    }
    if (!member && lhsClass->is(TYPE_OPTIONAL)) {
      // Unwrap optional and look up there
      resultStmt = transform(N<AssignMemberStmt>(
          N<CallExpr>(N<IdExpr>(FN_UNWRAP), stmt->lhs), stmt->member, stmt->rhs));
      return;
    }

    if (!member)
      E(Error::DOT_NO_ATTR, stmt->lhs, lhsClass->prettyString(), stmt->member);
    if (lhsClass->getRecord())
      E(Error::ASSIGN_UNEXPECTED_FROZEN, stmt->lhs);

    transform(stmt->rhs);
    auto typ = ctx->instantiate(stmt->lhs->getSrcInfo(), member, lhsClass);
    if (!wrapExpr(stmt->rhs, typ))
      return;
    unify(stmt->rhs->type, typ);
    if (stmt->rhs->isDone())
      stmt->setDone();
  }
}

/// Transform in-place and atomic updates.
/// @example
///   `a += b` -> `a.__iadd__(a, b)` if `__iadd__` exists
///   Atomic operations (when the needed magics are available):
///   `a = b`         -> `type(a).__atomic_xchg__(__ptr__(a), b)`
///   `a += b`        -> `type(a).__atomic_add__(__ptr__(a), b)`
///   `a = min(a, b)` -> `type(a).__atomic_min__(__ptr__(a), b)` (same for `max`)
/// @return a tuple indicating whether (1) the update statement can be replaced with an
///         expression, and (2) the replacement expression.
std::pair<bool, ExprPtr> TypecheckVisitor::transformInplaceUpdate(AssignStmt *stmt) {
  // Case: in-place updates (e.g., `a += b`).
  // They are stored as `Update(a, Binary(a + b, inPlace=true))`
  auto bin = stmt->rhs->getBinary();
  if (bin && bin->inPlace) {
    transform(bin->lexpr);
    transform(bin->rexpr);
    if (bin->lexpr->type->getClass() && bin->rexpr->type->getClass()) {
      if (auto transformed = transformBinaryInplaceMagic(bin, stmt->isAtomicUpdate())) {
        unify(stmt->rhs->type, transformed->type);
        return {true, transformed};
      } else if (!stmt->isAtomicUpdate()) {
        // If atomic, call normal magic and then use __atomic_xchg__ below
        return {false, nullptr};
      }
    } else { // Not yet completed
      unify(stmt->lhs->type, unify(stmt->rhs->type, ctx->getUnbound()));
      return {true, nullptr};
    }
  }

  // Case: atomic min/max operations.
  // Note: check only `a = min(a, b)`; does NOT check `a = min(b, a)`
  auto lhsClass = stmt->lhs->getType()->getClass();
  auto call = stmt->rhs->getCall();
  if (stmt->isAtomicUpdate() && call && stmt->lhs->getId() &&
      (call->expr->isId("min") || call->expr->isId("max")) && call->args.size() == 2 &&
      call->args[0].value->isId(std::string(stmt->lhs->getId()->value))) {
    // `type(a).__atomic_min__(__ptr__(a), b)`
    auto ptrTyp = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(),
                                          ctx->forceFind("Ptr")->type, {lhsClass});
    call->args[1].value = transform(call->args[1].value);
    auto rhsTyp = call->args[1].value->getType()->getClass();
    if (auto method = findBestMethod(
            lhsClass, format("__atomic_{}__", call->expr->getId()->value),
            {ptrTyp, rhsTyp})) {
      return {true, transform(N<CallExpr>(N<IdExpr>(method->ast->name),
                                          N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs),
                                          call->args[1].value))};
    }
  }

  // Case: atomic assignments
  if (stmt->isAtomicUpdate() && lhsClass) {
    // `type(a).__atomic_xchg__(__ptr__(a), b)`
    transform(stmt->rhs);
    if (auto rhsClass = stmt->rhs->getType()->getClass()) {
      auto ptrType = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(),
                                             ctx->forceFind("Ptr")->type, {lhsClass});
      if (auto m = findBestMethod(lhsClass, "__atomic_xchg__", {ptrType, rhsClass})) {
        return {true,
                N<CallExpr>(N<IdExpr>(m->ast->name),
                            N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs), stmt->rhs)};
      }
    }
  }

  return {false, nullptr};
}

} // namespace codon::ast
