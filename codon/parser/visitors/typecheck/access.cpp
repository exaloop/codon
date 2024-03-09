// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

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

/// Typecheck identifiers. If an identifier is a static variable, evaluate it and
/// replace it with its value (e.g., a @c IntExpr ). Also ensure that the identifier of
/// a generic function or a type is fully qualified (e.g., replace `Ptr` with
/// `Ptr[byte]`).
/// For tuple identifiers, generate appropriate class. See @c generateTuple for
/// details.
void TypecheckVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->value);
  // if (!val && ctx->getBase()->pyCaptures) {
  //   ctx->getBase()->pyCaptures->insert(expr->value);
  //   resultExpr = N<IndexExpr>(N<IdExpr>("__pyenv__"), N<StringExpr>(expr->value));
  //   return;
  // } else
  // if (ctx->isOuter(val) && !ctx->isCanonicalName(expr->value))
  //   ctx->getBase()->captures.insert(expr->value);
  if (!val) {
    // ctx->dump();
    // LOG("=================================================================");
    // ctx->cache->typeCtx->dump();
    E(Error::ID_NOT_FOUND, expr, expr->value);
  }
  auto o = in(ctx->cache->overloads, val->canonicalName);
  if (expr->type->getUnbound() && o && o->size() > 1) {
    // LOG("dispatch: {}", val->canonicalName);
    val = ctx->forceFind(getDispatch(val->canonicalName)->ast->name);
  }

  // If we are accessing an outside variable, capture it or raise an error
  auto captured = checkCapture(val);
  if (captured)
    val = ctx->forceFind(expr->value);

  // Replace the variable with its canonical name
  expr->value = val->canonicalName;

  // Set up type
  unify(expr->type, ctx->instantiate(val->type));

  // Realize a type or a function if possible and replace the identifier with the fully
  // typed identifier (e.g., `foo` -> `foo[int]`)
  if (realize(expr->type)) {
    if (auto s = expr->type->getStatic()) {
      resultExpr = transform(s->getStaticExpr());
      return;
    }
    if (!val->isVar())
      expr->value = expr->type->realizedName();
    expr->setDone();
  }
}

/// Flatten imports.
/// @example
///   `a.b.c`      -> canonical name of `c` in `a.b` if `a.b` is an import
///   `a.B.c`      -> canonical name of `c` in class `a.B`
///   `python.foo` -> internal.python._get_identifier("foo")
/// Other cases are handled during the type checking.
/// See @c transformDot for details.
void TypecheckVisitor::visit(DotExpr *expr) { resultExpr = transformDot(expr); }

/// Access identifiers from outside of the current function/class scope.
/// Either use them as-is (globals), capture them if allowed (nonlocals),
/// or raise an error.
bool TypecheckVisitor::checkCapture(const TypeContext::Item &val) {
  if (!ctx->isOuter(val))
    return false;
  if ((val->isType() && !val->isGeneric()) || val->isFunc())
    return false;

  // Ensure that outer variables can be captured (i.e., do not cross no-capture
  // boundary). Example:
  // def foo():
  //   x = 1
  //   class T:      # <- boundary (classes cannot capture locals)
  //     t: int = x  # x cannot be accessed
  //     def bar():  # <- another boundary
  //                 # (class methods cannot capture locals except class generics)
  //       print(x)  # x cannot be accessed
  bool crossCaptureBoundary = false;
  bool localGeneric = val->isGeneric() && val->getBaseName() == ctx->getBaseName();
  bool parentClassGeneric =
      val->isGeneric() && !ctx->getBase()->isType() &&
      (ctx->bases.size() > 1 && ctx->bases[ctx->bases.size() - 2].isType() &&
       ctx->bases[ctx->bases.size() - 2].name == val->getBaseName());
  auto i = ctx->bases.size();
  for (; i-- > 0;) {
    if (ctx->bases[i].name == val->getBaseName())
      break;
    if (!localGeneric && !parentClassGeneric)
      crossCaptureBoundary = true;
  }

  // Mark methods (class functions that access class generics)
  if (parentClassGeneric)
    ctx->getBase()->attributes->set(Attr::Method);

  // Ignore generics
  if (parentClassGeneric || localGeneric)
    return false;

  // Case: a global variable that has not been marked with `global` statement
  if (val->isVar() && val->getBaseName().empty() && val->scope.size() == 1) {
    ctx->cache->addGlobal(val->canonicalName);
    return false;
  }

  // Check if a real variable (not a static) is defined outside the current scope
  if (crossCaptureBoundary)
    E(Error::ID_CANNOT_CAPTURE, getSrcInfo(), ctx->cache->rev(val->canonicalName));

  // Case: a nonlocal variable that has not been marked with `nonlocal` statement
  //       and capturing is enabled
  // auto captures = ctx->getBase()->captures;
  // if (captures && !in(*captures, val->canonicalName)) {
  //   // Captures are transformed to function arguments; generate new name for that
  //   // argument
  //   ExprPtr typ = nullptr;
  //   if (val->isType())
  //     typ = N<IdExpr>("type");
  //   if (auto st = val->isStatic())
  //     typ = N<IndexExpr>(N<IdExpr>("Static"),
  //                        N<IdExpr>(st == StaticValue::INT ? "int" : "str"));
  //   auto [newName, _] = (*captures)[val->canonicalName] = {
  //       ctx->generateCanonicalName(val->canonicalName), typ};
  //   ctx->cache->reverseIdentifierLookup[newName] = newName;
  //   // Add newly generated argument to the context
  //   std::shared_ptr<TypecheckItem> newVal = nullptr;
  //   if (val->isType())
  //     newVal = ctx->addType(ctx->cache->rev(val->canonicalName), newName, val->type);
  //   else
  //     newVal = ctx->addVar(ctx->cache->rev(val->canonicalName), newName, val->type);
  //   newVal->baseName = ctx->getBaseName();
  //   newVal->canShadow = false; // todo)) needed here? remove noshadow on fn
  //   boundaries? newVal->scope = ctx->getBase()->scope; return true;
  // }

  // Case: a nonlocal variable that has not been marked with `nonlocal` statement
  //       and capturing is *not* enabled
  E(Error::ID_NONLOCAL, getSrcInfo(), ctx->cache->rev(val->canonicalName));
  return false;
}

