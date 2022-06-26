#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

void TypecheckVisitor::visit(BoolExpr *expr) {
  unify(expr->type, ctx->findInternal("bool"));
  expr->done = true;
}

void TypecheckVisitor::visit(IntExpr *expr) {
  unify(expr->type, ctx->findInternal("int"));
  expr->done = true;
}

void TypecheckVisitor::visit(FloatExpr *expr) {
  unify(expr->type, ctx->findInternal("float"));
  expr->done = true;
}

void TypecheckVisitor::visit(StringExpr *expr) {
  unify(expr->type, ctx->findInternal("str"));
  expr->done = true;
}

}