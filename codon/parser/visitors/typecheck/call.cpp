// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;

/// Just ensure that this expression is not independent of CallExpr where it is handled.
void TypecheckVisitor::visit(StarExpr *expr) {
  E(Error::UNEXPECTED_TYPE, expr, "star");
}

/// Just ensure that this expression is not independent of CallExpr where it is handled.
void TypecheckVisitor::visit(KeywordStarExpr *expr) {
  E(Error::UNEXPECTED_TYPE, expr, "kwstar");
}

/// Typechecks an ellipsis. Ellipses are typically replaced during the typechecking; the
/// only remaining ellipses are those that belong to PipeExprs.
void TypecheckVisitor::visit(EllipsisExpr *expr) {
  unify(expr->type, ctx->getUnbound());
  if (expr->mode == EllipsisExpr::PIPE && realize(expr->type)) {
    expr->setDone();
  }

  if (expr->mode == EllipsisExpr::STANDALONE) {
    resultExpr = transform(N<CallExpr>(N<IdExpr>("ellipsis")));
    unify(expr->type, resultExpr->type);
  }
}

/// Typecheck a call expression. This is the most complex expression to typecheck.
/// @example
///   `fn(1, 2, x=3, y=4)` -> `func(a=1, x=3, args=(2,), kwargs=KwArgs(y=4), T=int)`
///   `fn(arg1, ...)`      -> `(_v = Partial.N10(arg1); _v)`
/// See @c transformCallArgs , @c getCalleeFn , @c callReorderArguments ,
///     @c typecheckCallArgs , @c transformSpecialCall and @c wrapExpr for more details.
void TypecheckVisitor::visit(CallExpr *expr) {
  if (expr->expr->isId("__internal__.undef") && expr->args.size() == 2 &&
      expr->args[0].value->getId()) {
    auto val = expr->args[0].value->getId()->value;
    val = val.substr(0, val.size() - 9);
    if (auto changed = in(ctx->cache->replacements, val)) {
      while (auto s = in(ctx->cache->replacements, val))
        val = changed->first, changed = s;
      if (!changed->second) {
        // TODO: add no-op expr
        resultExpr = transform(N<BoolExpr>(false));
        return;
      }
    }
  }

  // Transform and expand arguments. Return early if it cannot be done yet
  if (!transformCallArgs(expr->args))
    return;

  // Check if this call is partial call
  PartialCallData part{!expr->args.empty() && expr->args.back().value->getEllipsis() &&
                       expr->args.back().value->getEllipsis()->mode ==
                           EllipsisExpr::PARTIAL};
  // Transform the callee
  if (!part.isPartial) {
    // Intercept method calls (e.g. `obj.method`) for faster compilation (because it
    // avoids partial calls). This intercept passes the call arguments to
    // @c transformDot to select the best overload as well
    if (auto dot = expr->expr->getDot()) {
      // Pick the best method overload
      if (auto edt = transformDot(dot, &expr->args))
        expr->expr = edt;
    } else if (auto id = expr->expr->getId()) {
      // Pick the best function overload
      auto overloads = in(ctx->cache->overloads, id->value);
      if (overloads && overloads->size() > 1) {
        if (auto bestMethod = getBestOverload(id, &expr->args)) {
          auto t = id->type;
          expr->expr = N<IdExpr>(bestMethod->ast->name);
          expr->expr->setType(unify(t, ctx->instantiate(bestMethod)));
        }
      }
    }
  }
  transform(expr->expr);
  auto [calleeFn, newExpr] = getCalleeFn(expr, part);
  if ((resultExpr = newExpr))
    return;
  if (!calleeFn)
    return;

  // Handle named and default arguments
  if ((resultExpr = callReorderArguments(calleeFn, expr, part)))
    return;

  // Handle special calls
  if (!part.isPartial) {
    auto [isSpecial, specialExpr] = transformSpecialCall(expr);
    if (isSpecial) {
      unify(expr->type, ctx->getUnbound());
      resultExpr = specialExpr;
      return;
    }
  }

  // Typecheck arguments with the function signature
  bool done = typecheckCallArgs(calleeFn, expr->args);
  if (!part.isPartial && realize(calleeFn)) {
    // Previous unifications can qualify existing identifiers.
    // Transform again to get the full identifier
    transform(expr->expr);
  }
  done &= expr->expr->isDone();

  // Emit the final call
  if (part.isPartial) {
    // Case: partial call. `calleeFn(args...)` -> `Partial.N<known>.<fn>(args...)`
    auto partialTypeName = generatePartialStub(part.known, calleeFn->getFunc().get());
    std::vector<ExprPtr> newArgs;
    for (auto &r : expr->args)
      if (!r.value->getEllipsis()) {
        newArgs.push_back(r.value);
        newArgs.back()->setAttr(ExprAttr::SequenceItem);
      }
    newArgs.push_back(part.args);
    newArgs.push_back(part.kwArgs);

    std::string var = ctx->cache->getTemporaryVar("part");
    ExprPtr call = nullptr;
    if (!part.var.empty()) {
      // Callee is already a partial call
      auto stmts = expr->expr->getStmtExpr()->stmts;
      stmts.push_back(N<AssignStmt>(N<IdExpr>(var),
                                    N<CallExpr>(N<IdExpr>(partialTypeName), newArgs)));
      call = N<StmtExpr>(stmts, N<IdExpr>(var));
    } else {
      // New partial call: `(part = Partial.N<known>.<fn>(stored_args...); part)`
      call =
          N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var),
                                    N<CallExpr>(N<IdExpr>(partialTypeName), newArgs)),
                      N<IdExpr>(var));
    }
    call->setAttr(ExprAttr::Partial);
    resultExpr = transform(call);
  } else {
    // Case: normal function call
    unify(expr->type, calleeFn->getRetType());
    if (done)
      expr->setDone();
  }
}

