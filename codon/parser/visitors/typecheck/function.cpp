// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Unify the function return type with `Generator[?]`.
/// The unbound type will be deduced from return/yield statements.
void TypecheckVisitor::visit(YieldExpr *expr) {
  unify(expr->type, ctx->getUnbound());
  unify(ctx->getRealizationBase()->returnType,
        ctx->instantiateGeneric(ctx->getType("Generator"), {expr->type}));
  if (realize(expr->type))
    expr->setDone();
}

/// Typecheck return statements. Empty return is transformed to `return NoneType()`.
/// Also partialize functions if they are being returned.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(ReturnStmt *stmt) {
  if (!stmt->expr && ctx->getRealizationBase()->type &&
      ctx->getRealizationBase()->type->getFunc()->ast->hasAttr(Attr::IsGenerator)) {
    stmt->setDone();
  } else {
    if (!stmt->expr) {
      stmt->expr = N<CallExpr>(N<IdExpr>("NoneType"));
    }
    transform(stmt->expr);
    // Wrap expression to match the return type
    if (!ctx->getRealizationBase()->returnType->getUnbound())
      if (!wrapExpr(stmt->expr, ctx->getRealizationBase()->returnType)) {
        return;
      }

    // Special case: partialize functions if we are returning them
    if (stmt->expr->getType()->getFunc() &&
        !(ctx->getRealizationBase()->returnType->getClass() &&
          ctx->getRealizationBase()->returnType->is("Function"))) {
      stmt->expr = partializeFunction(stmt->expr->type->getFunc());
    }

    unify(ctx->getRealizationBase()->returnType, stmt->expr->type);
  }

  // If we are not within conditional block, ignore later statements in this function.
  // Useful with static if statements.
  if (!ctx->blockLevel)
    ctx->returnEarly = true;

  if (!stmt->expr || stmt->expr->isDone())
    stmt->setDone();
}

/// Typecheck yield statements. Empty yields assume `NoneType`.
void TypecheckVisitor::visit(YieldStmt *stmt) {
  stmt->expr = transform(stmt->expr ? stmt->expr : N<CallExpr>(N<IdExpr>("NoneType")));

  auto t = ctx->instantiateGeneric(ctx->getType("Generator"), {stmt->expr->type});
  unify(ctx->getRealizationBase()->returnType, t);

  if (stmt->expr->isDone())
    stmt->setDone();
}

/// Parse a function stub and create a corresponding generic function type.
/// Also realize built-ins and extern C functions.
void TypecheckVisitor::visit(FunctionStmt *stmt) {
  // Function should be constructed only once
  stmt->setDone();

  auto funcTyp = makeFunctionType(stmt);
  // If this is a class method, update the method lookup table
  bool isClassMember = !stmt->attributes.parentClass.empty();
  if (isClassMember) {
    auto m =
        ctx->cache->getMethod(ctx->find(stmt->attributes.parentClass)->type->getClass(),
                              ctx->cache->rev(stmt->name));
    bool found = false;
    for (auto &i : ctx->cache->overloads[m])
      if (i.name == stmt->name) {
        ctx->cache->functions[i.name].type = funcTyp;
        found = true;
        break;
      }
    seqassert(found, "cannot find matching class method for {}", stmt->name);
  }

  // Update the visited table
  // Functions should always be visible, so add them to the toplevel
  ctx->addToplevel(stmt->name,
                   std::make_shared<TypecheckItem>(TypecheckItem::Func, funcTyp));
  ctx->cache->functions[stmt->name].type = funcTyp;

  // Ensure that functions with @C, @force_realize, and @export attributes can be
  // realized
  if (stmt->attributes.has(Attr::ForceRealize) || stmt->attributes.has(Attr::Export) ||
      (stmt->attributes.has(Attr::C) && !stmt->attributes.has(Attr::CVarArg))) {
    if (!funcTyp->canRealize())
      E(Error::FN_REALIZE_BUILTIN, stmt);
  }

  // Debug information
  LOG_REALIZE("[stmt] added func {}: {}", stmt->name, funcTyp);
}

