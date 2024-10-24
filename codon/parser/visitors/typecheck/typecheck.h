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
  types::FuncType *getDispatch(const std::string &);

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
  std::pair<bool, Expr *> transformStaticTupleIndex(types::ClassType *, Expr *, Expr *);
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
  std::pair<std::shared_ptr<types::FuncType>, Expr *> getCalleeFn(CallExpr *,
                                                                  PartialCallData &);
  Expr *callReorderArguments(types::FuncType *, CallExpr *, PartialCallData &);
  bool typecheckCallArgs(types::FuncType *, std::vector<CallArg> &);
  std::pair<bool, Expr *> transformSpecialCall(CallExpr *);
  std::vector<types::TypePtr> getSuperTypes(types::ClassType *);

  /* Assignments (assign.cpp) */
  void visit(AssignExpr *) override;
  void visit(AssignStmt *) override;
  Stmt *transformUpdate(AssignStmt *);
  Stmt *transformAssignment(AssignStmt *, bool = false);
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
  Expr *partializeFunction(types::FuncType *);
  std::shared_ptr<types::ClassType> getFuncTypeBase(size_t);

private:
  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  std::vector<types::TypePtr> parseBaseClasses(std::vector<Expr *> &,
                                               std::vector<Param> &, Stmt *,
                                               const std::string &, Expr *,
                                               types::ClassType *);
  void autoDeduceMembers(ClassStmt *, std::vector<Param> &);
  std::vector<Stmt *> getClassMethods(Stmt *s);
  void transformNestedClasses(ClassStmt *, std::vector<Stmt *> &, std::vector<Stmt *> &,
                              std::vector<Stmt *> &);
  Stmt *codegenMagic(const std::string &, Expr *, const std::vector<Param> &, bool);
  int generateKwId(const std::vector<std::string> & = {});

public:
  types::ClassType *generateTuple(size_t n, bool = true);

private:
  /* The rest (typecheck.cpp) */
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  void visit(StmtExpr *) override;
  void visit(CommentStmt *stmt) override;
  void visit(CustomStmt *) override;

public:
  /* Type inference (infer.cpp) */
  types::Type *unify(types::Type *a, types::Type *b);
  types::Type *unify(types::Type *a, types::TypePtr &&b) { return unify(a, b.get()); }
  types::Type *realize(types::Type *);
  types::TypePtr &&realize(types::TypePtr &&t) {
    realize(t.get());
    return std::move(t);
  }

private:
  Stmt *inferTypes(Stmt *, bool isToplevel = false);
  types::Type *realizeFunc(types::FuncType *, bool = false);
  types::Type *realizeType(types::ClassType *);
  SuiteStmt *generateSpecialAst(types::FuncType *);
  size_t getRealizationID(types::ClassType *, types::FuncType *);
  codon::ir::types::Type *makeIRType(types::ClassType *);
  codon::ir::Func *
  makeIRFunction(const std::shared_ptr<Cache::Function::FunctionRealization> &);

private:
  types::FuncType *findBestMethod(types::ClassType *typ, const std::string &member,
                                  const std::vector<types::Type *> &args);
  types::FuncType *findBestMethod(types::ClassType *typ, const std::string &member,
                                  const std::vector<Expr *> &args);
  types::FuncType *
  findBestMethod(types::ClassType *typ, const std::string &member,
                 const std::vector<std::pair<std::string, types::Type *>> &args);
  int canCall(types::FuncType *, const std::vector<CallArg> &,
              types::ClassType * = nullptr);
  std::vector<types::FuncType *> findMatchingMethods(
      types::ClassType *typ, const std::vector<types::FuncType *> &methods,
      const std::vector<CallArg> &args, types::ClassType *part = nullptr);
  Expr *castToSuperClass(Expr *expr, types::ClassType *superTyp, bool = false);
  void prepareVTables();
  std::vector<std::pair<std::string, Expr *>> extractNamedTuple(Expr *);
  std::vector<types::TypePtr> getClassFieldTypes(types::ClassType *);
  std::vector<std::pair<size_t, Expr *>> findEllipsis(Expr *);

public:
  bool wrapExpr(Expr **expr, types::Type *expectedType,
                types::FuncType *callee = nullptr, bool allowUnwrap = true);
  std::tuple<bool, types::TypePtr, std::function<Expr *(Expr *)>>
  canWrapExpr(types::Type *exprType, types::Type *expectedType,
            types::FuncType *callee = nullptr, bool allowUnwrap = true,
            bool isEllipsis = false);
  std::vector<Cache::Class::ClassField> getClassFields(types::ClassType *) const;
  std::shared_ptr<TypeContext> getCtx() const { return ctx; }
  Expr *generatePartialCall(const std::vector<char> &, types::FuncType *,
                            Expr * = nullptr, Expr * = nullptr);

  friend class Cache;
  friend class TypeContext;
  friend class types::CallableTrait;
  friend class types::UnionType;