/// Transform call arguments. Expand *args and **kwargs to the list of @c CallExpr::Arg
/// objects.
/// @return false if expansion could not be completed; true otherwise
bool TypecheckVisitor::transformCallArgs(std::vector<CallExpr::Arg> &args) {
  for (auto ai = 0; ai < args.size();) {
    if (auto star = args[ai].value->getStar()) {
      // Case: *args expansion
      transform(star->what);
      auto typ = star->what->type->getClass();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        star->what = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), star->what));
        typ = star->what->type->getClass();
      }
      if (!typ) // Process later
        return false;
      if (!typ->getRecord())
        E(Error::CALL_BAD_UNPACK, args[ai], typ->prettyString());
      auto fields = getClassFields(typ.get());
      for (size_t i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
        args.insert(args.begin() + ai,
                    {"", transform(N<DotExpr>(clone(star->what), fields[i].name))});
      }
      args.erase(args.begin() + ai);
    } else if (auto kwstar = CAST(args[ai].value, KeywordStarExpr)) {
      // Case: **kwargs expansion
      kwstar->what = transform(kwstar->what);
      auto typ = kwstar->what->type->getClass();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        kwstar->what = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), kwstar->what));
        typ = kwstar->what->type->getClass();
      }
      if (!typ)
        return false;
      if (!typ->getRecord() || typ->name == TYPE_TUPLE)
        E(Error::CALL_BAD_KWUNPACK, args[ai], typ->prettyString());
      auto fields = getClassFields(typ.get());
      for (size_t i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
        args.insert(args.begin() + ai,
                    {fields[i].name,
                     transform(N<DotExpr>(clone(kwstar->what), fields[i].name))});
      }
      args.erase(args.begin() + ai);
    } else {
      // Case: normal argument (no expansion)
      transform(args[ai++].value);
    }
  }

  // Check if some argument names are reused after the expansion
  std::set<std::string> seen;
  for (auto &a : args)
    if (!a.name.empty()) {
      if (in(seen, a.name))
        E(Error::CALL_REPEATED_NAME, a, a.name);
      seen.insert(a.name);
    }

  return true;
}

/// Extract the @c FuncType that represents the function to be called by the callee.
/// Also handle special callees: constructors and partial functions.
/// @return a pair with the callee's @c FuncType and the replacement expression
///         (when needed; otherwise nullptr).
std::pair<FuncTypePtr, ExprPtr> TypecheckVisitor::getCalleeFn(CallExpr *expr,
                                                              PartialCallData &part) {
  auto callee = expr->expr->type->getClass();
  if (!callee) {
    // Case: unknown callee, wait until it becomes known
    unify(expr->type, ctx->getUnbound());
    return {nullptr, nullptr};
  }

  if (expr->expr->isType() && callee->getRecord()) {
    // Case: tuple constructor. Transform to: `T.__new__(args)`
    return {nullptr,
            transform(N<CallExpr>(N<DotExpr>(expr->expr, "__new__"), expr->args))};
  }

  if (expr->expr->isType()) {
    // Case: reference type constructor. Transform to
    // `ctr = T.__new__(); v.__init__(args)`
    ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("ctr"));
    auto clsName = expr->expr->type->getClass()->name;
    auto newInit =
        N<AssignStmt>(clone(var), N<CallExpr>(N<DotExpr>(expr->expr, "__new__")));
    auto e = N<StmtExpr>(N<SuiteStmt>(newInit), clone(var));
    auto init =
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__init__"), expr->args));
    e->stmts.emplace_back(init);
    return {nullptr, transform(e)};
  }

  auto calleeFn = callee->getFunc();
  if (auto partType = callee->getPartial()) {
    // Case: calling partial object `p`. Transform roughly to
    // `part = callee; partial_fn(*part.args, args...)`
    ExprPtr var = N<IdExpr>(part.var = ctx->cache->getTemporaryVar("partcall"));
    expr->expr = transform(N<StmtExpr>(N<AssignStmt>(clone(var), expr->expr),
                                       N<IdExpr>(partType->func->ast->name)));

    // Ensure that we got a function
    calleeFn = expr->expr->type->getFunc();
    seqassert(calleeFn, "not a function: {}", expr->expr->type);

    // Unify partial generics with types known thus far
    for (size_t i = 0, j = 0, k = 0; i < partType->known.size(); i++)
      if (partType->func->ast->args[i].status == Param::Generic) {
        if (partType->known[i])
          unify(calleeFn->funcGenerics[j].type,
                ctx->instantiate(partType->func->funcGenerics[j].type));
        j++;
      } else if (partType->known[i]) {
        unify(calleeFn->getArgTypes()[i - j], partType->generics[k].type);
        k++;
      }
    part.known = partType->known;
    return {calleeFn, nullptr};
  } else if (!callee->getFunc()) {
    // Case: callee is not a function. Try __call__ method instead
    return {nullptr,
            transform(N<CallExpr>(N<DotExpr>(expr->expr, "__call__"), expr->args))};
  }
  return {calleeFn, nullptr};
}

