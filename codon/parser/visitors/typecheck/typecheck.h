#pragma once

#include <map>
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

namespace codon {
namespace ast {

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
  static StmtPtr apply(Cache *cache, StmtPtr stmts);

public:
  explicit TypecheckVisitor(
      std::shared_ptr<TypeContext> ctx,
      const std::shared_ptr<std::vector<StmtPtr>> &stmts = nullptr);

public:
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
  ExprPtr transform(ExprPtr &e, bool allowTypes);
  ExprPtr transform(const ExprPtr &expr, bool allowTypes) {
    auto e = expr;
    return transform(e, allowTypes);
  }
  ExprPtr transformType(ExprPtr &expr);
  ExprPtr transformType(const ExprPtr &expr) {
    auto e = expr;
    return transformType(e);
  }
  types::TypePtr realize(types::TypePtr typ);

private:
  void defaultVisit(Expr *e) override;
  void defaultVisit(Stmt *s) override;

public:
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
  types::FuncTypePtr getBestOverload(Expr *, std::vector<CallExpr::Arg> *);
  types::FuncTypePtr getDispatch(const std::string &);

  /* Collection and comprehension expressions (collections.cpp) */
  void visit(TupleExpr *) override;
  void visit(GeneratorExpr *) override;

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
  std::string getMagic(const std::string &);
  ExprPtr transformBinaryInplaceMagic(BinaryExpr *, bool);
  ExprPtr transformBinaryMagic(BinaryExpr *);
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;
  ExprPtr transformStaticTupleIndex(const types::ClassTypePtr &, ExprPtr &, ExprPtr &);
  int64_t translateIndex(int64_t, int64_t, bool = false);
  int64_t sliceAdjustIndices(int64_t, int64_t *, int64_t *, int64_t);
  void visit(InstantiateExpr *) override;
  void visit(SliceExpr *) override;

  /* Calls (call.cpp) */
  /// Holds partial call information for a CallExpr.
  struct PartialCallData {
    bool isPartial = false;                   // true if the call is partial
    std::string var = "";                     // set if calling a partial type itself
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
  ExprPtr transformSuper(CallExpr *expr);
  ExprPtr transformPtr(CallExpr *expr);
  ExprPtr transformArray(CallExpr *expr);
  ExprPtr transformIsInstance(CallExpr *expr);
  ExprPtr transformStaticLen(CallExpr *expr);
  ExprPtr transformHasAttr(CallExpr *expr);
  ExprPtr transformGetAttr(CallExpr *expr);
  ExprPtr transformCompileError(CallExpr *expr);
  ExprPtr transformTypeFn(CallExpr *expr);
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

  /* Errors and exceptions (error.cpp) */
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;

  /* Functions (function.cpp) */
  void visit(YieldExpr *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(FunctionStmt *) override;
  ExprPtr partializeFunction(const types::FuncTypePtr &);
  std::shared_ptr<types::RecordType> getFuncTypeBase(int nargs);

  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  std::string generateTuple(size_t, const std::string & = "Tuple",
                            std::vector<std::string> = {}, bool = true);

  /* The rest (typecheck.cpp) */
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  /// Use type of an inner expression.
  void visit(StmtExpr *) override;
  void visit(CommentStmt *stmt) override { stmt->done = true; }

public:
  /// Picks the best method of a given expression that matches the given argument
  /// types. Prefers methods whose signatures are closer to the given arguments:
  /// e.g. foo(int) will match (int) better that a foo(T).
  /// Also takes care of the Optional arguments.
  /// If multiple equally good methods are found, return the first one.
  /// Return nullptr if no methods were found.
  types::FuncTypePtr findBestMethod(const Expr *expr, const std::string &member,
                                    const std::vector<types::TypePtr> &args);

private:
  types::FuncTypePtr findBestMethod(const Expr *expr, const std::string &member,
                                    const std::vector<CallExpr::Arg> &args);
  types::FuncTypePtr findBestMethod(const std::string &fn,
                                    const std::vector<CallExpr::Arg> &args);
  std::vector<types::FuncTypePtr> findSuperMethods(const types::FuncTypePtr &func);
  std::vector<types::FuncTypePtr>
  findMatchingMethods(const types::ClassTypePtr &typ,
                      const std::vector<types::FuncTypePtr> &methods,
                      const std::vector<CallExpr::Arg> &args);
  bool wrapExpr(ExprPtr &expr, types::TypePtr expectedType,
                const types::FuncTypePtr &callee = nullptr, bool undoOnSuccess = false,
                bool allowUnwrap = true);

public:
  types::TypePtr unify(types::TypePtr &a, const types::TypePtr &b,
                       bool undoOnSuccess = false);
  types::TypePtr unify(types::TypePtr &&a, const types::TypePtr &b,
                       bool undoOnSuccess = false) {
    auto x = a;
    return unify(x, b, undoOnSuccess);
  }

private:
  types::TypePtr realizeType(types::ClassType *typ);
  types::TypePtr realizeFunc(types::FuncType *typ);
  std::pair<int, StmtPtr> inferTypes(StmtPtr stmt, bool keepLast,
                                     const std::string &name);
  codon::ir::types::Type *getLLVMType(const types::ClassType *t);

  bool isTuple(const std::string &s) const { return startswith(s, TYPE_TUPLE); }
};

} // namespace ast
} // namespace codon