private: // Helpers
  std::shared_ptr<std::vector<std::pair<std::string, types::Type *>>>
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

private:
  template <typename... Ts> void log(const std::string &prefix, Ts &&...args) {
    fmt::print(codon::getLogger().log, "[{}] [{}${}]: " + prefix + "\n",
               ctx->getSrcInfo(), ctx->getBaseName(), ctx->getBase()->iteration,
               std::forward<Ts>(args)...);
  }

public:
  types::Type *extractType(types::Type *t);
  types::Type *extractType(Expr *e);
  types::Type *extractType(const std::string &);
  types::ClassType *extractClassType(Expr *e);
  types::ClassType *extractClassType(types::Type *t);
  types::ClassType *extractClassType(const std::string &s);
  bool isUnbound(types::Type *t) const;
  bool isUnbound(Expr *e) const;
  bool hasOverloads(const std::string &root);
  std::vector<std::string> getOverloads(const std::string &root);
  std::string getUnmangledName(const std::string &s) const;
  Cache::Class *getClass(const std::string &t) const;
  Cache::Class *getClass(types::Type *t) const;
  Cache::Function *getFunction(const std::string &n) const;
  Cache::Function *getFunction(types::Type *t) const;
  Cache::Class::ClassRealization *getClassRealization(types::Type *t) const;
  std::string getRootName(types::FuncType *t);
  bool isTypeExpr(Expr *e);
  Cache::Module *getImport(const std::string &s);
  std::string getArgv() const;
  std::string getRootModulePath() const;
  std::vector<std::string> getPluginImportPaths() const;
  bool isDispatch(const std::string &s);
  bool isDispatch(FunctionStmt *ast);
  bool isDispatch(types::Type *f);
  void addClassGenerics(types::ClassType *typ, bool func = false,
                        bool onlyMangled = false, bool instantiate = false);
  template <typename F>
  auto withClassGenerics(types::ClassType *typ, F fn, bool func = false,
                         bool onlyMangled = false, bool instantiate = false) {
    ctx->addBlock();
    addClassGenerics(typ, func, onlyMangled, instantiate);
    auto t = fn();
    ctx->popBlock();
    return t;
  }
  types::TypePtr instantiateTypeVar(types::Type *t);
  void registerGlobal(const std::string &s, bool = false);
  types::ClassType *getStdLibType(const std::string &type);
  types::Type *extractClassGeneric(types::Type *t, int idx = 0);
  types::Type *extractFuncGeneric(types::Type *t, int idx = 0);
  types::Type *extractFuncArgType(types::Type *t, int idx = 0);
  std::string getClassMethod(types::Type *typ, const std::string &member);
  std::string getTemporaryVar(const std::string &s);
  bool isImportFn(const std::string &s);

  int64_t getIntLiteral(types::Type *t, size_t pos = 0);
  bool getBoolLiteral(types::Type *t, size_t pos = 0);
  std::string getStrLiteral(types::Type *t, size_t pos = 0);

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
  Expr *transformStaticFnCanCall(CallExpr *);
  Expr *transformStaticFnArgHasType(CallExpr *);
  Expr *transformStaticFnArgGetType(CallExpr *);
  Expr *transformStaticFnArgs(CallExpr *);
  Expr *transformStaticFnHasDefault(CallExpr *);
  Expr *transformStaticFnGetDefault(CallExpr *);
  Expr *transformStaticFnWrapCallArgs(CallExpr *);
  Expr *transformStaticVars(CallExpr *);
  Expr *transformStaticTupleType(CallExpr *);
  SuiteStmt *generateClassPopulateVTablesAST();
  SuiteStmt *generateBaseDerivedDistAST(types::FuncType *);
  FunctionStmt *generateThunkAST(types::FuncType *fp, types::ClassType *base,
                                 types::ClassType *derived);
  SuiteStmt *generateFunctionCallInternalAST(types::FuncType *);
  SuiteStmt *generateUnionNewAST(types::FuncType *);
  SuiteStmt *generateUnionTagAST(types::FuncType *);
  SuiteStmt *generateNamedKeysAST(types::FuncType *);
  SuiteStmt *generateTupleMulAST(types::FuncType *);
  std::vector<Stmt *> populateStaticTupleLoop(Expr *, const std::vector<std::string> &);
  std::vector<Stmt *> populateSimpleStaticRangeLoop(Expr *,
                                                    const std::vector<std::string> &);
  std::vector<Stmt *> populateStaticRangeLoop(Expr *, const std::vector<std::string> &);
  std::vector<Stmt *> populateStaticFnOverloadsLoop(Expr *,
                                                    const std::vector<std::string> &);
  std::vector<Stmt *> populateStaticEnumerateLoop(Expr *,
                                                  const std::vector<std::string> &);
  std::vector<Stmt *> populateStaticVarsLoop(Expr *, const std::vector<std::string> &);
  std::vector<Stmt *> populateStaticVarTypesLoop(Expr *,
                                                 const std::vector<std::string> &);
  std::vector<Stmt *>
  populateStaticHeterogenousTupleLoop(Expr *, const std::vector<std::string> &);

