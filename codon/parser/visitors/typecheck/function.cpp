// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <string>
#include <tuple>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;
using namespace matcher;

/// Unify the function return type with `Generator[?]`.
/// The unbound type will be deduced from return/yield statements.
void TypecheckVisitor::visit(LambdaExpr *expr) {
  std::vector<Param> params;
  std::string name = getTemporaryVar("lambda");
  params.reserve(expr->size());
  for (auto &s : *expr)
    params.emplace_back(s);
  Stmt *f = N<FunctionStmt>(name, nullptr, params,
                            N<SuiteStmt>(N<ReturnStmt>(expr->getExpr())));
  if (auto err = ScopingVisitor::apply(ctx->cache, N<SuiteStmt>(f)))
    throw exc::ParserException(std::move(err));
  f->setAttribute(Attr::ExprTime, getTime()); // to handle captures properly
  f = transform(f);
  if (auto a = expr->getAttribute(Attr::Bindings))
    f->setAttribute(Attr::Bindings, a->clone());
  prependStmts->push_back(f);
  resultExpr = transform(N<CallExpr>(N<IdExpr>(name), N<EllipsisExpr>()));
}

/// Unify the function return type with `Generator[?]`.
/// The unbound type will be deduced from return/yield statements.
void TypecheckVisitor::visit(YieldExpr *expr) {
  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, expr, "yield");

  unify(ctx->getBase()->returnType.get(),
        instantiateType(getStdLibType("Generator"), {expr->getType()}));
  if (realize(expr->getType()))
    expr->setDone();
}

/// Typecheck return statements. Empty return is transformed to `return NoneType()`.
/// Also partialize functions if they are being returned.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(ReturnStmt *stmt) {
  if (stmt->hasAttribute(Attr::Internal)) {
    stmt->expr = transform(N<CallExpr>(N<IdExpr>("NoneType.__new__:0")));
    stmt->setDone();
    return;
  }

  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, stmt, "return");

  if (!stmt->expr && ctx->getBase()->func->hasAttribute(Attr::IsGenerator)) {
    stmt->setDone();
  } else {
    if (!stmt->expr)
      stmt->expr = N<CallExpr>(N<IdExpr>("NoneType"));
    stmt->expr = transform(stmt->getExpr());

    // Wrap expression to match the return type
    if (!ctx->getBase()->returnType->getUnbound())
      if (!wrapExpr(&stmt->expr, ctx->getBase()->returnType.get())) {
        return;
      }

    // Special case: partialize functions if we are returning them
    if (stmt->getExpr()->getType()->getFunc() &&
        !(ctx->getBase()->returnType->getClass() &&
          ctx->getBase()->returnType->is("Function"))) {
      stmt->expr = transform(partializeFunction(stmt->getExpr()->getType()->getFunc()));
    }

    if (!ctx->getBase()->returnType->isStaticType() &&
        stmt->getExpr()->getType()->getStatic())
      stmt->getExpr()->setType(stmt->getExpr()
                                   ->getType()
                                   ->getStatic()
                                   ->getNonStaticType()
                                   ->shared_from_this());
    unify(ctx->getBase()->returnType.get(), stmt->getExpr()->getType());
  }

  // If we are not within conditional block, ignore later statements in this function.
  // Useful with static if statements.
  if (!ctx->blockLevel)
    ctx->returnEarly = true;

  if (!stmt->getExpr() || stmt->getExpr()->isDone())
    stmt->setDone();
}

/// Typecheck yield statements. Empty yields assume `NoneType`.
void TypecheckVisitor::visit(YieldStmt *stmt) {
  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, stmt, "yield");

  stmt->expr =
      transform(stmt->getExpr() ? stmt->getExpr() : N<CallExpr>(N<IdExpr>("NoneType")));
  unify(ctx->getBase()->returnType.get(),
        instantiateType(getStdLibType("Generator"), {stmt->getExpr()->getType()}));

  if (stmt->getExpr()->isDone())
    stmt->setDone();
}

