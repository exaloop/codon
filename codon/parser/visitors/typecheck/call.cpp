// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
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
  std::vector<CallArg> args;
  args.reserve(stmt->items.size());
  for (auto &i : stmt->items)
    args.emplace_back("", transform(i));
  if (!stmt->hasNewline())
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
  if (expr->mode == EllipsisExpr::PIPE && realize(expr->getType())) {
    expr->setDone();
  } else if (expr->mode == EllipsisExpr::STANDALONE) {
    resultExpr = transform(N<CallExpr>(N<IdExpr>("ellipsis")));
    unify(expr->getType(), resultExpr->getType());
  }
}

/// Typecheck a call expression. This is the most complex expression to typecheck.
/// @example
///   `fn(1, 2, x=3, y=4)` -> `func(a=1, x=3, args=(2,), kwargs=KwArgs(y=4), T=int)`
///   `fn(arg1, ...)`      -> `(_v = Partial.N10(arg1); _v)`
/// See @c transformCallArgs , @c getCalleeFn , @c callReorderArguments ,
///     @c typecheckCallArgs , @c transformSpecialCall and @c wrapExpr for more details.
void TypecheckVisitor::visit(CallExpr *expr) {
  auto orig = expr->toString(0);

  // Check if this call is partial call
  PartialCallData part;

  expr->setAttribute("CallExpr");
  expr->expr = transform(expr->getExpr());
  expr->eraseAttribute("CallExpr");
  if (expr->getExpr()->getType()->getUnbound())
    return; // delay

  auto [calleeFn, newExpr] = getCalleeFn(expr, part);
  if ((resultExpr = newExpr))
    return;
  if (!calleeFn)
    return;

  // Transform `tuple(i for i in tup)` into a GeneratorExpr that will be handled during
  // the type checking.
  if (calleeFn->ast->getName() == "Tuple.__new__:dispatch" && expr->size() == 1 &&
      cast<GeneratorExpr>(expr->begin()->value)) {
    auto g = cast<GeneratorExpr>(expr->begin()->value);
    if (!g || g->kind != GeneratorExpr::Generator || g->loopCount() != 1)
      E(Error::CALL_TUPLE_COMPREHENSION, expr->begin()->value);
    g->kind = GeneratorExpr::TupleGenerator;
    resultExpr = transform(g);
    return;
  }

  ctx->addBlock();
  addFunctionGenerics(calleeFn.get(), true);
  auto a = transformCallArgs(expr);
  ctx->popBlock();
  if (!a)
    return;
  part.isPartial =
      !expr->empty() && cast<EllipsisExpr>(expr->back().value) &&
      cast<EllipsisExpr>(expr->back().value)->mode == EllipsisExpr::PARTIAL;

  // Early dispatch modifier
  if (endswith(calleeFn->ast->getName(), ":dispatch")) {
    std::vector<FuncTypePtr> m;
    if (auto id = cast<IdExpr>(expr->getExpr())) {
      // Case: function overloads (IdExpr)
      std::vector<types::FuncTypePtr> methods;
      auto key = id->getValue();
      if (endswith(key, ":dispatch"))
        key = key.substr(0, key.size() - 9);
      for (auto &m : ctx->cache->overloads[key])
        if (!endswith(m, ":dispatch"))
          methods.push_back(ctx->cache->functions[m].type);
      std::reverse(methods.begin(), methods.end());
      m = findMatchingMethods(calleeFn->funcParent ? calleeFn->funcParent->getClass()
                                                   : nullptr,
                              methods, expr->items);
    }
    bool doDispatch = m.size() == 0;
    if (m.size() > 1) {
      for (auto &a : *expr) {
        if (auto u = a.value->getType()->getUnbound())
          doDispatch = true;
      }
    }
    if (!doDispatch) {
      calleeFn = ctx->instantiate(m.front(), calleeFn->funcParent
                                                 ? calleeFn->funcParent->getClass()
                                                 : nullptr)
                     ->getFunc();
      auto e = N<IdExpr>(calleeFn->ast->getName());
      e->setType(calleeFn);
      if (cast<IdExpr>(expr->getExpr())) {
        expr->expr = e;
      } else {
        expr->expr = N<StmtExpr>(N<ExprStmt>(expr->getExpr()), e);
      }
      expr->getExpr()->setType(calleeFn);
    } else if (m.size() == 0) {
      std::vector<std::string> a;
      for (auto &t : *expr)
        a.emplace_back(fmt::format("{}", t.value->getType()->getStatic()
                                             ? t.value->getClassType()->name
                                             : t.value->getType()->prettyString()));
      auto argsNice = fmt::format("({})", fmt::join(a, ", "));
      E(Error::FN_NO_ATTR_ARGS, expr, ctx->cache->rev(calleeFn->ast->getName()),
        argsNice);
    }
  }

  bool isVirtual = false;
  if (auto dot = cast<DotExpr>(expr->getExpr()->getOrigExpr())) {
    if (auto baseTyp = dot->getExpr()->getClassType()) {
      auto cls = ctx->cache->getClass(baseTyp);
      isVirtual = bool(in(cls->virtuals, dot->getMember())) && cls->rtti &&
                  !expr->expr->getType()->is("type") &&
                  !endswith(calleeFn->ast->getName(), ":dispatch") &&
                  !calleeFn->ast->hasAttribute(Attr::StaticMethod) &&
                  !calleeFn->ast->hasAttribute(Attr::Property);
    }
  }

  // Handle named and default arguments
  if ((resultExpr = callReorderArguments(calleeFn, expr, part)))
    return;

  // Handle special calls
  if (!part.isPartial) {
    auto [isSpecial, specialExpr] = transformSpecialCall(expr);
    if (isSpecial) {
      resultExpr = specialExpr;
      return;
    }
  }

  // Typecheck arguments with the function signature
  bool done = typecheckCallArgs(calleeFn, expr->items);
  if (!part.isPartial && realize(calleeFn)) {
    // Previous unifications can qualify existing identifiers.
    // Transform again to get the full identifier
    expr->expr = transform(expr->expr);
  }
  done &= expr->expr->isDone();

  // Emit the final call
  if (part.isPartial) {
    // Case: partial call. `calleeFn(args...)` -> `Partial(args..., fn, mask)`
    std::vector<Expr *> newArgs;
    for (auto &r : *expr)
      if (!cast<EllipsisExpr>(r.value)) {
        newArgs.push_back(r.value);
        newArgs.back()->setAttribute(Attr::ExprSequenceItem);
      }
    newArgs.push_back(part.args);
    auto partialCall = generatePartialCall(part.known, calleeFn->getFunc().get(),
                                           N<TupleExpr>(newArgs), part.kwArgs);
    std::string var = ctx->cache->getTemporaryVar("part");
    Expr *call = nullptr;
    if (!part.var.empty()) {
      // Callee is already a partial call
      auto stmts = cast<StmtExpr>(expr->expr)->stmts;
      stmts.push_back(N<AssignStmt>(N<IdExpr>(var), partialCall));
      call = N<StmtExpr>(stmts, N<IdExpr>(var));
    } else {
      // New partial call: `(part = Partial(stored_args...); part)`
      call = N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), partialCall), N<IdExpr>(var));
    }
    call->setAttribute(Attr::ExprPartial);
    resultExpr = transform(call);

    // LOG("{}: {} --------> {}", getSrcInfo(), orig, resultExpr->toString(0));
  } else if (isVirtual) {
    if (!realize(calleeFn))
      return;
    auto vid = getRealizationID(calleeFn->funcParent->getClass().get(), calleeFn.get());

    // Function[Tuple[TArg1, TArg2, ...], TRet]
    std::vector<Expr *> ids;
    for (auto &t : calleeFn->getArgTypes())
      ids.push_back(N<IdExpr>(t->realizedName()));
    auto fnType = N<InstantiateExpr>(
        N<IdExpr>("Function"),
        std::vector<Expr *>{N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), ids),
                            N<IdExpr>(calleeFn->getRetType()->realizedName())});
    // Function[Tuple[TArg1, TArg2, ...],TRet](
    //    __internal__.class_get_rtti_vtable(expr)[T[VIRTUAL_ID]]
    // )
    auto e = N<CallExpr>(fnType,
                         N<IndexExpr>(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"),
                                                             "class_get_rtti_vtable"),
                                                  expr->front().value),
                                      N<IntExpr>(vid)));
    std::vector<CallArg> args;
    for (auto &a : *expr)
      args.emplace_back(a.value);
    resultExpr = transform(N<CallExpr>(e, args));
  } else {
    // Case: normal function call
    unify(expr->getType(), calleeFn->getRetType());
    if (done)
      expr->setDone();
  }
}

