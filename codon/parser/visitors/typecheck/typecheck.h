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
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

/**
 * Visitor that infers expression types and performs type-guided transformations.
 *
 * -> Note: this stage *modifies* the provided AST. Clone it before simplification
 *    if you need it intact.
 */
class TypecheckVisitor : public ReplacingCallbackASTVisitor {
  /// Shared simplification context.
  std::shared_ptr<TypeContext> ctx;
  /// Statements to prepend before the current statement.
  std::shared_ptr<std::vector<Stmt *>> prependStmts = nullptr;
  std::shared_ptr<std::vector<Stmt *>> preamble = nullptr;

  /// Each new expression is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  Expr *resultExpr = nullptr;
  /// Each new statement is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  Stmt *resultStmt = nullptr;

public:
  // static Stmt * apply(Cache *cache, const Stmt * &stmts);
  static Stmt *
  apply(Cache *cache, Stmt *node, const std::string &file,
        const std::unordered_map<std::string, std::string> &defines = {},
        const std::unordered_map<std::string, std::string> &earlyDefines = {},
        bool barebones = false);
  static Stmt *apply(const std::shared_ptr<TypeContext> &cache, Stmt *node,
                     const std::string &file = "<internal>");

private:
  static void loadStdLibrary(Cache *, const std::shared_ptr<std::vector<Stmt *>> &,
                             const std::unordered_map<std::string, std::string> &,
                             bool);

public:
  explicit TypecheckVisitor(
      std::shared_ptr<TypeContext> ctx,
      const std::shared_ptr<std::vector<Stmt *>> &preamble = nullptr,
      const std::shared_ptr<std::vector<Stmt *>> &stmts = nullptr);

public: // Convenience transformators
  Expr *transform(Expr *e) override;
  Expr *transform(Expr *expr, bool allowTypes);
  Stmt *transform(Stmt *s) override;
  Expr *transformType(Expr *expr, bool allowTypeOf = true);

private:
  void defaultVisit(Expr *e) override;
  void defaultVisit(Stmt *s) override;

private: // Node typechecking rules
  /* Basic type expressions (basic.cpp) */
  void visit(NoneExpr *) override;
  void visit(BoolExpr *) override;
  void visit(IntExpr *) override;
  Expr *transformInt(IntExpr *);
  void visit(FloatExpr *) override;
  Expr *transformFloat(FloatExpr *);
  void visit(StringExpr *) override;

  /* Identifier access expressions (access.cpp) */
  void visit(IdExpr *) override;
  bool checkCapture(const TypeContext::Item &);
  void visit(DotExpr *) override;
  std::pair<size_t, TypeContext::Item> getImport(const std::vector<std::string> &);
  Expr *getClassMember(DotExpr *);
  types::TypePtr findSpecialMember(const std::string &);
  types::FuncTypePtr getBestOverload(Expr *);
  types::FuncTypePtr getDispatch(const std::string &);

  /* Collection and comprehension expressions (collections.cpp) */
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  void visit(DictExpr *) override;
  Expr *transformComprehension(const std::string &, const std::string &,
                               std::vector<Expr *> &);
  void visit(GeneratorExpr *) override;

