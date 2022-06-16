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
 *  - All imports are flattened making the resulting AST a self-containing (but fairly
 *    large) AST.
 *  - All identifiers are normalized (no two distinct objects share the same name).
 *  - Variadic classes (Tuple.N and Function.N) are generated.
 *  - Any AST node that can be trivially represented as a set of "simpler" nodes
 *    type is transformed accordingly. If a transformation requires a type information,
 *    it is delayed until the next transformation stage (type-checking).
 *
 * ➡️ Note: This visitor *copies* the incoming AST and does not modify it.
 */
class SimplifyVisitor : public CallbackASTVisitor<ExprPtr, StmtPtr> {
public:
  /// Simplification step will divide the input AST into four sub-ASTs that are stored
  /// here:
  ///   - Type (class) signatures
  ///   - Global variable signatures (w/o rhs)
  ///   - Functions
  ///   - Top-level statements.
  /// Each of these divisions will be populated via first-come first-serve method.
  /// This way, type and global signatures will be exposed to all executable statements,
  /// and we can assume that there won't be any back-references (any signatures depends
  /// only on the previously seen signatures). We also won't have to maintain complex
  /// structures to access global variables, or worry about recursive imports.
  /// This approach also allows us to generate global types without having to
  /// worry about initialization order.
  struct Preamble {
    std::vector<StmtPtr> globals;
    std::vector<StmtPtr> functions;
  };
  std::shared_ptr<std::vector<StmtPtr>> prependStmts;

private:
  /// Shared simplification context.
  std::shared_ptr<SimplifyContext> ctx;

  /// Preamble contains shared definition statements and is shared across all visitors
  /// (in all modules). See Preamble (type) for more details.
  std::shared_ptr<Preamble> preamble;

  /// Each new expression is stored here (as visit() does not return anything) and
  /// later returned by a transform() call.
  ExprPtr resultExpr;
  /// Each new statement is stored here (as visit() does not return anything) and
  /// later returned by a transform() call.
  StmtPtr resultStmt;

public:
  /// Static method that applies SimplifyStage on a given AST node.
  /// Loads standard library if needed.
  /// @param cache Pointer to the shared transformation cache.
  /// @param file Filename of a AST node.
  /// @param barebones Set if a bare-bones standard library is used during testing.
  /// @param defines
  ///        User-defined static values (typically passed via seqc -DX=Y).
  ///        Each value is passed as a string (integer part is ignored).
  ///        The method will replace this map with a map that links canonical names
  ///        to their string and integer values.
  static StmtPtr apply(Cache *cache, const StmtPtr &node, const std::string &file,
                       const std::unordered_map<std::string, std::string> &defines,
                       bool barebones = false);

  /// Static method that applies SimplifyStage on a given AST node after the standard
  /// library was loaded.
  static StmtPtr apply(std::shared_ptr<SimplifyContext> cache, const StmtPtr &node,
                       const std::string &file, int atAge = -1);

public:
  explicit SimplifyVisitor(std::shared_ptr<SimplifyContext> ctx,
                           std::shared_ptr<Preamble> preamble,
                           std::shared_ptr<std::vector<StmtPtr>> stmts = nullptr);

  /// Transform an AST expression node.
  /// @raise ParserException if a node describes a type (use transformType instead).
  ExprPtr transform(const ExprPtr &expr) override;
  /// Transform an AST statement node.
  StmtPtr transform(const StmtPtr &stmt) override;
  StmtPtr transformInScope(const StmtPtr &stmt);
  /// Transform an AST expression node.
  ExprPtr transform(const ExprPtr &expr, bool allowTypes);
  /// Transform an AST type expression node.
  /// @raise ParserException if a node does not describe a type (use transform instead).
  ExprPtr transformType(const ExprPtr &expr, bool allowTypeOf = true);

private:
  /// These functions just clone a given node (nothing to be simplified).
  void defaultVisit(Expr *e) override;
  void defaultVisit(Stmt *s) override;

public:
  /* Basic type expressions (basic.cpp) */
  void visit(NoneExpr *) override;
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
  std::pair<size_t, SimplifyContext::Item> getImport(const std::deque<std::string> &);

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
  void visit(PipeExpr *) override;
  void visit(IndexExpr *) override;

