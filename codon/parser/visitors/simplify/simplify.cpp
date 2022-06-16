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

namespace codon::ast {

using namespace types;

StmtPtr
SimplifyVisitor::apply(Cache *cache, const StmtPtr &node, const std::string &file,
                       const std::unordered_map<std::string, std::string> &defines,
                       bool barebones) {
  auto preamble = std::make_shared<Preamble>();
  seqassertn(cache->module, "cache's module is not set");

  // Load standard library if it has not been loaded
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

    // This code must be placed in a preamble (these are not POD types but are
    // referenced by the various preamble Function.N and Tuple.N stubs)
    stdlib->isStdlibLoading = true;
    stdlib->moduleName = {ImportFile::STDLIB, stdlibPath->path, "__init__"};
    // Load the standard library
    stdlib->setFilename(stdlibPath->path);
    preamble->globals.push_back(
        SimplifyVisitor(stdlib, preamble)
            .transform(parseFile(stdlib->cache, stdlibPath->path)));
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

  auto suite = std::make_shared<SuiteStmt>();
  for (auto &d : defines) {
    suite->stmts.push_back(std::make_shared<AssignStmt>(
        std::make_shared<IdExpr>(d.first), std::make_shared<IntExpr>(d.second),
        std::make_shared<IndexExpr>(std::make_shared<IdExpr>("Static"),
                                    std::make_shared<IdExpr>("int"))));
  }
  suite->stmts.push_back(std::make_shared<AssignStmt>(
      std::make_shared<IdExpr>("__name__"), std::make_shared<StringExpr>(MODULE_MAIN)));
  suite->stmts.push_back(node);
  auto n = SimplifyVisitor(ctx, preamble).transform(suite);

  suite = std::make_shared<SuiteStmt>();
  suite->stmts.push_back(std::make_shared<SuiteStmt>(preamble->globals));
  if (in(ctx->scopeStmts, ctx->scope.back()))
    suite->stmts.insert(suite->stmts.end(), ctx->scopeStmts[ctx->scope.back()].begin(),
                        ctx->scopeStmts[ctx->scope.back()].end());
  AssignReplacementVisitor(ctx->cache, ctx->scopeRenames).transform(n);
  suite->stmts.push_back(n);
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
  for (auto &s : stmts)
    suite->stmts.push_back(s);
  return suite;
}

/**************************************************************************************/

SimplifyVisitor::SimplifyVisitor(std::shared_ptr<SimplifyContext> ctx,
                                 std::shared_ptr<Preamble> preamble,
                                 std::shared_ptr<std::vector<StmtPtr>> stmts)
    : ctx(std::move(ctx)), preamble(std::move(preamble)) {
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

/**************************************************************************************/

ExprPtr SimplifyVisitor::transform(const ExprPtr &expr) {
  return transform(expr, false);
}

/// Transform an expression node.
/// @throw @c ParserException if a node is a type and @param allowTypes is not set
///        (use @c transformType instead).
ExprPtr SimplifyVisitor::transform(const ExprPtr &expr, bool allowTypes) {
  if (!expr)
    return nullptr;
  SimplifyVisitor v(ctx, preamble);
  v.prependStmts = prependStmts;
  v.setSrcInfo(expr->getSrcInfo());
  ctx->pushSrcInfo(expr->getSrcInfo());
  const_cast<Expr *>(expr.get())->accept(v);
  ctx->popSrcInfo();
  if (!allowTypes && v.resultExpr && v.resultExpr->isType())
    error("unexpected type expression");
  if (v.resultExpr)
    v.resultExpr->attributes |= expr->attributes;
  return v.resultExpr;
}

/// Transform a type expression node.
/// @param allowTypeOf Set if `type()` expressions are allowed. Usually disallowed in
///                    class/function definitions.
/// @throw @c ParserException if a node is not a type (use @c transform instead).
ExprPtr SimplifyVisitor::transformType(const ExprPtr &expr, bool allowTypeOf) {
  auto oldTypeOf = ctx->allowTypeOf;
  ctx->allowTypeOf = allowTypeOf;
  auto e = transform(expr, true);
  ctx->allowTypeOf = oldTypeOf;
  if (e && !e->isType())
    error("expected type expression");
  return e;
}

/// Transform a statement node.
StmtPtr SimplifyVisitor::transform(const StmtPtr &stmt) {
  if (!stmt)
    return nullptr;

  SimplifyVisitor v(ctx, preamble);
  v.setSrcInfo(stmt->getSrcInfo());
  ctx->pushSrcInfo(stmt->getSrcInfo());
  const_cast<Stmt *>(stmt.get())->accept(v);
  ctx->popSrcInfo();
  if (v.resultStmt)
    v.resultStmt->age = ctx->cache->age;
  if (!v.prependStmts->empty()) {
    // Handle prepends
    if (v.resultStmt)
      v.prependStmts->push_back(v.resultStmt);
    v.resultStmt = N<SuiteStmt>(*v.prependStmts);
    v.resultStmt->age = ctx->cache->age;
  }
  return v.resultStmt;
}

/// Transform a statement in conditional scope.
/// Because variables and forward declarations within conditiona scopes can be
/// added later after the domination analysis, ensure that all such declarations
/// are prepended.
StmtPtr SimplifyVisitor::transformConditionalScope(const StmtPtr &stmt) {
  if (stmt) {
    ctx->addScope();
    auto s = transform(stmt);
    SuiteStmt *suite = s->getSuite();
    if (!suite) {
      s = N<SuiteStmt>(s);
      suite = s->getSuite();
    }
    ctx->popScope(&suite->stmts);
    return s;
  }
  return nullptr;
}

void SimplifyVisitor::defaultVisit(Expr *e) { resultExpr = e->clone(); }
void SimplifyVisitor::defaultVisit(Stmt *s) { resultStmt = s->clone(); }

/**************************************************************************************/

void SimplifyVisitor::visit(StmtExpr *expr) {
  std::vector<StmtPtr> stmts;
  for (auto &s : expr->stmts)
    stmts.emplace_back(transform(s));
  auto e = transform(expr->expr);
  auto s = N<StmtExpr>(stmts, e);
  s->attributes = expr->attributes;
  resultExpr = s;
}

void SimplifyVisitor::visit(StarExpr *expr) {
  resultExpr = N<StarExpr>(transform(expr->what));
}

void SimplifyVisitor::visit(KeywordStarExpr *expr) {
  resultExpr = N<KeywordStarExpr>(transform(expr->what));
}

/// Manually handled in @c CallExpr
void SimplifyVisitor::visit(EllipsisExpr *expr) {
  error("unexpected ellipsis expression");
}

/// Only allowed in @c MatchStmt
void SimplifyVisitor::visit(RangeExpr *expr) {
  error("unexpected pattern range expression");
}

/// Handled during the type checking
void SimplifyVisitor::visit(SliceExpr *expr) {
  resultExpr = N<SliceExpr>(transform(expr->start), transform(expr->stop),
                            transform(expr->step));
}

void SimplifyVisitor::visit(SuiteStmt *stmt) {
  std::vector<StmtPtr> r;
  for (const auto &s : stmt->stmts)
    SuiteStmt::flatten(transform(s), r);
  resultStmt = N<SuiteStmt>(r);
}

void SimplifyVisitor::visit(ExprStmt *stmt) {
  resultStmt = N<ExprStmt>(transform(stmt->expr, true));
}

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