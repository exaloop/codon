// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

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

/// Generate ASTs for all __internal__ functions that deal with vtable generation.
/// Intended to be called once the typechecking is done.
/// TODO: add JIT compatibility.

void TypecheckVisitor::prepareVTables() {
  // def class_get_thunk_id(F, T):
  //   return VID
  auto fn = getFunction(
      getMangledMethod("std.internal.core", "__internal__", "class_get_thunk_id"));
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

  fn = getFunction(
      getMangledMethod("std.internal.core", "__internal__", "class_populate_vtables"));
  fn->ast->suite = generateClassPopulateVTablesAST();
  auto typ = fn->realizations.begin()->second->getType();
  typ->ast = fn->ast;
  LOG_REALIZE("[poly] {} : {}", typ->debugString(2), fn->ast->toString(2));
  realizeFunc(typ, true);

  // def class_base_derived_dist(B, D):
  //   return Tuple[<types before B is reached in D>].__elemsize__
  fn = getFunction(
      getMangledMethod("std.internal.core", "__internal__", "class_base_derived_dist"));
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
  for (const auto &cls : ctx->cache->classes | std::views::values) {
    for (const auto &[r, real] : cls.realizations) {
      if (real->vtable.empty())
        continue;
      // __internal__.class_set_rtti_vtable(real.ID, size, real.type)
      suite->addStmt(N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_set_rtti_vtable"),
                      N<IntExpr>(real->id), N<IntExpr>(ctx->cache->thunkIds.size() + 2),
                      N<IdExpr>(r))));
      LOG_REALIZE("[poly] {} -> {}", r, real->id);
      for (const auto &[key, fn] : real->vtable) {
        auto id = in(ctx->cache->thunkIds, key);
        seqassert(id, "key {} not found in thunkIds", key);
        std::vector<Expr *> ids;
        for (const auto &t : *fn)
          ids.push_back(N<IdExpr>(t.getType()->realizedName()));
        // p[real.ID].__setitem__(f.ID, Function[<TYPE_F>](f).__raw__())
        LOG_REALIZE("[poly] vtable[{}!!{}][{}] = {}", real->getType()->realizedName(),
                    real->id, *id, fn->realizedName());
        Expr *fnCall = N<CallExpr>(
            N<InstantiateExpr>(
                N<IdExpr>("Function"),
                std::vector<Expr *>{N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), ids),
                                    N<IdExpr>(fn->getRetType()->realizedName())}),
            N<IdExpr>(fn->realizedName()));
        suite->addStmt(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(N<IdExpr>("__internal__"), "class_set_rtti_vtable_fn"),
            N<IntExpr>(real->id), N<IntExpr>(int64_t(*id)),
            N<CallExpr>(N<DotExpr>(fnCall, "__raw__")), N<IdExpr>(r))));
      }
    }
  }
  return suite;
}