  /* Calls (call.cpp) */
  void visit(PrintStmt *) override;
  void visit(CallExpr *) override;
  ExprPtr transformSpecialCall(ExprPtr, const std::vector<CallExpr::Arg> &);
  ExprPtr transformTupleGenerator(const std::vector<CallExpr::Arg> &);
  ExprPtr transformNamedTuple(const std::vector<CallExpr::Arg> &);
  ExprPtr transformFunctoolsPartial(const std::vector<CallExpr::Arg> &);

  /* Assignments (assign.cpp) */
  void visit(AssignStmt *) override;
  StmtPtr transformAssignment(const ExprPtr &, const ExprPtr &,
                              const ExprPtr & = nullptr, bool = false);
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
  StmtPtr transformPythonImport(Expr *, const std::vector<Param> &, const Expr *,
                                const std::string &);
  void transformNewImport(const ImportFile &);

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
  /// Transforms type definitions and extensions.
  /// This currently consists of adding default magic methods (described in
  /// codegenMagic() method below).
  void visit(ClassStmt *) override;
  Attr parseClassDecorators(Attr attr, const std::vector<ExprPtr> &decorators);
  void parseBaseClasses(const std::vector<ExprPtr> &baseClasses,
                        std::vector<ClassStmt *> &baseASTs,
                        std::vector<Param> &hiddenGenerics, const Attr &attr);
  std::pair<StmtPtr, FunctionStmt *> autoDeduceMembers(ClassStmt *stmt,
                                                       std::vector<Param> &args);
  void transformNestedClasses(ClassStmt *stmt, std::vector<StmtPtr> &clsStmts,
                              std::vector<StmtPtr> &fnStmts);
  /// Generate a magic method __op__ for a type described by typExpr and type arguments
  /// args.
  /// Currently able to generate:
  ///   Constructors: __new__, __init__
  ///   Utilities: __raw__, __hash__, __repr__
  ///   Iteration: __iter__, __getitem__, __len__, __contains__
  //    Comparisons: __eq__, __ne__, __lt__, __le__, __gt__, __ge__
  //    Pickling: __pickle__, __unpickle__
  //    Python: __to_py__, __from_py__
  StmtPtr codegenMagic(const std::string &op, const Expr *typExpr,
                       const std::vector<Param> &args, bool isRecord);

////////

  // This expression is transformed during the type-checking stage
  // because we need raw SliceExpr to handle static tuple slicing.
  void visit(SliceExpr *) override;
  /// Disallow ellipsis except in a call expression.
  void visit(EllipsisExpr *) override;

  /// Transform var := expr to a statement expression:
  ///   var = expr; var
  /// Disallowed in dependent parts of short-circuiting expressions
  /// (i.e. b and b2 in "a and b", "a or b" or "b if cond else b2").
  void visit(AssignExpr *) override;
  /// Disallow ranges except in match statements.
  void visit(RangeExpr *) override;
  /// Parse all statements in StmtExpr.
  void visit(StmtExpr *) override;
  /// Transform a star-expression *args to:
  ///   List(args.__iter__()).
  /// This function is called only if other nodes (CallExpr, AssignStmt, ListExpr) do
  /// not handle their star-expression cases.
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *expr) override;

  /// Transform all statements in a suite and flatten them (unless a suite is a variable
  /// scope).
  void visit(SuiteStmt *) override;

  void visit(ExprStmt *) override;



  void visit(CustomStmt *) override;

  using CallbackASTVisitor<ExprPtr, StmtPtr>::transform;

private:
  // Return a list of all function statements within a given class suite. Checks each
  // suite recursively, and assumes that each statement is either a function or a
  // doc-string.
  std::vector<StmtPtr> getClassMethods(const StmtPtr &s);
};

struct AssignReplacementVisitor : ReplaceASTVisitor {
  Cache *cache;
  std::map<std::string, std::pair<std::string, bool>> &replacements;
  AssignReplacementVisitor(Cache *c,
                           std::map<std::string, std::pair<std::string, bool>> &r);
  void visit(IdExpr *expr) override;
  void transform(ExprPtr &e) override;
  void visit(TryStmt *stmt) override;
  void visit(ForStmt *stmt) override;
  void transform(StmtPtr &e) override;
};
} // namespace codon::ast
