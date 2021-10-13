#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify_ctx.h"
#include "codon/parser/visitors/typecheck/typecheck_ctx.h"
#include "codon/util/fmt/format.h"

using fmt::format;

namespace codon {
namespace ast {

using namespace types;

TypecheckVisitor::TypecheckVisitor(std::shared_ptr<TypeContext> ctx,
                                   const std::shared_ptr<std::vector<StmtPtr>> &stmts)
    : ctx(std::move(ctx)), allowVoidExpr(false) {
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

StmtPtr TypecheckVisitor::apply(std::shared_ptr<Cache> cache, StmtPtr stmts) {
  auto ctx = std::make_shared<TypeContext>(cache);
  cache->typeCtx = ctx;
  TypecheckVisitor v(ctx);
  auto infer = v.inferTypes(stmts->clone(), true, "<top>");
  return std::move(infer.second);
}

TypePtr TypecheckVisitor::unify(TypePtr &a, const TypePtr &b) {
  if (!a)
    return a = b;
  seqassert(b, "rhs is nullptr");
  types::Type::Unification undo;
  if (a->unify(b.get(), &undo) >= 0)
    return a;
  undo.undo();
  // LOG("{} / {}", a->debugString(true), b->debugString(true));
  a->unify(b.get(), &undo);
  error("cannot unify {} and {}", a->toString(), b->toString());
  return nullptr;
}

} // namespace ast
} // namespace codon
