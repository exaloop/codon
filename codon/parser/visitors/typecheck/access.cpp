// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Typecheck identifiers. If an identifier is a static variable, evaluate it and
/// replace it with its value (e.g., a @c IntExpr ). Also ensure that the identifier of
/// a generic function or a type is fully qualified (e.g., replace `Ptr` with
/// `Ptr[byte]`).
void TypecheckVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->value);
  if (!val) {
    E(Error::ID_NOT_FOUND, expr, expr->value);
  }
  auto o = in(ctx->cache->overloads, val->canonicalName);
  if (expr->getType()->getUnbound() && o && o->size() > 1) {
    val = ctx->forceFind(getDispatch(val->canonicalName)->ast->name);
  }

  // If we are accessing an outside variable, capture it or raise an error
  auto captured = checkCapture(val);
  if (captured)
    val = ctx->forceFind(expr->value);

  // Replace the variable with its canonical name
  expr->value = val->canonicalName;

  // Set up type
  unify(expr->getType(), ctx->instantiate(val->type));

  // Realize a type or a function if possible and replace the identifier with the fully
  // typed identifier (e.g., `foo` -> `foo[int]`)
  if (realize(expr->getType())) {
    if (auto s = expr->getType()->getStatic()) {
      resultExpr = transform(s->getStaticExpr());
      return;
    }
    if (!val->isVar())
      expr->value = expr->getType()->realizedName();
    expr->setDone();
  }

  if (expr->hasAttribute(Attr::ExprDominatedUndefCheck)) {
    auto controlVar = fmt::format("{}.__used__", ctx->cache->rev(val->canonicalName));
    if (ctx->find(controlVar)) {
      auto checkStmt = N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(N<IdExpr>("__internal__"), "undef"), N<IdExpr>(controlVar),
          N<StringExpr>(ctx->cache->rev(val->canonicalName))));
      expr->eraseAttribute(Attr::ExprDominatedUndefCheck);
      resultExpr = transform(N<StmtExpr>(checkStmt, expr));
    }
  }
}

/// Flatten imports.
/// @example
///   `a.b.c`      -> canonical name of `c` in `a.b` if `a.b` is an import
///   `a.B.c`      -> canonical name of `c` in class `a.B`
///   `python.foo` -> internal.python._get_identifier("foo")
/// Other cases are handled during the type checking.
/// Transform a dot expression. Select the best method overload if possible.
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

