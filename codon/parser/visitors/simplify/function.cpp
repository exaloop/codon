#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

/// Ensure that `(yield)` is in a function.
void SimplifyVisitor::visit(YieldExpr *expr) {
  if (!ctx->inFunction())
    error("expected function body");
}

/// Transform lambdas. Capture outer expressions.
/// @example
///   `lambda a, b: a+b+c` -> ```def fn(a, b, c):
///                                return a+b+c
///                              fn(c=c, ...)```
/// See @c makeAnonFn
void SimplifyVisitor::visit(LambdaExpr *expr) {
  resultExpr =
      makeAnonFn(std::vector<StmtPtr>{N<ReturnStmt>(clone(expr->expr))}, expr->vars);
}

/// Ensure that `return` is in a function.
void SimplifyVisitor::visit(ReturnStmt *stmt) {
  if (!ctx->inFunction())
    error("expected function body");
  transform(stmt->expr);
}

/// Ensure that `yield` is in a function.
void SimplifyVisitor::visit(YieldStmt *stmt) {
  if (!ctx->inFunction())
    error("expected function body");
  transform(stmt->expr);
}

/// Transform `yield from` statements.
/// @example
///   `yield from a` -> `for var in a: yield var`
void SimplifyVisitor::visit(YieldFromStmt *stmt) {
  auto var = ctx->cache->getTemporaryVar("yield");
  resultStmt =
      transform(N<ForStmt>(N<IdExpr>(var), stmt->expr, N<YieldStmt>(N<IdExpr>(var))));
}

/// Process `global` statements. Remove them upon completion.
void SimplifyVisitor::visit(GlobalStmt *stmt) {
  if (!ctx->inFunction())
    error("global or nonlocal outside of a function");

  // Dominate the binding
  auto val = ctx->findDominatingBinding(stmt->var);
  if (!val || !val->isVar())
    error("identifier '{}' not found", stmt->var);
  if (val->getBaseName() == ctx->getBaseName())
    error("identifier '{}' already defined", stmt->var);

  // Check global/nonlocal distinction
  if (!stmt->nonLocal && !val->getBaseName().empty())
    error("not a global variable");
  else if (stmt->nonLocal && val->getBaseName().empty())
    error("not a nonlocal variable");
  seqassert(!val->canonicalName.empty(), "'{}' does not have a canonical name",
            stmt->var);

  // Register as global if needed
  if (!in(ctx->cache->globals, val->canonicalName))
    ctx->cache->globals[val->canonicalName] = nullptr;

  val = ctx->addVar(stmt->var, val->canonicalName, stmt->getSrcInfo());
  val->baseName = ctx->getBaseName();
  // Globals/nonlocals cannot be shadowed in children scopes (as in Python)
  val->noShadow = true;
  // Erase the statement
  resultStmt = N<SuiteStmt>();
}

