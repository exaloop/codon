// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <fmt/args.h>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/cir/types/types.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Generate ASTs for all internal functions that deal with vtable generation.
/// Intended to be called once the typechecking is done.
/// TODO: add JIT compatibility.

void TypecheckVisitor::prepareVTables() {
  // def RTTIType._get_thunk_id(F, T):
  //   return VID
  auto fn = getFunction(getMangledMethod("", "RTTIType", "_get_thunk_id"));
  auto oldAst = fn->ast;
  // Keep iterating as thunks can generate more thunks.
  std::unordered_set<std::string> cache;
  for (bool added = true; added;) {
    added = false;
    for (const auto &[rn, real] : fn->realizations) {
      if (in(cache, rn))
        continue;
      cache.insert(rn);
      added = true;
      fn->ast->suite = generateGetThunkIDAst(real->getType());
      real->type->ast = fn->ast;
      LOG_REALIZE("[poly] {} : {}", real->type->debugString(2), fn->ast->toString(2));
      realizeFunc(real->type.get(), true);
      fn->ast = oldAst;
    }
  }

  fn = getFunction(getMangledMethod("", "RTTIType", "_populate_vtables"));
  fn->ast->suite = generateClassPopulateVTablesAST();
  auto typ = fn->realizations.begin()->second->getType();
  typ->ast = fn->ast;
  LOG_REALIZE("[poly] {} : {}", typ->debugString(2), fn->ast->toString(2));
  realizeFunc(typ, true);

  // def RTTIType._dist(B, D):
  //   return Tuple[<types before B is reached in D>].__elemsize__
  fn = getFunction(getMangledMethod("", "RTTIType", "_dist"));
  oldAst = fn->ast;
  for (const auto &real : fn->realizations | std::views::values) {
    fn->ast->suite = generateBaseDerivedDistAST(real->getType());
    real->type->ast = fn->ast;
    LOG_REALIZE("[poly] {} : {}", real->type->debugString(2), fn->ast->toString(2));
    realizeFunc(real->type.get(), true);
  }
  fn->ast = oldAst;
}

SuiteStmt *TypecheckVisitor::generateClassPopulateVTablesAST() {
  auto suite = N<SuiteStmt>();
  for (const auto &[cls_name, cls] : ctx->cache->classes) {
    for (const auto &[r, real] : cls.realizations) {
      if (real->vtable.empty())
        continue;
      LOG_REALIZE("[poly] {} -> {}", r, real->id);
      suite->addStmt(N<ExprStmt>(N<CallExpr>(
          N<IdExpr>(getMangledMethod("", "TypeInfo", "cache")), N<IdExpr>("vtable"),
          N<IdExpr>(real->getType()->realizedName()))));

      std::vector<std::pair<std::pair<std::string, std::string>, size_t>> thunks;
      for (const auto &key : real->vtable | std::views::keys) {
        auto id = in(ctx->cache->thunkIds, key);
        seqassert(id, "key {} not found in thunkIds", key);
        thunks.emplace_back(key, *id);
      }
      std::sort(thunks.begin(), thunks.end(),
                [](const auto &a, const auto &b) { return a.second < b.second; });
      for (const auto &[key, id] : thunks) {
        auto fn = real->vtable[key];
        std::vector<Expr *> ids;
        for (const auto &t : *fn)
          ids.push_back(N<IdExpr>(t.getType()->realizedName()));
        // p[real.ID].__setitem__(f.ID, Function[<TYPE_F>](f).__raw__())
        LOG_REALIZE("[poly] vtable[{}!!{}][{}] = {}", real->getType()->realizedName(),
                    real->id, id, fn->realizedName());
        Expr *fnCall =
            N<CallExpr>(N<InstantiateExpr>(
                            N<IdExpr>(StdlibTypes::Function),
                            std::vector<Expr *>{
                                N<InstantiateExpr>(N<IdExpr>(StdlibTypes::Tuple), ids),
                                N<IdExpr>(fn->getRetType()->realizedName())}),
                        N<IdExpr>(fn->realizedName()));
        suite->addStmt(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("vtable"), "set_thunk"), N<IntExpr>(real->id),
            N<IntExpr>(int64_t(id)), N<CallExpr>(N<DotExpr>(fnCall, "__raw__")))));
      }
    }
  }
  return suite;
}

SuiteStmt *TypecheckVisitor::generateBaseDerivedDistAST(FuncType *f) {
  // Dist from Base to Derived. Assumes Derived is indeed a derived class of base.
  // Rules:
  // - Base is within Derived.
  // - Use MRO order.
  auto baseTyp = extractFuncGeneric(f, 0)->getClass();
  auto derivedTyp = extractFuncGeneric(f, 1)->getClass();

  auto derivedBases = getBaseClasses(derivedTyp);
  auto fields = getClassFields(derivedTyp);
  size_t di = 0, fi = 0;
  for (; di < derivedBases.size(); di++) {
    if (derivedBases[di]->getClass()->realizedName() == baseTyp->realizedName())
      break;
    while (fi < fields.size() &&
           fields[fi].baseClass == derivedBases[di]->getClass()->name)
      fi++;
  }
  seqassert(di < derivedBases.size(), "class {} is not a base class of {}",
            baseTyp->debugString(2), derivedTyp->debugString(2));

  if (fi == 0)
    return SuiteStmt::wrap(N<ReturnStmt>(N<IntExpr>(0)));
  Stmt *suite = N<ReturnStmt>(
      N<CallExpr>(N<IdExpr>(getMangledMethod("", "type", "_get_class_offset")),
                  N<IdExpr>(derivedTyp->realizedName()), N<IntExpr>(fi)));
  return SuiteStmt::wrap(suite);
}

