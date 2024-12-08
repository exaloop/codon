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
  auto a = N<AssignStmt>(clone(expr->getVar()), expr->getExpr());
  a->cloneAttributesFrom(expr);
  resultExpr = transform(N<StmtExpr>(a, expr->getVar()));
}

/// Transform assignments. Handle dominated assignments, forward declarations, static
/// assignments and type/function aliases.
/// See @c transformAssignment and @c unpackAssignments for more details.
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignStmt *stmt) {
  if (cast<TupleExpr>(stmt->lhs) || cast<ListExpr>(stmt->lhs)) {
    resultStmt = transform(unpackAssignment(stmt->lhs, stmt->rhs));
    return;
  }

  bool mustUpdate = stmt->isUpdate() || stmt->isAtomicUpdate();
  mustUpdate |= stmt->getLhs()->hasAttribute(Attr::ExprDominated);
  mustUpdate |= stmt->getLhs()->hasAttribute(Attr::ExprDominatedUsed);
  if (cast<BinaryExpr>(stmt->getRhs()) &&
      cast<BinaryExpr>(stmt->getRhs())->isInPlace()) {
    // Update case: a += b
    seqassert(!stmt->getTypeExpr(), "invalid AssignStmt {}", stmt->toString(0));
    mustUpdate = true;
  }

  resultStmt = transformAssignment(stmt, mustUpdate);
  if (stmt->getLhs()->hasAttribute(Attr::ExprDominatedUsed)) {
    // If this is dominated, set __used__ if needed
    stmt->getLhs()->eraseAttribute(Attr::ExprDominatedUsed);
    auto e = cast<IdExpr>(stmt->getLhs());
    seqassert(e, "dominated bad assignment");
    resultStmt = transform(N<SuiteStmt>(
        resultStmt,
        N<AssignStmt>(
            N<IdExpr>(format("{}{}", getUnmangledName(e->getValue()), VAR_USED_SUFFIX)),
            N<BoolExpr>(true), nullptr, AssignStmt::UpdateMode::Update)));
  }
}