/// Transform `yield from` statements.
/// @example
///   `yield from a` -> `for var in a: yield var`
void TypecheckVisitor::visit(YieldFromStmt *stmt) {
  auto var = getTemporaryVar("yield");
  resultStmt = transform(
      N<ForStmt>(N<IdExpr>(var), stmt->getExpr(), N<YieldStmt>(N<IdExpr>(var))));
}

/// Process `global` statements. Remove them upon completion.
void TypecheckVisitor::visit(GlobalStmt *stmt) { resultStmt = N<SuiteStmt>(); }

/// Parse a function stub and create a corresponding generic function type.
/// Also realize built-ins and extern C functions.
void TypecheckVisitor::visit(FunctionStmt *stmt) {
  if (stmt->hasAttribute(Attr::Python)) {
    // Handle Python block
    resultStmt =
        transformPythonDefinition(stmt->getName(), stmt->items, stmt->getReturn(),
                                  stmt->getSuite()->firstInBlock());
    return;
  }
  auto origStmt = clean_clone(stmt);

  // Parse attributes
  std::vector<std::string> attributes;
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (!stmt->decorators[i])
      continue;
    auto [isAttr, attrName] = getDecorator(stmt->decorators[i]);
    if (!attrName.empty()) {
      if (isAttr) {
        if (attrName == "std.internal.attributes.realize_without_self.0:0")
          stmt->setAttribute(Attr::RealizeWithoutSelf);
        else if (attrName == "std.internal.attributes.test.0:0")
          stmt->setAttribute(Attr::Test);
        else if (attrName == "std.internal.attributes.export.0:0")
          stmt->setAttribute(Attr::Export);
        else if (attrName == "std.internal.attributes.inline.0:0")
          stmt->setAttribute(Attr::Inline);
        else if (attrName == "std.internal.attributes.no_arg_reorder.0:0")
          stmt->setAttribute(Attr::NoArgReorder);
        else if (attrName == "overload:0")
          stmt->setAttribute(Attr::Overload);

        if (!stmt->hasAttribute(Attr::FunctionAttributes))
          stmt->setAttribute(Attr::FunctionAttributes,
                             std::make_unique<ir::KeyValueAttribute>());
        stmt->getAttribute<ir::KeyValueAttribute>(Attr::FunctionAttributes)
            ->attributes[attrName] = "";
        stmt->decorators[i] = nullptr; // remove it from further consideration
      }
    }
  }

  bool isClassMember = ctx->inClass();
  if (stmt->hasAttribute(Attr::ForceRealize) && (!ctx->isGlobal() || isClassMember))
    E(Error::EXPECTED_TOPLEVEL, getSrcInfo(), "builtin function");

  // All overloads share the same canonical name except for the number at the
  // end (e.g., `foo.1:0`, `foo.1:1` etc.)
  std::string rootName;
  if (isClassMember) {
    // Case 1: method overload
    if (auto n = in(getClass(ctx->getBase()->name)->methods, stmt->getName()))
      rootName = *n;
  } else if (stmt->hasAttribute(Attr::Overload)) {
    // Case 2: function overload
    if (auto c = ctx->find(stmt->getName(), getTime())) {
      if (c->isFunc() && c->getModule() == ctx->getModule() &&
          c->getBaseName() == ctx->getBaseName()) {
        rootName = c->canonicalName;
      }
    }
  }
  if (rootName.empty())
    rootName = ctx->generateCanonicalName(stmt->getName(), true, isClassMember);
  // Append overload number to the name
  auto canonicalName = rootName;
  if (!in(ctx->cache->overloads, rootName))
    ctx->cache->overloads.insert({rootName, {}});
  canonicalName += format(":{}", getOverloads(rootName).size());
  ctx->cache->reverseIdentifierLookup[canonicalName] = stmt->getName();

  if (isClassMember) {
    // Set the enclosing class name
    stmt->setAttribute(Attr::ParentClass, ctx->getBase()->name);
    // Add the method to the class' method list
    getClass(ctx->getBase()->name)->methods[stmt->getName()] = rootName;
  }

  // Handle captures. Add additional argument to the function for every capture.
  // Make sure to account for **kwargs if present
  std::map<std::string, TypeContext::Item> captures;
  if (auto b = stmt->getAttribute<BindingsAttribute>(Attr::Bindings))
    for (auto &[c, t] : b->captures) {
      if (auto v = ctx->find(c, getTime())) {
        if (t != BindingsAttribute::CaptureType::Global && !v->isGlobal()) {
          bool parentClassGeneric =
              ctx->bases.back().isType() && ctx->bases.back().name == v->getBaseName();
          if (v->isGeneric() && parentClassGeneric) {
            stmt->setAttribute(Attr::Method);
          }
          if (!v->isGeneric() || (v->isStatic() && !parentClassGeneric)) {
            if (!v->isFunc()) {
              // log("capture: {} -> {} '' {}", canonicalName, c,
              //     v->getType()->debugString(2));
              captures[c] = v;
            }
          }
        }
      }
    }
  std::vector<CallArg> partialArgs;
  if (!captures.empty()) {
    std::vector<std::string> itemKeys;
    itemKeys.reserve(captures.size());
    for (const auto &[key, _] : captures)
      itemKeys.emplace_back(key);

    // Handle partial arguments (and static special cases)
    Param kw;
    if (!stmt->empty() && startswith(stmt->back().name, "**")) {
      kw = stmt->back();
      stmt->items.pop_back();
    }
    std::array<const char *, 4> op{"", "int", "str", "bool"};
    for (auto &[c, v] : captures) {
      if (v->isType()) {
        stmt->items.emplace_back(c, N<IdExpr>(TYPE_TYPE));
      } else if (auto si = v->isStatic()) {
        stmt->items.emplace_back(c,
                                 N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>(op[si])));
      } else {
        stmt->items.emplace_back(c);
      }
      if (v->isFunc()) {
        partialArgs.emplace_back(c,
                                 N<CallExpr>(N<IdExpr>(v->canonicalName),
                                             N<EllipsisExpr>(EllipsisExpr::PARTIAL)));
      } else {
        partialArgs.emplace_back(c, N<IdExpr>(v->canonicalName));
      }
    }
    if (!kw.name.empty())
      stmt->items.emplace_back(kw);
    partialArgs.emplace_back("", N<EllipsisExpr>(EllipsisExpr::PARTIAL));
  }

  std::vector<Param> args;
  Stmt *suite = nullptr;
  Expr *ret = nullptr;
  std::vector<ClassType::Generic> explicits;
  std::shared_ptr<types::ClassType> baseType = nullptr;
  {
    // Set up the base
    TypeContext::BaseGuard br(ctx.get(), canonicalName);
    ctx->getBase()->func = stmt;

    // Parse arguments and add them to the context
    for (auto &a : *stmt) {
      auto [stars, varName] = a.getNameWithStars();
      auto name = ctx->generateCanonicalName(varName);

      // Mark as method if the first argument is self
      if (isClassMember && stmt->hasAttribute(Attr::HasSelf) && a.getName() == "self")
        stmt->setAttribute(Attr::Method);

      // Handle default values
      auto defaultValue = a.getDefault();
      if (a.getType() && defaultValue && cast<NoneExpr>(defaultValue)) {
        // Special case: `arg: Callable = None` -> `arg: Callable = NoneType()`
        if (match(a.getType(), M<IndexExpr>(M<IdExpr>(TYPE_CALLABLE), M_)))
          defaultValue = N<CallExpr>(N<IdExpr>("NoneType"));
        // Special case: `arg: type = None` -> `arg: type = NoneType`
        if (match(a.getType(), M<IdExpr>(MOr(TYPE_TYPE, TYPE_TYPEVAR))))
          defaultValue = N<IdExpr>("NoneType");
      }
      /// TODO: Python-style defaults
      args.emplace_back(std::string(stars, '*') + name, a.getType(), defaultValue,
                        a.status);

      // Add generics to the context
      if (!a.isValue()) {
        // Generic and static types
        auto generic = instantiateUnbound();
        auto typId = generic->getLink()->id;
        generic->genericName = varName;
        auto defType = transform(clone(a.getDefault()));
        if (auto st = getStaticGeneric(a.getType())) {
          auto val = ctx->addVar(varName, name, generic);
          val->generic = true;
          generic->isStatic = st;
          if (defType)
            generic->defaultType = extractType(defType)->shared_from_this();
        } else {
          if (match(a.getType(), M<InstantiateExpr>(M<IdExpr>(TYPE_TYPEVAR), M_))) {
            // Parse TraitVar
            auto l =
                transformType(cast<InstantiateExpr>(a.getType())->front())->getType();
            if (l->getLink() && l->getLink()->trait)
              generic->getLink()->trait = l->getLink()->trait;
            else
              generic->getLink()->trait =
                  std::make_shared<types::TypeTrait>(l->shared_from_this());
          }
          auto val = ctx->addType(varName, name, generic);
          val->generic = true;
          if (defType)
            generic->defaultType = extractType(defType)->shared_from_this();
        }
        auto g = generic->generalize(ctx->typecheckLevel);
        explicits.emplace_back(name, varName, g, typId, g->isStaticType());
      }
    }

    // Prepare list of all generic types
    ClassType *parentClass = nullptr;
    if (isClassMember && stmt->hasAttribute(Attr::Method)) {
      // Get class generics (e.g., T for `class Cls[T]: def foo:`)
      auto aa = stmt->getAttribute<ir::StringValueAttribute>(Attr::ParentClass);
      parentClass = extractClassType(aa->value);
    }
    // Add function generics
    std::vector<TypePtr> generics;
    generics.reserve(explicits.size());
    for (const auto &i : explicits)
      generics.emplace_back(extractType(i.name)->shared_from_this());

    // Handle function arguments
    // Base type: `Function[[args,...], ret]`
    baseType = getFuncTypeBase(stmt->size() - explicits.size());
    ctx->typecheckLevel++;

    // Parse arguments to the context. Needs to be done after adding generics
    // to support cases like `foo(a: T, T: type)`
    for (auto &a : args) {
      a.type = transformType(a.getType(), false);
      a.defaultValue = transform(a.getDefault(), true);
    }

    // Unify base type generics with argument types. Add non-generic arguments to the
    // context. Delayed to prevent cases like `def foo(a, b=a)`
    auto argType = extractClassGeneric(baseType.get())->getClass();
    for (int ai = 0, aj = 0; ai < stmt->size(); ai++) {
      if (!(*stmt)[ai].isValue())
        continue;
      auto [_, canName] = (*stmt)[ai].getNameWithStars();
      if (!(*stmt)[ai].getType()) {
        if (parentClass && ai == 0 && (*stmt)[ai].getName() == "self") {
          // Special case: self in methods
          unify(extractClassGeneric(argType, aj), parentClass);
        } else {
          generics.push_back(extractClassGeneric(argType, aj)->shared_from_this());
        }
      } else if (startswith((*stmt)[ai].getName(), "*")) {
        // Special case: `*args: type` and `**kwargs: type`. Do not add this type to the
        // signature (as the real type is `Tuple[type, ...]`); it will be used during
        // call typechecking
        generics.push_back(extractClassGeneric(argType, aj)->shared_from_this());
      } else {
        unify(extractClassGeneric(argType, aj),
              extractType(transformType((*stmt)[ai].getType())));
      }
      aj++;
    }

    // Parse the return type
    ret = transformType(stmt->getReturn(), false);
    auto retType = extractClassGeneric(baseType.get(), 1);
    if (ret) {
      unify(retType, extractType(ret));
      if (isId(ret, "Union"))
        extractClassGeneric(retType)->getUnbound()->kind = LinkType::Generic;
    } else {
      generics.push_back(unify(retType, instantiateUnbound())->shared_from_this());
    }
    ctx->typecheckLevel--;

    // Generalize generics and remove them from the context
    for (const auto &g : generics) {
      for (auto &u : g->getUnbounds())
        if (u->getUnbound()) {
          u->getUnbound()->kind = LinkType::Generic;
        }
    }

    // Parse function body
    if (!stmt->hasAttribute(Attr::Internal) && !stmt->hasAttribute(Attr::C)) {
      if (stmt->hasAttribute(Attr::LLVM)) {
        suite = transformLLVMDefinition(stmt->getSuite()->firstInBlock());
      } else if (stmt->hasAttribute(Attr::C)) {
        // Do nothing
      } else {
        suite = clone(stmt->getSuite());
      }
    }
  }
  stmt->setAttribute(Attr::Module, ctx->moduleName.path);

  // Make function AST and cache it for later realization
  auto f = N<FunctionStmt>(canonicalName, ret, args, suite);
  f->cloneAttributesFrom(stmt);
  auto &fn = ctx->cache->functions[canonicalName] =
      Cache::Function{ctx->getModulePath(),
                      rootName,
                      f,
                      nullptr,
                      origStmt,
                      ctx->getModule().empty() && ctx->isGlobal()};
  f->setDone();
  auto aa = stmt->getAttribute<ir::StringValueAttribute>(Attr::ParentClass);
  auto parentClass = aa ? extractClassType(aa->value) : nullptr;

  // Construct the type
  auto funcTyp =
      std::make_shared<types::FuncType>(baseType.get(), fn.ast, 0, explicits);
  funcTyp->setSrcInfo(getSrcInfo());
  if (isClassMember && stmt->hasAttribute(Attr::Method)) {
    funcTyp->funcParent = parentClass->shared_from_this();
  }
  funcTyp = std::static_pointer_cast<types::FuncType>(
      funcTyp->generalize(ctx->typecheckLevel));
  fn.type = funcTyp;

  auto &overloads = ctx->cache->overloads[rootName];
  if (rootName == "Tuple.__new__") {
    overloads.insert(
        std::upper_bound(overloads.begin(), overloads.end(), canonicalName,
                         [&](const auto &a, const auto &b) {
                           return getFunction(a)->getType()->funcGenerics.size() <
                                  getFunction(b)->getType()->funcGenerics.size();
                         }),
        canonicalName);
  } else {
    overloads.push_back(canonicalName);
  }

  auto val = ctx->addFunc(stmt->name, rootName, funcTyp);
  // val->time = getTime();
  ctx->addFunc(canonicalName, canonicalName, funcTyp);
  if (stmt->hasAttribute(Attr::Overload) || isClassMember) {
    ctx->remove(stmt->name); // first overload will handle it!
  }

  // Special method handling
  if (isClassMember) {
    auto m = getClassMethod(parentClass, getUnmangledName(canonicalName));
    bool found = false;
    for (auto &i : getOverloads(m))
      if (i == canonicalName) {
        getFunction(i)->type = funcTyp;
        found = true;
        break;
      }
    seqassert(found, "cannot find matching class method for {}", canonicalName);
  } else {
    // Hack so that we can later use same helpers for class overloads
    getClass(VAR_CLASS_TOPLEVEL)->methods[stmt->getName()] = rootName;
  }

  // Ensure that functions with @C, @force_realize, and @export attributes can be
  // realized
  if (stmt->hasAttribute(Attr::ForceRealize) || stmt->hasAttribute(Attr::Export) ||
      (stmt->hasAttribute(Attr::C) && !stmt->hasAttribute(Attr::CVarArg))) {
    if (!funcTyp->canRealize())
      E(Error::FN_REALIZE_BUILTIN, stmt);
  }

  // Expression to be used if function binding is modified by captures or decorators
  Expr *finalExpr = nullptr;
  Expr *selfAssign = nullptr;
  // If there are captures, replace `fn` with `fn(cap1=cap1, cap2=cap2, ...)`
  if (!captures.empty()) {
    if (isClassMember)
      E(Error::ID_CANNOT_CAPTURE, getSrcInfo(), captures.begin()->first);

    finalExpr = N<CallExpr>(N<IdExpr>(canonicalName), partialArgs);
    // Add updated self reference in case function is recursive!
    auto pa = partialArgs;
    for (auto &a : pa) {
      if (!a.getName().empty())
        a.value = N<IdExpr>(a.getName());
      else
        a.value = clone(a.getExpr());
    }
    // todo)) right now this adds a capture hook for recursive calls
    selfAssign = N<CallExpr>(N<IdExpr>(stmt->getName()), pa);
  }

  // Parse remaining decorators
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (stmt->decorators[i]) {
      if (isClassMember)
        E(Error::FN_NO_DECORATORS, stmt->decorators[i]);
      // Replace each decorator with `decorator(finalExpr)` in the reverse order
      finalExpr = N<CallExpr>(stmt->decorators[i],
                              finalExpr ? finalExpr : N<IdExpr>(canonicalName));
      // selfAssign = N<CallExpr>(clone(stmt->decorators[i]),
      //                          selfAssign ? selfAssign : N<IdExpr>(canonicalName));
    }
  }

  if (selfAssign)
    f->suite =
        N<SuiteStmt>(N<AssignStmt>(N<IdExpr>(stmt->getName()), selfAssign), suite);
  if (finalExpr) {
    resultStmt = N<SuiteStmt>(
        f, transform(N<AssignStmt>(N<IdExpr>(stmt->getName()), finalExpr)));
  } else {
    resultStmt = f;
  }
}