FunctionStmt *TypecheckVisitor::generateThunkAST(const FuncType *fp, ClassType *base,
                                                 const ClassType *derived) {
  auto ct = instantiateType(extractClassType(derived->name), base->getClass());
  std::vector<types::Type *> args;
  for (const auto &a : *fp)
    args.push_back(a.getType());
  args[0] = ct.get();
  auto m = findBestMethod(ct->getClass(), getUnmangledName(fp->getFuncName()), args);
  if (!m) {
    // Print a nice error message
    std::vector<std::string> a;
    for (auto &t : args)
      a.emplace_back(fmt::format("{}", t->prettyString()));
    std::string argsNice = fmt::format("({})", join(a, ", "));
    E(Error::DOT_NO_ATTR_ARGS, getSrcInfo(), ct->prettyString(),
      getUnmangledName(fp->getFuncName()), argsNice);
  }

  std::vector<std::string> ns;
  for (auto &a : args)
    ns.push_back(a->realizedName());
  auto thunkName =
      fmt::format("_thunk.{}.{}.{}", base->name, fp->getFuncName(), join(ns, "."));
  if (getFunction(getMangledFunc("", thunkName, 0, 0, /* noCore */ true)))
    return nullptr;

  // Thunk contents:
  // def _thunk.<BASE>.<FN>.<ARGS>(self, <ARGS...>):
  //   return <FN>(RTTIType._cast(self, <DERIVED>), <ARGS...>)
  std::vector<Param> fnArgs;
  fnArgs.emplace_back("self", N<IdExpr>(base->realizedName()), nullptr);
  for (size_t i = 1; i < args.size(); i++)
    fnArgs.emplace_back(getUnmangledName((*fp->ast)[i].getName()),
                        N<IdExpr>(args[i]->realizedName()), nullptr);
  std::vector<Expr *> callArgs;
  callArgs.emplace_back(N<CallExpr>(N<DotExpr>(N<IdExpr>("RTTIType"), "_cast"),
                                    N<IdExpr>("self"),
                                    N<IdExpr>(derived->realizedName())));
  for (size_t i = 1; i < args.size(); i++)
    callArgs.emplace_back(N<IdExpr>(getUnmangledName((*fp->ast)[i].getName())));

  std::vector<Expr *> debugCallArgs{N<StringExpr>(base->name),
                                    N<StringExpr>(fp->getFuncName()),
                                    N<StringExpr>(join(ns, "."))};
  debugCallArgs.insert(debugCallArgs.end(), callArgs.begin(), callArgs.end());
  auto thunkAst = N<FunctionStmt>(
      thunkName, nullptr, fnArgs,
      N<SuiteStmt>(
          // For debugging
          N<ExprStmt>(
              N<CallExpr>(N<IdExpr>(getMangledMethod("", "RTTIType", "_thunk_debug")),
                          debugCallArgs)),
          N<ReturnStmt>(N<CallExpr>(N<IdExpr>(m->ast->getName()), callArgs))));
  thunkAst->setAttribute(Attr::Inline);
  return cast<FunctionStmt>(transform(thunkAst));
}

/// Generate thunks in all derived classes for a given virtual function (must be fully
/// realizable) and the corresponding base class.
/// @return unique thunk ID.
SuiteStmt *TypecheckVisitor::generateGetThunkIDAst(types::FuncType *f) {
  auto fp = extractType(extractFuncGeneric(f))->getFunc();
  auto cp = extractType(extractFuncGeneric(f, 1))->getClass();

  seqassert(cp && cp->canRealize() && fp && fp->canRealize() &&
                fp->getRetType()->canRealize(),
            "bad {}", f->debugString(2));

  // Function signature for storing thunks.
  // Needs to append function generics to realized name.
  // TODO: refactor / remove (why is this needed)?
  auto sig = [&](const types::FuncType *ft) -> std::string {
    std::vector<std::string> gs;
    for (const auto &a : *ft)
      gs.emplace_back(a.getType()->realizedName());
    gs.emplace_back("|");
    for (auto &a : ft->funcGenerics)
      if (!a.name.empty())
        gs.push_back(a.type->realizedName());
    return fmt::format("{}:{}", getUnmangledName(ft->getFuncName()), join(gs, ","));
  };

  // Set up the base class information
  auto baseCls = cp->name;
  auto fnSig = sig(fp);
  auto key = std::make_pair(baseCls, fnSig);

  // Add or extract thunk ID
  auto baseRealization = getClassRealization(cp);
  seqassert(!in(baseRealization->vtable, key), "thunk {}.{} already added", baseCls,
            fnSig);
  if (!in(ctx->cache->thunkIds, key))
    ctx->cache->thunkIds[key] = 1 + ctx->cache->thunkIds.size();
  auto vid = ctx->cache->thunkIds[key];
  baseRealization->vtable[key] =
      std::static_pointer_cast<FuncType>(fp->shared_from_this());

  // Iterate through all derived classes and instantiate the corresponding thunk
  for (const auto &[clsName, cls] : ctx->cache->classes) {
    // First check if our class descends from our base class
    // (ignore generics for now; this is just a speed-up).
    // TODO: use hashmap
    bool inMro = false;
    for (auto &m : cls.mro)
      if (m && m->is(baseCls)) {
        inMro = true;
        break;
      }
    if (!inMro || clsName == baseCls)
      continue;
    for (const auto &real : cls.realizations | std::views::values) {
      // Now check if generics match!
      inMro = false;
      for (auto &mro : real->bases) // now check realizations!
        if (mro->realizedName() == cp->realizedName()) {
          inMro = true;
          break;
        }
      if (!inMro)
        continue;
      if (auto thunkAst = generateThunkAST(fp, cp, real->getType())) {
        auto thunkFn = getFunction(thunkAst->name);
        auto ti =
            std::static_pointer_cast<FuncType>(instantiateType(thunkFn->getType()));
        auto tm = realizeFunc(ti.get(), true);
        seqassert(tm, "bad thunk {}", thunkFn->type->debugString(2));
        seqassert(!in(real->vtable, key), "thunk {}.{} already added to {}", baseCls,
                  fnSig, real->getType()->realizedName());
        real->vtable[key] = std::static_pointer_cast<FuncType>(tm->shared_from_this());
        LOG_REALIZE("[thunk]: {}->{}@{} == {}", baseCls,
                    real->getType()->realizedName(), key, vid);
      }
    }
  }
  return N<SuiteStmt>(N<ReturnStmt>(N<IntExpr>(vid)));
}

SuiteStmt *TypecheckVisitor::generateFunctionCallInternalAST(FuncType *type) {
  // Special case: Function.__call_internal__
  /// TODO: move to IR one day
  std::vector<Stmt *> items;
  items.push_back(nullptr);
  std::vector<std::string> ll;
  std::vector<std::string> lla;
  seqassert(extractFuncArgType(type, 1)->is(StdlibTypes::Tuple),
            "bad function base: {}", extractFuncArgType(type, 1)->debugString(2));
  auto as = extractFuncArgType(type, 1)->getClass()->generics.size();
  auto [_, ag] = (*type->ast)[1].getNameWithStars();
  for (int i = 0; i < as; i++) {
    ll.push_back(fmt::format("%{} = extractvalue {{}} %args, {}", i, i));
    items.push_back(N<ExprStmt>(N<IdExpr>(ag)));
  }
  items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
  for (int i = 0; i < as; i++) {
    items.push_back(N<ExprStmt>(N<IndexExpr>(N<IdExpr>(ag), N<IntExpr>(i))));
    lla.push_back(fmt::format("{{}} %{}", i));
  }
  items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
  ll.push_back(fmt::format("%{} = call {{}} %self({})", as, combine2(lla)));
  ll.push_back(fmt::format("ret {{}} %{}", as));
  items[0] = N<ExprStmt>(N<StringExpr>(combine2(ll, "\n")));
  return N<SuiteStmt>(items);
}

SuiteStmt *TypecheckVisitor::generateUnionNewAST(const FuncType *type) {
  auto unionType = type->funcParent->getUnion();
  seqassert(unionType, "expected union, got {}", *(type->funcParent));

  Stmt *suite = N<ReturnStmt>(N<CallExpr>(
      N<DotExpr>(N<IdExpr>(StdlibTypes::Union), "_new"),
      N<IdExpr>(type->ast->begin()->name), N<IdExpr>(unionType->realizedName())));
  return SuiteStmt::wrap(suite);
}

