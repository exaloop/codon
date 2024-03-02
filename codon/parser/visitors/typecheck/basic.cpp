// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Set type to `Optional[?]`
void TypecheckVisitor::visit(NoneExpr *expr) {
  unify(expr->type, ctx->instantiate(ctx->getType(TYPE_OPTIONAL)));
  if (realize(expr->type)) {
    // Realize the appropriate `Optional.__new__` for the translation stage
    auto cls = expr->type->getClass();
    auto f = ctx->forceFind(TYPE_OPTIONAL ".__new__:0")->type;
    auto t = realize(ctx->instantiate(f, cls)->getFunc());
    expr->setDone();
  }
}

/// Set type to `bool`
void TypecheckVisitor::visit(BoolExpr *expr) {
  unify(expr->type, ctx->getType("bool"));
  expr->setDone();
}

/// Set type to `int`
void TypecheckVisitor::visit(IntExpr *expr) {
  unify(expr->type, ctx->getType("int"));
  expr->setDone();
}

/// Set type to `float`
void TypecheckVisitor::visit(FloatExpr *expr) {
  unify(expr->type, ctx->getType("float"));
  expr->setDone();
}

/// Set type to `str`
void TypecheckVisitor::visit(StringExpr *expr) {
  unify(expr->type, ctx->getType("str"));
  expr->setDone();
}

} // namespace codon::ast
