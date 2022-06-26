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

void TypecheckVisitor::visit(BreakStmt *stmt) { stmt->done = true; }

void TypecheckVisitor::visit(ContinueStmt *stmt) { stmt->done = true; }

void TypecheckVisitor::visit(WhileStmt *stmt) {
  stmt->cond = transform(stmt->cond);
  ctx->blockLevel++;
  stmt->suite = transform(stmt->suite);
  ctx->blockLevel--;
  stmt->done = stmt->cond->done && stmt->suite->done;
}

void TypecheckVisitor::visit(ForStmt *stmt) {
  if (stmt->decorator)
    stmt->decorator = transform(stmt->decorator, false, true);

  stmt->iter = transform(stmt->iter);
  // Extract the type of the for variable.
  if (!stmt->iter->getType()->canRealize())
    return; // continue after the iterator is realizable

  auto iterType = stmt->iter->getType()->getClass();
  if (auto tuple = iterType->getHeterogenousTuple()) {
    // Case 1: iterating heterogeneous tuple.
    // Unroll a separate suite for each tuple member.
    auto block = N<SuiteStmt>();
    auto tupleVar = ctx->cache->getTemporaryVar("tuple");
    block->stmts.push_back(N<AssignStmt>(N<IdExpr>(tupleVar), stmt->iter));

    auto cntVar = ctx->cache->getTemporaryVar("idx");
    std::vector<StmtPtr> forBlock;
    for (int ai = 0; ai < tuple->args.size(); ai++) {
      std::vector<StmtPtr> stmts;
      stmts.push_back(N<AssignStmt>(clone(stmt->var),
                                    N<IndexExpr>(N<IdExpr>(tupleVar), N<IntExpr>(ai))));
      stmts.push_back(clone(stmt->suite));
      forBlock.push_back(N<IfStmt>(
          N<BinaryExpr>(N<IdExpr>(cntVar), "==", N<IntExpr>(ai)), N<SuiteStmt>(stmts)));
    }
    block->stmts.push_back(
        N<ForStmt>(N<IdExpr>(cntVar),
                   N<CallExpr>(N<IdExpr>("std.internal.types.range.range"),
                               N<IntExpr>(tuple->args.size())),
                   N<SuiteStmt>(forBlock)));
    ctx->blockLevel++;
    resultStmt = transform(block);
    ctx->blockLevel--;
  } else {
    // Case 2: iterating a generator. Standard for loop logic.
    if (iterType->name != "Generator" && !stmt->wrapped) {
      stmt->iter = transform(N<CallExpr>(N<DotExpr>(stmt->iter, "__iter__")));
      stmt->wrapped = true;
    }
    seqassert(stmt->var->getId(), "for variable corrupt: {}", stmt->var->toString());

    auto e = stmt->var->getId();
    bool changed = in(ctx->cache->replacements, e->value);
    while (auto s = in(ctx->cache->replacements, e->value))
      e->value = s->first;
    if (changed) {
      auto u =
          N<AssignStmt>(N<IdExpr>(format("{}.__used__", e->value)), N<BoolExpr>(true));
      u->setUpdate();
      stmt->suite = N<SuiteStmt>(u, stmt->suite);
    }
    std::string varName = e->value;

    TypePtr varType = nullptr;
    if (stmt->ownVar) {
      varType = ctx->addUnbound(stmt->var.get(), ctx->typecheckLevel);
      ctx->add(TypecheckItem::Var, varName, varType);
    } else {
      auto val = ctx->find(varName);
      seqassert(val, "'{}' not found", varName);
      varType = val->type;
    }
    if ((iterType = stmt->iter->getType()->getClass())) {
      if (iterType->name != "Generator")
        error("for loop expected a generator");
      if (getSrcInfo().line == 378 && varType->toString() == "List[int]" &&
          iterType->generics[0].type->toString() == "int") {
        ctx->find(varName);
      }
      unify(varType, iterType->generics[0].type);
      if (varType->is("void"))
        error("expression with void type");
    }

    unify(stmt->var->type, varType);

    ctx->blockLevel++;
    stmt->suite = transform(stmt->suite);
    ctx->blockLevel--;
    stmt->done = stmt->iter->done && stmt->suite->done;
  }
}

}