types::FuncTypePtr TypecheckVisitor::makeFunctionType(FunctionStmt *stmt) {
  // Handle generics
  bool isClassMember = !stmt->attributes.parentClass.empty();
  auto explicits = std::vector<ClassType::Generic>();
  for (const auto &a : stmt->args) {
    if (a.status == Param::Generic) {
      // Generic and static types
      auto generic = ctx->getUnbound();
      generic->isStatic = getStaticGeneric(a.type.get());
      auto typId = generic->getLink()->id;
      generic->genericName = ctx->cache->rev(a.name);
      if (a.defaultValue) {
        auto defType = transformType(clone(a.defaultValue));
        generic->defaultType = defType->type;
      }
      ctx->add(TypecheckItem::Type, a.name, generic);
      explicits.emplace_back(a.name, ctx->cache->rev(a.name),
                             generic->generalize(ctx->typecheckLevel), typId);
    }
  }

  // Prepare list of all generic types
  std::vector<TypePtr> generics;
  ClassTypePtr parentClass = nullptr;
  if (isClassMember && stmt->attributes.has(Attr::Method)) {
    // Get class generics (e.g., T for `class Cls[T]: def foo:`)
    auto parentClassAST = ctx->cache->classes[stmt->attributes.parentClass].ast.get();
    parentClass = ctx->forceFind(stmt->attributes.parentClass)->type->getClass();
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
  // Add function generics
  for (const auto &i : explicits)
    generics.push_back(ctx->find(i.name)->type);

  // Handle function arguments
  // Base type: `Function[[args,...], ret]`
  auto baseType = getFuncTypeBase(stmt->args.size() - explicits.size());
  ctx->typecheckLevel++;
  if (stmt->ret) {
    unify(baseType->generics[1].type, transformType(stmt->ret)->getType());
    if (stmt->ret->isId("Union")) {
      baseType->generics[1].type->getUnion()->generics[0].type->getUnbound()->kind =
          LinkType::Generic;
    }
  } else {
    generics.push_back(unify(baseType->generics[1].type, ctx->getUnbound()));
  }
  // Unify base type generics with argument types
  auto argType = baseType->generics[0].type->getRecord();
  for (int ai = 0, aj = 0; ai < stmt->args.size(); ai++) {
    if (stmt->args[ai].status == Param::Normal && !stmt->args[ai].type) {
      if (parentClass && ai == 0 && ctx->cache->rev(stmt->args[ai].name) == "self") {
        // Special case: self in methods
        unify(argType->args[aj], parentClass);
      } else {
        unify(argType->args[aj], ctx->getUnbound());
      }
      generics.push_back(argType->args[aj++]);
    } else if (stmt->args[ai].status == Param::Normal &&
               startswith(stmt->args[ai].name, "*")) {
      // Special case: `*args: type` and `**kwargs: type`. Do not add this type to the
      // signature (as the real type is `Tuple[type, ...]`); it will be used during call
      // typechecking
      unify(argType->args[aj], ctx->getUnbound());
      generics.push_back(argType->args[aj++]);
    } else if (stmt->args[ai].status == Param::Normal) {
      unify(argType->args[aj], transformType(stmt->args[ai].type)->getType());
      generics.push_back(argType->args[aj++]);
    }
  }
  ctx->typecheckLevel--;

  // Generalize generics and remove them from the context
  for (const auto &g : generics) {
    for (auto &u : g->getUnbounds())
      if (u->getUnbound())
        u->getUnbound()->kind = LinkType::Generic;
  }

  // Construct the type
  auto funcTyp = std::make_shared<FuncType>(
      baseType, ctx->cache->functions[stmt->name].ast.get(), explicits);

  funcTyp->setSrcInfo(getSrcInfo());
  if (isClassMember && stmt->attributes.has(Attr::Method)) {
    funcTyp->funcParent = ctx->find(stmt->attributes.parentClass)->type;
  }
  funcTyp =
      std::static_pointer_cast<FuncType>(funcTyp->generalize(ctx->typecheckLevel));
  return funcTyp;
}

/// Make an empty partial call `fn(...)` for a given function.
ExprPtr TypecheckVisitor::partializeFunction(const types::FuncTypePtr &fn) {
  // Create function mask
  std::vector<char> mask(fn->ast->args.size(), 0);
  for (int i = 0, j = 0; i < fn->ast->args.size(); i++)
    if (fn->ast->args[i].status == Param::Generic) {
      if (!fn->funcGenerics[j].type->getUnbound())
        mask[i] = 1;
      j++;
    }

  // Generate partial class
  auto partialTypeName = generatePartialStub(mask, fn.get());
  std::string var = ctx->cache->getTemporaryVar("partial");
  // Generate kwtuple for potential **kwargs
  auto kwName = generateTuple(0, TYPE_KWTUPLE, {});
  // `partial = Partial.MASK((), KwTuple())`
  // (`()` for *args and `KwTuple()` for **kwargs)
  ExprPtr call =
      N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var),
                                N<CallExpr>(N<IdExpr>(partialTypeName), N<TupleExpr>(),
                                            N<CallExpr>(N<IdExpr>(kwName)))),
                  N<IdExpr>(var));
  call->setAttr(ExprAttr::Partial);
  transform(call);
  seqassert(call->type->getPartial(), "expected partial type");
  return call;
}

/// Generate and return `Function[Tuple[args...], ret]` type
std::shared_ptr<RecordType> TypecheckVisitor::getFuncTypeBase(size_t nargs) {
  auto baseType = ctx->instantiate(ctx->forceFind("Function")->type)->getRecord();
  unify(baseType->generics[0].type, ctx->instantiateTuple(nargs)->getRecord());
  return baseType;
}

} // namespace codon::ast
