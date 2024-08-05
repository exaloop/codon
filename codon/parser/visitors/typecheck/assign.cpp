// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/cir/attribute.h"
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
  auto a = N<AssignStmt>(clone(expr->var), expr->expr);
  a->cloneAttributesFrom(expr);
  resultExpr = transform(N<StmtExpr>(a, expr->var));
}

/// Transform assignments. Handle dominated assignments, forward declarations, static
/// assignments and type/function aliases.
/// See @c transformAssignment and @c unpackAssignments for more details.
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignStmt *stmt) {
  bool mustUpdate = stmt->isUpdate() || stmt->isAtomicUpdate();
  mustUpdate |= stmt->hasAttribute(Attr::ExprDominated);
  mustUpdate |= stmt->hasAttribute(Attr::ExprDominatedUsed);
  if (stmt->rhs && cast<BinaryExpr>(stmt->rhs) &&
      cast<BinaryExpr>(stmt->rhs)->isInPlace()) {
    // Update case: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString(0));
    mustUpdate = true;
  }
  resultStmt = transformAssignment(stmt, mustUpdate);
  if (stmt->hasAttribute(Attr::ExprDominatedUsed)) {
    stmt->eraseAttribute(Attr::ExprDominatedUsed);
    auto lei = cast<IdExpr>(stmt->lhs);
    seqassert(lei, "dominated bad assignment");
    resultStmt = transform(N<SuiteStmt>(
        resultStmt,
        N<AssignStmt>(
            N<IdExpr>(format("{}.__used__", ctx->cache->rev(lei->getValue()))),
            N<BoolExpr>(true), nullptr, AssignStmt::UpdateMode::Update)));
  }
}

