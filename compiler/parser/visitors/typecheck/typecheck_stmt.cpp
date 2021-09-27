/*
 * typecheck_stmt.cpp --- Type inference for AST statements.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/visitors/simplify/simplify_ctx.h"
#include "parser/visitors/typecheck/typecheck.h"
#include "parser/visitors/typecheck/typecheck_ctx.h"

using fmt::format;
using std::deque;
using std::dynamic_pointer_cast;
using std::get;
using std::ostream;
using std::stack;
using std::static_pointer_cast;

namespace seq {
namespace ast {

using namespace types;

StmtPtr TypecheckVisitor::transform(const StmtPtr &stmt_) {
  auto &stmt = const_cast<StmtPtr &>(stmt_);
  if (!stmt || stmt->done)
    return stmt;
  TypecheckVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  auto oldAge = ctx->age;
  stmt->age = ctx->age = std::max(stmt->age, oldAge);
  stmt->accept(v);
  ctx->age = oldAge;
  if (v.resultStmt)
    stmt = v.resultStmt;
  if (!v.prependStmts->empty()) {
    if (stmt)
      v.prependStmts->push_back(stmt);
    bool done = true;
    for (auto &s : *(v.prependStmts))
      done &= s->done;
    stmt = N<SuiteStmt>(*v.prependStmts);
    stmt->done = done;
  }
  return stmt;
}

void TypecheckVisitor::defaultVisit(Stmt *s) {
  seqassert(false, "unexpected AST node {}", s->toString());
}

/**************************************************************************************/

void TypecheckVisitor::visit(SuiteStmt *stmt) {
  vector<StmtPtr> stmts;
  stmt->done = true;
  for (auto &s : stmt->stmts)
    if (auto t = transform(s)) {
      stmts.push_back(t);
      stmt->done &= stmts.back()->done;
    }
  stmt->stmts = stmts;
}

void TypecheckVisitor::visit(BreakStmt *stmt) { stmt->done = true; }

void TypecheckVisitor::visit(ContinueStmt *stmt) { stmt->done = true; }

void TypecheckVisitor::visit(ExprStmt *stmt) {
  // Make sure to allow expressions with void type.
  stmt->expr = transform(stmt->expr, false, true);
  stmt->done = stmt->expr->done;
}

void TypecheckVisitor::visit(AssignStmt *stmt) {
  // Simplify stage ensures that lhs is always IdExpr.
  string lhs;
  if (auto e = stmt->lhs->getId())
    lhs = e->value;
  seqassert(!lhs.empty(), "invalid AssignStmt {}", stmt->lhs->toString());
  stmt->rhs = transform(stmt->rhs);
  stmt->type = transformType(stmt->type);
  TypecheckItem::Kind kind;
  if (!stmt->rhs) { // Case 1: forward declaration: x: type
    unify(stmt->lhs->type, stmt->type
                               ? stmt->type->getType()
                               : ctx->addUnbound(stmt->lhs.get(), ctx->typecheckLevel));
    ctx->add(kind = TypecheckItem::Var, lhs, stmt->lhs->type);
    stmt->done = realize(stmt->lhs->type) != nullptr;
  } else if (stmt->type && stmt->type->getType()->isStaticType()) {
    if (!stmt->rhs->isStatic())
      error("right-hand side is not a static expression");
    seqassert(stmt->rhs->staticValue.evaluated, "static not evaluated");
    unify(stmt->type->type, make_shared<StaticType>(stmt->rhs, ctx));
    unify(stmt->lhs->type, stmt->type->getType());
    ctx->add(kind = TypecheckItem::Var, lhs, stmt->lhs->type);
    stmt->done = realize(stmt->lhs->type) != nullptr;
  } else { // Case 2: Normal assignment
    if (stmt->type && stmt->type->getType()->getClass()) {
      auto t = ctx->instantiate(stmt->type.get(), stmt->type->getType());
      unify(stmt->lhs->type, t);
      wrapExpr(stmt->rhs, stmt->lhs->getType(), nullptr);
      unify(stmt->lhs->type, stmt->rhs->type);
    }
    auto type = stmt->rhs->getType();
    kind = stmt->rhs->isType()
               ? TypecheckItem::Type
               : (type->getFunc() ? TypecheckItem::Func : TypecheckItem::Var);
    ctx->add(kind, lhs,
             kind != TypecheckItem::Var ? type->generalize(ctx->typecheckLevel) : type);
    stmt->done = stmt->rhs->done;
  }
  // Save the variable to the local realization context
  ctx->bases.back().visitedAsts[lhs] = {kind, stmt->lhs->type};
}