/// Transform call arguments. Expand *args and **kwargs to the list of @c CallArg
/// objects.
/// @return false if expansion could not be completed; true otherwise
bool TypecheckVisitor::transformCallArgs(CallExpr *expr) {
  for (auto ai = 0; ai < expr->size();) {
    if (auto star = cast<StarExpr>((*expr)[ai].value)) {
      // Case: *args expansion
      star->expr = transform(star->getExpr());
      auto typ = star->getExpr()->getClassType();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        star->expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), star->getExpr()));
        typ = star->getExpr()->getClassType();
      }
      if (!typ) // Process later
        return false;
      if (!typ->isRecord())
        E(Error::CALL_BAD_UNPACK, (*expr)[ai], typ->prettyString());
      auto fields = getClassFields(typ.get());
      for (size_t i = 0; i < fields.size(); i++, ai++) {
        expr->items.insert(
            expr->items.begin() + ai,
            {"", transform(N<DotExpr>(clone(star->getExpr()), fields[i].name))});
      }
      expr->items.erase(expr->items.begin() + ai);
    } else if (auto kwstar = cast<KeywordStarExpr>((*expr)[ai].value)) {
      // Case: **kwargs expansion
      kwstar->expr = transform(kwstar->getExpr());
      auto typ = kwstar->getExpr()->getClassType();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        kwstar->expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), kwstar->getExpr()));
        typ = kwstar->getExpr()->getClassType();
      }
      if (!typ)
        return false;
      if (typ->is("NamedTuple")) {
        auto id = typ->generics[0].type->getIntStatic();
        seqassert(id->value >= 0 && id->value < ctx->cache->generatedTupleNames.size(),
                  "bad id: {}", id->value);
        auto names = ctx->cache->generatedTupleNames[id->value];
        for (size_t i = 0; i < names.size(); i++, ai++) {
          expr->items.insert(
              expr->items.begin() + ai,
              CallArg{names[i],
                      transform(N<DotExpr>(N<DotExpr>(kwstar->getExpr(), "args"),
                                           format("item{}", i + 1)))});
        }
        expr->items.erase(expr->items.begin() + ai);
      } else if (typ->isRecord()) {
        auto fields = getClassFields(typ.get());
        for (size_t i = 0; i < fields.size(); i++, ai++) {
          expr->items.insert(
              expr->items.begin() + ai,
              CallArg{fields[i].name,
                      transform(N<DotExpr>(kwstar->expr, fields[i].name))});
        }
        expr->items.erase(expr->items.begin() + ai);
      } else {
        E(Error::CALL_BAD_KWUNPACK, (*expr)[ai], typ->prettyString());
      }
    } else {
      if (auto el = cast<EllipsisExpr>((*expr)[ai].value)) {
        if (ai + 1 == expr->size() && (*expr)[ai].name.empty() &&
            el->mode != EllipsisExpr::PIPE)
          el->mode = EllipsisExpr::PARTIAL;
      }

      // Case: normal argument (no expansion)
      (*expr)[ai].value = transform((*expr)[ai].value);
      ai++;
    }
  }

  // Check if some argument names are reused after the expansion
  std::set<std::string> seen;
  for (auto &a : *expr)
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
std::pair<FuncTypePtr, Expr *> TypecheckVisitor::getCalleeFn(CallExpr *expr,
                                                             PartialCallData &part) {
  auto callee = expr->getExpr()->getClassType();
  if (!callee) {
    // Case: unknown callee, wait until it becomes known
    return {nullptr, nullptr};
  }

  if (expr->getExpr()->getType()->is("type")) {
    auto typ = expr->getExpr()->getClassType();
    if (!isId(expr->getExpr(), "type"))
      typ = typ->generics[0].type->getClass();
    if (!typ)
      return {nullptr, nullptr};
    auto clsName = typ->name;
    if (typ->isRecord()) {
      // Case: tuple constructor. Transform to: `T.__new__(args)`
      return {nullptr, transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "__new__"),
                                             expr->items))};
    }

    // Case: reference type constructor. Transform to
    // `ctr = T.__new__(); v.__init__(args)`
    Expr *var = N<IdExpr>(ctx->cache->getTemporaryVar("ctr"));
    auto newInit =
        N<AssignStmt>(clone(var), N<CallExpr>(N<DotExpr>(expr->getExpr(), "__new__")));
    auto e = N<StmtExpr>(N<SuiteStmt>(newInit), clone(var));
    auto init =
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__init__"), expr->items));
    e->stmts.emplace_back(init);
    return {nullptr, transform(e)};
  }

  if (auto partType = callee->getPartial()) {
    auto mask = partType->getPartialMask();
    auto calleeFn =
        ctx->instantiate(partType->getPartialFunc()->generalize(0))->getFunc();

    if (!partType->isPartialEmpty() ||
        std::any_of(mask.begin(), mask.end(), [](char c) { return c; })) {
      // Case: calling partial object `p`. Transform roughly to
      // `part = callee; partial_fn(*part.args, args...)`
      Expr *var = N<IdExpr>(part.var = ctx->cache->getTemporaryVar("partcall"));
      expr->expr = transform(N<StmtExpr>(N<AssignStmt>(clone(var), expr->getExpr()),
                                         N<IdExpr>(calleeFn->ast->name)));
      part.known = mask;
      // LOG("{}: got partial {} / {} / {}", getSrcInfo(), partType->debugString(2),
      // calleeFn->debugString(2), mask);
    } else {
      expr->expr = transform(N<IdExpr>(calleeFn->ast->name));
    }
    seqassert(expr->getExpr()->getType()->getFunc(), "not a function: {}",
              expr->getExpr()->getType());
    unify(expr->getExpr()->getType()->getFunc(), calleeFn);

    // Unify partial generics with types known thus far
    auto knownArgTypes = partType->generics[1].type->getClass();
    for (size_t i = 0, j = 0, k = 0; i < mask.size(); i++)
      if ((*calleeFn->ast)[i].status == Param::Generic) {
        j++;
      } else if (mask[i]) {
        unify(calleeFn->getArgTypes()[i - j], knownArgTypes->generics[k].type);
        k++;
      }
    return {calleeFn, nullptr};
  } else if (!callee->getFunc()) {
    // Case: callee is not a function. Try __call__ method instead
    return {nullptr, transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "__call__"),
                                           expr->items))};
  } else {
    return {callee->getFunc(), nullptr};
  }
}

