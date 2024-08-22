// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

/// Import and parse a new module into its own context.
/// Also handle special imports ( see @c transformSpecialImport ).
/// To simulate Python's dynamic import logic and import stuff only once,
/// each import statement is guarded as follows:
///   if not _import_N_done:
///     _import_N()
///     _import_N_done = True
/// See @c transformNewImport and below for more details.
void SimplifyVisitor::visit(ImportStmt *stmt) {
  seqassert(!ctx->inClass(), "imports within a class");
  if ((resultStmt = transformSpecialImport(stmt)))
    return;

  // Fetch the import
  auto components = getImportPath(stmt->from.get(), stmt->dots);
  auto path = combine2(components, "/");
  auto file = getImportFile(ctx->cache->argv0, path, ctx->getFilename(), false,
                            ctx->cache->module0, ctx->cache->pluginImportPaths);
  if (!file) {
    std::string s(stmt->dots, '.');
    for (size_t i = 0; i < components.size(); i++)
      if (components[i] == "..") {
        continue;
      } else if (!s.empty() && s.back() != '.') {
        s += "." + components[i];
      } else {
        s += components[i];
      }
    E(Error::IMPORT_NO_MODULE, stmt->from, s);
  }

  // If the file has not been seen before, load it into cache
  bool handled = true;
  if (ctx->cache->imports.find(file->path) == ctx->cache->imports.end()) {
    resultStmt = transformNewImport(*file);
    if (!resultStmt)
      handled = false; // we need an import
  }

  const auto &import = ctx->cache->imports[file->path];
  std::string importVar = import.importVar;
  if (!import.loadedAtToplevel)
    handled = false;

  // Construct `if _import_done.__invert__(): (_import(); _import_done = True)`.
  // Do not do this during the standard library loading (we assume that standard library
  // imports are "clean" and do not need guards). Note that the importVar is empty if
  // the import has been loaded during the standard library loading.
  if (!handled) {
    resultStmt = N<ExprStmt>(N<CallExpr>(N<IdExpr>(importVar + ":0")));
    LOG_TYPECHECK("[import] loading {}", importVar);
  }

  // Import requested identifiers from the import's scope to the current scope
  if (!stmt->what) {
    // Case: import foo
    auto name = stmt->as.empty() ? path : stmt->as;
    ctx->addVar(name, importVar, stmt->getSrcInfo())->importPath = file->path;
  } else if (stmt->what->isId("*")) {
    // Case: from foo import *
    seqassert(stmt->as.empty(), "renamed star-import");
    // Just copy all symbols from import's context here.
    for (auto &i : *(import.ctx)) {
      if ((!startswith(i.first, "_") ||
           (ctx->isStdlibLoading && startswith(i.first, "__")))) {
        // Ignore all identifiers that start with `_` but not those that start with
        // `__` while the standard library is being loaded
        auto c = i.second.front();
        if (c->isConditional() && i.first.find('.') == std::string::npos) {
          c = import.ctx->findDominatingBinding(i.first);
        }
        // Imports should ignore  noShadow property
        ctx->Context<SimplifyItem>::add(i.first, c);
      }
    }
  } else {
    // Case 3: from foo import bar
    auto i = stmt->what->getId();
    seqassert(i, "not a valid import what expression");
    auto c = import.ctx->find(i->value);
    // Make sure that we are importing an existing global symbol
    if (!c)
      E(Error::IMPORT_NO_NAME, i, i->value, file->module);
    if (c->isConditional())
      c = import.ctx->findDominatingBinding(i->value);
    // Imports should ignore  noShadow property
    ctx->Context<SimplifyItem>::add(stmt->as.empty() ? i->value : stmt->as, c);
  }

  if (!resultStmt) {
    resultStmt = N<SuiteStmt>(); // erase it
  }
}

