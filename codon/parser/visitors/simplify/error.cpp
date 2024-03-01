// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

/// Transform asserts.
/// @example
///   `assert foo()` ->
///   `if not foo(): raise __internal__.seq_assert([file], [line], "")`
///   `assert foo(), msg` ->
///   `if not foo(): raise __internal__.seq_assert([file], [line], str(msg))`
/// Use `seq_assert_test` instead of `seq_assert` and do not raise anything during unit
/// testing (i.e., when the enclosing function is marked with `@test`).
void SimplifyVisitor::visit(AssertStmt *stmt) {
  ExprPtr msg = N<StringExpr>("");
  if (stmt->message)
    msg = N<CallExpr>(N<IdExpr>("str"), clone(stmt->message));
  auto test = ctx->inFunction() && (ctx->getBase()->attributes &&
                                    ctx->getBase()->attributes->has(Attr::Test));
  auto ex = N<CallExpr>(
      N<DotExpr>("__internal__", test ? "seq_assert_test" : "seq_assert"),
      N<StringExpr>(stmt->getSrcInfo().file), N<IntExpr>(stmt->getSrcInfo().line), msg);
  auto cond = N<UnaryExpr>("!", clone(stmt->expr));
  if (test) {
    resultStmt = transform(N<IfStmt>(cond, N<ExprStmt>(ex)));
  } else {
    resultStmt = transform(N<IfStmt>(cond, N<ThrowStmt>(ex)));
  }
}

void SimplifyVisitor::visit(TryStmt *stmt) {
  transformConditionalScope(stmt->suite);
  for (auto &c : stmt->catches) {
    ctx->enterConditionalBlock();
    if (!c.var.empty()) {
      c.var = ctx->generateCanonicalName(c.var);
      ctx->addVar(ctx->cache->rev(c.var), c.var, c.suite->getSrcInfo());
    }
    transform(c.exc, true);
    transformConditionalScope(c.suite);
    ctx->leaveConditionalBlock();
  }
  transformConditionalScope(stmt->finally);
}

void SimplifyVisitor::visit(ThrowStmt *stmt) { transform(stmt->expr); }

/// Transform with statements.
/// @example
///   `with foo(), bar() as a: ...` ->
///   ```tmp = foo()
///      tmp.__enter__()
///      try:
///        a = bar()
///        a.__enter__()
///        try:
///          ...
///        finally:
///          a.__exit__()
///      finally:
///        tmp.__exit__()```
void SimplifyVisitor::visit(WithStmt *stmt) {
  seqassert(!stmt->items.empty(), "stmt->items is empty");
  std::vector<StmtPtr> content;
  for (auto i = stmt->items.size(); i-- > 0;) {
    std::string var =
        stmt->vars[i].empty() ? ctx->cache->getTemporaryVar("with") : stmt->vars[i];
    content = std::vector<StmtPtr>{
        N<AssignStmt>(N<IdExpr>(var), clone(stmt->items[i])),
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(var, "__enter__"))),
        N<TryStmt>(
            !content.empty() ? N<SuiteStmt>(content) : clone(stmt->suite),
            std::vector<TryStmt::Catch>{},
            N<SuiteStmt>(N<ExprStmt>(N<CallExpr>(N<DotExpr>(var, "__exit__")))))};
  }
  resultStmt = transform(N<SuiteStmt>(content));
}

} // namespace codon::ast
