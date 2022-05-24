#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(TryStmt *stmt) {
  std::vector<TryStmt::Catch> catches;
  auto suite = transformInScope(stmt->suite);
  for (auto &c : stmt->catches) {
    ctx->addScope();
    auto var = c.var;
    if (!c.var.empty()) {
      var = ctx->generateCanonicalName(c.var);
      ctx->addVar(c.var, var, c.suite->getSrcInfo());
    }
    catches.push_back({var, transformType(c.exc), transformInScope(c.suite)});
    ctx->popScope();
  }
  resultStmt = N<TryStmt>(suite, catches, transformInScope(stmt->finally));
}

void SimplifyVisitor::visit(ThrowStmt *stmt) {
  resultStmt = N<ThrowStmt>(transform(stmt->expr));
}

void SimplifyVisitor::visit(WithStmt *stmt) {
  seqassert(!stmt->items.empty(), "stmt->items is empty");
  std::vector<StmtPtr> content;
  for (int i = int(stmt->items.size()) - 1; i >= 0; i--) {
    std::string var =
        stmt->vars[i].empty() ? ctx->cache->getTemporaryVar("with") : stmt->vars[i];
    content = std::vector<StmtPtr>{
        N<AssignStmt>(N<IdExpr>(var), clone(stmt->items[i]), nullptr, true),
        N<ExprStmt>(N<CallExpr>(N<DotExpr>(var, "__enter__"))),
        N<TryStmt>(!content.empty() ? N<SuiteStmt>(content, true) : clone(stmt->suite),
                   std::vector<TryStmt::Catch>{},
                   N<SuiteStmt>(std::vector<StmtPtr>{N<ExprStmt>(
                                    N<CallExpr>(N<DotExpr>(var, "__exit__")))},
                                true))};
  }
  resultStmt = transform(N<SuiteStmt>(content, true));
}

} // namespace codon::ast