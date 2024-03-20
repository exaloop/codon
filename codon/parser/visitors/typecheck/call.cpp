// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;

/// Transform print statement.
/// @example
///   `print a, b` -> `print(a, b)`
///   `print a, b,` -> `print(a, b, end=' ')`
void TypecheckVisitor::visit(PrintStmt *stmt) {
  std::vector<CallExpr::Arg> args;
  args.reserve(stmt->items.size());
  for (auto &i : stmt->items)
    args.emplace_back("", transform(i));
  if (stmt->isInline)
    args.emplace_back("end", N<StringExpr>(" "));
  resultStmt = transform(N<ExprStmt>(N<CallExpr>(N<IdExpr>("print"), args)));
}

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
  // Special case!
  if (expr->type->getUnbound()) {
    auto callExpr = transform(clean_clone(expr->expr));
    if (callExpr->isId("std.collections.namedtuple.0")) {
      resultExpr = transformNamedTuple(expr);
      return;
    } else if (callExpr->isId("std.functools.partial.0:0")) {
      resultExpr = transformFunctoolsPartial(expr);
      return;
    } else if (callExpr->isId("tuple") && expr->args.size() == 1 &&
               CAST(expr->args.front().value, GeneratorExpr)) {
      resultExpr = transformTupleGenerator(expr);
      return;
    } else if (callExpr->isId("std.internal.static.static_print.0")) {
      transformCallArgs(expr->args);
      for (auto &a : expr->args) {
        LOG("[print] {}: {}", a.value->toString(0), a.value->type->debugString(2));
      }
      expr->setDone();
      return;
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
      transform(expr->expr);
      id = expr->expr->getId();
      // Pick the best function overload
      if (endswith(id->value, ":dispatch")) {
        if (auto bestMethod = getBestOverload(id, &expr->args)) {
          auto t = id->type;
          expr->expr = N<IdExpr>(bestMethod->ast->name);
          expr->expr->setType(ctx->instantiate(bestMethod));
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
    // Case: partial call. `calleeFn(args...)` -> `Partial(args..., fn, mask)`
    std::vector<ExprPtr> newArgs;
    for (auto &r : expr->args)
      if (!r.value->getEllipsis()) {
        newArgs.push_back(r.value);
        newArgs.back()->setAttr(ExprAttr::SequenceItem);
      }
    newArgs.push_back(part.args);
    auto partialCall = generatePartialCall(part.known, calleeFn->getFunc().get(),
                                           N<TupleExpr>(newArgs), part.kwArgs);

    std::string var = ctx->cache->getTemporaryVar("part");
    ExprPtr call = nullptr;
    if (!part.var.empty()) {
      // Callee is already a partial call
      auto stmts = expr->expr->getStmtExpr()->stmts;
      stmts.push_back(N<AssignStmt>(N<IdExpr>(var), partialCall));
      call = N<StmtExpr>(stmts, N<IdExpr>(var));
    } else {
      // New partial call: `(part = Partial(stored_args...); part)`
      call = N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), partialCall), N<IdExpr>(var));
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
      if (!typ->isRecord())
        E(Error::CALL_BAD_UNPACK, args[ai], typ->prettyString());
      const auto &fields = ctx->cache->classes[typ->name].fields;
      for (size_t i = 0; i < fields.size(); i++, ai++) {
        args.insert(args.begin() + ai,
                    {"", transform(N<DotExpr>(clone(star->what), fields[i].name))});
      }
      args.erase(args.begin() + ai);
    } else if (auto kwstar = args[ai].value->getKwStar()) {
      // Case: **kwargs expansion
      kwstar->what = transform(kwstar->what);
      auto typ = kwstar->what->type->getClass();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        kwstar->what = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), kwstar->what));
        typ = kwstar->what->type->getClass();
      }
      if (!typ)
        return false;
      if (typ->is("NamedTuple")) {
        auto id = typ->generics[0].type->getIntStatic();
        seqassert(id->value >= 0 && id->value < ctx->cache->generatedTupleNames.size(),
                  "bad id: {}", id->value);
        auto names = ctx->cache->generatedTupleNames[id->value];
        for (size_t i = 0; i < names.size(); i++, ai++) {
          args.insert(args.begin() + ai,
                      {names[i], transform(N<DotExpr>(N<DotExpr>(kwstar->what, "args"),
                                                      format("item{}", i + 1)))});
        }
        args.erase(args.begin() + ai);
      } else if (typ->isRecord()) {
        const auto &fields = ctx->cache->classes[typ->name].fields;
        for (size_t i = 0; i < fields.size(); i++, ai++) {
          args.insert(
              args.begin() + ai,
              {fields[i].name, transform(N<DotExpr>(kwstar->what, fields[i].name))});
        }
        args.erase(args.begin() + ai);
      } else {
        E(Error::CALL_BAD_KWUNPACK, args[ai], typ->prettyString());
      }
    } else {
      if (auto el = args[ai].value->getEllipsis()) {
        if (ai + 1 == args.size() && args[ai].name.empty() &&
            el->mode != EllipsisExpr::PIPE)
          el->mode = EllipsisExpr::PARTIAL;
      }
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
    return {nullptr, nullptr};
  }

  if (expr->expr->type->is("type")) {
    auto typ = expr->expr->type->getClass();
    if (!expr->expr->isId("type"))
      typ = typ->generics[0].type->getClass();
    if (!typ)
      return {nullptr, nullptr};
    auto clsName = typ->name;
    if (typ->isRecord()) {
      // Case: tuple constructor. Transform to: `T.__new__(args)`
      return {nullptr,
              transform(N<CallExpr>(N<DotExpr>(expr->expr, "__new__"), expr->args))};
    }

    // Case: reference type constructor. Transform to
    // `ctr = T.__new__(); v.__init__(args)`
    ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("ctr"));
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
    auto mask = partType->getPartialMask();
    auto func = partType->getPartialFunc();

    // Case: calling partial object `p`. Transform roughly to
    // `part = callee; partial_fn(*part.args, args...)`
    ExprPtr var = N<IdExpr>(part.var = ctx->cache->getTemporaryVar("partcall"));
    expr->expr = transform(
        N<StmtExpr>(N<AssignStmt>(clone(var), expr->expr), N<IdExpr>(func->ast->name)));

    // Ensure that we got a function
    calleeFn = expr->expr->type->getFunc();
    seqassert(calleeFn, "not a function: {}", expr->expr->type);

    // Unify partial generics with types known thus far
    auto knownArgTypes = partType->generics[2].type->getClass();
    for (size_t i = 0, j = 0, k = 0; i < mask.size(); i++)
      if (func->ast->args[i].status == Param::Generic) {
        if (mask[i])
          unify(calleeFn->funcGenerics[j].type,
                ctx->instantiate(func->funcGenerics[j].type));
        j++;
      } else if (mask[i]) {
        unify(calleeFn->getArgTypes()[i - j], knownArgTypes->generics[k].type);
        k++;
      }
    part.known = mask;
    return {calleeFn, nullptr};
  } else if (!calleeFn) {
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
    auto id = transform(N<DotExpr>(N<IdExpr>(part.var), "args"));
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
          extra.push_back(N<StarExpr>(getPartialArg(-1)));
        for (auto &e : slots[si]) {
          extra.push_back(expr->args[e].value);
        }
        ExprPtr e = N<TupleExpr>(extra);
        e->setAttr(ExprAttr::StarArgument);
        if (!expr->expr->isId("hasattr"))
          e = transform(e);
        if (partial) {
          part.args = e;
          args.emplace_back(realName,
                            transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
          newMask[si] = 0;
        } else {
          args.emplace_back(realName, e);
        }
      } else if (si == kwstarArgIndex &&
                 !(slots[si].size() == 1 &&
                   expr->args[slots[si][0]].value->hasAttr(ExprAttr::KwStarArgument))) {
        // Case: **kwargs. Build the named tuple that holds them all
        std::vector<std::string> names;
        std::vector<ExprPtr> values;
        if (!part.known.empty()) {
          auto e = transform(N<DotExpr>(N<IdExpr>(part.var), "kwargs"));
          for (auto &[n, ne] : extractNamedTuple(e)) {
            names.emplace_back(n);
            values.emplace_back(transform(ne));
          }
        }
        for (auto &e : slots[si]) {
          names.emplace_back(expr->args[e].name);
          values.emplace_back(expr->args[e].value);
        }

        auto kwid = generateKwId(names);
        auto e = transform(N<CallExpr>(N<IdExpr>("NamedTuple"), N<TupleExpr>(values),
                                       N<IntExpr>(kwid)));
        e->setAttr(ExprAttr::KwStarArgument);
        if (partial) {
          part.kwArgs = e;
          args.emplace_back(realName,
                            transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
          newMask[si] = 0;
        } else {
          args.emplace_back(realName, e);
        }
      } else if (slots[si].empty()) {
        // Case: no argument. Check if the arguments is provided by the partial type (if
        // calling it) or if a default argument can be used
        if (!part.known.empty() && part.known[si]) {
          args.emplace_back(realName, getPartialArg(pi++));
        } else if (partial) {
          args.emplace_back(realName,
                            transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
          newMask[si] = 0;
        } else {
          auto es = calleeFn->ast->args[si].defaultValue->toString();
          if (in(ctx->defaultCallDepth, es))
            E(Error::CALL_RECURSIVE_DEFAULT, expr,
              ctx->cache->rev(calleeFn->ast->args[si].name));
          ctx->defaultCallDepth.insert(es);
          args.emplace_back(
              realName, transform(clean_clone(calleeFn->ast->args[si].defaultValue)));
          ctx->defaultCallDepth.erase(es);
        }
      } else {
        // Case: argument provided
        seqassert(slots[si].size() == 1, "call transformation failed");
        args.emplace_back(realName, expr->args[slots[si][0]].value);
      }
    }
    ctx->popBlock();
    return 0;
  };

  // Reorder arguments if needed
  part.args = part.kwArgs = nullptr; // Stores partial *args/**kwargs expression
  if (expr->hasAttr(ExprAttr::OrderedCall)) {
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
    if (!part.kwArgs)
      part.kwArgs = transform(N<CallExpr>(N<IdExpr>("NamedTuple"))); // use NamedTuple()
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
        auto typ = ctx->getType(typeArgs[si]->type);
        if (calleeFn->funcGenerics[si].isStatic)
          if (!typ->isStaticType())
            E(Error::EXPECTED_STATIC, typeArgs[si]);
        unify(typ, calleeFn->funcGenerics[si].type);
        // LOG("-> {}: {} {} => {}", calleeFn, si,
        //     int(calleeFn->funcGenerics[si].isStatic), typ);
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

  ctx->addBlock();
  addFunctionGenerics(calleeFn.get());
  for (size_t si = 0; si < calleeFn->getArgTypes().size(); si++) {
    if (startswith(calleeFn->ast->args[si].name, "*") && calleeFn->ast->args[si].type &&
        args[si].value->getCall()) {
      // Special case: `*args: type` and `**kwargs: type`
      auto typ = ctx->getType(transform(clone(calleeFn->ast->args[si].type))->type);
      auto callExpr = args[si].value;
      if (startswith(calleeFn->ast->args[si].name, "**"))
        callExpr = args[si].value->getCall()->args[0].value;
      for (auto &ca : callExpr->getCall()->args) {
        if (wrapExpr(ca.value, typ, calleeFn)) {
          unify(ca.value->type, typ);
        } else {
          wrappingDone = false;
        }
      }
      auto name = callExpr->type->getClass()->name;
      auto tup = transform(N<CallExpr>(N<IdExpr>(name), callExpr->getCall()->args));

      if (startswith(calleeFn->ast->args[si].name, "**")) {
        args[si].value =
            transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("NamedTuple"), "__new__"), tup,
                                  N<IntExpr>(args[si]
                                                 .value->type->getClass()
                                                 ->generics[0]
                                                 .type->getIntStatic()
                                                 ->value)));
      } else {
        args[si].value = tup;
      }
      replacements.push_back(args[si].value->type);
    } else {
      if (calleeFn->ast->args[si].type && !calleeFn->getArgTypes()[si]->canRealize()) {
        auto t =
            ctx->getType(transform(clean_clone(calleeFn->ast->args[si].type))->type);
        unify(calleeFn->getArgTypes()[si], t);
      }
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
  ctx->popBlock();

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
        unify(calleeFn->funcGenerics[j].type, def->getType());
      }
      j++;
    }

  // Replace the arguments
  for (size_t si = 0; si < replacements.size(); si++) {
    if (replacements[si]) {
      // calleeFn->getArgTypes()[si] = replacements[si];
      calleeFn->generics[0].type->getClass()->generics[si].type = replacements[si];
      calleeFn->generics[0].type->getClass()->_rn = "";
      calleeFn->getClass()->_rn = ""; // TODO TERRIBLE!
    }
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
  // LOG(".. {}", val);
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
  } else if (startswith(val, "hasattr:")) {
    return {true, transformHasAttr(expr)};
  } else if (val == "getattr") {
    return {true, transformGetAttr(expr)};
  } else if (val == "setattr") {
    return {true, transformSetAttr(expr)};
  } else if (val == "type.__new__") {
    return {true, transformTypeFn(expr)};
  } else if (val == "compile_error") {
    return {true, transformCompileError(expr)};
  } else if (val == "tuple") {
    return {true, transformTupleFn(expr)};
  } else if (val == "__realized__") {
    return {true, transformRealizedFn(expr)};
  } else if (val == "std.internal.static.static_print.0") {
    return {false, transformStaticPrintFn(expr)};
  } else if (val == "__has_rtti__") {
    return {true, transformHasRttiFn(expr)};
  } else {
    return transformInternalStaticFn(expr);
  }
}

