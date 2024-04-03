// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

/**
 * Visitor that infers expression types and performs type-guided transformations.
 *
 * -> Note: this stage *modifies* the provided AST. Clone it before simplification
 *    if you need it intact.
 */
class TypecheckVisitor : public CallbackASTVisitor<ExprPtr, StmtPtr> {
  /// Shared simplification context.
  std::shared_ptr<TypeContext> ctx;
  /// Statements to prepend before the current statement.
  std::shared_ptr<std::vector<StmtPtr>> prependStmts;

  /// Each new expression is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  ExprPtr resultExpr;
  /// Each new statement is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  StmtPtr resultStmt;

public:
  static StmtPtr apply(Cache *cache, const StmtPtr &stmts);

public:
  explicit TypecheckVisitor(
      std::shared_ptr<TypeContext> ctx,
      const std::shared_ptr<std::vector<StmtPtr>> &stmts = nullptr);

public: // Convenience transformators
  ExprPtr transform(ExprPtr &e) override;
  ExprPtr transform(const ExprPtr &expr) override {
    auto e = expr;
    return transform(e);
  }
  StmtPtr transform(StmtPtr &s) override;
  StmtPtr transform(const StmtPtr &stmt) override {
    auto s = stmt;
    return transform(s);
  }
  ExprPtr transformType(ExprPtr &expr);
  ExprPtr transformType(const ExprPtr &expr) {
    auto e = expr;
    return transformType(e);
  }

private:
  void defaultVisit(Expr *e) override;
  void defaultVisit(Stmt *s) override;

private: // Node typechecking rules
  /* Basic type expressions (basic.cpp) */
  void visit(NoneExpr *) override;
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  void visit(FloatExpr *) override;
  void visit(StringExpr *) override;

  /* Identifier access expressions (access.cpp) */
  void visit(IdExpr *) override;
  void visit(DotExpr *) override;
  ExprPtr transformDot(DotExpr *, std::vector<CallExpr::Arg> * = nullptr);
  ExprPtr getClassMember(DotExpr *, std::vector<CallExpr::Arg> *);
  types::TypePtr findSpecialMember(const std::string &);
  types::FuncTypePtr getBestOverload(Expr *, std::vector<CallExpr::Arg> *);
  types::FuncTypePtr getDispatch(const std::string &);

  /* Collection and comprehension expressions (collections.cpp) */
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  void visit(DictExpr *) override;
  void visit(GeneratorExpr *) override;
  ExprPtr transformComprehension(const std::string &, const std::string &,
                                 std::vector<ExprPtr> &);

  /* Conditional expression and statements (cond.cpp) */
  void visit(IfExpr *) override;
  void visit(IfStmt *) override;

  /* Operators (op.cpp) */
  void visit(UnaryExpr *) override;
  ExprPtr evaluateStaticUnary(UnaryExpr *);
  void visit(BinaryExpr *) override;
  ExprPtr evaluateStaticBinary(BinaryExpr *);
  ExprPtr transformBinarySimple(BinaryExpr *);
  ExprPtr transformBinaryIs(BinaryExpr *);
  std::pair<std::string, std::string> getMagic(const std::string &);
  ExprPtr transformBinaryInplaceMagic(BinaryExpr *, bool);
  ExprPtr transformBinaryMagic(BinaryExpr *);
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;
  std::pair<bool, ExprPtr> transformStaticTupleIndex(const types::ClassTypePtr &,
                                                     const ExprPtr &, const ExprPtr &);
  int64_t translateIndex(int64_t, int64_t, bool = false);
  int64_t sliceAdjustIndices(int64_t, int64_t *, int64_t *, int64_t);
  void visit(InstantiateExpr *) override;
  void visit(SliceExpr *) override;

