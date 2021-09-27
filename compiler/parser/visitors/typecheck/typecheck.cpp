/*
 * typecheck.cpp --- Type inference and type-dependent AST transformations.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include "util/fmt/format.h"
#include <memory>
#include <utility>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/visitors/simplify/simplify_ctx.h"
#include "parser/visitors/typecheck/typecheck.h"
#include "parser/visitors/typecheck/typecheck_ctx.h"

using fmt::format;
using std::deque;
using std::dynamic_pointer_cast;
using std::get;
using std::move;
using std::ostream;
using std::stack;
using std::static_pointer_cast;

namespace seq {
namespace ast {

using namespace types;

TypecheckVisitor::TypecheckVisitor(shared_ptr<TypeContext> ctx,
                                   const shared_ptr<vector<StmtPtr>> &stmts)
    : ctx(move(ctx)), allowVoidExpr(false) {
  prependStmts = stmts ? stmts : make_shared<vector<StmtPtr>>();
}

StmtPtr TypecheckVisitor::apply(shared_ptr<Cache> cache, StmtPtr stmts) {
  auto ctx = make_shared<TypeContext>(cache);
  cache->typeCtx = ctx;
  TypecheckVisitor v(ctx);
  auto infer = v.inferTypes(stmts->clone(), true, "<top>");
  return move(infer.second);
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
} // namespace seq