/// Check if a access chain (a.b.c.d...) contains an import or class prefix.
std::pair<size_t, TypeContext::Item>
TypecheckVisitor::getImport(const std::vector<std::string> &chain) {
  size_t importEnd = 0;
  std::string importName;

  // Find the longest prefix that corresponds to the existing import
  // (e.g., `a.b.c.d` -> `a.b.c` if there is `import a.b.c`)
  TypeContext::Item val = nullptr;
  for (auto i = chain.size(); i-- > 0;) {
    auto name = join(chain, "/", 0, i + 1);
    val = ctx->find(name);
    if (val && val->type->is("Import") && name != "Import") {
      importName = val->type->getClass()->generics[0].type->getStrStatic()->value;
      importEnd = i + 1;
      break;
    }
  }
  auto importVal = val;

  // Special checks for imports
  if (importEnd != chain.size()) { // false when a.b.c points to import itself
    // Find the longest prefix that corresponds to the existing class
    // (e.g., `a.b.c` -> `a.b` if there is `class a: class b:`)
    std::string itemName;
    size_t itemEnd = 0;
    auto fctx = importName.empty() ? ctx : ctx->cache->imports[importName].ctx;
    for (auto i = chain.size(); i-- > importEnd;) {
      if (fctx->getModule() == "std.python" && importEnd < chain.size()) {
        // Special case: importing from Python.
        // Fake TypecheckItem that indicates std.python access
        val = std::make_shared<TypecheckItem>("", "", fctx->getModule(),
                                              fctx->getUnbound());
        return {importEnd, val};
      } else {
        auto key = join(chain, ".", importEnd, i + 1);
        val = fctx->find(key);
        bool isOverload = val && val->isFunc() &&
                          in(ctx->cache->overloads, val->canonicalName) &&
                          ctx->cache->overloads[val->canonicalName].size() > 1;
        if (val && !isOverload &&
            (importName.empty() || val->isType() || !val->isConditional())) {
          itemName = val->canonicalName, itemEnd = i + 1;
          break;
        }
        if (ctx->findMember("Import", key))
          return {importEnd, importVal};
        if (!ctx->findMethod("Import", key).empty())
          return {importEnd, importVal};
      }
    }
    if (itemName.empty() && importName.empty()) {
      if (ctx->getBase()->pyCaptures)
        return {1, nullptr};
      E(Error::IMPORT_NO_MODULE, getSrcInfo(), chain[importEnd]);
    }
    if (itemName.empty()) {
      E(Error::IMPORT_NO_NAME, getSrcInfo(), chain[importEnd],
        ctx->cache->imports[importName].name);
    }
    importEnd = itemEnd;
  }
  return {importEnd, val};
}

