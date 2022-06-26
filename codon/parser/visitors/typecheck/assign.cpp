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

void TypecheckVisitor::visit(AssignStmt *stmt) {
  if (stmt->isUpdate()) {
    visitUpdate(stmt);
    return;
  }

  // Simplify stage ensures that lhs is always IdExpr.
  std::string lhs;
  if (auto e = stmt->lhs->getId())
    lhs = e->value;
  seqassert(!lhs.empty(), "invalid AssignStmt {}", stmt->lhs->toString());

  if (in(ctx->cache->replacements, lhs)) {
    auto value = lhs;
    bool hasUsed = false;
    while (auto v = in(ctx->cache->replacements, value))
      value = v->first, hasUsed = v->second;
    if (stmt->rhs && hasUsed) {
      auto u = N<AssignStmt>(N<IdExpr>(fmt::format("{}.__used__", value)),
                             N<BoolExpr>(true));
      u->setUpdate();
      prependStmts->push_back(transform(u));
    } else if (stmt->rhs) {
      ;
    } else if (hasUsed) {
      stmt->lhs = N<IdExpr>(fmt::format("{}.__used__", value));
      stmt->rhs = N<BoolExpr>(true);
    }
    stmt->setUpdate();
    visitUpdate(stmt);
    return;
  }

  stmt->rhs = transform(stmt->rhs);
  stmt->type = transformType(stmt->type);
  TypecheckItem::Kind kind;
  if (!stmt->rhs) { // FORWARD DECLARATION FROM DOMINATION
    seqassert(!stmt->type, "no forward declarations allowed: {}",
              stmt->lhs->toString());
    unify(stmt->lhs->type, ctx->addUnbound(stmt->lhs.get(), ctx->typecheckLevel));
    ctx->add(kind = TypecheckItem::Var, lhs, stmt->lhs->type);
    stmt->done = realize(stmt->lhs->type) != nullptr;
  } else if (stmt->type && stmt->type->getType()->isStaticType()) {
    if (!stmt->rhs->isStatic())
      error("right-hand side is not a static expression");
    seqassert(stmt->rhs->staticValue.evaluated, "static not evaluated");
    unify(stmt->type->type, std::make_shared<StaticType>(stmt->rhs, ctx));
    unify(stmt->lhs->type, stmt->type->getType());
    ctx->add(kind = TypecheckItem::Var, lhs, stmt->lhs->type);
    stmt->done = realize(stmt->lhs->type) != nullptr;
  } else { // Case 2: Normal assignment
    if (stmt->type) {
      auto t = ctx->instantiate(stmt->type.get(), stmt->type->getType());
      unify(stmt->lhs->type, t);
      wrapExpr(stmt->rhs, stmt->lhs->getType(), nullptr);
      unify(stmt->lhs->type, stmt->rhs->type);
    }
    auto type = stmt->rhs->getType();
    kind = stmt->rhs->isType()
               ? TypecheckItem::Type
               : (type->getFunc() ? TypecheckItem::Func : TypecheckItem::Var);
    auto val = std::make_shared<TypecheckItem>(
        kind,
        kind != TypecheckItem::Var ? type->generalize(ctx->typecheckLevel - 1) : type);
    if (in(ctx->cache->globals, lhs)) {
      ctx->addToplevel(lhs, val);
      if (kind != TypecheckItem::Var)
        ctx->cache->globals.erase(lhs);
    } else if (startswith(ctx->bases.back().name, "._import_") &&
               kind == TypecheckItem::Type) { // import toplevel type aliases
      ctx->addToplevel(lhs, val);
    } else {
      ctx->add(lhs, val);
    }
    if (stmt->lhs->getId() && kind != TypecheckItem::Var) { // type/function renames
      // if (stmt->lhs->type)
      // unify(stmt->lhs->type, ctx->find(lhs)->type);
      stmt->rhs->type = nullptr;
      stmt->done = true;
    } else {
      stmt->done = stmt->rhs->done;
    }
  }
  // Save the variable to the local realization context
  ctx->bases.back().visitedAsts[lhs] = {kind, stmt->lhs->type};
}