void TypecheckVisitor::visit(UpdateStmt *stmt) {
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
    if (auto nb = transformBinary(b, stmt->isAtomic, &noReturn))
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
  if (stmt->isAtomic && stmt->lhs->getId() &&
      (c = const_cast<CallExpr *>(stmt->rhs->getCall())) &&
      (c->expr->isId("min") || c->expr->isId("max")) && c->args.size() == 2 &&
      c->args[0].value->isId(string(stmt->lhs->getId()->value))) {
    auto ptrTyp =
        ctx->instantiateGeneric(stmt->lhs.get(), ctx->findInternal("Ptr"), {lhsClass});
    c->args[1].value = transform(c->args[1].value);
    auto rhsTyp = c->args[1].value->getType()->getClass();
    if (auto method = ctx->findBestMethod(
            stmt->lhs.get(), format("__atomic_{}__", c->expr->getId()->value),
            {{"", ptrTyp}, {"", rhsTyp}})) {
      resultStmt = transform(N<ExprStmt>(N<CallExpr>(
          N<IdExpr>(method->ast->name), N<PtrExpr>(stmt->lhs), c->args[1].value)));
      return;
    }
  }

  stmt->rhs = transform(stmt->rhs);
  auto rhsClass = stmt->rhs->getType()->getClass();
  // Case 3: check for an atomic assignment.
  if (stmt->isAtomic && lhsClass && rhsClass) {
    auto ptrType =
        ctx->instantiateGeneric(stmt->lhs.get(), ctx->findInternal("Ptr"), {lhsClass});
    if (auto m = ctx->findBestMethod(stmt->lhs.get(), "__atomic_xchg__",
                                     {{"", ptrType}, {"", rhsClass}})) {
      resultStmt = transform(N<ExprStmt>(
          N<CallExpr>(N<IdExpr>(m->ast->name), N<PtrExpr>(stmt->lhs), stmt->rhs)));
      return;
    }
    stmt->isAtomic = false;
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

void TypecheckVisitor::visit(ReturnStmt *stmt) {
  stmt->expr = transform(stmt->expr);
  if (stmt->expr) {
    auto &base = ctx->bases.back();
    wrapExpr(stmt->expr, base.returnType, nullptr);

    if (stmt->expr->getType()->getFunc() &&
        !(base.returnType->getClass() &&
          startswith(base.returnType->getClass()->name, TYPE_FUNCTION)))
      stmt->expr = partializeFunction(stmt->expr);
    unify(base.returnType, stmt->expr->type);
    auto retTyp = stmt->expr->getType()->getClass();
    stmt->done = stmt->expr->done;
  } else {
    stmt->done = true;
  }
}

void TypecheckVisitor::visit(YieldStmt *stmt) {
  if (stmt->expr)
    stmt->expr = transform(stmt->expr);
  auto baseTyp = stmt->expr ? stmt->expr->getType() : ctx->findInternal("void");
  auto t = ctx->instantiateGeneric(stmt->expr ? stmt->expr.get()
                                              : N<IdExpr>("<yield>").get(),
                                   ctx->findInternal("Generator"), {baseTyp});
  unify(ctx->bases.back().returnType, t);
  stmt->done = stmt->expr ? stmt->expr->done : true;
}

void TypecheckVisitor::visit(WhileStmt *stmt) {
  stmt->cond = transform(stmt->cond);
  stmt->suite = transform(stmt->suite);
  stmt->done = stmt->cond->done && stmt->suite->done;
}

void TypecheckVisitor::visit(ForStmt *stmt) {
  if (stmt->decorator)
    stmt->decorator = transform(stmt->decorator, false, true);

  stmt->iter = transform(stmt->iter);
  // Extract the type of the for variable.
  if (!stmt->iter->getType()->canRealize())
    return; // continue after the iterator is realizable

  auto iterType = stmt->iter->getType()->getClass();
  if (auto tuple = iterType->getHeterogenousTuple()) {
    // Case 1: iterating heterogeneous tuple.
    // Unroll a separate suite for each tuple member.
    auto block = N<SuiteStmt>();
    auto tupleVar = ctx->cache->getTemporaryVar("tuple");
    block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), stmt->iter));

    auto cntVar = ctx->cache->getTemporaryVar("idx");
    vector<StmtPtr> forBlock;
    for (int ai = 0; ai < tuple->args.size(); ai++) {
      vector<StmtPtr> stmts;
      stmts.push_back(N<AssignStmt>(clone(stmt->var),
                                    N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))));
      stmts.push_back(clone(stmt->suite));
      forBlock.push_back(
          N<IfStmt>(N<BinaryExpr>(N<IdExpr>(cntVar), "==", N<IntExpr>(ai)),
                    N<SuiteStmt>(stmts, true)));
    }
    block->stmts.push_back(
        N<ForStmt>(N<IdExpr>(cntVar),
                   N<CallExpr>(N<IdExpr>("std.internal.types.range.range"),
                               N<IntExpr>(tuple->args.size())),
                   N<SuiteStmt>(forBlock)));
    resultStmt = transform(block);
  } else {
    // Case 2: iterating a generator. Standard for loop logic.
    if (iterType->name != "Generator" && !stmt->wrapped) {
      stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->iter, "__iter__")));
      stmt->wrapped = true;
    }
    TypePtr varType = ctx->addUnbound(stmt->var.get(), ctx->typecheckLevel);
    if ((iterType = stmt->iter->getType()->getClass())) {
      if (iterType->name != "Generator")
        error("for loop expected a generator");
      unify(varType, iterType->generics[0].type);
      if (varType->is("void"))
        error("expression with void type");
    }
    string varName;
    if (auto e = stmt->var->getId())
      varName = e->value;
    seqassert(!varName.empty(), "empty for variable {}", stmt->var->toString());
    unify(stmt->var->type, varType);
    ctx->add(TypecheckItem::Var, varName, varType);
    stmt->suite = transform(stmt->suite);
    stmt->done = stmt->iter->done && stmt->suite->done;
  }
}

