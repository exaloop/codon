// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

using namespace types;

/// Typecheck identifiers.
/// If an identifier is a static variable, evaluate it and replace it with its value
///   (e.g., change `N` to `IntExpr(16)`).
/// If the identifier of a generic is fully qualified, use its qualified name
///   (e.g., replace `Ptr` with `Ptr[byte]`).
void TypecheckVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->getValue(), getTime());
  if (!val) {
    E(Error::ID_NOT_FOUND, expr, expr->getValue());
  }

  // If this is an overload, use the dispatch function
  if (isUnbound(expr) && hasOverloads(val->getName())) {
    val = ctx->forceFind(getDispatch(val->getName())->getFuncName());
  }

  // If we are accessing an outside variable, capture it or raise an error
  auto captured = checkCapture(val);
  if (captured)
    val = ctx->forceFind(expr->getValue());

  // Replace the variable with its canonical name
  expr->value = val->getName();

  // Set up type
  unify(expr->getType(), instantiateType(val->getType()));
  if (auto f = expr->getType()->getFunc()) {
    expr->value = f->getFuncName(); // resolve overloads
  }

  // Realize a type or a function if possible and replace the identifier with
  // a qualified identifier or a static expression (e.g., `foo` -> `foo[int]`)
  if (expr->getType()->canRealize()) {
    if (auto s = expr->getType()->getStatic()) {
      resultExpr = transform(s->getStaticExpr());
      return;
    }
    if (!val->isVar()) {
      if (!(expr->hasAttribute(Attr::ExprDoNotRealize) && expr->getType()->getFunc())) {
        if (auto r = realize(expr->getType())) {
          expr->value = r->realizedName();
          expr->setDone();
        }
      }
    } else {
      realize(expr->getType());
      expr->setDone();
    }
  }

  // If this identifier needs __used__ checks (see @c ScopeVisitor), add them
  if (expr->hasAttribute(Attr::ExprDominatedUndefCheck)) {
    auto controlVar =
        fmt::format("{}{}", getUnmangledName(val->canonicalName), VAR_USED_SUFFIX);
    if (ctx->find(controlVar, getTime())) {
      auto checkStmt = N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(N<IdExpr>("__internal__"), "undef"), N<IdExpr>(controlVar),
          N<StringExpr>(getUnmangledName(val->canonicalName))));
      expr->eraseAttribute(Attr::ExprDominatedUndefCheck);
      resultExpr = transform(N<StmtExpr>(checkStmt, expr));
    }
  }
}

