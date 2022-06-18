#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/ctx.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/parser/visitors/typecheck/typecheck_ctx.h"

using fmt::format;

namespace codon {
namespace ast {

using namespace types;

StmtPtr TypecheckVisitor::transform(StmtPtr &stmt) {
  if (!stmt || stmt->done)
    return stmt;
  TypecheckVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  auto oldAge = ctx->age;
  stmt->age = ctx->age = std::max(stmt->age, oldAge);
  ctx->pushSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  ctx->popSrcInfo();
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
  if (stmt->done)
    ctx->changedNodes++;
  return stmt;
}

void TypecheckVisitor::defaultVisit(Stmt *s) {
  seqassert(false, "unexpected AST node {}", s->toString());
}

/**************************************************************************************/

void TypecheckVisitor::visit(SuiteStmt *stmt) {
  std::vector<StmtPtr> stmts;
  stmt->done = true;
  for (auto &s : stmt->stmts) {
    if (ctx->returnEarly)
      break;
    if (auto t = transform(s)) {
      stmts.push_back(t);
      stmt->done &= stmts.back()->done;
    }
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

void TypecheckVisitor::visit(ReturnStmt *stmt) {
  stmt->expr = transform(stmt->expr);
  if (stmt->expr) {
    auto &base = ctx->bases.back();
    if (!base.returnType->getUnbound())
      wrapExpr(stmt->expr, base.returnType, nullptr);

    if (stmt->expr->getType()->getFunc() &&
        !(base.returnType->getClass() &&
          startswith(base.returnType->getClass()->name, "Function")))
      stmt->expr = partializeFunction(stmt->expr);
    unify(base.returnType, stmt->expr->type);
    stmt->done = stmt->expr->done;
  } else {
    if (!ctx->blockLevel)
      ctx->returnEarly = true;
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
  ctx->blockLevel++;
  stmt->suite = transform(stmt->suite);
  ctx->blockLevel--;
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
    std::vector<StmtPtr> forBlock;
    for (int ai = 0; ai < tuple->args.size(); ai++) {
      std::vector<StmtPtr> stmts;
      stmts.push_back(N<AssignStmt>(clone(stmt->var),
                                    N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))));
      stmts.push_back(clone(stmt->suite));
      forBlock.push_back(N<IfStmt>(
          N<BinaryExpr>(N<IdExpr>(cntVar), "==", N<IntExpr>(ai)), N<SuiteStmt>(stmts)));
    }
    block->stmts.push_back(
        N<ForStmt>(N<IdExpr>(cntVar),
                   N<CallExpr>(N<IdExpr>("std.internal.types.range.range"),
                               N<IntExpr>(tuple->args.size())),
                   N<SuiteStmt>(forBlock)));
    ctx->blockLevel++;
    resultStmt = transform(block);
    ctx->blockLevel--;
  } else {
    // Case 2: iterating a generator. Standard for loop logic.
    if (iterType->name != "Generator" && !stmt->wrapped) {
      stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->iter, "__iter__")));
      stmt->wrapped = true;
    }
    seqassert(stmt->var->getId(), "for variable corrupt: {}", stmt->var->toString());

    auto e = stmt->var->getId();
    bool changed = in(ctx->cache->replacements, e->value);
    while (auto s = in(ctx->cache->replacements, e->value))
      e->value = s->first;
    if (changed) {
      auto u =
          N<AssignStmt>(N<IdExpr>(format("{}.__used__", e->value)), N<BoolExpr>(true));
      u->setUpdate();
      stmt->suite = N<SuiteStmt>(u, stmt->suite);
    }
    std::string varName = e->value;

    TypePtr varType = nullptr;
    if (stmt->ownVar) {
      varType = ctx->addUnbound(stmt->var.get(), ctx->typecheckLevel);
      ctx->add(TypecheckItem::Var, varName, varType);
    } else {
      auto val = ctx->find(varName);
      seqassert(val, "'{}' not found", varName);
      varType = val->type;
    }
    if ((iterType = stmt->iter->getType()->getClass())) {
      if (iterType->name != "Generator")
        error("for loop expected a generator");
      if (getSrcInfo().line == 378 && varType->toString() == "List[int]" &&
          iterType->generics[0].type->toString() == "int") {
        ctx->find(varName);
      }
      unify(varType, iterType->generics[0].type);
      if (varType->is("void"))
        error("expression with void type");
    }

    unify(stmt->var->type, varType);