/// Reorder the call arguments to match the signature order. Ensure that every @c
/// CallExpr::Arg has a set name. Form *args/**kwargs tuples if needed, and use partial
/// and default values where needed.
/// @example
///   `foo(1, 2, baz=3, baf=4)` -> `foo(a=1, baz=2, args=(3, ), kwargs=KwArgs(baf=4))`
ExprPtr TypecheckVisitor::callReorderArguments(FuncTypePtr calleeFn, CallExpr *expr,
                                               PartialCallData &part) {
  std::vector<CallExpr::Arg> args; // stores ordered and processed arguments
  std::vector<ExprPtr> typeArgs;   // stores type and static arguments (e.g., `T: type`)
  auto newMask = std::vector<char>(calleeFn->ast->args.size(), 1);

  // Extract pi-th partial argument from a partial object
  auto getPartialArg = [&](size_t pi) {
    auto id = transform(N<IdExpr>(part.var));
    // Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
    auto ex = transformStaticTupleIndex(id->type->getClass(), id, N<IntExpr>(pi));
    seqassert(ex.first && ex.second, "partial indexing failed: {}", id->type);
    return ex.second;
  };

  // Handle reordered arguments (see @c reorderNamedArgs for details)
  bool partial = false;
  auto reorderFn = [&](int starArgIndex, int kwstarArgIndex,
                       const std::vector<std::vector<int>> &slots, bool _partial) {
    partial = _partial;
    ctx->addBlock(); // add function generics to typecheck default arguments
    addFunctionGenerics(calleeFn->getFunc().get());
    for (size_t si = 0, pi = 0; si < slots.size(); si++) {
      // Get the argument name to be used later
      auto rn = calleeFn->ast->args[si].name;
      trimStars(rn);
      auto realName = ctx->cache->rev(rn);

      if (calleeFn->ast->args[si].status == Param::Generic) {
        // Case: generic arguments. Populate typeArgs
        typeArgs.push_back(slots[si].empty() ? nullptr
                                             : expr->args[slots[si][0]].value);
        newMask[si] = slots[si].empty() ? 0 : 1;
      } else if (si == starArgIndex &&
                 !(slots[si].size() == 1 &&
                   expr->args[slots[si][0]].value->hasAttr(ExprAttr::StarArgument))) {
        // Case: *args. Build the tuple that holds them all
        std::vector<ExprPtr> extra;
        if (!part.known.empty())
          extra.push_back(N<StarExpr>(getPartialArg(-2)));
        for (auto &e : slots[si]) {
          extra.push_back(expr->args[e].value);
        }
        ExprPtr e = N<TupleExpr>(extra);
        e->setAttr(ExprAttr::StarArgument);
        if (!expr->expr->isId("hasattr"))
          e = transform(e);
        if (partial) {
          part.args = e;
          args.push_back({realName, transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL))});
          newMask[si] = 0;
        } else {
          args.push_back({realName, e});
        }
      } else if (si == kwstarArgIndex &&
                 !(slots[si].size() == 1 &&
                   expr->args[slots[si][0]].value->hasAttr(ExprAttr::KwStarArgument))) {
        // Case: **kwargs. Build the named tuple that holds them all
        std::vector<std::string> names;
        std::vector<CallExpr::Arg> values;
        if (!part.known.empty()) {
          auto e = getPartialArg(-1);
          auto t = e->getType()->getRecord();
          seqassert(t && startswith(t->name, TYPE_KWTUPLE), "{} not a kwtuple", e);
          auto ff = getClassFields(t.get());
          for (int i = 0; i < t->getRecord()->args.size(); i++) {
            names.emplace_back(ff[i].name);
            values.emplace_back(
                CallExpr::Arg(transform(N<DotExpr>(clone(e), ff[i].name))));
          }
        }
        for (auto &e : slots[si]) {
          names.emplace_back(expr->args[e].name);
          values.emplace_back(CallExpr::Arg(expr->args[e].value));
        }
        auto kwName = generateTuple(names.size(), TYPE_KWTUPLE, names);
        auto e = transform(N<CallExpr>(N<IdExpr>(kwName), values));
        e->setAttr(ExprAttr::KwStarArgument);
        if (partial) {
          part.kwArgs = e;
          args.push_back({realName, transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL))});
          newMask[si] = 0;
        } else {
          args.push_back({realName, e});
        }
      } else if (slots[si].empty()) {
        // Case: no argument. Check if the arguments is provided by the partial type (if
        // calling it) or if a default argument can be used
        if (!part.known.empty() && part.known[si]) {
          args.push_back({realName, getPartialArg(pi++)});
        } else if (partial) {
          args.push_back({realName, transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL))});
          newMask[si] = 0;
        } else {
          auto es = calleeFn->ast->args[si].defaultValue->toString();
          if (in(ctx->defaultCallDepth, es))
            E(Error::CALL_RECURSIVE_DEFAULT, expr,
              ctx->cache->rev(calleeFn->ast->args[si].name));
          ctx->defaultCallDepth.insert(es);

          if (calleeFn->ast->args[si].defaultValue->getNone() &&
              !calleeFn->ast->args[si].type) {
            args.push_back(
                {realName, transform(N<CallExpr>(N<InstantiateExpr>(
                               N<IdExpr>("Optional"), N<IdExpr>("NoneType"))))});
          } else {
            args.push_back(
                {realName, transform(clone(calleeFn->ast->args[si].defaultValue))});
          }
          ctx->defaultCallDepth.erase(es);
        }
      } else {
        // Case: argument provided
        seqassert(slots[si].size() == 1, "call transformation failed");
        args.push_back({realName, expr->args[slots[si][0]].value});
      }
    }
    ctx->popBlock();
    return 0;
  };

  // Reorder arguments if needed
  part.args = part.kwArgs = nullptr; // Stores partial *args/**kwargs expression
  if (expr->hasAttr(ExprAttr::OrderedCall) || expr->expr->isId("superf")) {
    args = expr->args;
  } else {
    ctx->reorderNamedArgs(
        calleeFn.get(), expr->args, reorderFn,
        [&](error::Error e, const SrcInfo &o, const std::string &errorMsg) {
          error::raise_error(e, o, errorMsg);
          return -1;
        },
        part.known);
  }

  // Populate partial data
  if (part.args != nullptr)
    part.args->setAttr(ExprAttr::SequenceItem);
  if (part.kwArgs != nullptr)
    part.kwArgs->setAttr(ExprAttr::SequenceItem);
  if (part.isPartial) {
    expr->args.pop_back();
    if (!part.args)
      part.args = transform(N<TupleExpr>()); // use ()
    if (!part.kwArgs) {
      auto kwName = generateTuple(0, TYPE_KWTUPLE, {});
      part.kwArgs = transform(N<CallExpr>(N<IdExpr>(kwName))); // use KwTuple()
    }
  }

  // Unify function type generics with the provided generics
  seqassert((expr->hasAttr(ExprAttr::OrderedCall) && typeArgs.empty()) ||
                (!expr->hasAttr(ExprAttr::OrderedCall) &&
                 typeArgs.size() == calleeFn->funcGenerics.size()),
            "bad vector sizes");
  if (!calleeFn->funcGenerics.empty()) {
    auto niGenerics = calleeFn->ast->getNonInferrableGenerics();
    for (size_t si = 0;
         !expr->hasAttr(ExprAttr::OrderedCall) && si < calleeFn->funcGenerics.size();
         si++) {
      if (typeArgs[si]) {
        auto typ = typeArgs[si]->type;
        if (calleeFn->funcGenerics[si].type->isStaticType()) {
          if (!typeArgs[si]->isStatic()) {
            E(Error::EXPECTED_STATIC, typeArgs[si]);
          }
          typ = Type::makeStatic(ctx->cache, typeArgs[si]);
        }
        unify(typ, calleeFn->funcGenerics[si].type);
      } else {
        if (calleeFn->funcGenerics[si].type->getUnbound() &&
            !calleeFn->ast->args[si].defaultValue && !partial &&
            in(niGenerics, calleeFn->funcGenerics[si].name)) {
          error("generic '{}' not provided", calleeFn->funcGenerics[si].niceName);
        }
      }
    }
  }

  // Special case: function instantiation (e.g., `foo(T=int)`)
  auto cnt = 0;
  for (auto &t : typeArgs)
    if (t)
      cnt++;
  if (part.isPartial && cnt && cnt == expr->args.size()) {
    transform(expr->expr); // transform again because it might have been changed
    unify(expr->type, expr->expr->getType());
    // Return the callee with the corrected type and do not go further
    return expr->expr;
  }

  expr->args = args;
  expr->setAttr(ExprAttr::OrderedCall);
  part.known = newMask;
  return nullptr;
}