void TypecheckVisitor::visit(IfStmt *stmt) {
  stmt->cond = transform(stmt->cond);
  if (stmt->cond->isStatic()) {
    if (!stmt->cond->staticValue.evaluated) {
      stmt->done = false; // do not typecheck this suite yet
      return;
    } else {
      bool isTrue = false;
      if (stmt->cond->staticValue.type == StaticValue::STRING)
        isTrue = !stmt->cond->staticValue.getString().empty();
      else
        isTrue = stmt->cond->staticValue.getInt();
      resultStmt = transform(isTrue ? stmt->ifSuite : stmt->elseSuite);
      if (!resultStmt)
        resultStmt = transform(N<SuiteStmt>());
      return;
    }
  } else {
    if (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
      stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));
    stmt->ifSuite = transform(stmt->ifSuite);
    stmt->elseSuite = transform(stmt->elseSuite);
    stmt->done = stmt->cond->done && (!stmt->ifSuite || stmt->ifSuite->done) &&
                 (!stmt->elseSuite || stmt->elseSuite->done);
  }
}

void TypecheckVisitor::visit(TryStmt *stmt) {
  vector<TryStmt::Catch> catches;
  stmt->suite = transform(stmt->suite);
  stmt->done = stmt->suite->done;
  for (auto &c : stmt->catches) {
    c.exc = transformType(c.exc);
    if (!c.var.empty())
      ctx->add(TypecheckItem::Var, c.var, c.exc->getType());
    c.suite = transform(c.suite);
    stmt->done &= (c.exc ? c.exc->done : true) && c.suite->done;
  }
  if (stmt->finally) {
    stmt->finally = transform(stmt->finally);
    stmt->done &= stmt->finally->done;
  }
}

void TypecheckVisitor::visit(ThrowStmt *stmt) {
  stmt->expr = transform(stmt->expr);
  auto tc = stmt->expr->type->getClass();
  if (!stmt->transformed && tc) {
    auto &f = ctx->cache->classes[tc->name].fields;
    if (f.empty() || !f[0].type->getClass() ||
        f[0].type->getClass()->name != TYPE_EXCHEADER)
      error("cannot throw non-exception (first object member must be of type "
            "ExcHeader)");
    auto var = ctx->cache->getTemporaryVar("exc");
    vector<CallExpr::Arg> args;
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(tc->name)});
    args.emplace_back(CallExpr::Arg{
        "", N<DotExpr>(N<DotExpr>(var, ctx->cache->classes[tc->name].fields[0].name),
                       "msg")});
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(ctx->bases.back().name)});
    args.emplace_back(CallExpr::Arg{"", N<StringExpr>(stmt->getSrcInfo().file)});
    args.emplace_back(CallExpr::Arg{"", N<IntExpr>(stmt->getSrcInfo().line)});
    args.emplace_back(CallExpr::Arg{"", N<IntExpr>(stmt->getSrcInfo().col)});
    resultStmt = transform(
        N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(var), stmt->expr),
                     N<AssignMemberStmt>(N<IdExpr>(var),
                                         ctx->cache->classes[tc->name].fields[0].name,
                                         N<CallExpr>(N<IdExpr>(TYPE_EXCHEADER), args)),
                     N<ThrowStmt>(N<IdExpr>(var), true)));
  } else {
    stmt->done = stmt->expr->done;
  }
}