/// Find an overload dispatch function for a given overload. If it does not exist and
/// there is more than one overload, generate it. Dispatch functions ensure that a
/// function call is being routed to the correct overload even when dealing with partial
/// functions and decorators.
/// @example This is how dispatch looks like:
///   ```def foo:dispatch(*args, **kwargs):
///        return foo(*args, **kwargs)```
types::FuncTypePtr TypecheckVisitor::getDispatch(const std::string &fn) {
  auto &overloads = ctx->cache->overloads[fn];

  // Single overload: just return it
  if (overloads.size() == 1)
    return ctx->forceFind(overloads.front())->type->getFunc();

  // Check if dispatch exists
  for (auto &m : overloads)
    if (endswith(ctx->cache->functions[m].ast->name, ":dispatch")) {
      return ctx->cache->functions[m].type;
    }

  // Dispatch does not exist. Generate it
  auto name = fn + ":dispatch";
  ExprPtr root; // Root function name used for calling
  auto a = ctx->cache->functions[overloads[0]].ast;
  if (!a->attributes.parentClass.empty())
    root = N<DotExpr>(N<IdExpr>(a->attributes.parentClass),
                      ctx->cache->reverseIdentifierLookup[fn]);
  else
    root = N<IdExpr>(fn);
  root = N<CallExpr>(root, N<StarExpr>(N<IdExpr>("args")),
                     N<KeywordStarExpr>(N<IdExpr>("kwargs")));
  auto nar = ctx->generateCanonicalName("args");
  auto nkw = ctx->generateCanonicalName("kwargs");
  auto ast = N<FunctionStmt>(
      name, nullptr, std::vector<Param>{Param("*" + nar), Param("**" + nkw)},
      N<SuiteStmt>(N<ReturnStmt>(root)), Attr({"autogenerated"}));
  ctx->cache->reverseIdentifierLookup[name] = ctx->cache->reverseIdentifierLookup[fn];

  auto baseType = getFuncTypeBase(2);
  auto typ = std::make_shared<FuncType>(baseType, ast.get());
  typ = std::static_pointer_cast<FuncType>(typ->generalize(ctx->typecheckLevel - 1));
  ctx->addFunc(name, name, typ);

  overloads.insert(overloads.begin(), name);
  ctx->cache->functions[name].ast = ast;
  ctx->cache->functions[name].type = typ;
  ast->setDone();
  prependStmts->push_back(ast);
  return typ;
}

