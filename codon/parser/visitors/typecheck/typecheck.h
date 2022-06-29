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
  types::FuncTypePtr getBestClassMethod(DotExpr *,
                                        const std::vector<types::FuncTypePtr> &,
                                        std::vector<CallExpr::Arg> *);
  types::FuncTypePtr getDispatch(const std::string &);

  /* Collection and comprehension expressions (collections.cpp) */
  void visit(TupleExpr *) override;
  void visit(GeneratorExpr *) override;

  /* Conditional expression and statements (cond.cpp) */
  /// Set type to the unification of both sides.
  /// Wrap a side with Optional.__new__() if other side is optional.
  /// Wrap conditional with .__bool__() if it is not a bool.
  /// Also evaluates static if expressions.
  void visit(IfExpr *) override;
  /// Check if a condition is static expression. If it is and evaluates to zero,
  /// DO NOT parse the enclosed suite.
  /// Otherwise, transform if cond to:
  ///   if cond.__bool__()
  /// if cond is not of type bool (no pun intended).
  void visit(IfStmt *) override;
  /// If a target type is Optional but the type of a given expression is not,
  /// replace the given expression with Optional(expr).
  void wrapOptionalIfNeeded(const types::TypePtr &targetType, ExprPtr &e);

  /* Operators (op.cpp) */
  /// Evaluate static unary expressions.
  void visit(UnaryExpr *) override;
  /// See transformBinary() below.
  /// Also evaluates static binary expressions.
  void visit(BinaryExpr *) override;
  /// Type-checks a pipe expression.
  /// Transform a stage CallExpr foo(x) without an ellipsis into:
  ///   foo(..., x).
  /// Transform any non-CallExpr stage foo into a CallExpr stage:
  ///   foo(...).
  /// If necessary, add stages (e.g. unwrap, float.__new__ or Optional.__new__)
  /// to support function call type adjustments.
  void visit(PipeExpr *) override;
  /// Transform an instantiation Instantiate(foo, {bar, baz}) to a canonical name:
  ///   IdExpr("foo[bar,baz]").
  void visit(InstantiateExpr *) override;
  /// Transform a slice start:stop:step to:
  ///   Slice(start, stop, step).
  /// Start, stop and step parameters can be nullptr; in that case, just use
  /// Optional.__new__() in place of a corresponding parameter.
  void visit(SliceExpr *) override;
  /// Transform an index expression expr[index] to:
  ///   InstantiateExpr(expr, index) if an expr is a function,
  ///   expr.itemN or a sub-tuple if index is static (see transformStaticTupleIndex()),
  ///   or expr.__getitem__(index).
  void visit(IndexExpr *) override;
  /// Transforms a binary operation a op b to a corresponding magic method call:
  ///   a.__magic__(b)
  /// Checks for the following methods:
  ///   __atomic_op__ (atomic inline operation: a += b),
  ///   __iop__ (inline operation: a += b),
  ///   __op__, and
  ///   __rop__.
  /// Also checks for the following special cases:
  ///   a and/or b -> preserves BinaryExpr to allow short-circuits later
  ///   a is None -> a.__bool__().__invert__() (if a is Optional), or False
  ///   a is b -> a.__raw__() == b.__raw__() (if a and b share the same reference type),
  ///             or a == b
  /// Return nullptr if no transformation was made.
  ExprPtr transformBinary(BinaryExpr *expr, bool isAtomic = false,
                          bool *noReturn = nullptr);
  /// Given a tuple type and the expression expr[index], check if an index is a static
  /// expression. If so, and if the type of expr is Tuple, statically extract the
  /// specified tuple item (or a sub-tuple if index is a slice).
  /// Supports both StaticExpr and IntExpr as static expressions.
  ExprPtr transformStaticTupleIndex(types::ClassType *tuple, ExprPtr &expr,
                                    ExprPtr &index);
  int64_t translateIndex(int64_t idx, int64_t len, bool clamp = false);
  int64_t sliceAdjustIndices(int64_t length, int64_t *start, int64_t *stop,
                             int64_t step);

  /* Calls (call.cpp) */
  struct PartialCallData {
    bool isPartial;               // is call itself partial?
    std::string var = "";         // variable if we are calling partialized fn
    std::vector<char> known = {}; // bitvector of known arguments
    ExprPtr args = nullptr, kwArgs = nullptr; // true if *args/**kwargs are partialized
  };
  /// See transformCall() below.
  void visit(CallExpr *) override;
  /// Transform a call expression callee(args...).
  /// Intercepts callees that are expr.dot, expr.dot[T1, T2] etc.
  /// Before any transformation, all arguments are expanded:
  ///   *args -> args[0], ..., args[N]
  ///   **kwargs -> name0=kwargs.name0, ..., nameN=kwargs.nameN
  /// Performs the following transformations:
  ///   Tuple(args...) -> Tuple.__new__(args...) (tuple constructor)
  ///   Class(args...) -> c = Class.__new__(); c.__init__(args...); c (StmtExpr)
  ///   Partial(args...) -> StmtExpr(p = Partial; PartialFn(args...))
  ///   obj(args...) -> obj.__call__(args...) (non-functions)
  /// This method will also handle the following use-cases:
  ///   named arguments,
  ///   default arguments,
  ///   default generics, and
  ///   *args / **kwargs.
  /// Any partial call will be transformed as follows:
  ///   callee(arg1, ...) -> StmtExpr(_v = Partial.callee.N10(arg1); _v).
  /// If callee is partial and it is satisfied, it will be replaced with the originating
  /// function call.
  /// Arguments are unified with the signature types. The following exceptions are
  /// supported:
  ///   Optional[T] -> T (via unwrap())
  ///   int -> float (via float.__new__())
  ///   T -> Optional[T] (via Optional.__new__())
  ///   T -> Generator[T] (via T.__iter__())
  ///   T -> Callable[T] (via T.__call__())
  ///
  /// Pipe notes: if inType and extraStage are set, this method will use inType as a
  /// pipe ellipsis type. extraStage will be set if an Optional conversion/unwrapping
  /// stage needs to be inserted before the current pipeline stage.
  ///
  /// Static call expressions: the following static expressions are supported:
  ///   isinstance(var, type) -> evaluates to bool
  ///   hasattr(type, string) -> evaluates to bool
  ///   staticlen(var) -> evaluates to int
  ///   compile_error(string) -> raises a compiler error
  ///   type(type) -> IdExpr(instantiated_type_name)
  ///
  /// Note: This is the most evil method in the whole parser suite. 🤦🏻‍
  ExprPtr transformCall(CallExpr *expr, const types::TypePtr &inType = nullptr,
                        ExprPtr *extraStage = nullptr);
  std::pair<bool, ExprPtr> transformSpecialCall(CallExpr *expr);
  bool callTransformCallArgs(std::vector<CallExpr::Arg> &args,
                             const types::TypePtr &inType = nullptr);
  ExprPtr callTransformCallee(ExprPtr &callee, std::vector<CallExpr::Arg> &args,
                              PartialCallData &part);
  ExprPtr callReorderArguments(types::ClassTypePtr callee, types::FuncTypePtr calleeFn,
                               CallExpr *expr, int &ellipsisStage,
                               PartialCallData &part);
  /// Find all generics on which a given function depends and add them to the context.
  void addFunctionGenerics(const types::FuncType *t);
  /// Generate a partial function type Partial.N01...01 (where 01...01 is a mask
  /// of size N) as follows:
  ///   @tuple @no_total_ordering @no_pickle @no_container @no_python
  ///   class Partial.N01...01[T0, T1, ..., TN]:
  ///     ptr: Function[T0, T1,...,TN]
  ///     aI: TI ... # (if mask[I-1] is zero for I >= 1)
  ///     def __call__(self, aI: TI...) -> T0: # (if mask[I-1] is one for I >= 1)
  ///       return self.ptr(self.a1, a2, self.a3...) # (depending on a mask pattern)
  /// The following partial constructor is added if an oldMask is set:
  ///     def __new_<old_mask>_<mask>(p, aI: TI...) # (if oldMask[I-1] != mask[I-1]):
  ///       return Partial.N<mask>.__new__(self.ptr, self.a1, a2, ...) # (see above)
  std::string generatePartialStub(const std::vector<char> &mask, types::FuncType *fn);
  ExprPtr transformSuper(const CallExpr *expr);
  std::vector<types::ClassTypePtr> getSuperTypes(const types::ClassTypePtr &cls);

  /* Assignments (assign.cpp) */
  void visit(AssignStmt *) override;
  void transformUpdate(AssignStmt *);
  void visit(AssignMemberStmt *) override;
  std::pair<bool, ExprPtr> transformInplaceUpdate(AssignStmt *);

  /* Loops (loops.cpp) */
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(WhileStmt *) override;
  /// Unpack heterogeneous tuple iteration: for i in t: <suite> to:
  ///   i = t[0]; <suite>
  ///   i = t[1]; <suite> ...
  /// Transform for i in t to:
  ///   for i in t.__iter__()
  /// if t is not a generator.
  void visit(ForStmt *) override;

  /* Errors and exceptions (error.cpp) */
  void visit(TryStmt *) override;
  /// Transform raise Exception() to:
  ///   _e = Exception()
  ///   _e._hdr = ExcHeader("<func>", "<file>", <line>, <col>)
  ///   raise _e
  /// Also ensure that the raised type is a tuple whose first element is ExcHeader.
  void visit(ThrowStmt *) override;

  /* Functions (function.cpp) */
  /// Unifies a function return type with a Generator[T] where T is a new unbound type.
  /// The expression itself will have type T.
  void visit(YieldExpr *) override;
  /// Wrap return a to:
  ///   return Optional(a)
  /// if a is of type T and return type is of type Optional[T]/
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  /// Parse a function stub and create a corresponding generic function type.
  /// Also realize built-ins and extern C functions.
  void visit(FunctionStmt *) override;
  /// Make an empty partial call fn(...) for a function fn.
  ExprPtr partializeFunction(ExprPtr expr);
  std::shared_ptr<types::RecordType> getFuncTypeBase(int nargs);

  /* Classes (class.cpp) */
  void visit(ClassStmt *) override;
  std::string generateTuple(size_t, const std::string & = "Tuple",
                            std::vector<std::string> = {}, bool = true);

  /* The rest (typecheck.cpp) */
  void visit(SuiteStmt *) override;
  void visit(ExprStmt *) override;
  /// Type-checks it with a new unbound type.
  void visit(EllipsisExpr *) override;
  /// Use type of an inner expression.
  void visit(StmtExpr *) override;
  void visit(StarExpr *) override;
  void visit(KeywordStarExpr *) override;
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
  findMatchingMethods(types::ClassType *typ,
                      const std::vector<types::FuncTypePtr> &methods,
                      const std::vector<CallExpr::Arg> &args);
  bool wrapExpr(ExprPtr &expr, types::TypePtr expectedType,
                const types::FuncTypePtr &callee, bool undoOnSuccess = false);

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