    ctx->blockLevel++;
    stmt->suite = transform(stmt->suite);
    ctx->blockLevel--;
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
      resultStmt = isTrue ? stmt->ifSuite : stmt->elseSuite;
      resultStmt = transform(resultStmt);
      if (!resultStmt)
        resultStmt = transform(N<SuiteStmt>());
      return;
    }
  } else {
    if (stmt->cond->type->getClass() && !stmt->cond->type->is("bool"))
      stmt->cond = transform(N<CallExpr>(N<DotExpr>(stmt->cond, "__bool__")));
    ctx->blockLevel++;
    stmt->ifSuite = transform(stmt->ifSuite);
    stmt->elseSuite = transform(stmt->elseSuite);
    ctx->blockLevel--;
    stmt->done = stmt->cond->done && (!stmt->ifSuite || stmt->ifSuite->done) &&
                 (!stmt->elseSuite || stmt->elseSuite->done);
  }
}

void TypecheckVisitor::visit(TryStmt *stmt) {
  std::vector<TryStmt::Catch> catches;
  ctx->blockLevel++;
  stmt->suite = transform(stmt->suite);
  ctx->blockLevel--;
  stmt->done = stmt->suite->done;
  for (auto &c : stmt->catches) {
    c.exc = transformType(c.exc);
    if (!c.var.empty()) {
      bool changed = in(ctx->cache->replacements, c.var);
      while (auto s = in(ctx->cache->replacements, c.var))
        c.var = s->first;
      if (changed) {
        auto u =
            N<AssignStmt>(N<IdExpr>(format("{}.__used__", c.var)), N<BoolExpr>(true));
        u->setUpdate();
        c.suite = N<SuiteStmt>(u, c.suite);
      }

      if (c.ownVar) {
        ctx->add(TypecheckItem::Var, c.var, c.exc->getType());
      } else {
        auto val = ctx->find(c.var);
        seqassert(val, "'{}' not found", c.var);
        unify(val->type, c.exc->getType());
      }
    }
    ctx->blockLevel++;
    c.suite = transform(c.suite);
    ctx->blockLevel--;
    stmt->done &= (c.exc ? c.exc->done : true) && c.suite->done;
  }
  if (stmt->finally) {
    ctx->blockLevel++;
    stmt->finally = transform(stmt->finally);
    ctx->blockLevel--;
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
    std::vector<CallExpr::Arg> args;
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
  auto explicits = std::vector<ClassType::Generic>();
  for (const auto &a : stmt->args)
    if (a.status == Param::Generic) {
      char staticType = 0;
      auto idx = a.type->getIndex();
      if (idx && idx->expr->isId("Static"))
        staticType = idx->index->isId("str") ? 1 : 2;
      auto t = ctx->addUnbound(N<IdExpr>(a.name).get(), ctx->typecheckLevel, true,
                               staticType);
      auto typId = ctx->cache->unboundCount - 1;
      t->genericName = ctx->cache->reverseIdentifierLookup[a.name];
      if (a.defaultValue) {
        auto dt = clone(a.defaultValue);
        dt = transformType(dt);
        t->defaultType = dt->type;
      }
      explicits.push_back({a.name, ctx->cache->reverseIdentifierLookup[a.name],
                           t->generalize(ctx->typecheckLevel), typId});
      LOG_REALIZE("[generic] {} -> {}", a.name, t->toString());
      ctx->add(TypecheckItem::Type, a.name, t);
    }
  std::vector<TypePtr> generics;
  ClassTypePtr parentClass = nullptr;
  if (isClassMember && attr.has(Attr::Method)) {
    // Fetch parent class generics.
    auto parentClassAST = ctx->cache->classes[attr.parentClass].ast.get();
    parentClass = ctx->find(attr.parentClass)->type->getClass();
    parentClass =
        parentClass->instantiate(ctx->typecheckLevel - 1, nullptr, nullptr)->getClass();
    seqassert(parentClass, "parent class not set");
    for (int i = 0, j = 0, k = 0; i < parentClassAST->args.size(); i++) {
      if (parentClassAST->args[i].status != Param::Normal) {
        generics.push_back(parentClassAST->args[i].status == Param::Generic
                               ? parentClass->generics[j++].type
                               : parentClass->hiddenGenerics[k++].type);
        ctx->add(TypecheckItem::Type, parentClassAST->args[i].name, generics.back());
      }
    }
  }
  for (const auto &i : explicits)
    generics.push_back(ctx->find(i.name)->type);
  // Add function arguments.
  auto baseType = getFuncTypeBase(stmt->args.size() - explicits.size());
  {
    ctx->typecheckLevel++;
    if (stmt->ret) {
      unify(baseType->generics[1].type, transformType(stmt->ret)->getType());
    } else {
      unify(baseType->generics[1].type,
            ctx->addUnbound(N<IdExpr>("<return>").get(), ctx->typecheckLevel));
      generics.push_back(baseType->generics[1].type);
    }
    // tuple
    auto argType = baseType->generics[0].type->getRecord();
    for (int ai = 0, aj = 0; ai < stmt->args.size(); ai++) {
      if (stmt->args[ai].status == Param::Normal && !stmt->args[ai].type) {
        if (parentClass && ai == 0 &&
            ctx->cache->reverseIdentifierLookup[stmt->args[ai].name] == "self") {
          unify(argType->args[aj], parentClass);
        } else {
          unify(argType->args[aj], ctx->addUnbound(N<IdExpr>(stmt->args[ai].name).get(),
                                                   ctx->typecheckLevel));
        }
        generics.push_back(argType->args[aj++]);
      } else if (stmt->args[ai].status == Param::Normal) {
        unify(argType->args[aj], transformType(stmt->args[ai].type)->getType());
        generics.push_back(argType->args[aj]);
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
  auto typ = std::make_shared<FuncType>(
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
    auto m = ctx->cache->classes[attr.parentClass]
                 .methods[ctx->cache->reverseIdentifierLookup[stmt->name]];
    bool found = false;
    for (auto &i : ctx->cache->overloads[m])
      if (i.name == stmt->name) {
        ctx->cache->functions[i.name].type = typ;
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
      typ = std::make_shared<RecordType>(
          stmt->name, ctx->cache->reverseIdentifierLookup[stmt->name]);
    else
      typ = std::make_shared<ClassType>(
          stmt->name, ctx->cache->reverseIdentifierLookup[stmt->name]);
    if (stmt->isRecord() && startswith(stmt->name, TYPE_PARTIAL)) {
      seqassert(in(ctx->cache->partials, stmt->name),
                "invalid partial initialization: {}", stmt->name);
      typ = std::make_shared<PartialType>(typ->getRecord(),
                                          ctx->cache->partials[stmt->name].first,
                                          ctx->cache->partials[stmt->name].second);
    }
    typ->setSrcInfo(stmt->getSrcInfo());
    ctx->add(TypecheckItem::Type, stmt->name, typ);
    ctx->bases[0].visitedAsts[stmt->name] = {TypecheckItem::Type, typ};

    // Parse class fields.
    for (const auto &a : stmt->args)
      if (a.status != Param::Normal) { // TODO
        char staticType = 0;
        auto idx = a.type->getIndex();
        if (idx && idx->expr->isId("Static"))
          staticType = idx->index->isId("str") ? 1 : 2;
        auto t = ctx->addUnbound(N<IdExpr>(a.name).get(), ctx->typecheckLevel, true,
                                 staticType);
        auto typId = ctx->cache->unboundCount - 1;
        t->getLink()->genericName = ctx->cache->reverseIdentifierLookup[a.name];
        if (a.defaultValue) {
          auto dt = clone(a.defaultValue);
          dt = transformType(dt);
          if (a.status == Param::Generic)
            t->defaultType = dt->type;
          else
            unify(dt->type, t);
        }
        ctx->add(TypecheckItem::Type, a.name, t);
        ClassType::Generic g{a.name, ctx->cache->reverseIdentifierLookup[a.name],
                             t->generalize(ctx->typecheckLevel), typId};
        if (a.status == Param::Generic) {
          typ->generics.push_back(g);
        } else {
          typ->hiddenGenerics.push_back(g);
        }
        LOG_REALIZE("[generic/{}] {} -> {}", int(a.status), a.name, t->toString());
      }
    {
      ctx->typecheckLevel++;
      for (auto ai = 0, aj = 0; ai < stmt->args.size(); ai++)
        if (stmt->args[ai].status == Param::Normal) {
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
      if (g.status != Param::Normal) {
        auto val = ctx->find(g.name);
        seqassert(val, "cannot find generic {}", g.name);
        auto t = val->type;
        if (g.status == Param::Generic) {
          seqassert(t && t->getLink() && t->getLink()->kind != types::LinkType::Link,
                    "generic has been unified");
          if (t->getLink()->kind == LinkType::Unbound)
            t->getLink()->kind = LinkType::Generic;
        }
        ctx->remove(g.name);
      }

    LOG_REALIZE("[class] {} -> {}", stmt->name, typ->debugString(1));
    for (auto &m : ctx->cache->classes[stmt->name].fields)
      LOG_REALIZE("       - member: {}: {}", m.name, m.type->debugString(1));
  }
  stmt->done = true;
}

std::string TypecheckVisitor::getRootName(const std::string &name) {
  auto p = name.rfind(':');
  seqassert(p != std::string::npos, ": not found in {}", name);
  return name.substr(0, p);
}

std::shared_ptr<RecordType> TypecheckVisitor::getFuncTypeBase(int nargs) {
  auto baseType = ctx->instantiate(nullptr, ctx->find("Function")->type)->getRecord();
  auto argType =
      ctx->instantiate(nullptr, ctx->find(generateTupleStub(nargs))->type)->getRecord();
  unify(baseType->generics[0].type, argType);
  return baseType;
}

} // namespace ast
} // namespace codon
