#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(ImportStmt *stmt) {
  seqassert(!ctx->inClass(), "imports within a class");
  if (stmt->from && stmt->from->isId("C")) {
    /// Handle C imports
    if (auto i = stmt->what->getId())
      resultStmt = transformCImport(i->value, stmt->args, stmt->ret.get(), stmt->as);
    else if (auto d = stmt->what->getDot())
      resultStmt = transformCDLLImport(d->expr.get(), d->member, stmt->args,
                                       stmt->ret.get(), stmt->as);
    else
      seqassert(false, "invalid C import statement");
    return;
  } else if (stmt->from && stmt->from->isId("python") && stmt->what) {
    resultStmt =
        transformPythonImport(stmt->what.get(), stmt->args, stmt->ret.get(), stmt->as);
    return;
  }

  // Transform import a.b.c.d to "a/b/c/d".
  std::vector<std::string> dirs; // Path components
  if (stmt->from) {
    Expr *e = stmt->from.get();
    while (auto d = e->getDot()) {
      dirs.push_back(d->member);
      e = d->expr.get();
    }
    if (!e->getId() || !stmt->args.empty() || stmt->ret ||
        (stmt->what && !stmt->what->getId()))
      error("invalid import statement");
    dirs.push_back(e->getId()->value);
  }
  // Handle dots (e.g. .. in from ..m import x).
  seqassert(stmt->dots >= 0, "negative dots in ImportStmt");
  for (int i = 0; i < stmt->dots - 1; i++)
    dirs.emplace_back("..");
  std::string path;
  for (int i = int(dirs.size()) - 1; i >= 0; i--)
    path += dirs[i] + (i ? "/" : "");
  // Fetch the import!
  auto file = getImportFile(ctx->cache->argv0, path, ctx->getFilename(), false,
                            ctx->cache->module0, ctx->cache->pluginImportPaths);
  if (!file)
    error("cannot locate import '{}'", join(dirs, "."));

  // If the imported file has not been seen before, load it.
  if (ctx->cache->imports.find(file->path) == ctx->cache->imports.end())
    transformNewImport(*file);
  const auto &import = ctx->cache->imports[file->path];
  std::string importVar = import.importVar;
  std::string importDoneVar = importVar + "_done";

  // Import variable is empty if it has already been loaded during the standard library
  // initialization.
  if (!ctx->isStdlibLoading && !importVar.empty()) {
    std::vector<StmtPtr> ifSuite;
    ifSuite.emplace_back(N<ExprStmt>(N<CallExpr>(N<IdExpr>(importVar))));
    ifSuite.emplace_back(N<UpdateStmt>(N<IdExpr>(importDoneVar), N<BoolExpr>(true)));
    resultStmt = N<IfStmt>(N<CallExpr>(N<DotExpr>(importDoneVar, "__invert__")),
                           N<SuiteStmt>(ifSuite));
  }

  // TODO: import bindings and new scoping
  if (!stmt->what) {
    // Case 1: import foo
    auto name = stmt->as.empty() ? path : stmt->as;
    auto var = importVar + "_var";
    resultStmt = N<SuiteStmt>(
        resultStmt, transform(N<AssignStmt>(N<IdExpr>(var),
                                            N<CallExpr>(N<IdExpr>("Import"),
                                                        N<StringExpr>(file->module),
                                                        N<StringExpr>(file->path)),
                                            N<IdExpr>("Import"))));
    ctx->addVar(name, var, stmt->getSrcInfo())->importPath = file->path;
  } else if (stmt->what->isId("*")) {
    // Case 2: from foo import *
    seqassert(stmt->as.empty(), "renamed star-import");
    // Just copy all symbols from import's context here.
    for (auto &i : *(import.ctx))
      if (!startswith(i.first, "_") && i.second.front()->scope.size() == 1) {
        ctx->add(i.first, i.second.front());
        // ctx->add(i.second.front()->canonicalName, i.second.front());
      }
  } else {
    // Case 3: from foo import bar
    auto i = stmt->what->getId();
    seqassert(i, "not a valid import what expression");
    auto c = import.ctx->find(i->value);
    // Make sure that we are importing an existing global symbol
    if (!c || c->scope.size() != 1)
      error("symbol '{}' not found in {}", i->value, file->path);
    ctx->add(stmt->as.empty() ? i->value : stmt->as, c);
    /// TODO: should we generate a top-level binding if a scope is not top-level?
  }
}