SuiteStmt *TypecheckVisitor::generateUnionTagAST(FuncType *type) {
  //   return Union._get_data(union, T0)
  auto tag = getIntLiteral(extractFuncGeneric(type));
  auto unionType = extractFuncArgType(type)->getUnion();
  auto unionTypes = unionType->getRealizationTypes();
  if (tag < 0 || tag >= unionTypes.size())
    E(Error::CUSTOM, getSrcInfo(), "bad union tag");
  auto selfVar = type->ast->begin()->name;
  auto suite = N<SuiteStmt>(N<ReturnStmt>(
      N<CallExpr>(N<IdExpr>(getMangledMethod("", "Union", "_get_data")),
                  N<IdExpr>(selfVar), N<IdExpr>(unionTypes[tag]->realizedName()))));
  return suite;
}

SuiteStmt *TypecheckVisitor::generateNamedKeysAST(FuncType *type) {
  auto n = getIntLiteral(extractFuncGeneric(type));
  if (n < 0 || n >= ctx->cache->generatedTupleNames.size())
    E(Error::CUSTOM, getSrcInfo(), "bad namedkeys index");
  std::vector<Expr *> s;
  for (auto &k : ctx->cache->generatedTupleNames[n])
    s.push_back(N<StringExpr>(k));
  auto suite = N<SuiteStmt>(N<ReturnStmt>(N<TupleExpr>(s)));
  return suite;
}

SuiteStmt *TypecheckVisitor::generateTupleMulAST(FuncType *type) {
  auto n = std::max(static_cast<int64_t>(0), getIntLiteral(extractFuncGeneric(type)));
  auto t = extractFuncArgType(type)->getClass();
  if (!t || !t->is(StdlibTypes::Tuple))
    return nullptr;
  std::vector<Expr *> exprs;
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < t->generics.size(); j++)
      exprs.push_back(
          N<IndexExpr>(N<IdExpr>(type->ast->front().getName()), N<IntExpr>(j)));
  auto suite = N<SuiteStmt>(N<ReturnStmt>(N<TupleExpr>(exprs)));
  return suite;
}

/// Generate ASTs for dynamically generated functions.
SuiteStmt *TypecheckVisitor::generateSpecialAst(types::FuncType *type) {
  // Clone the generic AST that is to be realized
  auto ast = type->ast;
  if (ast->hasAttribute(Attr::AutoGenerated) && endswith(ast->name, ".__iter__:0") &&
      isHeterogenous(extractFuncArgType(type, 0))) {
    // Special case: do not realize auto-generated heterogenous __iter__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (ast->hasAttribute(Attr::AutoGenerated) &&
             endswith(ast->name, ".__getitem__:0") &&
             isHeterogenous(extractFuncArgType(type, 0))) {
    // Special case: do not realize auto-generated heterogenous __getitem__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (startswith(ast->name, "Function.__call_internal__")) {
    return generateFunctionCallInternalAST(type);
  } else if (startswith(ast->name, "Union.__new__")) {
    return generateUnionNewAST(type);
  } else if (startswith(ast->name, getMangledMethod("", "Union", "_tag"))) {
    return generateUnionTagAST(type);
  } else if (startswith(ast->name, getMangledMethod("", "NamedTuple", "_namedkeys"))) {
    return generateNamedKeysAST(type);
  } else if (startswith(ast->name, getMangledMethod("", "__magic__", "mul"))) {
    return generateTupleMulAST(type);
  } else if (startswith(ast->name, getMangledMethod("", "TypeInfo", "_init_params"))) {
    return generateTypeInfoInitAst(type);
  } else if (startswith(ast->name, getMangledMethod("", "Super", "_dispatch"))) {
    return generateSuperDispatchAst(type);
  }
  return nullptr;
}