/// Transform a dot expression. Select the best method overload if possible.
/// @param args (optional) list of class method arguments used to select the best
///             overload. nullptr if not available.
/// @example
///   `obj.__class__`   -> `type(obj)`
///   `cls.__name__`    -> `"class"` (same for functions)
///   `obj.method`      -> `cls.method(obj, ...)` or
///                        `cls.method(obj)` if method has `@property` attribute
///   @c getClassMember examples:
///   `obj.GENERIC`     -> `GENERIC` (IdExpr with generic/static value)
///   `optional.member` -> `unwrap(optional).member`
///   `pyobj.member`    -> `pyobj._getattr("member")`
/// @return nullptr if no transformation was made
/// See @c getClassMember and @c getBestOverload
ExprPtr TypecheckVisitor::transformDot(DotExpr *expr,
                                       std::vector<CallExpr::Arg> *args) {
  // First flatten the imports:
  // transform Dot(Dot(a, b), c...) to {a, b, c, ...}
  std::vector<std::string> chain;
  Expr *root = expr;
  for (; root->getDot(); root = root->getDot()->expr.get())
    chain.push_back(root->getDot()->member);

  ExprPtr nexpr = expr->shared_from_this();
  if (auto id = root->getId()) {
    // Case: a.bar.baz
    chain.push_back(id->value);
    std::reverse(chain.begin(), chain.end());
    auto [pos, val] = getImport(chain);
    if (!val) {
      seqassert(ctx->getBase()->pyCaptures, "unexpected py capture");
      ctx->getBase()->pyCaptures->insert(chain[0]);
      nexpr = N<IndexExpr>(N<IdExpr>("__pyenv__"), N<StringExpr>(chain[0]));
    } else if (val->getModule() == "std.python") {
      nexpr = transform(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(N<IdExpr>("internal"), "python"), "_get_identifier"),
          N<StringExpr>(chain[pos++])));
    } else if (val->getModule() == ctx->getModule() && pos == 1) {
      nexpr = transform(N<IdExpr>(chain[0]), true);
    } else {
      nexpr = N<IdExpr>(val->canonicalName);
    }
    while (pos < chain.size())
      nexpr = N<DotExpr>(nexpr, chain[pos++]);
  }
  if (!nexpr->getDot()) {
    return transform(nexpr);
  } else {
    expr->expr = nexpr->getDot()->expr;
    expr->member = nexpr->getDot()->member;
  }

  // Special case: obj.__class__
  if (expr->member == "__class__") {
    /// TODO: prevent cls.__class__ and type(cls)
    return N<CallExpr>(N<IdExpr>("type"), expr->expr);
  }
  transform(expr->expr);

  // Special case: fn.__name__
  // Should go before cls.__name__ to allow printing generic functions
  if (expr->expr->type->getFunc() && expr->member == "__name__") {
    return transform(N<StringExpr>(expr->expr->type->prettyString()));
  }
  // Special case: fn.__llvm_name__ or obj.__llvm_name__
  if (expr->member == "__llvm_name__") {
    if (realize(expr->expr->type))
      return transform(N<StringExpr>(expr->expr->type->realizedName()));
    return nullptr;
  }
  // Special case: cls.__name__
  if (expr->expr->type->is("type") && expr->member == "__name__") {
    if (realize(expr->expr->type))
      return transform(N<StringExpr>(ctx->getType(expr->expr->type)->prettyString()));
    return nullptr;
  }
  // Special case: expr.__is_static__
  if (expr->member == "__is_static__") {
    if (expr->expr->isDone())
      return transform(N<IntExpr>(bool(expr->expr->type->getStatic())));
    return nullptr;
  }
  // Special case: cls.__id__
  if (expr->expr->type->is("type") && expr->member == "__id__") {
    if (auto c = realize(expr->expr->type))
      return transform(N<IntExpr>(ctx->cache->classes[c->getClass()->name]
                                      .realizations[c->getClass()->realizedTypeName()]
                                      ->id));
    return nullptr;
  }

  // Ensure that the type is known (otherwise wait until it becomes known)
  if (!getType(expr->expr))
    return nullptr;
  auto typ = getType(expr->expr)->getClass();
  if (!typ)
    return nullptr;

  // Check if this is a method or member access
  if (ctx->findMethod(typ->name, expr->member).empty())
    return getClassMember(expr, args);
  auto bestMethod = getBestOverload(expr, args);

  if (args) {
    unify(expr->type, ctx->instantiate(bestMethod, typ));

    // A function is deemed virtual if it is marked as such and
    // if a base class has a RTTI
    bool isVirtual = in(ctx->cache->classes[typ->name].virtuals, expr->member);
    isVirtual &= ctx->cache->classes[typ->name].rtti;
    isVirtual &= !expr->expr->type->is("type");
    if (isVirtual && !bestMethod->ast->attributes.has(Attr::StaticMethod) &&
        !bestMethod->ast->attributes.has(Attr::Property)) {
      // Special case: route the call through a vtable
      if (realize(expr->type)) {
        auto fn = expr->type->getFunc();
        auto vid = getRealizationID(typ.get(), fn.get());

        // Function[Tuple[TArg1, TArg2, ...], TRet]
        std::vector<ExprPtr> ids;
        for (auto &t : fn->getArgTypes())
          ids.push_back(N<IdExpr>(t->realizedName()));
        auto name = generateTuple(ids.size());
        auto fnType = N<InstantiateExpr>(
            N<IdExpr>("Function"),
            std::vector<ExprPtr>{N<InstantiateExpr>(N<IdExpr>(name), ids),
                                 N<IdExpr>(fn->getRetType()->realizedName())});
        // Function[Tuple[TArg1, TArg2, ...],TRet](
        //    __internal__.class_get_rtti_vtable(expr)[T[VIRTUAL_ID]]
        // )
        auto e = N<CallExpr>(
            fnType, N<IndexExpr>(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"),
                                                        "class_get_rtti_vtable"),
                                             expr->expr),
                                 N<IntExpr>(vid)));
        return transform(e);
      }
    }
  }

  // Check if a method is a static or an instance method and transform accordingly
  if (expr->expr->type->is("type") || args) {
    // Static access: `cls.method`
    ExprPtr e = N<IdExpr>(bestMethod->ast->name);
    unify(e->type, unify(expr->type, ctx->instantiate(bestMethod, typ)));
    return transform(e); // Realize if needed
  } else {
    // Instance access: `obj.method`
    // Transform y.method to a partial call `type(obj).method(args, ...)`
    std::vector<ExprPtr> methodArgs;
    // Do not add self if a method is marked with @staticmethod
    if (!bestMethod->ast->attributes.has(Attr::StaticMethod))
      methodArgs.push_back(expr->expr);
    // If a method is marked with @property, just call it directly
    if (!bestMethod->ast->attributes.has(Attr::Property))
      methodArgs.push_back(N<EllipsisExpr>(EllipsisExpr::PARTIAL));
    auto e = transform(N<CallExpr>(N<IdExpr>(bestMethod->ast->name), methodArgs));
    unify(expr->type, e->type);
    return e;
  }
}

