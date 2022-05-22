#include <memory>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(DotExpr *expr) {
  /// First flatten the imports.
  Expr *e = expr;
  std::deque<std::string> chain;
  while (auto d = e->getDot()) {
    chain.push_front(d->member);
    e = d->expr.get();
  }
  if (auto d = e->getId()) {
    chain.push_front(d->value);

    /// Check if this is a import or a class access:
    /// (import1.import2...).(class1.class2...)?.method?
    int importEnd = 0, itemEnd = 0;
    std::string importName, itemName;
    std::shared_ptr<SimplifyItem> val = nullptr;
    for (int i = int(chain.size()) - 1; i >= 0; i--) {
      auto s = join(chain, "/", 0, i + 1);
      val = ctx->find(s);
      if (val && val->isImport()) {
        importName = val->importPath;
        importEnd = i + 1;
        break;
      }
    }
    // a.b.c is completely import name
    if (importEnd == chain.size()) {
      resultExpr = transform(N<IdExpr>(val->canonicalName));
      return;
    }
    auto fctx = importName.empty() ? ctx : ctx->cache->imports[importName].ctx;
    for (int i = int(chain.size()) - 1; i >= importEnd; i--) {
      auto s = join(chain, ".", importEnd, i + 1);
      val = fctx->find(s);
      // Make sure that we access only global imported variables.
      if (val && (importName.empty() || val->isType() || val->scope.size() == 1)) {
        itemName = val->canonicalName;
        itemEnd = i + 1;
        //        if (!importName.empty()) TODO: why was this originally here?
        //          ctx->add(val->canonicalName, val);
        break;
      }
    }
    if (itemName.empty() && importName.empty())
      error("identifier '{}' not found", chain[importEnd]);
    if (itemName.empty())
      error("identifier '{}' not found in {}", chain[importEnd], importName);
    resultExpr = N<IdExpr>(itemName);
    if (importName.empty()) // <- needed to transform captures!
      resultExpr = transform(resultExpr, true);
    if (val->isType() && itemEnd == chain.size())
      resultExpr->markType();
    for (int i = itemEnd; i < chain.size(); i++)
      resultExpr = N<DotExpr>(resultExpr, chain[i]);
  } else {
    resultExpr = N<DotExpr>(transform(expr->expr, true), expr->member);
  }
}

} // namespace codon::ast