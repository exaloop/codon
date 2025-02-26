// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <vector>

#include "codon/compiler/error.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"

namespace codon::ast {

/**
 * Base Seq AST visitor.
 * Each visit() by default calls an appropriate defaultVisit().
 */
struct ASTVisitor {
protected:
  /// Default expression node visitor if a particular visitor is not overloaded.
  virtual void defaultVisit(Expr *expr);
  /// Default statement node visitor if a particular visitor is not overloaded.
  virtual void defaultVisit(Stmt *stmt);

public:
  virtual void visit(NoneExpr *);
  virtual void visit(BoolExpr *);
  virtual void visit(IntExpr *);
  virtual void visit(FloatExpr *);
  virtual void visit(StringExpr *);
  virtual void visit(IdExpr *);
  virtual void visit(StarExpr *);
  virtual void visit(KeywordStarExpr *);
  virtual void visit(TupleExpr *);
  virtual void visit(ListExpr *);
  virtual void visit(SetExpr *);
  virtual void visit(DictExpr *);
  virtual void visit(GeneratorExpr *);
  virtual void visit(IfExpr *);
  virtual void visit(UnaryExpr *);
  virtual void visit(BinaryExpr *);
  virtual void visit(ChainBinaryExpr *);
  virtual void visit(PipeExpr *);
  virtual void visit(IndexExpr *);
  virtual void visit(CallExpr *);
  virtual void visit(DotExpr *);
  virtual void visit(SliceExpr *);
  virtual void visit(EllipsisExpr *);
  virtual void visit(LambdaExpr *);
  virtual void visit(YieldExpr *);
  virtual void visit(AssignExpr *);
  virtual void visit(RangeExpr *);
  virtual void visit(InstantiateExpr *);
  virtual void visit(StmtExpr *);

  virtual void visit(AssignMemberStmt *);
  virtual void visit(SuiteStmt *);
  virtual void visit(BreakStmt *);
  virtual void visit(ContinueStmt *);
  virtual void visit(ExprStmt *);
  virtual void visit(AssignStmt *);
  virtual void visit(DelStmt *);
  virtual void visit(PrintStmt *);
  virtual void visit(ReturnStmt *);
  virtual void visit(YieldStmt *);
  virtual void visit(AssertStmt *);
  virtual void visit(WhileStmt *);
  virtual void visit(ForStmt *);
  virtual void visit(IfStmt *);
  virtual void visit(MatchStmt *);
  virtual void visit(ImportStmt *);
  virtual void visit(TryStmt *);
  virtual void visit(ExceptStmt *);
  virtual void visit(GlobalStmt *);
  virtual void visit(ThrowStmt *);
  virtual void visit(FunctionStmt *);
  virtual void visit(ClassStmt *);
  virtual void visit(YieldFromStmt *);
  virtual void visit(WithStmt *);
  virtual void visit(CustomStmt *);
  virtual void visit(CommentStmt *);
};

/**
 * Callback AST visitor.
 * This visitor extends base ASTVisitor and stores node's source location (SrcObject).
 * Function transform() will visit a node and return the appropriate transformation. As
 * each node type (expression or statement) might return a different type,
 * this visitor is generic for each different return type.
 */
template <typename TE, typename TS>
struct CallbackASTVisitor : public ASTVisitor, public SrcObject {
  virtual TE transform(Expr *expr) = 0;
  virtual TS transform(Stmt *stmt) = 0;