/// Unify the call arguments' types with the function declaration signatures.
/// Also apply argument transformations to ensure the type compatibility and handle
/// default generics.
/// @example
///   `foo(1, 2)` -> `foo(1, Optional(2), T=int)`
bool TypecheckVisitor::typecheckCallArgs(const FuncTypePtr &calleeFn,
                                         std::vector<CallExpr::Arg> &args) {
  bool wrappingDone = true;          // tracks whether all arguments are wrapped
  std::vector<TypePtr> replacements; // list of replacement arguments
  for (size_t si = 0; si < calleeFn->getArgTypes().size(); si++) {
    if (startswith(calleeFn->ast->args[si].name, "*") && calleeFn->ast->args[si].type &&
        args[si].value->getCall()) {
      // Special case: `*args: type` and `**kwargs: type`
      auto typ = transform(clone(calleeFn->ast->args[si].type))->type;
      for (auto &ca : args[si].value->getCall()->args) {
        if (wrapExpr(ca.value, typ, calleeFn)) {
          unify(ca.value->type, typ);
        } else {
          wrappingDone = false;
        }
      }
      auto name = args[si].value->type->getClass()->name;
      args[si].value =
          transform(N<CallExpr>(N<IdExpr>(name), args[si].value->getCall()->args));
      replacements.push_back(args[si].value->type);
    } else {
      if (wrapExpr(args[si].value, calleeFn->getArgTypes()[si], calleeFn)) {
        unify(args[si].value->type, calleeFn->getArgTypes()[si]);
      } else {
        wrappingDone = false;
      }
      replacements.push_back(!calleeFn->getArgTypes()[si]->getClass()
                                 ? args[si].value->type
                                 : calleeFn->getArgTypes()[si]);
    }
  }

  // Realize arguments
  bool done = true;
  for (auto &a : args) {
    // Previous unifications can qualify existing identifiers.
    // Transform again to get the full identifier
    if (realize(a.value->type))
      transform(a.value);
    done &= a.value->isDone();
  }

  // Handle default generics
  for (size_t i = 0, j = 0; wrappingDone && i < calleeFn->ast->args.size(); i++)
    if (calleeFn->ast->args[i].status == Param::Generic) {
      if (calleeFn->ast->args[i].defaultValue &&
          calleeFn->funcGenerics[j].type->getUnbound()) {
        ctx->addBlock(); // add function generics to typecheck default arguments
        addFunctionGenerics(calleeFn->getFunc().get());
        auto def = transform(clone(calleeFn->ast->args[i].defaultValue));
        ctx->popBlock();
        unify(calleeFn->funcGenerics[j].type,
              def->isStatic() ? Type::makeStatic(ctx->cache, def) : def->getType());
      }
      j++;
    }

  // Replace the arguments
  for (size_t si = 0; si < replacements.size(); si++) {
    if (replacements[si])
      calleeFn->getArgTypes()[si] = replacements[si];
  }

  return done;
}

/// Transform and typecheck the following special call expressions:
///   `superf(fn)`
///   `super()`
///   `__ptr__(var)`
///   `__array__[int](sz)`
///   `isinstance(obj, type)`
///   `staticlen(tup)`
///   `hasattr(obj, "attr")`
///   `getattr(obj, "attr")`
///   `type(obj)`
///   `compile_err("msg")`
/// See below for more details.
std::pair<bool, ExprPtr> TypecheckVisitor::transformSpecialCall(CallExpr *expr) {
  if (!expr->expr->getId())
    return {false, nullptr};
  auto val = expr->expr->getId()->value;
  if (val == "superf") {
    return {true, transformSuperF(expr)};
  } else if (val == "super:0") {
    return {true, transformSuper()};
  } else if (val == "__ptr__") {
    return {true, transformPtr(expr)};
  } else if (val == "__array__.__new__:0") {
    return {true, transformArray(expr)};
  } else if (val == "isinstance") {
    return {true, transformIsInstance(expr)};
  } else if (val == "staticlen") {
    return {true, transformStaticLen(expr)};
  } else if (val == "hasattr") {
    return {true, transformHasAttr(expr)};
  } else if (val == "getattr") {
    return {true, transformGetAttr(expr)};
  } else if (val == "setattr") {
    return {true, transformSetAttr(expr)};
  } else if (val == "type.__new__:0") {
    return {true, transformTypeFn(expr)};
  } else if (val == "compile_error") {
    return {true, transformCompileError(expr)};
  } else if (val == "tuple") {
    return {true, transformTupleFn(expr)};
  } else if (val == "__realized__") {
    return {true, transformRealizedFn(expr)};
  } else if (val == "std.internal.static.static_print") {
    return {false, transformStaticPrintFn(expr)};
  } else if (val == "__has_rtti__") {
    return {true, transformHasRttiFn(expr)};
  } else {
    return transformInternalStaticFn(expr);
  }
}

/// Typecheck superf method. This method provides the access to the previous matching
/// overload.
/// @example
///   ```class cls:
///        def foo(): print('foo 1')
///        def foo():
///          superf()  # access the previous foo
///          print('foo 2')
///      cls.foo()```
///   prints "foo 1" followed by "foo 2"
ExprPtr TypecheckVisitor::transformSuperF(CallExpr *expr) {
  auto func = ctx->getRealizationBase()->type->getFunc();

  // Find list of matching superf methods
  std::vector<types::FuncTypePtr> supers;
  if (!func->ast->attributes.parentClass.empty() &&
      !endswith(func->ast->name, ":dispatch")) {
    auto p = ctx->find(func->ast->attributes.parentClass)->type;
    if (p && p->getClass()) {
      if (auto c = in(ctx->cache->classes, p->getClass()->name)) {
        if (auto m = in(c->methods, ctx->cache->rev(func->ast->name))) {
          for (auto &overload : ctx->cache->overloads[*m]) {
            if (endswith(overload.name, ":dispatch"))
              continue;
            if (overload.name == func->ast->name)
              break;
            supers.emplace_back(ctx->cache->functions[overload.name].type);
          }
        }
      }
      std::reverse(supers.begin(), supers.end());
    }
  }
  if (supers.empty())
    E(Error::CALL_SUPERF, expr);
  auto m = findMatchingMethods(
      func->funcParent ? func->funcParent->getClass() : nullptr, supers, expr->args);
  if (m.empty())
    E(Error::CALL_SUPERF, expr);
  return transform(N<CallExpr>(N<IdExpr>(m[0]->ast->name), expr->args));
}