/// Transform `tuple(i for i in tup)` into a GeneratorExpr that will be handled during
/// the type checking.
ExprPtr TypecheckVisitor::transformTupleGenerator(CallExpr *expr) {
  // We currently allow only a simple iterations over tuples
  if (expr->args.size() != 1)
    E(Error::CALL_TUPLE_COMPREHENSION, expr->args[0].value->origExpr);
  auto g = CAST(expr->args[0].value, GeneratorExpr);
  if (!g || g->kind != GeneratorExpr::Generator || g->loopCount() != 1)
    E(Error::CALL_TUPLE_COMPREHENSION, expr->args[0].value);
  g->kind = GeneratorExpr::TupleGenerator;
  return transform(g->shared_from_this());
}

/// Transform named tuples.
/// @example
///   `namedtuple("NT", ["a", ("b", int)])` -> ```@tuple
///                                               class NT[T1]:
///                                                 a: T1
///                                                 b: int```
ExprPtr TypecheckVisitor::transformNamedTuple(CallExpr *expr) {
  // Ensure that namedtuple call is valid
  if (expr->args.size() != 2 || !(expr->args[0].value->getString()) ||
      !(expr->args[1].value->getList()))
    E(Error::CALL_NAMEDTUPLE, getSrcInfo());

  auto name = expr->args[0].value->getString()->getValue();
  // Construct the class statement
  std::vector<Param> generics, params;
  int ti = 1;
  for (auto &i : expr->args[1].value->getList()->items) {
    if (auto s = i->getString()) {
      generics.emplace_back(format("T{}", ti), N<IdExpr>("type"), nullptr, true);
      params.emplace_back(s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr);
    } else if (i->getTuple() && i->getTuple()->items.size() == 2 &&
               i->getTuple()->items[0]->getString()) {
      params.emplace_back(i->getTuple()->items[0]->getString()->getValue(),
                          transformType(i->getTuple()->items[1]), nullptr);
    } else {
      E(Error::CALL_NAMEDTUPLE, i);
    }
  }
  for (auto &g : generics)
    params.push_back(g);
  prependStmts->push_back(transform(
      N<ClassStmt>(name, params, nullptr, std::vector<ExprPtr>{N<IdExpr>("tuple")})));
  return transformType(N<IdExpr>(name));
}