/// Transform Python code blocks.
/// @example
///   ```@python
///      def foo(x: int, y) -> int:
///        [code]
///   ``` -> ```
///      pyobj._exec("def foo(x, y): [code]")
///      from python import __main__.foo(int, _) -> int
///   ```
Stmt *TypecheckVisitor::transformPythonDefinition(const std::string &name,
                                                  const std::vector<Param> &args,
                                                  Expr *ret, Stmt *codeStmt) {
  seqassert(codeStmt && cast<ExprStmt>(codeStmt) &&
                cast<StringExpr>(cast<ExprStmt>(codeStmt)->getExpr()),
            "invalid Python definition");

  auto code = cast<StringExpr>(cast<ExprStmt>(codeStmt)->getExpr())->getValue();
  std::vector<std::string> pyargs;
  pyargs.reserve(args.size());
  for (const auto &a : args)
    pyargs.emplace_back(a.getName());
  code = format("def {}({}):\n{}\n", name, join(pyargs, ", "), code);
  return transform(N<SuiteStmt>(
      N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(N<IdExpr>("pyobj"), "_exec"), N<StringExpr>(code))),
      N<ImportStmt>(N<IdExpr>("python"), N<DotExpr>(N<IdExpr>("__main__"), name),
                    clone(args), ret ? clone(ret) : N<IdExpr>("pyobj"))));
}