/// Typecheck and transform super method. Replace it with the current self object cast
/// to the first inherited type.
/// TODO: only an empty super() is currently supported.
ExprPtr TypecheckVisitor::transformSuper() {
  if (!ctx->getRealizationBase()->type)
    E(Error::CALL_SUPER_PARENT, getSrcInfo());
  auto funcTyp = ctx->getRealizationBase()->type->getFunc();
  if (!funcTyp || !funcTyp->ast->hasAttr(Attr::Method))
    E(Error::CALL_SUPER_PARENT, getSrcInfo());
  if (funcTyp->getArgTypes().empty())
    E(Error::CALL_SUPER_PARENT, getSrcInfo());

  ClassTypePtr typ = funcTyp->getArgTypes()[0]->getClass();
  auto cands = ctx->cache->classes[typ->name].staticParentClasses;
  if (cands.empty()) {
    // Dynamic inheritance: use MRO
    // TODO: maybe super() should be split into two separate functions...
    auto vCands = ctx->cache->classes[typ->name].mro;
    if (vCands.size() < 2)
      E(Error::CALL_SUPER_PARENT, getSrcInfo());

    auto superTyp = ctx->instantiate(vCands[1]->type, typ)->getClass();
    auto self = N<IdExpr>(funcTyp->ast->args[0].name);
    self->type = typ;

    auto typExpr = N<IdExpr>(superTyp->name);
    typExpr->setType(superTyp);
    return transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"),
                                 self, typExpr, N<IntExpr>(1)));
  }

  auto name = cands.front(); // the first inherited type
  auto superTyp = ctx->instantiate(ctx->forceFind(name)->type)->getClass();
  if (typ->getRecord()) {
    // Case: tuple types. Return `tuple(obj.args...)`
    std::vector<ExprPtr> members;
    for (auto &field : getClassFields(superTyp.get()))
      members.push_back(N<DotExpr>(N<IdExpr>(funcTyp->ast->args[0].name), field.name));
    ExprPtr e = transform(N<TupleExpr>(members));
    e->type = unify(superTyp, e->type); // see super_tuple test for this line
    return e;
  } else {
    // Case: reference types. Return `__internal__.class_super(self, T)`
    auto self = N<IdExpr>(funcTyp->ast->args[0].name);
    self->type = typ;
    return castToSuperClass(self, superTyp);
  }
}

/// Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
/// the argument is a variable binding.
ExprPtr TypecheckVisitor::transformPtr(CallExpr *expr) {
  auto id = expr->args[0].value->getId();
  auto val = id ? ctx->find(id->value) : nullptr;
  if (!val || val->kind != TypecheckItem::Var)
    E(Error::CALL_PTR_VAR, expr->args[0]);

  transform(expr->args[0].value);
  unify(expr->type,
        ctx->instantiateGeneric(ctx->getType("Ptr"), {expr->args[0].value->type}));
  if (expr->args[0].value->isDone())
    expr->setDone();
  return nullptr;
}

/// Typecheck __array__ method. This method creates a stack-allocated array via alloca.
ExprPtr TypecheckVisitor::transformArray(CallExpr *expr) {
  auto arrTyp = expr->expr->type->getFunc();
  unify(expr->type,
        ctx->instantiateGeneric(ctx->getType("Array"),
                                {arrTyp->funcParent->getClass()->generics[0].type}));
  if (realize(expr->type))
    expr->setDone();
  return nullptr;
}

/// Transform isinstance method to a static boolean expression.
/// Special cases:
///   `isinstance(obj, ByVal)` is True if `type(obj)` is a tuple type
///   `isinstance(obj, ByRef)` is True if `type(obj)` is a reference type
ExprPtr TypecheckVisitor::transformIsInstance(CallExpr *expr) {
  expr->setType(unify(expr->type, ctx->getType("bool")));
  expr->staticValue.type = StaticValue::INT; // prevent branching until this is resolved
  transform(expr->args[0].value);
  auto typ = expr->args[0].value->type->getClass();
  if (!typ || !typ->canRealize())
    return nullptr;

  transform(expr->args[0].value); // transform again to realize it

  auto &typExpr = expr->args[1].value;
  if (auto c = typExpr->getCall()) {
    // Handle `isinstance(obj, (type1, type2, ...))`
    if (typExpr->origExpr && typExpr->origExpr->getTuple()) {
      ExprPtr result = transform(N<BoolExpr>(false));
      for (auto &i : typExpr->origExpr->getTuple()->items) {
        result = transform(N<BinaryExpr>(
            result, "||",
            N<CallExpr>(N<IdExpr>("isinstance"), expr->args[0].value, i)));
      }
      return result;
    }
  }

  expr->staticValue.type = StaticValue::INT;
  if (typExpr->isId(TYPE_TUPLE) || typExpr->isId("tuple")) {
    return transform(N<BoolExpr>(typ->name == TYPE_TUPLE));
  } else if (typExpr->isId("ByVal")) {
    return transform(N<BoolExpr>(typ->getRecord() != nullptr));
  } else if (typExpr->isId("ByRef")) {
    return transform(N<BoolExpr>(typ->getRecord() == nullptr));
  } else if (typExpr->isId("Union")) {
    return transform(N<BoolExpr>(typ->getUnion() != nullptr));
  } else if (!typExpr->type->getUnion() && typ->getUnion()) {
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      if (typExpr->type->unify(unionTypes[ui].get(), nullptr) >= 0) {
        tag = ui;
        break;
      }
    }
    if (tag == -1)
      return transform(N<BoolExpr>(false));
    return transform(N<BinaryExpr>(
        N<CallExpr>(N<IdExpr>("__internal__.union_get_tag:0"), expr->args[0].value),
        "==", N<IntExpr>(tag)));
  } else if (typExpr->type->is("pyobj") && !typExpr->isType()) {
    if (typ->is("pyobj")) {
      expr->staticValue.type = StaticValue::NOT_STATIC;
      return transform(N<CallExpr>(N<IdExpr>("std.internal.python._isinstance:0"),
                                   expr->args[0].value, expr->args[1].value));
    } else {
      return transform(N<BoolExpr>(false));
    }
  }

  transformType(typExpr);

  // Check super types (i.e., statically inherited) as well
  for (auto &tx : getSuperTypes(typ->getClass())) {
    types::Type::Unification us;
    auto s = tx->unify(typExpr->type.get(), &us);
    us.undo();
    if (s >= 0)
      return transform(N<BoolExpr>(true));
  }
  return transform(N<BoolExpr>(false));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
