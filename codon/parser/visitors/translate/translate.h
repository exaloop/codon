// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/cir/cir.h"
#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/translate/translate_ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

class TranslateVisitor : public CallbackASTVisitor<ir::Value *, ir::Value *> {
  std::shared_ptr<TranslateContext> ctx;
  ir::Value *result;

public:
  explicit TranslateVisitor(std::shared_ptr<TranslateContext> ctx);
  static codon::ir::Func *apply(Cache *cache, const StmtPtr &stmts);

  ir::Value *transform(const ExprPtr &expr) override;
  ir::Value *transform(const StmtPtr &stmt) override;

private:
  void defaultVisit(Expr *expr) override;
  void defaultVisit(Stmt *expr) override;

public:
  void visit(NoneExpr *) override;
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  void visit(FloatExpr *) override;
  void visit(StringExpr *) override;
  void visit(IdExpr *) override;
  void visit(IfExpr *) override;
  void visit(CallExpr *) override;
  void visit(DotExpr *) override;
  void visit(YieldExpr *) override;
  void visit(StmtExpr *) override;
  void visit(PipeExpr *) override;

  void visit(SuiteStmt *) override;
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(ExprStmt *) override;
  void visit(AssignStmt *) override;
  void visit(AssignMemberStmt *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  void visit(IfStmt *) override;
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;
  void visit(CommentStmt *) override {}

private:
  ir::types::Type *getType(const types::TypePtr &t);

  void transformFunctionRealizations(const std::string &name, bool isLLVM);
  void transformFunction(types::FuncType *type, FunctionStmt *ast, ir::Func *func);
  void transformLLVMFunction(types::FuncType *type, FunctionStmt *ast, ir::Func *func);

  template <typename ValueType, typename... Args> ValueType *make(Args &&...args) {
    auto *ret = ctx->getModule()->N<ValueType>(std::forward<Args>(args)...);
    return ret;
  }
};

} // namespace codon::ast