/// Transform special `from C` and `from python` imports.
/// See @c transformCImport, @c transformCDLLImport and @c transformPythonImport
StmtPtr SimplifyVisitor::transformSpecialImport(ImportStmt *stmt) {
  if (stmt->from && stmt->from->isId("C") && stmt->what->getId() && stmt->isFunction) {
    // C function imports
    return transformCImport(stmt->what->getId()->value, stmt->args, stmt->ret.get(),
                            stmt->as);
  }
  if (stmt->from && stmt->from->isId("C") && stmt->what->getId()) {
    // C variable imports
    return transformCVarImport(stmt->what->getId()->value, stmt->ret.get(), stmt->as);
  } else if (stmt->from && stmt->from->isId("C") && stmt->what->getDot()) {
    // dylib C imports
    return transformCDLLImport(stmt->what->getDot()->expr.get(),
                               stmt->what->getDot()->member, stmt->args,
                               stmt->ret.get(), stmt->as, stmt->isFunction);
  } else if (stmt->from && stmt->from->isId("python") && stmt->what) {
    // Python imports
    return transformPythonImport(stmt->what.get(), stmt->args, stmt->ret.get(),
                                 stmt->as);
  }
  return nullptr;
}

/// Transform Dot(Dot(a, b), c...) into "{a, b, c, ...}".
/// Useful for getting import paths.
std::vector<std::string> SimplifyVisitor::getImportPath(Expr *from, size_t dots) {
  std::vector<std::string> components; // Path components
  if (from) {
    for (; from->getDot(); from = from->getDot()->expr.get())
      components.push_back(from->getDot()->member);
    seqassert(from->getId(), "invalid import statement");
    components.push_back(from->getId()->value);
  }

  // Handle dots (i.e., `..` in `from ..m import x`)
  for (size_t i = 1; i < dots; i++)
    components.emplace_back("..");
  std::reverse(components.begin(), components.end());
  return components;
}

/// Transform a C function import.
/// @example
///   `from C import foo(int) -> float as f` ->
///   ```@.c
///      def foo(a1: int) -> float:
///        pass
///      f = foo # if altName is provided```
/// No return type implies void return type. *args is treated as C VAR_ARGS.
StmtPtr SimplifyVisitor::transformCImport(const std::string &name,
                                          const std::vector<Param> &args,
                                          const Expr *ret, const std::string &altName) {
  std::vector<Param> fnArgs;
  auto attr = Attr({Attr::C});
  for (size_t ai = 0; ai < args.size(); ai++) {
    seqassert(args[ai].name.empty(), "unexpected argument name");
    seqassert(!args[ai].defaultValue, "unexpected default argument");
    seqassert(args[ai].type, "missing type");
    if (args[ai].type->getEllipsis() && ai + 1 == args.size()) {
      // C VAR_ARGS support
      attr.set(Attr::CVarArg);
      fnArgs.emplace_back(Param{"*args", nullptr, nullptr});
    } else {
      fnArgs.emplace_back(
          Param{args[ai].name.empty() ? format("a{}", ai) : args[ai].name,
                args[ai].type->clone(), nullptr});
    }
  }
  ctx->generateCanonicalName(name); // avoid canonicalName == name
  StmtPtr f = N<FunctionStmt>(name, ret ? ret->clone() : N<IdExpr>("NoneType"), fnArgs,
                              nullptr, attr);
  f = transform(f); // Already in the preamble
  if (!altName.empty()) {
    auto val = ctx->forceFind(name);
    ctx->add(altName, val);
    ctx->remove(name);
  }
  return f;
}

/// Transform a C variable import.
/// @example
///   `from C import foo: int as f` ->
///   ```f: int = "foo"```
StmtPtr SimplifyVisitor::transformCVarImport(const std::string &name, const Expr *type,
                                             const std::string &altName) {
  auto canonical = ctx->generateCanonicalName(name);
  auto val = ctx->addVar(altName.empty() ? name : altName, canonical);
  val->noShadow = true;
  auto s = N<AssignStmt>(N<IdExpr>(canonical), nullptr, transformType(type->clone()));
  s->lhs->setAttr(ExprAttr::ExternVar);
  return s;
}