/// Select the requested class member.
/// @param args (optional) list of class method arguments used to select the best
///             overload if the member is optional. nullptr if not available.
/// @example
///   `obj.GENERIC`     -> `GENERIC` (IdExpr with generic/static value)
///   `optional.member` -> `unwrap(optional).member`
///   `pyobj.member`    -> `pyobj._getattr("member")`
ExprPtr TypecheckVisitor::getClassMember(DotExpr *expr,
                                         std::vector<CallExpr::Arg> *args) {
  auto typ = getType(expr->expr)->getClass();
  seqassert(typ, "not a class");

  // Case: object member access (`obj.member`)
  if (!expr->expr->type->is("type")) {
    if (auto member = ctx->findMember(typ->name, expr->member)) {
      unify(expr->type, ctx->instantiate(member->type, typ));
      if (!expr->type->canRealize() && member->typeExpr) {
        ctx->addBlock();
        addClassGenerics(typ);
        auto t = ctx->getType(transform(clean_clone(member->typeExpr))->type);
        ctx->popBlock();
        unify(expr->type, t);
      }
      if (expr->expr->isDone() && realize(expr->type))
        expr->setDone();
      return nullptr;
    }
  }

  // Case: class variable (`Cls.var`)
  if (auto cls = in(ctx->cache->classes, typ->name))
    if (auto var = in(cls->classVars, expr->member)) {
      return transform(N<IdExpr>(*var));
    }

  // Case: special members
  if (auto mtyp = findSpecialMember(expr->member)) {
    unify(expr->type, mtyp);
    if (expr->expr->isDone() && realize(expr->type))
      expr->setDone();
    return nullptr;
  }

  // Case: object generic access (`obj.T`)
  ClassType::Generic *generic = nullptr;
  for (auto &g : typ->generics)
    if (ctx->cache->reverseIdentifierLookup[g.name] == expr->member) {
      generic = &g;
      break;
    }
  if (generic) {
    unify(expr->type, generic->type);
    if (realize(expr->type)) {
      if (!generic->isStatic) {
        return transform(N<IdExpr>(generic->type->realizedName()));
      } else {
        expr->type = nullptr; // to prevent unify(T, Static[T]) error
        return transform(generic->type->getStatic()->getStaticExpr());
      }
    }
    return nullptr;
  }

  // Case: transform `optional.member` to `unwrap(optional).member`
  if (typ->is(TYPE_OPTIONAL)) {
    auto dot = N<DotExpr>(transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->expr)),
                          expr->member);
    dot->setType(ctx->getUnbound()); // as dot is not transformed
    if (auto d = transformDot(dot.get(), args))
      return d;
    return dot;
  }

  // Case: transform `pyobj.member` to `pyobj._getattr("member")`
  if (typ->is("pyobj")) {
    return transform(
        N<CallExpr>(N<DotExpr>(expr->expr, "_getattr"), N<StringExpr>(expr->member)));
  }

  // Case: transform `union.m` to `__internal__.get_union_method(union, "m", ...)`
  if (typ->getUnion()) {
    return transform(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("__internal__"), "get_union_method"),
        std::vector<CallExpr::Arg>{{"union", expr->expr},
                                   {"method", N<StringExpr>(expr->member)},
                                   {"", N<EllipsisExpr>(EllipsisExpr::PARTIAL)}}));
  }

  // For debugging purposes: ctx->findMethod(typ->name, expr->member);
  ctx->findMethod(typ->name, expr->member);
  E(Error::DOT_NO_ATTR, expr, typ->prettyString(), expr->member);
  return nullptr;
}

TypePtr TypecheckVisitor::findSpecialMember(const std::string &member) {
  if (member == "__elemsize__")
    return ctx->getType("int");
  if (member == "__atomic__")
    return ctx->getType("bool");
  if (member == "__contents_atomic__")
    return ctx->getType("bool");
  if (member == "__name__")
    return ctx->getType("str");
  return nullptr;
}

