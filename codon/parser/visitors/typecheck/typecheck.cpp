#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/ctx.h"
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

StmtPtr TypecheckVisitor::apply(Cache *cache, StmtPtr stmts) {
  if (!cache->typeCtx) {
    auto ctx = std::make_shared<TypeContext>(cache);
    cache->typeCtx = ctx;
  }
  TypecheckVisitor v(cache->typeCtx);
  auto infer = v.inferTypes(stmts->clone(), true, "<top>");
  return std::move(infer.second);
}

TypePtr TypecheckVisitor::unify(TypePtr &a, const TypePtr &b, bool undoOnSuccess) {
  if (!a)
    return a = b;
  seqassert(b, "rhs is nullptr");
  types::Type::Unification undo;
  undo.realizator = this;
  if (a->unify(b.get(), &undo) >= 0) {
    if (undoOnSuccess)
      undo.undo();
    return a;
  } else {
    undo.undo();
  }
  if (a->toString()=="._lambda_125:0[int]")
    a->unify(b.get(), &undo);
  if (!undoOnSuccess)
    a->unify(b.get(), &undo);
  error("cannot unify {} and {}", a->toString(), b->toString());
  return nullptr;
}

} // namespace ast
} // namespace codon
