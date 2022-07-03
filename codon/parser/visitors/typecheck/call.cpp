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

/// Just ensure that this expression is not independent of CallExpr where it is handled.
void TypecheckVisitor::visit(StarExpr *) { error("cannot use star-expression"); }

/// Just ensure that this expression is not independent of CallExpr where it is handled.
void TypecheckVisitor::visit(KeywordStarExpr *) { error("cannot use star-expression"); }

/// Typechecks an ellipsis. Ellipses are typically replaced during the typechecking; the
/// only remaining ellipses are those that belong to PipeExprs.
void TypecheckVisitor::visit(EllipsisExpr *expr) {
  unify(expr->type, ctx->getUnbound());
  if (expr->isPipeArg && realize(expr->type))
    expr->setDone();
}

/// Typecheck a call expression---the most complex expression to typecheck.
/// Transform a call expression callee(args...).
/// Intercepts callees that are expr.dot, expr.dot[T1, T2] etc.
/// Before any transformation, all arguments are expanded:
///   *args -> args[0], ..., args[N]
///   **kwargs -> name0=kwargs.name0, ..., nameN=kwargs.nameN
/// Performs the following transformations:
///   Tuple(args...) -> Tuple.__new__(args...) (tuple constructor)
///   Class(args...) -> c = Class.__new__(); c.__init__(args...); c (StmtExpr)
///   Partial(args...) -> StmtExpr(p = Partial; PartialFn(args...))
///   obj(args...) -> obj.__call__(args...) (non-functions)
/// This method will also handle the following use-cases:
///   named arguments,
///   default arguments,
///   default generics, and
///   *args / **kwargs.
/// Any partial call will be transformed as follows:
///   callee(arg1, ...) -> StmtExpr(_v = Partial.callee.N10(arg1); _v).
/// If callee is partial and it is satisfied, it will be replaced with the originating
/// function call.
/// Arguments are unified with the signature types. The following exceptions are
/// supported:
///   Optional[T] -> T (via unwrap())
///   int -> float (via float.__new__())
///   T -> Optional[T] (via Optional.__new__())
///   T -> Generator[T] (via T.__iter__())
///   T -> Callable[T] (via T.__call__())
///
/// Pipe notes: if inType and extraStage are set, this method will use inType as a
/// pipe ellipsis type. extraStage will be set if an Optional conversion/unwrapping
/// stage needs to be inserted before the current pipeline stage.
///
/// Static call expressions: the following static expressions are supported:
///   isinstance(var, type) -> evaluates to bool
///   hasattr(type, string) -> evaluates to bool
///   staticlen(var) -> evaluates to int
///   compile_error(string) -> raises a compiler error
///   type(type) -> IdExpr(instantiated_type_name)
///
/// Note: This is the most evil method in the whole parser suite. 🤦🏻‍
void TypecheckVisitor::visit(CallExpr *expr) {
  // Transform and expand arguments. Return early if it cannot be done yet
  if (!transformCallArgs(expr->args))
    return;

  // Check if this call is partial call
  PartialCallData part{!expr->args.empty() && expr->args.back().value->getEllipsis() &&
                       !expr->args.back().value->getEllipsis()->isPipeArg &&
                       expr->args.back().name.empty()};

  // Transform callee
  transformCallee(expr->expr, expr->args, part);
  auto [calleeFn, newExpr] = getCalleeFn(expr, part);
  if ((resultExpr = newExpr))
    return;

  // Handle named and default arguments
  if ((resultExpr = callReorderArguments(calleeFn, expr, part)))
    return;

  // Handle special calls
  auto [isSpecial, specialExpr] = transformSpecialCall(expr);
  if (isSpecial) {
    resultExpr = specialExpr;
    return;
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
  for (size_t ai = 0; ai < args.size();) {
    if (auto star = args[ai].value->getStar()) {
      // Case: *args expansion
      transform(star->what);
      auto typ = star->what->type->getClass();
      if (!typ) // Process later
        return false;
      if (!typ->getRecord())
        error("can only unpack tuple types");
      auto &fields = ctx->cache->classes[typ->name].fields;
      for (size_t i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
        args.insert(args.begin() + ai,
                    {"", transform(N<DotExpr>(clone(star->what), fields[i].name))});
      }
      args.erase(args.begin() + ai);
    } else if (auto kwstar = CAST(args[ai].value, KeywordStarExpr)) {
      // Case: **kwargs expansion
      kwstar->what = transform(kwstar->what);
      auto typ = kwstar->what->type->getClass();
      if (!typ)
        return false;
      if (!typ->getRecord() || startswith(typ->name, TYPE_TUPLE))
        error("can only unpack named tuple types: {}", typ->toString());
      auto &fields = ctx->cache->classes[typ->name].fields;
      for (size_t i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
        args.insert(args.begin() + ai,
                    {fields[i].name,
                     transform(N<DotExpr>(clone(kwstar->what), fields[i].name))});
      }
      args.erase(args.begin() + ai);
    } else {
      // Case: normal argument (no expansion)
      transform(args[ai].value, true); // can be type as well
      ai++;
    }
  }

  // Check if some argument names are reused after the expansion
  std::set<std::string> seen;
  for (auto &a : args)
    if (!a.name.empty()) {
      if (in(seen, a.name))
        error("repeated named argument '{}'", a.name);
      seen.insert(a.name);
    }

  return true;
}

/// Transform a call expression callee.
/// TODO: handle intercept
void TypecheckVisitor::transformCallee(ExprPtr &callee,
                                       std::vector<CallExpr::Arg> &args,
                                       PartialCallData &part) {
  if (!part.isPartial) {
    // Intercept method calls (e.g. `obj.method`) for faster compilation (because it
    // avoids partial calls). This intercept passes the call arguments to transformDot
    // to select the best overload as well.
    ExprPtr *lhs = &callee;
    if (auto ei = callee->getIndex()) {
      lhs = &(ei->expr);
    } else if (auto inst = CAST(callee, InstantiateExpr)) {
      // Special case: type instantiation (`foo.bar[T]`)
      /// TODO: get rid of?
      lhs = &(inst->typeExpr);
    }
    if (auto dot = (*lhs)->getDot()) {
      // Pick the best overload!
      if (auto edt = transformDot(dot, &args))
        *lhs = edt;
    } else if (auto id = (*lhs)->getId()) {
      // If this is an overloaded identifier, pick the best overload
      auto overloads = in(ctx->cache->overloads, id->value);
      if (overloads && overloads->size() > 1) {
        if (auto bestMethod = findBestMethod(id->value, args)) {
          ExprPtr e = N<IdExpr>(bestMethod->ast->name);
          e->setType(ctx->instantiate(bestMethod));
          unify(id->type, e->type);
          *lhs = e;
        } else {
          /// TODO refactor to use getBestClassMethod
          std::vector<std::string> nice;
          for (auto &t : args)
            nice.emplace_back(format("{} = {}", t.name, t.value->type->toString()));
          error("cannot find an overload '{}' with arguments {}", id->value,
                join(nice, ", "));
        }
      }
    }
  }
  transform(callee, true); // can be type as well
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
    // `ctr = T.__new__(); std.internal.gc.register_finalizer(v); v.__init__(args)`
    ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("ctr"));
    return {nullptr,
            transform(N<StmtExpr>(
                N<SuiteStmt>(
                    N<AssignStmt>(clone(var),
                                  N<CallExpr>(N<DotExpr>(expr->expr, "__new__"))),
                    N<ExprStmt>(N<CallExpr>(
                        N<IdExpr>("std.internal.gc.register_finalizer"), clone(var))),
                    N<ExprStmt>(
                        N<CallExpr>(N<DotExpr>(clone(var), "__init__"), expr->args))),
                clone(var)))};
  }

  auto calleeFn = callee->getFunc();
  if (auto partType = callee->getPartial()) {
    // Case: calling partial object `p`. Transform roughly to
    // `part = callee; partial_fn(*part.args, args...)`
    ExprPtr var = N<IdExpr>(part.var = ctx->cache->getTemporaryVar("part"));
    expr->expr = transform(N<StmtExpr>(N<AssignStmt>(clone(var), expr->expr),
                                       N<IdExpr>(partType->func->ast->name)));

    // Ensure that we got a function
    calleeFn = expr->expr->type->getFunc();
    seqassert(calleeFn, "not a function: {}", expr->expr->type->toString());

    // Unify partial generics with types known thus far
    for (size_t i = 0, j = 0; i < partType->known.size(); i++)
      if (partType->func->ast->args[i].status == Param::Generic) {
        if (partType->known[i])
          unify(calleeFn->funcGenerics[j].type,
                ctx->instantiate(partType->func->funcGenerics[j].type));
        j++;
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

ExprPtr TypecheckVisitor::callReorderArguments(FuncTypePtr calleeFn, CallExpr *expr,
                                               PartialCallData &part) {
  std::vector<CallExpr::Arg> args; // stores ordered and processed arguments
  std::vector<ExprPtr> typeArgs;   // stores type and static arguments (e.g., `T: type`)
  auto newMask = std::vector<char>(calleeFn->ast->args.size(), 1);

  // Extract pi-th partial argument from a partial object
  auto getPartialArg = [&](int pi) {
    auto id = transform(N<IdExpr>(part.var));
    ExprPtr it = N<IntExpr>(pi);
    // Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
    auto ex = transformStaticTupleIndex(id->type->getClass(), id, it);
    seqassert(ex, "partial indexing failed: {}", id->type->debugString(true));
    return ex;
  };

  // Handle reordered arguments (see @c reorderNamedArgs for details)
  auto reorderFn = [&](int starArgIndex, int kwstarArgIndex,
                       const std::vector<std::vector<int>> &slots, bool partial) {
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
        if (!expr->expr->isId("hasattr:0"))
          e = transform(e);
        if (partial) {
          part.args = e;
          args.push_back({realName, transform(N<EllipsisExpr>())});
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
          seqassert(t && startswith(t->name, "KwTuple"), "{} not a kwtuple",
                    e->toString());
          auto &ff = ctx->cache->classes[t->name].fields;
          for (int i = 0; i < t->getRecord()->args.size(); i++) {
            names.emplace_back(ff[i].name);
            values.emplace_back(
                CallExpr::Arg{"", transform(N<DotExpr>(clone(e), ff[i].name))});
          }
        }
        for (auto &e : slots[si]) {
          names.emplace_back(expr->args[e].name);
          values.emplace_back(CallExpr::Arg{"", expr->args[e].value});
        }
        auto kwName = generateTuple(names.size(), "KwTuple", names);
        auto e = transform(N<CallExpr>(N<IdExpr>(kwName), values));
        e->setAttr(ExprAttr::KwStarArgument);
        if (partial) {
          part.kwArgs = e;
          args.push_back({realName, transform(N<EllipsisExpr>())});
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
          args.push_back({realName, transform(N<EllipsisExpr>())});
          newMask[si] = 0;
        } else {
          auto es = calleeFn->ast->args[si].defaultValue->toString();
          if (in(ctx->defaultCallDepth, es))
            error("recursive default arguments");
          ctx->defaultCallDepth.insert(es);
          args.push_back(
              {realName, transform(clone(calleeFn->ast->args[si].defaultValue))});
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
        [&](const std::string &errorMsg) {
          error("{}", errorMsg);
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
      auto kwName = generateTuple(0, "KwTuple", {});
      part.kwArgs = transform(N<CallExpr>(N<IdExpr>(kwName))); // use KwTuple()
    }
  }

  // Unify function type generics with the provided generics
  seqassert((expr->hasAttr(ExprAttr::OrderedCall) && typeArgs.empty()) ||
                (!expr->hasAttr(ExprAttr::OrderedCall) &&
                 typeArgs.size() == calleeFn->funcGenerics.size()),
            "bad vector sizes");
  for (size_t si = 0;
       !expr->hasAttr(ExprAttr::OrderedCall) && si < calleeFn->funcGenerics.size();
       si++) {
    if (typeArgs[si]) {
      auto typ = typeArgs[si]->type;
      if (calleeFn->funcGenerics[si].type->isStaticType()) {
        if (!typeArgs[si]->isStatic())
          error("expected static expression");
        typ = std::make_shared<StaticType>(typeArgs[si], ctx);
      }
      unify(typ, calleeFn->funcGenerics[si].type);
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
    return expr->expr; /// TODO: why short circuit? continue instead?
  }

  expr->args = args;
  expr->setAttr(ExprAttr::OrderedCall);
  part.known = newMask;
  return nullptr;
}

std::pair<bool, ExprPtr> TypecheckVisitor::transformSpecialCall(CallExpr *expr) {
  if (!expr->expr->getId())
    return {false, nullptr};

  auto val = expr->expr->getId()->value;
  // LOG("-- {}", val);
  if (val == "superf") {
    if (ctx->bases.back().supers.empty())
      error("no matching superf methods are available");
    auto parentCls = ctx->bases.back().type->getFunc()->funcParent;
    auto m = findMatchingMethods(parentCls ? parentCls->getClass() : nullptr,
                                 ctx->bases.back().supers, expr->args);
    if (m.empty())
      error("no matching superf methods are available");
    ExprPtr e = N<CallExpr>(N<IdExpr>(m[0]->ast->name), expr->args);
    return {true, transform(e)};
  } else if (val == "super:0") {
    auto e = transformSuper(expr);
    return {true, e};
  } else if (val == "__ptr__") {
    auto id = expr->args[0].value->getId();
    auto v = id ? ctx->find(id->value) : nullptr;
    if (v && v->kind == TypecheckItem::Var) {
      expr->args[0].value = transform(expr->args[0].value);
      auto t =
          ctx->instantiateGeneric(ctx->getType("Ptr"), {expr->args[0].value->type});
      unify(expr->type, t);
      expr->done = expr->args[0].value->done;
      return {true, nullptr};
    } else {
      error("__ptr__ only accepts a variable identifier");
    }
  } else if (val == "__array__.__new__:0") {
    auto fnt = expr->expr->type->getFunc();
    auto szt = fnt->funcGenerics[0].type->getStatic();
    if (!szt->canRealize())
      return {true, nullptr};
    auto sz = szt->evaluate().getInt();
    auto typ = fnt->funcParent->getClass()->generics[0].type;
    auto t = ctx->instantiateGeneric(ctx->getType("Array"), {typ});
    unify(expr->type, t);
    // Realize the Array[T] type of possible.
    if (auto rt = realize(expr->type)) {
      unify(expr->type, rt);
      expr->done = true;
    }
    return {true, nullptr};
  } else if (val == "isinstance") {
    // Make sure not to activate new unbound here, as we just need to check type
    // equality.
    expr->staticValue.type = StaticValue::INT;
    expr->type = unify(expr->type, ctx->getType("bool"));
    expr->args[0].value = transform(expr->args[0].value, true);
    auto typ = expr->args[0].value->type->getClass();
    if (!typ || !typ->canRealize()) {
      return {true, nullptr};
    } else {
      expr->args[0].value = transform(expr->args[0].value, true); // to realize it
      if (expr->args[1].value->isId("Tuple") || expr->args[1].value->isId("tuple")) {
        return {true, transform(N<BoolExpr>(startswith(typ->name, TYPE_TUPLE)))};
      } else if (expr->args[1].value->isId("ByVal")) {
        return {true, transform(N<BoolExpr>(typ->getRecord() != nullptr))};
      } else if (expr->args[1].value->isId("ByRef")) {
        return {true, transform(N<BoolExpr>(typ->getRecord() == nullptr))};
      } else {
        expr->args[1].value = transformType(expr->args[1].value);
        auto t = expr->args[1].value->type;
        auto hierarchy = getSuperTypes(typ->getClass());

        for (auto &tx : hierarchy) {
          auto unifyOK = tx->unify(t.get(), nullptr) >= 0;
          if (unifyOK) {
            return {true, transform(N<BoolExpr>(true))};
          }
        }
        return {true, transform(N<BoolExpr>(false))};
      }
    }
  } else if (val == "staticlen") {
    expr->staticValue.type = StaticValue::INT;
    expr->args[0].value = transform(expr->args[0].value);
    auto typ = expr->args[0].value->getType();
    if (auto s = typ->getStatic()) {
      if (s->expr->staticValue.type != StaticValue::STRING)
        error("expected a static string");
      if (!s->expr->staticValue.evaluated)
        return {true, nullptr};
      return {true, transform(N<IntExpr>(s->expr->staticValue.getString().size()))};
    }
    if (!typ->getClass())
      return {true, nullptr};
    else if (!typ->getRecord())
      error("{} is not a tuple type", typ->toString());
    else
      return {true, transform(N<IntExpr>(typ->getRecord()->args.size()))};
  } else if (startswith(val, "hasattr:")) {
    expr->staticValue.type = StaticValue::INT;
    auto typ = expr->args[0].value->getType()->getClass();
    if (!typ)
      return {true, nullptr};
    auto member = expr->expr->type->getFunc()
                      ->funcGenerics[0]
                      .type->getStatic()
                      ->evaluate()
                      .getString();
    std::vector<TypePtr> args{typ};
    if (val == "hasattr:0") {
      auto tup = expr->args[1].value->getTuple();
      seqassert(tup, "not a tuple");
      for (auto &a : tup->items) {
        a = transformType(a);
        if (!a->getType()->getClass())
          return {true, nullptr};
        args.push_back(a->getType());
      }
    }
    bool exists = !ctx->findMethod(typ->getClass()->name, member).empty() ||
                  ctx->findMember(typ->getClass()->name, member);
    if (exists && args.size() > 1)
      exists &= findBestMethod(expr->args[0].value.get(), member, args) != nullptr;
    return {true, transform(N<BoolExpr>(exists))};
  } else if (val == "compile_error") {
    auto fnt = expr->expr->type->getFunc();
    auto szt = fnt->funcGenerics[0].type->getStatic();
    if (!szt->canRealize())
      return {true, nullptr};
    error("custom error: {}", szt->evaluate().getString());
  } else if (val == "type.__new__:0") {
    expr->markType();
    expr->args[0].value = transform(expr->args[0].value);
    unify(expr->type, expr->args[0].value->getType());
    if (auto rt = realize(expr->type)) {
      unify(expr->type, rt);
      auto resultExpr = N<IdExpr>(expr->type->realizedName());
      unify(resultExpr->type, expr->type);
      resultExpr->done = true;
      resultExpr->markType();
      return {true, resultExpr};
    }
    return {true, nullptr};
  } else if (val == "getattr") {
    auto fnt = expr->expr->type->getFunc();
    auto szt = fnt->funcGenerics[0].type->getStatic();
    if (!szt->canRealize())
      return {true, nullptr};
    expr->args[0].value = transform(expr->args[0].value);
    auto e = transform(N<DotExpr>(expr->args[0].value, szt->evaluate().getString()));
    // LOG("-> {}", e->toString());
    return {true, e};
  }
  return {false, nullptr};
}
bool TypecheckVisitor::typecheckCallArgs(const FuncTypePtr &calleeFn,
                                         std::vector<CallExpr::Arg> &args) {
  // Unify the arguments with the corresponding signatures
  bool wrappingDone = true;          // tracks whether all arguments are wrapped
  std::vector<TypePtr> replacements; // list of replacement arguments
  for (size_t si = 0; si < calleeFn->getArgTypes().size(); si++) {
    if (!wrapExpr(args[si].value, calleeFn->getArgTypes()[si], calleeFn))
      wrappingDone = false;
    replacements.push_back(!calleeFn->getArgTypes()[si]->getClass()
                               ? args[si].value->type
                               : calleeFn->getArgTypes()[si]);
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
        auto def = transform(clone(calleeFn->ast->args[i].defaultValue), true);
        unify(calleeFn->funcGenerics[j].type,
              def->isStatic() ? std::make_shared<StaticType>(def, ctx)
                              : def->getType());
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

ExprPtr TypecheckVisitor::transformSuper(const CallExpr *expr) {
  // For now, we just support casting to the _FIRST_ overload (i.e. empty super())
  if (ctx->bases.empty() || !ctx->bases.back().type)
    error("no parent classes available");
  auto fptyp = ctx->bases.back().type->getFunc();
  if (!fptyp || !fptyp->ast->hasAttr(Attr::Method))
    error("no parent classes available");
  if (fptyp->getArgTypes().empty())
    error("no parent classes available");
  ClassTypePtr typ = fptyp->getArgTypes()[0]->getClass();
  auto &cands = ctx->cache->classes[typ->name].parentClasses;
  if (cands.empty())
    error("no parent classes available");

  // find parent typ
  // unify top N args with parent typ args
  // realize & do bitcast
  // call bitcast() . method

  auto name = cands[0];
  auto val = ctx->find(name);
  seqassert(val, "cannot find '{}'", name);
  auto ftyp = ctx->instantiate(val->type)->getClass();

  if (typ->getRecord()) {
    std::vector<ExprPtr> members;
    for (auto &f : ctx->cache->classes[name].fields)
      members.push_back(N<DotExpr>(N<IdExpr>(fptyp->ast->args[0].name), f.name));
    ExprPtr e = transform(
        N<CallExpr>(N<IdExpr>(format(TYPE_TUPLE "{}", members.size())), members));
    unify(e->type, ftyp);
    e->type = ftyp;
    return e;
  } else {
    for (auto &f : ctx->cache->classes[typ->name].fields) {
      for (auto &nf : ctx->cache->classes[name].fields)
        if (f.name == nf.name) {
          auto t = ctx->instantiate(f.type, typ);
          auto ft = ctx->instantiate(nf.type, ftyp);
          unify(t, ft);
        }
    }

    ExprPtr typExpr = N<IdExpr>(name);
    typExpr->setType(ftyp);
    auto self = fptyp->ast->args[0].name;
    ExprPtr e = transform(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "to_class_ptr"),
                    N<CallExpr>(N<DotExpr>(N<IdExpr>(self), "__raw__")), typExpr));
    return e;
  }
}

std::vector<ClassTypePtr> TypecheckVisitor::getSuperTypes(const ClassTypePtr &cls) {
  std::vector<ClassTypePtr> result;
  if (!cls)
    return result;
  result.push_back(cls);
  int start = 0;
  for (auto &name : ctx->cache->classes[cls->name].parentClasses) {
    auto val = ctx->find(name);
    seqassert(val, "cannot find '{}'", name);
    auto ftyp = ctx->instantiate(val->type)->getClass();
    for (auto &f : ctx->cache->classes[cls->name].fields) {
      for (auto &nf : ctx->cache->classes[name].fields)
        if (f.name == nf.name) {
          auto t = ctx->instantiate(f.type, cls);
          auto ft = ctx->instantiate(nf.type, ftyp);
          unify(t, ft);
          break;
        }
    }
    for (auto &t : getSuperTypes(ftyp))
      result.push_back(t);
  }
  return result;
}

} // namespace codon::ast