/// Transform partial calls (Python syntax).
/// @example
///   `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
ExprPtr TypecheckVisitor::transformFunctoolsPartial(CallExpr *expr) {
  if (expr->args.empty())
    E(Error::CALL_PARTIAL, getSrcInfo());
  std::vector<CallExpr::Arg> args(expr->args.begin() + 1, expr->args.end());
  args.emplace_back("", N<EllipsisExpr>(EllipsisExpr::PARTIAL));
  return transform(N<CallExpr>(expr->args[0].value, args));
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
  auto func = ctx->getBase()->type->getFunc();

  // Find list of matching superf methods
  std::vector<types::FuncTypePtr> supers;
  if (!func->ast->attributes.parentClass.empty() &&
      !endswith(func->ast->name, ":dispatch")) {
    auto p = ctx->getType(func->ast->attributes.parentClass);
    if (p->getClass()) {
      if (auto c = in(ctx->cache->classes, p->getClass()->name)) {
        if (auto m = in(c->methods, ctx->cache->rev(func->ast->name))) {
          for (auto &overload : ctx->cache->overloads[*m]) {
            if (endswith(overload, ":dispatch"))
              continue;
            if (overload == func->ast->name)
              break;
            supers.emplace_back(ctx->cache->functions[overload].type);
          }
        }
      }
      std::reverse(supers.begin(), supers.end());
    }
  }
  if (supers.empty())
    E(Error::CALL_SUPERF, expr);

  seqassert(expr->args.size() == 1 && expr->args[0].value->getCall(),
            "bad superf call");
  std::vector<CallExpr::Arg> newArgs;
  for (auto &a : expr->args[0].value->getCall()->args)
    newArgs.push_back({"", a.value});
  auto m = findMatchingMethods(
      func->funcParent ? func->funcParent->getClass() : nullptr, supers, newArgs);
  if (m.empty())
    E(Error::CALL_SUPERF, expr);
  auto c = transform(N<CallExpr>(N<IdExpr>(m[0]->ast->name), newArgs));
  return c;
}