/// Transform LLVM functions.
/// @example
///   ```@llvm
///      def foo(x: int) -> float:
///        [code]
///   ``` -> ```
///      def foo(x: int) -> float:
///        StringExpr("[code]")
///        SuiteStmt(referenced_types)
///   ```
/// As LLVM code can reference types and static expressions in `{=expr}` blocks,
/// all block expression will be stored in the `referenced_types` suite.
/// "[code]" is transformed accordingly: each `{=expr}` block will
/// be replaced with `{}` so that @c fmt::format can fill the gaps.
/// Note that any brace (`{` or `}`) that is not part of a block is
/// escaped (e.g. `{` -> `{{` and `}` -> `}}`) so that @c fmt::format can process them.
Stmt *TypecheckVisitor::transformLLVMDefinition(Stmt *codeStmt) {
  StringExpr *codeExpr;
  auto m = match(codeStmt, M<ExprStmt>(MVar<StringExpr>(codeExpr)));
  seqassert(m, "invalid LLVM definition");
  auto code = codeExpr->getValue();

  std::vector<Stmt *> items;
  std::string finalCode;
  items.push_back(nullptr);

  // Parse LLVM code and look for expression blocks that start with `{=`
  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < code.size(); i++) {
    if (i < code.size() - 1 && code[i] == '{' && code[i + 1] == '=') {
      if (braceStart < i)
        finalCode += escapeFStringBraces(code, braceStart, i - braceStart) + '{';
      if (!braceCount) {
        braceStart = i + 2;
        braceCount++;
      } else {
        E(Error::FN_BAD_LLVM, getSrcInfo());
      }
    } else if (braceCount && code[i] == '}') {
      braceCount--;
      std::string exprCode = code.substr(braceStart, i - braceStart);
      auto offset = getSrcInfo();
      offset.col += i;
      auto exprOrErr = parseExpr(ctx->cache, exprCode, offset);
      if (!exprOrErr)
        throw exc::ParserException(exprOrErr.takeError());
      auto expr = transform(exprOrErr->first, true);
      items.push_back(N<ExprStmt>(expr));
      braceStart = i + 1;
      finalCode += '}';
    }
  }
  if (braceCount)
    E(Error::FN_BAD_LLVM, getSrcInfo());
  if (braceStart != code.size())
    finalCode += escapeFStringBraces(code, braceStart, int(code.size()) - braceStart);
  items[0] = N<ExprStmt>(N<StringExpr>(finalCode));
  return N<SuiteStmt>(items);
}