ExprPtr TypecheckVisitor::transformStaticLen(CallExpr *expr) {
  expr->staticValue.type = StaticValue::INT;
  transform(expr->args[0].value);
  auto typ = expr->args[0].value->getType();

  if (auto s = typ->getStatic()) {
    // Case: staticlen on static strings
    if (s->expr->staticValue.type != StaticValue::STRING)
      E(Error::EXPECTED_STATIC_SPECIFIED, expr->args[0].value, "string");
    if (!s->expr->staticValue.evaluated)
      return nullptr;
    return transform(N<IntExpr>(s->expr->staticValue.getString().size()));
  }
  if (!typ->getClass())
    return nullptr;
  if (typ->getUnion()) {
    if (realize(typ))
      return transform(N<IntExpr>(typ->getUnion()->getRealizationTypes().size()));
    return nullptr;
  }
  if (!typ->getRecord())
    E(Error::EXPECTED_TUPLE, expr->args[0].value);
  return transform(N<IntExpr>(typ->getRecord()->args.size()));
}

/// Transform hasattr method to a static boolean expression.
/// This method also supports additional argument types that are used to check
/// for a matching overload (not available in Python).
ExprPtr TypecheckVisitor::transformHasAttr(CallExpr *expr) {
  expr->staticValue.type = StaticValue::INT;
  auto typ = expr->args[0].value->getType()->getClass();
  if (!typ)
    return nullptr;
  auto member = expr->expr->type->getFunc()
                    ->funcGenerics[0]
                    .type->getStatic()
                    ->evaluate()
                    .getString();
  std::vector<std::pair<std::string, TypePtr>> args{{"", typ}};

  // Case: passing argument types via *args
  auto tup = expr->args[1].value->getTuple();
  seqassert(tup, "not a tuple");
  for (auto &a : tup->items) {
    transform(a);
    if (!a->getType()->getClass())
      return nullptr;
    args.emplace_back("", a->getType());
  }
  auto kwtup = expr->args[2].value->origExpr->getCall();
  seqassert(expr->args[2].value->origExpr && expr->args[2].value->origExpr->getCall(),
            "expected call: {}", expr->args[2].value->origExpr);
  auto kw = expr->args[2].value->origExpr->getCall();
  auto kwCls =
      in(ctx->cache->classes, expr->args[2].value->getType()->getClass()->name);
  seqassert(kwCls, "cannot find {}", expr->args[2].value->getType()->getClass()->name);
  for (size_t i = 0; i < kw->args.size(); i++) {
    auto &a = kw->args[i].value;
    transform(a);
    if (!a->getType()->getClass())
      return nullptr;
    args.emplace_back(kwCls->fields[i].name, a->getType());
  }

  if (typ->getUnion()) {
    ExprPtr cond = nullptr;
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      auto tu = realize(unionTypes[ui]);
      if (!tu)
        return nullptr;
      auto te = N<IdExpr>(tu->getClass()->realizedTypeName());
      auto e = N<BinaryExpr>(
          N<CallExpr>(N<IdExpr>("isinstance"), expr->args[0].value, te), "&&",
          N<CallExpr>(N<IdExpr>("hasattr"), te, N<StringExpr>(member)));
      cond = !cond ? e : N<BinaryExpr>(cond, "||", e);
    }
    if (!cond)
      return transform(N<BoolExpr>(false));
    return transform(cond);
  }

  bool exists = !ctx->findMethod(typ->getClass().get(), member).empty() ||
                ctx->findMember(typ->getClass(), member);
  if (exists && args.size() > 1)
    exists &= findBestMethod(typ, member, args) != nullptr;
  return transform(N<BoolExpr>(exists));
}

/// Transform getattr method to a DotExpr.
ExprPtr TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto staticTyp = funcTyp->funcGenerics[0].type->getStatic();
  if (!staticTyp->canRealize())
    return nullptr;
  return transform(N<DotExpr>(expr->args[0].value, staticTyp->evaluate().getString()));
}

/// Transform setattr method to a AssignMemberStmt.
ExprPtr TypecheckVisitor::transformSetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto staticTyp = funcTyp->funcGenerics[0].type->getStatic();
  if (!staticTyp->canRealize())
    return nullptr;
  return transform(N<StmtExpr>(N<AssignMemberStmt>(expr->args[0].value,
                                                   staticTyp->evaluate().getString(),
                                                   expr->args[1].value),
                               N<CallExpr>(N<IdExpr>("NoneType"))));
}

/// Raise a compiler error.
ExprPtr TypecheckVisitor::transformCompileError(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto staticTyp = funcTyp->funcGenerics[0].type->getStatic();
  if (staticTyp->canRealize())
    E(Error::CUSTOM, expr, staticTyp->evaluate().getString());
  return nullptr;
}

/// Convert a class to a tuple.
ExprPtr TypecheckVisitor::transformTupleFn(CallExpr *expr) {
  auto cls = expr->args.front().value->type->getClass();
  if (!cls)
    return nullptr;

  // tuple(ClassType) is a tuple type that corresponds to a class
  if (expr->args.front().value->isType()) {
    if (!realize(cls))
      return expr->clone();

    std::vector<ExprPtr> items;
    for (auto &ft : getClassFields(cls.get())) {
      auto t = ctx->instantiate(ft.type, cls);
      auto rt = realize(t);
      seqassert(rt, "cannot realize '{}' in {}", t, ft.name);
      items.push_back(NT<IdExpr>(t->realizedName()));
    }
    auto e = transform(NT<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), items));
    return e;
  }

  std::vector<ExprPtr> args;
  std::string var = ctx->cache->getTemporaryVar("tup");
  for (auto &field : getClassFields(cls.get()))
    args.emplace_back(N<DotExpr>(N<IdExpr>(var), field.name));

  return transform(N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), expr->args.front().value),
                               N<TupleExpr>(args)));
}