/// Transform named tuples.
/// @example
///   `namedtuple("NT", ["a", ("b", int)])` -> ```@tuple
///                                               class NT[T1]:
///                                                 a: T1
///                                                 b: int```
Expr *TypecheckVisitor::transformNamedTuple(CallExpr *expr) {
  // Ensure that namedtuple call is valid
  auto name = getStrLiteral(extractFuncGeneric(expr->getExpr()->getType()));
  if (expr->size() != 1)
    E(Error::CALL_NAMEDTUPLE, expr);

  // Construct the class statement
  std::vector<Param> generics, params;
  auto orig = cast<TupleExpr>(expr->front().getExpr()->getOrigExpr());
  size_t ti = 1;
  for (auto *i : *orig) {
    if (auto s = cast<StringExpr>(i)) {
      generics.emplace_back(fmt::format("T{}", ti), N<IdExpr>(StdlibTypes::Type),
                            nullptr, true);
      params.emplace_back(s->getValue(), N<IdExpr>(fmt::format("T{}", ti++)), nullptr);
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
  auto cls = N<SuiteStmt>(
      N<ClassStmt>(name, params, nullptr, std::vector<Expr *>{N<IdExpr>("tuple")}));
  if (auto err = ast::ScopingVisitor::apply(ctx->cache, cls))
    throw exc::ParserException(std::move(err));
  prependStmts->push_back(transform(cls));
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
  if (!isDispatch(func)) {
    if (auto a = func->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass)) {
      auto c = getClass(a->value);
      if (auto m = in(c->methods, getUnmangledName(func->getFuncName()))) {
        for (auto &overload : getOverloads(*m)) {
          if (isDispatch(overload))
            continue;
          if (overload == func->getFuncName())
            break;
          supers.emplace_back(getFunction(overload)->getType());
        }
      }
      std::ranges::reverse(supers);
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
  if (funcTyp->empty())
    E(Error::CALL_SUPER_PARENT, getSrcInfo());

  ClassType *typ = extractFuncArgType(funcTyp)->getClass();
  auto self = N<IdExpr>(funcTyp->ast->begin()->name);
  self->setType(typ->shared_from_this());
  auto typExpr = N<IdExpr>(typ->getClass()->name);
  typExpr->setType(instantiateTypeVar(typ->getClass()));
  return transform(
      N<CallExpr>(N<IdExpr>(getMangledMethod("", "Super", "__new__")), typExpr, self));
}

/// Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
/// the argument is a variable binding.
Expr *TypecheckVisitor::transformPtr(CallExpr *expr) {
  expr->begin()->value = transform(expr->begin()->getExpr());

  auto head = getHeadExpr(expr->begin()->getExpr());
  std::vector<std::string> members;
  for (bool last = true;; last = false) {
    auto t = extractClassType(head);
    if (!t)
      return nullptr;
    if (!last && !t->isRecord())
      E(Error::CALL_PTR_VAR, expr->begin()->getExpr());

    if (auto id = cast<IdExpr>(head)) {
      auto val = id ? ctx->find(id->getValue(), getTime()) : nullptr;
      if (!val || !val->isVar())
        E(Error::CALL_PTR_VAR, expr->begin()->getExpr());
      break;
    } else if (auto dot = cast<DotExpr>(head)) {
      head = dot->getExpr();
    } else {
      E(Error::CALL_PTR_VAR, expr->begin()->getExpr());
      break;
    }
  }

  unify(expr->getType(), instantiateType(getStdLibType(StdlibTypes::Ptr),
                                         {expr->begin()->getExpr()->getType()}));
  if (expr->begin()->getExpr()->isDone())
    expr->setDone();
  return nullptr;
}

/// Typecheck __array__ method. This method creates a stack-allocated array via alloca.
Expr *TypecheckVisitor::transformArray(CallExpr *expr) {
  auto arrTyp = expr->expr->getType()->getFunc();
  unify(expr->getType(),
        instantiateType(getStdLibType(StdlibTypes::Array),
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
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  expr->begin()->value = transform(expr->begin()->getExpr());
  auto typ = expr->begin()->getExpr()->getClassType();
  if (!typ || !typ->canRealize())
    return nullptr;

  expr->begin()->value = transform(expr->begin()->getExpr()); // again to realize it

  typ = extractClassType(typ);
  auto &typExpr = (*expr)[1].value;
  if (cast<CallExpr>(typExpr)) {
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
  if (tei && tei->getValue() == "type") {
    return transform(N<BoolExpr>(isTypeExpr(expr->begin()->value)));
  } else if (tei && tei->getValue() == "type[Tuple]") {
    return transform(N<BoolExpr>(typ->is(StdlibTypes::Tuple)));
  } else if (tei && tei->getValue() == "type[ByVal]") {
    return transform(N<BoolExpr>(typ->isRecord()));
  } else if (tei && tei->getValue() == "type[ByRef]") {
    return transform(N<BoolExpr>(!typ->isRecord()));
  } else if (!extractType(typExpr)->getUnion() && typ->getUnion()) {
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    int tag = -1;
    for (size_t ui = 0; ui < unionTypes.size(); ui++) {
      if (extractType(typExpr)->unify(unionTypes[ui], nullptr) >= 0) {
        tag = static_cast<int>(ui);
        break;
      }
    }
    if (tag == -1)
      return transform(N<BoolExpr>(false));
    return transform(
        N<BinaryExpr>(N<CallExpr>(N<DotExpr>(N<IdExpr>(StdlibTypes::Union), "_get_tag"),
                                  expr->begin()->getExpr()),
                      "==", N<IntExpr>(tag)));
  } else if (typExpr->getType()->is("pyobj")) {
    if (typ->is("pyobj")) {
      return transform(
          N<CallExpr>(N<IdExpr>(getMangledFunc("std.internal.python", "_isinstance")),
                      expr->begin()->getExpr(), (*expr)[1].getExpr()));
    } else {
      return transform(N<BoolExpr>(false));
    }
  }

  typExpr = transformType(typExpr);
  auto targetType = extractType(typExpr);

  // Check type match
  types::Type::Unification us;
  auto s = typ->unify(targetType, &us);
  us.undo();
  if (s >= 0)
    return transform(N<BoolExpr>(true));

  std::string instCall = ctx->expectedType && ctx->expectedType->is(StdlibTypes::Bool)
                             ? "_getinstance"
                             : "_isinstance";
  if (typ->is(StdlibTypes::Any) && !isTypeExpr(expr->begin()->value)) {
    return transform(N<CallExpr>(N<IdExpr>(getMangledMethod("", "Any", instCall)),
                                 expr->begin()->getExpr(), (*expr)[1].getExpr()));
  }

  if (getClass(targetType->getClass())->hasRTTI() &&
      getClass(typ->getClass())->hasRTTI()) {
    // Check RTTI super types
    for (auto &tx : getMRO(typ->getClass())) {
      types::Type::Unification us;
      auto s = tx->unify(targetType, &us);
      us.undo();
      if (s >= 0)
        return transform(N<BoolExpr>(true));
    }

    // TODO: disallow all impossible cases that are not related to any MRO!
    return transform(N<CallExpr>(N<IdExpr>(getMangledMethod("", "RTTIType", instCall)),
                                 expr->begin()->getExpr(), (*expr)[1].getExpr()));
  }

  return transform(N<BoolExpr>(false));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
Expr *TypecheckVisitor::transformStaticLen(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Int;

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
Expr *TypecheckVisitor::transformHasAttr(CallExpr *expr, bool allow_dynamic) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto typ = extractClassType((*expr)[0].getExpr());
  typ = typ->is(StdlibTypes::TypeWrap) ? extractClassGeneric(typ)->getClass() : typ;
  if (!typ)
    return nullptr;

  auto attr = getStrLiteral(extractFuncGeneric(expr->getExpr()->getType()));
  std::vector<std::pair<std::string, types::Type *>> args{{"", typ}};

  if (auto tup = cast<CallExpr>((*expr)[1].getExpr())) {
    for (auto &a : *tup) {
      a.value = transform(a.getExpr());
      if (!a.getExpr()->getClassType())
        return nullptr;
      auto t = extractType(a);
      args.emplace_back("", t->is(StdlibTypes::TypeWrap) ? extractClassGeneric(t) : t);
    }
  }
  for (auto &[n, ne] : extractNamedTuple((*expr)[2].getExpr())) {
    ne = transform(ne);
    auto t = extractType(ne);
    args.emplace_back(n, t->is(StdlibTypes::TypeWrap) ? extractClassGeneric(t) : t);
  }

  if (typ->getUnion() && allow_dynamic) {
    Expr *cond = nullptr;
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    for (auto &unionType : unionTypes) {
      auto tu = realize(unionType);
      if (!tu)
        return nullptr;
      auto te = N<IdExpr>(tu->getClass()->realizedName());
      auto e = N<BinaryExpr>(
          N<CallExpr>(N<IdExpr>("isinstance"), (*expr)[0].getExpr(), te), "&&",
          N<CallExpr>(N<IdExpr>("hasattr"), te, N<StringExpr>(attr)));
      cond = !cond ? e : N<BinaryExpr>(cond, "||", e);
    }
    if (!cond)
      return transform(N<BoolExpr>(false));
    return transform(cond);
  } else if (typ->is(StdlibTypes::NamedTuple)) {
    if (!typ->canRealize())
      return nullptr;
    auto id = getIntLiteral(typ);
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    const auto &names = ctx->cache->generatedTupleNames[id];
    return transform(N<BoolExpr>(in(names, attr)));
  }

  bool exists =
      !findMethod(typ->getClass(), attr).empty() || findMember(typ->getClass(), attr);
  if (exists && args.size() > 1) {
    exists &= findBestMethod(typ, attr, args) != nullptr;
  }

  if (!exists && allow_dynamic && getClass(typ)->hasRTTI()) {
    return transform(
        N<CallExpr>(N<IdExpr>(getMangledMethod("", "RTTIType", "_hasattr")),
                    expr->begin()->getExpr(), N<StringExpr>(attr)));
  }
  return transform(N<BoolExpr>(exists));
}

/// Transform getattr method to a DotExpr.
Expr *TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto attr = getStrLiteral(extractFuncGeneric(expr->expr->getType()));
  auto attrType = extractType(extractFuncGeneric(expr->expr->getType(), 1));
  auto [newExpr, found] = getAttr(expr->begin()->getExpr(), attr);

  auto typ = extractClassType((*expr)[0].getExpr());
  typ = typ->is(StdlibTypes::TypeWrap) ? extractClassGeneric(typ)->getClass() : typ;
  if (!found) {
    if (!typ)
      return nullptr;
    if (getClass(typ)->hasRTTI()) {
      return transform(
          N<CallExpr>(N<IdExpr>(getMangledMethod("", "RTTIType", "_getattr")),
                      expr->begin()->getExpr(), N<StringExpr>(attr),
                      N<IdExpr>(attrType->realizedName())));
    } else {
      E(Error::DOT_NO_ATTR, expr, extractType(expr->begin()->getExpr())->prettyString(),
        attr);
    }
  } else {
    if (!newExpr)
      newExpr = transform(N<DotExpr>(expr->begin()->getExpr(), attr));
    if (!attrType->is(StdlibTypes::NoneType)) {
      if (wrapExpr(&newExpr, attrType))
        unify(newExpr->getType(), attrType);
    }
    return newExpr;
  }
  return nullptr;
}

/// Transform setattr method to a AssignMemberStmt.
Expr *TypecheckVisitor::transformSetAttr(CallExpr *expr) {
  auto attr = getStrLiteral(extractFuncGeneric(expr->expr->getType()));
  auto typ = extractClassType((*expr)[0].getExpr());
  typ = typ->is(StdlibTypes::TypeWrap) ? extractClassGeneric(typ)->getClass() : typ;
  if (!typ)
    return nullptr;
  if (getClass(typ)->hasRTTI()) {
    auto [_, found] = getAttr(expr->begin()->getExpr(), attr);
    if (!found) {
      return transform(N<CallExpr>(
          N<IdExpr>(getMangledMethod("", "RTTIType", "_setattr")),
          expr->begin()->getExpr(), N<StringExpr>(attr), (*expr)[1].getExpr()));
    }
  }
  return transform(
      N<StmtExpr>(N<AssignMemberStmt>((*expr)[0].getExpr(), attr, (*expr)[1].getExpr()),
                  N<CallExpr>(N<IdExpr>(StdlibTypes::NoneType))));
}

/// Raise a compiler error.
Expr *TypecheckVisitor::transformCompileError(CallExpr *expr) const {
  auto msg = getStrLiteral(extractFuncGeneric(expr->expr->getType()));
  E(Error::CUSTOM, expr, msg.c_str());
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
    auto e = transform(N<InstantiateExpr>(N<IdExpr>(StdlibTypes::Tuple), items));
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
  unify(expr->getType(), instantiateTypeVar(expr->begin()->getExpr()->getType()));
  if (!realize(expr->getType()))
    return nullptr;

  auto e = N<IdExpr>(expr->getType()->realizedName());
  e->setType(expr->getType()->shared_from_this());
  e->setDone();
  return e;
}

/// Transform static.realized function to a fully realized type identifier.
Expr *TypecheckVisitor::transformRealizedFn(CallExpr *expr) {
  auto fn = extractType((*expr)[0].getExpr()->getType())->shared_from_this();
  if (auto fns = fn->getStrStatic()) {
    // First argument can just be a literal string of function canonical name
    auto val = ctx->find(fns->value);
    if (val && val->isFunc())
      fn = instantiateType(val->getType());
  } else {
    auto pt = (*expr)[0].getExpr()->getType()->getPartial();
    if (!fn->getFunc() && pt && pt->isPartialEmpty()) {
      auto pft = pt->getPartialFunc()->generalize(0);
      fn = instantiateType(pft.get());
    }
  }
  if (!fn->getFunc())
    E(Error::CALL_REALIZED_FN, (*expr)[0].getExpr());
  auto argt = (*expr)[1].getExpr()->getType()->getClass();
  if (!argt)
    return nullptr;
  seqassert(argt->name == StdlibTypes::Tuple, "not a tuple");
  for (size_t i = 0; i < std::min(argt->size(), fn->getFunc()->size()); i++) {
    auto at = (*argt)[i]->is(StdlibTypes::TypeWrap) ? extractClassGeneric((*argt)[i])
                                                    : (*argt)[i];
    unify((*fn->getFunc())[i], at);
  }
  if (auto f = realize(fn.get())) {
    auto e = N<IdExpr>(f->getFunc()->realizedName());
    e->setType(f->shared_from_this());
    e->setDone();
    return e;
  }
  return nullptr;
}

/// Transform __static_print__ function to a fully realized type identifier.
Expr *TypecheckVisitor::transformStaticPrintFn(CallExpr *expr) const {
  for (auto &a : *cast<CallExpr>(expr->begin()->getExpr())) {
    fmt::print(stderr, "[print] {}: {} ({}){}\n", getSrcInfo(),
               a.getExpr()->getType() ? a.getExpr()->getType()->debugString(2) : "-",
               a.getExpr()->getType() ? a.getExpr()->getType()->realizedName() : "-",
               a.getExpr()->getType()->getStatic() ? " [static]" : "");
  }
  return nullptr;
}

/// Transform static.has_rtti to a static boolean that indicates RTTI status of a type.
Expr *TypecheckVisitor::transformHasRttiFn(const CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto t = extractFuncGeneric(expr->getExpr()->getType())->getClass();
  if (!t)
    return nullptr;
  return transform(N<BoolExpr>(getClass(t)->hasRTTI()));
}

// Transform internal.static calls
Expr *TypecheckVisitor::transformStaticFnCanCall(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto typ = extractClassType((*expr)[0].getExpr());
  if (!typ)
    return nullptr;

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
    // log("=> {} / {} / {}", fn->debugString(2), callArgs, canCall(fn, callArgs));
    return transform(N<BoolExpr>(canCall(fn, callArgs) >= 0));
  } else if (auto pt = typ->getPartial()) {
    return transform(N<BoolExpr>(canCall(pt->getPartialFunc(), callArgs, pt) >= 0));
  } else {
    compilationWarning("cannot use fn_can_call on non-functions", getSrcInfo().file,
                       getSrcInfo().line, getSrcInfo().col);
    return transform(N<BoolExpr>(false));
  }
}

Expr *TypecheckVisitor::transformStaticFnArgHasType(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto fn = extractFunction(expr->begin()->getExpr()->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
      expr->begin()->getExpr()->getType()->prettyString());
  auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
  seqassert(idx, "expected a static integer");
  return transform(N<BoolExpr>(idx->value >= 0 && idx->value < fn->size() &&
                               (*fn)[idx->value]->canRealize()));
}

Expr *TypecheckVisitor::transformStaticFnArgGetType(CallExpr *expr) {
  auto fn = extractFunction(expr->begin()->getExpr()->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
      expr->begin()->getExpr()->getType()->prettyString());
  auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
  seqassert(idx, "expected a static integer");
  if (idx->value < 0 || idx->value >= fn->size() || !(*fn)[idx->value]->canRealize())
    E(Error::CUSTOM, getSrcInfo(), "argument does not have type");
  return transform(N<IdExpr>((*fn)[idx->value]->realizedName()));
}

Expr *TypecheckVisitor::transformStaticFnArgs(CallExpr *expr) {
  auto fn = extractFunction(expr->begin()->value->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
      expr->begin()->getExpr()->getType()->prettyString());
  std::vector<Expr *> v;
  v.reserve(fn->ast->size());
  for (const auto &a : *fn->ast) {
    auto [_, n] = a.getNameWithStars();
    n = getUnmangledName(n);
    v.push_back(N<StringExpr>(n));
  }
  return transform(N<TupleExpr>(v));
}

Expr *TypecheckVisitor::transformStaticFnHasDefault(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto fn = extractFunction(expr->begin()->getExpr()->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
      expr->begin()->getExpr()->getType()->prettyString());
  auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
  seqassert(idx, "expected a static integer");
  if (idx->value < 0 || idx->value >= fn->ast->size())
    E(Error::CUSTOM, getSrcInfo(), "argument out of bounds");
  return transform(N<BoolExpr>((*fn->ast)[idx->value].getDefault() != nullptr));
}

Expr *TypecheckVisitor::transformStaticFnGetDefault(CallExpr *expr) {
  auto fn = extractFunction(expr->begin()->getExpr()->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
      expr->begin()->getExpr()->getType()->prettyString());
  auto idx = extractFuncGeneric(expr->getExpr()->getType())->getIntStatic();
  seqassert(idx, "expected a static integer");
  if (idx->value < 0 || idx->value >= fn->ast->size())
    E(Error::CUSTOM, getSrcInfo(), "argument out of bounds");
  return transform((*fn->ast)[idx->value].getDefault());
}

Expr *TypecheckVisitor::transformStaticFnWrapCallArgs(CallExpr *expr) {
  auto typ = expr->begin()->getExpr()->getClassType();
  if (!typ)
    return nullptr;

  auto fn = extractFunction(expr->begin()->getExpr()->getType());
  if (!fn)
    E(Error::CUSTOM, getSrcInfo(), "expected a function, got '{}'",
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
    return nullptr;

  std::vector<Expr *> tupArgs;
  for (auto &a : *cast<CallExpr>(tempCall))
    tupArgs.push_back(a.getExpr());
  return transform(N<TupleExpr>(tupArgs));
}

Expr *TypecheckVisitor::transformStaticVars(CallExpr *expr) {
  auto t = extractFuncGeneric(expr->getExpr()->getType());
  if (!t || !t->getClass())
    return nullptr;
  auto withIdx = getBoolLiteral(t);

  types::ClassType *typ = nullptr;
  std::vector<Expr *> tupleItems;
  auto e = transform(expr->begin()->getExpr());
  if (!((typ = e->getClassType())))
    return nullptr;

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
  return transform(N<TupleExpr>(tupleItems));
}

Expr *TypecheckVisitor::transformStaticChildren(CallExpr *expr) {
  auto t = extractFuncGeneric(expr->getExpr()->getType());
  if (!t || !t->getClass())
    return nullptr;

  std::vector<Expr *> tupleItems;
  auto typ = t->getClass();
  for (auto &n : getClass(typ)->descendants) {
    if (n == typ->name)
      continue;
    for (auto &[cn, cr] : getClass(n)->realizations) {
      for (auto &b : cr->bases) {
        if (b->realizedName() == typ->realizedName()) {
          tupleItems.push_back(N<IdExpr>(cr->type->realizedName()));
          break;
        }
      }
    }
  }
  return transform(N<TupleExpr>(tupleItems));
}

Expr *TypecheckVisitor::transformStaticTupleType(const CallExpr *expr) {
  auto funcTyp = expr->getExpr()->getType()->getFunc();
  auto t = extractFuncGeneric(funcTyp)->getClass();
  if (!t || !realize(t))
    return nullptr;
  auto n = getIntLiteral(extractFuncGeneric(funcTyp, 1));
  types::TypePtr typ = nullptr;
  auto f = getClassFields(t);
  if (n < 0 || n >= f.size())
    E(Error::CUSTOM, getSrcInfo(), "invalid index");
  auto rt = realize(instantiateType(f[n].getType(), t));
  return transform(N<IdExpr>(rt->realizedName()));
}

/// Transform staticlen method to a static integer expression. This method supports only
/// static strings and tuple types.
Expr *TypecheckVisitor::transformStaticFormat(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::String;

  auto funcTyp = expr->getExpr()->getType()->getFunc();
  auto fmt = getStrLiteral(extractFuncGeneric(funcTyp, 0));
  auto arg = getStrLiteral(extractFuncGeneric(funcTyp, 1));
  size_t start = 0;
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  while ((start = fmt.find("%%", start)) != std::string::npos) {
    fmt.replace(start, 2, "{}");
    store.push_back(arg);
    start += 2;
  }
  return transform(N<StringExpr>(fmt::vformat(fmt, store)));
}

/// Transform int() method to a static string expression.
Expr *TypecheckVisitor::transformStaticIntToStr(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::String;

  auto funcTyp = expr->getExpr()->getType()->getFunc();
  auto val = getIntLiteral(extractFuncGeneric(funcTyp, 0));
  return transform(N<StringExpr>(std::to_string(val)));
}

SuiteStmt *TypecheckVisitor::generateTypeInfoInitAst(FuncType *type) {
  auto t = extractFuncGeneric(type)->getClass();
  if (!t || !t->canRealize())
    return nullptr;

  auto suite = N<SuiteStmt>();
  // Add extra initialization here!
  suite->addStmt(N<AssignStmt>(N<DotExpr>(N<IdExpr>("self"), "_base_name"),
                               N<StringExpr>(t->name)));
  if (!t->is(StdlibTypes::UnrealizedType)) {
    for (auto &g : t->generics) {
      auto tp = g.getType()->shared_from_this();
      if (tp->getStatic())
        tp = tp->getStatic()->getNonStaticType()->shared_from_this();
      if (tp->getFunc()) {
        tp = std::make_shared<ClassType>(realize(tp.get())->getClass());
      }
      suite->addStmt(N<ExprStmt>(N<CallExpr>(
          N<IdExpr>(getMangledMethod("", "TypeInfo", "cache")),
          std::vector<CallArg>{CallArg{"vtable", N<IdExpr>("vt")},
                               CallArg{"T", N<IdExpr>(tp->realizedName())}})));
      suite->addStmt(N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(N<DotExpr>(N<IdExpr>("self"), "_params"), "append"),
                      N<IntExpr>(getClassRealization(tp.get())->id))));
    }
  }
  size_t fi = 0;
  for (auto &[fn, ft] : getClassRealization(t)->fields) {
    auto tp = ft.get();
    auto stat = tp->getStatic();
    if (stat)
      tp = stat->getNonStaticType();
    suite->addStmt(N<ExprStmt>(N<CallExpr>(
        N<IdExpr>(getMangledMethod("", "TypeInfo", "cache")),
        std::vector<CallArg>{CallArg{"vtable", N<IdExpr>("vt")},
                             CallArg{"T", N<IdExpr>(tp->realizedName())}})));
    suite->addStmt(N<ExprStmt>(N<CallExpr>(
        N<DotExpr>(N<DotExpr>(N<IdExpr>("self"), "_fields"), "append"),
        N<TupleExpr>(std::vector<Expr *>{
            N<StringExpr>(fn), N<IntExpr>(getClassRealization(tp)->id),
            N<CallExpr>(N<IdExpr>(getMangledMethod("", "type", "_get_class_offset")),
                        N<IdExpr>(t->realizedName()), N<IntExpr>(fi++))}))));
  }
  return suite;
}

SuiteStmt *TypecheckVisitor::generateSuperDispatchAst(FuncType *type) {
  auto attr = extractFuncGeneric(type)->getStrStatic();
  if (!attr)
    return nullptr;

  auto superTyp = extractFuncArgType(type)->getClass();
  auto typ = extractClassGeneric(superTyp)->getClass();
  auto suite = clone(getFunction(type->getFuncName())->ast->getSuite());

  seqassert(extractFuncArgType(type, 1)->is(StdlibTypes::Tuple) &&
                extractFuncArgType(type, 2)->is(StdlibTypes::NamedTuple),
            "invalid arguments");
  std::vector<CallArg> callArgs;
  callArgs.emplace_back("", N<NoneExpr>());
  for (auto &g : extractFuncArgType(type, 1)->getClass()->generics) {
    callArgs.emplace_back("", N<NoneExpr>());
    callArgs.back().getExpr()->setType(g.getType()->shared_from_this());
  }
  auto id = getIntLiteral(extractFuncArgType(type, 2));
  const auto &names = ctx->cache->generatedTupleNames[id];
  auto kwt = extractClassGeneric(extractFuncArgType(type, 2), 1)->getClass();
  for (size_t gi = 0; gi < kwt->generics.size(); gi++) {
    callArgs.emplace_back(names[gi], N<NoneExpr>());
    callArgs.back().getExpr()->setType(kwt->generics[gi].getType()->shared_from_this());
  }

  std::unordered_map<std::string, TypePtr> nextMro;
  for (auto &n : getClass(typ)->descendants) {
    auto nc = getClass(n);
    size_t i = 0;
    for (; i < nc->mro.size() - 1; i++) {
      if (nc->mro[i]->name == typ->name) {
        break;
      }
    }
    i++;
    if (i < nc->mro.size()) {
      auto tb = instantiateType(nc->mro[i].get(), typ);
      if (!tb->canRealize()) {
        W(Error::CUSTOM, getSrcInfo(),
          "cannot realize superclass {} of {}; ignoring its super",
          nc->mro[i]->prettyString(), typ->prettyString());
        continue;
      }
      realize(tb);
      auto methods = findMethod(tb->getClass(), attr->value, false);
      callArgs[0].getExpr()->setType(tb);
      for (auto &bm : findMatchingMethods(tb->getClass(), methods, callArgs)) {
        auto a = bm->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass);
        if (a && a->value == tb->getClass()->name) {
          nextMro[nc->mro[i]->getClass()->name] = tb;
          break;
        }
      }
    }
  }
  for (auto &tb : nextMro | std::views::values) {
    Stmt *ret = N<ReturnStmt>(N<CallExpr>(
        N<DotExpr>(N<IdExpr>(tb->getClass()->name), attr->value),
        N<CallExpr>(N<IdExpr>(getMangledMethod("", "RTTIType", "_cast")),
                    N<DotExpr>(N<IdExpr>("self"), "_obj"),
                    N<IdExpr>(tb->realizedName())),
        N<StarExpr>(N<IdExpr>("args")), N<KeywordStarExpr>(N<IdExpr>("kwargs"))));
    suite->addStmt(
        nextMro.size() == 1
            ? ret
            : N<IfStmt>(N<BinaryExpr>(N<IdExpr>("base"), "==",
                                      N<IntExpr>(getClassRealization(tb.get())->id)),
                        ret));
  }
  return suite;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticTupleLoop(Expr *iter,
                                          const std::vector<std::string> &vars) {
  std::vector<Stmt *> block;
  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  auto call = cast<CallExpr>(cast<CallExpr>(iter)->front());
  if (vars.size() != 1)
    E(Error::CUSTOM, getSrcInfo(), "expected one item");
  for (auto &a : *call) {
    stmt->rhs = transform(clean_clone(a.value));
    if (auto st = stmt->rhs->getType()->getStatic()) {
      stmt->type = N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>(st->name));
    } else {
      stmt->type = nullptr;
    }
    block.push_back(clone(stmt));
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateSimpleStaticRangeLoop(Expr *iter,
                                                const std::vector<std::string> &vars) {
  if (vars.size() != 1)
    E(Error::CUSTOM, getSrcInfo(), "expected one item");
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  std::vector<Stmt *> block;
  auto ed = getIntLiteral(extractFuncGeneric(fn->getType()));
  if (ed > MAX_STATIC_ITER)
    E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER, ed);
  for (int64_t i = 0; i < ed; i++) {
    stmt->rhs = N<IntExpr>(i);
    stmt->type = N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"));
    block.push_back(clone(stmt));
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticRangeLoop(Expr *iter,
                                          const std::vector<std::string> &vars) {
  if (vars.size() != 1)
    E(Error::CUSTOM, getSrcInfo(), "expected one item");
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  std::vector<Stmt *> block;
  auto st = getIntLiteral(extractFuncGeneric(fn->getType(), 0));
  auto ed = getIntLiteral(extractFuncGeneric(fn->getType(), 1));
  auto step = getIntLiteral(extractFuncGeneric(fn->getType(), 2));
  if (std::abs(st - ed) / std::abs(step) > MAX_STATIC_ITER)
    E(Error::STATIC_RANGE_BOUNDS, fn, MAX_STATIC_ITER,
      std::abs(st - ed) / std::abs(step));
  for (int64_t i = st; step > 0 ? i < ed : i > ed; i += step) {
    stmt->rhs = N<IntExpr>(i);
    stmt->type = N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"));
    block.push_back(clone(stmt));
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticFnOverloadsLoop(Expr *iter,
                                                const std::vector<std::string> &vars) {
  if (vars.size() != 1)
    E(Error::CUSTOM, getSrcInfo(), "expected one item");
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  auto stmt = N<AssignStmt>(N<IdExpr>(vars[0]), nullptr, nullptr);
  std::vector<Stmt *> block;
  auto typ = extractFuncGeneric(fn->getType(), 0)->getClass();
  seqassert(extractFuncGeneric(fn->getType(), 1)->getStrStatic(), "bad static string");
  auto name = getStrLiteral(extractFuncGeneric(fn->getType(), 1));

  std::vector<std::string> overloads;
  if (typ->is(StdlibTypes::NoneType)) {
    if (auto func = ctx->cache->typeCtx->find(name)) {
      auto root = getRootName(func->getType()->getFunc());
      overloads = getOverloads(root);
    }
  } else {
    if (auto n = in(getClass(typ)->methods, name))
      overloads = getOverloads(*n);
  }
  if (!overloads.empty()) {
    for (int mti = static_cast<int>(overloads.size()) - 1; mti >= 0; mti--) {
      auto &method = overloads[mti];
      auto cfn = getFunction(method);
      if (isDispatch(method) || !cfn->type)
        continue;
      if (isHeterogenous(typ)) {
        if (cfn->ast->hasAttribute(Attr::AutoGenerated) &&
            (endswith(cfn->ast->name, ".__iter__:0") ||
             endswith(cfn->ast->name, ".__getitem__:0"))) {
          // ignore __getitem__ and other heterogenuous methods
          continue;
        }
      }
      stmt->rhs = N<IdExpr>(method);
      block.push_back(clone(stmt));
    }
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticEnumerateLoop(Expr *iter,
                                              const std::vector<std::string> &vars) {
  if (vars.size() != 2)
    E(Error::CUSTOM, getSrcInfo(), "expected two items");
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  std::vector<Stmt *> block;
  auto typ = extractFuncArgType(fn->getType())->getClass();
  if (typ && typ->isRecord()) {
    for (size_t i = 0; i < getClassFields(typ).size(); i++) {
      auto b = N<SuiteStmt>(std::vector<Stmt *>{
          N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                        N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"))),
          N<AssignStmt>(
              N<IdExpr>(vars[1]),
              N<IndexExpr>(clone((*cast<CallExpr>(iter))[0].value), N<IntExpr>(i)))});
      block.push_back(b);
    }
  } else {
    E(Error::CUSTOM, getSrcInfo(), "static.enumerate needs a tuple");
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticVarsLoop(Expr *iter,
                                         const std::vector<std::string> &vars) {
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  bool withIdx = getBoolLiteral(extractFuncGeneric(fn->getType()));
  if (!withIdx && vars.size() != 2)
    E(Error::CUSTOM, getSrcInfo(), "expected two items");
  else if (withIdx && vars.size() != 3)
    E(Error::CUSTOM, getSrcInfo(), "expected three items");
  std::vector<Stmt *> block;
  auto typ = extractFuncArgType(fn->getType())->getClass();
  size_t idx = 0;
  if (typ->is(StdlibTypes::TypeWrap)) { // type passed!
    for (auto &f : getClass(extractClassGeneric(typ))->classVars) {
      std::vector<Stmt *> stmts;
      if (withIdx) {
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                          N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"))));
      }
      stmts.push_back(
          N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<StringExpr>(f.first),
                        N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("str"))));
      stmts.push_back(N<AssignStmt>(N<IdExpr>(vars[withIdx + 1]), N<IdExpr>(f.second)));
      auto b = N<SuiteStmt>(stmts);
      block.push_back(b);
      idx++;
    }
  } else {
    for (auto &f : getClassFields(typ)) {
      std::vector<Stmt *> stmts;
      if (withIdx) {
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                          N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"))));
      }
      stmts.push_back(
          N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<StringExpr>(f.name),
                        N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("str"))));
      stmts.push_back(
          N<AssignStmt>(N<IdExpr>(vars[withIdx + 1]),
                        N<DotExpr>(clone((*cast<CallExpr>(iter))[0].value), f.name)));
      auto b = N<SuiteStmt>(stmts);
      block.push_back(b);
      idx++;
    }
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticVarTypesLoop(Expr *iter,
                                             const std::vector<std::string> &vars) {
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  auto typ = realize(extractFuncGeneric(fn->getType(), 0)->getClass());
  bool withIdx = getBoolLiteral(extractFuncGeneric(fn->getType(), 1));
  if (!withIdx && vars.size() != 1)
    E(Error::CUSTOM, getSrcInfo(), "expected one item");
  else if (withIdx && vars.size() != 2)
    E(Error::CUSTOM, getSrcInfo(), "expected two items");

  seqassert(typ, "vars_types expects a realizable type, got '{}' instead",
            *(extractFuncGeneric(fn->getType(), 0)));
  std::vector<Stmt *> block;
  if (auto utyp = typ->getUnion()) {
    for (size_t i = 0; i < utyp->getRealizationTypes().size(); i++) {
      std::vector<Stmt *> stmts;
      if (withIdx) {
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(i),
                          N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"))));
      }
      stmts.push_back(
          N<AssignStmt>(N<IdExpr>(vars[1]),
                        N<IdExpr>(utyp->getRealizationTypes()[i]->realizedName())));
      auto b = N<SuiteStmt>(stmts);
      block.push_back(b);
    }
  } else {
    size_t idx = 0;
    for (auto &f : getClassFields(typ->getClass())) {
      auto ta = realize(instantiateType(f.type.get(), typ->getClass()));
      seqassert(ta, "cannot realize '{}'", f.type->debugString(2));
      std::vector<Stmt *> stmts;
      if (withIdx) {
        stmts.push_back(
            N<AssignStmt>(N<IdExpr>(vars[0]), N<IntExpr>(idx),
                          N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("int"))));
      }
      stmts.push_back(
          N<AssignStmt>(N<IdExpr>(vars[withIdx]), N<IdExpr>(ta->realizedName())));
      auto b = N<SuiteStmt>(stmts);
      block.push_back(b);
      idx++;
    }
  }
  return block;
}

std::vector<Stmt *>
TypecheckVisitor::populateStaticMethodsLoop(Expr *iter,
                                            const std::vector<std::string> &vars) {
  auto fn =
      cast<CallExpr>(iter) ? cast<IdExpr>(cast<CallExpr>(iter)->getExpr()) : nullptr;
  auto typ = realize(extractFuncGeneric(fn->getType(), 0)->getClass());
  seqassert(typ, "methods expects a realizable type, got '{}' instead",
            *(extractFuncGeneric(fn->getType(), 0)));
  if (typ->is(StdlibTypes::TypeWrap))
    typ = extractClassGeneric(typ)->getClass();
  std::vector<Stmt *> block;
  size_t idx = 0;
  for (auto &m : getClass(typ->getClass())->methods | std::views::keys) {
    auto b = N<SuiteStmt>(
        N<AssignStmt>(N<IdExpr>(vars[0]), N<StringExpr>(m),
                      N<IndexExpr>(N<IdExpr>("Literal"), N<IdExpr>("str"))));
    block.push_back(b);
  }
  return block;
}

std::vector<Stmt *> TypecheckVisitor::populateStaticHeterogenousTupleLoop(
    Expr *iter, const std::vector<std::string> &vars) {
  std::vector<Stmt *> block;
  std::string tupleVar;
  Stmt *preamble = nullptr;
  if (!cast<IdExpr>(iter)) {
    tupleVar = getTemporaryVar("tuple");
    preamble = N<AssignStmt>(N<IdExpr>(tupleVar), iter);
  } else {
    tupleVar = cast<IdExpr>(iter)->getValue();
  }
  for (size_t i = 0; i < iter->getClassType()->generics.size(); i++) {
    auto s = N<SuiteStmt>();
    if (vars.size() > 1) {
      for (size_t j = 0; j < vars.size(); j++) {
        s->addStmt(
            N<AssignStmt>(N<IdExpr>(vars[j]),
                          N<IndexExpr>(N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(i)),
                                       N<IntExpr>(j))));
      }
    } else {
      s->addStmt(N<AssignStmt>(N<IdExpr>(vars[0]),
                               N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(i))));
    }
    block.push_back(s);
  }
  block.push_back(preamble);
  return block;
}

} // namespace codon::ast