/// Reorder the call arguments to match the signature order. Ensure that every @c
/// CallArg has a set name. Form *args/**kwargs tuples if needed, and use partial
/// and default values where needed.
/// @example
///   `foo(1, 2, baz=3, baf=4)` -> `foo(a=1, baz=2, args=(3, ), kwargs=KwArgs(baf=4))`
Expr *TypecheckVisitor::callReorderArguments(FuncTypePtr calleeFn, CallExpr *expr,
                                             PartialCallData &part) {
  if (calleeFn->ast->hasAttribute("std.internal.attributes.no_arg_reorder.0:0"))
    return nullptr;

  std::vector<CallArg> args;    // stores ordered and processed arguments
  std::vector<Expr *> typeArgs; // stores type and static arguments (e.g., `T: type`)
  auto newMask = std::vector<char>(calleeFn->ast->size(), 1);

  // Extract pi-th partial argument from a partial object
  auto getPartialArg = [&](size_t pi) {
    auto id = transform(N<DotExpr>(N<IdExpr>(part.var), "args"));
    // Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
    auto ex = transformStaticTupleIndex(id->getClassType(), id, N<IntExpr>(pi));
    seqassert(ex.first && ex.second, "partial indexing failed: {}", id->getType());
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
      auto rn = (*calleeFn->ast)[si].name;
      trimStars(rn);
      auto realName = ctx->cache->rev(rn);

      if ((*calleeFn->ast)[si].status == Param::Generic) {
        // Case: generic arguments. Populate typeArgs
        typeArgs.push_back(slots[si].empty() ? nullptr : (*expr)[slots[si][0]].value);
        newMask[si] = slots[si].empty() ? 0 : 1;
      } else if (si == starArgIndex &&
                 !(slots[si].size() == 1 &&
                   (*expr)[slots[si][0]].value->hasAttribute(Attr::ExprStarArgument))) {
        // Case: *args. Build the tuple that holds them all
        std::vector<Expr *> extra;
        if (!part.known.empty())
          extra.push_back(N<StarExpr>(getPartialArg(-1)));
        for (auto &e : slots[si]) {
          extra.push_back((*expr)[e].value);
        }
        Expr *e = N<TupleExpr>(extra);
        e->setAttribute(Attr::ExprStarArgument);
        if (!(cast<IdExpr>(expr->expr) &&
              cast<IdExpr>(expr->expr)->getValue() == "hasattr"))
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
                 !(slots[si].size() == 1 && (*expr)[slots[si][0]].value->hasAttribute(
                                                Attr::ExprKwStarArgument))) {
        // Case: **kwargs. Build the named tuple that holds them all
        std::vector<std::string> names;
        std::vector<Expr *> values;
        if (!part.known.empty()) {
          auto e = transform(N<DotExpr>(N<IdExpr>(part.var), "kwargs"));
          for (auto &[n, ne] : extractNamedTuple(e)) {
            names.emplace_back(n);
            values.emplace_back(transform(ne));
          }
        }
        for (auto &e : slots[si]) {
          names.emplace_back((*expr)[e].name);
          values.emplace_back((*expr)[e].value);
        }

        auto kwid = generateKwId(names);
        auto e = transform(N<CallExpr>(N<IdExpr>("NamedTuple"), N<TupleExpr>(values),
                                       N<IntExpr>(kwid)));
        e->setAttribute(Attr::ExprKwStarArgument);
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
          if (cast<NoneExpr>((*calleeFn->ast)[si].defaultValue) &&
              !(*calleeFn->ast)[si].type) {
            args.push_back(
                {realName, transform(N<CallExpr>(N<InstantiateExpr>(
                               N<IdExpr>("Optional"), N<IdExpr>("NoneType"))))});
          } else {
            args.push_back(
                {realName, transform(clean_clone((*calleeFn->ast)[si].defaultValue))});
          }
        }
      } else {
        // Case: argument provided
        seqassert(slots[si].size() == 1, "call transformation failed");
        args.emplace_back(realName, (*expr)[slots[si][0]].value);
      }
    }
    ctx->popBlock();
    return 0;
  };

  // Reorder arguments if needed
  part.args = part.kwArgs = nullptr; // Stores partial *args/**kwargs expression
  if (expr->hasAttribute(Attr::ExprOrderedCall)) {
    args = expr->items;
  } else {
    ctx->reorderNamedArgs(
        calleeFn.get(), expr->items, reorderFn,
        [&](error::Error e, const SrcInfo &o, const std::string &errorMsg) {
          error::raise_error(e, o, errorMsg);
          return -1;
        },
        part.known);
  }

  // Populate partial data
  if (part.args != nullptr)
    part.args->setAttribute(Attr::ExprSequenceItem);
  if (part.kwArgs != nullptr)
    part.kwArgs->setAttribute(Attr::ExprSequenceItem);
  if (part.isPartial) {
    expr->items.pop_back();
    if (!part.args)
      part.args = transform(N<TupleExpr>()); // use ()
    if (!part.kwArgs)
      part.kwArgs = transform(N<CallExpr>(N<IdExpr>("NamedTuple"))); // use NamedTuple()
  }

  // Unify function type generics with the provided generics
  seqassert((expr->hasAttribute(Attr::ExprOrderedCall) && typeArgs.empty()) ||
                (!expr->hasAttribute(Attr::ExprOrderedCall) &&
                 typeArgs.size() == calleeFn->funcGenerics.size()),
            "bad vector sizes");
  if (!calleeFn->funcGenerics.empty()) {
    auto niGenerics = calleeFn->ast->getNonInferrableGenerics();
    for (size_t si = 0; !expr->hasAttribute(Attr::ExprOrderedCall) &&
                        si < calleeFn->funcGenerics.size();
         si++) {
      if (typeArgs[si]) {
        auto typ = ctx->getType(typeArgs[si]->getType());
        if (calleeFn->funcGenerics[si].isStatic)
          if (!typ->isStaticType())
            E(Error::EXPECTED_STATIC, typeArgs[si]);
        unify(typ, calleeFn->funcGenerics[si].type);
      } else {
        if (calleeFn->funcGenerics[si].type->getUnbound() &&
            !(*calleeFn->ast)[si].defaultValue && !partial &&
            in(niGenerics, calleeFn->funcGenerics[si].name)) {
          error("generic '{}' not provided", calleeFn->funcGenerics[si].niceName);
        }
      }
    }
  }

  // Special case: function instantiation (e.g., `foo(T=int)`)
  // auto cnt = 0;
  // for (auto &t : typeArgs)
  //   if (t)
  //     cnt++;
  // if (part.isPartial && cnt && cnt == expr->size()) {
  //   // transform again because it might have been changed
  //   expr->expr = transform(expr->expr);
  //   unify(expr->getType(), expr->expr->getType());
  //   // Return the callee with the corrected type and do not go further
  //   return expr->expr;
  // }

  expr->items = args;
  expr->setAttribute(Attr::ExprOrderedCall);
  part.known = newMask;
  // if (partial)
  // LOG("{}: {} / {}", getSrcInfo(), calleeFn->debugString(2), part.known);
  return nullptr;
}