void TypecheckVisitor::visit(DotExpr *expr) {
  // First flatten the imports:
  // transform Dot(Dot(a, b), c...) to {a, b, c, ...}

  CallExpr *parentCall = cast<CallExpr>(ctx->getParentNode());
  if (parentCall && !parentCall->hasAttribute("CallExpr"))
    parentCall = nullptr;

  std::vector<std::string> chain;
  Expr *root = expr;
  for (; cast<DotExpr>(root); root = cast<DotExpr>(root)->getExpr())
    chain.push_back(cast<DotExpr>(root)->getMember());

  Expr *nexpr = expr;
  if (auto id = cast<IdExpr>(root)) {
    // Case: a.bar.baz
    chain.push_back(id->getValue());
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
  if (!cast<DotExpr>(nexpr)) {
    resultExpr = transform(nexpr);
    return;
  } else {
    expr->expr = cast<DotExpr>(nexpr)->getExpr();
    expr->member = cast<DotExpr>(nexpr)->getMember();
  }

  // Special case: obj.__class__
  if (expr->getMember() == "__class__") {
    /// TODO: prevent cls.__class__ and type(cls)
    resultExpr = transform(N<CallExpr>(N<IdExpr>("type"), expr->getExpr()));
    return;
  }
  expr->expr = transform(expr->getExpr());

  // Special case: fn.__name__
  // Should go before cls.__name__ to allow printing generic functions
  if (ctx->getType(expr->getExpr()->getType())->getFunc() &&
      expr->getMember() == "__name__") {
    resultExpr = transform(
        N<StringExpr>(ctx->getType(expr->getExpr()->getType())->prettyString()));
    return;
  }
  // Special case: fn.__llvm_name__ or obj.__llvm_name__
  if (expr->getMember() == "__llvm_name__") {
    if (realize(expr->getExpr()->getType()))
      resultExpr = transform(N<StringExpr>(expr->getExpr()->getType()->realizedName()));
    return;
  }
  // Special case: cls.__name__
  if (expr->getExpr()->getType()->is("type") && expr->getMember() == "__name__") {
    if (realize(expr->getExpr()->getType()))
      resultExpr = transform(
          N<StringExpr>(ctx->getType(expr->getExpr()->getType())->prettyString()));
    return;
  }
  // Special case: expr.__is_static__
  if (expr->getMember() == "__is_static__") {
    if (expr->getExpr()->isDone())
      resultExpr =
          transform(N<BoolExpr>(bool(expr->getExpr()->getType()->getStatic())));
    return;
  }
  // Special case: cls.__id__
  if (expr->getExpr()->getType()->is("type") && expr->getMember() == "__id__") {
    if (auto c = realize(getType(expr->getExpr())))
      resultExpr =
          transform(N<IntExpr>(ctx->cache->getClass(c->getClass())
                                   ->realizations[c->getClass()->realizedName()]
                                   ->id));
    return;
  }

  // Ensure that the type is known (otherwise wait until it becomes known)
  auto typ = getType(expr->getExpr()) ? getType(expr->getExpr())->getClass() : nullptr;
  if (!typ)
    return;

  // Check if this is a method or member access
  while (true) {
    auto methods = ctx->findMethod(typ.get(), expr->getMember());
    if (methods.empty())
      resultExpr = getClassMember(expr);
    auto oldExpr = expr->getExpr();
    if (!resultExpr && expr->getExpr() != oldExpr) {
      typ = getType(expr->getExpr()) ? getType(expr->getExpr())->getClass() : nullptr;
      if (!typ)
        return;
      continue;
    }

    if (!methods.empty()) {
      auto bestMethod =
          methods.size() > 1
              ? getDispatch(
                    ctx->cache->functions[methods.front()->ast->getName()].rootName)
              : methods.front();
      Expr *e = N<IdExpr>(bestMethod->ast->getName());
      e->setType(unify(expr->getType(), ctx->instantiate(bestMethod, typ)));
      if (expr->getExpr()->getType()->is("type")) {
        // Static access: `cls.method`
      } else if (parentCall && !bestMethod->ast->hasAttribute(Attr::StaticMethod) &&
                 !bestMethod->ast->hasAttribute(Attr::Property)) {
        // Instance access: `obj.method`
        parentCall->items.insert(parentCall->items.begin(), expr->getExpr());
        // LOG("|-> adding dot: {} // {} ####### {}", expr->toString(0),
        // e->getType()->debugString(2), parentCall->toString(2));
      } else {
        // Instance access: `obj.method`
        // Transform y.method to a partial call `type(obj).method(args, ...)`
        std::vector<Expr *> methodArgs;
        // Do not add self if a method is marked with @staticmethod
        if (!bestMethod->ast->hasAttribute(Attr::StaticMethod))
          methodArgs.push_back(expr->getExpr());
        // If a method is marked with @property, just call it directly
        if (!bestMethod->ast->hasAttribute(Attr::Property))
          methodArgs.push_back(N<EllipsisExpr>(EllipsisExpr::PARTIAL));
        e = N<CallExpr>(e, methodArgs);
      }
      resultExpr = transform(e);
    }
    break;
  }
}

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
    ctx->getBase()->func->setAttribute(Attr::Method);

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
  TypeContext::Item val = nullptr, importVal = nullptr;
  for (auto i = chain.size(); i-- > 0;) {
    auto name = join(chain, "/", 0, i + 1);
    val = ctx->find(name);
    if (val && val->type->is("Import") && name != "Import") {
      importName = val->type->getClass()->generics[0].type->getStrStatic()->value;
      importEnd = i + 1;
      importVal = val;
      break;
    }
  }

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
        if (val && i + 1 != chain.size() && val->type->is("Import") &&
            key != "Import") {
          importVal = val;
          importName = val->type->getClass()->generics[0].type->getStrStatic()->value;
          importEnd = i + 1;
          fctx = ctx->cache->imports[importName].ctx;
          i = chain.size();
          continue;
        }
        bool isOverload = val && val->isFunc() &&
                          in(ctx->cache->overloads, val->canonicalName) &&
                          ctx->cache->overloads[val->canonicalName].size() > 1;
        if (isOverload && importEnd == i) { // top-level overload
          itemName = val->canonicalName, itemEnd = i + 1;
          break;
        }
        if (val && !isOverload &&
            (importName.empty() || val->isType() || !val->isConditional())) {
          itemName = val->canonicalName, itemEnd = i + 1;
          break;
        }
        if (auto i = ctx->find("Import")) {
          auto t = ctx->getType(i->type)->getClass();
          if (ctx->findMember(t, key))
            return {importEnd, importVal};
          if (!ctx->findMethod(t.get(), key).empty())
            return {importEnd, importVal};
        }
      }
    }
    if (itemName.empty() && importName.empty()) {
      if (ctx->getBase()->pyCaptures)
        return {1, nullptr};
      E(Error::IMPORT_NO_MODULE, getSrcInfo(), chain[importEnd]);
    } else if (itemName.empty()) {
      if (!ctx->isStdlibLoading && endswith(importName, "__init__.codon")) {
        auto import = ctx->cache->imports[importName];
        auto file =
            getImportFile(ctx->cache->argv0, chain[importEnd], importName, false,
                          ctx->cache->module0, ctx->cache->pluginImportPaths);
        if (file) { // auto-load support
          Stmt *s = N<SuiteStmt>(N<ImportStmt>(N<IdExpr>(chain[importEnd]), nullptr));
          ScopingVisitor::apply(ctx->cache, s);
          s = TypecheckVisitor(import.ctx, preamble).transform(s);
          prependStmts->push_back(s);
          return getImport(chain);
        }
      }
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
  Expr *root; // Root function name used for calling
  auto a = ctx->cache->functions[overloads[0]].ast;
  if (auto aa = a->getAttribute<ir::StringValueAttribute>(Attr::ParentClass))
    root = N<DotExpr>(N<IdExpr>(aa->value), ctx->cache->reverseIdentifierLookup[fn]);
  else
    root = N<IdExpr>(fn);
  root = N<CallExpr>(root, N<StarExpr>(N<IdExpr>("args")),
                     N<KeywordStarExpr>(N<IdExpr>("kwargs")));
  auto nar = ctx->generateCanonicalName("args");
  auto nkw = ctx->generateCanonicalName("kwargs");
  auto ast = N<FunctionStmt>(name, nullptr,
                             std::vector<Param>{Param("*" + nar), Param("**" + nkw)},
                             N<SuiteStmt>(N<ReturnStmt>(root)));
  ast->setAttribute("autogenerated");
  ast->setAttribute(Attr::Module, ctx->moduleName.path);
  ctx->cache->reverseIdentifierLookup[name] = ctx->cache->reverseIdentifierLookup[fn];

  auto baseType = getFuncTypeBase(2);
  auto typ = std::make_shared<FuncType>(baseType, ast, 0);
  typ->funcParent = ctx->cache->functions[overloads[0]].type->funcParent;
  typ = std::static_pointer_cast<FuncType>(typ->generalize(ctx->typecheckLevel - 1));
  ctx->addFunc(name, name, typ);
  // LOG("-[D]-> {} / {}", typ->debugString(2), typ->funcParent->debugString(2));

  overloads.insert(overloads.begin(), name);
  ctx->cache->functions[name].ast = ast;
  ctx->cache->functions[name].type = typ;
  ast->setDone();
  // prependStmts->push_back(ast);
  return typ;
}

