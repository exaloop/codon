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
  /* Basic expressions */
  void visit(NoneExpr *) override;
  void visit(IntExpr *) override;
  ExprPtr transformInt(IntExpr *);
  void visit(FloatExpr *) override;
  ExprPtr transformFloat(FloatExpr *);
  void visit(StringExpr *) override;
  ExprPtr transformFString(const std::string &);

  /* Identifier expressions */
  void visit(IdExpr *) override;
  bool checkCapture(const SimplifyContext::Item &);
  void visit(DotExpr *) override;
  std::pair<size_t, SimplifyContext::Item> getImport(const std::deque<std::string> &);

  /* Collection and comprehension expressions */
  void visit(TupleExpr *) override;
  void visit(ListExpr *) override;
  void visit(SetExpr *) override;
  ExprPtr transformComprehension(const std::string &, const std::string &,
                                 const std::vector<ExprPtr> &);
  void visit(DictExpr *) override;
  void visit(GeneratorExpr *) override;
  void visit(DictGeneratorExpr *) override;
  StmtPtr transformGeneratorBody(const std::vector<GeneratorBody> &, SuiteStmt *&);

  /* Conditional expression and statements */
  void visit(IfExpr *) override;
  void visit(IfStmt *) override;
  void visit(MatchStmt *) override;
  StmtPtr transformPattern(ExprPtr, ExprPtr, StmtPtr);

  /// Transform a unary expression to the corresponding magic call
  /// (__invert__, __pos__ or __neg__).
  /// Special case: not a is transformed to
  ///   a.__bool__().__invert__()
  /// Note: static expressions are not transformed.
  void visit(UnaryExpr *) override;
  /// Transform the following binary expressions:
  ///   a and b -> b.__bool__() if a.__bool__() else False
  ///   a or b -> True if a.__bool__() else b.__bool__()
  ///   a is not b -> (a is b).__invert__()
  ///   a not in b -> not (a in b)
  ///   a in b -> a.__contains__(b)
  ///   None is None -> True
  ///   None is b -> b is None.
  /// Other cases are handled during the type-checking stage.
  /// Note: static expressions are not transformed.
  void visit(BinaryExpr *) override;
  /// Transform chain binary expression:
  ///   a <= b <= c -> (a <= b) and (b <= c).
  /// Ensures that all expressions (a, b, and c) are executed only once.
  void visit(ChainBinaryExpr *) override;
  /// Pipes will be handled during the type-checking stage.
  void visit(PipeExpr *) override;
  /// Perform the following transformations:
  ///   tuple[T1, ... TN],
  ///   Tuple[T1, ... TN] -> Tuple.N(T1, ..., TN)
  ///     (and generate class Tuple.N)
  ///   function[R, T1, ... TN],
  ///   Function[R, T1, ... TN] -> Function.N(R, T1, ..., TN)
  ///     (and generate class Function.N)
  /// Otherwise, if the index expression is a type instantiation, convert it to an
  /// InstantiateExpr. All other cases are handled during the type-checking stage.
  void visit(IndexExpr *) override;

  /* Call expression */

  void visit(CallExpr *) override;
  /// Check for special calls that are handled in simplification stage:
  ///   - tuple(i for i in tup) (tuple generatoris)
  ///   - std.collections.namedtuple (sugar for @tuple class)
  ///   - std.functools.partial (sugar for partial calls)
  ExprPtr transformSpecialCall(ExprPtr callee, const std::vector<CallExpr::Arg> &args);
  ExprPtr transformTupleGenerator(const std::vector<CallExpr::Arg> &args);
  ExprPtr transformNamedTuple(const std::vector<CallExpr::Arg> &args);
  ExprPtr transformFunctoolsPartial(const std::vector<CallExpr::Arg> &args);

  // This expression is transformed during the type-checking stage
  // because we need raw SliceExpr to handle static tuple slicing.
  void visit(SliceExpr *) override;
  /// Disallow ellipsis except in a call expression.
  void visit(EllipsisExpr *) override;
  /// Ensure that a yield is in a function.
  void visit(YieldExpr *) override;
  /// Transform lambda a, b: a+b+c to:
  ///   def _lambda(a, b, c): return a+b+c
  ///   _lambda(..., ..., c)
  void visit(LambdaExpr *) override;
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
  /// Ensure that a continue is in a loop.
  void visit(ContinueStmt *) override;
  /// Ensure that a break is in a loop.
  /// If a loop break variable is available (loop-else block), transform a break to:
  ///   loop_var = false; break
  void visit(BreakStmt *) override;
  void visit(ExprStmt *) override;
  /// Performs assignment and unpacking transformations.
  /// See transformAssignment() and unpackAssignments() for more details.
  void visit(AssignStmt *) override;
  /// Transform del a[x] to:
  ///   del a -> a = typeof(a)() (and removes a from the context)
  ///   del a[x] -> a.__delitem__(x)
  void visit(DelStmt *) override;
  /// Transform print a, b to:
  ///   print(a, b)
  /// Add end=' ' if inPlace is set.
  void visit(PrintStmt *) override;
  /// Ensure that a return is in a function.
  void visit(ReturnStmt *) override;
  /// Ensure that a yield is in a function.
  void visit(YieldStmt *) override;
  /// Transform yield from a to:
  ///   for var in a: yield var
  void visit(YieldFromStmt *) override;
  /// Transform assert foo(), "message" to:
  ///   if not foo(): seq_assert(<file>, <line>, "message")
  /// Transform assert foo() to:
  ///   if not foo(): seq_assert(<file>, <line>, "")
  /// If simplification stage is invoked during unit testing, call seq_assert_test()
  /// instead.
  void visit(AssertStmt *) override;
  /// Transform while cond to:
  ///   while cond.__bool__()
  /// Transform while cond: ... else: ... to:
  ///   no_break = True
  ///   while cond.__bool__(): ...
  ///   if no_break.__bool__(): ...
  void visit(WhileStmt *) override;
  /// Transform for i in it: ... to:
  ///   for i in it: ...
  /// Transform for i, j in it: ... to:
  ///   for tmp in it:
  ///      i, j = tmp; ...
  /// This transformation uses AssignStmt and supports all unpack operations that are
  /// handled there.
  /// Transform for i in it: ... else: ... to:
  ///   no_break = True
  ///   for i in it: ...
  ///   if no_break.__bool__(): ...
  void visit(ForStmt *) override;

  void visit(TryStmt *) override;
  void visit(ThrowStmt *) override;
  /// Transform with foo(), bar() as a: ... to:
  ///   block:
  ///     tmp = foo()
  ///     tmp.__enter__()
  ///     try:
  ///       a = bar()
  ///       a.__enter__()
  ///       try:
  ///         ...
  ///       finally:
  ///         a.__exit__()
  ///     finally:
  ///       tmp.__exit__()
  void visit(WithStmt *) override;
  /// Perform the global checks and remove the statement from the consideration.
  void visit(GlobalStmt *) override;
  /// Import a module into its own context.
  /// Unless we are loading the standard library, each import statement is replaced
  /// with:
  ///   if not _import_N_done:
  ///     _import_N()
  ///     _import_N_done = True
  /// to make sure that the first _executed_ import statement executes its statements
  /// (like Python). See transformNewImport() and below for details.
  ///
  /// This function also handles FFI imports (C, Python etc). For the details, see
  /// transformCImport(), transformCDLLImport() and transformPythonImport().
  void visit(ImportStmt *) override;
  /// Transforms function definitions.
  ///
  /// At this stage, the only meaningful transformation happens for "self" arguments in
  /// a class method that have no type annotation (they will get one automatically).
  ///
  /// For Python and LLVM definition transformations, see transformPythonDefinition()
  /// and transformLLVMDefinition().
  void visit(FunctionStmt *) override;

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

  void visit(CustomStmt *) override;

  using CallbackASTVisitor<ExprPtr, StmtPtr>::transform;