/// Transform deletions.
/// @example
///   `del a`    -> `a = type(a)()` and remove `a` from the context
///   `del a[x]` -> `a.__delitem__(x)`
void TypecheckVisitor::visit(DelStmt *stmt) {
  if (auto idx = cast<IndexExpr>(stmt->expr)) {
    resultStmt = N<ExprStmt>(
        transform(N<CallExpr>(N<DotExpr>(idx->expr, "__delitem__"), idx->index)));
  } else if (auto ei = cast<IdExpr>(stmt->expr)) {
    // Assign `a` to `type(a)()` to mark it for deletion
    auto tA = N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)));
    resultStmt = N<AssignStmt>(transform(stmt->expr), transform(tA));
    cast<AssignStmt>(resultStmt)->setUpdate();

    // Allow deletion *only* if the binding is dominated
    auto val = ctx->find(ei->getValue());
    if (!val)
      E(Error::ID_NOT_FOUND, ei, ei->getValue());
    if (ctx->getScope() != val->scope)
      E(Error::DEL_NOT_ALLOWED, ei, ei->getValue());
    ctx->remove(ei->getValue());
    ctx->remove(ctx->cache->rev(ei->getValue()));
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
Stmt *TypecheckVisitor::transformAssignment(AssignStmt *stmt, bool mustExist) {
  if (auto idx = cast<IndexExpr>(stmt->lhs)) {
    // Case: a[x] = b
    seqassert(!stmt->type, "unexpected type annotation");
    if (auto b = cast<BinaryExpr>(stmt->rhs)) {
      if (mustExist && b->isInPlace() && !cast<IdExpr>(b->getRhs())) {
        auto var = ctx->cache->getTemporaryVar("assign");
        return transform(N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(var), idx->index),
            N<ExprStmt>(N<CallExpr>(
                N<DotExpr>(idx->expr, "__setitem__"), N<IdExpr>(var),
                N<BinaryExpr>(N<IndexExpr>(clone(idx->expr), N<IdExpr>(var)),
                              b->getOp(), b->getRhs(), true)))));
      }
    }
    return transform(N<ExprStmt>(
        N<CallExpr>(N<DotExpr>(idx->expr, "__setitem__"), idx->index, stmt->rhs)));
  }

  if (auto dot = cast<DotExpr>(stmt->lhs)) {
    // Case: a.x = b
    seqassert(!stmt->type, "unexpected type annotation");
    dot->expr = transform(dot->expr, true);
    // If we are deducing class members, check if we can deduce a member from this
    // assignment
    // todo)) deduction!
    // auto deduced = ctx->getClassBase() ? ctx->getClassBase()->deducedMembers :
    // nullptr; if (deduced && dot->expr->isId(ctx->getBase()->selfName) &&
    //     !in(*deduced, dot->member))
    //   deduced->push_back(dot->member);
    return transform(N<AssignMemberStmt>(dot->expr, dot->member, transform(stmt->rhs)));
  }

  // Case: a (: t) = b
  auto e = cast<IdExpr>(stmt->lhs);
  if (!e)
    E(Error::ASSIGN_INVALID, stmt->lhs);

  auto val = ctx->find(e->getValue());
  // Make sure that existing values that cannot be shadowed are only updated
  // mustExist |= val && !ctx->isOuter(val);
  if (mustExist) {
    if (val) {
      // commented out: should be handled by namevisitor
      auto s = N<AssignStmt>(stmt->lhs, stmt->rhs);
      if (!ctx->getBase()->isType() && ctx->getBase()->func->hasAttribute(Attr::Atomic))
        s->setAtomicUpdate();
      else
        s->setUpdate();
      if (auto u = transformUpdate(s))
        return u;
      return s;
    } else {
      E(Error::ASSIGN_LOCAL_REFERENCE, e, e->getValue(), e->getSrcInfo());
    }
  }

  stmt->rhs = transform(stmt->rhs, true);
  stmt->type = transformType(stmt->type, false);

  // Generate new canonical variable name for this assignment and add it to the context
  auto canonical = ctx->generateCanonicalName(e->getValue());
  auto assign = N<AssignStmt>(N<IdExpr>(canonical), stmt->rhs, stmt->type);
  assign->lhs->cloneAttributesFrom(stmt->lhs);

  if (!stmt->lhs->getType())
    assign->lhs->setType(ctx->getUnbound(assign->lhs->getSrcInfo()));
  else
    assign->lhs->setType(stmt->lhs->getType());
  if (!stmt->rhs && !stmt->type && ctx->find("NoneType")) {
    // All declarations that are not handled are to be marked with NoneType later on
    assign->lhs->getType()->getLink()->defaultType = ctx->getType("NoneType");
    ctx->getBase()->pendingDefaults.insert(assign->lhs->getType());
  }
  if (stmt->type) {
    unify(assign->lhs->getType(),
          ctx->instantiate(stmt->type->getSrcInfo(), getType(stmt->type)));
  }
  val = std::make_shared<TypecheckItem>(canonical, ctx->getBaseName(), ctx->getModule(),
                                        assign->lhs->getType(), ctx->getScope());
  val->setSrcInfo(getSrcInfo());
  ctx->add(e->getValue(), val);
  ctx->addAlwaysVisible(val);

  if (assign->rhs) { // not a declaration!
    // Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
    if (wrapExpr(&assign->rhs, assign->lhs->getType()))
      unify(assign->lhs->getType(), assign->rhs->getType());
    // auto type = assign->lhs->getType();

    // Generalize non-variable types. That way we can support cases like:
    // `a = foo(x, ...); a(1); a('s')`
    if (!val->isVar()) {
      val->type = val->type->generalize(ctx->typecheckLevel - 1);
      assign->lhs->setType(val->type);
      assign->rhs->setType(val->type);
      // LOG("->gene: {} / {} / {}", val->type, assign->toString(),
      // val->type->canRealize()); realize(assign->lhs->type);
    }
  }

  if ((!assign->rhs || assign->rhs->isDone()) && realize(assign->lhs->getType())) {
    assign->setDone();
  } else if (assign->rhs && !val->isVar() && !val->type->hasUnbounds()) {
    // TODO: this is?!
    assign->setDone();
  }

  // Register all toplevel variables as global in JIT mode
  bool isGlobal = (ctx->cache->isJit && val->isGlobal() && !val->isGeneric()) ||
                  (canonical == VAR_ARGV);
  if (isGlobal && val->isVar())
    ctx->cache->addGlobal(canonical);

  return assign;
}

/// Transform binding updates. Special handling is done for atomic or in-place
/// statements (e.g., `a += b`).
/// See @c transformInplaceUpdate and @c wrapExpr for details.
Stmt *TypecheckVisitor::transformUpdate(AssignStmt *stmt) {
  stmt->lhs = transform(stmt->lhs);

  // Check inplace updates
  auto [inPlace, inPlaceExpr] = transformInplaceUpdate(stmt);
  if (inPlace) {
    if (inPlaceExpr) {
      auto s = N<ExprStmt>(inPlaceExpr);
      if (inPlaceExpr->isDone())
        s->setDone();
      return s;
    }
    return nullptr;
  }

  stmt->rhs = transform(stmt->rhs);
  // Case: wrap expressions if needed (e.g. floats or optionals)
  if (wrapExpr(&stmt->rhs, stmt->lhs->getType()))
    unify(stmt->rhs->getType(), stmt->lhs->getType());
  if (stmt->rhs->isDone() && realize(stmt->lhs->getType()))
    stmt->setDone();
  return nullptr;
}