/// Fetch a decorator canonical name. The first pair member indicates if a decorator is
/// actually an attribute (a function with `@__attribute__`).
std::pair<bool, std::string> TypecheckVisitor::getDecorator(Expr *e) {
  auto dt = transform(clone(e));
  auto id = cast<IdExpr>(cast<CallExpr>(dt) ? cast<CallExpr>(dt)->getExpr() : dt);
  if (id) {
    auto ci = ctx->find(id->getValue(), getTime());
    if (ci && ci->isFunc()) {
      auto fn = ci->getName();
      auto f = getFunction(fn);
      if (!f) {
        if (auto o = in(ctx->cache->overloads, fn)) {
          if (o->size() == 1)
            f = getFunction(o->front());
        }
      }
      if (f)
        return {f->ast->hasAttribute(Attr::Attribute), ci->getName()};
    }
  }
  return {false, ""};
}

/// Make an empty partial call `fn(...)` for a given function.
Expr *TypecheckVisitor::partializeFunction(types::FuncType *fn) {
  // Create function mask
  std::vector<char> mask(fn->ast->size(), 0);
  for (int i = 0, j = 0; i < fn->ast->size(); i++)
    if ((*fn->ast)[i].isGeneric()) {
      if (!extractFuncGeneric(fn, j)->getUnbound())
        mask[i] = 1;
      j++;
    }

  // Generate partial class
  return generatePartialCall(mask, fn);
}

/// Generate and return `Function[Tuple[args...], ret]` type
std::shared_ptr<ClassType> TypecheckVisitor::getFuncTypeBase(size_t nargs) {
  auto baseType = instantiateType(getStdLibType("Function"));
  unify(extractClassGeneric(baseType->getClass()),
        instantiateType(generateTuple(nargs, false)));
  return std::static_pointer_cast<types::ClassType>(baseType);
}

} // namespace codon::ast