StmtPtr SimplifyVisitor::transformCImport(const std::string &name,
                                          const std::vector<Param> &args,
                                          const Expr *ret, const std::string &altName) {
  std::vector<Param> fnArgs;
  auto attr = Attr({Attr::C});
  for (int ai = 0; ai < args.size(); ai++) {
    seqassert(args[ai].name.empty(), "unexpected argument name");
    seqassert(!args[ai].deflt, "unexpected default argument");
    seqassert(args[ai].type, "missing type");
    if (dynamic_cast<EllipsisExpr *>(args[ai].type.get()) && ai + 1 == args.size()) {
      attr.set(Attr::CVarArg);
      fnArgs.emplace_back(Param{"*args", nullptr, nullptr});
    } else {
      fnArgs.emplace_back(
          Param{args[ai].name.empty() ? format("a{}", ai) : args[ai].name,
                args[ai].type->clone(), nullptr});
    }
  }
  auto f = N<FunctionStmt>(name, ret ? ret->clone() : N<IdExpr>("void"), fnArgs,
                           nullptr, attr);
  StmtPtr tf = transform(f); // Already in the preamble
  if (!altName.empty()) {
    ctx->add(altName, ctx->find(name));
    ctx->remove(name);
  }
  return tf;
}

StmtPtr SimplifyVisitor::transformCDLLImport(const Expr *dylib, const std::string &name,
                                             const std::vector<Param> &args,
                                             const Expr *ret,
                                             const std::string &altName) {
  // name : Function[args] = _dlsym(dylib, "name", Fn=Function[args])
  std::vector<ExprPtr> fnArgs{N<ListExpr>(std::vector<ExprPtr>{}),
                              ret ? ret->clone() : N<IdExpr>("void")};
  for (const auto &a : args) {
    seqassert(a.name.empty(), "unexpected argument name");
    seqassert(!a.deflt, "unexpected default argument");
    seqassert(a.type, "missing type");
    const_cast<ListExpr *>(fnArgs[0]->getList())->items.emplace_back(clone(a.type));
  }
  auto type = N<IndexExpr>(N<IdExpr>("Function"), N<TupleExpr>(fnArgs));
  return transform(N<AssignStmt>(
      N<IdExpr>(altName.empty() ? name : altName),
      N<CallExpr>(N<IdExpr>("_dlsym"),
                  std::vector<CallExpr::Arg>{
                      {"", dylib->clone()}, {"", N<StringExpr>(name)}, {"Fn", type}})));
}

StmtPtr SimplifyVisitor::transformPythonImport(const Expr *what,
                                               const std::vector<Param> &args,
                                               const Expr *ret,
                                               const std::string &altName) {
  // Get a module name (e.g. os.path)
  std::vector<std::string> dirs;
  auto e = const_cast<Expr *>(what);
  while (auto d = e->getDot()) {
    dirs.push_back(d->member);
    e = d->expr.get();
  }
  seqassert(e && e->getId(), "invalid import python statement");
  dirs.push_back(e->getId()->value);
  std::string name = dirs[0], lib;
  for (int i = int(dirs.size()) - 1; i > 0; i--)
    lib += dirs[i] + (i > 1 ? "." : "");

  // Simple module import: from python import foo
  if (!ret && args.empty())
    // altName = pyobj._import("name")
    return transform(N<AssignStmt>(
        N<IdExpr>(altName.empty() ? name : altName),
        N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                    N<StringExpr>((lib.empty() ? "" : lib + ".") + name))));

  // Typed function import: from python import foo.bar(int) -> float.
  // f = pyobj._import("lib")._getattr("name")
  auto call = N<AssignStmt>(
      N<IdExpr>("f"), N<CallExpr>(N<DotExpr>(N<CallExpr>(N<DotExpr>("pyobj", "_import"),
                                                         N<StringExpr>(lib)),
                                             "_getattr"),
                                  N<StringExpr>(name)));
  // Make a call expression: f(args...)
  std::vector<Param> params;
  std::vector<ExprPtr> callArgs;
  for (int i = 0; i < args.size(); i++) {
    params.emplace_back(Param{format("a{}", i), clone(args[i].type), nullptr});
    callArgs.emplace_back(N<IdExpr>(format("a{}", i)));
  }
  // Make a return expression: return f(args...),
  // or return retType.__from_py__(f(args...))
  ExprPtr retExpr = N<CallExpr>(N<IdExpr>("f"), callArgs);
  if (ret && !ret->isId("void"))
    retExpr =
        N<CallExpr>(N<DotExpr>(ret->clone(), "__from_py__"), N<DotExpr>(retExpr, "p"));
  StmtPtr retStmt = nullptr;
  if (ret && ret->isId("void"))
    retStmt = N<ExprStmt>(retExpr);
  else
    retStmt = N<ReturnStmt>(retExpr);
  // Return a wrapper function
  return transform(N<FunctionStmt>(altName.empty() ? name : altName,
                                   ret ? ret->clone() : nullptr, params,
                                   N<SuiteStmt>(call, retStmt)));
}