private:
  /// Make an anonymous function _lambda_XX with provided statements and argument names.
  /// Function definition is prepended to the current statement.
  /// If the statements refer to outer variables, those variables will be captured and
  /// added to the list of arguments. Returns a call expression that calls this
  /// function with captured variables.
  /// @li Given a statement a+b and argument names a, this generates
  ///            def _lambda(a, b): return a+b
  ///          and returns
  ///            _lambda(b).
  ExprPtr makeAnonFn(std::vector<StmtPtr> stmts,
                     const std::vector<std::string> &argNames = {});

  /// Transforms a simple assignment:
  ///   a[x] = b -> a.__setitem__(x, b)
  ///   a.x = b -> AssignMemberStmt
  ///   a : type = b -> AssignStmt
  ///   a = b -> AssignStmt or UpdateStmt if a exists in the same scope (or is global)
  StmtPtr transformAssignment(const ExprPtr &lhs, const ExprPtr &rhs,
                              const ExprPtr &type, bool mustExist);
  /// Unpack an assignment expression lhs = rhs into a list of simple assignment
  /// expressions (either a = b, or a.x = b, or a[x] = b).
  /// Used to handle various Python unpacking rules, such as:
  ///   (a, b) = c
  ///   a, b = c
  ///   [a, *x, b] = c.
  /// Non-trivial right-hand expressions are first stored in a temporary variable:
  ///   a, b = c, d + foo() -> tmp = (c, d + foo); a = tmp[0]; b = tmp[1].
  /// Processes each assignment recursively to support cases like:
  ///   a, (b, c) = d
  void unpackAssignments(ExprPtr lhs, ExprPtr rhs, std::vector<StmtPtr> &stmts,
                         bool mustExist);

  /// Transform a C import (from C import foo(int) -> float as f) to:
  ///   @.c
  ///   def foo(a1: int) -> float: pass
  ///   f = foo (only if altName is provided).
  StmtPtr transformCImport(const std::string &name, const std::vector<Param> &args,
                           const Expr *ret, const std::string &altName);
  /// Transform a dynamic C import (from C import lib.foo(int) -> float as f) to:
  ///   def foo(a1: int) -> float:
  ///     fptr = _dlsym(lib, "foo")
  ///     f = Function[float, int](fptr)
  ///     return f(a1)  (if return type is void, just call f(a1))
  StmtPtr transformCDLLImport(const Expr *dylib, const std::string &name,
                              const std::vector<Param> &args, const Expr *ret,
                              const std::string &altName);
  /// Transform a Python module import (from python import module as f) to:
  ///   f = pyobj._import("module")
  /// Transform a Python function import (from python import lib.foo(int) -> float as f)
  /// to:
  ///   def f(a0: int) -> float:
  ///     f = pyobj._import("lib")._getattr("foo")
  ///     return float.__from_py__(f(a0))
  /// If a return type is nullptr, the function just returns f (raw pyobj).
  StmtPtr transformPythonImport(const Expr *what, const std::vector<Param> &args,
                                const Expr *ret, const std::string &altName);
  /// Import a new file (with a given module name) into its own context and wrap its
  /// top-level statements into a function to support Python-style runtime import
  /// loading. Once import is done, generate:
  ///   _import_N_done = False
  ///   def _import_N():
  ///     global <imported global variables>...
  ///     __name__ = moduleName
  ///     <imported top-level statements>.
  void transformNewImport(const ImportFile &file);
  /// Transform a Python code-block @python def foo(x: int, y) -> int: <python code> to:
  ///   pyobj._exec("def foo(x, y): <python code>")
  ///   from python import __main__.foo(int, _) -> int
  StmtPtr transformPythonDefinition(const std::string &name,
                                    const std::vector<Param> &args, const Expr *ret,
                                    Stmt *codeStmt);
  /// Transform LLVM code @llvm def foo(x: int) -> float: <llvm code> to:
  ///   def foo(x: int) -> float:
  ///     StringExpr("<llvm code>")
  ///     SuiteStmt(referenced_types)
  /// As LLVM code can reference types and static expressions in {= expr} block,
  /// all such referenced expression will be stored in the above referenced_types.
  /// "<llvm code>" will also be transformed accordingly: each {= expr} reference will
  /// be replaced with {} so that fmt::format can easily later fill the gaps.
  /// Note that any brace ({ or }) that is not part of {= expr} reference will be
  /// escaped (e.g. { -> {{ and } -> }}) so that fmt::format can print them as-is.
  StmtPtr transformLLVMDefinition(Stmt *codeStmt);
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