/// Transform type function to a type IdExpr identifier.
ExprPtr TypecheckVisitor::transformTypeFn(CallExpr *expr) {
  expr->markType();
  transform(expr->args[0].value);

  unify(expr->type, expr->args[0].value->getType());

  if (!realize(expr->type))
    return nullptr;

  auto e = NT<IdExpr>(expr->type->realizedName());
  e->setType(expr->type);
  e->setDone();
  return e;
}

/// Transform __realized__ function to a fully realized type identifier.
ExprPtr TypecheckVisitor::transformRealizedFn(CallExpr *expr) {
  auto call =
      transform(N<CallExpr>(expr->args[0].value, N<StarExpr>(expr->args[1].value)));
  if (!call->getCall()->expr->type->getFunc())
    E(Error::CALL_REALIZED_FN, expr->args[0].value);
  if (auto f = realize(call->getCall()->expr->type)) {
    auto e = N<IdExpr>(f->getFunc()->realizedName());
    e->setType(f);
    e->setDone();
    return e;
  }
  return nullptr;
}

/// Transform __static_print__ function to a fully realized type identifier.
ExprPtr TypecheckVisitor::transformStaticPrintFn(CallExpr *expr) {
  auto &args = expr->args[0].value->getCall()->args;
  for (size_t i = 0; i < args.size(); i++) {
    realize(args[i].value->type);
    fmt::print(stderr, "[static_print] {}: {} := {}{} (iter: {})\n", getSrcInfo(),
               FormatVisitor::apply(args[i].value),
               args[i].value->type ? args[i].value->type->debugString(1) : "-",
               args[i].value->isStatic() ? " [static]" : "",
               ctx->getRealizationBase()->iteration);
  }
  return nullptr;
}

/// Transform __has_rtti__ to a static boolean that indicates RTTI status of a type.
ExprPtr TypecheckVisitor::transformHasRttiFn(CallExpr *expr) {
  expr->staticValue.type = StaticValue::INT;
  auto funcTyp = expr->expr->type->getFunc();
  auto t = funcTyp->funcGenerics[0].type->getClass();
  if (!t)
    return nullptr;
  auto c = in(ctx->cache->classes, t->name);
  seqassert(c, "bad class {}", t->name);
  return transform(N<BoolExpr>(const_cast<Cache::Class *>(c)->rtti));
}

// Transform internal.static calls
std::pair<bool, ExprPtr> TypecheckVisitor::transformInternalStaticFn(CallExpr *expr) {
  unify(expr->type, ctx->getUnbound());
  if (expr->expr->isId("std.internal.static.fn_can_call")) {
    expr->staticValue.type = StaticValue::INT;
    auto typ = expr->args[0].value->getType()->getClass();
    if (!typ)
      return {true, nullptr};

    auto inargs = unpackTupleTypes(expr->args[1].value);
    auto kwargs = unpackTupleTypes(expr->args[2].value);
    seqassert(inargs && kwargs, "bad call to fn_can_call");

    std::vector<CallExpr::Arg> callArgs;
    for (auto &a : *inargs) {
      callArgs.push_back({a.first, std::make_shared<NoneExpr>()}); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    for (auto &a : *kwargs) {
      callArgs.push_back({a.first, std::make_shared<NoneExpr>()}); // dummy expression
      callArgs.back().value->setType(a.second);
    }

    if (auto fn = expr->args[0].value->type->getFunc()) {
      return {true, transform(N<BoolExpr>(canCall(fn, callArgs) >= 0))};
    } else if (auto pt = expr->args[0].value->type->getPartial()) {
      return {true, transform(N<BoolExpr>(canCall(pt->func, callArgs, pt) >= 0))};
    } else {
      compilationWarning("cannot use fn_can_call on non-functions", getSrcInfo().file,
                         getSrcInfo().line, getSrcInfo().col);
      return {true, transform(N<BoolExpr>(false))};
    }
  } else if (expr->expr->isId("std.internal.static.fn_arg_has_type")) {
    expr->staticValue.type = StaticValue::INT;
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = ctx->getStaticInt(expr->expr->type->getFunc()->funcGenerics[0].type);
    seqassert(idx, "expected a static integer");
    auto &args = fn->getArgTypes();
    return {true, transform(N<BoolExpr>(*idx >= 0 && *idx < args.size() &&
                                        args[*idx]->canRealize()))};
  } else if (expr->expr->isId("std.internal.static.fn_arg_get_type")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = ctx->getStaticInt(expr->expr->type->getFunc()->funcGenerics[0].type);
    seqassert(idx, "expected a static integer");
    auto &args = fn->getArgTypes();
    if (*idx < 0 || *idx >= args.size() || !args[*idx]->canRealize())
      error("argument does not have type");
    return {true, transform(NT<IdExpr>(args[*idx]->realizedName()))};
  } else if (expr->expr->isId("std.internal.static.fn_args")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    std::vector<ExprPtr> v;
    for (size_t i = 0; i < fn->ast->args.size(); i++) {
      auto n = fn->ast->args[i].name;
      trimStars(n);
      n = ctx->cache->rev(n);
      v.push_back(N<StringExpr>(n));
    }
    return {true, transform(N<TupleExpr>(v))};
  } else if (expr->expr->isId("std.internal.static.fn_has_default")) {
    expr->staticValue.type = StaticValue::INT;
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = ctx->getStaticInt(expr->expr->type->getFunc()->funcGenerics[0].type);
    seqassert(idx, "expected a static integer");
    auto &args = fn->ast->args;
    if (*idx < 0 || *idx >= args.size())
      error("argument out of bounds");
    return {true, transform(N<IntExpr>(args[*idx].defaultValue != nullptr))};
  } else if (expr->expr->isId("std.internal.static.fn_get_default")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = ctx->getStaticInt(expr->expr->type->getFunc()->funcGenerics[0].type);
    seqassert(idx, "expected a static integer");
    auto &args = fn->ast->args;
    if (*idx < 0 || *idx >= args.size())
      error("argument out of bounds");
    return {true, transform(args[*idx].defaultValue)};
  } else if (expr->expr->isId("std.internal.static.fn_wrap_call_args")) {
    auto typ = expr->args[0].value->getType()->getClass();
    if (!typ)
      return {true, nullptr};

    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());

    std::vector<CallExpr::Arg> callArgs;
    if (auto tup = expr->args[1].value->origExpr->getTuple()) {
      for (auto &a : tup->items) {
        callArgs.push_back({"", a});
      }
    }
    if (auto kw = expr->args[1].value->origExpr->getCall()) {
      auto kwCls = in(ctx->cache->classes, expr->getType()->getClass()->name);
      seqassert(kwCls, "cannot find {}", expr->getType()->getClass()->name);
      for (size_t i = 0; i < kw->args.size(); i++) {
        callArgs.push_back({kwCls->fields[i].name, kw->args[i].value});
      }
    }
    auto zzz = transform(N<CallExpr>(N<IdExpr>(fn->ast->name), callArgs));
    if (!zzz->isDone())
      return {true, nullptr};

    std::vector<ExprPtr> tupArgs;
    for (auto &a : zzz->getCall()->args)
      tupArgs.push_back(a.value);
    return {true, transform(N<TupleExpr>(tupArgs))};
  } else if (expr->expr->isId("std.internal.static.vars")) {
    auto funcTyp = expr->expr->type->getFunc();
    auto t = funcTyp->funcGenerics[0].type->getStatic();
    if (!t)
      return {true, nullptr};
    auto withIdx = t->evaluate().getInt();

    types::ClassTypePtr typ = nullptr;
    std::vector<ExprPtr> tupleItems;
    auto e = transform(expr->args[0].value);
    if (!(typ = e->type->getClass()))
      return {true, nullptr};

    size_t idx = 0;
    for (auto &f : getClassFields(typ.get())) {
      auto k = N<StringExpr>(f.name);
      auto v = N<DotExpr>(expr->args[0].value, f.name);
      if (withIdx) {
        auto i = N<IntExpr>(idx);
        tupleItems.push_back(N<TupleExpr>(std::vector<ExprPtr>{i, k, v}));
      } else {
        tupleItems.push_back(N<TupleExpr>(std::vector<ExprPtr>{k, v}));
      }
      idx++;
    }
    return {true, transform(N<TupleExpr>(tupleItems))};
  } else if (expr->expr->isId("std.internal.static.tuple_type")) {
    auto funcTyp = expr->expr->type->getFunc();
    auto t = funcTyp->funcGenerics[0].type;
    if (!t || !realize(t))
      return {true, nullptr};
    auto tn = funcTyp->funcGenerics[1].type->getStatic();
    if (!tn)
      return {true, nullptr};
    auto n = tn->evaluate().getInt();
    types::TypePtr typ = nullptr;
    if (t->getRecord()) {
      if (n < 0 || n >= t->getRecord()->args.size())
        error("invalid index");
      typ = t->getRecord()->args[n];
    } else {
      auto f = getClassFields(t->getClass().get());
      if (n < 0 || n >= f.size())
        error("invalid index");
      typ = ctx->instantiate(f[n].type, t->getClass());
    }
    typ = realize(typ);
    return {true, transform(NT<IdExpr>(typ->realizedName()))};
  } else {
    return {false, nullptr};
  }
}

