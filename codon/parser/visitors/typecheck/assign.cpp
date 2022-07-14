#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Transform assignments. Handle dominated assignments, forward declarations, static
/// assignments and type/function aliases.
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignStmt *stmt) {
  // Update statements are handled by @c visitUpdate
  if (stmt->isUpdate()) {
    transformUpdate(stmt);
    return;
  }

  seqassert(stmt->lhs->getId(), "invalid AssignStmt {}", stmt->lhs->toString());
  std::string lhs = stmt->lhs->getId()->value;

  // Special case: this assignment has been dominated and is not a true assignment but
  //               an update of the dominating binding.
  if (in(ctx->cache->replacements, lhs)) {
    const std::pair<std::string, bool> *changed = nullptr;
    while ((changed = in(ctx->cache->replacements, lhs)))
      lhs = changed->first;
    if (stmt->rhs && changed && changed->second) {
      // Mark the dominating binding as used: `var.__used__ = True`
      auto u =
          N<AssignStmt>(N<IdExpr>(fmt::format("{}.__used__", lhs)), N<BoolExpr>(true));
      u->setUpdate();
      prependStmts->push_back(transform(u));
    } else if (changed && changed->second && !stmt->rhs) {
      // This assignment was a declaration only. Just mark the dominating binding as
      // used: `var.__used__ = True`
      stmt->lhs = N<IdExpr>(fmt::format("{}.__used__", lhs));
      stmt->rhs = N<BoolExpr>(true);
    }
    // Change this to the update and follow the update logic
    stmt->setUpdate();
    transformUpdate(stmt);
    return;
  }

  transform(stmt->rhs);
  transformType(stmt->type);
  TypecheckItem::Kind kind;
  if (!stmt->rhs) {
    // Forward declarations (happens with dominating bindings).
    // The type is unknown and will be deduced later
    unify(stmt->lhs->type, ctx->getUnbound(stmt->lhs->getSrcInfo()));
    ctx->add((kind = TypecheckItem::Var), lhs, stmt->lhs->type);
    if (realize(stmt->lhs->type))
      stmt->setDone();
  } else if (stmt->type && stmt->type->getType()->isStaticType()) {
    // Static assignments (e.g., `x: Static[int] = 5`)
    if (!stmt->rhs->isStatic())
      error("right-hand side is not a static expression");
    seqassert(stmt->rhs->staticValue.evaluated, "static not evaluated");
    unify(stmt->lhs->type,
          unify(stmt->type->type, std::make_shared<StaticType>(stmt->rhs, ctx)));
    ctx->add(kind = TypecheckItem::Var, lhs, stmt->lhs->type);
    if (realize(stmt->lhs->type))
      stmt->setDone();
  } else {
    // Normal assignments
    if (stmt->type) {
      unify(stmt->lhs->type,
            ctx->instantiate(stmt->type->getSrcInfo(), stmt->type->getType()));
      // Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
      wrapExpr(stmt->rhs, stmt->lhs->getType());
      unify(stmt->lhs->type, stmt->rhs->type);
    }
    auto type = stmt->rhs->getType();
    if (stmt->rhs->isType())
      kind = TypecheckItem::Type;
    else if (type->getFunc())
      kind = TypecheckItem::Func;
    else
      kind = TypecheckItem::Var;
    // Generalize non-variable types. That way we can support cases like:
    // `a = foo(x, ...); a(1); a('s')`
    auto val = std::make_shared<TypecheckItem>(
        kind,
        kind != TypecheckItem::Var ? type->generalize(ctx->typecheckLevel - 1) : type);

    if (in(ctx->cache->globals, lhs)) {
      // Make globals always visible!
      ctx->addToplevel(lhs, val);
      if (kind != TypecheckItem::Var)
        ctx->cache->globals.erase(lhs);
    } else if (startswith(ctx->getRealizationBase()->name, "._import_") &&
               kind == TypecheckItem::Type) {
      // Make import toplevel type aliases (e.g., `a = Ptr[byte]`) visible
      ctx->addToplevel(lhs, val);
    } else {
      ctx->add(lhs, val);
    }

    if (stmt->lhs->getId() && kind != TypecheckItem::Var) {
      // Special case: type/function renames
      stmt->rhs->type = nullptr;
      stmt->setDone();
    } else if (stmt->rhs->isDone()) {
      stmt->setDone();
    }
  }
}

/// Transform binding updates. Special handling is done for atomic or in-place
/// statements (e.g., `a += b`).
/// See @c transformInplaceUpdate and @c wrapExpr for details.
void TypecheckVisitor::transformUpdate(AssignStmt *stmt) {
  transform(stmt->lhs);
  if (stmt->lhs->isStatic())
    error("cannot modify static expression");

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
  wrapExpr(stmt->rhs, stmt->lhs->getType());
  unify(stmt->lhs->type, stmt->rhs->type);
  if (stmt->rhs->done)
    stmt->setDone();
}

/// Typecheck instance member assignments (e.g., `a.b = c`) and handle optional
/// instances. Disallow
/// @example
///   `opt.foo = bar` -> `unwrap(opt).foo = wrap(bar)`
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignMemberStmt *stmt) {
  transform(stmt->lhs);
  transform(stmt->rhs);

  if (auto lhsClass = stmt->lhs->getType()->getClass()) {
    auto member = ctx->findMember(lhsClass->name, stmt->member);
    if (!member && lhsClass->is(TYPE_OPTIONAL)) {
      // Unwrap optional and look up there:
      resultStmt = transform(N<AssignMemberStmt>(
          N<CallExpr>(N<IdExpr>(FN_UNWRAP), stmt->lhs), stmt->member, stmt->rhs));
      return;
    }

    if (!member)
      error("cannot find '{}' in {}", stmt->member, lhsClass->name);
    if (lhsClass->getRecord())
      error("tuple element '{}' is read-only", stmt->member);
    auto typ = ctx->instantiate(stmt->lhs->getSrcInfo(), member, lhsClass);
    wrapExpr(stmt->rhs, typ);
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
    auto ptrTyp = ctx->instantiateGeneric(stmt->lhs->getSrcInfo(), ctx->getType("Ptr"),
                                          {lhsClass});
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
