// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
  auto orig = expr->toString(0);
  if (match(expr->getExpr(), M<IdExpr>("tuple")) && expr->size() == 1)
    expr->setAttribute("TupleFn");

  // Check if this call is partial call
  PartialCallData part;

  expr->setAttribute("CallExpr");
  expr->expr = transform(expr->getExpr());
  expr->eraseAttribute("CallExpr");
  if (isUnbound(expr->getExpr()))
    return; // delay

  auto [calleeFn, newExpr] = getCalleeFn(expr, part);
  // Transform `tuple(i for i in tup)` into a GeneratorExpr that will be handled during
  // the type checking.
  if (!calleeFn && expr->hasAttribute("TupleFn")) {
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
  part.isPartial = !expr->empty() && cast<EllipsisExpr>(expr->back().getExpr()) &&
                   cast<EllipsisExpr>(expr->back().getExpr())->isPartial();
  // Early dispatch modifier
  if (calleeFn->getFuncName() == "Tuple.__new__:dispatch")
    generateTuple(expr->size());
  if (isDispatch(calleeFn.get())) {
    std::unique_ptr<std::vector<FuncType *>> m = nullptr;
    if (auto id = cast<IdExpr>(expr->getExpr())) {
      // Case: function overloads (IdExpr)
      std::vector<types::FuncType *> methods;
      auto key = id->getValue();
      if (endswith(key, FN_DISPATCH_SUFFIX))
        key = key.substr(0, key.size() - FN_DISPATCH_SUFFIX.size());
      for (auto &m : getOverloads(key))
        if (!endswith(m, FN_DISPATCH_SUFFIX))
          methods.push_back(getFunction(m)->getType());
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
      calleeFn = std::static_pointer_cast<types::FuncType>(ctx->instantiate(
          m->front(),
          calleeFn->funcParent ? calleeFn->funcParent->getClass() : nullptr));
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
      auto argsNice = fmt::format("({})", fmt::join(a, ", "));
      E(Error::FN_NO_ATTR_ARGS, expr, getUnmangledName(calleeFn->getFuncName()),
        argsNice);
    }
  }

  bool isVirtual = false;
  if (auto dot = cast<DotExpr>(expr->getExpr()->getOrigExpr())) {
    if (auto baseTyp = dot->getExpr()->getClassType()) {
      auto cls = getClass(baseTyp);
      isVirtual = bool(in(cls->virtuals, dot->getMember())) && cls->rtti &&
                  !expr->expr->getType()->is("type") &&
                  !endswith(calleeFn->getFuncName(), ":dispatch") &&
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
  bool done = typecheckCallArgs(calleeFn.get(), expr->items);
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
    for (auto &t : calleeFn->getArgs())
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
      if (auto el = cast<EllipsisExpr>((*expr)[ai].getExpr())) {
        if (ai + 1 == expr->size() && (*expr)[ai].getName().empty() && !el->isPipe())
          el->mode = EllipsisExpr::PARTIAL;
      }
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

  if (isTypeExpr(expr->getExpr())) {
    auto typ = expr->getExpr()->getClassType();
    if (!isId(expr->getExpr(), "type"))
      typ = extractClassGeneric(typ)->getClass();
    if (!typ)
      return {nullptr, nullptr};
    auto clsName = typ->name;
    if (typ->isRecord()) {
      if (expr->hasAttribute("TupleFn")) {
        if (extractType(expr->getExpr())->is("Tuple"))
          return {nullptr, nullptr};
        expr->eraseAttribute("TupleFn");
      }
      // Case: tuple constructor. Transform to: `T.__new__(args)`
      return {nullptr, transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "__new__"),
                                             expr->items))};
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
        std::static_pointer_cast<types::FuncType>(ctx->instantiate(genFn.get()));

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
          for (size_t si = 0, pi = 0; si < slots.size(); si++) {
            // Get the argument name to be used later
            auto rn = (*calleeFn->ast)[si].getName();
            trimStars(rn);
            auto realName = getUnmangledName(rn);

            if ((*calleeFn->ast)[si].isGeneric()) {
              // Case: generic arguments. Populate typeArgs
              typeArgs.push_back(slots[si].empty() ? nullptr
                                                   : (*expr)[slots[si][0]].getExpr());
              newMask[si] = slots[si].empty() ? 0 : 1;
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
              } else if (partial) {
                args.emplace_back(realName,
                                  transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
                newMask[si] = 0;
              } else {
                if (cast<NoneExpr>((*calleeFn->ast)[si].getDefault()) &&
                    !(*calleeFn->ast)[si].type) {
                  args.push_back(
                      {realName, transform(N<CallExpr>(N<InstantiateExpr>(
                                     N<IdExpr>("Optional"), N<IdExpr>("NoneType"))))});
                } else {
                  args.push_back({realName, transform(clean_clone(
                                                (*calleeFn->ast)[si].getDefault()))});
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
    ctx->reorderNamedArgs(
        calleeFn, expr->items, reorderFn,
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
        auto typ = extractType(typeArgs[si]);
        if (calleeFn->funcGenerics[si].isStatic)
          if (!typ->isStaticType())
            E(Error::EXPECTED_STATIC, typeArgs[si]);
        unify(typ, calleeFn->funcGenerics[si].getType());
      } else {
        if (isUnbound(calleeFn->funcGenerics[si].getType()) &&
            !(*calleeFn->ast)[si].getDefault() && !partial &&
            in(niGenerics, calleeFn->funcGenerics[si].name)) {
          error("generic '{}' not provided", calleeFn->funcGenerics[si].niceName);
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
bool TypecheckVisitor::typecheckCallArgs(FuncType *calleeFn,
                                         std::vector<CallArg> &args) {
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
            if ((*calleeFn->ast)[i].getType() &&
                !extractFuncArgType(calleeFn, si)->canRealize()) {
              auto gt = extractType((*calleeFn->ast)[i].getType())->generalize(0);
              unify(extractFuncArgType(calleeFn, si), ctx->instantiate(gt.get()));
            }
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
  for (size_t i = 0, j = 0; wrappingDone && i < calleeFn->ast->size(); i++)
    if ((*calleeFn->ast)[i].status == Param::Generic) {
      if ((*calleeFn->ast)[i].defaultValue &&
          isUnbound(extractFuncGeneric(calleeFn, j))) {
        auto def = extractType(withClassGenerics(
            calleeFn,
            [&]() { return transform(clone((*calleeFn->ast)[i].getDefault())); },
            true));
        unify(extractFuncGeneric(calleeFn, j), def);
      }
      j++;
    }

  // Replace the arguments
  for (size_t si = 0; si < replacements.size(); si++) {
    if (replacements[si]) {
      calleeFn->generics[0].type->getClass()->generics[si].type =
          replacements[si]->shared_from_this();
      calleeFn->generics[0].type->getClass()->_rn = "";
      calleeFn->getClass()->_rn = ""; /// TODO: TERRIBLE!
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
  auto name = extractFuncGeneric(expr->getExpr()->getType())->getStrStatic()->value;
  if (expr->size() != 1)
    E(Error::CALL_NAMEDTUPLE, expr);

  // Construct the class statement
  std::vector<Param> generics, params;
  auto orig = cast<TupleExpr>(expr->front().getExpr()->getOrigExpr());
  size_t ti = 1;
  for (auto *i : *orig) {
    if (auto s = cast<StringExpr>(i)) {
      generics.emplace_back(format("T{}", ti), N<IdExpr>("type"), nullptr, true);
      params.emplace_back(s->getValue(), N<IdExpr>(format("T{}", ti++)), nullptr);
      continue;
    }
    auto t = cast<TupleExpr>(i);
    if (t && t->size() == 2 && cast<StringExpr>((*t)[0])) {
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
  std::vector<types::FuncType *> supers;
  if (!endswith(func->getFuncName(), ":dispatch")) {
    if (auto aa =
            func->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass)) {
      auto p = ctx->getType(aa->value);
      if (auto pc = p->getClass()) {
        if (auto c = getClass(pc)) {
          if (auto m = in(c->methods, getUnmangledName(func->getFuncName()))) {
            for (auto &overload : getOverloads(*m)) {
              if (isDispatch(overload))
                continue;
              if (overload == func->getFuncName())
                break;
              supers.emplace_back(getFunction(overload)->getType());
            }
          }
        }
        std::reverse(supers.begin(), supers.end());
      }
    }
  }
  if (supers.empty())
    E(Error::CALL_SUPERF, expr);

  seqassert(expr->size() == 1 && cast<CallExpr>(expr->begin()->getExpr()),
            "bad superf call");
  std::vector<CallArg> newArgs;
  for (const auto &a : *cast<CallExpr>(expr->begin()->getExpr()))
    newArgs.emplace_back(a.getExpr());
  auto m = findMatchingMethods(
      func->funcParent ? func->funcParent->getClass() : nullptr, supers, newArgs);
  if (m.empty())
    E(Error::CALL_SUPERF, expr);
  auto c = transform(N<CallExpr>(N<IdExpr>(m[0]->getFuncName()), newArgs));
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
  if (funcTyp->getArgs().empty())
    E(Error::CALL_SUPER_PARENT, getSrcInfo());

  ClassType *typ = extractFuncArgType(funcTyp)->getClass();
  auto cls = getClass(typ);
  auto cands = cls->staticParentClasses;
  if (cands.empty()) {
    // Dynamic inheritance: use MRO
    // TODO: maybe super() should be split into two separate functions...
    const auto &vCands = cls->mro;
    if (vCands.size() < 2)
      E(Error::CALL_SUPER_PARENT, getSrcInfo());

    auto superTyp = ctx->instantiate(vCands[1].get(), typ);
    auto self = N<IdExpr>(funcTyp->ast->begin()->name);
    self->setType(typ->shared_from_this());

    auto typExpr = N<IdExpr>(superTyp->getClass()->name);
    typExpr->setType(
        ctx->instantiateGeneric(getStdLibType("type"), {superTyp->getClass()}));
    // LOG("-> {:c} : {:c} {:c}", typ, vCands[1], typExpr->type);
    return transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"),
                                 self, typExpr, N<IntExpr>(1)));
  }

  auto name = cands.front(); // the first inherited type
  auto superTyp = ctx->instantiate(ctx->getType(name), typ);
  if (typ->isRecord()) {
    // Case: tuple types. Return `tuple(obj.args...)`
    std::vector<Expr *> members;
    for (auto &field : getClassFields(superTyp->getClass()))
      members.push_back(
          N<DotExpr>(N<IdExpr>(funcTyp->ast->begin()->getName()), field.name));
    Expr *e = transform(N<TupleExpr>(members));
    auto ft = getClassFieldTypes(superTyp->getClass());
    for (size_t i = 0; i < ft.size(); i++)
      unify(ft[i].get(), extractClassGeneric(e->getType(), i)); // see super_tuple test
    e->setType(superTyp->shared_from_this());
    return e;
  } else {
    // Case: reference types. Return `__internal__.class_super(self, T)`
    auto self = N<IdExpr>(funcTyp->ast->begin()->name);
    self->setType(typ->shared_from_this());
    return castToSuperClass(self, superTyp->getClass());
  }
}

/// Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
/// the argument is a variable binding.
Expr *TypecheckVisitor::transformPtr(CallExpr *expr) {
  auto id = cast<IdExpr>(expr->begin()->getExpr());
  auto val = id ? ctx->find(id->getValue()) : nullptr;
  if (!val || !val->isVar())
    E(Error::CALL_PTR_VAR, expr->begin()->getExpr());

  expr->begin()->value = transform(expr->begin()->getExpr());
  unify(expr->getType(),
        ctx->instantiateGeneric(getStdLibType("Ptr"),
                                {expr->begin()->getExpr()->getType()}));
  if (expr->begin()->getExpr()->isDone())
    expr->setDone();
  return nullptr;
}

/// Typecheck __array__ method. This method creates a stack-allocated array via alloca.
Expr *TypecheckVisitor::transformArray(CallExpr *expr) {
  auto arrTyp = expr->expr->getType()->getFunc();
  unify(expr->getType(),
        ctx->instantiateGeneric(getStdLibType("Array"),
                                {extractClassGeneric(arrTyp->getParentType())}));
  if (realize(expr->getType()))
    expr->setDone();
  return nullptr;
}

/// Transform isinstance method to a static boolean expression.
/// Special cases:
///   `isinstance(obj, ByVal)` is True if `type(obj)` is a tuple type
///   `isinstance(obj, ByRef)` is True if `type(obj)` is a reference type
Expr *TypecheckVisitor::transformIsInstance(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->getExpr());
  auto typ = expr->begin()->getExpr()->getClassType();
  if (!typ || !typ->canRealize())
    return nullptr;

  expr->begin()->value = transform(expr->begin()->getExpr()); // again to realize it

  typ = extractClassType(typ);
  auto &typExpr = (*expr)[1].value;
  if (auto c = cast<CallExpr>(typExpr)) {
    // Handle `isinstance(obj, (type1, type2, ...))`
    if (typExpr->getOrigExpr() && cast<TupleExpr>(typExpr->getOrigExpr())) {
      Expr *result = transform(N<BoolExpr>(false));
      for (auto *i : *cast<TupleExpr>(typExpr->getOrigExpr())) {
        result = transform(N<BinaryExpr>(
            result, "||",
            N<CallExpr>(N<IdExpr>("isinstance"), expr->begin()->getExpr(), i)));
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
  } else if (!extractType(typExpr)->getUnion() && typ->getUnion()) {
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      if (extractType(typExpr)->unify(unionTypes[ui], nullptr) >= 0) {
        tag = int(ui);
        break;
      }
    }
    if (tag == -1)
      return transform(N<BoolExpr>(false));
    return transform(N<BinaryExpr>(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_tag"),
                    expr->begin()->getExpr()),
        "==", N<IntExpr>(tag)));
  } else if (typExpr->getType()->is("pyobj")) {
    if (typ->is("pyobj")) {
      return transform(N<CallExpr>(N<IdExpr>("std.internal.python._isinstance.0"),
                                   expr->begin()->getExpr(), (*expr)[1].getExpr()));
    } else {
      return transform(N<BoolExpr>(false));
    }
  }

  typExpr = transformType(typExpr);
  auto targetType = extractType(typExpr);
  // Check super types (i.e., statically inherited) as well
  for (auto &tx : getSuperTypes(typ->getClass())) {
    types::Type::Unification us;
    auto s = tx->unify(targetType, &us);
    us.undo();
    if (s >= 0)
      return transform(N<BoolExpr>(true));
  }
  return transform(N<BoolExpr>(false));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
Expr *TypecheckVisitor::transformStaticLen(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->getExpr());
  auto typ = extractType(expr->begin()->getExpr());

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
    E(Error::EXPECTED_TUPLE, expr->begin()->getExpr());
  return transform(N<IntExpr>(getClassFields(typ->getClass()).size()));
}

/// Transform hasattr method to a static boolean expression.
/// This method also supports additional argument types that are used to check
/// for a matching overload (not available in Python).
Expr *TypecheckVisitor::transformHasAttr(CallExpr *expr) {
  auto typ = extractClassType((*expr)[0].getExpr());
  if (!typ)
    return nullptr;

  auto member = extractFuncGeneric(expr->getExpr()->getType())->getStrStatic()->value;
  std::vector<std::pair<std::string, types::Type *>> args{{"", typ}};

  if (auto tup = cast<TupleExpr>((*expr)[1].getExpr())) {
    for (auto &a : *tup) {
      a = transform(a);
      if (!a->getClassType())
        return nullptr;
      args.emplace_back("", extractType(a));
    }
  }
  for (auto &[n, ne] : extractNamedTuple((*expr)[2].getExpr())) {
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
          N<CallExpr>(N<IdExpr>("isinstance"), (*expr)[0].getExpr(), te), "&&",
          N<CallExpr>(N<IdExpr>("hasattr"), te, N<StringExpr>(member)));
      cond = !cond ? e : N<BinaryExpr>(cond, "||", e);
    }
    if (!cond)
      return transform(N<BoolExpr>(false));
    return transform(cond);
  }

  bool exists = !ctx->findMethod(typ->getClass(), member).empty() ||
                ctx->findMember(typ->getClass(), member);
  if (exists && args.size() > 1)
    exists &= findBestMethod(typ, member, args) != nullptr;
  return transform(N<BoolExpr>(exists));
}

/// Transform getattr method to a DotExpr.
Expr *TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto name = extractFuncGeneric(expr->expr->getType())->getStrStatic()->value;

  // special handling for NamedTuple
  if (expr->begin()->getExpr()->getType() &&
      expr->begin()->getExpr()->getType()->is("NamedTuple")) {
    auto val = expr->begin()->getExpr()->getClassType();
    auto id = val->generics[0].type->getIntStatic()->value;
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    auto names = ctx->cache->generatedTupleNames[id];
    for (size_t i = 0; i < names.size(); i++)
      if (names[i] == name) {
        return transform(
            N<IndexExpr>(N<DotExpr>(expr->begin()->getExpr(), "args"), N<IntExpr>(i)));
      }
    E(Error::DOT_NO_ATTR, expr, val->prettyString(), name);
  }
  return transform(N<DotExpr>(expr->begin()->getExpr(), name));
}

/// Transform setattr method to a AssignMemberStmt.
Expr *TypecheckVisitor::transformSetAttr(CallExpr *expr) {
  auto attr = extractFuncGeneric(expr->expr->getType())->getStrStatic()->value;
  return transform(
      N<StmtExpr>(N<AssignMemberStmt>((*expr)[0].getExpr(), attr, (*expr)[1].getExpr()),
                  N<CallExpr>(N<IdExpr>("NoneType"))));
}

/// Raise a compiler error.
Expr *TypecheckVisitor::transformCompileError(CallExpr *expr) {
  auto msg = extractFuncGeneric(expr->expr->getType())->getStrStatic()->value;
  E(Error::CUSTOM, expr, msg);
  return nullptr;
}

/// Convert a class to a tuple.
Expr *TypecheckVisitor::transformTupleFn(CallExpr *expr) {
  for (auto &a : *expr)
    a.value = transform(a.getExpr());
  auto cls = extractClassType(expr->begin()->getExpr()->getType());
  if (!cls)
    return nullptr;

  // tuple(ClassType) is a tuple type that corresponds to a class
  if (isTypeExpr(expr->begin()->getExpr())) {
    if (!realize(cls))
      return expr;

    std::vector<Expr *> items;
    auto ft = getClassFieldTypes(cls);
    for (size_t i = 0; i < ft.size(); i++) {
      auto rt = realize(ft[i].get());
      seqassert(rt, "cannot realize '{}' in {}", getClass(cls)->fields[i].name,
                cls->debugString(2));
      items.push_back(N<IdExpr>(rt->realizedName()));
    }
    auto e = transform(N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), items));
    return e;
  }

  std::vector<Expr *> args;
  std::string var = getTemporaryVar("tup");
  for (auto &field : getClassFields(cls))
    args.emplace_back(N<DotExpr>(N<IdExpr>(var), field.name));

  return transform(N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var), expr->begin()->getExpr()),
                               N<TupleExpr>(args)));
}

