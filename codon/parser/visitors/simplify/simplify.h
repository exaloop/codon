#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/ctx.h"
#include "codon/parser/visitors/visitor.h"

namespace codon::ast {

/**
 * Visitor that implements the initial AST simplification transformation.
 * In this stage. the following steps are done:
 *  - All imports are flattened resulting in a single self-containing
 *    (and fairly large) AST
 *  - All identifiers are normalized (no two distinct objects share the same name)
 *  - Variadic classes (e.g., Tuple.N) are generated
 *  - Any AST node that can be trivially expressed as a set of "simpler" nodes
 *    type is simplified. If a transformation requires a type information,
 *    it is done during the type checking.
 *
 * -> Note: this stage *modifies* the provided AST. Clone it before simplification
 *    if you need it intact.
 */
class SimplifyVisitor : public CallbackASTVisitor<ExprPtr, StmtPtr> {
  /// Shared simplification context.
  std::shared_ptr<SimplifyContext> ctx;
  /// Preamble contains definition statements shared across all visitors
  /// in all modules. It is executed before simplified statements.
  std::shared_ptr<std::vector<StmtPtr>> preamble;
  /// Statements to prepend before the current statement.
  std::shared_ptr<std::vector<StmtPtr>> prependStmts;

  /// Each new expression is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  ExprPtr resultExpr;
  /// Each new statement is stored here (as @c visit does not return anything) and
  /// later returned by a @c transform call.
  StmtPtr resultStmt;

public:
  static StmtPtr apply(Cache *cache, const StmtPtr &node, const std::string &file,
                       const std::unordered_map<std::string, std::string> &defines,
                       bool barebones = false);
  static StmtPtr apply(std::shared_ptr<SimplifyContext> cache, const StmtPtr &node,
                       const std::string &file, int atAge = -1);

public:
  explicit SimplifyVisitor(std::shared_ptr<SimplifyContext> ctx,
                           std::shared_ptr<std::vector<StmtPtr>> preamble,
                           std::shared_ptr<std::vector<StmtPtr>> stmts = nullptr);

public:
  ExprPtr transform(ExprPtr &expr) override;
  ExprPtr transform(const ExprPtr &expr) override {
    auto e = expr;
    return transform(e);
  }
  ExprPtr transform(ExprPtr &expr, bool allowTypes);
  ExprPtr transform(ExprPtr &&expr, bool allowTypes) {
    return transform(expr, allowTypes);
  }
  ExprPtr transformType(ExprPtr &expr, bool allowTypeOf = true);
  ExprPtr transformType(ExprPtr &&expr, bool allowTypeOf = true) {
    return transformType(expr, allowTypeOf);
  }
  StmtPtr transform(StmtPtr &stmt) override;
  StmtPtr transform(const StmtPtr &stmt) override {
    auto s = stmt;
    return transform(s);
  }
  StmtPtr transformConditionalScope(StmtPtr &stmt);

public:
  /* Basic type expressions (basic.cpp) */
  void visit(IntExpr *) override;
  ExprPtr transformInt(IntExpr *);
  void visit(FloatExpr *) override;
  ExprPtr transformFloat(FloatExpr *);
  void visit(StringExpr *) override;
  ExprPtr transformFString(const std::string &);

  /* Identifier access expressions (access.cpp) */
  void visit(IdExpr *) override;
  bool checkCapture(const SimplifyContext::Item &);
  void visit(DotExpr *) override;
  std::pair<size_t, SimplifyContext::Item> getImport(const std::vector<std::string> &);

  /* Collection and comprehension expressions (collections.cpp) */
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  ExprPtr transformComprehension(const std::string &, const std::string &,
                                 const std::vector<ExprPtr> &);
  void visit(DictExpr *) override;
  void visit(GeneratorExpr *) override;
  void visit(DictGeneratorExpr *) override;
  StmtPtr transformGeneratorBody(const std::vector<GeneratorBody> &, SuiteStmt *&);

