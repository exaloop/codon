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

void TypecheckVisitor::visit(CallExpr *expr) { resultExpr = transformCall(expr); }

ExprPtr TypecheckVisitor::transformCall(CallExpr *expr, const types::TypePtr &inType,
                                        ExprPtr *extraStage) {
  // keep the old expression if we end up with an extra stage
  ExprPtr oldExpr = extraStage ? expr->clone() : nullptr;

  if (!callTransformCallArgs(expr->args, inType))
    return nullptr;

  PartialCallData part{!expr->args.empty() && expr->args.back().value->getEllipsis() &&
                       !expr->args.back().value->getEllipsis()->isPipeArg &&
                       expr->args.back().name.empty()};
  expr->expr = callTransformCallee(expr->expr, expr->args, part);

  auto callee = expr->expr->getType()->getClass();
  FuncTypePtr calleeFn = callee ? callee->getFunc() : nullptr;
  if (!callee) {
    // Case 1: Unbound callee, will be resolved later.
    unify(expr->type, ctx->addUnbound(expr, ctx->typecheckLevel));
    return nullptr;
  } else if (expr->expr->isType()) {
    if (callee->getRecord()) {
      // Case 2a: Tuple constructor. Transform to: t.__new__(args)
      return transform(N<CallExpr>(N<DotExpr>(expr->expr, "__new__"), expr->args));
    } else {
      // Case 2b: Type constructor. Transform to a StmtExpr:
      //   c = t.__new__(); c.__init__(args); c
      ExprPtr var = N<IdExpr>(ctx->cache->getTemporaryVar("v"));
      return transform(N<StmtExpr>(
          N<SuiteStmt>(
              N<AssignStmt>(clone(var), N<CallExpr>(N<DotExpr>(expr->expr, "__new__"))),
              N<ExprStmt>(N<CallExpr>(N<IdExpr>("std.internal.gc.register_finalizer"),
                                      clone(var))),
              N<ExprStmt>(N<CallExpr>(N<DotExpr>(clone(var), "__init__"), expr->args))),
          clone(var)));
    }
  } else if (auto pc = callee->getPartial()) {
    ExprPtr var = N<IdExpr>(part.var = ctx->cache->getTemporaryVar("pt"));
    expr->expr = transform(N<StmtExpr>(N<AssignStmt>(clone(var), expr->expr),
                                       N<IdExpr>(pc->func->ast->name)));
    calleeFn = expr->expr->type->getFunc();
    // Fill in generics
    for (int i = 0, j = 0; i < pc->known.size(); i++)
      if (pc->func->ast->args[i].status == Param::Generic) {
        if (pc->known[i])
          unify(calleeFn->funcGenerics[j].type,
                ctx->instantiate(expr, pc->func->funcGenerics[j].type));
        j++;
      }
    part.known = pc->known;
    seqassert(calleeFn, "not a function: {}", expr->expr->type->toString());
  } else if (!callee->getFunc()) {
    // Case 3: callee is not a named function. Route it through a __call__ method.
    ExprPtr newCall = N<CallExpr>(N<DotExpr>(expr->expr, "__call__"), expr->args);
    return transform(newCall, false);
  }

  // Handle named and default arguments
  int ellipsisStage = -1;
  if (auto e = callReorderArguments(callee, calleeFn, expr, ellipsisStage, part))
    return e;

  auto special = transformSpecialCall(expr);
  if (special.first) {
    return special.second;
  }

  // Check if ellipsis is in the *args/*kwArgs
  if (extraStage && ellipsisStage != -1) {
    *extraStage = expr->args[ellipsisStage].value;
    expr->args[ellipsisStage].value = N<EllipsisExpr>();
    const_cast<CallExpr *>(oldExpr->getCall())->args = expr->args;
    const_cast<CallExpr *>(oldExpr->getCall())->ordered = true;
    return oldExpr;
  }

  bool unificationsDone = true;
  std::vector<TypePtr> replacements(calleeFn->getArgTypes().size(), nullptr);
  for (int si = 0; si < calleeFn->getArgTypes().size(); si++) {
    bool isPipeArg = extraStage && expr->args[si].value->getEllipsis();
    auto orig = expr->args[si].value.get();
    if (!wrapExpr(expr->args[si].value, calleeFn->getArgTypes()[si], calleeFn))
      unificationsDone = false;

    replacements[si] = !calleeFn->getArgTypes()[si]->getClass()
                           ? expr->args[si].value->type
                           : calleeFn->getArgTypes()[si];
    if (isPipeArg && orig != expr->args[si].value.get()) {
      *extraStage = expr->args[si].value;
      return oldExpr;
    }
  }

  // Realize arguments.
  expr->done = true;
  for (auto &a : expr->args) {
    if (auto rt = realize(a.value->type)) {
      unify(rt, a.value->type);
      a.value = transform(a.value);
    }
    expr->done &= a.value->done;
  }

  // Handle default generics (calleeFn.g. foo[S, T=int]) only if all arguments were
  // unified.
  // TODO: remove once the proper partial handling of overloaded functions land
  if (unificationsDone) {
    for (int i = 0, j = 0; i < calleeFn->ast->args.size(); i++)
      if (calleeFn->ast->args[i].status == Param::Generic) {
        if (calleeFn->ast->args[i].defaultValue &&
            calleeFn->funcGenerics[j].type->getUnbound()) {
          auto de = transform(calleeFn->ast->args[i].defaultValue, true);
          TypePtr t = nullptr;
          if (de->isStatic())
            t = std::make_shared<StaticType>(de, ctx);
          else
            t = de->getType();
          unify(calleeFn->funcGenerics[j].type, t);
        }
        j++;
      }
  }
  for (int si = 0; si < replacements.size(); si++)
    if (replacements[si]) {
      // calleeFn->generics[si + 1].type =
      calleeFn->getArgTypes()[si] = replacements[si];
    }
  if (!part.isPartial) {
    if (auto rt = realize(calleeFn)) {
      unify(rt, std::static_pointer_cast<Type>(calleeFn));
      expr->expr = transform(expr->expr);
    }
  }
  expr->done &= expr->expr->done;

  // Emit the final call.
  if (part.isPartial) {
    // Case 1: partial call.
    // Transform calleeFn(args...) to Partial.N<known>.<calleeFn>(args...).
    auto partialTypeName = generatePartialStub(part.known, calleeFn->getFunc().get());
    std::vector<ExprPtr> newArgs;
    for (auto &r : expr->args)
      if (!r.value->getEllipsis()) {
        newArgs.push_back(r.value);
        newArgs.back()->setAttr(ExprAttr::SequenceItem);
      }
    newArgs.push_back(part.args);
    newArgs.push_back(part.kwArgs);

    std::string var = ctx->cache->getTemporaryVar("partial");
    ExprPtr call = nullptr;
    if (!part.var.empty()) {
      auto stmts = const_cast<StmtExpr *>(expr->expr->getStmtExpr())->stmts;
      stmts.push_back(N<AssignStmt>(N<IdExpr>(var),
                                    N<CallExpr>(N<IdExpr>(partialTypeName), newArgs)));
      call = N<StmtExpr>(stmts, N<IdExpr>(var));
    } else {
      call =
          N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var),
                                    N<CallExpr>(N<IdExpr>(partialTypeName), newArgs)),
                      N<IdExpr>(var));
    }
    call->setAttr(ExprAttr::Partial);
    call = transform(call, false);
    seqassert(call->type->getPartial(), "expected partial type");
    return call;
  } else {
    // Case 2. Normal function call.
    unify(expr->type, calleeFn->getRetType()); // function return type
    return nullptr;
  }
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
    auto m =
        findMatchingMethods(parentCls ? CAST(parentCls, types::ClassType) : nullptr,
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
      auto t = ctx->instantiateGeneric(expr, ctx->getType("Ptr"),
                                       {expr->args[0].value->type});
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
    auto t = ctx->instantiateGeneric(expr, ctx->getType("Array"), {typ});
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

bool TypecheckVisitor::callTransformCallArgs(std::vector<CallExpr::Arg> &args,
                                             const types::TypePtr &inType) {
  for (int ai = 0; ai < args.size(); ai++) {
    if (auto es = args[ai].value->getStar()) {
      // Case 1: *arg unpacking
      es->what = transform(es->what);
      auto t = es->what->type->getClass();
      if (!t)
        return false;
      if (!t->getRecord())
        error("can only unpack tuple types");
      auto &ff = ctx->cache->classes[t->name].fields;
      for (int i = 0; i < t->getRecord()->args.size(); i++, ai++)
        args.insert(
            args.begin() + ai,
            CallExpr::Arg{"", transform(N<DotExpr>(clone(es->what), ff[i].name))});
      args.erase(args.begin() + ai);
      ai--;
    } else if (auto ek = CAST(args[ai].value, KeywordStarExpr)) {
      // Case 2: **kwarg unpacking
      ek->what = transform(ek->what);
      auto t = ek->what->type->getClass();
      if (!t)
        return false;
      if (!t->getRecord() || startswith(t->name, TYPE_TUPLE))
        error("can only unpack named tuple types: {}", t->toString());
      auto &ff = ctx->cache->classes[t->name].fields;
      for (int i = 0; i < t->getRecord()->args.size(); i++, ai++)
        args.insert(args.begin() + ai,
                    CallExpr::Arg{ff[i].name,
                                  transform(N<DotExpr>(clone(ek->what), ff[i].name))});
      args.erase(args.begin() + ai);
      ai--;
    } else {
      // Case 3: Normal argument
      args[ai].value = transform(args[ai].value, true);
      // Unbound inType might become a generator that will need to be extracted, so
      // don't unify it yet.
      if (inType && !inType->getUnbound() && args[ai].value->getEllipsis() &&
          args[ai].value->getEllipsis()->isPipeArg)
        unify(args[ai].value->type, inType);
    }
  }
  std::set<std::string> seenNames;
  for (auto &i : args)
    if (!i.name.empty()) {
      if (in(seenNames, i.name))
        error("repeated named argument '{}'", i.name);
      seenNames.insert(i.name);
    }
  return true;
}

ExprPtr TypecheckVisitor::callTransformCallee(ExprPtr &callee,
                                              std::vector<CallExpr::Arg> &args,
                                              PartialCallData &part) {
  if (!part.isPartial) {
    // Intercept dot-callees (e.g. expr.foo). Needed in order to select a proper
    // overload for magic methods and to avoid dealing with partial calls
    // (a non-intercepted object DotExpr (e.g. expr.foo) will get transformed into a
    // partial call).
    ExprPtr *lhs = &callee;
    // Make sure to check for instantiation DotExpr (e.g. a.b[T]) as well.
    if (auto ei = callee->getIndex()) {
      // A potential function instantiation
      lhs = &ei->expr;
    } else if (auto eii = CAST(callee, InstantiateExpr)) {
      // Real instantiation
      lhs = &eii->typeExpr;
    }
    if (auto ed = const_cast<DotExpr *>((*lhs)->getDot())) {
      if (auto edt = transformDot(ed, &args))
        *lhs = edt;
    } else if (auto ei = const_cast<IdExpr *>((*lhs)->getId())) {
      // check if this is an overloaded function?
      auto i = ctx->cache->overloads.find(ei->value);
      if (i != ctx->cache->overloads.end() && i->second.size() != 1) {
        if (auto bestMethod = findBestMethod(ei->value, args)) {
          ExprPtr e = N<IdExpr>(bestMethod->ast->name);
          auto t = ctx->instantiate(callee.get(), bestMethod);
          unify(e->type, t);
          unify(ei->type, e->type);
          *lhs = e;
        } else {
          std::vector<std::string> nice;
          for (auto &t : args)
            nice.emplace_back(format("{} = {}", t.name, t.value->type->toString()));
          error("cannot find an overload '{}' with arguments {}", ei->value,
                join(nice, ", "));
        }
      }
    }
  }
  return transform(callee, true);
}

ExprPtr TypecheckVisitor::callReorderArguments(ClassTypePtr callee,
                                               FuncTypePtr calleeFn, CallExpr *expr,
                                               int &ellipsisStage,
                                               PartialCallData &part) {
  std::vector<CallExpr::Arg> args;
  std::vector<ExprPtr> typeArgs;
  int typeArgCount = 0;
  auto newMask = std::vector<char>(calleeFn->ast->args.size(), 1);
  auto getPartialArg = [&](int pi) {
    auto id = transform(N<IdExpr>(part.var));
    ExprPtr it = N<IntExpr>(pi);
    // Manual call to transformStaticTupleIndex needed because otherwise
    // IndexExpr routes this to InstantiateExpr.
    auto ex = transformStaticTupleIndex(callee.get(), id, it);
    seqassert(ex, "partial indexing failed");
    return ex;
  };

  part.args = part.kwArgs = nullptr;
  if (expr->ordered || expr->expr->isId("superf"))
    args = expr->args;
  else
    ctx->reorderNamedArgs(
        calleeFn.get(), expr->args,
        [&](int starArgIndex, int kwstarArgIndex,
            const std::vector<std::vector<int>> &slots, bool partial) {
          ctx->addBlock(); // add generics for default arguments.
          addFunctionGenerics(calleeFn->getFunc().get());
          for (int si = 0, pi = 0; si < slots.size(); si++) {
            if (calleeFn->ast->args[si].status == Param::Generic) {
              typeArgs.push_back(slots[si].empty() ? nullptr
                                                   : expr->args[slots[si][0]].value);
              typeArgCount += typeArgs.back() != nullptr;
              newMask[si] = slots[si].empty() ? 0 : 1;
            } else if (si == starArgIndex) {
              std::vector<ExprPtr> extra;
              if (!part.known.empty())
                extra.push_back(N<StarExpr>(getPartialArg(-2)));
              for (auto &e : slots[si]) {
                extra.push_back(expr->args[e].value);
                if (extra.back()->getEllipsis())
                  ellipsisStage = args.size();
              }
              ExprPtr e = N<TupleExpr>(extra);
              if (!expr->expr->isId("hasattr:0"))
                e = transform(e);
              if (partial) {
                part.args = e;
                args.push_back({"", transform(N<EllipsisExpr>())});
                newMask[si] = 0;
              } else {
                args.push_back({"", e});
              }
            } else if (si == kwstarArgIndex) {
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
                if (values.back().value->getEllipsis())
                  ellipsisStage = args.size();
              }
              auto kwName = generateTuple(names.size(), "KwTuple", names);
              auto e = transform(N<CallExpr>(N<IdExpr>(kwName), values));
              if (partial) {
                part.kwArgs = e;
                args.push_back({"", transform(N<EllipsisExpr>())});
                newMask[si] = 0;
              } else {
                args.push_back({"", e});
              }
            } else if (slots[si].empty()) {
              if (!part.known.empty() && part.known[si]) {
                args.push_back({"", getPartialArg(pi++)});
              } else if (partial) {
                args.push_back({"", transform(N<EllipsisExpr>())});
                newMask[si] = 0;
              } else {
                auto es = calleeFn->ast->args[si].defaultValue->toString();
                if (in(ctx->defaultCallDepth, es))
                  error("recursive default arguments");
                ctx->defaultCallDepth.insert(es);
                args.push_back(
                    {"", transform(clone(calleeFn->ast->args[si].defaultValue))});
                ctx->defaultCallDepth.erase(es);
              }
            } else {
              seqassert(slots[si].size() == 1, "call transformation failed");
              args.push_back({"", expr->args[slots[si][0]].value});
            }
          }
          ctx->popBlock();
          return 0;
        },
        [&](const std::string &errorMsg) {
          error("{}", errorMsg);
          return -1;
        },
        part.known);
  if (part.args != nullptr)
    part.args->setAttr(ExprAttr::SequenceItem);
  if (part.kwArgs != nullptr)
    part.kwArgs->setAttr(ExprAttr::SequenceItem);
  if (part.isPartial) {
    expr->args.pop_back();
    if (!part.args)
      part.args = transform(N<TupleExpr>());
    if (!part.kwArgs) {
      auto kwName = generateTuple(0, "KwTuple", {});
      part.kwArgs = transform(N<CallExpr>(N<IdExpr>(kwName)));
    }
  }

  // Typecheck given arguments with the expected (signature) types.
  seqassert((expr->ordered && typeArgs.empty()) ||
                (!expr->ordered && typeArgs.size() == calleeFn->funcGenerics.size()),
            "bad vector sizes");
  for (int si = 0; !expr->ordered && si < calleeFn->funcGenerics.size(); si++)
    if (typeArgs[si]) {
      auto t = typeArgs[si]->type;
      if (calleeFn->funcGenerics[si].type->isStaticType()) {
        if (!typeArgs[si]->isStatic())
          error("expected static expression");
        t = std::make_shared<StaticType>(typeArgs[si], ctx);
      }
      unify(t, calleeFn->funcGenerics[si].type);
    }

  // Special case: function instantiation
  if (part.isPartial && typeArgCount && typeArgCount == expr->args.size()) {
    auto e = transform(expr->expr);
    unify(expr->type, e->getType());
    return e;
  }

  expr->args = args;
  expr->ordered = true;
  part.known = newMask;
  return nullptr;
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
  auto ftyp = ctx->instantiate(expr, val->type)->getClass();

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
          auto t = ctx->instantiate(expr, f.type, typ.get());
          auto ft = ctx->instantiate(expr, nf.type, ftyp.get());
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
    auto ftyp = ctx->instantiate(nullptr, val->type)->getClass();
    for (auto &f : ctx->cache->classes[cls->name].fields) {
      for (auto &nf : ctx->cache->classes[name].fields)
        if (f.name == nf.name) {
          auto t = ctx->instantiate(nullptr, f.type, cls.get());
          auto ft = ctx->instantiate(nullptr, nf.type, ftyp.get());
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