/// Transform type function to a type IdExpr identifier.
Expr *TypecheckVisitor::transformTypeFn(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->getExpr());
  unify(expr->getType(),
        ctx->instantiateGeneric(getStdLibType("type"),
                                {expr->begin()->getExpr()->getType()}));
  if (!realize(expr->getType()))
    return nullptr;

  auto e = N<IdExpr>(expr->getType()->realizedName());
  e->setType(expr->getType()->shared_from_this());
  e->setDone();
  return e;
}

/// Transform __realized__ function to a fully realized type identifier.
Expr *TypecheckVisitor::transformRealizedFn(CallExpr *expr) {
  auto call = cast<CallExpr>(
      transform(N<CallExpr>((*expr)[0].getExpr(), N<StarExpr>((*expr)[1].getExpr()))));
  if (!call || !call->getExpr()->getType()->getFunc())
    E(Error::CALL_REALIZED_FN, (*expr)[0].getExpr());
  if (auto f = realize(call->getExpr()->getType())) {
    auto e = N<IdExpr>(f->getFunc()->realizedName());
    e->setType(f->shared_from_this());
    e->setDone();
    return e;
  }
  return nullptr;
}

/// Transform __static_print__ function to a fully realized type identifier.
Expr *TypecheckVisitor::transformStaticPrintFn(CallExpr *expr) {
  for (auto &a : *cast<CallExpr>(expr->begin()->getExpr())) {
    realize(a.getExpr()->getType());
    fmt::print(stderr, "[static_print] {}: {} ({}){}\n", getSrcInfo(),
               a.getExpr()->getType() ? a.getExpr()->getType()->debugString(2) : "-",
               a.getExpr()->getType() ? a.getExpr()->getType()->realizedName() : "-",
               a.getExpr()->getType()->getStatic() ? " [static]" : "");
  }
  return nullptr;
}

