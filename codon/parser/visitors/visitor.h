// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
  virtual void visit(DictGeneratorExpr *);
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
  virtual void visit(GlobalStmt *);
  virtual void visit(ThrowStmt *);
  virtual void visit(FunctionStmt *);
  virtual void visit(ClassStmt *);
  virtual void visit(YieldFromStmt *);
  virtual void visit(WithStmt *);
  virtual void visit(CustomStmt *);
  virtual void visit(CommentStmt *);
};

template <typename TE, typename TS>
/**
 * Callback AST visitor.
 * This visitor extends base ASTVisitor and stores node's source location (SrcObject).
 * Function simplify() will visit a node and return the appropriate transformation. As
 * each node type (expression or statement) might return a different type,
 * this visitor is generic for each different return type.
 */
struct CallbackASTVisitor : public ASTVisitor, public SrcObject {
  virtual TE transform(const std::shared_ptr<Expr> &expr) = 0;
  virtual TE transform(std::shared_ptr<Expr> &expr) {
    return transform(static_cast<const std::shared_ptr<Expr> &>(expr));
  }
  virtual TS transform(const std::shared_ptr<Stmt> &stmt) = 0;
  virtual TS transform(std::shared_ptr<Stmt> &stmt) {
    return transform(static_cast<const std::shared_ptr<Stmt> &>(stmt));
  }

  /// Convenience method that transforms a vector of nodes.
  template <typename T> auto transform(const std::vector<T> &ts) {
    std::vector<T> r;
    for (auto &e : ts)
      r.push_back(transform(e));
    return r;
  }

  /// Convenience method that constructs a clone of a node.
  template <typename Tn> auto N(const Tn &ptr) { return std::make_shared<Tn>(ptr); }
  /// Convenience method that constructs a node.
  /// @param s source location.
  template <typename Tn, typename... Ts> auto N(codon::SrcInfo s, Ts &&...args) {
    auto t = std::make_shared<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(s);
    return t;
  }
  /// Convenience method that constructs a node with the visitor's source location.
  template <typename Tn, typename... Ts> auto N(Ts &&...args) {
    auto t = std::make_shared<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    return t;
  }
  template <typename Tn, typename... Ts> auto NT(Ts &&...args) {
    auto t = std::make_shared<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    t->markType();
    return t;
  }

  /// Convenience method that raises an error at the current source location.
  template <typename... TArgs> void error(const char *format, TArgs &&...args) {
    error::raise_error(-1, getSrcInfo(), fmt::format(format, args...).c_str());
  }

  /// Convenience method that raises an error at the source location of p.
  template <typename T, typename... TArgs>
  void error(const T &p, const char *format, TArgs &&...args) {
    error::raise_error(-1, p->getSrcInfo(), fmt::format(format, args...).c_str());
  }

  /// Convenience method that raises an internal error.
  template <typename T, typename... TArgs>
  void internalError(const char *format, TArgs &&...args) {
    throw exc::ParserException(
        fmt::format("INTERNAL: {}", fmt::format(format, args...), getSrcInfo()));
  }

public:
  void visit(NoneExpr *expr) override {}
  void visit(BoolExpr *expr) override {}
  void visit(IntExpr *expr) override {}
  void visit(FloatExpr *expr) override {}
  void visit(StringExpr *expr) override {}
  void visit(IdExpr *expr) override {}
  void visit(StarExpr *expr) override { transform(expr->what); }
  void visit(KeywordStarExpr *expr) override { transform(expr->what); }
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
  void visit(GeneratorExpr *expr) override {
    transform(expr->expr);
    for (auto &l : expr->loops) {
      transform(l.vars);
      transform(l.gen);
      for (auto &c : l.conds)
        transform(c);
    }
  }
  void visit(DictGeneratorExpr *expr) override {
    transform(expr->key);
    transform(expr->expr);
    for (auto &l : expr->loops) {
      transform(l.vars);
      transform(l.gen);
      for (auto &c : l.conds)
        transform(c);
    }
  }
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
    for (auto &a : expr->args)
      transform(a.value);
  }
  void visit(DotExpr *expr) override { transform(expr->expr); }
  void visit(SliceExpr *expr) override {
    transform(expr->start);
    transform(expr->stop);
    transform(expr->step);
  }
  void visit(EllipsisExpr *expr) override {}
  void visit(LambdaExpr *expr) override { transform(expr->expr); }
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
    transform(expr->typeExpr);
    for (auto &e : expr->typeParams)
      transform(e);
  }
  void visit(StmtExpr *expr) override {
    for (auto &s : expr->stmts)
      transform(s);
    transform(expr->expr);
  }
  void visit(SuiteStmt *stmt) override {
    for (auto &s : stmt->stmts)
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
    transform(stmt->what);
    for (auto &m : stmt->cases) {
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
    for (auto &a : stmt->catches) {
      transform(a.exc);
      transform(a.suite);
    }
    transform(stmt->finally);
  }
  void visit(GlobalStmt *stmt) override {}
  void visit(ThrowStmt *stmt) override { transform(stmt->expr); }
  void visit(FunctionStmt *stmt) override {
    transform(stmt->ret);
    for (auto &a : stmt->args) {
      transform(a.type);
      transform(a.defaultValue);
    }
    transform(stmt->suite);
    for (auto &d : stmt->decorators)
      transform(d);
  }
  void visit(ClassStmt *stmt) override {
    for (auto &a : stmt->args) {
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

} // namespace codon::ast