/// Select the requested class member.
/// @param args (optional) list of class method arguments used to select the best
///             overload if the member is optional. nullptr if not available.
/// @example
///   `obj.GENERIC`     -> `GENERIC` (IdExpr with generic/static value)
///   `optional.member` -> `unwrap(optional).member`
///   `pyobj.member`    -> `pyobj._getattr("member")`
Expr *TypecheckVisitor::getClassMember(DotExpr *expr) {
  auto typ = getType(expr->expr)->getClass();
  seqassert(typ, "not a class");

  // Case: object member access (`obj.member`)
  if (!expr->expr->getType()->is("type")) {
    if (auto member = ctx->findMember(typ, expr->member)) {
      unify(expr->getType(), ctx->instantiate(member->type, typ));
      if (!expr->getType()->canRealize() && member->typeExpr) {
        ctx->addBlock();
        addClassGenerics(typ);
        auto t = ctx->getType(transform(clean_clone(member->typeExpr))->getType());
        ctx->popBlock();
        unify(expr->getType(), t);
      }
      if (expr->expr->isDone() && realize(expr->getType()))
        expr->setDone();
      return nullptr;
    }
  }

  // Case: class variable (`Cls.var`)
  if (auto cls = ctx->cache->getClass(typ))
    if (auto var = in(cls->classVars, expr->member)) {
      return transform(N<IdExpr>(*var));
    }

  // Case: special members
  if (auto mtyp = findSpecialMember(expr->member)) {
    unify(expr->getType(), mtyp);
    if (expr->expr->isDone() && realize(expr->getType()))
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
    if (generic->isStatic) {
      unify(expr->getType(), generic->type);
      if (realize(expr->getType()))
        return transform(generic->type->getStatic()->getStaticExpr());
    } else {
      unify(expr->getType(),
            ctx->instantiateGeneric(ctx->getType("type"), {generic->type}));
      if (realize(expr->getType()))
        return transform(N<IdExpr>(generic->type->realizedName()));
    }
    return nullptr;
  }

  // Case: transform `optional.member` to `unwrap(optional).member`
  if (typ->is(TYPE_OPTIONAL)) {
    expr->expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->expr));
    return nullptr;
  }

  // Case: transform `pyobj.member` to `pyobj._getattr("member")`
  if (typ->is("pyobj")) {
    return transform(
        N<CallExpr>(N<DotExpr>(expr->expr, "_getattr"), N<StringExpr>(expr->member)));
  }

  // Case: transform `union.m` to `__internal__.get_union_method(union, "m", ...)`
  if (typ->getUnion()) {
    if (!typ->canRealize())
      return nullptr; // delay!
    return transform(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_member"),
                    std::vector<CallArg>{{"union", expr->expr},
                                         {"member", N<StringExpr>(expr->member)}}));
  }

  // For debugging purposes:
  // ctx->findMethod(typ.get(), expr->member);
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
// FuncTypePtr TypecheckVisitor::getBestOverload(Expr *expr, std::vector<CallArg> *args)
// {
//   // Prepare the list of method arguments if possible
//   std::unique_ptr<std::vector<CallArg>> methodArgs;

