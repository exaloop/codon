// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

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
  if (ctx->simpleTypes)
    E(Error::CALL_NO_TYPE, expr);
  if (match(expr->getExpr(), M<IdExpr>("tuple")) && expr->size() == 1) {
    expr->setAttribute(Attr::TupleCall);
  }

  validateCall(expr);

  // Check if this call is partial call
  PartialCallData part;
  if (!expr->empty()) {
    if (auto el = cast<EllipsisExpr>(expr->back().getExpr()); el && el->isPartial()) {
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
    auto id = cast<IdExpr>(getHeadExpr(expr->getExpr()));
    if (id && part.var.empty()) {
      // Case: function overloads (IdExpr)
      // Make sure to ignore partial constructs (they are also StmtExpr(IdExpr, ...))
      std::vector<types::FuncType *> methods;
      auto key = id->getValue();
      if (isDispatch(key))
        key = key.substr(0, key.size() - std::string(FN_DISPATCH_SUFFIX).size());
      for (auto &ovs : getOverloads(key)) {
        if (!isDispatch(ovs))
          methods.push_back(getFunction(ovs)->getType());
      }
      std::ranges::reverse(methods);
      m = std::make_unique<std::vector<FuncType *>>(findMatchingMethods(
          calleeFn->funcParent ? calleeFn->funcParent->getClass() : nullptr, methods,
          expr->items, expr->getExpr()->getType()->getPartial()));
    }
    // partials have dangling ellipsis that messes up with the unbound check below
    bool doDispatch = !m || m->empty() || part.isPartial;
    if (!doDispatch && m && m->size() > 1) {
      for (auto &a : *expr) {
        if (isUnbound(a.getExpr()))
          return; // typecheck this later once we know the argument
      }
    }
    if (!doDispatch) {
      calleeFn = instantiateType(m->front(), calleeFn->funcParent
                                                 ? calleeFn->funcParent->getClass()
                                                 : nullptr);
      auto e = N<IdExpr>(calleeFn->getFuncName());
      e->setType(calleeFn);
      if (cast<IdExpr>(expr->getExpr())) {
        expr->expr = e;
      } else if (cast<StmtExpr>(expr->getExpr())) {
        // Side effect...
        for (auto *se = cast<StmtExpr>(expr->getExpr());;) {
          if (auto ne = cast<StmtExpr>(se->getExpr())) {
            se = ne;
          } else {
            se->expr = e;
            break;
          }
        }
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
      if (auto a =
              calleeFn->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass))
        name = fmt::format("{}.{}", getUserFacingName(a->value), name);
      E(Error::FN_NO_ATTR_ARGS, expr, name, argsNice);
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
  bool done = typecheckCallArgs(calleeFn.get(), expr->items, part);
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
    foundEllipsis |= static_cast<bool>(cast<EllipsisExpr>(a.value));
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
        star->expr =
            transform(N<CallExpr>(N<IdExpr>(FN_OPTIONAL_UNWRAP), star->getExpr()));
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
            CallArg{"", transform(N<DotExpr>(clone(star->getExpr()), fields[i].name))});
      }
      expr->items.erase(expr->items.begin() + ai);
    } else if (const auto kwstar = cast<KeywordStarExpr>((*expr)[ai].getExpr())) {
      // Case: **kwargs expansion
      kwstar->expr = transform(kwstar->getExpr());
      auto typ = kwstar->getExpr()->getClassType();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        kwstar->expr =
            transform(N<CallExpr>(N<IdExpr>(FN_OPTIONAL_UNWRAP), kwstar->getExpr()));
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
                                           fmt::format("item{}", i + 1)))});
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
    if (!partType->isPartialEmpty() || std::ranges::any_of(mask, [](char c) {
          return c != ClassType::PartialFlag::Missing;
        })) {
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
      } else if (mask[i] == ClassType::PartialFlag::Included) {
        unify(extractFuncArgType(calleeFn.get(), i - j),
              extractClassGeneric(knownArgTypes, k));
        k++;
      } else if (mask[i] == ClassType::PartialFlag::Default) {
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

  bool inOrder = true;
  std::vector<std::pair<size_t, Expr **>> ordered;

  std::vector<CallArg> args;    // stores ordered and processed arguments
  std::vector<Expr *> typeArgs; // stores type and static arguments (e.g., `T: type`)
  int64_t starIdx = -1;         // for *args
  std::vector<Expr *> starArgs;
  int64_t kwStarIdx = -1; // for **kwargs
  std::vector<std::string> kwStarNames;
  std::vector<Expr *> kwStarArgs;
  auto newMask = std::string(calleeFn->ast->size(), ClassType::PartialFlag::Included);

  // Extract pi-th partial argument from a partial object
  auto getPartialArg = [&](size_t pi) {
    auto id = transform(N<DotExpr>(N<IdExpr>(part.var), "args"));
    // Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
    auto ex = transformStaticTupleIndex(id->getClassType(), id, N<IntExpr>(pi));
    seqassert(ex.first && ex.second, "partial indexing failed: {}", *(id->getType()));
    return ex.second;
  };

  auto addReordered = [&](size_t i) -> bool {
    Expr **e = &((*expr)[i].value);
    if (hasSideEffect(*e)) {
      if (!ordered.empty() && i < ordered.back().first)
        inOrder = false;
      ordered.emplace_back(i, e);
      return true;
    }
    return false;
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
                  if (!part.known.empty() &&
                      part.known[si] == ClassType::PartialFlag::Included) {
                    auto t = N<IdExpr>(realName);
                    t->setType(
                        calleeFn->funcGenerics[gi].getType()->shared_from_this());
                    typeArgs.emplace_back(t);
                  } else {
                    typeArgs.emplace_back(transform(N<IdExpr>(realName.substr(1))));
                  }
                } else {
                  typeArgs.emplace_back((*expr)[slots[si][0]].getExpr());
                  if (addReordered(slots[si][0]))
                    inOrder = false; // type arguments always need preprocessing
                }
                newMask[si] = ClassType::PartialFlag::Included;
              } else if (slots[si].empty()) {
                typeArgs.push_back(nullptr);
                newMask[si] = ClassType::PartialFlag::Missing;
              } else {
                typeArgs.push_back((*expr)[slots[si][0]].getExpr());
                newMask[si] = ClassType::PartialFlag::Included;
                if (addReordered(slots[si][0]))
                  inOrder = false; // type arguments always need preprocessing
              }
              gi++;
            } else if (si == starArgIndex &&
                       !(slots[si].size() == 1 &&
                         (*expr)[slots[si][0]].getExpr()->hasAttribute(
                             Attr::ExprStarArgument))) {
              // Case: *args. Build the tuple that holds them all
              if (!part.known.empty()) {
                starArgs.push_back(N<StarExpr>(getPartialArg(-1)));
              }
              for (auto &e : slots[si]) {
                starArgs.push_back((*expr)[e].getExpr());
                addReordered(e);
              }
              starIdx = static_cast<int>(args.size());
              args.emplace_back(realName, nullptr);
              if (partial)
                newMask[si] = ClassType::PartialFlag::Missing;
            } else if (si == kwstarArgIndex &&
                       !(slots[si].size() == 1 &&
                         (*expr)[slots[si][0]].getExpr()->hasAttribute(
                             Attr::ExprKwStarArgument))) {
              // Case: **kwargs. Build the named tuple that holds them all
              std::unordered_set<std::string> newNames;
              for (auto &e : slots[si]) // kwargs names can be overriden later
                newNames.insert((*expr)[e].getName());
              if (!part.known.empty()) {
                auto e = transform(N<DotExpr>(N<IdExpr>(part.var), "kwargs"));
                for (auto &[n, ne] : extractNamedTuple(e)) {
                  if (!in(newNames, n)) {
                    newNames.insert(n);
                    kwStarNames.emplace_back(n);
                    kwStarArgs.emplace_back(transform(ne));
                  }
                }
              }
              for (auto &e : slots[si]) {
                kwStarNames.emplace_back((*expr)[e].getName());
                kwStarArgs.emplace_back((*expr)[e].getExpr());
                addReordered(e);
              }

              kwStarIdx = static_cast<int>(args.size());
              args.emplace_back(realName, nullptr);
              if (partial)
                newMask[si] = ClassType::PartialFlag::Missing;
            } else if (slots[si].empty()) {
              // Case: no arguments provided.
              if (!part.known.empty() &&
                  part.known[si] == ClassType::PartialFlag::Included) {
                // Case 1: Argument captured by partial
                args.emplace_back(realName, getPartialArg(pi));
                pi++;
              } else if (startswith(realName, "$")) {
                // Case 3: Local name capture
                bool added = false;
                if (partial) {
                  if (auto val = ctx->find(realName.substr(1))) {
                    if (val->isFunc() && val->getType()->getFunc()->ast->getName() ==
                                             calleeFn->ast->getName()) {
                      // Special case: fn(fn=fn)
                      // Delay this one.
                      args.emplace_back(
                          realName, transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
                      newMask[si] = ClassType::PartialFlag::Missing;
                      added = true;
                    }
                  }
                }
                if (!added)
                  args.emplace_back(realName, transform(N<IdExpr>(realName.substr(1))));
              } else if ((*calleeFn->ast)[si].getDefault()) {
                // Case 4: default is present
                if (auto ai = cast<IdExpr>((*calleeFn->ast)[si].getDefault())) {
                  // Case 4a: non-values (Ids / .default names)
                  if (!part.known.empty() &&
                      part.known[si] == ClassType::PartialFlag::Default) {
                    // Case 4a/1: Default already captured by partial.
                    args.emplace_back(realName, getPartialArg(pi));
                    pi++;
                  } else {
                    // TODO: check if the value is toplevel and avoid capturing it if so
                    auto e = transform(N<IdExpr>(ai->getValue()));
                    seqassert(e->getType()->getLink(), "not a link type");
                    args.emplace_back(realName, e);
                  }
                  if (partial)
                    newMask[si] = ClassType::PartialFlag::Default;
                } else if (!partial) {
                  // Case 4b: values / non-Id defaults (None, etc.)
                  if (cast<NoneExpr>((*calleeFn->ast)[si].getDefault()) &&
                      !(*calleeFn->ast)[si].type) {
                    args.emplace_back(
                        realName, transform(N<CallExpr>(N<InstantiateExpr>(
                                      N<IdExpr>("Optional"), N<IdExpr>("NoneType")))));
                  } else {
                    args.emplace_back(
                        realName,
                        transform(clean_clone((*calleeFn->ast)[si].getDefault())));
                  }
                } else {
                  args.emplace_back(realName,
                                    transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
                  newMask[si] = ClassType::PartialFlag::Missing;
                }
              } else if (partial) {
                // Case 5: this is partial call. Just add ... for missing arguments
                args.emplace_back(realName,
                                  transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
                newMask[si] = ClassType::PartialFlag::Missing;
              } else {
                seqassert(expr, "cannot happen");
              }
            } else {
              // Case: argument provided
              seqassert(slots[si].size() == 1, "call transformation failed");

              args.emplace_back(realName, (*expr)[slots[si][0]].getExpr());
              addReordered(slots[si][0]);
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

  // Do reordering
  if (!inOrder) {
    std::vector<Stmt *> prepends;
    std::ranges::sort(ordered,
                      [](const auto &a, const auto &b) { return a.first < b.first; });
    for (auto &eptr : ordered | std::views::values) {
      auto name = getTemporaryVar("call");
      auto front = transform(
          N<AssignStmt>(N<IdExpr>(name), *eptr, getParamType((*eptr)->getType())));
      auto swap = transform(N<IdExpr>(name));
      *eptr = swap;
      prepends.emplace_back(front);
    }
    return transform(N<StmtExpr>(prepends, expr));
  }

  // Handle *args
  if (starIdx != -1) {
    Expr *se = N<TupleExpr>(starArgs);
    se->setAttribute(Attr::ExprStarArgument);
    if (!match(expr->getExpr(), M<IdExpr>("hasattr")))
      se = transform(se);
    if (partial) {
      part.args = se;
      args[starIdx].value = transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL));
    } else {
      args[starIdx].value = se;
    }
  }

  // Handle **kwargs
  if (kwStarIdx != -1) {
    auto kwid = generateKwId(kwStarNames);
    auto kwe = transform(N<CallExpr>(N<IdExpr>("NamedTuple"), N<TupleExpr>(kwStarArgs),
                                     N<IntExpr>(kwid)));
    kwe->setAttribute(Attr::ExprKwStarArgument);
    if (partial) {
      part.kwArgs = kwe;
      args[kwStarIdx] = transform(N<EllipsisExpr>(EllipsisExpr::PARTIAL));
    } else {
      args[kwStarIdx] = kwe;
    }
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
        if (gen.staticKind && !typ->getStaticKind()) {
          E(Error::EXPECTED_STATIC, typeArgs[si]);
        }
        unify(typ, gen.getType());
      } else {
        if (isUnbound(gen.getType()) && !(*calleeFn->ast)[si].getDefault() &&
            !partial && in(niGenerics, gen.name)) {
          E(Error::CUSTOM, getSrcInfo(), "generic '{}' not provided",
            getUnmangledName(gen.name));
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
                                         const PartialCallData &partial) {
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
          } else if (partial.isPartial && !partial.known.empty() &&
                     partial.known[si] == ClassType::PartialFlag::Default) {
            // Defaults should not be unified (yet)!
            replacements.push_back(extractFuncArgType(calleeFn, si));
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
  if (!partial.isPartial)
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

  auto ei = cast<IdExpr>(expr->getExpr());
  if (!ei)
    return {false, nullptr};
  auto isF = [](const IdExpr *val, const std::string &module, const std::string &cls,
                const std::string &name = "") {
    if (name.empty())
      return val->getValue() == getMangledFunc(module, cls);
    else
      return val->getValue() == getMangledMethod(module, cls, name);
  };
  if (isF(ei, "std.internal.core", "superf")) {
    return {true, transformSuperF(expr)};
  } else if (isF(ei, "std.internal.core", "super")) {
    return {true, transformSuper()};
  } else if (isF(ei, "std.internal.core", "__ptr__")) {
    return {true, transformPtr(expr)};
  } else if (isF(ei, "std.internal.core", "__array__", "__new__")) {
    return {true, transformArray(expr)};
  } else if (isF(ei, "std.internal.core", "isinstance")) { // static
    return {true, transformIsInstance(expr)};
  } else if (isF(ei, "std.internal.static", "len")) { // static
    return {true, transformStaticLen(expr)};
  } else if (isF(ei, "std.internal.core", "hasattr")) { // static
    return {true, transformHasAttr(expr)};
  } else if (isF(ei, "std.internal.core", "getattr")) {
    return {true, transformGetAttr(expr)};
  } else if (isF(ei, "std.internal.core", "setattr")) {
    return {true, transformSetAttr(expr)};
  } else if (isF(ei, "std.internal.core", "type", "__new__")) {
    return {true, transformTypeFn(expr)};
  } else if (isF(ei, "std.internal.core", "compile_error")) {
    return {true, transformCompileError(expr)};
  } else if (isF(ei, "std.internal.static", "print")) {
    return {false, transformStaticPrintFn(expr)};
  } else if (isF(ei, "std.collections", "namedtuple")) {
    return {true, transformNamedTuple(expr)};
  } else if (isF(ei, "std.functools", "partial")) {
    return {true, transformFunctoolsPartial(expr)};
  } else if (isF(ei, "std.internal.static", "has_rtti")) { // static
    return {true, transformHasRttiFn(expr)};
  } else if (isF(ei, "std.internal.static", "function", "realized")) {
    return {true, transformRealizedFn(expr)};
  } else if (isF(ei, "std.internal.static", "function", "can_call")) { // static
    return {true, transformStaticFnCanCall(expr)};
  } else if (isF(ei, "std.internal.static", "function", "has_type")) { // static
    return {true, transformStaticFnArgHasType(expr)};
  } else if (isF(ei, "std.internal.static", "function", "get_type")) {
    return {true, transformStaticFnArgGetType(expr)};
  } else if (isF(ei, "std.internal.static", "function", "args")) {
    return {true, transformStaticFnArgs(expr)};
  } else if (isF(ei, "std.internal.static", "function", "has_default")) { // static
    return {true, transformStaticFnHasDefault(expr)};
  } else if (isF(ei, "std.internal.static", "function", "get_default")) {
    return {true, transformStaticFnGetDefault(expr)};
  } else if (isF(ei, "std.internal.static", "function", "wrap_args")) {
    return {true, transformStaticFnWrapCallArgs(expr)};
  } else if (isF(ei, "std.internal.static", "vars")) {
    return {true, transformStaticVars(expr)};
  } else if (isF(ei, "std.internal.static", "tuple_type")) {
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
Expr *TypecheckVisitor::generatePartialCall(const std::string &mask,
                                            types::FuncType *fn, Expr *args,
                                            Expr *kwargs) {
  if (!args)
    args = N<TupleExpr>(std::vector<Expr *>{N<TupleExpr>()});
  if (!kwargs)
    kwargs = N<CallExpr>(N<IdExpr>("NamedTuple"));

  auto efn = N<IdExpr>(fn->getFuncName());
  efn->setType(instantiateType(getStdLibType("unrealized_type"),
                               std::vector<types::Type *>{fn->getFunc()}));
  efn->setDone();
  Expr *call = N<CallExpr>(
      N<IdExpr>("Partial"),
      std::vector<CallArg>{CallArg{"args", args}, CallArg{"kwargs", kwargs},
                           CallArg{"M", N<StringExpr>(mask)}, CallArg{"F", efn}});
  return call;
}

} // namespace codon::ast