/// Transform deletions.
/// @example
///   `del a`    -> `a = type(a)()` and remove `a` from the context
///   `del a[x]` -> `a.__delitem__(x)`
void TypecheckVisitor::visit(DelStmt *stmt) {
  if (auto idx = cast<IndexExpr>(stmt->getExpr())) {
    resultStmt = N<ExprStmt>(transform(
        N<CallExpr>(N<DotExpr>(idx->getExpr(), "__delitem__"), idx->getIndex())));
  } else if (auto ei = cast<IdExpr>(stmt->getExpr())) {
    // Assign `a` to `type(a)()` to mark it for deletion
    resultStmt = transform(N<AssignStmt>(
        stmt->getExpr(),
        N<CallExpr>(N<CallExpr>(N<IdExpr>(TYPE_TYPE), clone(stmt->getExpr()))), nullptr,
        AssignStmt::Update));

    // Allow deletion *only* if the binding is dominated
    auto val = ctx->find(ei->getValue());
    if (!val)
      E(Error::ID_NOT_FOUND, ei, ei->getValue());
    if (ctx->getScope() != val->scope)
      E(Error::DEL_NOT_ALLOWED, ei, ei->getValue());
    ctx->remove(ei->getValue());
    ctx->remove(getUnmangledName(ei->getValue()));
  } else {
    E(Error::DEL_INVALID, stmt);
  }
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
Stmt *TypecheckVisitor::unpackAssignment(Expr *lhs, Expr *rhs) {
  std::vector<Expr *> leftSide;
  if (auto et = cast<TupleExpr>(lhs)) {
    // Case: (a, b) = ...
    for (auto *i : *et)
      leftSide.push_back(i);
  } else if (auto el = cast<ListExpr>(lhs)) {
    // Case: [a, b] = ...
    for (auto *i : *el)
      leftSide.push_back(i);
  } else {
    return N<AssignStmt>(lhs, rhs);
  }

  // Prepare the right-side expression
  auto oldSrcInfo = getSrcInfo();
  setSrcInfo(rhs->getSrcInfo());
  auto srcPos = rhs;
  SuiteStmt *block = N<SuiteStmt>();
  if (!cast<IdExpr>(rhs)) {
    // Store any non-trivial right-side expression into a variable
    auto var = getTemporaryVar("assign");
    auto newRhs = N<IdExpr>(var);
    block->addStmt(N<AssignStmt>(newRhs, ast::clone(rhs)));
    rhs = newRhs;
  }

  // Process assignments until the fist StarExpr (if any)
  size_t st = 0;
  for (; st < leftSide.size(); st++) {
    if (cast<StarExpr>(leftSide[st]))
      break;
    // Transformation: `leftSide_st = rhs[st]` where `st` is static integer
    auto rightSide = N<IndexExpr>(ast::clone(rhs), N<IntExpr>(st));
    // Recursively process the assignment because of cases like `(a, (b, c)) = d)`
    auto ns = unpackAssignment(leftSide[st], rightSide);
    block->addStmt(ns);
  }
  // Process StarExpr (if any) and the assignments that follow it
  if (st < leftSide.size() && cast<StarExpr>(leftSide[st])) {
    // StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes `d[1:-2]`)
    auto rightSide = N<IndexExpr>(
        ast::clone(rhs),
        N<SliceExpr>(N<IntExpr>(st),
                     // this slice is either [st:] or [st:-lhs_len + st + 1]
                     leftSide.size() == st + 1 ? nullptr
                                               : N<IntExpr>(-leftSide.size() + st + 1),
                     nullptr));
    auto ns = unpackAssignment(cast<StarExpr>(leftSide[st])->getExpr(), rightSide);
    block->addStmt(ns);
    st += 1;
    // Process remaining assignments. They will use negative indices (-1, -2 etc.)
    // because we do not know how big is StarExpr
    for (; st < leftSide.size(); st++) {
      if (cast<StarExpr>(leftSide[st]))
        E(Error::ASSIGN_MULTI_STAR, leftSide[st]->getSrcInfo());
      rightSide = N<IndexExpr>(ast::clone(rhs), N<IntExpr>(-int(leftSide.size() - st)));
      auto ns = unpackAssignment(leftSide[st], rightSide);
      block->addStmt(ns);
    }
  }
  setSrcInfo(oldSrcInfo);
  return block;
}

/// Transform simple assignments.
/// @example
///   `a[x] = b`    -> `a.__setitem__(x, b)`
///   `a.x = b`     -> @c AssignMemberStmt
///   `a: type` = b -> @c AssignStmt
///   `a = b`       -> @c AssignStmt or @c UpdateStmt (see below)
Stmt *TypecheckVisitor::transformAssignment(AssignStmt *stmt, bool mustExist) {
  if (auto idx = cast<IndexExpr>(stmt->getLhs())) {
    // Case: a[x] = b
    seqassert(!stmt->type, "unexpected type annotation");
    if (auto b = cast<BinaryExpr>(stmt->getRhs())) {
      // Case: a[x] += b (inplace operator)
      if (mustExist && b->isInPlace() && !cast<IdExpr>(b->getRhs())) {
        auto var = getTemporaryVar("assign");
        return transform(N<SuiteStmt>(
            N<AssignStmt>(N<IdExpr>(var), idx->getIndex()),
            N<ExprStmt>(N<CallExpr>(
                N<DotExpr>(idx->getExpr(), "__setitem__"), N<IdExpr>(var),
                N<BinaryExpr>(N<IndexExpr>(clone(idx->getExpr()), N<IdExpr>(var)),
                              b->getOp(), b->getRhs(), true)))));
      }
    }
    return transform(N<ExprStmt>(N<CallExpr>(N<DotExpr>(idx->getExpr(), "__setitem__"),
                                             idx->getIndex(), stmt->getRhs())));
  }

  if (auto dot = cast<DotExpr>(stmt->getLhs())) {
    // Case: a.x = b
    seqassert(!stmt->type, "unexpected type annotation");
    dot->expr = transform(dot->getExpr(), true);
    return transform(
        N<AssignMemberStmt>(dot->getExpr(), dot->member, transform(stmt->getRhs())));
  }

  // Case: a (: t) = b
  auto e = cast<IdExpr>(stmt->getLhs());
  if (!e)
    E(Error::ASSIGN_INVALID, stmt->getLhs());

  auto val = ctx->find(e->getValue());
  // Make sure that existing values that cannot be shadowed are only updated
  // mustExist |= val && !ctx->isOuter(val);
  if (mustExist) {
    if (!val)
      E(Error::ASSIGN_LOCAL_REFERENCE, e, e->getValue(), e->getSrcInfo());

    auto s = N<AssignStmt>(stmt->getLhs(), stmt->getRhs());
    if (!ctx->getBase()->isType() && ctx->getBase()->func->hasAttribute(Attr::Atomic))
      s->setAtomicUpdate();
    else
      s->setUpdate();
    if (auto u = transformUpdate(s))
      return u;
    else
      return s; // delay
  }

  stmt->rhs = transform(stmt->getRhs(), true);
  stmt->type = transformType(stmt->getTypeExpr(), false);

  // Generate new canonical variable name for this assignment and add it to the context
  auto canonical = ctx->generateCanonicalName(e->getValue());
  auto assign =
      N<AssignStmt>(N<IdExpr>(canonical), stmt->getRhs(), stmt->getTypeExpr());
  assign->getLhs()->cloneAttributesFrom(stmt->getLhs());
  assign->getLhs()->setType(stmt->getLhs()->getType()
                                ? stmt->getLhs()->getType()->shared_from_this()
                                : instantiateUnbound(assign->getLhs()->getSrcInfo()));
  if (!stmt->getRhs() && !stmt->getTypeExpr() && ctx->find("NoneType")) {
    // All declarations that are not handled are to be marked with NoneType later on
    // (useful for dangling declarations that are not initialized afterwards due to
    //  static check)
    assign->getLhs()->getType()->getLink()->defaultType =
        getStdLibType("NoneType")->shared_from_this();
    ctx->getBase()->pendingDefaults.insert(
        assign->getLhs()->getType()->shared_from_this());
  }
  if (stmt->getTypeExpr()) {
    unify(assign->getLhs()->getType(),
          instantiateType(stmt->getTypeExpr()->getSrcInfo(),
                          extractType(stmt->getTypeExpr())));
  }
  val = std::make_shared<TypecheckItem>(canonical, ctx->getBaseName(), ctx->getModule(),
                                        assign->getLhs()->getType()->shared_from_this(),
                                        ctx->getScope());
  val->setSrcInfo(getSrcInfo());
  ctx->add(e->getValue(), val);
  ctx->addAlwaysVisible(val);

  if (assign->getRhs()) { // not a declaration!
    // Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
    if (wrapExpr(&assign->rhs, assign->getLhs()->getType()))
      unify(assign->getLhs()->getType(), assign->getRhs()->getType());

    // Generalize non-variable types. That way we can support cases like:
    // `a = foo(x, ...); a(1); a('s')`
    if (!val->isVar()) {
      val->type = val->type->generalize(ctx->typecheckLevel - 1);
      assign->getLhs()->setType(val->type);
      assign->getRhs()->setType(val->type);
    }
  }

  // Mark declarations or generalizedtype/functions as done
  if (((!assign->getRhs() || assign->getRhs()->isDone()) &&
       realize(assign->getLhs()->getType())) ||
      (assign->getRhs() && !val->isVar() && !val->type->hasUnbounds())) {
    assign->setDone();
  }

  // Register all toplevel variables as global in JIT mode
  bool isGlobal = (ctx->cache->isJit && val->isGlobal() && !val->isGeneric()) ||
                  (canonical == VAR_ARGV);
  if (isGlobal && val->isVar())
    registerGlobal(canonical, assign->getRhs());

  return assign;
}

/// Transform binding updates. Special handling is done for atomic or in-place
/// statements (e.g., `a += b`).
/// See @c transformInplaceUpdate and @c wrapExpr for details.
Stmt *TypecheckVisitor::transformUpdate(AssignStmt *stmt) {
  stmt->lhs = transform(stmt->getLhs());

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

  stmt->rhs = transform(stmt->getRhs());

  // Case: wrap expressions if needed (e.g. floats or optionals)
  if (wrapExpr(&stmt->rhs, stmt->getLhs()->getType()))
    unify(stmt->getRhs()->getType(), stmt->getLhs()->getType());
  if (stmt->getRhs()->isDone() && realize(stmt->getLhs()->getType()))
    stmt->setDone();
  return nullptr;
}

/// Typecheck instance member assignments (e.g., `a.b = c`) and handle optional
/// instances. Disallow tuple updates.
/// @example
///   `opt.foo = bar` -> `unwrap(opt).foo = wrap(bar)`
/// See @c wrapExpr for more examples.
void TypecheckVisitor::visit(AssignMemberStmt *stmt) {
  stmt->lhs = transform(stmt->getLhs());

  if (auto lhsClass = extractClassType(stmt->getLhs())) {
    auto member = findMember(lhsClass, stmt->getMember());
    if (!member) {
      // Case: property setters
      auto setters =
          findMethod(lhsClass, format("{}{}", FN_SETTER_SUFFIX, stmt->getMember()));
      if (!setters.empty()) {
        resultStmt =
            transform(N<ExprStmt>(N<CallExpr>(N<IdExpr>(setters.front()->getFuncName()),
                                              stmt->getLhs(), stmt->getRhs())));
        return;
      }
      // Case: class variables
      if (auto cls = getClass(lhsClass))
        if (auto var = in(cls->classVars, stmt->getMember())) {
          auto a = N<AssignStmt>(N<IdExpr>(*var), transform(stmt->getRhs()));
          a->setUpdate();
          resultStmt = transform(a);
          return;
        }
    }
    if (!member && lhsClass->is(TYPE_OPTIONAL)) {
      // Unwrap optional and look up there
      resultStmt = transform(
          N<AssignMemberStmt>(N<CallExpr>(N<IdExpr>(FN_UNWRAP), stmt->getLhs()),
                              stmt->getMember(), stmt->getRhs()));
      return;
    }

    if (!member)
      E(Error::DOT_NO_ATTR, stmt->getLhs(), lhsClass->prettyString(),
        stmt->getMember());
    if (lhsClass->isRecord()) // prevent tuple member assignment
      E(Error::ASSIGN_UNEXPECTED_FROZEN, stmt->getLhs());

    stmt->rhs = transform(stmt->getRhs());
    auto ftyp =
        instantiateType(stmt->getLhs()->getSrcInfo(), member->getType(), lhsClass);
    if (!ftyp->canRealize() && member->typeExpr) {
      unify(ftyp.get(), extractType(withClassGenerics(lhsClass, [&]() {
              return transform(clean_clone(member->typeExpr));
            })));
    }
    if (!wrapExpr(&stmt->rhs, ftyp.get()))
      return;
    unify(stmt->getRhs()->getType(), ftyp.get());
    if (stmt->getRhs()->isDone())
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

  auto bin = cast<BinaryExpr>(stmt->getRhs());
  if (bin && bin->isInPlace()) {
    bin->lexpr = transform(bin->getLhs());
    bin->rexpr = transform(bin->getRhs());

    if (!stmt->getRhs()->getType())
      stmt->getRhs()->setType(instantiateUnbound());
    if (bin->getLhs()->getClassType() && bin->getRhs()->getClassType()) {
      if (auto transformed = transformBinaryInplaceMagic(bin, stmt->isAtomicUpdate())) {
        unify(stmt->getRhs()->getType(), transformed->getType());
        return {true, transformed};
      } else if (!stmt->isAtomicUpdate()) {
        // If atomic, call normal magic and then use __atomic_xchg__ below
        return {false, nullptr};
      }
    } else { // Delay
      unify(stmt->lhs->getType(),
            unify(stmt->getRhs()->getType(), instantiateUnbound()));
      return {true, nullptr};
    }
  }

  // Case: atomic min/max operations.
  // Note: check only `a = min(a, b)`; does NOT check `a = min(b, a)`
  auto lhsClass = extractClassType(stmt->getLhs());
  auto call = cast<CallExpr>(stmt->getRhs());
  auto lei = cast<IdExpr>(stmt->getLhs());
  auto cei = call ? cast<IdExpr>(call->getExpr()) : nullptr;
  if (stmt->isAtomicUpdate() && call && lei && cei &&
      (cei->getValue() == "min" || cei->getValue() == "max") && call->size() == 2) {
    call->front().value = transform(call->front());
    if (cast<IdExpr>(call->front()) &&
        cast<IdExpr>(call->front())->getValue() == lei->getValue()) {
      // `type(a).__atomic_min__(__ptr__(a), b)`
      auto ptrTyp = instantiateType(stmt->getLhs()->getSrcInfo(), getStdLibType("Ptr"),
                                    std::vector<types::Type *>{lhsClass});
      (*call)[1].value = transform((*call)[1]);
      auto rhsTyp = extractClassType((*call)[1].value);
      if (auto method =
              findBestMethod(lhsClass, format("__atomic_{}__", cei->getValue()),
                             {ptrTyp.get(), rhsTyp})) {
        return {true,
                transform(N<CallExpr>(N<IdExpr>(method->getFuncName()),
                                      N<CallExpr>(N<IdExpr>("__ptr__"), stmt->getLhs()),
                                      (*call)[1]))};
      }
    }
  }

  // Case: atomic assignments
  if (stmt->isAtomicUpdate() && lhsClass) {
    // `type(a).__atomic_xchg__(__ptr__(a), b)`
    stmt->rhs = transform(stmt->getRhs());
    if (auto rhsClass = stmt->getRhs()->getClassType()) {
      auto ptrType = instantiateType(stmt->getLhs()->getSrcInfo(), getStdLibType("Ptr"),
                                     std::vector<types::Type *>{lhsClass});
      if (auto m =
              findBestMethod(lhsClass, "__atomic_xchg__", {ptrType.get(), rhsClass})) {
        return {true, N<CallExpr>(N<IdExpr>(m->getFuncName()),
                                  N<CallExpr>(N<IdExpr>("__ptr__"), stmt->getLhs()),
                                  stmt->getRhs())};
      }
    }
  }

  return {false, nullptr};
}

} // namespace codon::ast
