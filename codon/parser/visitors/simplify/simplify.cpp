#include "simplify.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify_ctx.h"

using fmt::format;

namespace codon {
namespace ast {

using namespace types;

StmtPtr
SimplifyVisitor::apply(Cache *cache, const StmtPtr &node, const std::string &file,
                       const std::unordered_map<std::string, std::string> &defines,
                       bool barebones) {
  std::vector<StmtPtr> stmts;
  auto preamble = std::make_shared<Preamble>();
  seqassert(cache->module, "cache's module is not set");

  // Load standard library if it has not been loaded.
  if (!in(cache->imports, STDLIB_IMPORT)) {
    // Load the internal module
    auto stdlib = std::make_shared<SimplifyContext>(STDLIB_IMPORT, cache);
    auto stdlibPath =
        getImportFile(cache->argv0, STDLIB_INTERNAL_MODULE, "", true, cache->module0);
    const std::string initFile = "__init__.codon";
    if (!stdlibPath || !endswith(stdlibPath->path, initFile))
      ast::error("cannot load standard library");
    if (barebones)
      stdlibPath->path =
          stdlibPath->path.substr(0, stdlibPath->path.size() - initFile.size()) +
          "__init_test__.codon";
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
          std::make_shared<ClassStmt>(canonical, std::vector<Param>(), nullptr);
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Add simple POD types to the preamble (these types are defined in LLVM and we
    // cannot properly define them in Seq)
    for (auto &name : {"void", "bool", "byte", "int", "float", "NoneType"}) {
      auto canonical = stdlib->generateCanonicalName(name);
      stdlib->add(SimplifyItem::Type, name, canonical, true);
      // Generate an AST for each POD type. All of them are tuples.
      cache->classes[canonical].ast =
          std::make_shared<ClassStmt>(canonical, std::vector<Param>(), nullptr,
                                      Attr({Attr::Internal, Attr::Tuple}));
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Add generic POD types to the preamble
    for (auto &name :
         std::vector<std::string>{"Ptr", "Generator", TYPE_OPTIONAL, "Int", "UInt"}) {
      auto canonical = stdlib->generateCanonicalName(name);
      stdlib->add(SimplifyItem::Type, name, canonical, true);
      std::vector<Param> generics;
      auto isInt = std::string(name) == "Int" || std::string(name) == "UInt";
      if (isInt)
        generics.emplace_back(
            Param{stdlib->generateCanonicalName("N"),
                  std::make_shared<IndexExpr>(std::make_shared<IdExpr>("Static"),
                                              std::make_shared<IdExpr>("int")),
                  nullptr, true});
      else
        generics.emplace_back(Param{
            stdlib->generateCanonicalName("T"), std::make_shared<IdExpr>("type"),
            name == TYPE_OPTIONAL ? std::make_shared<IdExpr>("NoneType") : nullptr,
            true});
      cache->classes[canonical].ast = std::make_shared<ClassStmt>(
          canonical, generics, nullptr, Attr({Attr::Internal, Attr::Tuple}));
      preamble->globals.emplace_back(clone(cache->classes[canonical].ast));
    }
    // Reserve the following static identifiers.
    for (auto name : {"staticlen", "compile_error", "isinstance", "hasattr", "type",
                      "TypeVar", "Callable", "argv", "super", "superf"})
      stdlib->generateCanonicalName(name);
    stdlib->add(SimplifyItem::Var, "super", "super", true);
    stdlib->add(SimplifyItem::Var, "superf", "superf", true);

    // This code must be placed in a preamble (these are not POD types but are
    // referenced by the various preamble Function.N and Tuple.N stubs)
    stdlib->isStdlibLoading = true;
    stdlib->moduleName = {ImportFile::STDLIB, stdlibPath->path, "__init__"};
    auto baseTypeCode =
        "@__internal__\nclass pyobj:\n  p: Ptr[byte]\n"
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
            .transform(std::make_shared<AssignStmt>(
                std::make_shared<IdExpr>(VAR_ARGV), nullptr,
                std::make_shared<IndexExpr>(std::make_shared<IdExpr>("Array"),
                                            std::make_shared<IdExpr>("str")))));
    stdlib->isStdlibLoading = false;

    // The whole standard library has the age of zero to allow back-references.
    cache->age++;
  }

  auto ctx = std::make_shared<SimplifyContext>(file, cache);
  cache->imports[file].filename = file;
  cache->imports[file].ctx = ctx;
  cache->imports[MAIN_IMPORT] = {file, ctx};
  ctx->setFilename(file);
  ctx->moduleName = {ImportFile::PACKAGE, file, MODULE_MAIN};
  auto before = std::make_shared<SuiteStmt>();
  // Load the command-line defines.
  for (auto &d : defines) {
    before->stmts.push_back(std::make_shared<AssignStmt>(
        std::make_shared<IdExpr>(d.first), std::make_shared<IntExpr>(d.second),
        std::make_shared<IndexExpr>(std::make_shared<IdExpr>("Static"),
                                    std::make_shared<IdExpr>("int"))));
  }
  // Prepend __name__ = "__main__".
  before->stmts.push_back(std::make_shared<AssignStmt>(
      std::make_shared<IdExpr>("__name__"), std::make_shared<StringExpr>(MODULE_MAIN)));
  // Transform the input node.
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(before));
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(node));

  auto suite = std::make_shared<SuiteStmt>();
  for (auto &s : preamble->globals)
    suite->stmts.push_back(s);
  for (auto &s : preamble->functions)
    suite->stmts.push_back(s);
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

StmtPtr SimplifyVisitor::apply(std::shared_ptr<SimplifyContext> ctx,
                               const StmtPtr &node, const std::string &file,
                               int atAge) {
  std::vector<StmtPtr> stmts;
  int oldAge = ctx->cache->age;
  if (atAge != -1)
    ctx->cache->age = atAge;
  auto preamble = std::make_shared<Preamble>();
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(node));
  if (atAge != -1)
    ctx->cache->age = oldAge;
  auto suite = std::make_shared<SuiteStmt>();
  for (auto &s : preamble->globals)
    suite->stmts.push_back(s);
  for (auto &s : preamble->functions)
    suite->stmts.push_back(s);
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

SimplifyVisitor::SimplifyVisitor(std::shared_ptr<SimplifyContext> ctx,
                                 std::shared_ptr<Preamble> preamble)
    : ctx(std::move(ctx)), preamble(std::move(preamble)) {}

} // namespace ast
} // namespace codon