  /* Conditional expression and statements (cond.cpp) */
  void visit(RangeExpr *) override;
  void visit(IfExpr *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  Stmt *transformPattern(Expr *, Expr *, Stmt *);

  /* Operators (op.cpp) */
  void visit(UnaryExpr *) override;
  Expr *evaluateStaticUnary(UnaryExpr *);
  void visit(BinaryExpr *) override;
  Expr *evaluateStaticBinary(BinaryExpr *);
  Expr *transformBinarySimple(BinaryExpr *);
  Expr *transformBinaryIs(BinaryExpr *);
  std::pair<std::string, std::string> getMagic(const std::string &);
  Expr *transformBinaryInplaceMagic(BinaryExpr *, bool);
  Expr *transformBinaryMagic(BinaryExpr *);
  void visit(ChainBinaryExpr *) override;
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;
  std::pair<bool, Expr *> transformStaticTupleIndex(const types::ClassTypePtr &, Expr *,
                                                    Expr *);
  int64_t translateIndex(int64_t, int64_t, bool = false);
  int64_t sliceAdjustIndices(int64_t, int64_t *, int64_t *, int64_t);
  void visit(InstantiateExpr *) override;
  void visit(SliceExpr *) override;

  /* Calls (call.cpp) */
  void visit(PrintStmt *) override;
  /// Holds partial call information for a CallExpr.
  struct PartialCallData {
    bool isPartial = false;                  // true if the call is partial
    std::string var;                         // set if calling a partial type itself
    std::vector<char> known = {};            // mask of known arguments
    Expr *args = nullptr, *kwArgs = nullptr; // partial *args/**kwargs expressions
  };
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *) override;
  void visit(EllipsisExpr *) override;
  void visit(CallExpr *) override;
  bool transformCallArgs(CallExpr *);
  std::pair<types::FuncTypePtr, Expr *> getCalleeFn(CallExpr *, PartialCallData &);
  Expr *callReorderArguments(types::FuncTypePtr, CallExpr *, PartialCallData &);
  bool typecheckCallArgs(const types::FuncTypePtr &, std::vector<CallArg> &);
  std::pair<bool, Expr *> transformSpecialCall(CallExpr *);
  Expr *transformNamedTuple(CallExpr *);
  Expr *transformFunctoolsPartial(CallExpr *);
  Expr *transformSuperF(CallExpr *);
  Expr *transformSuper();
  Expr *transformPtr(CallExpr *);
  Expr *transformArray(CallExpr *);
  Expr *transformIsInstance(CallExpr *);
  Expr *transformStaticLen(CallExpr *);
  Expr *transformHasAttr(CallExpr *);
  Expr *transformGetAttr(CallExpr *);
  Expr *transformSetAttr(CallExpr *);
  Expr *transformCompileError(CallExpr *);
  Expr *transformTupleFn(CallExpr *);
  Expr *transformTypeFn(CallExpr *);
  Expr *transformRealizedFn(CallExpr *);
  Expr *transformStaticPrintFn(CallExpr *);
  Expr *transformHasRttiFn(CallExpr *);
  std::pair<bool, Expr *> transformInternalStaticFn(CallExpr *);
  std::vector<types::ClassTypePtr> getSuperTypes(const types::ClassTypePtr &);
  void addFunctionGenerics(const types::FuncType *t, bool = false);

  /* Assignments (assign.cpp) */
  void visit(AssignExpr *) override;
  void visit(AssignStmt *) override;
  Stmt *transformUpdate(AssignStmt *);
  Stmt *transformAssignment(AssignStmt *, bool = false);
  void unpackAssignments(Expr *, Expr *, std::vector<Stmt *> &);
  void visit(DelStmt *) override;
  void visit(AssignMemberStmt *) override;
  std::pair<bool, Expr *> transformInplaceUpdate(AssignStmt *);

  /* Imports (import.cpp) */
  void visit(ImportStmt *) override;
  Stmt *transformSpecialImport(ImportStmt *);
  std::vector<std::string> getImportPath(Expr *, size_t = 0);
  Stmt *transformCImport(const std::string &, const std::vector<Param> &, Expr *,
                         const std::string &);
  Stmt *transformCVarImport(const std::string &, Expr *, const std::string &);
  Stmt *transformCDLLImport(Expr *, const std::string &, const std::vector<Param> &,
                            Expr *, const std::string &, bool);
  Stmt *transformPythonImport(Expr *, const std::vector<Param> &, Expr *,
                              const std::string &);
  Stmt *transformNewImport(const ImportFile &);

  /* Loops (loops.cpp) */
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  Expr *transformForDecorator(Expr *);
  Stmt *transformHeterogenousTupleFor(ForStmt *);
  std::pair<bool, Stmt *> transformStaticForLoop(ForStmt *);

  /* Errors and exceptions (error.cpp) */
  void visit(AssertStmt *) override;
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(WithStmt *) override;