/// Select the best overloaded function or method.
/// @param expr    a DotExpr (for methods) or an IdExpr (for overloaded functions)
/// @param methods List of available methods.
/// @param args    (optional) list of class method arguments used to select the best
///                overload if the member is optional. nullptr if not available.
FuncTypePtr TypecheckVisitor::getBestOverload(Expr *expr,
                                              std::vector<CallExpr::Arg> *args) {
  // Prepare the list of method arguments if possible
  std::unique_ptr<std::vector<CallExpr::Arg>> methodArgs;

  if (args) {
    // Case: method overloads (DotExpr)
    bool addSelf = true;
    if (auto dot = expr->getDot()) {
      auto methods =
          ctx->findMethod(getType(dot->expr)->getClass()->name, dot->member, false);
      if (!methods.empty() && methods.front()->ast->attributes.has(Attr::StaticMethod))
        addSelf = false;
    }

    // Case: arguments explicitly provided (by CallExpr)
    if (addSelf && expr->getDot() && !expr->getDot()->expr->type->is("type")) {
      // Add `self` as the first argument
      args->insert(args->begin(), {"", expr->getDot()->expr});
    }
    methodArgs = std::make_unique<std::vector<CallExpr::Arg>>();
    for (auto &a : *args)
      methodArgs->push_back(a);
  } else {
    // Partially deduced type thus far
    auto typeSoFar =
        expr->type ? getType(expr->shared_from_this())->getClass() : nullptr;
    if (typeSoFar && typeSoFar->getFunc()) {
      // Case: arguments available from the previous type checking round
      methodArgs = std::make_unique<std::vector<CallExpr::Arg>>();
      if (expr->getDot() && !expr->getDot()->expr->type->is("type")) { // Add `self`
        auto n = N<NoneExpr>();
        n->setType(expr->getDot()->expr->type);
        methodArgs->push_back({"", n});
      }
      for (auto &a : typeSoFar->getFunc()->getArgTypes()) {
        auto n = N<NoneExpr>();
        n->setType(a);
        methodArgs->push_back({"", n});
      }
    }
  }

  if (methodArgs) {
    FuncTypePtr bestMethod = nullptr;
    // Use the provided arguments to select the best method
    if (auto dot = expr->getDot()) {
      // Case: method overloads (DotExpr)
      auto methods =
          ctx->findMethod(getType(dot->expr)->getClass()->name, dot->member, false);
      auto m =
          findMatchingMethods(getType(dot->expr)->getClass(), methods, *methodArgs);
      bestMethod = m.empty() ? nullptr : m[0];
    } else if (auto id = expr->getId()) {
      // Case: function overloads (IdExpr)
      std::vector<types::FuncTypePtr> methods;
      auto key = id->value;
      if (endswith(key, ":dispatch"))
        key = key.substr(0, key.size() - 9);
      for (auto &m : ctx->cache->overloads[key])
        if (!endswith(m, ":dispatch"))
          methods.push_back(ctx->cache->functions[m].type);
      std::reverse(methods.begin(), methods.end());
      auto m = findMatchingMethods(nullptr, methods, *methodArgs);
      bestMethod = m.empty() ? nullptr : m[0];
    }
    if (bestMethod)
      return bestMethod;
  } else {
    // If overload is ambiguous, route through a dispatch function
    std::string name;
    if (auto dot = expr->getDot()) {
      name = ctx->cache->getMethod(getType(dot->expr)->getClass(), dot->member);
    } else {
      name = expr->getId()->value;
    }
    return getDispatch(name);
  }

  // Print a nice error message
  std::string argsNice;
  if (methodArgs) {
    std::vector<std::string> a;
    for (auto &t : *methodArgs)
      a.emplace_back(fmt::format("{}",
      t.value->type->getStatic() ? t.value->type->getClass()->name :
      t.value->type->prettyString()));
    argsNice = fmt::format("({})", fmt::join(a, ", "));
  }

  if (auto dot = expr->getDot()) {
    // getBestOverload(expr, args);
    E(Error::DOT_NO_ATTR_ARGS, expr, getType(dot->expr)->prettyString(), dot->member,
      argsNice);
  } else {
    E(Error::FN_NO_ATTR_ARGS, expr, ctx->cache->rev(expr->getId()->value), argsNice);
  }

  return nullptr;
}

} // namespace codon::ast