/// Typecheck and transform super method. Replace it with the current self object cast
/// to the first inherited type.
/// TODO: only an empty super() is currently supported.
ExprPtr TypecheckVisitor::transformSuper() {
  if (!ctx->getBase()->type)
    E(Error::CALL_SUPER_PARENT, getSrcInfo());
  auto funcTyp = ctx->getBase()->type->getFunc();
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

    auto superTyp = ctx->instantiate(vCands[1], typ)->getClass();
    auto self = N<IdExpr>(funcTyp->ast->args[0].name);
    self->type = typ;

    auto typExpr = N<IdExpr>(superTyp->name);
    typExpr->setType(ctx->instantiateGeneric(ctx->getType("type"), {superTyp}));
    // LOG("-> {:c} : {:c} {:c}", typ, vCands[1], typExpr->type);
    return transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"),
                                 self, typExpr, N<IntExpr>(1)));
  }

  auto name = cands.front(); // the first inherited type
  auto superTyp = ctx->instantiate(ctx->getType(name), typ)->getClass();
  if (typ->isRecord()) {
    // Case: tuple types. Return `tuple(obj.args...)`
    std::vector<ExprPtr> members;
    for (auto &field : ctx->cache->classes[name].fields)
      members.push_back(N<DotExpr>(N<IdExpr>(funcTyp->ast->args[0].name), field.name));
    ExprPtr e =
        transform(N<CallExpr>(N<IdExpr>(generateTuple(members.size())), members));
    auto ft = getClassFieldTypes(superTyp);
    for (size_t i = 0; i < ft.size(); i++)
      unify(
          ft[i],
          e->type->getClass()->generics[i].type); // see super_tuple test for this line
    e->type = superTyp;
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
  if (!val || !val->isVar())
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
  transform(expr->args[0].value);
  auto typ = expr->args[0].value->type->getClass();
  if (!typ || !typ->canRealize())
    return nullptr;

  transform(expr->args[0].value); // transform again to realize it

  typ = ctx->getType(typ)->getClass();
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

  if (typExpr->isId("type[Tuple]")) {
    return transform(N<BoolExpr>(startswith(typ->name, TYPE_TUPLE)));
  } else if (typExpr->isId("type[ByVal]")) {
    return transform(N<BoolExpr>(typ->isRecord()));
  } else if (typExpr->isId("type[ByRef]")) {
    return transform(N<BoolExpr>(!typ->isRecord()));
  } else if (!typExpr->type->getUnion() && typ->getUnion()) {
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      if (typExpr->type->unify(unionTypes[ui].get(), nullptr) >= 0) {
        tag = int(ui);
        break;
      }
    }
    if (tag == -1)
      return transform(N<BoolExpr>(false));
    return transform(N<BinaryExpr>(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_tag"),
                    expr->args[0].value),
        "==", N<BoolExpr>(tag)));
  } else if (typExpr->type->is("pyobj")) {
    if (typ->is("pyobj")) {
      return transform(N<CallExpr>(N<IdExpr>("std.internal.python._isinstance.0"),
                                   expr->args[0].value, expr->args[1].value));
    } else {
      return transform(N<BoolExpr>(false));
    }
  }

  transformType(typExpr);
  auto targetType = getType(typExpr);

  // Check super types (i.e., statically inherited) as well
  for (auto &tx : getSuperTypes(typ->getClass())) {
    if (tx->unify(targetType.get(), nullptr) >= 0)
      return transform(N<BoolExpr>(true));
  }
  return transform(N<BoolExpr>(false));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