/// Unify the call arguments' types with the function declaration signatures.
/// Also apply argument transformations to ensure the type compatibility and handle
/// default generics.
/// @example
///   `foo(1, 2)` -> `foo(1, Optional(2), T=int)`
bool TypecheckVisitor::typecheckCallArgs(const FuncTypePtr &calleeFn,
                                         std::vector<CallArg> &args) {
  bool wrappingDone = true;          // tracks whether all arguments are wrapped
  std::vector<TypePtr> replacements; // list of replacement arguments

  ctx->addBlock();
  addFunctionGenerics(calleeFn.get());
  for (size_t i = 0, si = 0; i < calleeFn->ast->size(); i++) {
    if ((*calleeFn->ast)[i].status == Param::Generic)
      continue;

    if (startswith((*calleeFn->ast)[i].name, "*") && (*calleeFn->ast)[i].type) {
      // Special case: `*args: type` and `**kwargs: type`
      if (auto callExpr = cast<CallExpr>(args[si].value)) {
        auto typ = ctx->getType(transform(clone((*calleeFn->ast)[i].type))->getType());
        if (startswith((*calleeFn->ast)[i].name, "**"))
          callExpr = cast<CallExpr>((*callExpr)[0].value);
        for (auto &ca : *callExpr) {
          if (wrapExpr(&ca.value, typ, calleeFn)) {
            unify(ca.value->getType(), typ);
          } else {
            wrappingDone = false;
          }
        }
        auto name = callExpr->getClassType()->name;
        auto tup = transform(N<CallExpr>(N<IdExpr>(name), callExpr->items));
        if (startswith((*calleeFn->ast)[i].name, "**")) {
          args[si].value =
              transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("NamedTuple"), "__new__"), tup,
                                    N<IntExpr>(args[si]
                                                   .value->getClassType()
                                                   ->generics[0]
                                                   .type->getIntStatic()
                                                   ->value)));
        } else {
          args[si].value = tup;
        }
      }
      replacements.push_back(args[si].value->getType());
      // else this is empty and is a partial call; leave it for later
    } else {
      if ((*calleeFn->ast)[i].type && !calleeFn->getArgTypes()[si]->canRealize()) {
        auto t = ctx->instantiate(
            ctx->getType((*calleeFn->ast)[i].type->getType())->generalize(0));
        unify(calleeFn->getArgTypes()[si], t);
      }
      if (wrapExpr(&args[si].value, calleeFn->getArgTypes()[si], calleeFn)) {
        unify(args[si].value->getType(), calleeFn->getArgTypes()[si]);
      } else {
        wrappingDone = false;
      }
      replacements.push_back(!calleeFn->getArgTypes()[si]->getClass()
                                 ? args[si].value->getType()
                                 : calleeFn->getArgTypes()[si]);
    }
    si++;
  }
  ctx->popBlock();

  // Realize arguments
  bool done = true;
  for (auto &a : args) {
    // Previous unifications can qualify existing identifiers.
    // Transform again to get the full identifier
    if (realize(a.value->getType()))
      a.value = transform(a.value);
    done &= a.value->isDone();
  }

  // Handle default generics
  for (size_t i = 0, j = 0; wrappingDone && i < calleeFn->ast->size(); i++)
    if ((*calleeFn->ast)[i].status == Param::Generic) {
      if ((*calleeFn->ast)[i].defaultValue &&
          calleeFn->funcGenerics[j].type->getUnbound()) {
        ctx->addBlock(); // add function generics to typecheck default arguments
        addFunctionGenerics(calleeFn->getFunc().get());
        auto def = transform(clone((*calleeFn->ast)[i].defaultValue));
        ctx->popBlock();
        unify(calleeFn->funcGenerics[j].type, ctx->getType(def->getType()));
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
std::pair<bool, Expr *> TypecheckVisitor::transformSpecialCall(CallExpr *expr) {
  auto ei = cast<IdExpr>(expr->expr);
  if (!ei)
    return {false, nullptr};
  auto val = ei->getValue();
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
  } else if (val == "hasattr") {
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
  } else if (val == "std.collections.namedtuple.0") {
    return {true, transformNamedTuple(expr)};
  } else if (val == "std.functools.partial.0:0") {
    return {true, transformFunctoolsPartial(expr)};
  } else {
    return transformInternalStaticFn(expr);
  }
}

/// Transform named tuples.
/// @example
///   `namedtuple("NT", ["a", ("b", int)])` -> ```@tuple
///                                               class NT[T1]:
///                                                 a: T1
///                                                 b: int```
Expr *TypecheckVisitor::transformNamedTuple(CallExpr *expr) {
  // Ensure that namedtuple call is valid
  auto name =
      expr->expr->getType()->getFunc()->funcGenerics[0].type->getStrStatic()->value;

  // Construct the class statement
  std::vector<Param> generics, params;
  int ti = 1;
  for (auto *i : *(cast<ListExpr>((*expr)[1].value))) {
    if (auto s = cast<StringExpr>(i)) {
      generics.emplace_back(format("T{}", ti), N<IdExpr>("type"), nullptr, true);
      params.emplace_back(s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr);
      continue;
    }
    auto t = cast<TupleExpr>(i);
    if (t && t->items.size() == 2 && cast<StringExpr>((*t)[0])) {
      params.emplace_back(cast<StringExpr>((*t)[0])->getValue(), transformType((*t)[1]),
                          nullptr);
      continue;
    }
    E(Error::CALL_NAMEDTUPLE, i);
  }
  for (auto &g : generics)
    params.push_back(g);
  prependStmts->push_back(transform(
      N<ClassStmt>(name, params, nullptr, std::vector<Expr *>{N<IdExpr>("tuple")})));
  return transformType(N<IdExpr>(name));
}

/// Transform partial calls (Python syntax).
/// @example
///   `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
Expr *TypecheckVisitor::transformFunctoolsPartial(CallExpr *expr) {
  if (expr->empty())
    E(Error::CALL_PARTIAL, getSrcInfo());
  std::vector<CallArg> args(expr->items.begin() + 1, expr->items.end());
  args.emplace_back("", N<EllipsisExpr>(EllipsisExpr::PARTIAL));
  return transform(N<CallExpr>(expr->begin()->value, args));
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
Expr *TypecheckVisitor::transformSuperF(CallExpr *expr) {
  auto func = ctx->getBase()->type->getFunc();

  // Find list of matching superf methods
  std::vector<types::FuncTypePtr> supers;
  if (!endswith(func->ast->name, ":dispatch")) {
    if (auto aa =
            func->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass)) {
      auto p = ctx->getType(aa->value);
      if (auto pc = p->getClass()) {
        if (auto c = ctx->cache->getClass(pc)) {
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
  }
  if (supers.empty())
    E(Error::CALL_SUPERF, expr);

  seqassert(expr->size() == 1 && cast<CallExpr>(expr->begin()->value),
            "bad superf call");
  std::vector<CallArg> newArgs;
  for (auto &a : *cast<CallExpr>(expr->begin()->value))
    newArgs.emplace_back("", a);
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
Expr *TypecheckVisitor::transformSuper() {
  if (!ctx->getBase()->type)
    E(Error::CALL_SUPER_PARENT, getSrcInfo());
  auto funcTyp = ctx->getBase()->type->getFunc();
  if (!funcTyp || !funcTyp->ast->hasAttribute(Attr::Method))
    E(Error::CALL_SUPER_PARENT, getSrcInfo());
  if (funcTyp->getArgTypes().empty())
    E(Error::CALL_SUPER_PARENT, getSrcInfo());

  ClassTypePtr typ = funcTyp->getArgTypes()[0]->getClass();
  auto cls = ctx->cache->getClass(typ);
  auto cands = cls->staticParentClasses;
  if (cands.empty()) {
    // Dynamic inheritance: use MRO
    // TODO: maybe super() should be split into two separate functions...
    auto vCands = cls->mro;
    if (vCands.size() < 2)
      E(Error::CALL_SUPER_PARENT, getSrcInfo());

    auto superTyp = ctx->instantiate(vCands[1], typ)->getClass();
    auto self = N<IdExpr>(funcTyp->ast->begin()->name);
    self->setType(typ);

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
    std::vector<Expr *> members;
    for (auto &field : getClassFields(superTyp.get()))
      members.push_back(N<DotExpr>(N<IdExpr>(funcTyp->ast->begin()->name), field.name));
    Expr *e = transform(N<TupleExpr>(members));
    auto ft = getClassFieldTypes(superTyp);
    for (size_t i = 0; i < ft.size(); i++)
      unify(ft[i],
            e->getClassType()->generics[i].type); // see super_tuple test for this line
    e->setType(superTyp);
    return e;
  } else {
    // Case: reference types. Return `__internal__.class_super(self, T)`
    auto self = N<IdExpr>(funcTyp->ast->begin()->name);
    self->setType(typ);
    return castToSuperClass(self, superTyp);
  }
}

/// Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
/// the argument is a variable binding.
Expr *TypecheckVisitor::transformPtr(CallExpr *expr) {
  auto id = cast<IdExpr>(expr->begin()->value);
  auto val = id ? ctx->find(id->getValue()) : nullptr;
  if (!val || !val->isVar())
    E(Error::CALL_PTR_VAR, expr->begin()->value);

  expr->begin()->value = transform(expr->begin()->value);
  unify(expr->getType(), ctx->instantiateGeneric(ctx->getType("Ptr"),
                                                 {expr->begin()->value->getType()}));
  if (expr->begin()->value->isDone())
    expr->setDone();
  return nullptr;
}

/// Typecheck __array__ method. This method creates a stack-allocated array via alloca.
Expr *TypecheckVisitor::transformArray(CallExpr *expr) {
  auto arrTyp = expr->expr->getType()->getFunc();
  unify(expr->getType(),
        ctx->instantiateGeneric(ctx->getType("Array"),
                                {arrTyp->funcParent->getClass()->generics[0].type}));
  if (realize(expr->getType()))
    expr->setDone();
  return nullptr;
}

/// Transform isinstance method to a static boolean expression.
/// Special cases:
///   `isinstance(obj, ByVal)` is True if `type(obj)` is a tuple type
///   `isinstance(obj, ByRef)` is True if `type(obj)` is a reference type
Expr *TypecheckVisitor::transformIsInstance(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->value);
  auto typ = expr->begin()->value->getClassType();
  if (!typ || !typ->canRealize())
    return nullptr;

  expr->begin()->value =
      transform(expr->begin()->value); // transform again to realize it

  typ = ctx->getType(typ)->getClass();
  auto &typExpr = (*expr)[1].value;
  if (auto c = cast<CallExpr>(typExpr)) {
    // Handle `isinstance(obj, (type1, type2, ...))`
    if (typExpr->getOrigExpr() && cast<TupleExpr>(typExpr->getOrigExpr())) {
      Expr *result = transform(N<BoolExpr>(false));
      for (auto *i : *cast<TupleExpr>(typExpr->getOrigExpr())) {
        result = transform(N<BinaryExpr>(
            result, "||",
            N<CallExpr>(N<IdExpr>("isinstance"), expr->begin()->value, i)));
      }
      return result;
    }
  }

  auto tei = cast<IdExpr>(typExpr);
  if (tei && tei->getValue() == "type[Tuple]") {
    return transform(N<BoolExpr>(typ->is(TYPE_TUPLE)));
  } else if (tei && tei->getValue() == "type[ByVal]") {
    return transform(N<BoolExpr>(typ->isRecord()));
  } else if (tei && tei->getValue() == "type[ByRef]") {
    return transform(N<BoolExpr>(!typ->isRecord()));
  } else if (tei && tei->getValue() == "type[Union]") {
    return transform(N<BoolExpr>(typ->getUnion() != nullptr));
  } else if (!getType(typExpr)->getUnion() && typ->getUnion()) {
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      if (getType(typExpr)->unify(unionTypes[ui].get(), nullptr) >= 0) {
        tag = int(ui);
        break;
      }
    }
    if (tag == -1)
      return transform(N<BoolExpr>(false));
    return transform(N<BinaryExpr>(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_tag"),
                    expr->begin()->value),
        "==", N<IntExpr>(tag)));
  } else if (typExpr->getType()->is("pyobj")) {
    if (typ->is("pyobj")) {
      return transform(N<CallExpr>(N<IdExpr>("std.internal.python._isinstance.0"),
                                   expr->begin()->value, (*expr)[1].value));
    } else {
      return transform(N<BoolExpr>(false));
    }
  }

  typExpr = transformType(typExpr);
  auto targetType = getType(typExpr);

  // Check super types (i.e., statically inherited) as well
  for (auto &tx : getSuperTypes(typ->getClass())) {
    types::Type::Unification us;
    auto s = tx->unify(targetType.get(), &us);
    us.undo();
    if (s >= 0)
      return transform(N<BoolExpr>(true));
  }
  return transform(N<BoolExpr>(false));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
Expr *TypecheckVisitor::transformStaticLen(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->value);
  auto typ = getType(expr->begin()->value);

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
    E(Error::EXPECTED_TUPLE, expr->begin()->value);
  return transform(N<IntExpr>(getClassFields(typ->getClass().get()).size()));
}

/// Transform hasattr method to a static boolean expression.
/// This method also supports additional argument types that are used to check
/// for a matching overload (not available in Python).
Expr *TypecheckVisitor::transformHasAttr(CallExpr *expr) {
  auto typ = ctx->getType((*expr)[0].value->getType())->getClass();
  if (!typ)
    return nullptr;

  auto member =
      expr->expr->getType()->getFunc()->funcGenerics[0].type->getStrStatic()->value;
  std::vector<std::pair<std::string, TypePtr>> args{{"", typ}};

  if (auto tup = cast<TupleExpr>((*expr)[1].value)) {
    for (auto &a : *tup) {
      a = transform(a);
      if (!a->getClassType())
        return nullptr;
      args.emplace_back("", getType(a));
    }
  }
  for (auto &[n, ne] : extractNamedTuple((*expr)[2].value)) {
    ne = transform(ne);
    args.emplace_back(n, ne->getType());
  }

  if (typ->getUnion()) {
    Expr *cond = nullptr;
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      auto tu = realize(unionTypes[ui]);
      if (!tu)
        return nullptr;
      auto te = N<IdExpr>(tu->getClass()->realizedName());
      auto e = N<BinaryExpr>(
          N<CallExpr>(N<IdExpr>("isinstance"), (*expr)[0].value, te), "&&",
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
Expr *TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->getType()->getFunc();
  auto name = funcTyp->funcGenerics[0].type->getStrStatic()->value;

  // special handling for NamedTuple
  if (expr->begin()->value->getType() &&
      expr->begin()->value->getType()->is("NamedTuple")) {
    auto val = expr->begin()->value->getClassType();
    auto id = val->generics[0].type->getIntStatic()->value;
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    auto names = ctx->cache->generatedTupleNames[id];
    for (size_t i = 0; i < names.size(); i++)
      if (names[i] == name) {
        return transform(
            N<IndexExpr>(N<DotExpr>(expr->begin()->value, "args"), N<IntExpr>(i)));
      }
    E(Error::DOT_NO_ATTR, expr, val->prettyString(), name);
  }
  return transform(N<DotExpr>(expr->begin()->value, name));
}

/// Transform setattr method to a AssignMemberStmt.
Expr *TypecheckVisitor::transformSetAttr(CallExpr *expr) {
  auto funcTyp = expr->expr->getType()->getFunc();
  auto attr = funcTyp->funcGenerics[0].type->getStrStatic()->value;
  return transform(
      N<StmtExpr>(N<AssignMemberStmt>((*expr)[0].value, attr, (*expr)[1].value),
                  N<CallExpr>(N<IdExpr>("NoneType"))));
}

/// Raise a compiler error.
Expr *TypecheckVisitor::transformCompileError(CallExpr *expr) {
  auto funcTyp = expr->expr->getType()->getFunc();
  auto msg = funcTyp->funcGenerics[0].type->getStrStatic()->value;
  E(Error::CUSTOM, expr, msg);
  return nullptr;
}

/// Convert a class to a tuple or handle a tuple generator.
Expr *TypecheckVisitor::transformTupleFn(CallExpr *expr) {
  if (expr->size() == 0)
    return transform(N<TupleExpr>());

  for (auto &e : *expr)
    e.value = transform(e.value);

  auto cls = ctx->getType(expr->begin()->value->getType())->getClass();
  if (!cls)
    return nullptr;

  // tuple(ClassType) is a tuple type that corresponds to a class
  if (expr->begin()->value->getType()->is("type")) {
    if (!realize(cls))
      return expr;

    std::vector<Expr *> items;
    auto ft = getClassFieldTypes(cls);
    for (size_t i = 0; i < ft.size(); i++) {
      auto rt = realize(ft[i]);
      seqassert(rt, "cannot realize '{}' in {}",
                ctx->cache->getClass(cls)->fields[i].name, cls);
      items.push_back(N<IdExpr>(rt->realizedName()));
    }
    auto e = transform(N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), items));
    return e;
  }

  std::vector<Expr *> args;
  std::string var = ctx->cache->getTemporaryVar("tup");
  for (auto &field : getClassFields(cls.get()))
    args.emplace_back(N<DotExpr>(N<IdExpr>(var), field.name));

  return transform(N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), expr->begin()->value),
                               N<TupleExpr>(args)));
}