//   if (args) {
//     methodArgs = std::make_unique<std::vector<CallArg>>();
//     for (auto &a : *args)
//       methodArgs->push_back(a);
//   }
//   // else {
//   //   // Partially deduced type thus far
//   //   auto typeSoFar = expr->getType() ? getType(expr)->getClass() : nullptr;
//   //   if (typeSoFar && typeSoFar->getFunc()) {
//   //     // Case: arguments available from the previous type checking round
//   //     methodArgs = std::make_unique<std::vector<CallArg>>();
//   //     if (cast<DotExpr>(expr) &&
//   //         !cast<DotExpr>(expr)->expr->getType()->is("type")) { // Add `self`
//   //       auto n = N<NoneExpr>();
//   //       n->setType(cast<DotExpr>(expr)->expr->getType());
//   //       methodArgs->push_back({"", n});
//   //     }
//   //     for (auto &a : typeSoFar->getFunc()->getArgTypes()) {
//   //       auto n = N<NoneExpr>();
//   //       n->setType(a);
//   //       methodArgs->push_back({"", n});
//   //     }
//   //   }
//   // }

//   std::vector<FuncTypePtr> m;
//   // Use the provided arguments to select the best method
//   if (auto dot = cast<DotExpr>(expr)) {
//     // Case: method overloads (DotExpr)
//     auto methods =
//         ctx->findMethod(getType(dot->expr)->getClass().get(), dot->member, false);
//     if (methodArgs)
//     m = findMatchingMethods(getType(dot->expr)->getClass(), methods, *methodArgs);
//   } else if (auto id = cast<IdExpr>(expr)) {
//     // Case: function overloads (IdExpr)
//     std::vector<types::FuncTypePtr> methods;
//     auto key = id->getValue();
//     if (endswith(key, ":dispatch"))
//       key = key.substr(0, key.size() - 9);
//     for (auto &m : ctx->cache->overloads[key])
//       if (!endswith(m, ":dispatch"))
//         methods.push_back(ctx->cache->functions[m].type);
//     std::reverse(methods.begin(), methods.end());
//     m = findMatchingMethods(nullptr, methods, *methodArgs);
//   }

//   bool goDispatch = false;
//   if (m.size() == 1) {
//     return m[0];
//   } else if (m.size() > 1) {
//     for (auto &a : *methodArgs) {
//       if (auto u = a.value->getType()->getUnbound()) {
//         goDispatch = true;
//       }
//     }
//     if (!goDispatch)
//       return m[0];
//   }

//   if (goDispatch) {
//     // If overload is ambiguous, route through a dispatch function
//     std::string name;
//     if (auto dot = cast<DotExpr>(expr)) {
//       auto methods =
//           ctx->findMethod(getType(dot->expr)->getClass().get(), dot->member, false);
//       seqassert(!methods.empty(), "unknown method");
//       name = ctx->cache->functions[methods.back()->ast->name].rootName;
//     } else {
//       name = cast<IdExpr>(expr)->getValue();
//     }
//     auto t = getDispatch(name);
//   }

//   // Print a nice error message
//   std::string argsNice;
//   if (methodArgs) {
//     std::vector<std::string> a;
//     for (auto &t : *methodArgs)
//       a.emplace_back(fmt::format("{}", t.value->getType()->getStatic()
//                                            ? t.value->getClassType()->name
//                                            : t.value->getType()->prettyString()));
//     argsNice = fmt::format("({})", fmt::join(a, ", "));
//   }

//   if (auto dot = cast<DotExpr>(expr)) {
//     // Debug:
//     // *args = std::vector<CallArg>(args->begin()+1, args->end());
//     getBestOverload(expr, args);
//     E(Error::DOT_NO_ATTR_ARGS, expr, getType(dot->expr)->prettyString(), dot->member,
//       argsNice);
//   } else {
//     E(Error::FN_NO_ATTR_ARGS, expr, ctx->cache->rev(cast<IdExpr>(expr)->getValue()),
//       argsNice);
//   }

//   return nullptr;
// }

} // namespace codon::ast