ExprPtr TypecheckVisitor::transformStaticLen(CallExpr *expr) {
  transform(expr->args[0].value);
  auto typ = expr->args[0].value->getType();

  if (auto ss = typ->getStrStatic()) {
    // Case: staticlen on static strings
    return transform(N<IntExpr>(ss->value.size()));
  }
  if (!typ->getClass())
    return nullptr;
  if (typ->getUnion()) {
    if (realize(typ))
      return transform(N<IntExpr>(typ->getUnion()->getRealizationTypes().size()));
    return nullptr;
  }
  if (!typ->getClass()->isRecord())
    E(Error::EXPECTED_TUPLE, expr->args[0].value);
  return transform(
      N<IntExpr>(ctx->cache->classes[typ->getClass()->name].fields.size()));
}

/// Transform hasattr method to a static boolean expression.
/// This method also supports additional argument types that are used to check
/// for a matching overload (not available in Python).
ExprPtr TypecheckVisitor::transformHasAttr(CallExpr *expr) {
  auto typ = ctx->getType(expr->args[0].value->getType())->getClass();
  if (!typ)
    return nullptr;

  auto member =
      expr->expr->type->getFunc()->funcGenerics[0].type->getStrStatic()->value;
  std::vector<std::pair<std::string, TypePtr>> args{{"", typ}};
  if (expr->expr->isId("hasattr:0")) {
    // Case: the first hasattr overload allows passing argument types via *args
    auto tup = expr->args[1].value->getCall();
    seqassert(tup, "not a tuple call");
    for (auto &a : tup->args) {
      if (!a.value->getType()->getClass())
        return nullptr;
      args.emplace_back("", a.value->getType());
    }
    for (auto &[n, ne] : extractNamedTuple(expr->args[2].value)) {
      transform(ne);
      args.emplace_back(n, ne->type);
    }
  }

  bool exists = !ctx->findMethod(typ->getClass()->name, member).empty() ||
                ctx->findMember(typ->getClass()->name, member);
  if (exists && args.size() > 1)
    exists &= findBestMethod(typ, member, args) != nullptr;
  return transform(N<BoolExpr>(exists));
}

