// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

/// Import and parse a new module into its own context.
/// Also handle special imports ( see @c transformSpecialImport ).
/// To simulate Python's dynamic import logic and import stuff only once,
/// each import statement is guarded as follows:
///   if not _import_N_done:
///     _import_N()
///     _import_N_done = True
/// See @c transformNewImport and below for more details.
void TypecheckVisitor::visit(ImportStmt *stmt) {
  seqassert(!ctx->inClass(), "imports within a class");
  if ((resultStmt = transformSpecialImport(stmt)))
    return;

  // Fetch the import
  auto components = getImportPath(stmt->getFrom(), stmt->getDots());
  auto path = combine2(components, "/");
  auto file = getImportFile(getArgv(), path, ctx->getFilename(), false,
                            getRootModulePath(), getPluginImportPaths());
  if (!file) {
    if (stmt->getDots() == 0 && ctx->autoPython) {
      auto newStr = FormatVisitor::apply(stmt->getFrom());
      if (stmt->getWhat())
        newStr += "." + FormatVisitor::apply(stmt->getWhat());
      compilationWarning(fmt::format("importing '{}' from Python", newStr),
                         stmt->getSrcInfo().file, stmt->getSrcInfo().line,
                         stmt->getSrcInfo().col);
      auto exprOrErr = parseExpr(ctx->cache, newStr, stmt->getFrom()->getSrcInfo());
      if (!exprOrErr)
        throw exc::ParserException(exprOrErr.takeError());
      resultStmt = transform(N<ImportStmt>(N<IdExpr>("python"), exprOrErr->first,
                                           stmt->getArgs(), stmt->getReturnType(),
                                           stmt->getAs()));
      return;
    }

    std::string s(stmt->getDots(), '.');
    for (auto &c : components) {
      if (c == "..") {
        continue;
      } else if (!s.empty() && s.back() != '.') {
        s += "." + c;
      } else {
        s += c;
      }
    }
    bool allDot = true;
    for (auto cp : s)
      if (cp != '.') {
        allDot = false;
        break;
      }
    if (allDot && match(stmt->getWhat(), M<IdExpr>()))
      s = cast<IdExpr>(stmt->getWhat())->getValue();
    E(Error::IMPORT_NO_MODULE, stmt, s);
  }

  // If the file has not been seen before, load it into cache
  bool handled = true;
  if (!in(ctx->cache->imports, file->path)) {
    resultStmt = transformNewImport(*file);
    if (!resultStmt)
      handled = false; // we need an import
  }

  const auto &import = getImport(file->path);
  std::string importVar = import->importVar;
  if (!import->loadedAtToplevel)
    handled = false;

  // Construct `if _import_done.__invert__(): (_import(); _import_done = True)`.
  // Do not do this during the standard library loading (we assume that standard library
  // imports are "clean" and do not need guards). Note that the importVar is empty if
  // the import has been loaded during the standard library loading.
  if (!handled) {
    resultStmt =
        N<ExprStmt>(N<CallExpr>(N<IdExpr>(fmt::format("{}_call.0", importVar))));
    LOG_TYPECHECK("[import] loading {}", importVar);
  }

  // Import requested identifiers from the import's scope to the current scope
  if (!stmt->getWhat()) {
    // Case: import foo
    auto name = stmt->as.empty() ? path : stmt->getAs();
    auto e = ctx->forceFind(importVar);
    ctx->add(name, e);
  } else if (cast<IdExpr>(stmt->getWhat()) &&
             cast<IdExpr>(stmt->getWhat())->getValue() == "*") {
    // Case: from foo import *
    seqassert(stmt->getAs().empty(), "renamed star-import");
    // Just copy all symbols from import's context here.
    for (auto &[i, ival] : *(import->ctx)) {
      if ((!startswith(i, "_") || (ctx->isStdlibLoading && startswith(i, "__")))) {
        // Ignore all identifiers that start with `_` but not those that start with
        // `__` while the standard library is being loaded
        auto c = ival.front();
        if (c->isConditional() && i.find('.') == std::string::npos)
          c = import->ctx->find(i);
        // Imports should ignore noShadow property
        ctx->add(i, c);
      }
    }
  } else {
    // Case 3: from foo import bar
    auto i = cast<IdExpr>(stmt->getWhat());
    seqassert(i, "not a valid import what expression");
    auto c = import->ctx->find(i->getValue());
    // Make sure that we are importing an existing global symbol
    if (!c)
      E(Error::IMPORT_NO_NAME, i, i->getValue(), file->module);
    if (c->isConditional())
      c = import->ctx->find(i->getValue());
    // Imports should ignore noShadow property
    ctx->add(stmt->getAs().empty() ? i->getValue() : stmt->getAs(), c);
  }
  resultStmt = transform(!resultStmt ? N<SuiteStmt>() : resultStmt); // erase it
}

