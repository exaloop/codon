// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;
using namespace matcher;

/// Transform print statement.
/// @example
///   `print a, b` -> `print(a, b)`
///   `print a, b,` -> `print(a, b, end=' ')`
void TypecheckVisitor::visit(PrintStmt *stmt) {
  std::vector<CallArg> args;
  args.reserve(stmt->size());
  for (auto &i : *stmt)
    args.emplace_back(i);
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
  if (expr->isPipe() && realize(expr->getType())) {
    expr->setDone();
  } else if (expr->isStandalone()) {
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
  if (match(expr->getExpr(), M<IdExpr>("tuple")) && expr->size() == 1) {
    expr->setAttribute(Attr::TupleCall);
  }

  validateCall(expr);

  // Check if this call is partial call
  PartialCallData part;
  if (!expr->empty()) {
    if (auto el = cast<EllipsisExpr>(expr->back().getExpr())) {
      if (expr->back().getName().empty() && !el->isPipe())
        el->mode = EllipsisExpr::PARTIAL;
      if (el->mode == EllipsisExpr::PARTIAL)
        part.isPartial = true;
    }
  }

  // Do not allow realization here (function will be realized later);
  // used to prevent early realization of compile_error
  expr->setAttribute(Attr::ParentCallExpr);
  if (part.isPartial)
    expr->getExpr()->setAttribute(Attr::ExprDoNotRealize);
  expr->expr = transform(expr->getExpr());
  expr->eraseAttribute(Attr::ParentCallExpr);
  if (isUnbound(expr->getExpr()))
    return; // delay

  auto [calleeFn, newExpr] = getCalleeFn(expr, part);
  // Transform `tuple(i for i in tup)` into a GeneratorExpr
  // that will be handled during the type checking.
  if (!calleeFn && expr->hasAttribute(Attr::TupleCall)) {
    if (cast<GeneratorExpr>(expr->begin()->getExpr())) {
      auto g = cast<GeneratorExpr>(expr->begin()->getExpr());
      if (!g || g->kind != GeneratorExpr::Generator || g->loopCount() != 1)
        E(Error::CALL_TUPLE_COMPREHENSION, expr->begin()->getExpr());
      g->kind = GeneratorExpr::TupleGenerator;
      resultExpr = transform(g);
      return;
    } else {
      resultExpr = transformTupleFn(expr);
      return;
    }
  } else if ((resultExpr = newExpr)) {
    return;
  } else if (!calleeFn) {
    return;
  }

  if (!withClassGenerics(
          calleeFn.get(), [&]() { return transformCallArgs(expr); }, true, true))
    return;
  // Early dispatch modifier
  if (isDispatch(calleeFn.get())) {
    if (startswith(calleeFn->getFuncName(), "Tuple.__new__")) {
      generateTuple(expr->size());
    }
    std::unique_ptr<std::vector<FuncType *>> m = nullptr;
    if (auto id = cast<IdExpr>(expr->getExpr())) {
      // Case: function overloads (IdExpr)
      std::vector<types::FuncType *> methods;
      auto key = id->getValue();
      if (isDispatch(key))
        key = key.substr(0, key.size() - std::string(FN_DISPATCH_SUFFIX).size());
      for (auto &m : getOverloads(key)) {
        if (!isDispatch(m))
          methods.push_back(getFunction(m)->getType());
      }
      std::reverse(methods.begin(), methods.end());
      m = std::make_unique<std::vector<FuncType *>>(findMatchingMethods(
          calleeFn->funcParent ? calleeFn->funcParent->getClass() : nullptr, methods,
          expr->items, expr->getExpr()->getType()->getPartial()));
    }
    // partials have dangling ellipsis that messes up with the unbound check below
    bool doDispatch = !m || m->size() == 0 || part.isPartial;
    if (!doDispatch && m && m->size() > 1) {
      auto unbounds = 0;
      for (auto &a : *expr) {
        if (isUnbound(a.getExpr()))
          return; // typecheck this later once we know the argument
      }
      if (unbounds)
        return;
    }
    if (!doDispatch) {
      calleeFn = instantiateType(m->front(), calleeFn->funcParent
                                                 ? calleeFn->funcParent->getClass()
                                                 : nullptr);
      auto e = N<IdExpr>(calleeFn->getFuncName());
      e->setType(calleeFn);
      if (cast<IdExpr>(expr->getExpr())) {
        expr->expr = e;
      } else {
        expr->expr = N<StmtExpr>(N<ExprStmt>(expr->getExpr()), e);
      }
      expr->getExpr()->setType(calleeFn);
    } else if (m && m->empty()) {
      std::vector<std::string> a;
      for (auto &t : *expr)
        a.emplace_back(fmt::format("{}", t.getExpr()->getType()->getStatic()
                                             ? t.getExpr()->getClassType()->name
                                             : t.getExpr()->getType()->prettyString()));
      auto argsNice = fmt::format("({})", join(a, ", "));
      auto name = getUnmangledName(calleeFn->getFuncName());
      if (calleeFn->getParentType() && calleeFn->getParentType()->getClass())
        name = format("{}.{}", calleeFn->getParentType()->getClass()->niceName, name);
      E(Error::FN_NO_ATTR_ARGS, expr, name, argsNice);
    }
  }

  bool isVirtual = false;
  if (auto dot = cast<DotExpr>(expr->getExpr()->getOrigExpr())) {
    if (auto baseTyp = dot->getExpr()->getClassType()) {
      auto cls = getClass(baseTyp);
      isVirtual = bool(in(cls->virtuals, dot->getMember())) && cls->rtti &&
                  !isTypeExpr(expr->getExpr()) && !isDispatch(calleeFn.get()) &&
                  !calleeFn->ast->hasAttribute(Attr::StaticMethod) &&
                  !calleeFn->ast->hasAttribute(Attr::Property);
    }
  }

  // Handle named and default arguments
  if ((resultExpr = callReorderArguments(calleeFn.get(), expr, part)))
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
  bool done = typecheckCallArgs(calleeFn.get(), expr->items, part.isPartial);
  if (!part.isPartial && calleeFn->canRealize()) {
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
      if (!cast<EllipsisExpr>(r.getExpr())) {
        newArgs.push_back(r.getExpr());
        newArgs.back()->setAttribute(Attr::ExprSequenceItem);
      }
    newArgs.push_back(part.args);
    auto partialCall = generatePartialCall(part.known, calleeFn->getFunc(),
                                           N<TupleExpr>(newArgs), part.kwArgs);
    std::string var = getTemporaryVar("part");
    Expr *call = nullptr;
    if (!part.var.empty()) {
      // Callee is already a partial call
      auto stmts = cast<StmtExpr>(expr->expr)->items;
      stmts.push_back(N<AssignStmt>(N<IdExpr>(var), partialCall));
      call = N<StmtExpr>(stmts, N<IdExpr>(var));
    } else {
      // New partial call: `(part = Partial(stored_args...); part)`
      call = N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), partialCall), N<IdExpr>(var));
    }
    call->setAttribute(Attr::ExprPartial);
    resultExpr = transform(call);
  } else if (isVirtual) {
    if (!realize(calleeFn))
      return;
    auto vid = getRealizationID(calleeFn->getParentType()->getClass(), calleeFn.get());

    // Function[Tuple[TArg1, TArg2, ...], TRet]
    std::vector<Expr *> ids;
    for (auto &t : *calleeFn)
      ids.push_back(N<IdExpr>(t.getType()->realizedName()));
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
                                                  expr->front().getExpr()),
                                      N<IntExpr>(vid)));
    std::vector<CallArg> args;
    for (auto &a : *expr)
      args.emplace_back(a.getExpr());
    resultExpr = transform(N<CallExpr>(e, args));
  } else {
    // Case: normal function call
    unify(expr->getType(), calleeFn->getRetType());
    if (done)
      expr->setDone();
  }
}