void SimplifyVisitor::transformNewImport(const ImportFile &file) {
  // Use a clean context to parse a new file.
  if (ctx->cache->age)
    ctx->cache->age++;
  auto ictx = std::make_shared<SimplifyContext>(file.path, ctx->cache);
  ictx->isStdlibLoading = ctx->isStdlibLoading;
  ictx->moduleName = file;
  auto import = ctx->cache->imports.insert({file.path, {file.path, ictx}}).first;
  // __name__ = <import name> (set the Python's __name__ variable)
  auto sn =
      SimplifyVisitor(ictx, preamble)
          .transform(N<SuiteStmt>(N<AssignStmt>(N<IdExpr>("__name__"),
                                                N<StringExpr>(ictx->moduleName.module),
                                                N<IdExpr>("str"), true),
                                  parseFile(ctx->cache, file.path)));

  // If we are loading standard library, we won't wrap imports in functions as we
  // assume that standard library has no recursive imports. We will just append the
  // top-level statements as-is.
  auto comment = N<CommentStmt>(format("import: {} at {}", file.module, file.path));
  if (ctx->isStdlibLoading) {
    resultStmt = N<SuiteStmt>(std::vector<StmtPtr>{comment, sn}, true);
  } else {
    // Generate import function identifier.
    std::string importVar = import->second.importVar =
                    ctx->cache->getTemporaryVar(format("import_{}", file.module), '.'),
                importDoneVar;
    // import_done = False (global variable that indicates if an import has been
    // loaded)
    preamble->globals.push_back(N<AssignStmt>(
        N<IdExpr>(importDoneVar = importVar + "_done"), N<BoolExpr>(false)));
    if (!in(ctx->cache->globals, importDoneVar))
      ctx->cache->globals[importDoneVar] = nullptr;
    std::vector<StmtPtr> stmts;
    stmts.push_back(nullptr); // placeholder to be filled later!
    // We need to wrap all imported top-level statements (not signatures! they have
    // already been handled and are in the preamble) into a function. We also take the
    // list of global variables so that we can access them via "global" statement.
    auto processStmt = [&](const StmtPtr &s) {
      if (s->getAssign() && s->getAssign()->lhs->getId()) { // a = ... globals
        auto a = const_cast<AssignStmt *>(s->getAssign());
        bool isStatic =
            a->type && a->type->getIndex() && a->type->getIndex()->expr->isId("Static");
        auto val = ictx->find(a->lhs->getId()->value);
        seqassert(val, "cannot locate '{}' in imported file {}",
                  s->getAssign()->lhs->getId()->value, file.path);
        if (val->kind == SimplifyItem::Var && val->scope.size() == 1 &&
            val->base.empty() && !isStatic) {
          stmts.push_back(N<UpdateStmt>(a->lhs, a->rhs));
          preamble->globals.push_back(
              N<AssignStmt>(a->lhs->clone(), nullptr, clone(a->type)));
          // Add imports manually to the global pool
          // TODO: make this dynamic in id.cpp
          if (!in(ctx->cache->globals, val->canonicalName))
            ctx->cache->globals[val->canonicalName] = nullptr;
        } else {
          stmts.push_back(s);
        }
      } else if (!s->getFunction() && !s->getClass()) {
        stmts.push_back(s);
      } else {
        preamble->globals.push_back(s);
      }
      // stmts.push_back(s);
    };
    processStmt(comment);
    if (auto st = const_cast<SuiteStmt *>(sn->getSuite()))
      for (auto &ss : st->stmts)
        processStmt(ss);
    else
      processStmt(sn);
    stmts[0] = N<SuiteStmt>();
    // Add a def import(): ... manually to the cache and to the preamble (it won't be
    // transformed here!).
    ctx->cache->overloads[importVar].push_back({importVar + ":0", ctx->cache->age});
    ctx->cache->functions[importVar + ":0"].ast =
        N<FunctionStmt>(importVar + ":0", nullptr, std::vector<Param>{},
                        N<SuiteStmt>(stmts), Attr({Attr::ForceRealize}));
    preamble->globals.push_back(ctx->cache->functions[importVar + ":0"].ast->clone());
  }
}

} // namespace codon::ast