/// Transform type function to a type IdExpr identifier.
Expr *TypecheckVisitor::transformTypeFn(CallExpr *expr) {
  if (!ctx->allowTypeOf)
    E(Error::CALL_NO_TYPE, getSrcInfo());
  expr->begin()->value = transform(expr->begin()->value);
  unify(expr->getType(), ctx->instantiateGeneric(ctx->getType("type"),
                                                 {expr->begin()->value->getType()}));
  if (!realize(expr->getType()))
    return nullptr;

  auto e = N<IdExpr>(expr->getType()->realizedName());
  e->setType(expr->getType());
  e->setDone();
  return e;
}

/// Transform __realized__ function to a fully realized type identifier.
Expr *TypecheckVisitor::transformRealizedFn(CallExpr *expr) {
  auto call = cast<CallExpr>(
      transform(N<CallExpr>((*expr)[0].value, N<StarExpr>((*expr)[1].value))));
  if (!call || !call->expr->getType()->getFunc())
    E(Error::CALL_REALIZED_FN, (*expr)[0].value);
  if (auto f = realize(call->expr->getType())) {
    auto e = N<IdExpr>(f->getFunc()->realizedName());
    e->setType(f);
    e->setDone();
    return e;
  }
  return nullptr;
}

/// Transform __static_print__ function to a fully realized type identifier.
Expr *TypecheckVisitor::transformStaticPrintFn(CallExpr *expr) {
  for (auto &a : *cast<CallExpr>(expr->begin()->value)) {
    realize(a.value->getType());
    fmt::print(stderr, "[static_print] {}: {} ({}){}\n", getSrcInfo(),
               //  FormatVisitor::apply(a.value),
               a.value->getType() ? a.value->getType()->debugString(2) : "-",
               a.value->getType() ? a.value->getType()->realizedName() : "-",
               a.value->getType()->getStatic() ? " [static]" : "");
  }
  return nullptr;
}