/// Transform __has_rtti__ to a static boolean that indicates RTTI status of a type.
Expr *TypecheckVisitor::transformHasRttiFn(CallExpr *expr) {
  auto t = extractFuncGeneric(expr->getExpr()->getType())->getClass();
  if (!t)
    return nullptr;
  return transform(N<BoolExpr>(getClass(t)->hasRTTI()));
}

// Transform internal.static calls
std::pair<bool, Expr *> TypecheckVisitor::transformInternalStaticFn(CallExpr *expr) {
  auto ei = cast<IdExpr>(expr->getExpr());
  if (ei && ei->getValue() == "std.internal.static.fn_can_call.0") {
    auto typ = extractClassType((*expr)[0].getExpr());
    if (!typ)
      return {true, nullptr};

    auto inargs = unpackTupleTypes((*expr)[1].getExpr());
    auto kwargs = unpackTupleTypes((*expr)[2].getExpr());
    seqassert(inargs && kwargs, "bad call to fn_can_call");

    std::vector<CallArg> callArgs;
    for (auto &[v, t] : *inargs) {
      callArgs.emplace_back(v, N<NoneExpr>()); // dummy expression
      callArgs.back().getExpr()->setType(t->shared_from_this());
    }
    for (auto &[v, t] : *kwargs) {
      callArgs.emplace_back(v, N<NoneExpr>()); // dummy expression
      callArgs.back().getExpr()->setType(t->shared_from_this());
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
    auto fn = ctx->extractFunction(expr->begin()->getExpr()->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());
    auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto &args = fn->getArgs();
    return {true, transform(N<BoolExpr>(idx->value >= 0 && idx->value < args.size() &&
                                        args[idx->value].getType()->canRealize()))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_arg_get_type.0") {
    auto fn = ctx->extractFunction(expr->begin()->getExpr()->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());
    auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
    seqassert(idx, "expected a static integer");
    const auto &args = fn->getArgs();
    if (idx->value < 0 || idx->value >= args.size() ||
        !args[idx->value].getType()->canRealize())
      error("argument does not have type");
    return {true, transform(N<IdExpr>(args[idx->value].getType()->realizedName()))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_args.0") {
    auto fn = ctx->extractFunction(expr->begin()->value->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());
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
    auto fn = ctx->extractFunction(expr->begin()->getExpr()->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());
    auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
    seqassert(idx, "expected a static integer");
    if (idx->value < 0 || idx->value >= fn->ast->size())
      error("argument out of bounds");
    return {true,
            transform(N<BoolExpr>((*fn->ast)[idx->value].getDefault() != nullptr))};
  } else if (ei && ei->getValue() == "std.internal.static.fn_get_default.0") {
    auto fn = ctx->extractFunction(expr->begin()->getExpr()->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());
    auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
    seqassert(idx, "expected a static integer");
    if (idx->value < 0 || idx->value >= fn->ast->size())
      error("argument out of bounds");
    return {true, transform((*fn->ast)[idx->value].getDefault())};
  } else if (ei && ei->getValue() == "std.internal.static.fn_wrap_call_args.0") {
    auto typ = expr->begin()->getExpr()->getClassType();
    if (!typ)
      return {true, nullptr};

    auto fn = ctx->extractFunction(expr->begin()->getExpr()->getType());
    if (!fn)
      error("expected a function, got '{}'",
            expr->begin()->getExpr()->getType()->prettyString());

    std::vector<CallArg> callArgs;
    if (auto tup = cast<TupleExpr>((*expr)[1].getExpr()->getOrigExpr())) {
      for (auto *a : *tup) {
        callArgs.emplace_back("", a);
      }
    }
    if (auto kw = cast<CallExpr>((*expr)[1].getExpr()->getOrigExpr())) {
      auto kwCls = getClass(expr->getClassType());
      seqassert(kwCls, "cannot find {}", expr->getClassType()->name);
      for (size_t i = 0; i < kw->size(); i++) {
        callArgs.emplace_back(kwCls->fields[i].name, (*kw)[i].getExpr());
      }
    }
    auto tempCall = transform(N<CallExpr>(N<IdExpr>(fn->getFuncName()), callArgs));
    if (!tempCall->isDone())
      return {true, nullptr};

    std::vector<Expr *> tupArgs;
    for (auto &a : *cast<CallExpr>(tempCall))
      tupArgs.push_back(a.getExpr());
    return {true, transform(N<TupleExpr>(tupArgs))};
  } else if (ei && ei->getValue() == "std.internal.static.vars.0") {
    auto withIdx =
        extractFuncGeneric(expr->getExpr()->getType())->getBoolStatic()->value;

    types::ClassType *typ = nullptr;
    std::vector<Expr *> tupleItems;
    auto e = transform(expr->begin()->getExpr());
    if (!(typ = e->getClassType()))
      return {true, nullptr};

    size_t idx = 0;
    for (auto &f : getClassFields(typ)) {
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
    auto t = extractFuncGeneric(funcTyp)->getClass();
    if (!t || !realize(t))
      return {true, nullptr};
    auto n = extractFuncGeneric(funcTyp, 1)->getIntStatic()->value;
    types::TypePtr typ = nullptr;
    auto f = getClassFields(t);
    if (n < 0 || n >= f.size())
      error("invalid index");
    auto rt = realize(ctx->instantiate(f[n].getType(), t));
    return {true, transform(N<IdExpr>(rt->realizedName()))};
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
    auto parentTyp = ctx->instantiate(ctx->getType(name));
    auto parentFields = getClassFields(parentTyp->getClass());
    for (auto &field : fields) {
      for (auto &parentField : parentFields)
        if (field.name == parentField.name) {
          auto t = ctx->instantiate(field.getType(), cls);
          unify(t.get(),
                ctx->instantiate(parentField.getType(), parentTyp->getClass()));
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
  efn->setType(
      ctx->instantiateGeneric(getStdLibType("unrealized_type"), {fn->getFunc()}));
  efn->setDone();
  Expr *call = N<CallExpr>(N<IdExpr>("Partial"),
                           std::vector<CallArg>{{"args", args},
                                                {"kwargs", kwargs},
                                                {"M", N<StringExpr>(strMask)},
                                                {"F", efn}});
  call = transform(call);
  seqassert(call->getType()->is("Partial"), "expected partial type: {:c}",
            *(call->getType()));
  return call;
}

} // namespace codon::ast