/// Transform special `from C` and `from python` imports.
/// See @c transformCImport, @c transformCDLLImport and @c transformPythonImport
Stmt *TypecheckVisitor::transformSpecialImport(ImportStmt *stmt) {
  if (auto fi = cast<IdExpr>(stmt->getFrom())) {
    if (fi->getValue() == "C") {
      auto wi = cast<IdExpr>(stmt->getWhat());
      if (wi && !stmt->isCVar()) {
        // C function imports
        return transformCImport(wi->getValue(), stmt->getArgs(), stmt->getReturnType(),
                                stmt->getAs());
      } else if (wi) {
        // C variable imports
        return transformCVarImport(wi->getValue(), stmt->getReturnType(),
                                   stmt->getAs());
      } else if (auto de = cast<DotExpr>(stmt->getWhat())) {
        // dylib C imports
        return transformCDLLImport(de->getExpr(), de->getMember(), stmt->getArgs(),
                                   stmt->getReturnType(), stmt->getAs(),
                                   !stmt->isCVar());
      }
    } else if (fi->getValue() == "python" && stmt->getWhat()) {
      // Python imports
      return transformPythonImport(stmt->getWhat(), stmt->getArgs(),
                                   stmt->getReturnType(), stmt->getAs());
    }
  }
  return nullptr;
}

