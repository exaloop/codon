/*
 * translate.h --- AST-to-IR translation.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parser/ast.h"
#include "parser/cache.h"
#include "parser/common.h"
#include "parser/visitors/translate/translate_ctx.h"
#include "parser/visitors/visitor.h"
#include "sir/sir.h"

namespace seq {
namespace ast {

class TranslateVisitor : public CallbackASTVisitor<ir::Value *, ir::Value *> {
  shared_ptr<TranslateContext> ctx;
  ir::Value *result;

public:
  explicit TranslateVisitor(shared_ptr<TranslateContext> ctx);
  static seq::ir::Module *apply(shared_ptr<Cache> cache, StmtPtr stmts);

  ir::Value *transform(const ExprPtr &expr) override;
  ir::Value *transform(const StmtPtr &stmt) override;

private:
  void defaultVisit(Expr *expr) override;
  void defaultVisit(Stmt *expr) override;

public:
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  void visit(FloatExpr *) override;
  void visit(StringExpr *) override;
  void visit(IdExpr *) override;
  void visit(IfExpr *) override;
  void visit(CallExpr *) override;
  void visit(StackAllocExpr *) override;
  void visit(DotExpr *) override;
  void visit(PtrExpr *) override;
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
  void visit(UpdateStmt *) override;
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(FunctionStmt *) override;
  void visit(ClassStmt *) override;

private:
  ir::types::Type *getType(const types::TypePtr &t);

  void transformFunction(types::FuncType *type, FunctionStmt *ast, ir::Func *func);
  void transformLLVMFunction(types::FuncType *type, FunctionStmt *ast, ir::Func *func);

  template <typename ValueType, typename... Args> ValueType *make(Args &&...args) {
    auto *ret = ctx->getModule()->N<ValueType>(std::forward<Args>(args)...);
    return ret;
  }
};

} // namespace ast
} // namespace seq
