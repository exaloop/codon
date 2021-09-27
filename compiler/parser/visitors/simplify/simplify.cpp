/*
 * simplify.h --- AST simplification transformation.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/peg/peg.h"
#include "parser/visitors/simplify/simplify.h"
#include "parser/visitors/simplify/simplify_ctx.h"

using fmt::format;
using std::deque;
using std::dynamic_pointer_cast;
using std::function;
using std::get;
using std::move;
using std::ostream;
using std::pair;
using std::stack;

namespace seq {
namespace ast {

using namespace types;

StmtPtr SimplifyVisitor::apply(shared_ptr<Cache> cache, const StmtPtr &node,
                               const string &file,
                               const unordered_map<string, string> &defines,
                               bool barebones) {
  vector<StmtPtr> stmts;
  auto preamble = make_shared<Preamble>();

  if (!cache->module)
    cache->module = new seq::ir::Module("", cache);

  // Load standard library if it has not been loaded.
  if (!in(cache->imports, STDLIB_IMPORT)) {
    // Load the internal module
    auto stdlib = make_shared<SimplifyContext>(STDLIB_IMPORT, cache);
    auto stdlibPath =
        getImportFile(cache->argv0, STDLIB_INTERNAL_MODULE, "", true, cache->module0);
    if (!stdlibPath ||
        stdlibPath->path.substr(stdlibPath->path.size() - 12) != "__init__.seq")
      ast::error("cannot load standard library");
    if (barebones)
      stdlibPath->path =
          stdlibPath->path.substr(0, stdlibPath->path.size() - 5) + "test__.seq";
    stdlib->setFilename(stdlibPath->path);
    cache->imports[STDLIB_IMPORT] = {stdlibPath->path, stdlib};

    // Add __internal__ class that will store functions needed by other internal
    // classes. We will call them as __internal__.fn because directly calling fn will
    // result in a unresolved dependency cycle.
    {
      auto name = "__internal__";
      auto canonical = stdlib->generateCanonicalName(name);
      stdlib->add(SimplifyItem::Type, name, canonical, true);
      // Generate an AST for each POD type. All of them are tuples.
      cache->classes[canonical].ast =
          make_shared<ClassStmt>(canonical, vector<Param>(), nullptr);
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Add simple POD types to the preamble (these types are defined in LLVM and we
    // cannot properly define them in Seq)
    for (auto &name : {"void", "bool", "byte", "int", "float", "NoneType"}) {
      auto canonical = stdlib->generateCanonicalName(name);
      stdlib->add(SimplifyItem::Type, name, canonical, true);
      // Generate an AST for each POD type. All of them are tuples.
      cache->classes[canonical].ast = make_shared<ClassStmt>(
          canonical, vector<Param>(), nullptr, Attr({Attr::Internal, Attr::Tuple}));
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Add generic POD types to the preamble
    for (auto &name :
         vector<string>{"Ptr", "Generator", TYPE_OPTIONAL, "Int", "UInt"}) {
      auto canonical = stdlib->generateCanonicalName(name);
      stdlib->add(SimplifyItem::Type, name, canonical, true);
      vector<Param> generics;
      auto isInt = string(name) == "Int" || string(name) == "UInt";
      if (isInt)
        generics.emplace_back(
            Param{stdlib->generateCanonicalName("N"),
                  make_shared<IndexExpr>(make_shared<IdExpr>("Static"),
                                         make_shared<IdExpr>("int")),
                  nullptr, true});
      else
        generics.emplace_back(Param{
            stdlib->generateCanonicalName("T"), make_shared<IdExpr>("type"),
            name == TYPE_OPTIONAL ? make_shared<IdExpr>("NoneType") : nullptr, true});
      cache->classes[canonical].ast = make_shared<ClassStmt>(
          canonical, generics, nullptr, Attr({Attr::Internal, Attr::Tuple}));
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Reserve the following static identifiers.
    for (auto name : {"staticlen", "compile_error", "isinstance", "hasattr", "type",
                      "TypeVar", "Callable"})
      stdlib->generateCanonicalName(name);

    // This code must be placed in a preamble (these are not POD types but are
    // referenced by the various preamble Function.N and Tuple.N stubs)
    stdlib->isStdlibLoading = true;
    stdlib->moduleName = {ImportFile::STDLIB, stdlibPath->path, "__init__"};
    auto baseTypeCode =
        "@__internal__\n@tuple\nclass pyobj:\n  p: Ptr[byte]\n"
        "@__internal__\n@tuple\nclass str:\n  ptr: Ptr[byte]\n  len: int\n";
    SimplifyVisitor(stdlib, preamble)
        .transform(parseCode(stdlib->cache, stdlibPath->path, baseTypeCode));
    // Load the standard library
    stdlib->setFilename(stdlibPath->path);
    stmts.push_back(SimplifyVisitor(stdlib, preamble)
                        .transform(parseFile(stdlib->cache, stdlibPath->path)));
    // Add __argv__ variable as __argv__: Array[str]
    preamble->globals.push_back(
        SimplifyVisitor(stdlib, preamble)
            .transform(make_shared<AssignStmt>(
                make_shared<IdExpr>(VAR_ARGV), nullptr,
                make_shared<IndexExpr>(make_shared<IdExpr>("Array"),
                                       make_shared<IdExpr>("str")))));
    stdlib->isStdlibLoading = false;

    // The whole standard library has the age of zero to allow back-references.
    cache->age++;
  }

  auto ctx = make_shared<SimplifyContext>(file, cache);
  cache->imports[file].filename = file;
  cache->imports[file].ctx = ctx;
  cache->imports[MAIN_IMPORT] = {file, ctx};
  ctx->setFilename(file);
  ctx->moduleName = {ImportFile::PACKAGE, file, MODULE_MAIN};
  auto before = make_shared<SuiteStmt>();
  // Load the command-line defines.
  for (auto &d : defines) {
    before->stmts.push_back(make_shared<AssignStmt>(
        make_shared<IdExpr>(d.first), make_shared<IntExpr>(d.second),
        make_shared<IndexExpr>(make_shared<IdExpr>("Static"),
                               make_shared<IdExpr>("int"))));
  }
  // Prepend __name__ = "__main__".
  before->stmts.push_back(make_shared<AssignStmt>(
      make_shared<IdExpr>("__name__"), make_shared<StringExpr>(MODULE_MAIN)));
  // Transform the input node.
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(before));
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(node));

  auto suite = make_shared<SuiteStmt>();
  for (auto &s : preamble->globals)
    suite->stmts.push_back(s);
  for (auto &s : preamble->functions)
    suite->stmts.push_back(s);
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

StmtPtr SimplifyVisitor::apply(shared_ptr<SimplifyContext> ctx, const StmtPtr &node,
                               const string &file, int atAge) {
  vector<StmtPtr> stmts;
  int oldAge = ctx->cache->age;
  if (atAge != -1)
    ctx->cache->age = atAge;
  auto preamble = make_shared<Preamble>();
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(node));
  if (atAge != -1)
    ctx->cache->age = oldAge;
  auto suite = make_shared<SuiteStmt>();
  for (auto &s : preamble->globals)
    suite->stmts.push_back(s);
  for (auto &s : preamble->functions)
    suite->stmts.push_back(s);
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

SimplifyVisitor::SimplifyVisitor(shared_ptr<SimplifyContext> ctx,
                                 shared_ptr<Preamble> preamble)
    : ctx(move(ctx)), preamble(move(preamble)) {}

} // namespace ast
} // namespace seq