/// Get the list that describes the inheritance hierarchy of a given type.
/// The first type in the list is the most recently inherited type.
std::vector<ClassTypePtr> TypecheckVisitor::getSuperTypes(const ClassTypePtr &cls) {
  std::vector<ClassTypePtr> result;
  if (!cls)
    return result;

  result.push_back(cls);
  for (auto &name : ctx->cache->classes[cls->name].staticParentClasses) {
    auto parentTyp = ctx->instantiate(ctx->forceFind(name)->type)->getClass();
    for (auto &field : getClassFields(cls.get())) {
      for (auto &parentField : getClassFields(parentTyp.get()))
        if (field.name == parentField.name) {
          unify(ctx->instantiate(field.type, cls),
                ctx->instantiate(parentField.type, parentTyp));
          break;
        }
    }
    for (auto &t : getSuperTypes(parentTyp))
      result.push_back(t);
  }
  return result;
}

/// Find all generics on which a function depends on and add them to the current
/// context.
void TypecheckVisitor::addFunctionGenerics(const FuncType *t) {
  for (auto parent = t->funcParent; parent;) {
    if (auto f = parent->getFunc()) {
      // Add parent function generics
      for (auto &g : f->funcGenerics) {
        // LOG("   -> {} := {}", g.name, g.type->debugString(true));
        ctx->add(TypecheckItem::Type, g.name, g.type);
      }
      parent = f->funcParent;
    } else {
      // Add parent class generics
      seqassert(parent->getClass(), "not a class: {}", parent);
      for (auto &g : parent->getClass()->generics) {
        // LOG("   => {} := {}", g.name, g.type->debugString(true));
        ctx->add(TypecheckItem::Type, g.name, g.type);
      }
      for (auto &g : parent->getClass()->hiddenGenerics) {
        // LOG("   :> {} := {}", g.name, g.type->debugString(true));
        ctx->add(TypecheckItem::Type, g.name, g.type);
      }
      break;
    }
  }
  // Add function generics
  for (auto &g : t->funcGenerics) {
    // LOG("   >> {} := {}", g.name, g.type->debugString(true));
    ctx->add(TypecheckItem::Type, g.name, g.type);
  }
}

/// Generate a partial type `Partial.N<mask>` for a given function.
/// @param mask a 0-1 vector whose size matches the number of function arguments.
///             1 indicates that the argument has been provided and is cached within
///             the partial object.
/// @example
///   ```@tuple
///      class Partial.N101[T0, T2]:
///        item0: T0  # the first cached argument
///        item2: T2  # the third cached argument
std::string TypecheckVisitor::generatePartialStub(const std::vector<char> &mask,
                                                  types::FuncType *fn) {
  std::string strMask(mask.size(), '1');
  int tupleSize = 0, genericSize = 0;
  for (size_t i = 0; i < mask.size(); i++) {
    if (!mask[i])
      strMask[i] = '0';
    else if (fn->ast->args[i].status == Param::Normal)
      tupleSize++;
    else
      genericSize++;
  }
  auto typeName = format(TYPE_PARTIAL "{}.{}", strMask, fn->toString());
  if (!ctx->find(typeName)) {
    ctx->cache->partials[typeName] = {fn->generalize(0)->getFunc(), mask};
    generateTuple(tupleSize + 2, typeName, {}, false);
  }
  return typeName;
}

} // namespace codon::ast