  /* Functions (function.cpp) */
  void visit(YieldExpr *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(YieldFromStmt *) override;
  void visit(LambdaExpr *) override;
  void visit(GlobalStmt *) override;
  void visit(FunctionStmt *) override;
  Stmt *transformPythonDefinition(const std::string &, const std::vector<Param> &,
                                  Expr *, Stmt *);
  Stmt *transformLLVMDefinition(Stmt *);
  std::pair<bool, std::string> getDecorator(Expr *);
  Expr *partializeFunction(const types::FuncTypePtr &);
  std::shared_ptr<types::ClassType> getFuncTypeBase(size_t);

private:
  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  std::vector<types::ClassTypePtr> parseBaseClasses(std::vector<Expr *> &,
                                                    std::vector<Param> &, Stmt *,
                                                    const std::string &, Expr *,
                                                    types::ClassTypePtr &);
  std::pair<Stmt *, FunctionStmt *> autoDeduceMembers(ClassStmt *,
                                                      std::vector<Param> &);
  std::vector<Stmt *> getClassMethods(Stmt *s);
  void transformNestedClasses(ClassStmt *, std::vector<Stmt *> &, std::vector<Stmt *> &,
                              std::vector<Stmt *> &);
  Stmt *codegenMagic(const std::string &, Expr *, const std::vector<Param> &, bool);
  types::ClassTypePtr generateTuple(size_t n, bool = true);
  int generateKwId(const std::vector<std::string> & = {});
  void addClassGenerics(const types::ClassTypePtr &, bool instantiate = false);

  /* The rest (typecheck.cpp) */
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  void visit(StmtExpr *) override;
  void visit(CommentStmt *stmt) override;
  void visit(CustomStmt *) override;

public:
  /* Type inference (infer.cpp) */
  types::TypePtr unify(const types::TypePtr &a, const types::TypePtr &b);
  types::TypePtr realize(types::TypePtr);

private:
  Stmt *inferTypes(Stmt *, bool isToplevel = false);
  types::TypePtr realizeFunc(types::FuncType *, bool = false);
  types::TypePtr realizeType(types::ClassType *);
  FunctionStmt *generateSpecialAst(types::FuncType *);
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
                                    const std::vector<Expr *> &args);
  types::FuncTypePtr
  findBestMethod(const types::ClassTypePtr &typ, const std::string &member,
                 const std::vector<std::pair<std::string, types::TypePtr>> &args);
  int canCall(const types::FuncTypePtr &, const std::vector<CallArg> &,
              const types::ClassTypePtr & = nullptr);
  std::vector<types::FuncTypePtr>
  findMatchingMethods(const types::ClassTypePtr &typ,
                      const std::vector<types::FuncTypePtr> &methods,
                      const std::vector<CallArg> &args,
                      const types::ClassTypePtr &part = nullptr);
  Expr *castToSuperClass(Expr *expr, types::ClassTypePtr superTyp, bool = false);
  Stmt *prepareVTables();
  std::vector<std::pair<std::string, Expr *>> extractNamedTuple(Expr *);
  std::vector<types::TypePtr> getClassFieldTypes(const types::ClassTypePtr &);
  std::vector<std::pair<size_t, Expr *>> findEllipsis(Expr *);

public:
  bool wrapExpr(Expr **expr, const types::TypePtr &expectedType,
                const types::FuncTypePtr &callee = nullptr, bool allowUnwrap = true);
  std::vector<Cache::Class::ClassField> getClassFields(types::ClassType *);
  std::shared_ptr<TypeContext> getCtx() const { return ctx; }
  Expr *generatePartialCall(const std::vector<char> &, types::FuncType *,
                            Expr * = nullptr, Expr * = nullptr);

  types::TypePtr getType(Expr *);

  friend class Cache;
  friend class TypeContext;
  friend class types::CallableTrait;
  friend class types::UnionType;

private: // Helpers
  std::shared_ptr<std::vector<std::pair<std::string, types::TypePtr>>>
  unpackTupleTypes(Expr *);
  std::tuple<bool, bool, Stmt *, std::vector<ASTNode *>>
  transformStaticLoopCall(Expr *, SuiteStmt **, Expr *,
                          const std::function<ASTNode *(Stmt *)> &, bool = false);

public:
  template <typename Tn, typename... Ts> Tn *N(Ts &&...args) {
    Tn *t = ctx->cache->N<Tn>(std::forward<Ts>(args)...);
    t->setSrcInfo(getSrcInfo());
    return t;
  }
  template <typename Tn, typename... Ts> Tn *NC(Ts &&...args) {
    Tn *t = ctx->cache->N<Tn>(std::forward<Ts>(args)...);
    return t;
  }
};

} // namespace codon::ast