  /* Conditional expression and statements (cond.cpp) */
  void visit(IfExpr *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  StmtPtr transformPattern(ExprPtr, ExprPtr, StmtPtr);

  /* Operators (op.cpp) */
  void visit(UnaryExpr *) override;
  void visit(BinaryExpr *) override;
  void visit(ChainBinaryExpr *) override;
  void visit(IndexExpr *) override;
  void visit(InstantiateExpr *) override;

  /* Calls (call.cpp) */
  void visit(PrintStmt *) override;
  void visit(CallExpr *) override;
  ExprPtr transformSpecialCall(ExprPtr, const std::vector<CallExpr::Arg> &);
  ExprPtr transformTupleGenerator(const std::vector<CallExpr::Arg> &);
  ExprPtr transformNamedTuple(const std::vector<CallExpr::Arg> &);
  ExprPtr transformFunctoolsPartial(std::vector<CallExpr::Arg>);

  /* Assignments (assign.cpp) */
  void visit(AssignExpr *) override;
  void visit(AssignStmt *) override;
  StmtPtr transformAssignment(ExprPtr, ExprPtr, ExprPtr = nullptr, bool = false);
  void unpackAssignments(ExprPtr, ExprPtr, std::vector<StmtPtr> &);
  void visit(DelStmt *) override;

  /* Imports (import.cpp) */
  void visit(ImportStmt *) override;
  StmtPtr transformSpecialImport(ImportStmt *);
  std::vector<std::string> getImportPath(Expr *, size_t = 0);
  StmtPtr transformCImport(const std::string &, const std::vector<Param> &,
                           const Expr *, const std::string &);
  StmtPtr transformCDLLImport(const Expr *, const std::string &,
                              const std::vector<Param> &, const Expr *,
                              const std::string &);
  StmtPtr transformPythonImport(Expr *, const std::vector<Param> &, Expr *,
                                const std::string &);
  StmtPtr transformNewImport(const ImportFile &);

  /* Loops (loops.cpp) */
  void visit(ContinueStmt *) override;
  void visit(BreakStmt *) override;
  void visit(WhileStmt *) override;
  void visit(ForStmt *) override;
  ExprPtr transformForDecorator(ExprPtr);

  /* Errors and exceptions (error.cpp) */
  void visit(AssertStmt *) override;
  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;
  void visit(WithStmt *) override;

  /* Functions (function.cpp) */
  void visit(YieldExpr *) override;
  void visit(LambdaExpr *) override;
  void visit(GlobalStmt *) override;
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(YieldFromStmt *) override;
  void visit(FunctionStmt *) override;
  ExprPtr makeAnonFn(std::vector<StmtPtr>, const std::vector<std::string> & = {});
  StmtPtr transformPythonDefinition(const std::string &, const std::vector<Param> &,
                                    const Expr *, Stmt *);
  StmtPtr transformLLVMDefinition(Stmt *);
  std::string *isAttribute(ExprPtr);

  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  Attr parseClassDecorators(Attr attr, const std::vector<ExprPtr> &decorators);
  std::vector<ClassStmt *> parseBaseClasses(const std::vector<ExprPtr> &baseClasses,
                                            std::vector<Param> &hiddenGenerics,
                                            const Attr &attr);
  std::pair<StmtPtr, FunctionStmt *> autoDeduceMembers(ClassStmt *stmt,
                                                       std::vector<Param> &args);
  std::vector<StmtPtr> getClassMethods(const StmtPtr &s);
  void transformNestedClasses(ClassStmt *stmt, std::vector<StmtPtr> &clsStmts,
                              std::vector<StmtPtr> &fnStmts);
  StmtPtr codegenMagic(const std::string &op, const Expr *typExpr,
                       const std::vector<Param> &args, bool isRecord);

  /* The rest (simplify.cpp) */
  void visit(StmtExpr *) override;
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *expr) override;
  void visit(RangeExpr *) override;
  void visit(SliceExpr *) override;
  void visit(EllipsisExpr *) override;
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  void visit(CustomStmt *) override;
};

} // namespace codon::ast