/// Typecheck instance member assignments (e.g., `a.b = c`) and handle optional
/// instances. Disallow tuple updates.
/// @example
///   `opt.foo = bar` -> `unwrap(opt).foo = wrap(bar)`
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignMemberStmt *stmt) {
  stmt->lhs = transform(stmt->lhs);

  if (auto lhsClass = getType(stmt->lhs)->getClass()) {
    auto member = ctx->findMember(lhsClass, stmt->member);
    if (!member) {
      // Case: setters
      auto setters = ctx->findMethod(lhsClass.get(), format(".set_{}", stmt->member));
      if (!setters.empty()) {
        resultStmt = transform(N<ExprStmt>(
            N<CallExpr>(N<IdExpr>(setters[0]->ast->name), stmt->lhs, stmt->rhs)));
        return;
      }
      // Case: class variables
      if (auto cls = ctx->cache->getClass(lhsClass))
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
    if (lhsClass->isRecord())
      E(Error::ASSIGN_UNEXPECTED_FROZEN, stmt->lhs);

    stmt->rhs = transform(stmt->rhs);
    auto ftyp = ctx->instantiate(stmt->lhs->getSrcInfo(), member->type, lhsClass);
    if (!ftyp->canRealize() && member->typeExpr) {
      ctx->addBlock();
      addClassGenerics(lhsClass);
      auto t = ctx->getType(transform(clean_clone(member->typeExpr))->getType());
      ctx->popBlock();
      unify(ftyp, t);
    }
    if (!wrapExpr(&stmt->rhs, ftyp))
      return;
    unify(stmt->rhs->getType(), ftyp);
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
std::pair<bool, Expr *> TypecheckVisitor::transformInplaceUpdate(AssignStmt *stmt) {
  // Case: in-place updates (e.g., `a += b`).
  // They are stored as `Update(a, Binary(a + b, inPlace=true))`

  auto bin = cast<BinaryExpr>(stmt->rhs);
  if (bin && bin->isInPlace()) {
    bin->lexpr = transform(bin->getLhs());
    bin->rexpr = transform(bin->getRhs());

    if (!stmt->rhs->getType())
      stmt->rhs->setType(ctx->getUnbound());
    if (bin->getLhs()->getClassType() && bin->getRhs()->getClassType()) {
      if (auto transformed = transformBinaryInplaceMagic(bin, stmt->isAtomicUpdate())) {
        unify(stmt->rhs->getType(), transformed->getType());
        return {true, transformed};
      } else if (!stmt->isAtomicUpdate()) {
        // If atomic, call normal magic and then use __atomic_xchg__ below
        return {false, nullptr};
      }
    } else { // Not yet completed
      unify(stmt->lhs->getType(), unify(stmt->rhs->getType(), ctx->getUnbound()));
      return {true, nullptr};
    }
  }

  // Case: atomic min/max operations.
  // Note: check only `a = min(a, b)`; does NOT check `a = min(b, a)`
  auto lhsClass = stmt->lhs->getClassType();
  auto call = cast<CallExpr>(stmt->rhs);
  auto lei = cast<IdExpr>(stmt->lhs);
  auto cei = call ? cast<IdExpr>(call->expr) : nullptr;
  if (stmt->isAtomicUpdate() && call && lei && cei &&
      (cei->getValue() == "min" || cei->getValue() == "max") && call->size() == 2) {
    (*call)[0].value = transform((*call)[0].value);
    if (cast<IdExpr>((*call)[0].value) &&
        cast<IdExpr>((*call)[0].value)->getValue() == lei->getValue()) {
      // `type(a).__atomic_min__(__ptr__(a), b)`
      auto ptrTyp = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(),
                                            ctx->getType("Ptr"), {lhsClass});
      (*call)[1].value = transform((*call)[1].value);
      auto rhsTyp = (*call)[1].value->getType()->getClass();
      if (auto method = findBestMethod(
              lhsClass, format("__atomic_{}__", cei->getValue()), {ptrTyp, rhsTyp})) {
        return {true,
                transform(N<CallExpr>(N<IdExpr>(method->ast->name),
                                      N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs),
                                      (*call)[1].value))};
      }
    }
  }

  // Case: atomic assignments
  if (stmt->isAtomicUpdate() && lhsClass) {
    // `type(a).__atomic_xchg__(__ptr__(a), b)`
    stmt->rhs = transform(stmt->rhs);
    if (auto rhsClass = stmt->rhs->getType()->getClass()) {
      auto ptrType = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(),
                                             ctx->getType("Ptr"), {lhsClass});
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
