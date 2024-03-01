// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "simplify.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/ctx.h"

using fmt::format;
using namespace codon::error;
namespace codon::ast {

using namespace types;

/// Simplify an AST node. Load standard library if needed.
/// @param cache     Pointer to the shared cache ( @c Cache )
/// @param file      Filename to be used for error reporting
/// @param barebones Use the bare-bones standard library for faster testing
/// @param defines   User-defined static values (typically passed as `codon run -DX=Y`).
///                  Each value is passed as a string.
StmtPtr
SimplifyVisitor::apply(Cache *cache, const StmtPtr &node, const std::string &file,
                       const std::unordered_map<std::string, std::string> &defines,
                       const std::unordered_map<std::string, std::string> &earlyDefines,
                       bool barebones) {
  auto preamble = std::make_shared<std::vector<StmtPtr>>();
  seqassertn(cache->module, "cache's module is not set");

#define N std::make_shared
  // Load standard library if it has not been loaded
  if (!in(cache->imports, STDLIB_IMPORT)) {
    // Load the internal.__init__
    auto stdlib = std::make_shared<SimplifyContext>(STDLIB_IMPORT, cache);
    auto stdlibPath =
        getImportFile(cache->argv0, STDLIB_INTERNAL_MODULE, "", true, cache->module0);
    const std::string initFile = "__init__.codon";
    if (!stdlibPath || !endswith(stdlibPath->path, initFile))
      E(Error::COMPILER_NO_STDLIB);

    /// Use __init_test__ for faster testing (e.g., #%% name,barebones)
    /// TODO: get rid of it one day...
    if (barebones) {
      stdlibPath->path =
          stdlibPath->path.substr(0, stdlibPath->path.size() - initFile.size()) +
          "__init_test__.codon";
    }
    stdlib->setFilename(stdlibPath->path);
    cache->imports[STDLIB_IMPORT] = {stdlibPath->path, stdlib};
    stdlib->isStdlibLoading = true;
    stdlib->moduleName = {ImportFile::STDLIB, stdlibPath->path, "__init__"};
    // Load the standard library
    stdlib->setFilename(stdlibPath->path);
    // Core definitions
    preamble->push_back(SimplifyVisitor(stdlib, preamble)
                            .transform(parseCode(stdlib->cache, stdlibPath->path,
                                                 "from internal.core import *")));
    for (auto &d : earlyDefines) {
      // Load early compile-time defines (for standard library)
      preamble->push_back(
          SimplifyVisitor(stdlib, preamble)
              .transform(
                  N<AssignStmt>(N<IdExpr>(d.first), N<IntExpr>(d.second),
                                N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int")))));
    }
    preamble->push_back(SimplifyVisitor(stdlib, preamble)
                            .transform(parseFile(stdlib->cache, stdlibPath->path)));
    stdlib->isStdlibLoading = false;

    // The whole standard library has the age of zero to allow back-references
    cache->age++;
  }

  // Set up the context and the cache
  auto ctx = std::make_shared<SimplifyContext>(file, cache);
  cache->imports[file].filename = file;
  cache->imports[file].ctx = ctx;
  cache->imports[MAIN_IMPORT] = {file, ctx};
  ctx->setFilename(file);
  ctx->moduleName = {ImportFile::PACKAGE, file, MODULE_MAIN};

  // Prepare the code
  auto suite = N<SuiteStmt>();
  suite->stmts.push_back(N<ClassStmt>(".toplevel", std::vector<Param>{}, nullptr,
                                      std::vector<ExprPtr>{N<IdExpr>(Attr::Internal)}));
  for (auto &d : defines) {
    // Load compile-time defines (e.g., codon run -DFOO=1 ...)
    suite->stmts.push_back(
        N<AssignStmt>(N<IdExpr>(d.first), N<IntExpr>(d.second),
                      N<IndexExpr>(N<IdExpr>("Static"), N<IdExpr>("int"))));
  }
  // Set up __name__
  suite->stmts.push_back(
      N<AssignStmt>(N<IdExpr>("__name__"), N<StringExpr>(MODULE_MAIN)));
  suite->stmts.push_back(node);
  auto n = SimplifyVisitor(ctx, preamble).transform(suite);

  suite = N<SuiteStmt>();
  suite->stmts.push_back(N<SuiteStmt>(*preamble));
  // Add dominated assignment declarations
  if (in(ctx->scope.stmts, ctx->scope.blocks.back()))
    suite->stmts.insert(suite->stmts.end(),
                        ctx->scope.stmts[ctx->scope.blocks.back()].begin(),
                        ctx->scope.stmts[ctx->scope.blocks.back()].end());
  suite->stmts.push_back(n);
#undef N

  if (!ctx->cache->errors.empty())
    throw exc::ParserException();

  return suite;
}

/// Simplify an AST node. Assumes that the standard library is loaded.
StmtPtr SimplifyVisitor::apply(const std::shared_ptr<SimplifyContext> &ctx,
                               const StmtPtr &node, const std::string &file,
                               int atAge) {
  std::vector<StmtPtr> stmts;
  int oldAge = ctx->cache->age;
  if (atAge != -1)
    ctx->cache->age = atAge;
  auto preamble = std::make_shared<std::vector<StmtPtr>>();
  stmts.emplace_back(SimplifyVisitor(ctx, preamble).transform(node));
  if (!ctx->cache->errors.empty())
    throw exc::ParserException();

  if (atAge != -1)
    ctx->cache->age = oldAge;
  auto suite = std::make_shared<SuiteStmt>();
  for (auto &s : *preamble)
    suite->stmts.push_back(s);
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

/**************************************************************************************/

SimplifyVisitor::SimplifyVisitor(std::shared_ptr<SimplifyContext> ctx,
                                 std::shared_ptr<std::vector<StmtPtr>> preamble,
                                 const std::shared_ptr<std::vector<StmtPtr>> &stmts)
    : ctx(std::move(ctx)), preamble(std::move(preamble)) {
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

/**************************************************************************************/

ExprPtr SimplifyVisitor::transform(ExprPtr &expr) { return transform(expr, false); }

/// Transform an expression node.
/// @throw @c ParserException if a node is a type and @param allowTypes is not set
///        (use @c transformType instead).
ExprPtr SimplifyVisitor::transform(ExprPtr &expr, bool allowTypes) {
  if (!expr)
    return nullptr;
  SimplifyVisitor v(ctx, preamble);
  v.prependStmts = prependStmts;
  v.setSrcInfo(expr->getSrcInfo());
  ctx->pushSrcInfo(expr->getSrcInfo());
  expr->accept(v);
  ctx->popSrcInfo();
  if (v.resultExpr) {
    v.resultExpr->attributes |= expr->attributes;
    expr = v.resultExpr;
  }
  if (!allowTypes && expr && expr->isType())
    E(Error::UNEXPECTED_TYPE, expr, "type");
  return expr;
}

/// Transform a type expression node.
/// @param allowTypeOf Set if `type()` expressions are allowed. Usually disallowed in
///                    class/function definitions.
/// @throw @c ParserException if a node is not a type (use @c transform instead).
ExprPtr SimplifyVisitor::transformType(ExprPtr &expr, bool allowTypeOf) {
  auto oldTypeOf = ctx->allowTypeOf;
  ctx->allowTypeOf = allowTypeOf;
  transform(expr, true);
  if (expr && expr->getNone())
    expr->markType();
  ctx->allowTypeOf = oldTypeOf;
  if (expr && !expr->isType())
    E(Error::EXPECTED_TYPE, expr, "type");
  return expr;
}

/// Transform a statement node.
StmtPtr SimplifyVisitor::transform(StmtPtr &stmt) {
  if (!stmt)
    return nullptr;

  SimplifyVisitor v(ctx, preamble);
  v.setSrcInfo(stmt->getSrcInfo());
  ctx->pushSrcInfo(stmt->getSrcInfo());
  try {
    stmt->accept(v);
  } catch (const exc::ParserException &e) {
    ctx->cache->errors.push_back(e);
    // throw;
  }
  ctx->popSrcInfo();
  if (v.resultStmt)
    stmt = v.resultStmt;
  stmt->age = ctx->cache->age;
  if (!v.prependStmts->empty()) {
    // Handle prepends
    if (stmt)
      v.prependStmts->push_back(stmt);
    stmt = N<SuiteStmt>(*v.prependStmts);
    stmt->age = ctx->cache->age;
  }
  return stmt;
}

/// Transform a statement in conditional scope.
/// Because variables and forward declarations within conditional scopes can be
/// added later after the domination analysis, ensure that all such declarations
/// are prepended.
StmtPtr SimplifyVisitor::transformConditionalScope(StmtPtr &stmt) {
  if (stmt) {
    ctx->enterConditionalBlock();
    transform(stmt);
    SuiteStmt *suite = stmt->getSuite();
    if (!suite) {
      stmt = N<SuiteStmt>(stmt);
      suite = stmt->getSuite();
    }
    ctx->leaveConditionalBlock(&suite->stmts);
    return stmt;
  }
  return stmt = nullptr;
}

/**************************************************************************************/

void SimplifyVisitor::visit(StmtExpr *expr) {
  for (auto &s : expr->stmts)
    transform(s);
  transform(expr->expr);
}

void SimplifyVisitor::visit(StarExpr *expr) { transform(expr->what); }

void SimplifyVisitor::visit(KeywordStarExpr *expr) { transform(expr->what); }

/// Only allowed in @c MatchStmt
void SimplifyVisitor::visit(RangeExpr *expr) {
  E(Error::UNEXPECTED_TYPE, expr, "range");
}

/// Handled during the type checking
void SimplifyVisitor::visit(SliceExpr *expr) {
  transform(expr->start);
  transform(expr->stop);
  transform(expr->step);
}

void SimplifyVisitor::visit(SuiteStmt *stmt) {
  for (auto &s : stmt->stmts)
    transform(s);
  resultStmt = N<SuiteStmt>(stmt->stmts); // needed for flattening
}

void SimplifyVisitor::visit(ExprStmt *stmt) { transform(stmt->expr, true); }

void SimplifyVisitor::visit(CustomStmt *stmt) {
  if (stmt->suite) {
    auto fn = ctx->cache->customBlockStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customBlockStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second.second(this, stmt);
  } else {
    auto fn = ctx->cache->customExprStmts.find(stmt->keyword);
    seqassert(fn != ctx->cache->customExprStmts.end(), "unknown keyword {}",
              stmt->keyword);
    resultStmt = fn->second(this, stmt);
  }
}

} // namespace codon::ast