/// Validate and transform function definitions.
/// Handle overloads, class methods, default arguments etc.
/// Also capture variables if necessary and apply decorators.
/// @example
///   ```a = 5
///      @dec
///      def foo(b):
///        return a+b
///   ``` -> ```
///      a = 5
///      def foo(b, a_cap):
///        return a_cap+b
///      foo = dec(foo(a_cap=a, ...))
///   ```
/// For Python and LLVM definition transformations, see
/// @c transformPythonDefinition and @c transformLLVMDefinition
void SimplifyVisitor::visit(FunctionStmt *stmt) {
  if (stmt->attributes.has(Attr::Python)) {
    // Handle Python block
    resultStmt = transformPythonDefinition(stmt->name, stmt->args, stmt->ret.get(),
                                           stmt->suite->firstInBlock());
    return;
  }

  // Parse attributes
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (auto n = isAttribute(stmt->decorators[i])) {
      stmt->attributes.set(*n);
      stmt->decorators[i] = nullptr; // remove it from further consideration
    }
  }

  bool isClassMember = ctx->inClass(), isEnclosedFunc = ctx->inFunction();
  if (stmt->attributes.has(Attr::ForceRealize) && (!ctx->isGlobal() || isClassMember))
    error("builtins must be defined at the toplevel");

  // All overloads share the same canonical name except for the number at the
  // end (e.g., `foo.1:0`, `foo.1:1` etc.)
  std::string rootName;
  if (isClassMember) {
    // Case 1: method overload
    if (auto n = in(ctx->cache->classes[ctx->getBase()->name].methods, stmt->name))
      rootName = *n;
  } else if (stmt->attributes.has(Attr::Overload)) {
    // Case 2: function overload
    if (auto c = ctx->find(stmt->name)) {
      if (c->isFunc() && c->getModule() == ctx->getModule() &&
          c->getBaseName() == ctx->getBaseName())
        rootName = c->canonicalName;
    }
  }
  if (rootName.empty())
    rootName = ctx->generateCanonicalName(stmt->name, true);
  // Append overload number to the name
  auto canonicalName =
      format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
  ctx->cache->reverseIdentifierLookup[canonicalName] = stmt->name;

  // Ensure that function binding does not shadow anything.
  // Function bindings cannot be dominated either
  if (!isClassMember) {
    auto funcVal = ctx->find(stmt->name);
    if (funcVal && funcVal->noShadow)
      error("cannot update global/nonlocal");
    funcVal = ctx->addFunc(stmt->name, rootName, stmt->getSrcInfo());
    ctx->addAlwaysVisible(funcVal);
  }

  // Set up the base
  ctx->bases.emplace_back(SimplifyContext::Base{canonicalName});
  ctx->addBlock();
  ctx->getBase()->attributes = &(stmt->attributes);

  // Parse arguments and add them to the context
  std::vector<Param> args;
  for (auto &a : stmt->args) {
    std::string varName = a.name;
    int stars = trimStars(varName);
    auto name = ctx->generateCanonicalName(varName);

    // Mark as method if the first argument is self
    if (isClassMember && stmt->attributes.has(Attr::HasSelf) && a.name == "self") {
      ctx->getBase()->selfName = name;
      stmt->attributes.set(Attr::Method);
    }

    // Handle default values
    auto defaultValue = a.defaultValue;
    if (a.type && defaultValue && defaultValue->getNone()) {
      // Special case: `arg: Callable = None` -> `arg: Callable = NoneType()`
      if (a.type->getIndex() && a.type->getIndex()->expr->isId(TYPE_CALLABLE))
        defaultValue = N<CallExpr>(N<IdExpr>("NoneType"));
      // Special case: `arg: type = None` -> `arg: type = NoneType`
      if (a.type->isId("type") || a.type->isId("TypeVar"))
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
    args.emplace_back(
        Param{std::string(stars, '*') + name, a.type, defaultValue, a.status});

    // Add generics to the context
    if (a.status != Param::Normal) {
      if (getStaticGeneric(a.type.get()))
        ctx->addVar(varName, name, stmt->getSrcInfo())->generic = true;
      else
        ctx->addType(varName, name, stmt->getSrcInfo())->generic = true;
    }
  }
  // Parse arguments to the context. Needs to be done after adding generics
  // to support cases like `foo(a: T, T: type)`
  for (auto &a : args) {
    a.type = transformType(a.type, false);
    a.defaultValue = transform(a.defaultValue, true);
  }
  // Add non-generic arguments to the context. Delayed to prevent cases like
  // `def foo(a, b=a)`
  for (auto &a : args) {
    if (a.status == Param::Normal) {
      std::string canName = a.name;
      trimStars(canName);
      ctx->addVar(ctx->cache->rev(canName), canName, stmt->getSrcInfo());
    }
  }

  // Parse the return type
  auto ret = transformType(stmt->ret, false);

  // Parse function body
  StmtPtr suite = nullptr;
  std::unordered_map<std::string, std::string> captures;
  if (!stmt->attributes.has(Attr::Internal) && !stmt->attributes.has(Attr::C)) {
    ctx->addBlock();
    if (stmt->attributes.has(Attr::LLVM)) {
      suite = transformLLVMDefinition(stmt->suite->firstInBlock());
    } else if (stmt->attributes.has(Attr::C)) {
      // Do nothing
    } else {
      if ((isEnclosedFunc || stmt->attributes.has(Attr::Capture)) && !isClassMember)
        ctx->getBase()->captures = &captures;
      suite = SimplifyVisitor(ctx, preamble).transformConditionalScope(stmt->suite);
    }
    ctx->popBlock();
  }

  ctx->bases.pop_back();
  ctx->popBlock();
  stmt->attributes.module =
      format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
             ctx->moduleName.module);
  ctx->cache->overloads[rootName].push_back({canonicalName, ctx->cache->age});

  // Special method handling
  if (isClassMember) {
    // Set the enclosing class name
    stmt->attributes.parentClass = ctx->getBase()->name;
    // Add the method to the class' method list
    ctx->cache->classes[ctx->getBase()->name].methods[stmt->name] = rootName;
  }

  // Handle captures. Add additional argument to the function for every capture.
  // Make sure to account for **kwargs if present
  std::vector<CallExpr::Arg> partialArgs;
  if (!captures.empty()) {
    Param kw;
    if (!args.empty() && startswith(args.back().name, "**")) {
      kw = args.back();
      args.pop_back();
    }
    for (auto &c : captures) {
      args.emplace_back(Param{c.second, nullptr, nullptr});
      partialArgs.push_back({c.second, N<IdExpr>(ctx->cache->rev(c.first))});
    }
    if (!kw.name.empty())
      args.push_back(kw);
    partialArgs.push_back({"", N<EllipsisExpr>()});
  }
  // Make function AST and cache it for later realization
  auto f = N<FunctionStmt>(canonicalName, ret, args, suite, stmt->attributes);
  ctx->cache->functions[canonicalName].ast = f;
  ctx->cache->functions[canonicalName].origAst =
      std::static_pointer_cast<FunctionStmt>(stmt->clone());

  // Expression to be used if function binding is modified by captures or decorators
  ExprPtr finalExpr = nullptr;
  // If there are captures, replace `fn` with `fn(cap1=cap1, cap2=cap2, ...)`
  if (!captures.empty())
    finalExpr = N<CallExpr>(N<IdExpr>(stmt->name), partialArgs);

  // Parse remaining decorators
  for (auto i = stmt->decorators.size(); i-- > 0;) {
    if (stmt->decorators[i]) {
      if (isClassMember)
        error("decorators cannot be applied to class methods");
      // Replace each decorator with `decorator(finalExpr)` in the reverse order
      finalExpr = N<CallExpr>(stmt->decorators[i],
                              finalExpr ? finalExpr : N<IdExpr>(stmt->name));
    }
  }

  if (finalExpr) {
    resultStmt =
        N<SuiteStmt>(f, transform(N<AssignStmt>(N<IdExpr>(stmt->name), finalExpr)));
  } else {
    resultStmt = f;
  }
}

