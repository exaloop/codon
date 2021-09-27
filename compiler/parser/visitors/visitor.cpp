/*
 * visitor.cpp --- Seq AST visitors.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include "parser/visitors/visitor.h"
#include "parser/ast.h"

namespace seq {
namespace ast {

void ASTVisitor::defaultVisit(Expr *expr) {}
void ASTVisitor::defaultVisit(Stmt *stmt) {}

void ASTVisitor::visit(NoneExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(BoolExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(IntExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(FloatExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(StringExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(IdExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(StarExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(KeywordStarExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(TupleExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(ListExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(SetExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(DictExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(GeneratorExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(DictGeneratorExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(IfExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(UnaryExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(BinaryExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(ChainBinaryExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(PipeExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(IndexExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(TupleIndexExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(CallExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(StackAllocExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(DotExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(SliceExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(EllipsisExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(PtrExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(LambdaExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(YieldExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(AssignExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(RangeExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(InstantiateExpr *expr) { defaultVisit(expr); }
void ASTVisitor::visit(StmtExpr *expr) { defaultVisit(expr); }

void ASTVisitor::visit(SuiteStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(BreakStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ContinueStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ExprStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(AssignStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(AssignMemberStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(UpdateStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(DelStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(PrintStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ReturnStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(YieldStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(AssertStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(WhileStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ForStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(IfStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(MatchStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ImportStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(TryStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(GlobalStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ThrowStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(FunctionStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(ClassStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(YieldFromStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(WithStmt *stmt) { defaultVisit(stmt); }
void ASTVisitor::visit(CustomStmt *stmt) { defaultVisit(stmt); }

void ReplaceASTVisitor::visit(NoneExpr *expr) {}
void ReplaceASTVisitor::visit(BoolExpr *expr) {}
void ReplaceASTVisitor::visit(IntExpr *expr) {}
void ReplaceASTVisitor::visit(FloatExpr *expr) {}
void ReplaceASTVisitor::visit(StringExpr *expr) {}
void ReplaceASTVisitor::visit(IdExpr *expr) {}
void ReplaceASTVisitor::visit(StarExpr *expr) { transform(expr->what); }
void ReplaceASTVisitor::visit(KeywordStarExpr *expr) { transform(expr->what); }
void ReplaceASTVisitor::visit(TupleExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}
void ReplaceASTVisitor::visit(ListExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}
void ReplaceASTVisitor::visit(SetExpr *expr) {
  for (auto &i : expr->items)
    transform(i);
}
void ReplaceASTVisitor::visit(DictExpr *expr) {
  for (auto &i : expr->items) {
    transform(i.key);
    transform(i.value);
  }
}
void ReplaceASTVisitor::visit(GeneratorExpr *expr) {
  transform(expr->expr);
  for (auto &l : expr->loops) {
    transform(l.vars);
    transform(l.gen);
    for (auto &c : l.conds)
      transform(c);
  }
}
void ReplaceASTVisitor::visit(DictGeneratorExpr *expr) {
  transform(expr->key);
  transform(expr->expr);
  for (auto &l : expr->loops) {
    transform(l.vars);
    transform(l.gen);
    for (auto &c : l.conds)
      transform(c);
  }
}
void ReplaceASTVisitor::visit(IfExpr *expr) {
  transform(expr->cond);
  transform(expr->ifexpr);
  transform(expr->elsexpr);
}
void ReplaceASTVisitor::visit(UnaryExpr *expr) { transform(expr->expr); }
void ReplaceASTVisitor::visit(BinaryExpr *expr) {
  transform(expr->lexpr);
  transform(expr->rexpr);
}
void ReplaceASTVisitor::visit(ChainBinaryExpr *expr) {
  for (auto &e : expr->exprs)
    transform(e.second);
}
void ReplaceASTVisitor::visit(PipeExpr *expr) {
  for (auto &e : expr->items)
    transform(e.expr);
}
void ReplaceASTVisitor::visit(IndexExpr *expr) {
  transform(expr->expr);
  transform(expr->index);
}
void ReplaceASTVisitor::visit(TupleIndexExpr *expr) { transform(expr->expr); }
void ReplaceASTVisitor::visit(CallExpr *expr) {
  transform(expr->expr);
  for (auto &a : expr->args)
    transform(a.value);
}
void ReplaceASTVisitor::visit(StackAllocExpr *expr) {
  transform(expr->typeExpr);
  transform(expr->expr);
}
void ReplaceASTVisitor::visit(DotExpr *expr) { transform(expr->expr); }
void ReplaceASTVisitor::visit(SliceExpr *expr) {
  transform(expr->start);
  transform(expr->stop);
  transform(expr->step);
}
void ReplaceASTVisitor::visit(EllipsisExpr *expr) {}
void ReplaceASTVisitor::visit(PtrExpr *expr) { transform(expr->expr); }
void ReplaceASTVisitor::visit(LambdaExpr *expr) { transform(expr->expr); }
void ReplaceASTVisitor::visit(YieldExpr *expr) {}
void ReplaceASTVisitor::visit(AssignExpr *expr) {
  transform(expr->var);
  transform(expr->expr);
}
void ReplaceASTVisitor::visit(RangeExpr *expr) {
  transform(expr->start);
  transform(expr->stop);
}
void ReplaceASTVisitor::visit(InstantiateExpr *expr) {
  transform(expr->typeExpr);
  for (auto &e : expr->typeParams)
    transform(e);
}
void ReplaceASTVisitor::visit(StmtExpr *expr) {
  for (auto &s : expr->stmts)
    transform(s);
  transform(expr->expr);
}
void ReplaceASTVisitor::visit(SuiteStmt *stmt) {
  for (auto &s : stmt->stmts)
    transform(s);
}
void ReplaceASTVisitor::visit(BreakStmt *stmt) {}
void ReplaceASTVisitor::visit(ContinueStmt *stmt) {}
void ReplaceASTVisitor::visit(ExprStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(AssignStmt *stmt) {
  transform(stmt->lhs);
  transform(stmt->rhs);
  transform(stmt->type);
}
void ReplaceASTVisitor::visit(AssignMemberStmt *stmt) {
  transform(stmt->lhs);
  transform(stmt->rhs);
}
void ReplaceASTVisitor::visit(UpdateStmt *stmt) {
  transform(stmt->lhs);
  transform(stmt->rhs);
}
void ReplaceASTVisitor::visit(DelStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(PrintStmt *stmt) {
  for (auto &e : stmt->items)
    transform(e);
}
void ReplaceASTVisitor::visit(ReturnStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(YieldStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(AssertStmt *stmt) {
  transform(stmt->expr);
  transform(stmt->message);
}
void ReplaceASTVisitor::visit(WhileStmt *stmt) {
  transform(stmt->cond);
  transform(stmt->suite);
  transform(stmt->elseSuite);
}
void ReplaceASTVisitor::visit(ForStmt *stmt) {
  transform(stmt->var);
  transform(stmt->iter);
  transform(stmt->suite);
  transform(stmt->elseSuite);
  transform(stmt->decorator);
  for (auto &a : stmt->ompArgs)
    transform(a.value);
}
void ReplaceASTVisitor::visit(IfStmt *stmt) {
  transform(stmt->cond);
  transform(stmt->ifSuite);
  transform(stmt->elseSuite);
}
void ReplaceASTVisitor::visit(MatchStmt *stmt) {
  transform(stmt->what);
  for (auto &m : stmt->cases) {
    transform(m.pattern);
    transform(m.guard);
    transform(m.suite);
  }
}
void ReplaceASTVisitor::visit(ImportStmt *stmt) {
  transform(stmt->from);
  transform(stmt->what);
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.deflt);
  }
  transform(stmt->ret);
}
void ReplaceASTVisitor::visit(TryStmt *stmt) {
  transform(stmt->suite);
  for (auto &a : stmt->catches) {
    transform(a.exc);
    transform(a.suite);
  }
  transform(stmt->finally);
}
void ReplaceASTVisitor::visit(GlobalStmt *stmt) {}
void ReplaceASTVisitor::visit(ThrowStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(FunctionStmt *stmt) {
  transform(stmt->ret);
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.deflt);
  }
  transform(stmt->suite);
  for (auto &d : stmt->decorators)
    transform(d);
}
void ReplaceASTVisitor::visit(ClassStmt *stmt) {
  for (auto &a : stmt->args) {
    transform(a.type);
    transform(a.deflt);
  }
  transform(stmt->suite);
  for (auto &d : stmt->decorators)
    transform(d);
  for (auto &d : stmt->baseClasses)
    transform(d);
}
void ReplaceASTVisitor::visit(YieldFromStmt *stmt) { transform(stmt->expr); }
void ReplaceASTVisitor::visit(WithStmt *stmt) {
  for (auto &a : stmt->items)
    transform(a);
  transform(stmt->suite);
}
void ReplaceASTVisitor::visit(CustomStmt *stmt) {
  transform(stmt->expr);
  transform(stmt->suite);
}

} // namespace ast
} // namespace seq