/// Transform a dot expression. Select the best method overload if possible.
/// @example
///   `obj.__class__`   -> `type(obj)`
///   `cls.__name__`    -> `"class"` (same for functions)
///   `obj.method`      -> `cls.method(obj, ...)` or
///                        `cls.method(obj)` if method has `@property` attribute
///   `obj.member`      -> see @c getClassMember
/// @return nullptr if no transformation was made
void TypecheckVisitor::visit(DotExpr *expr) {
  // Check if this is being called from CallExpr (e.g., foo.bar(...))
  // and mark it as such (to inline useless partial expression)
  CallExpr *parentCall = cast<CallExpr>(ctx->getParentNode());
  if (parentCall && !parentCall->hasAttribute(Attr::ParentCallExpr))
    parentCall = nullptr;

  // Flatten imports:
  //   `a.b.c`      -> canonical name of `c` in `a.b` if `a.b` is an import
  //   `a.B.c`      -> canonical name of `c` in class `a.B`
  //   `python.foo` -> internal.python._get_identifier("foo")
  std::vector<std::string> chain;
  Expr *head = expr;
  for (; cast<DotExpr>(head); head = cast<DotExpr>(head)->getExpr())
    chain.push_back(cast<DotExpr>(head)->getMember());
  Expr *final = expr;
  if (auto id = cast<IdExpr>(head)) {
    // Case: a.bar.baz
    chain.push_back(id->getValue());
    std::reverse(chain.begin(), chain.end());
    auto [pos, val] = getImport(chain);
    if (!val) {
      // Python capture
      seqassert(ctx->getBase()->pyCaptures, "unexpected py capture");
      ctx->getBase()->pyCaptures->insert(chain[0]);
      final = N<IndexExpr>(N<IdExpr>("__pyenv__"), N<StringExpr>(chain[0]));
    } else if (val->getModule() == "std.python" &&
               ctx->getModule() != val->getModule()) {
      // Import from python (e.g., pyobj.foo)
      final = transform(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(N<IdExpr>("internal"), "python"), "_get_identifier"),
          N<StringExpr>(chain[pos++])));
    } else if (val->getModule() == ctx->getModule() && pos == 1) {
      final = transform(N<IdExpr>(chain[0]), true);
    } else {
      final = N<IdExpr>(val->canonicalName);
    }
    while (pos < chain.size())
      final = N<DotExpr>(final, chain[pos++]);
  }
  if (auto dot = cast<DotExpr>(final)) {
    expr->expr = dot->getExpr();
    expr->member = dot->getMember();
  } else {
    resultExpr = transform(final);
    return;
  }

  // Special case: obj.__class__
  if (expr->getMember() == "__class__") {
    /// TODO: prevent cls.__class__ and type(cls)
    resultExpr = transform(N<CallExpr>(N<IdExpr>(TYPE_TYPE), expr->getExpr()));
    return;
  }
  expr->expr = transform(expr->getExpr());

  // Special case: fn.__name__
  // Should go before cls.__name__ to allow printing generic functions
  if (extractType(expr->getExpr())->getFunc() && expr->getMember() == "__name__") {
    resultExpr = transform(N<StringExpr>(extractType(expr->getExpr())->prettyString()));
    return;
  }
  if (expr->getExpr()->getType()->getPartial() && expr->getMember() == "__name__") {
    resultExpr = transform(
        N<StringExpr>(expr->getExpr()->getType()->getPartial()->prettyString()));
    return;
  }
  // Special case: fn.__llvm_name__ or obj.__llvm_name__
  if (expr->getMember() == "__llvm_name__") {
    if (realize(expr->getExpr()->getType()))
      resultExpr = transform(N<StringExpr>(expr->getExpr()->getType()->realizedName()));
    return;
  }
  // Special case: cls.__name__
  if (isTypeExpr(expr->getExpr()) && expr->getMember() == "__name__") {
    if (realize(expr->getExpr()->getType()))
      resultExpr =
          transform(N<StringExpr>(extractType(expr->getExpr())->prettyString()));
    return;
  }
  if (isTypeExpr(expr->getExpr()) && expr->getMember() == "__repr__") {
    resultExpr =
        transform(N<CallExpr>(N<IdExpr>("std.internal.internal.__type_repr__.0"),
                              expr->getExpr(), N<EllipsisExpr>()));
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
  if (isTypeExpr(expr->getExpr()) && expr->getMember() == "__id__") {
    if (auto c = realize(extractType(expr->getExpr())))
      resultExpr = transform(N<IntExpr>(getClassRealization(c)->id));
    return;
  }

  // Ensure that the type is known (otherwise wait until it becomes known)
  auto typ = extractClassType(expr->getExpr());
  if (!typ)
    return;

  // Check if this is a method or member access
  while (true) {
    auto methods = findMethod(typ, expr->getMember());
    if (methods.empty())
      resultExpr = getClassMember(expr);

    // If the expression changed during the @c getClassMember (e.g., optional unwrap),
    // keep going further to be able to select the appropriate method or member
    auto oldExpr = expr->getExpr();
    if (!resultExpr && expr->getExpr() != oldExpr) {
      typ = extractClassType(expr->getExpr());
      if (!typ)
        return; // delay typechecking
      continue;
    }

    if (!methods.empty()) {
      // If a method is ambiguous use dispatch
      auto bestMethod = methods.size() > 1 ? getDispatch(getRootName(methods.front()))
                                           : methods.front();
      Expr *e = N<IdExpr>(bestMethod->getFuncName());
      e->setType(instantiateType(bestMethod, typ));
      if (isTypeExpr(expr->getExpr())) {
        // Static access: `cls.method`
        unify(expr->getType(), e->getType());
      } else if (parentCall && !bestMethod->ast->hasAttribute(Attr::StaticMethod) &&
                 !bestMethod->ast->hasAttribute(Attr::Property)) {
        // Instance access: `obj.method` from the call
        // Modify the call to push `self` to the front of the argument list.
        // Avoids creating partial functions.
        parentCall->items.insert(parentCall->items.begin(), expr->getExpr());
        unify(expr->getType(), e->getType());
      } else {
        // Instance access: `obj.method`
        // Transform y.method to a partial call `type(y).method(y, ...)`
        std::vector<Expr *> methodArgs;
        // Do not add self if a method is marked with @staticmethod
        if (!bestMethod->ast->hasAttribute(Attr::StaticMethod))
          methodArgs.emplace_back(expr->getExpr());
        // If a method is marked with @property, just call it directly
        if (!bestMethod->ast->hasAttribute(Attr::Property))
          methodArgs.emplace_back(N<EllipsisExpr>(EllipsisExpr::PARTIAL));
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
    registerGlobal(val->getName(), true);
    return false;
  }

  // Check if a real variable (not a static) is defined outside the current scope
  if (crossCaptureBoundary)
    E(Error::ID_CANNOT_CAPTURE, getSrcInfo(), getUnmangledName(val->getName()));

  // Case: a nonlocal variable that has not been marked with `nonlocal` statement
  //       and capturing is *not* enabled
  E(Error::ID_NONLOCAL, getSrcInfo(), getUnmangledName(val->getName()));
  return false;
}

/// Check if a chain (a.b.c.d...) contains an import or a class prefix.
std::pair<size_t, TypeContext::Item>
TypecheckVisitor::getImport(const std::vector<std::string> &chain) {
  size_t importEnd = 0;
  std::string importName;

  // 1. Find the longest prefix that corresponds to the existing import
  //    (e.g., `a.b.c.d` -> `a.b.c` if there is `import a.b.c`)
  TypeContext::Item val = nullptr, importVal = nullptr;
  for (auto i = chain.size(); i-- > 0;) {
    auto name = join(chain, "/", 0, i + 1);
    val = ctx->find(name, getTime());
    if (val && val->type->is("Import") && startswith(val->getName(), "%_import_")) {
      importName = getStrLiteral(val->type.get());
      importEnd = i + 1;
      importVal = val;
      break;
    }
  }

  // Case: the whole chain is import itself
  if (importEnd == chain.size())
    return {importEnd, val};

  // Find the longest prefix that corresponds to the existing class
  // (e.g., `a.b.c` -> `a.b` if there is `class a: class b:`)
  std::string itemName;
  size_t itemEnd = 0;
  auto ictx = importName.empty() ? ctx : getImport(importName)->ctx;
  for (auto i = chain.size(); i-- > importEnd;) {
    if (ictx->getModule() == "std.python" && importEnd < chain.size()) {
      // Special case: importing from Python.
      // Fake TypecheckItem that indicates std.python access
      val = std::make_shared<TypecheckItem>(
          "", "", ictx->getModule(), TypecheckVisitor(ictx).instantiateUnbound());
      return {importEnd, val};
    } else {
      auto key = join(chain, ".", importEnd, i + 1);
      val = ictx->find(key);
      if (val && i + 1 != chain.size() && val->getType()->is("Import") &&
          startswith(val->getName(), "%_import_")) {
        importName = getStrLiteral(val->getType());
        importEnd = i + 1;
        importVal = val;
        ictx = getImport(importName)->ctx;
        i = chain.size();
        continue;
      }
      bool isOverload = val && val->isFunc() && hasOverloads(val->canonicalName);
      if (isOverload && importEnd == i) { // top-level overload
        itemName = val->canonicalName, itemEnd = i + 1;
        break;
      }
      // Class member
      if (val && !isOverload &&
          (importName.empty() || val->isType() || !val->isConditional())) {
        itemName = val->canonicalName, itemEnd = i + 1;
        break;
      }
      // Resolve the identifier from the import
      if (auto i = ctx->find("Import")) {
        auto t = extractClassType(i->getType());
        if (findMember(t, key))
          return {importEnd, importVal};
        if (!findMethod(t, key).empty())
          return {importEnd, importVal};
      }
    }
  }
  if (itemName.empty() && importName.empty()) {
    if (ctx->getBase()->pyCaptures)
      return {1, nullptr};
    E(Error::IMPORT_NO_MODULE, getSrcInfo(), chain[importEnd]);
  } else if (itemName.empty()) {
    auto import = getImport(importName);
    if (!ctx->isStdlibLoading && endswith(importName, "__init__.codon")) {
      // Special case: subimport is not yet loaded
      //   (e.g., import a; a.b.x where a.b is a module as well)
      auto file = getImportFile(getArgv(), chain[importEnd], importName, false,
                                getRootModulePath(), getPluginImportPaths());
      if (file) { // auto-load support
        Stmt *s = N<SuiteStmt>(N<ImportStmt>(N<IdExpr>(chain[importEnd]), nullptr));
        if (auto err = ScopingVisitor::apply(ctx->cache, s))
          throw exc::ParserException(std::move(err));
        s = TypecheckVisitor(import->ctx, preamble).transform(s);
        prependStmts->push_back(s);
        return getImport(chain);
      }
    }
    E(Error::IMPORT_NO_NAME, getSrcInfo(), chain[importEnd], import->name);
  }
  importEnd = itemEnd;
  return {importEnd, val};
}

/// Find or generate an overload dispatch function for a given overload.
///  Dispatch functions ensure that a function call is being routed to the correct
///  overload
/// even when dealing with partial functions and decorators.
/// @example This is how dispatch looks like:
///   ```def foo:dispatch(*args, **kwargs):
///        return foo(*args, **kwargs)```
types::FuncType *TypecheckVisitor::getDispatch(const std::string &fn) {
  auto &overloads = ctx->cache->overloads[fn];

  // Single overload: just return it
  if (overloads.size() == 1)
    return ctx->forceFind(overloads.front())->type->getFunc();

  // Check if dispatch exists
  for (auto &m : overloads)
    if (isDispatch(getFunction(m)->ast))
      return getFunction(m)->getType();

  // Dispatch does not exist. Generate it
  auto name = fmt::format("{}{}", fn, FN_DISPATCH_SUFFIX);
  Expr *root; // Root function name used for calling
  auto ofn = getFunction(overloads[0]);
  if (auto aa = ofn->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass))
    root = N<DotExpr>(N<IdExpr>(aa->value), getUnmangledName(fn));
  else
    root = N<IdExpr>(fn);
  root = N<CallExpr>(root, N<StarExpr>(N<IdExpr>("args")),
                     N<KeywordStarExpr>(N<IdExpr>("kwargs")));
  auto nar = ctx->generateCanonicalName("args");
  auto nkw = ctx->generateCanonicalName("kwargs");
  auto ast = N<FunctionStmt>(name, nullptr,
                             std::vector<Param>{Param("*" + nar), Param("**" + nkw)},
                             N<SuiteStmt>(N<ReturnStmt>(root)));
  ast->setAttribute(Attr::AutoGenerated);
  ast->setAttribute(Attr::Module, ctx->moduleName.path);
  ctx->cache->reverseIdentifierLookup[name] = getUnmangledName(fn);

  auto baseType = getFuncTypeBase(2);
  auto typ = std::make_shared<FuncType>(baseType.get(), ast, 0);
  /// Make sure that parent is set so that the parent type can be passed to the inner
  /// call
  ///   (e.g., A[B].foo -> A.foo.dispatch() { A[B].foo() })
  typ->funcParent = ofn->type->funcParent;
  typ = std::static_pointer_cast<FuncType>(typ->generalize(ctx->typecheckLevel - 1));
  ctx->addFunc(name, name, typ);

  overloads.insert(overloads.begin(), name);
  ctx->cache->functions[name] = Cache::Function{"", fn, ast, typ};
  ast->setDone();
  return typ.get(); // stored in Cache::Function, hence not destroyed
}

/// Find a class member.
/// @example
///   `obj.GENERIC`     -> `GENERIC` (IdExpr with generic/static value)
///   `optional.member` -> `unwrap(optional).member`
///   `pyobj.member`    -> `pyobj._getattr("member")`
Expr *TypecheckVisitor::getClassMember(DotExpr *expr) {
  auto typ = extractClassType(expr->getExpr());
  seqassert(typ, "not a class");

  // Case: object member access (`obj.member`)
  if (!isTypeExpr(expr->getExpr())) {
    if (auto member = findMember(typ, expr->getMember())) {
      unify(expr->getType(), instantiateType(member->getType(), typ));
      if (!expr->getType()->canRealize() && member->typeExpr) {
        unify(expr->getType(), extractType(withClassGenerics(typ, [&]() {
                return transform(clean_clone(member->typeExpr));
              })));
      }
      if (expr->getExpr()->isDone() && realize(expr->getType()))
        expr->setDone();
      return nullptr;
    }
  }

  // Case: class variable (`Cls.var`)
  if (auto cls = getClass(typ))
    if (auto var = in(cls->classVars, expr->getMember())) {
      return transform(N<IdExpr>(*var));
    }

  // Case: special members
  std::unordered_map<std::string, std::string> specialMembers{
      {"__elemsize__", "int"}, {"__atomic__", "bool"}, {"__contents_atomic__", "bool"}};
  if (auto mtyp = in(specialMembers, expr->getMember())) {
    unify(expr->getType(), getStdLibType(*mtyp));
    if (expr->getExpr()->isDone() && realize(expr->getType()))
      expr->setDone();
    return nullptr;
  }
  if (expr->getMember() == "__name__" && isTypeExpr(expr->getExpr())) {
    unify(expr->getType(), getStdLibType("str"));
    if (expr->getExpr()->isDone() && realize(expr->getType()))
      expr->setDone();
    return nullptr;
  }

  // Case: object generic access (`obj.T`)
  ClassType::Generic *generic = nullptr;
  for (auto &g : typ->generics)
    if (expr->getMember() == getUnmangledName(g.name)) {
      generic = &g;
      break;
    }
  if (generic) {
    if (generic->isStatic) {
      unify(expr->getType(), generic->getType());
      if (realize(expr->getType())) {
        return transform(generic->type->getStatic()->getStaticExpr());
      }
    } else {
      unify(expr->getType(), instantiateTypeVar(generic->getType()));
      if (realize(expr->getType()))
        return transform(N<IdExpr>(generic->getType()->realizedName()));
    }
    return nullptr;
  }

  // Case: transform `optional.member` to `unwrap(optional).member`
  if (typ->is(TYPE_OPTIONAL)) {
    expr->expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->getExpr()));
    return nullptr;
  }

  // Case: transform `pyobj.member` to `pyobj._getattr("member")`
  if (typ->is("pyobj")) {
    return transform(N<CallExpr>(N<DotExpr>(expr->getExpr(), "_getattr"),
                                 N<StringExpr>(expr->getMember())));
  }

  // Case: transform `union.m` to `__internal__.get_union_method(union, "m", ...)`
  if (typ->getUnion()) {
    if (!typ->canRealize())
      return nullptr; // delay!
    return transform(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("__internal__"), "union_member"),
        std::vector<CallArg>{{"union", expr->getExpr()},
                             {"member", N<StringExpr>(expr->getMember())}}));
  }

  // For debugging purposes:
  findMethod(typ, expr->getMember());
  E(Error::DOT_NO_ATTR, expr, typ->prettyString(), expr->getMember());
  return nullptr;
}

} // namespace codon::ast