  /* Calls (call.cpp) */
  /// Holds partial call information for a CallExpr.
  struct PartialCallData {
    bool isPartial = false;                   // true if the call is partial
    std::string var;                          // set if calling a partial type itself
    std::vector<char> known = {};             // mask of known arguments
    ExprPtr args = nullptr, kwArgs = nullptr; // partial *args/**kwargs expressions
  };
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *) override;
  void visit(EllipsisExpr *) override;
  void visit(CallExpr *) override;
  bool transformCallArgs(std::vector<CallExpr::Arg> &);
  std::pair<types::FuncTypePtr, ExprPtr> getCalleeFn(CallExpr *, PartialCallData &);
  ExprPtr callReorderArguments(types::FuncTypePtr, CallExpr *, PartialCallData &);
  bool typecheckCallArgs(const types::FuncTypePtr &, std::vector<CallExpr::Arg> &);
  std::pair<bool, ExprPtr> transformSpecialCall(CallExpr *);
  ExprPtr transformSuperF(CallExpr *expr);
  ExprPtr transformSuper();
  ExprPtr transformPtr(CallExpr *expr);
  ExprPtr transformArray(CallExpr *expr);
  ExprPtr transformIsInstance(CallExpr *expr);
  ExprPtr transformStaticLen(CallExpr *expr);
  ExprPtr transformHasAttr(CallExpr *expr);
  ExprPtr transformGetAttr(CallExpr *expr);
  ExprPtr transformSetAttr(CallExpr *expr);
  ExprPtr transformCompileError(CallExpr *expr);
  ExprPtr transformTupleFn(CallExpr *expr);
  ExprPtr transformTypeFn(CallExpr *expr);
  ExprPtr transformRealizedFn(CallExpr *expr);
  ExprPtr transformStaticPrintFn(CallExpr *expr);
  ExprPtr transformHasRttiFn(CallExpr *expr);
  std::pair<bool, ExprPtr> transformInternalStaticFn(CallExpr *expr);
  std::vector<types::ClassTypePtr> getSuperTypes(const types::ClassTypePtr &cls);
  void addFunctionGenerics(const types::FuncType *t);
  std::string generatePartialStub(const std::vector<char> &mask, types::FuncType *fn);

  /* Assignments (assign.cpp) */
  void visit(AssignStmt *) override;
  void transformUpdate(AssignStmt *);
  void visit(AssignMemberStmt *) override;
  std::pair<bool, ExprPtr> transformInplaceUpdate(AssignStmt *);

  /* Loops (loops.cpp) */
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  StmtPtr transformHeterogenousTupleFor(ForStmt *);
  StmtPtr transformStaticForLoop(ForStmt *);

  /* Errors and exceptions (error.cpp) */
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;

  /* Functions (function.cpp) */
  void visit(YieldExpr *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(FunctionStmt *) override;
  ExprPtr partializeFunction(const types::FuncTypePtr &);
  std::shared_ptr<types::RecordType> getFuncTypeBase(size_t);

public:
  types::FuncTypePtr makeFunctionType(FunctionStmt *);

private:
  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  void parseBaseClasses(ClassStmt *);
  std::string generateTuple(size_t, const std::string & = TYPE_TUPLE,
                            std::vector<std::string> = {}, bool = true);

  /* The rest (typecheck.cpp) */
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  void visit(StmtExpr *) override;
  void visit(CommentStmt *stmt) override;

private:
  /* Type inference (infer.cpp) */
  types::TypePtr unify(types::TypePtr &a, const types::TypePtr &b);
  types::TypePtr unify(types::TypePtr &&a, const types::TypePtr &b) {
    auto x = a;
    return unify(x, b);
  }
  StmtPtr inferTypes(StmtPtr, bool isToplevel = false);
  types::TypePtr realize(types::TypePtr);
  types::TypePtr realizeFunc(types::FuncType *, bool = false);
  types::TypePtr realizeType(types::ClassType *);
  std::shared_ptr<FunctionStmt> generateSpecialAst(types::FuncType *);
  size_t getRealizationID(types::ClassType *, types::FuncType *);
  codon::ir::types::Type *makeIRType(types::ClassType *);
  codon::ir::Func *
  makeIRFunction(const std::shared_ptr<Cache::Function::FunctionRealization> &);

private:
  types::FuncTypePtr findBestMethod(const types::ClassTypePtr &typ,
                                    const std::string &member,
                                    const std::vector<types::TypePtr> &args);
  types::FuncTypePtr findBestMethod(const types::ClassTypePtr &typ,
                                    const std::string &member,
                                    const std::vector<ExprPtr> &args);
  types::FuncTypePtr
  findBestMethod(const types::ClassTypePtr &typ, const std::string &member,
                 const std::vector<std::pair<std::string, types::TypePtr>> &args);
  int canCall(const types::FuncTypePtr &, const std::vector<CallExpr::Arg> &,
              std::shared_ptr<types::PartialType> = nullptr);
  std::vector<types::FuncTypePtr>
  findMatchingMethods(const types::ClassTypePtr &typ,
                      const std::vector<types::FuncTypePtr> &methods,
                      const std::vector<CallExpr::Arg> &args);
  bool wrapExpr(ExprPtr &expr, const types::TypePtr &expectedType,
                const types::FuncTypePtr &callee = nullptr, bool allowUnwrap = true);
  ExprPtr castToSuperClass(ExprPtr expr, types::ClassTypePtr superTyp, bool = false);
  StmtPtr prepareVTables();

public:
  bool isTuple(const std::string &s) const { return s == TYPE_TUPLE; }
  std::vector<Cache::Class::ClassField> &getClassFields(types::ClassType *);

  friend class Cache;
  friend class types::CallableTrait;
  friend class types::UnionType;

private: // Helpers
  std::shared_ptr<std::vector<std::pair<std::string, types::TypePtr>>>
      unpackTupleTypes(ExprPtr);
  std::pair<bool, std::vector<std::shared_ptr<codon::SrcObject>>>
  transformStaticLoopCall(const std::vector<std::string> &, ExprPtr,
                          std::function<std::shared_ptr<codon::SrcObject>(StmtPtr)>);
};

} // namespace codon::ast