void TypecheckVisitor::visit(FunctionStmt *stmt) {
  auto &attr = stmt->attributes;
  if (ctx->findInVisited(stmt->name).second) {
    stmt->done = true;
    return;
  }

  // Parse preamble.
  bool isClassMember = !attr.parentClass.empty();
  auto explicits = vector<ClassType::Generic>();
  for (const auto &a : stmt->args)
    if (a.generic) {
      char staticType = 0;
      auto idx = a.type->getIndex();
      if (idx && idx->expr->isId("Static"))
        staticType = idx->index->isId("str") ? 1 : 2;
      auto t = ctx->addUnbound(N<IdExpr>(a.name).get(), ctx->typecheckLevel, true,
                               staticType);
      auto typId = ctx->cache->unboundCount - 1;
      t->genericName = ctx->cache->reverseIdentifierLookup[a.name];
      if (a.deflt) {
        auto dt = clone(a.deflt);
        dt = transformType(dt);
        t->defaultType = dt->type;
      }
      explicits.push_back({a.name, ctx->cache->reverseIdentifierLookup[a.name],
                           t->generalize(ctx->typecheckLevel), typId});
      LOG_REALIZE("[generic] {} -> {}", a.name, t->toString());
      ctx->add(TypecheckItem::Type, a.name, t);
    }
  vector<TypePtr> generics;
  if (isClassMember && attr.has(Attr::Method)) {
    // Fetch parent class generics.
    auto parentClassAST = ctx->cache->classes[attr.parentClass].ast.get();
    auto parentClass = ctx->find(attr.parentClass)->type->getClass();
    seqassert(parentClass, "parent class not set");
    for (int i = 0, j = 0; i < parentClassAST->args.size(); i++)
      if (parentClassAST->args[i].generic) {
        // Keep the same ID
        generics.push_back(parentClass->generics[j++].type->instantiate(
            ctx->typecheckLevel - 1, nullptr, nullptr));
        ctx->add(TypecheckItem::Type, parentClassAST->args[i].name, generics.back());
      }
  }
  for (const auto &i : explicits)
    generics.push_back(ctx->find(i.name)->type);
  // Add function arguments.
  auto baseType = ctx->instantiate(N<IdExpr>(stmt->name).get(),
                                   ctx->find(generateFunctionStub(stmt->args.size() -
                                                                  explicits.size()))
                                       ->type)
                      ->getRecord();
  {
    ctx->typecheckLevel++;
    if (stmt->ret) {
      unify(baseType->args[0], transformType(stmt->ret)->getType());
    } else {
      unify(baseType->args[0],
            ctx->addUnbound(N<IdExpr>("<return>").get(), ctx->typecheckLevel));
      generics.push_back(baseType->args[0]);
    }
    for (int ai = 0, aj = 1; ai < stmt->args.size(); ai++) {
      if (!stmt->args[ai].generic && !stmt->args[ai].type) {
        unify(baseType->args[aj], ctx->addUnbound(N<IdExpr>(stmt->args[ai].name).get(),
                                                  ctx->typecheckLevel));
        generics.push_back(baseType->args[aj++]);
      } else if (!stmt->args[ai].generic) {
        unify(baseType->args[aj], transformType(stmt->args[ai].type)->getType());
        generics.push_back(baseType->args[aj]);
        aj++;
      }
    }
    ctx->typecheckLevel--;
  }
  // Generalize generics.
  for (auto g : generics) {
    // seqassert(g && g->getLink() && g->getLink()->kind != types::LinkType::Link,
    // "generic has been unified");
    for (auto &u : g->getUnbounds())
      u->getUnbound()->kind = LinkType::Generic;
  }
  // Construct the type.
  auto typ = make_shared<FuncType>(
      baseType, ctx->cache->functions[stmt->name].ast.get(), explicits);
  if (attr.has(Attr::ForceRealize) || (attr.has(Attr::C) && !attr.has(Attr::CVarArg)))
    if (!typ->canRealize())
      error("builtins and external functions must be realizable");
  if (isClassMember && attr.has(Attr::Method))
    typ->funcParent = ctx->find(attr.parentClass)->type;
  typ->setSrcInfo(stmt->getSrcInfo());
  typ = std::static_pointer_cast<FuncType>(typ->generalize(ctx->typecheckLevel));
  // Check if this is a class method; if so, update the class method lookup table.
  if (isClassMember) {
    auto &methods = ctx->cache->classes[attr.parentClass]
                        .methods[ctx->cache->reverseIdentifierLookup[stmt->name]];
    bool found = false;
    for (auto &i : methods)
      if (i.name == stmt->name) {
        i.type = typ;
        found = true;
        break;
      }
    seqassert(found, "cannot find matching class method for {}", stmt->name);
  }
  // Update visited table.
  ctx->bases[0].visitedAsts[stmt->name] = {TypecheckItem::Func, typ};
  ctx->add(TypecheckItem::Func, stmt->name, typ);
  ctx->cache->functions[stmt->name].type = typ;
  LOG_TYPECHECK("[stmt] added func {}: {} (base={})", stmt->name, typ->debugString(1),
                ctx->getBase());
  stmt->done = true;
}