/// Transform getattr method to a DotExpr.
ExprPtr TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto name = funcTyp->funcGenerics[0].type->getStrStatic()->value;

  // special handling for NamedTuple
  if (expr->args[0].value->type && expr->args[0].value->type->is("NamedTuple")) {
    auto val = expr->args[0].value->type->getClass();
    auto id = val->generics[0].type->getIntStatic()->value;
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    auto names = ctx->cache->generatedTupleNames[id];
    for (size_t i = 0; i < names.size(); i++)
      if (names[i] == name) {
        return transform(
            N<IndexExpr>(N<DotExpr>(expr->args[0].value, "args"), N<IntExpr>(i)));
      }
    E(Error::DOT_NO_ATTR, expr, val->prettyString(), name);
  }
  return transform(N<DotExpr>(expr->args[0].value, name));
}

/// Transform setattr method to a AssignMemberStmt.
ExprPtr TypecheckVisitor::transformSetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto attr = funcTyp->funcGenerics[0].type->getStrStatic()->value;
  return transform(
      N<StmtExpr>(N<AssignMemberStmt>(expr->args[0].value, attr, expr->args[1].value),
                  N<CallExpr>(N<IdExpr>("NoneType"))));
}

/// Raise a compiler error.
ExprPtr TypecheckVisitor::transformCompileError(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto msg = funcTyp->funcGenerics[0].type->getStrStatic()->value;
  E(Error::CUSTOM, expr, msg);
  return nullptr;
}

/// Convert a class to a tuple.
ExprPtr TypecheckVisitor::transformTupleFn(CallExpr *expr) {
  auto cls = ctx->getType(expr->args.front().value->type)->getClass();
  if (!cls)
    return nullptr;

  // tuple(ClassType) is a tuple type that corresponds to a class
  if (expr->args.front().value->type->is("type")) {
    if (!realize(cls))
      return expr->shared_from_this();

    std::vector<ExprPtr> items;
    auto ft = getClassFieldTypes(cls);
    auto tn = generateTuple(ctx->cache->classes[cls->name].fields.size());
    for (size_t i = 0; i < ft.size(); i++) {
      auto rt = realize(ft[i]);
      seqassert(rt, "cannot realize '{}' in {}",
                ctx->cache->classes[cls->name].fields[i].name, cls);
      items.push_back(N<IdExpr>(rt->realizedName()));
    }
    auto e = transform(N<InstantiateExpr>(N<IdExpr>(tn), items));
    return e;
  }

  std::vector<ExprPtr> args;
  args.reserve(ctx->cache->classes[cls->name].fields.size());
  std::string var = ctx->cache->getTemporaryVar("tup");
  for (auto &field : ctx->cache->classes[cls->name].fields)
    args.emplace_back(N<DotExpr>(N<IdExpr>(var), field.name));

  return transform(
      N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), expr->args.front().value),
                  N<CallExpr>(N<IdExpr>(generateTuple(args.size())), args)));
}

