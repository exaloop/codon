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

void TypecheckVisitor::visit(TupleExpr *expr) {
  for (int ai = 0; ai < expr->items.size(); ai++)
    if (auto es = const_cast<StarExpr *>(expr->items[ai]->getStar())) {
      // Case 1: *arg unpacking
      es->what = transform(es->what);
      auto t = es->what->type->getClass();
      if (!t)
        return;
      if (!t->getRecord())
        error("can only unpack tuple types");
      auto &ff = ctx->cache->classes[t->name].fields;
      for (int i = 0; i < t->getRecord()->args.size(); i++, ai++)
        expr->items.insert(expr->items.begin() + ai,
                           transform(N<DotExpr>(clone(es->what), ff[i].name)));
      expr->items.erase(expr->items.begin() + ai);
      ai--;
    } else {
      expr->items[ai] = transform(expr->items[ai]);
    }
  auto name = generateTupleStub(expr->items.size());
  resultExpr = transform(N<CallExpr>(N<DotExpr>(name, "__new__"), clone(expr->items)));
}

void TypecheckVisitor::visit(GeneratorExpr *expr) {
  seqassert(expr->kind == GeneratorExpr::Generator && expr->loops.size() == 1 &&
                expr->loops[0].conds.empty(),
            "invalid tuple generator");

  auto gen = transform(expr->loops[0].gen);
  if (!gen->type->getRecord())
    return; // continue after the iterator is realizable

  auto tuple = gen->type->getRecord();
  if (!startswith(tuple->name, TYPE_TUPLE))
    error("can only iterate over a tuple");

  auto block = N<SuiteStmt>();
  auto tupleVar = ctx->cache->getTemporaryVar("tuple");
  block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), gen));

  std::vector<ExprPtr> items;
  for (int ai = 0; ai < tuple->args.size(); ai++)
    items.emplace_back(
        N<StmtExpr>(N<AssignStmt>(clone(expr->loops[0].vars),
                                  N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))),
                    clone(expr->expr)));
  resultExpr = transform(N<StmtExpr>(block, N<TupleExpr>(items)));
}

}