void TypecheckVisitor::validateCall(CallExpr *expr) {
  if (expr->hasAttribute(Attr::Validated))
    return;
  bool namesStarted = false, foundEllipsis = false;
  for (auto &a : *expr) {
    if (a.name.empty() && namesStarted &&
        !(cast<KeywordStarExpr>(a.value) || cast<EllipsisExpr>(a.value)))
      E(Error::CALL_NAME_ORDER, a.value);
    if (!a.name.empty() && (cast<StarExpr>(a.value) || cast<KeywordStarExpr>(a.value)))
      E(Error::CALL_NAME_STAR, a.value);
    if (cast<EllipsisExpr>(a.value) && foundEllipsis)
      E(Error::CALL_ELLIPSIS, a.value);
    foundEllipsis |= bool(cast<EllipsisExpr>(a.value));
    namesStarted |= !a.name.empty();
  }
  expr->setAttribute(Attr::Validated);
}

/// Transform call arguments. Expand *args and **kwargs to the list of @c CallArg
/// objects.
/// @return false if expansion could not be completed; true otherwise
bool TypecheckVisitor::transformCallArgs(CallExpr *expr) {
  for (auto ai = 0; ai < expr->size();) {
    if (auto star = cast<StarExpr>((*expr)[ai].getExpr())) {
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
      auto fields = getClassFields(typ);
      for (size_t i = 0; i < fields.size(); i++, ai++) {
        expr->items.insert(
            expr->items.begin() + ai,
            {"", transform(N<DotExpr>(clone(star->getExpr()), fields[i].name))});
      }
      expr->items.erase(expr->items.begin() + ai);
    } else if (auto kwstar = cast<KeywordStarExpr>((*expr)[ai].getExpr())) {
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
        auto id = getIntLiteral(typ);
        seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}",
                  id);
        auto names = ctx->cache->generatedTupleNames[id];
        for (size_t i = 0; i < names.size(); i++, ai++) {
          expr->items.insert(
              expr->items.begin() + ai,
              CallArg{names[i],
                      transform(N<DotExpr>(N<DotExpr>(kwstar->getExpr(), "args"),
                                           format("item{}", i + 1)))});
        }
        expr->items.erase(expr->items.begin() + ai);
      } else if (typ->isRecord()) {
        auto fields = getClassFields(typ);
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
      // Case: normal argument (no expansion)
      (*expr)[ai].value = transform((*expr)[ai].getExpr());
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
std::pair<std::shared_ptr<FuncType>, Expr *>
TypecheckVisitor::getCalleeFn(CallExpr *expr, PartialCallData &part) {
  auto callee = expr->getExpr()->getClassType();
  if (!callee) {
    // Case: unknown callee, wait until it becomes known
    return {nullptr, nullptr};
  }

  if (expr->hasAttribute(Attr::TupleCall) &&
      (extractType(expr->getExpr())->is(TYPE_TUPLE) ||
       (callee->getFunc() &&
        startswith(callee->getFunc()->ast->name, "std.internal.static.tuple."))))
    return {nullptr, nullptr};

  if (isTypeExpr(expr->getExpr())) {
    auto typ = expr->getExpr()->getClassType();
    if (!isId(expr->getExpr(), TYPE_TYPE))
      typ = extractClassGeneric(typ)->getClass();
    if (!typ)
      return {nullptr, nullptr};
    auto clsName = typ->name;
    if (typ->isRecord()) {
      if (expr->hasAttribute(Attr::TupleCall)) {
        expr->eraseAttribute(Attr::TupleCall);
      }
      // Case: tuple constructor. Transform to: `T.__new__(args)`
      auto e =
          transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "__new__"), expr->items));
      return {nullptr, e};
    }

    // Case: reference type constructor. Transform to
    // `ctr = T.__new__(); v.__init__(args)`
    Expr *var = N<IdExpr>(getTemporaryVar("ctr"));
    auto newInit =
        N<AssignStmt>(clone(var), N<CallExpr>(N<DotExpr>(expr->getExpr(), "__new__")));
    auto e = N<StmtExpr>(N<SuiteStmt>(newInit), clone(var));
    auto init =
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__init__"), expr->items));
    e->items.emplace_back(init);
    return {nullptr, transform(e)};
  }

  if (auto partType = callee->getPartial()) {
    auto mask = partType->getPartialMask();
    auto genFn = partType->getPartialFunc()->generalize(0);
    auto calleeFn =
        std::static_pointer_cast<types::FuncType>(instantiateType(genFn.get()));

    if (!partType->isPartialEmpty() ||
        std::any_of(mask.begin(), mask.end(), [](char c) { return c; })) {
      // Case: calling partial object `p`. Transform roughly to
      // `part = callee; partial_fn(*part.args, args...)`
      Expr *var = N<IdExpr>(part.var = getTemporaryVar("partcall"));
      expr->expr = transform(N<StmtExpr>(N<AssignStmt>(clone(var), expr->getExpr()),
                                         N<IdExpr>(calleeFn->getFuncName())));
      part.known = mask;
    } else {
      expr->expr = transform(N<IdExpr>(calleeFn->getFuncName()));
    }
    seqassert(expr->getExpr()->getType()->getFunc(), "not a function: {}",
              *(expr->getExpr()->getType()));
    unify(expr->getExpr()->getType(), calleeFn);

    // Unify partial generics with types known thus far
    auto knownArgTypes = extractClassGeneric(partType, 1)->getClass();
    for (size_t i = 0, j = 0, k = 0; i < mask.size(); i++)
      if ((*calleeFn->ast)[i].isGeneric()) {
        j++;
      } else if (mask[i]) {
        unify(extractFuncArgType(calleeFn.get(), i - j),
              extractClassGeneric(knownArgTypes, k));
        k++;
      }
    return {calleeFn, nullptr};
  } else if (!callee->getFunc()) {
    // Case: callee is not a function. Try __call__ method instead
    return {nullptr, transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "__call__"),
                                           expr->items))};
  } else {
    return {std::static_pointer_cast<types::FuncType>(
                callee->getFunc()->shared_from_this()),
            nullptr};
  }
}