/// Transform type function to a type IdExpr identifier.
ExprPtr TypecheckVisitor::transformTypeFn(CallExpr *expr) {
  if (!ctx->allowTypeOf)
    E(Error::CALL_NO_TYPE, getSrcInfo());
  transform(expr->args[0].value);
  unify(expr->type, ctx->instantiateGeneric(ctx->getType("type"),
                                            {expr->args[0].value->getType()}));
  if (!realize(expr->type))
    return nullptr;

  auto e = N<IdExpr>(expr->type->realizedName());
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
  for (auto &a : expr->args[0].value->getCall()->args) {
    realize(a.value->type);
    fmt::print(stderr, "[static_print] {}: {} := {}{}\n", getSrcInfo(),
               FormatVisitor::apply(a.value),
               a.value->type ? a.value->type->debugString(2) : "-",
               a.value->type->getStatic() ? " [static]" : "");
  }
  return nullptr;
}

/// Transform __has_rtti__ to a static boolean that indicates RTTI status of a type.
ExprPtr TypecheckVisitor::transformHasRttiFn(CallExpr *expr) {
  auto funcTyp = expr->expr->type->getFunc();
  auto t = funcTyp->funcGenerics[0].type->getClass();
  if (!t)
    return nullptr;
  auto c = in(ctx->cache->classes, t->name);
  seqassert(c, "bad class {}", t->name);
  return transform(N<BoolExpr>(c->hasRTTI()));
}