/// Transform __has_rtti__ to a static boolean that indicates RTTI status of a type.
Expr *TypecheckVisitor::transformHasRttiFn(CallExpr *expr) {
  auto funcTyp = expr->expr->getType()->getFunc();
  auto t = funcTyp->funcGenerics[0].type->getClass();
  if (!t)
    return nullptr;
  auto c = ctx->cache->getClass(t);
  seqassert(c, "bad class {}", t->name);
  // LOG(" :: {} / {}", t->debugString(2), c->hasRTTI());
  return transform(N<BoolExpr>(c->hasRTTI()));
}

// Transform internal.static calls
std::pair<bool, Expr *> TypecheckVisitor::transformInternalStaticFn(CallExpr *expr) {
  auto ei = cast<IdExpr>(expr->expr);
  if (ei && ei->getValue() == "std.internal.static.fn_can_call.0") {
    auto typ = getType((*expr)[0].value)->getClass();
    if (!typ)
      return {true, nullptr};

    auto inargs = unpackTupleTypes((*expr)[1].value);
    auto kwargs = unpackTupleTypes((*expr)[2].value);
    seqassert(inargs && kwargs, "bad call to fn_can_call");

    std::vector<CallArg> callArgs;
    for (auto &a : *inargs) {
      callArgs.emplace_back(a.first, N<NoneExpr>()); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    for (auto &a : *kwargs) {
      callArgs.emplace_back(a.first, N<NoneExpr>()); // dummy expression
      callArgs.back().value->setType(a.second);
    }
    if (auto fn = typ->getFunc()) {
      return {true, transform(N<BoolExpr>(canCall(fn, callArgs) >= 0))};
    } else if (auto pt = typ->getPartial()) {
      return {true,
              transform(N<BoolExpr>(canCall(pt->getPartialFunc(), callArgs, pt) >= 0))};
    } else {
      compilationWarning("cannot use fn_can_call on non-functions", getSrcInfo().file,
                         getSrcInfo().line, getSrcInfo().col);
      return {true, transform(N<BoolExpr>(false))};
    }
  } else if (ei && ei->getValue() == "std.internal.static.fn_arg_has_type.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());
    auto idx = expr->expr->getType()->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto args = fn->getArgTypes();
    return {true, transform(N<BoolExpr>(idx->value >= 0 && idx->value < args.size() &&
                                        args[idx->value]->canRealize()))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_arg_get_type.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());
    auto idx = expr->expr->getType()->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto args = fn->getArgTypes();
    if (idx->value < 0 || idx->value >= args.size() || !args[idx->value]->canRealize())
      error("argument does not have type");
    return {true, transform(N<IdExpr>(args[idx->value]->realizedName()))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_args.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());
    std::vector<Expr *> v;
    v.reserve(fn->ast->size());
    for (const auto &a : *fn->ast) {
      auto n = a.name;
      trimStars(n);
      n = ctx->cache->rev(n);
      v.push_back(N<StringExpr>(n));
    }
    return {true, transform(N<TupleExpr>(v))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_has_default.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());
    auto idx = expr->expr->getType()->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    if (idx->value < 0 || idx->value >= fn->ast->size())
      error("argument out of bounds");
    return {true,
            transform(N<BoolExpr>((*fn->ast)[idx->value].defaultValue != nullptr))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_get_default.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());
    auto idx = expr->expr->getType()->getFunc()->funcGenerics[0].type->getIntStatic();
    seqassert(idx, "expected a static integer");
    if (idx->value < 0 || idx->value >= fn->ast->size())
      error("argument out of bounds");
    return {true, transform((*fn->ast)[idx->value].defaultValue)};
  } else if (ei && ei->getValue() == "std.internal.static.fn_wrap_call_args.0") {
    auto typ = expr->begin()->value->getType()->getClass();
    if (!typ)
      return {true, nullptr};

    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->value->getType()->prettyString());

    std::vector<CallArg> callArgs;
    if (auto tup = cast<TupleExpr>((*expr)[1].value->getOrigExpr())) {
      for (auto *a : *tup) {
        callArgs.emplace_back("", a);
      }
    }
    if (auto kw = cast<CallExpr>((*expr)[1].value->getOrigExpr())) {
      auto kwCls = ctx->cache->getClass(expr->getType()->getClass());
      seqassert(kwCls, "cannot find {}", expr->getType()->getClass()->name);
      for (size_t i = 0; i < kw->size(); i++) {
        callArgs.emplace_back(kwCls->fields[i].name, (*kw)[i].value);
      }
    }
    auto zzz = transform(N<CallExpr>(N<IdExpr>(fn->ast->name), callArgs));
    if (!zzz->isDone())
      return {true, nullptr};

    std::vector<Expr *> tupArgs;
    for (auto &a : *cast<CallExpr>(zzz))
      tupArgs.push_back(a.value);
    return {true, transform(N<TupleExpr>(tupArgs))};
  } else if (ei && ei->getValue() == "std.internal.static.vars.0") {
    auto funcTyp = expr->expr->getType()->getFunc();
    auto withIdx = funcTyp->funcGenerics[0].type->getBoolStatic()->value;

    types::ClassTypePtr typ = nullptr;
    std::vector<Expr *> tupleItems;
    auto e = transform(expr->begin()->value);
    if (!(typ = e->getClassType()))
      return {true, nullptr};

    size_t idx = 0;
    for (auto &f : getClassFields(typ.get())) {
      auto k = N<StringExpr>(f.name);
      auto v = N<DotExpr>(expr->begin()->value, f.name);
      if (withIdx) {
        auto i = N<IntExpr>(idx);
        tupleItems.push_back(N<TupleExpr>(std::vector<Expr *>{i, k, v}));
      } else {
        tupleItems.push_back(N<TupleExpr>(std::vector<Expr *>{k, v}));
      }
      idx++;
    }
    return {true, transform(N<TupleExpr>(tupleItems))};
  } else if (ei && ei->getValue() == "std.internal.static.tuple_type.0") {
    auto funcTyp = expr->expr->getType()->getFunc();
    auto t = funcTyp->funcGenerics[0].type->getClass();
    if (!t || !realize(t))
      return {true, nullptr};
    auto n = funcTyp->funcGenerics[1].type->getIntStatic()->value;
    types::TypePtr typ = nullptr;
    auto f = getClassFields(t.get());
    if (n < 0 || n >= f.size())
      error("invalid index");
    typ = ctx->instantiate(f[n].type, t->getClass());
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
  auto c = ctx->cache->getClass(cls);
  auto fields = getClassFields(cls.get());
  for (auto &name : c->staticParentClasses) {
    auto parentTyp = ctx->instantiate(ctx->getType(name))->getClass();
    auto parentFields = getClassFields(parentTyp.get());
    for (auto &field : fields) {
      for (auto &parentField : parentFields)
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
void TypecheckVisitor::addFunctionGenerics(const FuncType *t, bool onlyMangled) {
  auto addT = [&](const ClassType::Generic &g) {
    TypeContext::Item v = nullptr;
    if (g.isStatic) {
      seqassert(g.type->isStaticType(), "{} not a static: {}", g.name, g.type);
      v = ctx->addType(onlyMangled ? g.name : ctx->cache->rev(g.name), g.name, g.type);
      v->generic = true;
    } else {
      auto c = g.type;
      if (!c->is("type"))
        c = ctx->instantiateGeneric(ctx->getType("type"), {c});
      v = ctx->addType(onlyMangled ? g.name : ctx->cache->rev(g.name), g.name, c);
    }
    if (!onlyMangled)
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
Expr *TypecheckVisitor::generatePartialCall(const std::vector<char> &mask,
                                            types::FuncType *fn, Expr *args,
                                            Expr *kwargs) {
  std::string strMask(mask.size(), '1');
  for (size_t i = 0; i < mask.size(); i++) {
    if (!mask[i])
      strMask[i] = '0';
  }

  if (!args)
    args = N<TupleExpr>(std::vector<Expr *>{N<TupleExpr>()});
  if (!kwargs)
    kwargs = N<CallExpr>(N<IdExpr>("NamedTuple"));

  auto efn = N<IdExpr>(fn->ast->name);
  efn->setType(
      ctx->instantiateGeneric(ctx->getType("unrealized_type"), {fn->getFunc()}));
  efn->setDone();
  Expr *call = N<CallExpr>(N<IdExpr>("Partial"),
                           std::vector<CallArg>{{"args", args},
                                                {"kwargs", kwargs},
                                                {"M", N<StringExpr>(strMask)},
                                                {"F", efn}});
  call = transform(call);
  seqassert(call->getType()->is("Partial"), "expected partial type: {:c}",
            call->getType());
  return call;
}

} // namespace codon::ast
