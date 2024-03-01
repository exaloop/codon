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
  seqassert(false, "AssignExpr should be inlined by a previous pass: '{}'", *expr);
}

/// Transform assignments. Handle dominated assignments, forward declarations, static
/// assignments and type/function aliases.
/// See @c transformAssignment and @c unpackAssignments for more details.
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignStmt *stmt) {
  bool mustUpdate = stmt->isUpdate();
  if (stmt->rhs && stmt->rhs->getBinary() && stmt->rhs->getBinary()->inPlace) {
    // Update case: a += b
    seqassert(!stmt->type, "invalid AssignStmt {}", stmt->toString());
    mustUpdate = true;
  }
  resultStmt = transformAssignment(stmt, mustUpdate);
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
    auto tA = N<CallExpr>(N<CallExpr>(N<IdExpr>("type"), clone(stmt->expr)));
    resultStmt = N<AssignStmt>(transform(stmt->expr), transform(tA));
    resultStmt->getAssign()->setUpdate();

    // Allow deletion *only* if the binding is dominated
    auto val = ctx->find(ei->value);
    if (!val)
      E(Error::ID_NOT_FOUND, ei, ei->value);
    if (ctx->getScope() != val->scope)
      E(Error::DEL_NOT_ALLOWED, ei, ei->value);
    ctx->remove(ei->value);
    ctx->remove(ctx->cache->rev(ei->value));
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
StmtPtr TypecheckVisitor::transformAssignment(AssignStmt *stmt, bool mustExist) {
  if (auto idx = stmt->lhs->getIndex()) {
    // Case: a[x] = b
    seqassert(!stmt->type, "unexpected type annotation");
    return transform(N<ExprStmt>(
        N<CallExpr>(N<DotExpr>(idx->expr, "__setitem__"), idx->index, stmt->rhs)));
  }

  if (auto dot = stmt->lhs->getDot()) {
    // Case: a.x = b
    seqassert(!stmt->type, "unexpected type annotation");
    transform(dot->expr, true);
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
  auto e = stmt->lhs->getId();
  if (!e)
    E(Error::ASSIGN_INVALID, stmt->lhs);

  auto val = ctx->find(e->value);
  // Make sure that existing values that cannot be shadowed are only updated
  // mustExist |= val && !ctx->isOuter(val);
  if (mustExist) {
    if (val) {
      // commented out: should be handled by namevisitor
      auto s = N<AssignStmt>(stmt->lhs, stmt->rhs);
      if (ctx->getBase()->attributes && ctx->getBase()->attributes->has(Attr::Atomic))
        s->setAtomicUpdate();
      else
        s->setUpdate();
      if (auto u = transformUpdate(s.get()))
        return u;
      return s;
    } else {
      E(Error::ASSIGN_LOCAL_REFERENCE, e, e->value, e->getSrcInfo());
    }
  }

  transform(stmt->rhs, true);
  transformType(stmt->type, false);

  // Generate new canonical variable name for this assignment and add it to the context
  auto canonical = ctx->generateCanonicalName(e->value);
  auto assign = N<AssignStmt>(N<IdExpr>(canonical), stmt->rhs, stmt->type);
  assign->lhs->attributes = stmt->lhs->attributes;

  unify(assign->lhs->type, ctx->getUnbound(assign->lhs->getSrcInfo()));
  if (!stmt->rhs && !stmt->type && ctx->find("NoneType")) {
    // All declarations that are not handled are to be marked with NoneType later on
    assign->lhs->type->getLink()->defaultType = ctx->getType("NoneType");
    ctx->pendingDefaults.insert(assign->lhs->type);
  }
  if (stmt->type) {
    unify(assign->lhs->type,
          ctx->instantiate(stmt->type->getSrcInfo(), getType(stmt->type)));
  }
  val = std::make_shared<TypecheckItem>(canonical, ctx->getBaseName(), ctx->getModule(),
                                        assign->lhs->type, ctx->getScope());
  val->setSrcInfo(getSrcInfo());
  ctx->add(e->value, val);
  ctx->addAlwaysVisible(val);

  if (assign->rhs) { // not a declaration!
    // Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
    if (wrapExpr(assign->rhs, assign->lhs->getType()))
      unify(assign->lhs->type, assign->rhs->type);
    // auto type = assign->lhs->getType();

    // Generalize non-variable types. That way we can support cases like:
    // `a = foo(x, ...); a(1); a('s')`
    if (!val->isVar()) {
      val->type = val->type->generalize(ctx->typecheckLevel - 1);
      assign->lhs->type = assign->rhs->type = val->type;
      // LOG("->gene: {} / {} / {}", val->type, assign->toString(), val->type->canRealize());
      // realize(assign->lhs->type);
    }
  }

  if ((!assign->rhs || assign->rhs->isDone()) && realize(assign->lhs->type)) {
    assign->setDone();
  } else if (assign->rhs && !val->isVar() && val->type->getUnbounds().empty()) {
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
StmtPtr TypecheckVisitor::transformUpdate(AssignStmt *stmt) {
  transform(stmt->lhs);

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

  transform(stmt->rhs);
  // Case: wrap expressions if needed (e.g. floats or optionals)
  if (wrapExpr(stmt->rhs, stmt->lhs->getType()))
    unify(stmt->rhs->type, stmt->lhs->type);
  if (stmt->rhs->done && realize(stmt->lhs->type))
    stmt->setDone();
  return nullptr;
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

    if (!member && stmt->lhs->type->is("type")) {
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
    transform(bin->lexpr, true, /* allowStatic */ false);
    transform(bin->rexpr, true, /* allowStatic */ false);

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
      (call->expr->isId("min") || call->expr->isId("max")) && call->args.size() == 2) {
    transform(call->args[0].value);
    if (call->args[0].value->isId(std::string(stmt->lhs->getId()->value))) {
      // `type(a).__atomic_min__(__ptr__(a), b)`
      auto ptrTyp = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(),
                                            ctx->getType("Ptr"), {lhsClass});
      call->args[1].value = transform(call->args[1].value);
      auto rhsTyp = call->args[1].value->getType()->getClass();
      if (auto method = findBestMethod(
              lhsClass, format("__atomic_{}__", call->expr->getId()->value),
              {ptrTyp, rhsTyp})) {
        return {true,
                transform(N<CallExpr>(N<IdExpr>(method->ast->name),
                                      N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs),
                                      call->args[1].value))};
      }
    }
  }

  // Case: atomic assignments
  if (stmt->isAtomicUpdate() && lhsClass) {
    // `type(a).__atomic_xchg__(__ptr__(a), b)`
    transform(stmt->rhs);
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
