// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

using namespace types;

/// Unify the function return type with `Generator[?]`.
/// The unbound type will be deduced from return/yield statements.
void TypecheckVisitor::visit(LambdaExpr *expr) {
  std::vector<Param> params;
  std::string name = ctx->cache->getTemporaryVar("lambda");
  params.reserve(expr->vars.size());
  for (auto &s : expr->vars)
    params.emplace_back(s);
  auto f =
      N<FunctionStmt>(name, nullptr, params, N<SuiteStmt>(N<ReturnStmt>(expr->expr)));
  if (auto a = expr->getAttribute(Attr::Bindings))
    f->setAttribute(Attr::Bindings, a->clone());
  resultExpr =
      transform(N<StmtExpr>(f, N<CallExpr>(N<IdExpr>(name), N<EllipsisExpr>())));
}

/// Unify the function return type with `Generator[?]`.
/// The unbound type will be deduced from return/yield statements.
void TypecheckVisitor::visit(YieldExpr *expr) {
  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, expr, "yield");

  unify(expr->type, ctx->getUnbound());
  unify(ctx->getBase()->returnType,
        ctx->instantiateGeneric(ctx->getType("Generator"), {expr->type}));
  if (realize(expr->type))
    expr->setDone();
}

/// Typecheck return statements. Empty return is transformed to `return NoneType()`.
/// Also partialize functions if they are being returned.
/// See @c wrapExpr for more details.
void TypecheckVisitor::visit(ReturnStmt *stmt) {
  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, stmt, "return");

  if (!stmt->expr && ctx->getBase()->func->hasAttribute(Attr::IsGenerator)) {
    stmt->setDone();
  } else {
    if (!stmt->expr)
      stmt->expr = N<CallExpr>(N<IdExpr>("NoneType"));
    stmt->expr = transform(stmt->expr);

    // Wrap expression to match the return type
    if (!ctx->getBase()->returnType->getUnbound())
      if (!wrapExpr(&stmt->expr, ctx->getBase()->returnType)) {
        return;
      }

    // Special case: partialize functions if we are returning them
    if (stmt->expr->getType()->getFunc() &&
        !(ctx->getBase()->returnType->getClass() &&
          ctx->getBase()->returnType->is("Function"))) {
      stmt->expr = partializeFunction(stmt->expr->type->getFunc());
    }

    if (!ctx->getBase()->returnType->isStaticType() && stmt->expr->type->getStatic())
      stmt->expr->type = stmt->expr->type->getStatic()->getNonStaticType();
    unify(ctx->getBase()->returnType, stmt->expr->type);
  }

  // If we are not within conditional block, ignore later statements in this function.
  // Useful with static if statements.
  if (!ctx->blockLevel)
    ctx->returnEarly = true;

  if (!stmt->expr || stmt->expr->isDone())
    stmt->setDone();
}

/// Typecheck yield statements. Empty yields assume `NoneType`.
void TypecheckVisitor::visit(YieldStmt *stmt) {
  if (!ctx->inFunction())
    E(Error::FN_OUTSIDE_ERROR, stmt, "yield");

  stmt->expr = transform(stmt->expr ? stmt->expr : N<CallExpr>(N<IdExpr>("NoneType")));
  auto t = ctx->instantiateGeneric(ctx->getType("Generator"), {stmt->expr->type});
  unify(ctx->getBase()->returnType, t);

  if (stmt->expr->isDone())
    stmt->setDone();
}

/// Transform `yield from` statements.
/// @example
///   `yield from a` -> `for var in a: yield var`
void TypecheckVisitor::visit(YieldFromStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("yield");
  resultStmt =
      transform(N<ForStmt>(N<IdExpr>(var), stmt->expr, N<YieldStmt>(N<IdExpr>(var))));
}

/// Process `global` statements. Remove them upon completion.
void TypecheckVisitor::visit(GlobalStmt *stmt) { resultStmt = N<SuiteStmt>(); }