public:
public:
  /// Get the current realization depth (i.e., the number of nested realizations).
  size_t getRealizationDepth() const;
  /// Get the name of the current realization stack (e.g., `fn1:fn2:...`).
  std::string getRealizationStackName() const;

public:
  /// Create an unbound type with the provided typechecking level.
  std::shared_ptr<types::LinkType> instantiateUnbound(const SrcInfo &info,
                                                      int level) const;
  std::shared_ptr<types::LinkType> instantiateUnbound(const SrcInfo &info) const;
  std::shared_ptr<types::LinkType> instantiateUnbound() const;

  /// Call `type->instantiate`.
  /// Prepare the generic instantiation table with the given generics parameter.
  /// Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
  ///          T=int.
  /// @param expr Expression that needs the type. Used to set type's srcInfo.
  /// @param setActive If True, add unbounds to activeUnbounds.
  types::TypePtr instantiateType(const SrcInfo &info, types::Type *type,
                                 types::ClassType *generics = nullptr);
  types::TypePtr instantiateType(const SrcInfo &info, types::Type *root,
                                 const std::vector<types::Type *> &generics);
  template <typename T>
  std::shared_ptr<T> instantiateType(T *type, types::ClassType *generics = nullptr) {
    return std::static_pointer_cast<T>(
        instantiateType(getSrcInfo(), std::move(type), generics));
  }
  template <typename T>
  std::shared_ptr<T> instantiateType(T *root,
                                     const std::vector<types::Type *> &generics) {
    return std::static_pointer_cast<T>(
        instantiateType(getSrcInfo(), std::move(root), generics));
  }
  std::shared_ptr<types::IntStaticType> instantiateStatic(int64_t i) {
    return std::make_shared<types::IntStaticType>(ctx->cache, i);
  }
  std::shared_ptr<types::StrStaticType> instantiateStatic(const std::string &s) {
    return std::make_shared<types::StrStaticType>(ctx->cache, s);
  }
  std::shared_ptr<types::BoolStaticType> instantiateStatic(bool i) {
    return std::make_shared<types::BoolStaticType>(ctx->cache, i);
  }

  /// Returns the list of generic methods that correspond to typeName.method.
  std::vector<types::FuncType *> findMethod(types::ClassType *type,
                                            const std::string &method,
                                            bool hideShadowed = true);
  /// Returns the generic type of typeName.member, if it exists (nullptr otherwise).
  /// Special cases: __elemsize__ and __atomic__.
  Cache::Class::ClassField *findMember(types::ClassType *, const std::string &) const;

  using ReorderDoneFn =
      std::function<int(int, int, const std::vector<std::vector<int>> &, bool)>;
  using ReorderErrorFn = std::function<int(error::Error, const SrcInfo &, std::string)>;
  /// Reorders a given vector or named arguments (consisting of names and the
  /// corresponding types) according to the signature of a given function.
  /// Returns the reordered vector and an associated reordering score (missing
  /// default arguments' score is half of the present arguments).
  /// Score is -1 if the given arguments cannot be reordered.
  /// @param known Bitmask that indicated if an argument is already provided
  ///              (partial function) or not.
  int reorderNamedArgs(types::FuncType *func, const std::vector<CallArg> &args,
                       const ReorderDoneFn &onDone, const ReorderErrorFn &onError,
                       const std::vector<char> &known = std::vector<char>());

  bool isCanonicalName(const std::string &name) const;
  types::FuncType *extractFunction(types::Type *t) const;

  ir::PyType cythonizeClass(const std::string &name);
  ir::PyType cythonizeIterator(const std::string &name);
  ir::PyFunction cythonizeFunction(const std::string &name);
  ir::Func *realizeIRFunc(types::FuncType *fn,
                          const std::vector<types::TypePtr> &generics = {});
  // types::Type *getType(const std::string &);
};

} // namespace codon::ast