void TypecheckVisitor::visitUpdate(AssignStmt *stmt) {
  stmt->lhs = transform(stmt->lhs);
  if (stmt->lhs->isStatic())
    error("cannot modify static expression");

  // Case 1: Check for atomic and in-place updates (a += b).
  // In-place updates (a += b) are stored as Update(a, Binary(a + b, inPlace=true)).
  auto b = const_cast<BinaryExpr *>(stmt->rhs->getBinary());
  if (b && b->inPlace) {
    bool noReturn = false;
    auto oldRhsType = stmt->rhs->type;
    b->lexpr = transform(b->lexpr);
    b->rexpr = transform(b->rexpr);
    if (auto nb = transformBinary(b, stmt->isAtomicUpdate(), &noReturn))
      stmt->rhs = nb;
    unify(oldRhsType, stmt->rhs->type);
    if (stmt->rhs->getBinary()) { // still BinaryExpr: will be transformed later.
      unify(stmt->lhs->type, stmt->rhs->type);
      return;
    } else if (noReturn) { // remove assignment, call update function instead
                           // (__i***__ or __atomic_***__)
      bool done = stmt->rhs->done;
      resultStmt = N<ExprStmt>(stmt->rhs);
      resultStmt->done = done;
      return;
    }
  }
  // Case 2: Check for atomic min and max operations: a = min(a, ...).
  // NOTE: does not check for a = min(..., a).
  auto lhsClass = stmt->lhs->getType()->getClass();
  CallExpr *c;
  if (stmt->isAtomicUpdate() && stmt->lhs->getId() &&
      (c = const_cast<CallExpr *>(stmt->rhs->getCall())) &&
      (c->expr->isId("min") || c->expr->isId("max")) && c->args.size() == 2 &&
      c->args[0].value->isId(std::string(stmt->lhs->getId()->value))) {
    auto ptrTyp =
        ctx->instantiateGeneric(stmt->lhs.get(), ctx->findInternal("Ptr"), {lhsClass});
    c->args[1].value = transform(c->args[1].value);
    auto rhsTyp = c->args[1].value->getType()->getClass();
    if (auto method = findBestMethod(stmt->lhs.get(),
                                     format("__atomic_{}__", c->expr->getId()->value),
                                     {ptrTyp, rhsTyp})) {
      resultStmt = transform(N<ExprStmt>(
          N<CallExpr>(N<IdExpr>(method->ast->name),
                      N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs), c->args[1].value)));
      return;
    }
  }

  stmt->rhs = transform(stmt->rhs);

  auto rhsClass = stmt->rhs->getType()->getClass();
  // Case 3: check for an atomic assignment.
  if (stmt->isAtomicUpdate() && lhsClass && rhsClass) {
    auto ptrType =
        ctx->instantiateGeneric(stmt->lhs.get(), ctx->findInternal("Ptr"), {lhsClass});
    if (auto m =
            findBestMethod(stmt->lhs.get(), "__atomic_xchg__", {ptrType, rhsClass})) {
      resultStmt = transform(N<ExprStmt>(
          N<CallExpr>(N<IdExpr>(m->ast->name),
                      N<CallExpr>(N<IdExpr>("__ptr__"), stmt->lhs), stmt->rhs)));
      return;
    }
    stmt->setUpdate(); // turn off atomic
  }
  // Case 4: handle optionals if needed.
  wrapExpr(stmt->rhs, stmt->lhs->getType(), nullptr);
  unify(stmt->lhs->type, stmt->rhs->type);
  stmt->done = stmt->rhs->done;
}

void TypecheckVisitor::visit(AssignMemberStmt *stmt) {
  stmt->lhs = transform(stmt->lhs);
  stmt->rhs = transform(stmt->rhs);
  auto lhsClass = stmt->lhs->getType()->getClass();

  if (lhsClass) {
    auto member = ctx->findMember(lhsClass->name, stmt->member);
    if (!member && lhsClass->name == TYPE_OPTIONAL) {
      // Unwrap optional and look up there:
      resultStmt = transform(N<AssignMemberStmt>(
          N<CallExpr>(N<IdExpr>(FN_UNWRAP), stmt->lhs), stmt->member, stmt->rhs));
      return;
    }
    if (!member)
      error("cannot find '{}' in {}", stmt->member, lhsClass->name);
    if (lhsClass->getRecord())
      error("tuple element '{}' is read-only", stmt->member);
    auto typ = ctx->instantiate(stmt->lhs.get(), member, lhsClass.get());
    wrapExpr(stmt->rhs, typ, nullptr);
    unify(stmt->rhs->type, typ);
    stmt->done = stmt->rhs->done;
  }
}

}