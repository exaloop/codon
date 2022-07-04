#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

/// Transform tuples.
/// Generate tuple classes (e.g., `Tuple.N`) if not available.
/// @example
///   `(a1, ..., aN)` -> `Tuple.N.__new__(a1, ..., aN)`
void TypecheckVisitor::visit(TupleExpr *expr) {
  for (int ai = 0; ai < expr->items.size(); ai++)
    if (auto star = expr->items[ai]->getStar()) {
      // Case: unpack star expressions (e.g., `*arg` -> `arg.item1, arg.item2, ...`)
      transform(star->what);
      auto t = star->what->type->getClass();
      if (!t) {
        return; // continue later when the type becomes known
      }

      if (!t->getRecord())
        error("can only unpack tuple types");
      auto &ff = ctx->cache->classes[t->name].fields;
      for (int i = 0; i < t->getRecord()->args.size(); i++, ai++) {
        expr->items.insert(expr->items.begin() + ai,
                           transform(N<DotExpr>(clone(star->what), ff[i].name)));
      }
      // Remove the star
      expr->items.erase(expr->items.begin() + ai);
      ai--;
    } else {
      expr->items[ai] = transform(expr->items[ai]);
    }
  auto tupleName = generateTuple(expr->items.size());
  resultExpr =
      transform(N<CallExpr>(N<DotExpr>(tupleName, "__new__"), clone(expr->items)));
}

/// Transform a tuple generator expression.
/// @example
///   `tuple(expr for i in tuple_generator)` -> `Tuple.N.__new__(expr...)`
void TypecheckVisitor::visit(GeneratorExpr *expr) {
  seqassert(expr->kind == GeneratorExpr::Generator && expr->loops.size() == 1 &&
                expr->loops[0].conds.empty(),
            "invalid tuple generator");

  auto gen = transform(expr->loops[0].gen);
  if (!gen->type->getRecord())
    return; // Wait until the iterator can be realized

  auto tuple = gen->type->getRecord();
  if (!startswith(tuple->name, TYPE_TUPLE))
    error("can only iterate over a tuple");

  auto block = N<SuiteStmt>();
  // `tuple = tuple_generator`
  auto tupleVar = ctx->cache->getTemporaryVar("tuple");
  block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

  // `a := tuple[i]` for each i
  std::vector<ExprPtr> items;
  for (int ai = 0; ai < tuple->args.size(); ai++) {
    items.emplace_back(
        N<StmtExpr>(N<AssignStmt>(clone(expr->loops[0].vars),
                                  N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))),
                    clone(expr->expr)));
  }

  // `((a := tuple[0]), (a := tuple[1]), ...)`
  resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(items)));
}

} // namespace codon::ast
