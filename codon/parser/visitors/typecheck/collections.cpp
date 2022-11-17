#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

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
      auto typ = star->what->type->getClass();
      while (typ && typ->is(TYPE_OPTIONAL)) {
        star->what = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), star->what));
        typ = star->what->type->getClass();
      }
      if (!typ)
        return; // continue later when the type becomes known
      if (!typ->getRecord())
        E(Error::CALL_BAD_UNPACK, star, typ->prettyString());
      auto &ff = ctx->cache->classes[typ->name].fields;
      for (int i = 0; i < typ->getRecord()->args.size(); i++, ai++) {
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

  unify(expr->type, ctx->getUnbound());

  auto gen = transform(expr->loops[0].gen);
  if (!gen->type->canRealize())
    return; // Wait until the iterator can be realized

  auto tuple = gen->type->getRecord();
  if (!tuple || !startswith(tuple->name, TYPE_TUPLE))
    E(Error::CALL_BAD_ITER, gen, gen->type->prettyString());

  auto block = N<SuiteStmt>();
  // `tuple = tuple_generator`
  auto tupleVar = ctx->cache->getTemporaryVar("tuple");
  block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

  // `a := tuple[i]` for each i
  std::vector<ExprPtr> items;
  items.reserve(tuple->args.size());
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