/// Transform Dot(Dot(a, b), c...) into "{a, b, c, ...}".
/// Useful for getting import paths.
std::vector<std::string> TypecheckVisitor::getImportPath(Expr *from, size_t dots) {
  std::vector<std::string> components; // Path components
  if (from) {
    for (; cast<DotExpr>(from); from = cast<DotExpr>(from)->getExpr())
      components.push_back(cast<DotExpr>(from)->getMember());
    seqassert(cast<IdExpr>(from), "invalid import statement");
    components.push_back(cast<IdExpr>(from)->getValue());
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
Stmt *TypecheckVisitor::transformCImport(const std::string &name,
                                         const std::vector<Param> &args, Expr *ret,
                                         const std::string &altName) {
  std::vector<Param> fnArgs;
  bool hasVarArgs = false;
  for (size_t ai = 0; ai < args.size(); ai++) {
    seqassert(args[ai].getName().empty(), "unexpected argument name");
    seqassert(!args[ai].getDefault(), "unexpected default argument");
    seqassert(args[ai].getType(), "missing type");
    if (cast<EllipsisExpr>(args[ai].getType()) && ai + 1 == args.size()) {
      // C VAR_ARGS support
      hasVarArgs = true;
      fnArgs.emplace_back("*args", nullptr, nullptr);
    } else {
      fnArgs.emplace_back(args[ai].getName().empty() ? format("a{}", ai)
                                                     : args[ai].getName(),
                          clone(args[ai].getType()), nullptr);
    }
  }
  ctx->generateCanonicalName(name); // avoid canonicalName == name
  Stmt *f =
      N<FunctionStmt>(name, ret ? clone(ret) : N<IdExpr>("NoneType"), fnArgs, nullptr);
  f->setAttribute(Attr::C);
  if (hasVarArgs)
    f->setAttribute(Attr::CVarArg);
  f = transform(f); // Already in the preamble
  if (!altName.empty()) {
    auto v = ctx->find(altName);
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
Stmt *TypecheckVisitor::transformCVarImport(const std::string &name, Expr *type,
                                            const std::string &altName) {
  auto canonical = ctx->generateCanonicalName(name);
  auto typ = transformType(clone(type));
  auto val = ctx->addVar(
      altName.empty() ? name : altName, canonical,
      std::make_shared<types::LinkType>(extractClassType(typ)->shared_from_this()),
      getTime());
  auto s = N<AssignStmt>(N<IdExpr>(canonical), nullptr, typ);
  s->lhs->setAttribute(Attr::ExprExternVar);
  s->lhs->setType(val->type);
  s->lhs->setDone();
  s->setDone();
  return s;
}

/// Transform a dynamic C import.
/// @example
///   `from C import lib.foo(int) -> float as f` ->
///   `f = _dlsym(lib, "foo", Fn=Function[[int], float]); f`
/// No return type implies void return type.
Stmt *TypecheckVisitor::transformCDLLImport(Expr *dylib, const std::string &name,
                                            const std::vector<Param> &args, Expr *ret,
                                            const std::string &altName,
                                            bool isFunction) {
  Expr *type = nullptr;
  if (isFunction) {
    std::vector<Expr *> fnArgs{N<ListExpr>(), ret ? clone(ret) : N<IdExpr>("NoneType")};
    for (const auto &a : args) {
      seqassert(a.getName().empty(), "unexpected argument name");
      seqassert(!a.getDefault(), "unexpected default argument");
      seqassert(a.getType(), "missing type");
      cast<ListExpr>(fnArgs[0])->items.emplace_back(clone(a.getType()));
    }
    type = N<IndexExpr>(N<IdExpr>("Function"), N<TupleExpr>(fnArgs));
  } else {
    type = clone(ret);
  }

  Expr *c = clone(dylib);
  return transform(N<AssignStmt>(
      N<IdExpr>(altName.empty() ? name : altName),
      N<CallExpr>(N<IdExpr>("_dlsym"),
                  std::vector<CallArg>{
                      CallArg(c), CallArg(N<StringExpr>(name)), {"Fn", type}})));
}

/// Transform a Python module and function imports.
/// @example
///   `from python import module as f` -> `f = pyobj._import("module")`
///   `from python import lib.foo(int) -> float as f` ->
///   ```def f(a0: int) -> float:
///        f = pyobj._import("lib")._getattr("foo")
///        return float.__from_py__(f(a0))```
/// If a return type is nullptr, the function just returns f (raw pyobj).
Stmt *TypecheckVisitor::transformPythonImport(Expr *what,
                                              const std::vector<Param> &args, Expr *ret,
                                              const std::string &altName) {
  // Get a module name (e.g., os.path)
  auto components = getImportPath(what);

  if (!ret && args.empty()) {
    // Simple import: `from python import foo.bar` -> `bar = pyobj._import("foo.bar")`
    return transform(
        N<AssignStmt>(N<IdExpr>(altName.empty() ? components.back() : altName),
                      N<CallExpr>(N<DotExpr>(N<IdExpr>("pyobj"), "_import"),
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
          N<DotExpr>(N<CallExpr>(N<DotExpr>(N<IdExpr>("pyobj"), "_import"),
                                 N<StringExpr>(combine2(components, ".", 0,
                                                        int(components.size()) - 1))),
                     "_getattr"),
          N<StringExpr>(components.back())));
  // f(a1, ...)
  std::vector<Param> params;
  std::vector<Expr *> callArgs;
  for (int i = 0; i < args.size(); i++) {
    params.emplace_back(format("a{}", i), clone(args[i].getType()), nullptr);
    callArgs.emplace_back(N<IdExpr>(format("a{}", i)));
  }
  // `return ret.__from_py__(f(a1, ...))`
  auto retType = (ret && !cast<NoneExpr>(ret)) ? clone(ret) : N<IdExpr>("NoneType");
  auto retExpr = N<CallExpr>(N<DotExpr>(clone(retType), "__from_py__"),
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
Stmt *TypecheckVisitor::transformNewImport(const ImportFile &file) {
  // Use a clean context to parse a new file
  auto moduleID = file.module;
  std::replace(moduleID.begin(), moduleID.end(), '.', '_');
  auto ictx = std::make_shared<TypeContext>(ctx->cache, file.path);
  ictx->isStdlibLoading = ctx->isStdlibLoading;
  ictx->moduleName = file;
  auto &import = ctx->cache->imports[file.path];
  import.update(file.module, file.path, ictx);
  import.loadedAtToplevel =
      getImport(ctx->moduleName.path)->loadedAtToplevel &&
      (ctx->isStdlibLoading || (ctx->isGlobal() && ctx->scope.size() == 1));
  auto importVar = import.importVar = getTemporaryVar(format("import_{}", moduleID));
  LOG_TYPECHECK("[import] initializing {} ({})", importVar, import.loadedAtToplevel);

  // __name__ = [import name]
  Stmt *n = nullptr;
  if (file.module != "internal.core") {
    // str is not defined when loading internal.core; __name__ is not needed anyway
    n = N<AssignStmt>(N<IdExpr>("__name__"), N<StringExpr>(ictx->moduleName.module));
    ctx->addBlock();
    preamble->addStmt(transform(
        N<AssignStmt>(N<IdExpr>(importVar),
                      N<CallExpr>(N<IdExpr>("Import.__new__"), N<BoolExpr>(false),
                                  N<StringExpr>(file.path), N<StringExpr>(file.module)),
                      N<IdExpr>("Import"))));
    auto val = ctx->forceFind(importVar);
    ctx->popBlock();
    val->scope = {0};
    val->baseName = "";
    val->moduleName = MODULE_MAIN;
    getImport(STDLIB_IMPORT)->ctx->addToplevel(importVar, val);
    registerGlobal(val->getName());
  }
  auto nodeOrErr = parseFile(ctx->cache, file.path);
  if (!nodeOrErr)
    throw exc::ParserException(nodeOrErr.takeError());
  n = N<SuiteStmt>(n, *nodeOrErr);
  auto tv = TypecheckVisitor(ictx, preamble);
  if (auto err = ScopingVisitor::apply(ctx->cache, n, &ictx->globalShadows))
    throw exc::ParserException(std::move(err));

  if (!ctx->cache->errors.empty())
    throw exc::ParserException(ctx->cache->errors);
  // Add comment to the top of import for easier dump inspection
  auto comment = N<CommentStmt>(format("import: {} at {}", file.module, file.path));
  auto suite = N<SuiteStmt>(comment, n);

  if (ctx->isStdlibLoading) {
    // When loading the standard library, imports are not wrapped.
    // We assume that the standard library has no recursive imports and that all
    // statements are executed before the user-provided code.
    return tv.transform(suite);
  } else {
    // Generate import identifier
    auto stmts = N<SuiteStmt>();
    auto ret = N<ReturnStmt>();
    ret->setAttribute(Attr::Internal); // do not trigger toplevel ReturnStmt error
    stmts->addStmt(N<IfStmt>(N<DotExpr>(N<IdExpr>(importVar), "loaded"), ret));
    stmts->addStmt(N<ExprStmt>(
        N<CallExpr>(N<IdExpr>("Import._set_loaded"),
                    N<CallExpr>(N<IdExpr>("__ptr__"), N<IdExpr>(importVar)))));
    stmts->addStmt(suite);

    // Wrap all imported top-level statements into a function.
    auto fnName = fmt::format("{}_call", importVar);
    Stmt *fn =
        N<FunctionStmt>(fnName, N<IdExpr>("NoneType"), std::vector<Param>{}, stmts);
    fn = tv.transform(fn);
    tv.realize(ictx->forceFind(fnName)->getType());
    preamble->addStmt(fn);
    // LOG_USER("[import] done importing {}", file.module);
  }
  return nullptr;
}

} // namespace codon::ast