/// Reorder the call arguments to match the signature order. Ensure that every @c
/// CallArg has a set name. Form *args/**kwargs tuples if needed, and use partial
/// and default values where needed.
/// @example
///   `foo(1, 2, baz=3, baf=4)` -> `foo(a=1, baz=2, args=(3, ), kwargs=KwArgs(baf=4))`
Expr *TypecheckVisitor::callReorderArguments(FuncType *calleeFn, CallExpr *expr,
                                             PartialCallData &part) {
  if (calleeFn->ast->hasAttribute(Attr::NoArgReorder))
    return nullptr;

  std::vector<CallArg> args;    // stores ordered and processed arguments
  std::vector<Expr *> typeArgs; // stores type and static arguments (e.g., `T: type`)
  auto newMask = std::vector<char>(calleeFn->ast->size(), 1);

  // Extract pi-th partial argument from a partial object
  auto getPartialArg = [&](size_t pi) {
    auto id = transform(N<DotExpr>(N<IdExpr>(part.var), "args"));
    // Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
    auto ex = transformStaticTupleIndex(id->getClassType(), id, N<IntExpr>(pi));
    seqassert(ex.first && ex.second, "partial indexing failed: {}", *(id->getType()));
    return ex.second;
  };

  // Handle reordered arguments (see @c reorderNamedArgs for details)
  bool partial = false;
  auto reorderFn = [&](int starArgIndex, int kwstarArgIndex,
                       const std::vector<std::vector<int>> &slots, bool _partial) {
    partial = _partial;
    return withClassGenerics(
        calleeFn,
        [&]() {
          for (size_t si = 0, pi = 0, gi = 0; si < slots.size(); si++) {
            // Get the argument name to be used later
            auto [_, rn] = (*calleeFn->ast)[si].getNameWithStars();
            auto realName = getUnmangledName(rn);

            if ((*calleeFn->ast)[si].isGeneric()) {
              // Case: generic arguments. Populate typeArgs
              if (startswith(realName, "$")) {
                if (slots[si].empty()) {
                  if (!part.known.empty() && part.known[si]) {
                    auto t = N<IdExpr>(realName);
                    t->setType(
                        calleeFn->funcGenerics[gi].getType()->shared_from_this());
                    typeArgs.emplace_back(t);
                  } else {
                    typeArgs.emplace_back(transform(N<IdExpr>(realName.substr(1))));
                  }
                } else {
                  typeArgs.emplace_back((*expr)[slots[si][0]].getExpr());
                }
                newMask[si] = 1;
              } else {
                typeArgs.push_back(slots[si].empty() ? nullptr
                                                     : (*expr)[slots[si][0]].getExpr());
                newMask[si] = slots[si].empty() ? 0 : 1;
              }
              gi++;
            } else if (si == starArgIndex &&
                       !(slots[si].size() == 1 &&
                         (*expr)[slots[si][0]].getExpr()->hasAttribute(
                             Attr::ExprStarArgument))) {
              // Case: *args. Build the tuple that holds them all
              std::vector<Expr *> extra;
              if (!part.known.empty())
                extra.push_back(N<StarExpr>(getPartialArg(-1)));
              for (auto &e : slots[si]) {
                extra.push_back((*expr)[e].getExpr());
              }
              Expr *e = N<TupleExpr>(extra);
              e->setAttribute(Attr::ExprStarArgument);
              if (!match(expr->getExpr(), M<IdExpr>("hasattr")))
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
                         (*expr)[slots[si][0]].getExpr()->hasAttribute(
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
                names.emplace_back((*expr)[e].getName());
                values.emplace_back((*expr)[e].getExpr());
              }

              auto kwid = generateKwId(names);
              auto e = transform(N<CallExpr>(N<IdExpr>("NamedTuple"),
                                             N<TupleExpr>(values), N<IntExpr>(kwid)));
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
              // Case: no argument. Check if the arguments is provided by the partial
              // type (if calling it) or if a default argument can be used
              if (!part.known.empty() && part.known[si]) {
                args.emplace_back(realName, getPartialArg(pi++));
              } else if (startswith(realName, "$")) {
                args.emplace_back(realName, transform(N<IdExpr>(realName.substr(1))));
              } else if (partial) {
                args.emplace_back(realName,
                                  transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
                newMask[si] = 0;
              } else {
                if (cast<NoneExpr>((*calleeFn->ast)[si].getDefault()) &&
                    !(*calleeFn->ast)[si].type) {
                  args.emplace_back(
                      realName, transform(N<CallExpr>(N<InstantiateExpr>(
                                    N<IdExpr>("Optional"), N<IdExpr>("NoneType")))));
                } else {
                  args.emplace_back(realName, transform(clean_clone(
                                                  (*calleeFn->ast)[si].getDefault())));
                }
              }
            } else {
              // Case: argument provided
              seqassert(slots[si].size() == 1, "call transformation failed");
              args.emplace_back(realName, (*expr)[slots[si][0]].getExpr());
            }
          }
          return 0;
        },
        true);
  };

  // Reorder arguments if needed
  part.args = part.kwArgs = nullptr; // Stores partial *args/**kwargs expression
  if (expr->hasAttribute(Attr::ExprOrderedCall)) {
    args = expr->items;
  } else {
    reorderNamedArgs(
        calleeFn, expr->items, reorderFn,
        [&](error::Error e, const SrcInfo &o, const std::string &errorMsg) {
          E(Error::CUSTOM, o, errorMsg.c_str());
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
      const auto &gen = calleeFn->funcGenerics[si];
      if (typeArgs[si]) {
        auto typ = extractType(typeArgs[si]);
        if (gen.isStatic)
          if (!typ->isStaticType())
            E(Error::EXPECTED_STATIC, typeArgs[si]);
        unify(typ, gen.getType());
      } else {
        if (isUnbound(gen.getType()) && !(*calleeFn->ast)[si].getDefault() &&
            !partial && in(niGenerics, gen.name)) {
          E(Error::CUSTOM, getSrcInfo(), "generic '{}' not provided", gen.niceName);
        }
      }
    }
  }

  expr->items = args;
  expr->setAttribute(Attr::ExprOrderedCall);
  part.known = newMask;
  return nullptr;
}

/// Unify the call arguments' types with the function declaration signatures.
/// Also apply argument transformations to ensure the type compatibility and handle
/// default generics.
/// @example
///   `foo(1, 2)` -> `foo(1, Optional(2), T=int)`
bool TypecheckVisitor::typecheckCallArgs(FuncType *calleeFn, std::vector<CallArg> &args,
                                         bool isPartial) {
  bool wrappingDone = true;         // tracks whether all arguments are wrapped
  std::vector<Type *> replacements; // list of replacement arguments

  withClassGenerics(
      calleeFn,
      [&]() {
        for (size_t i = 0, si = 0; i < calleeFn->ast->size(); i++) {
          if ((*calleeFn->ast)[i].isGeneric())
            continue;

          if (startswith((*calleeFn->ast)[i].getName(), "*") &&
              (*calleeFn->ast)[i].getType()) {
            // Special case: `*args: type` and `**kwargs: type`
            if (auto callExpr = cast<CallExpr>(args[si].getExpr())) {
              auto typ = extractType(transform(clone((*calleeFn->ast)[i].getType())));
              if (startswith((*calleeFn->ast)[i].getName(), "**"))
                callExpr = cast<CallExpr>(callExpr->front().getExpr());
              for (auto &ca : *callExpr) {
                if (wrapExpr(&ca.value, typ, calleeFn)) {
                  unify(ca.getExpr()->getType(), typ);
                } else {
                  wrappingDone = false;
                }
              }
              auto name = callExpr->getClassType()->name;
              auto tup = transform(N<CallExpr>(N<IdExpr>(name), callExpr->items));
              if (startswith((*calleeFn->ast)[i].getName(), "**")) {
                args[si].value = transform(N<CallExpr>(
                    N<DotExpr>(N<IdExpr>("NamedTuple"), "__new__"), tup,
                    N<IntExpr>(extractClassGeneric(args[si].getExpr()->getType())
                                   ->getIntStatic()
                                   ->value)));
              } else {
                args[si].value = tup;
              }
            }
            replacements.push_back(args[si].getExpr()->getType());
            // else this is empty and is a partial call; leave it for later
          } else {
            if (wrapExpr(&args[si].value, extractFuncArgType(calleeFn, si), calleeFn)) {
              unify(args[si].getExpr()->getType(), extractFuncArgType(calleeFn, si));
            } else {
              wrappingDone = false;
            }
            replacements.push_back(!extractFuncArgType(calleeFn, si)->getClass()
                                       ? args[si].getExpr()->getType()
                                       : extractFuncArgType(calleeFn, si));
          }
          si++;
        }
        return true;
      },
      true);

  // Realize arguments
  bool done = true;
  for (auto &a : args) {
    // Previous unifications can qualify existing identifiers.
    // Transform again to get the full identifier
    if (realize(a.getExpr()->getType()))
      a.value = transform(a.getExpr());
    done &= a.getExpr()->isDone();
  }

  // Handle default generics
  if (!isPartial)
    for (size_t i = 0, j = 0; wrappingDone && i < calleeFn->ast->size(); i++)
      if ((*calleeFn->ast)[i].isGeneric()) {
        if ((*calleeFn->ast)[i].getDefault() &&
            isUnbound(extractFuncGeneric(calleeFn, j))) {
          auto def = extractType(withClassGenerics(
              calleeFn,
              [&]() {
                return transform(clean_clone((*calleeFn->ast)[i].getDefault()));
              },
              true));
          unify(extractFuncGeneric(calleeFn, j), def);
        }
        j++;
      }

  // Replace the arguments
  for (size_t si = 0; si < replacements.size(); si++) {
    if (replacements[si]) {
      extractClassGeneric(calleeFn)->getClass()->generics[si].type =
          replacements[si]->shared_from_this();
    }
  }
  extractClassGeneric(calleeFn)->getClass()->_rn = "";
  calleeFn->getClass()->_rn = ""; /// TODO: TERRIBLE!

  return done;
}

/// Transform and typecheck the following special call expressions:
///   `superf(fn)`
///   `super()`
///   `__ptr__(var)`
///   `__array__[int](sz)`
///   `isinstance(obj, type)`
///   `static.len(tup)`
///   `hasattr(obj, "attr")`
///   `getattr(obj, "attr")`
///   `type(obj)`
///   `compile_err("msg")`
/// See below for more details.
std::pair<bool, Expr *> TypecheckVisitor::transformSpecialCall(CallExpr *expr) {
  if (expr->hasAttribute(Attr::ExprNoSpecial))
    return {false, nullptr};

  auto ei = cast<IdExpr>(expr->expr);
  if (!ei)
    return {false, nullptr};
  auto isF = [](IdExpr *val, const std::string &s) {
    return val->getValue() == s || val->getValue() == s + ":0" ||
           val->getValue() == s + ".0:0";
  };
  if (isF(ei, "superf")) {
    return {true, transformSuperF(expr)};
  } else if (isF(ei, "super")) {
    return {true, transformSuper()};
  } else if (isF(ei, "__ptr__")) {
    return {true, transformPtr(expr)};
  } else if (isF(ei, "__array__.__new__")) {
    return {true, transformArray(expr)};
  } else if (isF(ei, "isinstance")) { // static
    return {true, transformIsInstance(expr)};
  } else if (isF(ei, "std.internal.static.len")) { // static
    return {true, transformStaticLen(expr)};
  } else if (isF(ei, "hasattr")) { // static
    return {true, transformHasAttr(expr)};
  } else if (isF(ei, "getattr")) {
    return {true, transformGetAttr(expr)};
  } else if (isF(ei, "setattr")) {
    return {true, transformSetAttr(expr)};
  } else if (isF(ei, "type.__new__")) {
    return {true, transformTypeFn(expr)};
  } else if (isF(ei, "compile_error")) {
    return {true, transformCompileError(expr)};
  } else if (isF(ei, "std.internal.static.print")) {
    return {false, transformStaticPrintFn(expr)};
  } else if (isF(ei, "std.collections.namedtuple")) {
    return {true, transformNamedTuple(expr)};
  } else if (isF(ei, "std.functools.partial")) {
    return {true, transformFunctoolsPartial(expr)};
  } else if (isF(ei, "std.internal.static.has_rtti")) { // static
    return {true, transformHasRttiFn(expr)};
  } else if (isF(ei, "std.internal.static.function.0.realized")) {
    return {true, transformRealizedFn(expr)};
  } else if (isF(ei, "std.internal.static.function.0.can_call")) { // static
    return {true, transformStaticFnCanCall(expr)};
  } else if (isF(ei, "std.internal.static.function.0.has_type")) { // static
    return {true, transformStaticFnArgHasType(expr)};
  } else if (isF(ei, "std.internal.static.function.0.get_type")) {
    return {true, transformStaticFnArgGetType(expr)};
  } else if (isF(ei, "std.internal.static.function.0.args")) {
    return {true, transformStaticFnArgs(expr)};
  } else if (isF(ei, "std.internal.static.function.0.has_default")) { // static
    return {true, transformStaticFnHasDefault(expr)};
  } else if (isF(ei, "std.internal.static.function.0.get_default")) {
    return {true, transformStaticFnGetDefault(expr)};
  } else if (isF(ei, "std.internal.static.function.0.wrap_args")) {
    return {true, transformStaticFnWrapCallArgs(expr)};
  } else if (isF(ei, "std.internal.static.vars")) {
    return {true, transformStaticVars(expr)};
  } else if (isF(ei, "std.internal.static.tuple_type")) {
    return {true, transformStaticTupleType(expr)};
  } else {
    return {false, nullptr};
  }
}

/// Get the list that describes the inheritance hierarchy of a given type.
/// The first type in the list is the most recently inherited type.
std::vector<TypePtr> TypecheckVisitor::getSuperTypes(ClassType *cls) {
  std::vector<TypePtr> result;
  if (!cls)
    return result;

  result.push_back(cls->shared_from_this());
  auto c = getClass(cls);
  auto fields = getClassFields(cls);
  for (auto &name : c->staticParentClasses) {
    auto parentTyp = instantiateType(extractClassType(name));
    auto parentFields = getClassFields(parentTyp->getClass());
    for (auto &field : fields) {
      for (auto &parentField : parentFields)
        if (field.name == parentField.name) {
          auto t = instantiateType(field.getType(), cls);
          unify(t.get(), instantiateType(parentField.getType(), parentTyp->getClass()));
          break;
        }
    }
    for (auto &t : getSuperTypes(parentTyp->getClass()))
      result.push_back(t);
  }
  return result;
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

  auto efn = N<IdExpr>(fn->getFuncName());
  efn->setType(instantiateType(getStdLibType("unrealized_type"),
                               std::vector<types::Type *>{fn->getFunc()}));
  efn->setDone();
  Expr *call = N<CallExpr>(N<IdExpr>("Partial"),
                           std::vector<CallArg>{{"args", args},
                                                {"kwargs", kwargs},
                                                {"M", N<StringExpr>(strMask)},
                                                {"F", efn}});
  return call;
}

} // namespace codon::ast