  /// Convenience method that transforms a vector of nodes.
  template <typename T> auto transform(const std::vector<T> &ts) {
    std::vector<T> r;
    for (auto &e : ts)
      r.push_back(transform(e));
    return r;
  }

public:
  void visit(NoneExpr *expr) override {}
  void visit(BoolExpr *expr) override {}
  void visit(IntExpr *expr) override {}
  void visit(FloatExpr *expr) override {}
  void visit(StringExpr *expr) override {}
  void visit(IdExpr *expr) override {}
  void visit(StarExpr *expr) override { transform(expr->expr); }
  void visit(KeywordStarExpr *expr) override { transform(expr->expr); }
  void visit(TupleExpr *expr) override {
    for (auto &i : expr->items)
      transform(i);
  }
  void visit(ListExpr *expr) override {
    for (auto &i : expr->items)
      transform(i);
  }
  void visit(SetExpr *expr) override {
    for (auto &i : expr->items)
      transform(i);
  }
  void visit(DictExpr *expr) override {
    for (auto &i : expr->items)
      transform(i);
  }
  void visit(GeneratorExpr *expr) override { transform(expr->loops); }
  void visit(IfExpr *expr) override {
    transform(expr->cond);
    transform(expr->ifexpr);
    transform(expr->elsexpr);
  }
  void visit(UnaryExpr *expr) override { transform(expr->expr); }
  void visit(BinaryExpr *expr) override {
    transform(expr->lexpr);
    transform(expr->rexpr);
  }
  void visit(ChainBinaryExpr *expr) override {
    for (auto &e : expr->exprs)
      transform(e.second);
  }
  void visit(PipeExpr *expr) override {
    for (auto &e : expr->items)
      transform(e.expr);
  }
  void visit(IndexExpr *expr) override {
    transform(expr->expr);
    transform(expr->index);
  }
  void visit(CallExpr *expr) override {
    transform(expr->expr);
    for (auto &a : expr->items)
      transform(a.value);
  }
  void visit(DotExpr *expr) override { transform(expr->expr); }
  void visit(SliceExpr *expr) override {
    transform(expr->start);
    transform(expr->stop);
    transform(expr->step);
  }
  void visit(EllipsisExpr *expr) override {}
  void visit(LambdaExpr *expr) override {
    for (auto &a : expr->items) {
      transform(a.type);
      transform(a.defaultValue);
    }
    transform(expr->expr);
  }
  void visit(YieldExpr *expr) override {}
  void visit(AssignExpr *expr) override {
    transform(expr->var);
    transform(expr->expr);
  }
  void visit(RangeExpr *expr) override {
    transform(expr->start);
    transform(expr->stop);
  }
  void visit(InstantiateExpr *expr) override {
    transform(expr->expr);
    for (auto &e : expr->items)
      transform(e);
  }
  void visit(StmtExpr *expr) override {
    for (auto &s : expr->items)
      transform(s);
    transform(expr->expr);
  }
  void visit(SuiteStmt *stmt) override {
    for (auto &s : stmt->items)
      transform(s);
  }
  void visit(BreakStmt *stmt) override {}
  void visit(ContinueStmt *stmt) override {}
  void visit(ExprStmt *stmt) override { transform(stmt->expr); }
  void visit(AssignStmt *stmt) override {
    transform(stmt->lhs);
    transform(stmt->rhs);
    transform(stmt->type);
  }
  void visit(AssignMemberStmt *stmt) override {
    transform(stmt->lhs);
    transform(stmt->rhs);
  }
  void visit(DelStmt *stmt) override { transform(stmt->expr); }
  void visit(PrintStmt *stmt) override {
    for (auto &e : stmt->items)
      transform(e);
  }
  void visit(ReturnStmt *stmt) override { transform(stmt->expr); }
  void visit(YieldStmt *stmt) override { transform(stmt->expr); }
  void visit(AssertStmt *stmt) override {
    transform(stmt->expr);
    transform(stmt->message);
  }
  void visit(WhileStmt *stmt) override {
    transform(stmt->cond);
    transform(stmt->suite);
    transform(stmt->elseSuite);
  }
  void visit(ForStmt *stmt) override {
    transform(stmt->var);
    transform(stmt->iter);
    transform(stmt->suite);
    transform(stmt->elseSuite);
    transform(stmt->decorator);
    for (auto &a : stmt->ompArgs)
      transform(a.value);
  }
  void visit(IfStmt *stmt) override {
    transform(stmt->cond);
    transform(stmt->ifSuite);
    transform(stmt->elseSuite);
  }
  void visit(MatchStmt *stmt) override {
    transform(stmt->expr);
    for (auto &m : stmt->items) {
      transform(m.pattern);
      transform(m.guard);
      transform(m.suite);
    }
  }
  void visit(ImportStmt *stmt) override {
    transform(stmt->from);
    transform(stmt->what);
    for (auto &a : stmt->args) {
      transform(a.type);
      transform(a.defaultValue);
    }
    transform(stmt->ret);
  }
  void visit(TryStmt *stmt) override {
    transform(stmt->suite);
    for (auto &a : stmt->items)
      transform(a);
    transform(stmt->elseSuite);
    transform(stmt->finally);
  }
  void visit(ExceptStmt *stmt) override {
    transform(stmt->exc);
    transform(stmt->suite);
  }
  void visit(GlobalStmt *stmt) override {}
  void visit(ThrowStmt *stmt) override {
    transform(stmt->expr);
    transform(stmt->from);
  }
  void visit(FunctionStmt *stmt) override {
    transform(stmt->ret);
    for (auto &a : stmt->items) {
      transform(a.type);
      transform(a.defaultValue);
    }
    transform(stmt->suite);
    for (auto &d : stmt->decorators)
      transform(d);
  }
  void visit(ClassStmt *stmt) override {
    for (auto &a : stmt->items) {
      transform(a.type);
      transform(a.defaultValue);
    }
    transform(stmt->suite);
    for (auto &d : stmt->decorators)
      transform(d);
    for (auto &d : stmt->baseClasses)
      transform(d);
    for (auto &d : stmt->staticBaseClasses)
      transform(d);
  }
  void visit(YieldFromStmt *stmt) override { transform(stmt->expr); }
  void visit(WithStmt *stmt) override {
    for (auto &a : stmt->items)
      transform(a);
    transform(stmt->suite);
  }
  void visit(CustomStmt *stmt) override {
    transform(stmt->expr);
    transform(stmt->suite);
  }
};

/**
 * Callback AST visitor.
 * This visitor extends base ASTVisitor and stores node's source location (SrcObject).
 * Function transform() will visit a node and return the appropriate transformation. As
 * each node type (expression or statement) might return a different type,
 * this visitor is generic for each different return type.
 */
struct ReplacingCallbackASTVisitor : public CallbackASTVisitor<Expr *, Stmt *> {
public:
  void visit(StarExpr *expr) override { expr->expr = transform(expr->expr); }
  void visit(KeywordStarExpr *expr) override { expr->expr = transform(expr->expr); }
  void visit(TupleExpr *expr) override {
    for (auto &i : expr->items)
      i = transform(i);
  }
  void visit(ListExpr *expr) override {
    for (auto &i : expr->items)
      i = transform(i);
  }
  void visit(SetExpr *expr) override {
    for (auto &i : expr->items)
      i = transform(i);
  }
  void visit(DictExpr *expr) override {
    for (auto &i : expr->items)
      i = transform(i);
  }
  void visit(GeneratorExpr *expr) override { expr->loops = transform(expr->loops); }
  void visit(IfExpr *expr) override {
    expr->cond = transform(expr->cond);
    expr->ifexpr = transform(expr->ifexpr);
    expr->elsexpr = transform(expr->elsexpr);
  }
  void visit(UnaryExpr *expr) override { expr->expr = transform(expr->expr); }
  void visit(BinaryExpr *expr) override {
    expr->lexpr = transform(expr->lexpr);
    expr->rexpr = transform(expr->rexpr);
  }
  void visit(ChainBinaryExpr *expr) override {
    for (auto &e : expr->exprs)
      e.second = transform(e.second);
  }
  void visit(PipeExpr *expr) override {
    for (auto &e : expr->items)
      e.expr = transform(e.expr);
  }
  void visit(IndexExpr *expr) override {
    expr->expr = transform(expr->expr);
    expr->index = transform(expr->index);
  }
  void visit(CallExpr *expr) override {
    expr->expr = transform(expr->expr);
    for (auto &a : expr->items)
      a.value = transform(a.value);
  }
  void visit(DotExpr *expr) override { expr->expr = transform(expr->expr); }
  void visit(SliceExpr *expr) override {
    expr->start = transform(expr->start);
    expr->stop = transform(expr->stop);
    expr->step = transform(expr->step);
  }
  void visit(EllipsisExpr *expr) override {}
  void visit(LambdaExpr *expr) override {
    for (auto &a : expr->items) {
      a.type = transform(a.type);
      a.defaultValue = transform(a.defaultValue);
    }
    expr->expr = transform(expr->expr);
  }
  void visit(YieldExpr *expr) override {}
  void visit(AssignExpr *expr) override {
    expr->var = transform(expr->var);
    expr->expr = transform(expr->expr);
  }
  void visit(RangeExpr *expr) override {
    expr->start = transform(expr->start);
    expr->stop = transform(expr->stop);
  }
  void visit(InstantiateExpr *expr) override {
    expr->expr = transform(expr->expr);
    for (auto &e : expr->items)
      e = transform(e);
  }
  void visit(StmtExpr *expr) override {
    for (auto &s : expr->items)
      s = transform(s);
    expr->expr = transform(expr->expr);
  }
  void visit(SuiteStmt *stmt) override {
    for (auto &s : stmt->items)
      s = transform(s);
  }
  void visit(ExprStmt *stmt) override { stmt->expr = transform(stmt->expr); }
  void visit(AssignStmt *stmt) override {
    stmt->lhs = transform(stmt->lhs);
    stmt->rhs = transform(stmt->rhs);
    stmt->type = transform(stmt->type);
  }
  void visit(AssignMemberStmt *stmt) override {
    stmt->lhs = transform(stmt->lhs);
    stmt->rhs = transform(stmt->rhs);
  }
  void visit(DelStmt *stmt) override { stmt->expr = transform(stmt->expr); }
  void visit(PrintStmt *stmt) override {
    for (auto &e : stmt->items)
      e = transform(e);
  }
  void visit(ReturnStmt *stmt) override { stmt->expr = transform(stmt->expr); }
  void visit(YieldStmt *stmt) override { stmt->expr = transform(stmt->expr); }
  void visit(AssertStmt *stmt) override {
    stmt->expr = transform(stmt->expr);
    stmt->message = transform(stmt->message);
  }
  void visit(WhileStmt *stmt) override {
    stmt->cond = transform(stmt->cond);
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
    stmt->elseSuite = SuiteStmt::wrap(transform(stmt->elseSuite));
  }
  void visit(ForStmt *stmt) override {
    stmt->var = transform(stmt->var);
    stmt->iter = transform(stmt->iter);
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
    stmt->elseSuite = SuiteStmt::wrap(transform(stmt->elseSuite));
    stmt->decorator = transform(stmt->decorator);
    for (auto &a : stmt->ompArgs)
      a.value = transform(a.value);
  }
  void visit(IfStmt *stmt) override {
    stmt->cond = transform(stmt->cond);
    stmt->ifSuite = SuiteStmt::wrap(transform(stmt->ifSuite));
    stmt->elseSuite = SuiteStmt::wrap(transform(stmt->elseSuite));
  }
  void visit(MatchStmt *stmt) override {
    stmt->expr = transform(stmt->expr);
    for (auto &m : stmt->items) {
      m.pattern = transform(m.pattern);
      m.guard = transform(m.guard);
      m.suite = SuiteStmt::wrap(transform(m.suite));
    }
  }
  void visit(ImportStmt *stmt) override {
    stmt->from = transform(stmt->from);
    stmt->what = transform(stmt->what);
    for (auto &a : stmt->args) {
      a.type = transform(a.type);
      a.defaultValue = transform(a.defaultValue);
    }
    stmt->ret = transform(stmt->ret);
  }
  void visit(TryStmt *stmt) override {
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
    for (auto &a : stmt->items)
      a = (ExceptStmt *)transform(a);
    stmt->elseSuite = SuiteStmt::wrap(transform(stmt->elseSuite));
    stmt->finally = SuiteStmt::wrap(transform(stmt->finally));
  }
  void visit(ExceptStmt *stmt) override {
    stmt->exc = transform(stmt->exc);
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
  }
  void visit(GlobalStmt *stmt) override {}
  void visit(ThrowStmt *stmt) override {
    stmt->expr = transform(stmt->expr);
    stmt->from = transform(stmt->from);
  }
  void visit(FunctionStmt *stmt) override {
    stmt->ret = transform(stmt->ret);
    for (auto &a : stmt->items) {
      a.type = transform(a.type);
      a.defaultValue = transform(a.defaultValue);
    }
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
    for (auto &d : stmt->decorators)
      d = transform(d);
  }
  void visit(ClassStmt *stmt) override {
    for (auto &a : stmt->items) {
      a.type = transform(a.type);
      a.defaultValue = transform(a.defaultValue);
    }
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
    for (auto &d : stmt->decorators)
      d = transform(d);
    for (auto &d : stmt->baseClasses)
      d = transform(d);
    for (auto &d : stmt->staticBaseClasses)
      d = transform(d);
  }
  void visit(YieldFromStmt *stmt) override { stmt->expr = transform(stmt->expr); }
  void visit(WithStmt *stmt) override {
    for (auto &a : stmt->items)
      a = transform(a);
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
  }
  void visit(CustomStmt *stmt) override {
    stmt->expr = transform(stmt->expr);
    stmt->suite = SuiteStmt::wrap(transform(stmt->suite));
  }
};

} // namespace codon::ast