// Transform internal.static calls
std::pair<bool, ExprPtr> TypecheckVisitor::transformInternalStaticFn(CallExpr *expr) {
  unify(expr->type, ctx->getUnbound());
  if (expr->expr->isId("std.internal.static.fn_can_call.0")) {
    auto typ = expr->args[0].value->getType()->getClass();
    if (!typ)
      return {true, nullptr};

    auto fn = expr->args[0].value->type->getFunc();
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());

    auto inargs = unpackTupleTypes(expr->args[1].value);
    auto kwargs = unpackTupleTypes(expr->args[2].value);
    seqassert(inargs && kwargs, "bad call to fn_can_call");

    std::vector<CallExpr::Arg> callArgs;
    for (auto &a : *inargs) {
      callArgs.emplace_back(a.first, std::make_shared<NoneExpr>()); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    for (auto &a : *kwargs) {
      callArgs.emplace_back(a.first, std::make_shared<NoneExpr>()); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    return {true, transform(N<BoolExpr>(canCall(fn, callArgs) >= 0))};
  } else if (expr->expr->isId("std.internal.static.fn_arg_has_type.0")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = expr->expr->type->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto args = fn->getArgTypes();
    return {true, transform(N<BoolExpr>(idx->value >= 0 && idx->value < args.size() &&
                                        args[idx->value]->canRealize()))};
  } else if (expr->expr->isId("std.internal.static.fn_arg_get_type.0")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = expr->expr->type->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto args = fn->getArgTypes();
    if (idx->value < 0 || idx->value >= args.size() || !args[idx->value]->canRealize())
      error("argument does not have type");
    return {true, transform(N<IdExpr>(args[idx->value]->realizedName()))};
  } else if (expr->expr->isId("std.internal.static.fn_args.0")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    std::vector<ExprPtr> v;
    v.reserve(fn->ast->args.size());
    for (auto &a : fn->ast->args) {
      auto n = a.name;
      trimStars(n);
      n = ctx->cache->rev(n);
      v.push_back(N<StringExpr>(n));
    }
    return {true, transform(N<TupleExpr>(v))};
  } else if (expr->expr->isId("std.internal.static.fn_has_default.0")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = expr->expr->type->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    auto &args = fn->ast->args;
    if (idx->value < 0 || idx->value >= args.size())
      error("argument out of bounds");
    return {true, transform(N<BoolExpr>(args[idx->value].defaultValue != nullptr))};
  } else if (expr->expr->isId("std.internal.static.fn_get_default.0")) {
    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());
    auto idx = expr->expr->type->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    auto &args = fn->ast->args;
    if (idx->value < 0 || idx->value >= args.size())
      error("argument out of bounds");
    return {true, transform(args[idx->value].defaultValue)};
  } else if (expr->expr->isId("std.internal.static.fn_wrap_call_args.0")) {
    auto typ = expr->args[0].value->getType()->getClass();
    if (!typ)
      return {true, nullptr};

    auto fn = ctx->extractFunction(expr->args[0].value->type);
    if (!fn)
      error("expected a function, got '{}'", expr->args[0].value->type->prettyString());

    std::vector<CallExpr::Arg> callArgs;
    if (auto tup = expr->args[1].value->origExpr->getTuple()) {
      for (auto &a : tup->items) {
        callArgs.emplace_back("", a);
      }
    }
    if (auto kw = expr->args[1].value->origExpr->getCall()) {
      auto kwCls = in(ctx->cache->classes, expr->getType()->getClass()->name);
      seqassert(kwCls, "cannot find {}", expr->getType()->getClass()->name);
      for (size_t i = 0; i < kw->args.size(); i++) {
        callArgs.emplace_back(kwCls->fields[i].name, kw->args[i].value);
      }
    }
    auto zzz = transform(N<CallExpr>(N<IdExpr>(fn->ast->name), callArgs));
    if (!zzz->isDone())
      return {true, nullptr};

    std::vector<ExprPtr> tupArgs;
    for (auto &a : zzz->getCall()->args)
      tupArgs.push_back(a.value);
    return {true, transform(N<TupleExpr>(tupArgs))};
  } else if (expr->expr->isId("std.internal.static.vars.0")) {
    auto funcTyp = expr->expr->type->getFunc();
    auto withIdx = funcTyp->funcGenerics[0].type->getBoolStatic()->value;

    types::ClassTypePtr typ = nullptr;
    std::vector<ExprPtr> tupleItems;
    auto e = transform(expr->args[0].value);
    if (!(typ = e->type->getClass()))
      return {true, nullptr};

    size_t idx = 0;
    for (auto &f : ctx->cache->classes[typ->name].fields) {
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
  } else if (expr->expr->isId("std.internal.static.tuple_type.0")) {
    auto funcTyp = expr->expr->type->getFunc();
    auto t = funcTyp->funcGenerics[0].type->getClass();
    if (!t || !realize(t))
      return {true, nullptr};
    auto n = funcTyp->funcGenerics[1].type->getIntStatic()->value;
    types::TypePtr typ = nullptr;
    if (n < 0 || n >= ctx->cache->classes[t->getClass()->name].fields.size())
      error("invalid index");
    typ = ctx->instantiate(ctx->cache->classes[t->getClass()->name].fields[n].type,
                           t->getClass());
    typ = realize(typ);
    return {true, transform(N<IdExpr>(typ->realizedName()))};
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
    auto parentTyp = ctx->instantiate(ctx->getType(name))->getClass();
    for (auto &field : ctx->cache->classes[cls->name].fields) {
      for (auto &parentField : ctx->cache->classes[name].fields)
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
  auto addT = [&](const ClassType::Generic &g) {
    TypeContext::Item v = nullptr;
    if (g.isStatic) {
      seqassert(g.type->isStaticType(), "{} not a static: {}", g.name, g.type);
      v = ctx->addType(ctx->cache->rev(g.name), g.name, g.type);
      v->generic = true;
    } else {
      auto c = g.type;
      if (!c->is("type"))
        c = ctx->instantiateGeneric(ctx->getType("type"), {c});
      v = ctx->addType(ctx->cache->rev(g.name), g.name, c);
    }
    // LOG("=[inst]=> {}: {} {}", t->debugString(2), g.name, v->type);
    ctx->add(g.name, v);
  };
  for (auto parent = t->funcParent; parent;) {
    if (auto f = parent->getFunc()) {
      // Add parent function generics
      for (auto &g : f->funcGenerics)
        addT(g);
      parent = f->funcParent;
    } else {
      // Add parent class generics
      seqassert(parent->getClass(), "not a class: {}", parent);
      for (auto &g : parent->getClass()->generics)
        addT(g);
      for (auto &g : parent->getClass()->hiddenGenerics)
        addT(g);
      break;
    }
  }
  // Add function generics
  for (auto &g : t->funcGenerics)
    addT(g);
}

/// Return a partial type call `Partial(args, kwargs, fn, mask)` for a given function
/// and a mask.
/// @param mask a 0-1 vector whose size matches the number of function arguments.
///             1 indicates that the argument has been provided and is cached within
///             the partial object.
ExprPtr TypecheckVisitor::generatePartialCall(const std::vector<char> &mask,
                                              types::FuncType *fn, ExprPtr args,
                                              ExprPtr kwargs) {
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

  if (!args)
    args = N<TupleExpr>(std::vector<ExprPtr>{N<TupleExpr>()});
  if (!kwargs)
    kwargs = N<CallExpr>(N<IdExpr>("NamedTuple"));

  // LOG("{}: partializing {}", getSrcInfo(), fn->ast->name);
  auto e = N<CallExpr>(N<IdExpr>("Partial"), args, kwargs, N<StringExpr>(fn->ast->name),
                       N<StringExpr>(strMask));
  return e;
}

} // namespace codon::ast
