/*
 * visitor.h --- Seq AST visitors.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <memory>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"

namespace seq {
namespace ast {

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
  virtual void visit(PtrExpr *);
  virtual void visit(TupleIndexExpr *);
  virtual void visit(StackAllocExpr *);
  virtual void visit(InstantiateExpr *);
  virtual void visit(StmtExpr *);

  virtual void visit(AssignMemberStmt *);
  virtual void visit(UpdateStmt *);
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
  virtual TE transform(const shared_ptr<Expr> &expr) = 0;
  virtual TS transform(const shared_ptr<Stmt> &stmt) = 0;

  /// Convenience method that transforms a vector of nodes.
  template <typename T> auto transform(const vector<T> &ts) {
    vector<T> r;
    for (auto &e : ts)
      r.push_back(transform(e));
    return r;
  }

  /// Convenience method that constructs a node with the visitor's source location.
  template <typename Tn, typename... Ts> auto N(Ts &&...args) {
    auto t = std::make_shared<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    return t;
  }

  /// Convenience method that constructs a node.
  /// @param s source location.
  template <typename Tn, typename... Ts>
  auto Nx(const seq::SrcObject *s, Ts &&...args) {
    auto t = std::make_shared<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(s->getSrcInfo());
    return t;
  }

  /// Convenience method that raises an error at the current source location.
  template <typename... TArgs> void error(const char *format, TArgs &&...args) {
    ast::error(getSrcInfo(), fmt::format(format, args...).c_str());
  }

  /// Convenience method that raises an error at the source location of p.
  template <typename T, typename... TArgs>
  void error(const T &p, const char *format, TArgs &&...args) {
    ast::error(p->getSrcInfo(), fmt::format(format, args...).c_str());
  }

  /// Convenience method that raises an internal error.
  template <typename T, typename... TArgs>
  void internalError(const char *format, TArgs &&...args) {
    throw exc::ParserException(
        fmt::format("INTERNAL: {}", fmt::format(format, args...), getSrcInfo()));
  }
};

/**
 * Replacement AST visitor.
 * Replaces expressions with transformed values.
 */
struct ReplaceASTVisitor : public ASTVisitor {
  virtual void transform(shared_ptr<Expr> &expr) = 0;
  virtual void transform(shared_ptr<Stmt> &stmt) = 0;

  void visit(NoneExpr *) override;
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  void visit(FloatExpr *) override;
  void visit(StringExpr *) override;
  void visit(IdExpr *) override;
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *) override;
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  void visit(DictExpr *) override;
  void visit(GeneratorExpr *) override;
  void visit(DictGeneratorExpr *) override;
  void visit(IfExpr *) override;
  void visit(UnaryExpr *) override;
  void visit(BinaryExpr *) override;
  void visit(ChainBinaryExpr *) override;
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;
  void visit(CallExpr *) override;
  void visit(DotExpr *) override;
  void visit(SliceExpr *) override;
  void visit(EllipsisExpr *) override;
  void visit(LambdaExpr *) override;
  void visit(YieldExpr *) override;
  void visit(AssignExpr *) override;
  void visit(RangeExpr *) override;
  void visit(PtrExpr *) override;
  void visit(TupleIndexExpr *) override;
  void visit(StackAllocExpr *) override;
  void visit(InstantiateExpr *) override;
  void visit(StmtExpr *) override;

  void visit(AssignMemberStmt *) override;
  void visit(UpdateStmt *) override;
  void visit(SuiteStmt *) override;
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(ExprStmt *) override;
  void visit(AssignStmt *) override;
  void visit(DelStmt *) override;
  void visit(PrintStmt *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(AssertStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  void visit(ImportStmt *) override;
  void visit(TryStmt *) override;
  void visit(GlobalStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(YieldFromStmt *) override;
  void visit(WithStmt *) override;
  void visit(CustomStmt *) override;
};

} // namespace ast
} // namespace seq