/// Transform a dynamic C import.
/// @example
///   `from C import lib.foo(int) -> float as f` ->
///   `f = _dlsym(lib, "foo", Fn=Function[[int], float]); f`
/// No return type implies void return type.
StmtPtr SimplifyVisitor::transformCDLLImport(const Expr *dylib, const std::string &name,
                                             const std::vector<Param> &args,
                                             const Expr *ret,
                                             const std::string &altName,
                                             bool isFunction) {
  ExprPtr type = nullptr;
  if (isFunction) {
    std::vector<ExprPtr> fnArgs{N<ListExpr>(std::vector<ExprPtr>{}),
                                ret ? ret->clone() : N<IdExpr>("NoneType")};
    for (const auto &a : args) {
      seqassert(a.name.empty(), "unexpected argument name");
      seqassert(!a.defaultValue, "unexpected default argument");
      seqassert(a.type, "missing type");
      fnArgs[0]->getList()->items.emplace_back(clone(a.type));
    }

    type = N<IndexExpr>(N<IdExpr>("Function"), N<TupleExpr>(fnArgs));
  } else {
    type = ret->clone();
  }

  return transform(N<AssignStmt>(
      N<IdExpr>(altName.empty() ? name : altName),
      N<CallExpr>(N<IdExpr>("_dlsym"),
                  std::vector<CallExpr::Arg>{CallExpr::Arg(dylib->clone()),
                                             CallExpr::Arg(N<StringExpr>(name)),
                                             {"Fn", type}})));
}

/// Transform a Python module and function imports.
/// @example
///   `from python import module as f` -> `f = pyobj._import("module")`
///   `from python import lib.foo(int) -> float as f` ->
///   ```def f(a0: int) -> float:
///        f = pyobj._import("lib")._getattr("foo")
///        return float.__from_py__(f(a0))```
/// If a return type is nullptr, the function just returns f (raw pyobj).
StmtPtr SimplifyVisitor::transformPythonImport(Expr *what,
                                               const std::vector<Param> &args,
                                               Expr *ret, const std::string &altName) {
  // Get a module name (e.g., os.path)
  auto components = getImportPath(what);

  if (!ret && args.empty()) {
    // Simple import: `from python import foo.bar` -> `bar = pyobj._import("foo.bar")`
    return transform(
        N<AssignStmt>(N<IdExpr>(altName.empty() ? components.back() : altName),
                      N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                                  N<StringExpr>(combine2(components, ".")))));
  }

  // Python function import:
  // `from python import foo.bar(int) -> float` ->
  // ```def bar(a1: int) -> float:
  //      f = pyobj._import("foo")._getattr("bar")
  //      return float.__from_py__(f(a1))```

  // f = pyobj._import("foo")._getattr("bar")
  auto call = N<AssignStmt>(
      N<IdExpr>("f"),
      N<CallExpr>(
          N<DotExpr>(N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                                 N<StringExpr>(combine2(components, ".", 0,
                                                        int(components.size()) - 1))),
                     "_getattr"),
          N<StringExpr>(components.back())));
  // f(a1, ...)
  std::vector<Param> params;
  std::vector<ExprPtr> callArgs;
  for (int i = 0; i < args.size(); i++) {
    params.emplace_back(Param{format("a{}", i), clone(args[i].type), nullptr});
    callArgs.emplace_back(N<IdExpr>(format("a{}", i)));
  }
  // `return ret.__from_py__(f(a1, ...))`
  auto retType = (ret && !ret->getNone()) ? ret->clone() : N<IdExpr>("NoneType");
  auto retExpr = N<CallExpr>(N<DotExpr>(retType->clone(), "__from_py__"),
                             N<DotExpr>(N<CallExpr>(N<IdExpr>("f"), callArgs), "p"));
  auto retStmt = N<ReturnStmt>(retExpr);
  // Create a function
  return transform(N<FunctionStmt>(altName.empty() ? components.back() : altName,
                                   retType, params, N<SuiteStmt>(call, retStmt)));
}

