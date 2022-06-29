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

void TypecheckVisitor::visit(YieldExpr *expr) {
  seqassert(!ctx->bases.empty(), "yield outside of a function");
  auto typ = ctx->instantiateGeneric(expr, ctx->getType("Generator"),
                                     {ctx->addUnbound(expr, ctx->typecheckLevel)});
  unify(ctx->bases.back().returnType, typ);
  unify(expr->type, typ->getClass()->generics[0].type);
  expr->done = realize(expr->type) != nullptr;
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
    stmt->expr = transform(N<CallExpr>(N<IdExpr>("NoneType")));
    stmt->done = true;
  }
}

void TypecheckVisitor::visit(YieldStmt *stmt) {
  if (stmt->expr)
    stmt->expr = transform(stmt->expr);
  auto baseTyp = stmt->expr ? stmt->expr->getType() : ctx->getType("NoneType");
  auto t = ctx->instantiateGeneric(stmt->expr ? stmt->expr.get()
                                              : N<IdExpr>("<yield>").get(),
                                   ctx->getType("Generator"), {baseTyp});
  unify(ctx->bases.back().returnType, t);
  stmt->done = stmt->expr ? stmt->expr->done : true;
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
  if (attr.has(Attr::ForceRealize) || attr.has(Attr::Export) ||
      (attr.has(Attr::C) && !attr.has(Attr::CVarArg)))
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

} // namespace codon::ast