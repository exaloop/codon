#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

void SimplifyVisitor::visit(ContinueStmt *stmt) {
  if (!ctx->inLoop())
    error("continue outside of a loop");
  resultStmt = stmt->clone();
}

void SimplifyVisitor::visit(BreakStmt *stmt) {
  if (!ctx->inLoop())
    error("break outside of a loop");
  if (!ctx->getLoop()->breakVar.empty()) {
    resultStmt =
        N<SuiteStmt>(transform(N<AssignStmt>(N<IdExpr>(ctx->getLoop()->breakVar),
                                             N<BoolExpr>(false))),
                     stmt->clone());
  } else {
    resultStmt = stmt->clone();
  }
}

void SimplifyVisitor::visit(WhileStmt *stmt) {
  ExprPtr cond = N<CallExpr>(N<DotExpr>(clone(stmt->cond), "__bool__"));
  std::string breakVar;
  StmtPtr assign = nullptr;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    breakVar = ctx->cache->getTemporaryVar("no_break");
    prependStmts->push_back(
        transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true))));
  }
  ctx->addScope();
  ctx->loops.push_back({breakVar, ctx->scope, {}});
  cond = transform(cond);
  StmtPtr whileStmt = N<WhileStmt>(cond, transformInScope(stmt->suite));
  ctx->popScope();
  for (auto &var : ctx->getLoop()->seenVars) {
    ctx->findDominatingBinding(var);
  }
  ctx->loops.pop_back();
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt = N<SuiteStmt>(
        whileStmt, N<IfStmt>(transform(N<CallExpr>(N<DotExpr>(breakVar, "__bool__"))),
                             transformInScope(stmt->elseSuite)));
  } else {
    resultStmt = whileStmt;
  }
}

void SimplifyVisitor::visit(ForStmt *stmt) {
  std::vector<CallExpr::Arg> ompArgs;
  ExprPtr decorator = clone(stmt->decorator);
  if (decorator) {
    ExprPtr callee = decorator;
    if (auto c = callee->getCall())
      callee = c->expr;
    if (!callee || !callee->isId("par"))
      error("for loop can only take parallel decorator");
    std::vector<CallExpr::Arg> args;
    std::string openmp;
    std::vector<CallExpr::Arg> omp;
    if (auto c = decorator->getCall())
      for (auto &a : c->args) {
        if (a.name == "openmp" ||
            (a.name.empty() && openmp.empty() && a.value->getString())) {
          omp = parseOpenMP(ctx->cache, a.value->getString()->getValue(),
                            a.value->getSrcInfo());
        } else {
          args.push_back({a.name, transform(a.value)});
        }
      }
    for (auto &a : omp)
      args.push_back({a.name, transform(a.value)});
    decorator = N<CallExpr>(transform(N<IdExpr>("for_par")), args);
  }

  std::string breakVar;
  auto iter = transform(stmt->iter); // needs in-advance transformation to prevent
                                     // name clashes with the iterator variable
  StmtPtr assign = nullptr;
  StmtPtr forStmt = nullptr;
  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    breakVar = ctx->cache->getTemporaryVar("no_break");
    assign = transform(N<AssignStmt>(N<IdExpr>(breakVar), N<BoolExpr>(true)));
  }
  ctx->addScope();
  ctx->loops.push_back({breakVar, ctx->scope, {}});
  std::string varName;
  if (auto i = stmt->var->getId()) {
    ctx->addVar(i->value, varName = ctx->generateCanonicalName(i->value),
                stmt->var->getSrcInfo());
    auto var = transform(stmt->var);
    forStmt = N<ForStmt>(var, clone(iter), transform(stmt->suite), nullptr, decorator,
                         ompArgs);
  } else {
    varName = ctx->cache->getTemporaryVar("for");
    ctx->addVar(varName, varName, stmt->var->getSrcInfo());
    auto var = N<IdExpr>(varName);
    std::vector<StmtPtr> stmts;
    stmts.push_back(N<AssignStmt>(clone(stmt->var), clone(var)));
    stmts.push_back(clone(stmt->suite));
    forStmt = N<ForStmt>(clone(var), clone(iter), transform(N<SuiteStmt>(stmts)),
                         nullptr, decorator, ompArgs);
  }
  ctx->popScope();
  for (auto &var : ctx->getLoop()->seenVars)
    ctx->findDominatingBinding(var);
  ctx->loops.pop_back();

  if (stmt->elseSuite && stmt->elseSuite->firstInBlock()) {
    resultStmt =
        N<SuiteStmt>(assign, forStmt,
                     N<IfStmt>(transform(N<CallExpr>(N<DotExpr>(breakVar, "__bool__"))),
                               transformInScope(stmt->elseSuite)));
  } else {
    resultStmt = forStmt;
  }
}

} // namespace codon::ast