/// Make a capturing anonymous function with the provided suite and argument names.
/// The resulting function will be added before the current statement.
/// Return an expression that can call this function (an @c IdExpr or a partial call).
ExprPtr SimplifyVisitor::makeAnonFn(std::vector<StmtPtr> suite,
                                    const std::vector<std::string> &argNames) {
  std::vector<Param> params;
  std::string name = ctx->cache->getTemporaryVar("lambda");
  for (auto &s : argNames)
    params.emplace_back(Param(s));
  auto f = transform(N<FunctionStmt>(name, nullptr, params, N<SuiteStmt>(move(suite)),
                                     Attr({Attr::Capture})));
  if (auto fs = f->getSuite()) {
    seqassert(fs->stmts.size() == 2 && fs->stmts[0]->getFunction() &&
                  fs->stmts[1]->getAssign(),
              "invalid function transform");
    prependStmts->push_back(fs->stmts[0]);
    return fs->stmts[1]->getAssign()->rhs;
  } else {
    prependStmts->push_back(f);
    return transform(N<IdExpr>(name));
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
StmtPtr SimplifyVisitor::transformPythonDefinition(const std::string &name,
                                                   const std::vector<Param> &args,
                                                   const Expr *ret, Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid Python definition");

  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  std::vector<std::string> pyargs;
  for (const auto &a : args)
    pyargs.emplace_back(a.name);
  code = format("def {}({}):\n{}\n", name, join(pyargs, ", "), code);
  return transform(N<SuiteStmt>(
      N<ExprStmt>(N<CallExpr>(N<DotExpr>("pyobj", "_exec"), N<StringExpr>(code))),
      N<ImportStmt>(N<IdExpr>("python"), N<DotExpr>("__main__", name), clone_nop(args),
                    ret ? ret->clone() : N<IdExpr>("pyobj"))));
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
StmtPtr SimplifyVisitor::transformLLVMDefinition(Stmt *codeStmt) {
  seqassert(codeStmt && codeStmt->getExpr() && codeStmt->getExpr()->expr->getString(),
            "invalid LLVM definition");

  auto code = codeStmt->getExpr()->expr->getString()->getValue();
  std::vector<StmtPtr> items;
  auto se = N<StringExpr>("");
  std::string finalCode = se->getValue();
  items.push_back(N<ExprStmt>(se));

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
        error("invalid LLVM substitution");
      }
    } else if (braceCount && code[i] == '}') {
      braceCount--;
      std::string exprCode = code.substr(braceStart, i - braceStart);
      auto offset = getSrcInfo();
      offset.col += i;
      auto expr = transform(parseExpr(ctx->cache, exprCode, offset), true);
      items.push_back(N<ExprStmt>(expr));
      braceStart = i + 1;
      finalCode += '}';
    }
  }
  if (braceCount)
    error("invalid LLVM substitution");
  if (braceStart != code.size())
    finalCode += escapeFStringBraces(code, braceStart, int(code.size()) - braceStart);
  se->strings[0].first = finalCode;
  return N<SuiteStmt>(items);
}

/// Check if a decorator is actually an attribute (a function with `@__attribute__`)
std::string *SimplifyVisitor::isAttribute(ExprPtr e) {
  auto dt = transform(clone(e));
  if (dt && dt->getId()) {
    auto ci = ctx->find(dt->getId()->value);
    if (ci && ci->isFunc()) {
      if (ctx->cache->overloads[ci->canonicalName].size() == 1)
        if (ctx->cache->functions[ctx->cache->overloads[ci->canonicalName][0].name]
                .ast->attributes.isAttribute) {
          return &(ci->canonicalName);
        }
    }
  }
  return nullptr;
}

} // namespace codon::ast