/// Import a new file into its own context and wrap its top-level statements into a
/// function to support Python-like runtime import loading.
/// @example
///   ```_import_[I]_done = False
///      def _import_[I]():
///        global [imported global variables]...
///        __name__ = [I]
///        [imported top-level statements]```
StmtPtr SimplifyVisitor::transformNewImport(const ImportFile &file) {
  // Use a clean context to parse a new file
  if (ctx->cache->age)
    ctx->cache->age++;
  auto ictx = std::make_shared<SimplifyContext>(file.path, ctx->cache);
  ictx->isStdlibLoading = ctx->isStdlibLoading;
  ictx->moduleName = file;
  auto import = ctx->cache->imports.insert({file.path, {file.path, ictx}}).first;
  import->second.loadedAtToplevel =
      ctx->cache->imports[ctx->moduleName.path].loadedAtToplevel &&
      (ctx->isStdlibLoading || (ctx->isGlobal() && ctx->scope.blocks.size() == 1));
  auto importVar = import->second.importVar =
      ctx->cache->getTemporaryVar(format("import_{}", file.module));
  import->second.moduleName = file.module;
  LOG_TYPECHECK("[import] initializing {} ({})", importVar,
                import->second.loadedAtToplevel);

  // __name__ = [import name]
  StmtPtr n = nullptr;
  if (ictx->moduleName.module != "internal.core") {
    // str is not defined when loading internal.core; __name__ is not needed anyway
    n = N<AssignStmt>(N<IdExpr>("__name__"), N<StringExpr>(ictx->moduleName.module));
    preamble->push_back(N<AssignStmt>(
        N<IdExpr>(importVar),
        N<CallExpr>(N<IdExpr>("Import.__new__"), N<StringExpr>(file.module),
                    N<StringExpr>(file.path), N<BoolExpr>(false)),
        N<IdExpr>("Import")));
    auto var = ctx->addAlwaysVisible(
        std::make_shared<SimplifyItem>(SimplifyItem::Var, ctx->getBaseName(), importVar,
                                       ctx->getModule(), std::vector<int>{0}));
    ctx->cache->addGlobal(importVar);
  }
  n = N<SuiteStmt>(n, parseFile(ctx->cache, file.path));
  n = SimplifyVisitor(ictx, preamble).transform(n);
  if (!ctx->cache->errors.empty())
    throw exc::ParserException();
  // Add comment to the top of import for easier dump inspection
  auto comment = N<CommentStmt>(format("import: {} at {}", file.module, file.path));
  if (ctx->isStdlibLoading) {
    // When loading the standard library, imports are not wrapped.
    // We assume that the standard library has no recursive imports and that all
    // statements are executed before the user-provided code.
    return N<SuiteStmt>(comment, n);
  } else {
    // Wrap all imported top-level statements into a function.
    // Make sure to register the global variables and set their assignments as
    // updates. Note: signatures/classes/functions are not wrapped
    std::vector<StmtPtr> stmts;
    stmts.push_back(
        N<IfStmt>(N<DotExpr>(N<IdExpr>(importVar), "loaded"), N<ReturnStmt>()));
    stmts.push_back(N<ExprStmt>(
        N<CallExpr>(N<IdExpr>("Import._set_loaded"),
                    N<CallExpr>(N<IdExpr>("__ptr__"), N<IdExpr>(importVar)))));
    auto processToplevelStmt = [&](const StmtPtr &s) {
      // Process toplevel statement
      if (auto a = s->getAssign()) {
        if (!a->isUpdate() && a->lhs->getId()) {
          // Global `a = ...`
          auto val = ictx->forceFind(a->lhs->getId()->value);
          if (val->isVar() && val->isGlobal())
            ctx->cache->addGlobal(val->canonicalName);
        }
      }
      stmts.push_back(s);
    };
    processToplevelStmt(comment);
    if (auto st = n->getSuite()) {
      for (auto &ss : st->stmts)
        if (ss)
          processToplevelStmt(ss);
    } else {
      processToplevelStmt(n);
    }

    // Create import function manually with ForceRealize
    ctx->cache->functions[importVar + ":0"].ast =
        N<FunctionStmt>(importVar + ":0", nullptr, std::vector<Param>{},
                        N<SuiteStmt>(stmts), Attr({Attr::ForceRealize}));
    preamble->push_back(ctx->cache->functions[importVar + ":0"].ast->clone());
    ctx->cache->overloads[importVar].push_back({importVar + ":0", ctx->cache->age});
  }
  return nullptr;
}

} // namespace codon::ast