void TypecheckVisitor::visit(ClassStmt *stmt) {
  auto &attr = stmt->attributes;
  bool extension = attr.has(Attr::Extend);
  if (ctx->findInVisited(stmt->name).second && !extension)
    return;

  ClassTypePtr typ = nullptr;
  if (!extension) {
    if (stmt->isRecord())
      typ = make_shared<RecordType>(stmt->name,
                                    ctx->cache->reverseIdentifierLookup[stmt->name]);
    else
      typ = make_shared<ClassType>(stmt->name,
                                   ctx->cache->reverseIdentifierLookup[stmt->name]);
    typ->setSrcInfo(stmt->getSrcInfo());
    ctx->add(TypecheckItem::Type, stmt->name, typ);
    ctx->bases[0].visitedAsts[stmt->name] = {TypecheckItem::Type, typ};

    // Parse class fields.
    for (const auto &a : stmt->args)
      if (a.generic) {
        char staticType = 0;
        auto idx = a.type->getIndex();
        if (idx && idx->expr->isId("Static"))
          staticType = idx->index->isId("str") ? 1 : 2;
        auto t = ctx->addUnbound(N<IdExpr>(a.name).get(), ctx->typecheckLevel, true,
                                 staticType);
        auto typId = ctx->cache->unboundCount - 1;
        t->getLink()->genericName = ctx->cache->reverseIdentifierLookup[a.name];
        if (a.deflt) {
          auto dt = clone(a.deflt);
          dt = transformType(dt);
          t->defaultType = dt->type;
        }
        typ->generics.push_back({a.name, ctx->cache->reverseIdentifierLookup[a.name],
                                 t->generalize(ctx->typecheckLevel), typId});
        LOG_REALIZE("[generic] {} -> {}", a.name, t->toString());
        ctx->add(TypecheckItem::Type, a.name, t);
      }
    {
      ctx->typecheckLevel++;
      for (auto ai = 0, aj = 0; ai < stmt->args.size(); ai++)
        if (!stmt->args[ai].generic) {
          auto si = stmt->args[ai].type->getSrcInfo();
          ctx->cache->classes[stmt->name].fields[aj].type =
              transformType(stmt->args[ai].type)
                  ->getType()
                  ->generalize(ctx->typecheckLevel - 1);
          ctx->cache->classes[stmt->name].fields[aj].type->setSrcInfo(si);
          if (stmt->isRecord())
            typ->getRecord()->args.push_back(
                ctx->cache->classes[stmt->name].fields[aj].type);
          aj++;
        }
      ctx->typecheckLevel--;
    }
    // Remove lingering generics.
    for (const auto &g : stmt->args)
      if (g.generic) {
        auto val = ctx->find(g.name);
        seqassert(val, "cannot find generic {}", g.name);
        auto t = val->type;
        seqassert(t && t->getLink() && t->getLink()->kind != types::LinkType::Link,
                  "generic has been unified");
        if (t->getLink()->kind == LinkType::Unbound)
          t->getLink()->kind = LinkType::Generic;
        ctx->remove(g.name);
      }

    LOG_REALIZE("[class] {} -> {}", stmt->name, typ->debugString(1));
    for (auto &m : ctx->cache->classes[stmt->name].fields)
      LOG_REALIZE("       - member: {}: {}", m.name, m.type->debugString(1));
  }
  stmt->done = true;
}

} // namespace ast
} // namespace seq