/// Parse a function stub and create a corresponding generic function type.
/// Also realize built-ins and extern C functions.
void TypecheckVisitor::visit(FunctionStmt *stmt) {
  if (stmt->hasAttribute(Attr::Python)) {
    // Handle Python block
    resultStmt = transformPythonDefinition(stmt->name, stmt->args, stmt->ret,
                                           stmt->suite->firstInBlock());
    return;
  }
  auto stmt_clone = clone(stmt, true); // clean clone

  // Parse attributes
  std::vector<std::string> attributes;
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (!stmt->decorators[i])
      continue;
    auto [isAttr, attrName] = getDecorator(stmt->decorators[i]);
    if (!attrName.empty()) {
      // LOG("-> {} {}", stmt->name, attrName);
      if (isAttr) {
        attributes.push_back(attrName);
        stmt->setAttribute(attrName);
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
    if (auto n = in(ctx->cache->classes[ctx->getBase()->name].methods, stmt->name))
      rootName = *n;
  } else if (stmt->hasAttribute(Attr::Overload)) {
    // Case 2: function overload
    if (auto c = ctx->find(stmt->name)) {
      if (c->isFunc() && c->getModule() == ctx->getModule() &&
          c->getBaseName() == ctx->getBaseName()) {
        rootName = c->canonicalName;
      }
    }
  }
  if (rootName.empty())
    rootName = ctx->generateCanonicalName(stmt->name, true, isClassMember);
  // Append overload number to the name
  auto canonicalName = rootName;
  // if (!ctx->cache->overloads[rootName].empty())
  canonicalName += format(":{}", ctx->cache->overloads[rootName].size());
  ctx->cache->reverseIdentifierLookup[canonicalName] = stmt->name;

  if (isClassMember) {
    // Set the enclosing class name
    stmt->setAttribute(Attr::ParentClass, ctx->getBase()->name);
    // Add the method to the class' method list
    ctx->cache->classes[ctx->getBase()->name].methods[stmt->name] = rootName;
  } else {
    // Ensure that function binding does not shadow anything.
    // Function bindings cannot be dominated either
    auto funcVal = ctx->find(stmt->name);
    //  if (funcVal && !funcVal->canShadow)
    // E(Error::CLASS_INVALID_BIND, stmt, stmt->name);
  }

  // Handle captures. Add additional argument to the function for every capture.
  // Make sure to account for **kwargs if present
  std::map<std::string, TypeContext::Item> captures;
  if (auto b = stmt->getAttribute<BindingsAttribute>(Attr::Bindings))
    for (auto &[c, t] : b->captures) {
      if (auto v = ctx->find(c)) {
        if (t != BindingsAttribute::CaptureType::Global && !v->isGlobal()) {
          bool parentClassGeneric =
              ctx->bases.back().isType() && ctx->bases.back().name == v->getBaseName();
          if (v->isGeneric() && parentClassGeneric) {
            stmt->setAttribute(Attr::Method);
          }
          if (!v->isGeneric() || (v->isStatic() && !parentClassGeneric)) {
            captures[c] = v;
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

    Param kw;
    if (!stmt->args.empty() && startswith(stmt->args.back().name, "**")) {
      kw = stmt->args.back();
      stmt->args.pop_back();
    }
    std::array<const char *, 4> op{"", "int", "str", "bool"};
    for (auto &[c, v] : captures) {
      if (v->isType())
        stmt->args.emplace_back(c, N<IdExpr>("type"));
      else if (auto si = v->isStatic())
        stmt->args.emplace_back(c,
                                N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>(op[si])));
      else
        stmt->args.emplace_back(c);
      partialArgs.emplace_back(c, N<IdExpr>(v->canonicalName));
    }
    if (!kw.name.empty())
      stmt->args.push_back(kw);
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
    for (auto &a : stmt->args) {
      std::string varName = a.name;
      int stars = trimStars(varName);
      auto name = ctx->generateCanonicalName(varName);

      // Mark as method if the first argument is self
      if (isClassMember && stmt->hasAttribute(Attr::HasSelf) && a.name == "self") {
        // ctx->getBase()->selfName = name;
        stmt->setAttribute(Attr::Method);
      }

      // Handle default values
      auto defaultValue = a.defaultValue;
      if (a.type && defaultValue && cast<NoneExpr>(defaultValue)) {
        // Special case: `arg: Callable = None` -> `arg: Callable = NoneType()`
        if (cast<IndexExpr>(a.type) &&
            isId(cast<IndexExpr>(a.type)->getExpr(), TYPE_CALLABLE))
          defaultValue = N<CallExpr>(N<IdExpr>("NoneType"));
        // Special case: `arg: type = None` -> `arg: type = NoneType`
        if (isId(a.type, "type") || isId(a.type, TYPE_TYPEVAR))
          defaultValue = N<IdExpr>("NoneType");
      }
      /// TODO: Uncomment for Python-style defaults
      // if (defaultValue) {
      //   auto defaultValueCanonicalName =
      //       ctx->generateCanonicalName(format("{}.{}", canonicalName, name));
      //   prependStmts->push_back(N<AssignStmt>(N<IdExpr>(defaultValueCanonicalName),
      //     defaultValue));
      //   defaultValue = N<IdExpr>(defaultValueCanonicalName);
      // }
      args.emplace_back(std::string(stars, '*') + name, a.type, defaultValue, a.status);

      // Add generics to the context
      if (a.status != Param::Normal) {
        // Generic and static types
        auto generic = ctx->getUnbound();
        auto typId = generic->getLink()->id;
        generic->genericName = varName;
        if (auto st = getStaticGeneric(a.type)) {
          auto val = ctx->addVar(varName, name, generic);
          val->generic = true;
          generic->isStatic = st;
          if (a.defaultValue) {
            auto defType = transform(clone(a.defaultValue));
            generic->defaultType = getType(defType);
          }
        } else {
          if (auto ti = cast<InstantiateExpr>(a.type)) {
            // Parse TraitVar
            seqassert(isId(ti->getExpr(), TYPE_TYPEVAR), "not a TypeVar instantiation");
            auto l = transformType((*ti)[0])->type;
            if (l->getLink() && l->getLink()->trait)
              generic->getLink()->trait = l->getLink()->trait;
            else
              generic->getLink()->trait = std::make_shared<types::TypeTrait>(l);
          }
          auto val = ctx->addType(varName, name, generic);
          val->generic = true;
          if (a.defaultValue) {
            auto defType = transformType(clone(a.defaultValue));
            generic->defaultType = getType(defType);
          }
        }
        auto g = generic->generalize(ctx->typecheckLevel);
        explicits.emplace_back(name, varName, g, typId, g->isStaticType());
      }
    }

    // Prepare list of all generic types
    ClassTypePtr parentClass = nullptr;
    if (isClassMember && stmt->hasAttribute(Attr::Method)) {
      // Get class generics (e.g., T for `class Cls[T]: def foo:`)
      // auto parentClassAST =
      // ctx->cache->classes[stmt->attributes.parentClass].ast.get();
      auto aa = stmt->getAttribute<ir::StringValueAttribute>(Attr::ParentClass);
      parentClass = ctx->getType(aa->value)->getClass();
      // parentClass = parentClass->instantiate(ctx->typecheckLevel - 1, nullptr,
      // nullptr)
      // ->getClass();
      // seqassert(parentClass, "parent class not set");
      // for (int i = 0, j = 0, k = 0; i < parentClassAST->args.size(); i++) {
      //   if (parentClassAST->args[i].status != Param::Normal) {
      //     generics.push_back(parentClassAST->args[i].status == Param::Generic
      //                            ? parentClass->generics[j++].type
      //                            : parentClass->hiddenGenerics[k++].type);
      //     ctx->addType(parentClassAST->args[i].name, parentClassAST->args[i].name,
      //                  generics.back())
      //         ->generic = true;
      //   }
      // }
    }
    // Add function generics
    std::vector<TypePtr> generics;
    generics.reserve(explicits.size());
    for (const auto &i : explicits)
      generics.emplace_back(ctx->getType(i.name));

    // Handle function arguments
    // Base type: `Function[[args,...], ret]`
    baseType = getFuncTypeBase(stmt->args.size() - explicits.size());
    ctx->typecheckLevel++;

    // Parse arguments to the context. Needs to be done after adding generics
    // to support cases like `foo(a: T, T: type)`
    for (auto &a : args) {
      // if (a.status == Param::Normal || a.type->is ) // todo)) makes typevar work!
      // need to check why...
      a.type = transformType(a.type, false);
      // if (a.type && a.type->type->getLink() && a.type->type->getLink()->trait)
      //   LOG("-> {:c}", a.type->type->getLink()->trait);
      a.defaultValue = transform(a.defaultValue, true);
    }

    // Unify base type generics with argument types. Add non-generic arguments to the
    // context. Delayed to prevent cases like `def foo(a, b=a)`
    auto argType = baseType->generics[0].type->getClass();
    for (int ai = 0, aj = 0; ai < stmt->args.size(); ai++) {
      if (stmt->args[ai].status != Param::Normal)
        continue;
      std::string canName = stmt->args[ai].name;
      trimStars(canName);
      if (!stmt->args[ai].type) {
        if (parentClass && ai == 0 && stmt->args[ai].name == "self") {
          // Special case: self in methods
          unify(argType->generics[aj].type, parentClass);
        } else {
          unify(argType->generics[aj].type, ctx->getUnbound());
          generics.push_back(argType->generics[aj].type);
        }
      } else if (startswith(stmt->args[ai].name, "*")) {
        // Special case: `*args: type` and `**kwargs: type`. Do not add this type to the
        // signature (as the real type is `Tuple[type, ...]`); it will be used during
        // call typechecking
        unify(argType->generics[aj].type, ctx->getUnbound());
        generics.push_back(argType->generics[aj].type);
      } else {
        unify(argType->generics[aj].type, getType(transformType(stmt->args[ai].type)));
        // generics.push_back(argType->args[aj++]);
      }
      aj++;
      // ctx->addVar(ctx->cache->rev(canName), canName, argType->args[aj]);
    }

    // Parse the return type
    ret = transformType(stmt->ret, false);
    if (ret) {
      unify(baseType->generics[1].type, getType(ret));
      if (isId(ret, "Union")) {
        baseType->generics[1].type->getUnion()->generics[0].type->getUnbound()->kind =
            LinkType::Generic;
      }
    } else {
      generics.push_back(unify(baseType->generics[1].type, ctx->getUnbound()));
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
        suite = transformLLVMDefinition(stmt->suite->firstInBlock());
      } else if (stmt->hasAttribute(Attr::C)) {
        // Do nothing
      } else {
        // if ((isEnclosedFunc || stmt->attributes.has(Attr::Capture)) &&
        // !isClassMember)
        //   ctx->getBase()->captures = &captures;
        // if (stmt->attributes.has("std.internal.attributes.pycapture"))
        //   ctx->getBase()->pyCaptures = &pyCaptures;
        suite = clone(stmt->suite);
        // suite = SimplifyVisitor(ctx,
        // preamble).transformConditionalScope(stmt->suite);
      }
    }
  }
  stmt->setAttribute(Attr::Module, ctx->moduleName.path);
  // format(
  //     "{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" :
  //     "::", ctx->moduleName.module);
  ctx->cache->overloads[rootName].push_back(canonicalName);

  // Make function AST and cache it for later realization
  auto f = N<FunctionStmt>(canonicalName, ret, args, suite);
  f->cloneAttributesFrom(stmt);
  ctx->cache->functions[canonicalName].module = ctx->moduleName.path;
  ctx->cache->functions[canonicalName].ast = f;
  ctx->cache->functions[canonicalName].origAst = stmt_clone;
  ctx->cache->functions[canonicalName].isToplevel =
      ctx->getModule().empty() && ctx->isGlobal();
  ctx->cache->functions[canonicalName].rootName = rootName;
  f->setDone();
  auto aa = stmt->getAttribute<ir::StringValueAttribute>(Attr::ParentClass);
  auto parentClass = aa ? ctx->getType(aa->value) : nullptr;

  // Construct the type
  auto funcTyp = std::make_shared<types::FuncType>(
      baseType, ctx->cache->functions[canonicalName].ast, 0, explicits);
  funcTyp->setSrcInfo(getSrcInfo());
  if (isClassMember && stmt->hasAttribute(Attr::Method)) {
    funcTyp->funcParent = parentClass;
  }
  funcTyp = std::static_pointer_cast<types::FuncType>(
      funcTyp->generalize(ctx->typecheckLevel));
  ctx->cache->functions[canonicalName].type = funcTyp;
  // LOG("-> {:c}", funcTyp);

  ctx->addFunc(stmt->name, rootName, funcTyp);
  ctx->addFunc(canonicalName, canonicalName, funcTyp);
  if (stmt->hasAttribute(Attr::Overload) || isClassMember) {
    ctx->remove(stmt->name); // first overload will handle it!
  }

  // Special method handling
  if (isClassMember) {
    auto m =
        ctx->cache->getMethod(parentClass->getClass(), ctx->cache->rev(canonicalName));
    bool found = false;
    for (auto &i : ctx->cache->overloads[m])
      if (i == canonicalName) {
        ctx->cache->functions[i].type = funcTyp;
        found = true;
        break;
      }
    seqassert(found, "cannot find matching class method for {}", canonicalName);
  } else {
    // Hack so that we can later use same helpers for class overloads
    ctx->cache->classes[".toplevel"].methods[stmt->name] = rootName;
  }

  // Ensure that functions with @C, @force_realize, and @export attributes can be
  // realized
  if (stmt->hasAttribute(Attr::ForceRealize) || stmt->hasAttribute(Attr::Export) ||
      (stmt->hasAttribute(Attr::C) && !stmt->hasAttribute(Attr::CVarArg))) {
    if (!funcTyp->canRealize())
      E(Error::FN_REALIZE_BUILTIN, stmt);
  }

  // Debug information
  // LOG("[func] added func {}: {}", canonicalName, funcTyp->debugString(2));

  // Expression to be used if function binding is modified by captures or decorators
  Expr *finalExpr = nullptr;
  // If there are captures, replace `fn` with `fn(cap1=cap1, cap2=cap2, ...)`
  if (!captures.empty()) {
    if (isClassMember)
      E(Error::ID_CANNOT_CAPTURE, getSrcInfo(), captures.begin()->first);

    finalExpr = N<CallExpr>(N<IdExpr>(canonicalName), partialArgs);
    // Add updated self reference in case function is recursive!
    auto pa = partialArgs;
    for (auto &a : pa) {
      if (!a.name.empty())
        a.value = N<IdExpr>(a.name);
      else
        a.value = clone(a.value);
    }
    // todo)) right now this adds a capture hook for recursive calls
    f->suite = N<SuiteStmt>(
        N<AssignStmt>(N<IdExpr>(stmt->name), N<CallExpr>(N<IdExpr>(stmt->name), pa)),
        suite);
  }

  // Parse remaining decorators
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (stmt->decorators[i]) {
      if (isClassMember)
        E(Error::FN_NO_DECORATORS, stmt->decorators[i]);
      // Replace each decorator with `decorator(finalExpr)` in the reverse order
      finalExpr = N<CallExpr>(stmt->decorators[i],
                              finalExpr ? finalExpr : N<IdExpr>(canonicalName));
    }
  }

  if (finalExpr) {
    resultStmt =
        N<SuiteStmt>(f, transform(N<AssignStmt>(N<IdExpr>(stmt->name), finalExpr)));
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
  seqassert(codeStmt && codeStmt->getExpr() &&
                cast<StringExpr>(codeStmt->getExpr()->expr),
            "invalid Python definition");

  auto code = cast<StringExpr>(codeStmt->getExpr()->expr)->getValue();
  std::vector<std::string> pyargs;
  pyargs.reserve(args.size());
  for (const auto &a : args)
    pyargs.emplace_back(a.name);
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
  seqassert(codeStmt && codeStmt->getExpr() &&
                cast<StringExpr>(codeStmt->getExpr()->expr),
            "invalid LLVM definition");

  auto code = cast<StringExpr>(codeStmt->getExpr()->expr)->getValue();
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
      auto expr = transform(parseExpr(ctx->cache, exprCode, offset).first, true);
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
    auto ci = ctx->find(id->getValue());
    if (ci && ci->isFunc()) {
      if (auto f = in(ctx->cache->functions, ci->canonicalName)) {
        return {
            ctx->cache->functions[ci->canonicalName].ast->hasAttribute(Attr::Attribute),
            ci->canonicalName};
      } else if (ctx->cache->overloads[ci->canonicalName].size() == 1) {
        return {ctx->cache->functions[ctx->cache->overloads[ci->canonicalName][0]]
                    .ast->hasAttribute(Attr::Attribute),
                ci->canonicalName};
      }
    }
  }
  return {false, ""};
}

/// Make an empty partial call `fn(...)` for a given function.
Expr *TypecheckVisitor::partializeFunction(const types::FuncTypePtr &fn) {
  // Create function mask
  std::vector<char> mask(fn->ast->args.size(), 0);
  for (int i = 0, j = 0; i < fn->ast->args.size(); i++)
    if (fn->ast->args[i].status == Param::Generic) {
      if (!fn->funcGenerics[j].type->getUnbound())
        mask[i] = 1;
      j++;
    }

  // Generate partial class
  auto call = generatePartialCall(mask, fn.get());
  return call;
}

/// Generate and return `Function[Tuple[args...], ret]` type
std::shared_ptr<ClassType> TypecheckVisitor::getFuncTypeBase(size_t nargs) {
  auto baseType = ctx->instantiate(ctx->getType("Function"))->getClass();
  unify(baseType->generics[0].type,
        ctx->instantiate(generateTuple(nargs, false))->getClass());
  return baseType;
}

} // namespace codon::ast
