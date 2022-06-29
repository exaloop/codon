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

/// Typecheck identifiers. If an identifier is a static variable, evaluate it and
/// replace it with its value (e.g., a @c IntExpr ). Also ensure that the identifier of
/// a generic function or a type is fully qualified (e.g., replace "Ptr" with
/// "Ptr[byte]").
/// For tuple identifiers, generate appropriate class. See @c generateTupleStub for
/// details.
void TypecheckVisitor::visit(IdExpr *expr) {
  // Generate tuple stubs if needed
  if (isTuple(expr->value))
    generateTupleStub(std::stoi(expr->value.substr(sizeof(TYPE_TUPLE) - 1)));

  // Handle empty callable references
  if (expr->value == TYPE_CALLABLE) {
    auto typ = ctx->addUnbound(expr, ctx->typecheckLevel);
    typ->getLink()->trait = std::make_shared<CallableTrait>(std::vector<TypePtr>{});
    unify(expr->type, typ);
    expr->markType();
    return;
  }

  // Replace identifiers that have been superseded by domination analysis during the
  // simplification
  while (auto s = in(ctx->cache->replacements, expr->value))
    expr->value = s->first;

  auto val = ctx->find(expr->value);
  if (!val) {
    // Handle overloads
    if (auto i = in(ctx->cache->overloads, expr->value)) {
      if (i->size() == 1) {
        val = ctx->forceFind(i->front().name);
      } else {
        auto d = findDispatch(expr->value);
        val = ctx->forceFind(d->ast->name);
      }
    }
    if (!val)
      seqassert(expr, "cannot find '{}'", expr->value);
  }
  unify(expr->type, ctx->instantiate(expr, val->type));

  if (val->type->isStaticType()) {
    // Evaluate static expression if possible
    expr->staticValue.type = StaticValue::Type(val->type->isStaticType());
    auto s = val->type->getStatic();
    seqassert(!expr->staticValue.evaluated, "expected unevaluated expression: {}",
              expr->toString());
    if (s && s->expr->staticValue.evaluated) {
      // Replace the identifier with static expression
      if (s->expr->staticValue.type == StaticValue::STRING)
        resultExpr = transform(N<StringExpr>(s->expr->staticValue.getString()));
      else
        resultExpr = transform(N<IntExpr>(s->expr->staticValue.getInt()));
    }
    return;
  }

  if (val->isType())
    expr->markType();

  // Realize a type or a function if possible and replace the identifier with the fully
  // typed identifier (e.g., `foo` -> `foo[int]`)
  if (realize(expr->type)) {
    if (!val->isVar())
      expr->value = expr->type->realizedName();
    expr->setDone();
  }
}

/// See @c transformDot for details
void TypecheckVisitor::visit(DotExpr *expr) {
  if ((resultExpr = transformDot(expr)))
    unify(expr->type, resultExpr->type);
  else if (!expr->type)
    unify(expr->type, ctx->addUnbound(expr, ctx->typecheckLevel));
}