SuiteStmt *TypecheckVisitor::generateBaseDerivedDistAST(FuncType *f) {
  auto baseTyp = extractFuncGeneric(f, 0)->getClass();
  size_t baseTypFields = 0;
  for (auto &fld : getClassFields(baseTyp)) {
    if (fld.baseClass == baseTyp->name) {
      baseTypFields++;
    }
  }

  auto derivedTyp = extractFuncGeneric(f, 1)->getClass();
  auto fields = getClassFields(derivedTyp);
  auto types = std::vector<Expr *>{};
  auto found = false;
  for (auto &fld : fields) {
    if (fld.baseClass == baseTyp->name) {
      found = true;
      break;
    } else {
      auto ft = realize(instantiateType(fld.getType(), derivedTyp));
      types.push_back(N<IdExpr>(ft->realizedName()));
    }
  }
  seqassert(found || !baseTypFields, "cannot find distance between {} and {}",
            derivedTyp->name, baseTyp->name);
  Stmt *suite = N<ReturnStmt>(
      N<DotExpr>(N<InstantiateExpr>(N<IdExpr>(TYPE_TUPLE), types), "__elemsize__"));
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
  if (getFunction(getMangledFunc("", thunkName)))
    return nullptr;

  // Thunk contents:
  // def _thunk.<BASE>.<FN>.<ARGS>(self, <ARGS...>):
  //   return <FN>(
  //     __internal__.class_base_to_derived(self, <BASE>, <DERIVED>),
  //     <ARGS...>)
  std::vector<Param> fnArgs;
  fnArgs.emplace_back("self", N<IdExpr>(base->realizedName()), nullptr);
  for (size_t i = 1; i < args.size(); i++)
    fnArgs.emplace_back(getUnmangledName((*fp->ast)[i].getName()),
                        N<IdExpr>(args[i]->realizedName()), nullptr);
  std::vector<Expr *> callArgs;
  callArgs.emplace_back(N<CallExpr>(
      N<DotExpr>(N<IdExpr>("__internal__"), "class_base_to_derived"), N<IdExpr>("self"),
      N<IdExpr>(base->realizedName()), N<IdExpr>(derived->realizedName())));
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
          N<ExprStmt>(N<CallExpr>(
              N<IdExpr>(getMangledMethod("std.internal.core", "__internal__",
                                         "class_thunk_debug")),
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

  // TODO: ugly, ugly; surely needs refactoring

  // Function signature for storing thunks
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
    bool inMro = false;
    for (auto &m : cls.mro)
      if (m && m->is(baseCls)) {
        inMro = true;
        break;
      }
    if (inMro && clsName != baseCls) {
      for (const auto &real : cls.realizations | std::views::values) {
        if (auto thunkAst = generateThunkAST(fp, cp, real->getType())) {
          auto thunkFn = getFunction(thunkAst->name);
          auto ti =
              std::static_pointer_cast<FuncType>(instantiateType(thunkFn->getType()));
          auto tm = realizeFunc(ti.get(), true);
          seqassert(tm, "bad thunk {}", thunkFn->type->debugString(2));
          seqassert(!in(real->vtable, key), "thunk {}.{} already added to {}", baseCls,
                    fnSig, real->getType()->realizedName());
          real->vtable[key] =
              std::static_pointer_cast<FuncType>(tm->shared_from_this());
          LOG_REALIZE("[thunk]: {}->{}@{} == {}", baseCls,
                      real->getType()->realizedName(), key, vid);
        }
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
  seqassert(extractFuncArgType(type, 1)->is(TYPE_TUPLE), "bad function base: {}",
            extractFuncArgType(type, 1)->debugString(2));
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
      N<DotExpr>(N<IdExpr>("__internal__"), "new_union"),
      N<IdExpr>(type->ast->begin()->name), N<IdExpr>(unionType->realizedName())));
  return SuiteStmt::wrap(suite);
}

SuiteStmt *TypecheckVisitor::generateUnionTagAST(FuncType *type) {
  //   return __internal__.union_get_data(union, T0)
  auto tag = getIntLiteral(extractFuncGeneric(type));
  auto unionType = extractFuncArgType(type)->getUnion();
  auto unionTypes = unionType->getRealizationTypes();
  if (tag < 0 || tag >= unionTypes.size())
    E(Error::CUSTOM, getSrcInfo(), "bad union tag");
  auto selfVar = type->ast->begin()->name;
  auto suite = N<SuiteStmt>(N<ReturnStmt>(
      N<CallExpr>(N<IdExpr>(getMangledMethod("std.internal.core", "__internal__",
                                             "union_get_data")),
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
  if (!t || !t->is(TYPE_TUPLE))
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
  if (ast->hasAttribute(Attr::AutoGenerated) && endswith(ast->name, ".__iter__") &&
      extractFuncArgType(type, 0)->getHeterogenousTuple()) {
    // Special case: do not realize auto-generated heterogenous __iter__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (ast->hasAttribute(Attr::AutoGenerated) &&
             endswith(ast->name, ".__getitem__") &&
             extractFuncArgType(type, 0)->getHeterogenousTuple()) {
    // Special case: do not realize auto-generated heterogenous __getitem__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (startswith(ast->name, "Function.__call_internal__")) {
    return generateFunctionCallInternalAST(type);
  } else if (startswith(ast->name, "Union.__new__")) {
    return generateUnionNewAST(type);
  } else if (startswith(ast->name, getMangledMethod("std.internal.core", "__internal__",
                                                    "get_union_tag"))) {
    return generateUnionTagAST(type);
  } else if (startswith(ast->name, getMangledMethod("std.internal.core", "__internal__",
                                                    "namedkeys"))) {
    return generateNamedKeysAST(type);
  } else if (startswith(ast->name,
                        getMangledMethod("std.internal.core", "__magic__", "mul"))) {
    return generateTupleMulAST(type);
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
      generics.emplace_back(fmt::format("T{}", ti), N<IdExpr>(TYPE_TYPE), nullptr,
                            true);
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
  auto cls = getClass(typ);
  auto cands = cls->staticParentClasses;
  if (cands.empty()) {
    // Dynamic inheritance: use MRO
    // TODO: maybe super() should be split into two separate functions...
    const auto &vCands = cls->mro;
    if (vCands.size() < 2)
      E(Error::CALL_SUPER_PARENT, getSrcInfo());

    auto superTyp = instantiateType(vCands[1].get(), typ);
    auto self = N<IdExpr>(funcTyp->ast->begin()->name);
    self->setType(typ->shared_from_this());

    auto typExpr = N<IdExpr>(superTyp->getClass()->name);
    typExpr->setType(instantiateTypeVar(superTyp->getClass()));
    return transform(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_super"),
                                 self, typExpr, N<IntExpr>(1)));
  }

  const auto &name = cands.front(); // the first inherited type
  auto superTyp = instantiateType(extractClassType(name), typ);
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

  unify(expr->getType(),
        instantiateType(getStdLibType("Ptr"), {expr->begin()->getExpr()->getType()}));
  if (expr->begin()->getExpr()->isDone())
    expr->setDone();
  return nullptr;
}

/// Typecheck __array__ method. This method creates a stack-allocated array via alloca.
Expr *TypecheckVisitor::transformArray(CallExpr *expr) {
  auto arrTyp = expr->expr->getType()->getFunc();
  unify(expr->getType(),
        instantiateType(getStdLibType("Array"),
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
        tag = static_cast<int>(ui);
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
      return transform(
          N<CallExpr>(N<IdExpr>(getMangledFunc("std.internal.python", "_isinstance")),
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
Expr *TypecheckVisitor::transformHasAttr(CallExpr *expr) {
  if (auto u = expr->getType()->getUnbound())
    u->staticKind = LiteralKind::Bool;

  auto typ = extractClassType((*expr)[0].getExpr());
  if (!typ)
    return nullptr;

  auto member = getStrLiteral(extractFuncGeneric(expr->getExpr()->getType()));
  std::vector<std::pair<std::string, types::Type *>> args{{"", typ}};

  if (auto tup = cast<CallExpr>((*expr)[1].getExpr())) {
    for (auto &a : *tup) {
      a.value = transform(a.getExpr());
      if (!a.getExpr()->getClassType())
        return nullptr;
      auto t = extractType(a);
      args.emplace_back("", t->is("TypeWrap") ? extractClassGeneric(t) : t);
    }
  }
  for (auto &[n, ne] : extractNamedTuple((*expr)[2].getExpr())) {
    ne = transform(ne);
    auto t = extractType(ne);
    args.emplace_back(n, t->is("TypeWrap") ? extractClassGeneric(t) : t);
  }

  if (typ->getUnion()) {
    Expr *cond = nullptr;
    auto unionTypes = typ->getUnion()->getRealizationTypes();
    for (auto &unionType : unionTypes) {
      auto tu = realize(unionType);
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
  } else if (typ->is("NamedTuple")) {
    if (!typ->canRealize())
      return nullptr;
    auto id = getIntLiteral(typ);
    seqassert(id >= 0 && id < ctx->cache->generatedTupleNames.size(), "bad id: {}", id);
    const auto &names = ctx->cache->generatedTupleNames[id];
    return transform(N<BoolExpr>(in(names, member)));
  }

  bool exists = !findMethod(typ->getClass(), member).empty() ||
                findMember(typ->getClass(), member);
  if (exists && args.size() > 1) {
    exists &= findBestMethod(typ, member, args) != nullptr;
  }
  return transform(N<BoolExpr>(exists));
}

/// Transform getattr method to a DotExpr.
Expr *TypecheckVisitor::transformGetAttr(CallExpr *expr) {
  auto name = getStrLiteral(extractFuncGeneric(expr->expr->getType()));

  // special handling for NamedTuple
  if (expr->begin()->getExpr()->getType() &&
      expr->begin()->getExpr()->getType()->is("NamedTuple")) {
    auto val = expr->begin()->getExpr()->getClassType();
    auto id = getIntLiteral(val);
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
  auto attr = getStrLiteral(extractFuncGeneric(expr->expr->getType()));
  return transform(
      N<StmtExpr>(N<AssignMemberStmt>((*expr)[0].getExpr(), attr, (*expr)[1].getExpr()),
                  N<CallExpr>(N<IdExpr>("NoneType"))));
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
  auto pt = (*expr)[0].getExpr()->getType()->getPartial();
  if (!fn->getFunc() && pt && pt->isPartialEmpty()) {
    auto pft = pt->getPartialFunc()->generalize(0);
    fn = instantiateType(pft.get());
  }
  if (!fn->getFunc())
    E(Error::CALL_REALIZED_FN, (*expr)[0].getExpr());
  auto argt = (*expr)[1].getExpr()->getType()->getClass();
  if (!argt)
    return nullptr;
  seqassert(argt->name == TYPE_TUPLE, "not a tuple");
  for (size_t i = 0; i < std::min(argt->size(), fn->getFunc()->size()); i++) {
    auto at = (*argt)[i]->is("TypeWrap") ? extractClassGeneric((*argt)[i]) : (*argt)[i];
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
  if (typ->is("NoneType")) {
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
      if (typ->is(TYPE_TUPLE) && typ->getHeterogenousTuple()) {
        if (cfn->ast->hasAttribute(Attr::AutoGenerated) &&
            (endswith(cfn->ast->name, ".__iter__") ||
             endswith(cfn->ast->name, ".__getitem__"))) {
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