/// Transforms a DotExpr expr.member to:
///   string(realized type of expr) if member is __class__.
///   unwrap(expr).member if expr is of type Optional,
///   expr._getattr("member") if expr is of type pyobj,
///   DotExpr(expr, member) if a member is a class field,
///   IdExpr(method_name) if expr.member is a class method, and
///   member(expr, ...) partial call if expr.member is an object method.
/// If args are set, this method will use it to pick the best overloaded method and
/// return its IdExpr without making the call partial (the rest will be handled by
/// CallExpr).
/// If there are multiple valid overloaded methods, pick the first one
///   (TODO: improve this).
/// Return nullptr if no transformation was made.
ExprPtr TypecheckVisitor::transformDot(DotExpr *expr,
                                       std::vector<CallExpr::Arg> *args) {
  // Special case: obj.__class__
  if (expr->member == "__class__") {
    /// TODO: prevent cls.__class__ and type(cls)
    return transformType(NT<CallExpr>(NT<IdExpr>("type"), expr->expr));
  }

  expr->expr = transform(expr->expr, true);

  // Special case: fn.__name__
  // Should go before cls.__name__ to allow printing generic functions
  if (expr->expr->type->getFunc() && expr->member == "__name__") {
    return transform(N<StringExpr>(expr->expr->type->toString()));
  }
  // Special case: cls.__name__
  if (expr->expr->isType() && expr->member == "__name__") {
    if (realize(expr->expr->type))
      return transform(N<StringExpr>(expr->expr->type->toString()));
    return nullptr;
  }

  // Ensure that the type is known (otherwise wait until it becomes known)
  auto typ = expr->expr->getType()->getClass();
  if (!typ)
    return nullptr;

  auto methods = ctx->findMethod(typ->name, expr->member);

  /* Case 1: member access */

  if (methods.empty()) {
    auto findGeneric = [this](ClassType *c, const std::string &m) -> TypePtr {
      for (auto &g : c->generics) {
        if (ctx->cache->reverseIdentifierLookup[g.name] == m)
          return g.type;
      }
      return nullptr;
    };
    if (auto member = ctx->findMember(typ->name, expr->member)) {
      // Case: object member access (`obj.member`)
      unify(expr->type, ctx->instantiate(expr, member, typ.get()));
      if (expr->expr->isDone() && realize(expr->type))
        expr->setDone();
      return nullptr;
    }
    if (auto t = findGeneric(typ.get(), expr->member)) {
      // Case: object generic access (`obj.T`)
      unify(expr->type, t);
      if (!t->isStaticType()) {
        expr->markType();
      } else {
        expr->staticValue.type = StaticValue::Type(t->isStaticType());
      }
      if (realize(expr->type)) {
        if (!t->isStaticType()) {
          return transform(N<IdExpr>(t->realizedName()));
        } else if (t->getStatic()->expr->staticValue.type == StaticValue::STRING) {
          expr->type = nullptr;  // to prevent unify(T, Static[T]) error
          return transform(
              N<StringExpr>(t->getStatic()->expr->staticValue.getString()));
        } else {
          expr->type = nullptr;  // to prevent unify(T, Static[T]) error
          return transform(N<IntExpr>(t->getStatic()->expr->staticValue.getInt()));
        }
      }
      return nullptr;
    }
    if (typ->name == TYPE_OPTIONAL) {
      // Case: transform `optional.member` to `unwrap(optional).member`
      auto dot = N<DotExpr>(transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->expr)),
                            expr->member);
      if (auto d = transformDot(dot.get(), args))
        return d;
      return dot;
    }
    if (typ->is("pyobj")) {
      // Case: transform `pyobj.member` to `pyobj._getattr("member")`
      return transform(
          N<CallExpr>(N<DotExpr>(expr->expr, "_getattr"), N<StringExpr>(expr->member)));
    }
    // For debugging purposes: ctx->findMethod(typ->name, expr->member);
    error("cannot find '{}' in {}", expr->member, typ->toString());
  }

  /* Case 2: method access */

  if (args) {
    // Use `args` (if provided by transformCall) to select the best overload
    std::vector<CallExpr::Arg> argTypes;
    if (!expr->expr->isType()) {
      // Add `self` as the first argument
      ExprPtr expr = N<IdExpr>("self");
      expr->setType(typ);
      argTypes.emplace_back(CallExpr::Arg{"", expr});
    }
    for (const auto &a : *args)
      argTypes.emplace_back(a);
    if (auto bestMethod = findBestMethod(expr->expr.get(), expr->member, argTypes)) {
      ExprPtr e = N<IdExpr>(bestMethod->ast->name);
      e->setType(ctx->instantiate(expr, bestMethod, typ.get()));
      unify(expr->type, e->type);
      if (!expr->expr->isType())
        args->insert(args->begin(), {"", expr->expr}); // `self`
      return transform(e);                             // Realize if needed
    }

    // No method found: generate error message
    // Debug: findBestMethod(expr->expr.get(), expr->member, argTypes);
    error("cannot find a method '{}' in {} with arguments {}", expr->member,
          typ->toString(), printArguments(argTypes));
  }

  FuncTypePtr bestMethod = nullptr;
  // Partially deduced type thus far
  auto typeSoFar = expr->getType() ? expr->getType()->getClass() : nullptr;
  // Select the overload
  if (methods.size() > 1 && typeSoFar && typeSoFar->getFunc()) {
    // If `typeSoFar` is a function, use its arguments to pick the best call
    std::vector<TypePtr> methodArgs;
    if (!expr->expr->isType()) // Add `self`
      methodArgs.emplace_back(typ);
    for (auto &a : typeSoFar->getFunc()->getArgTypes())
      methodArgs.emplace_back(a);
    if (!(bestMethod = findBestMethod(expr->expr.get(), expr->member, methodArgs))) {
      error("cannot find a method '{}' in {} with arguments {}", expr->member,
            typ->toString(), printArguments(methodArgs));
    }
  } else if (methods.size() > 1) {
    // If overload is ambiguous, use dispatch function to select the correct overload
    // during the instantiation
    bestMethod = findDispatch(typ->name, expr->member);
  } else {
    // Only one overload available
    bestMethod = methods.front();
  }

  // Now we have a single method. Check if this is a static or an instance access and
  // transform accordingly
  if (expr->expr->isType()) {
    // Static access: `cls.method`
    ExprPtr e = N<IdExpr>(bestMethod->ast->name);
    unify(e->type, unify(expr->type, ctx->instantiate(expr, bestMethod, typ.get())));
    return transform(e); // Realize if needed
  } else {
    // Instance access: `obj.method`
    // Transform y.method to a partial call `type(obj).method(args, ...)`
    std::vector<ExprPtr> methodArgs{expr->expr};
    // If a method is marked with @property, just call it directly
    if (!bestMethod->ast->attributes.has(Attr::Property))
      methodArgs.push_back(N<EllipsisExpr>());
    return transform(N<CallExpr>(N<IdExpr>(bestMethod->ast->name), methodArgs));
  }

  return nullptr;
}

} // namespace codon::ast