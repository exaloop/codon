/*
 * typecheck.h --- Type inference and type-dependent AST transformations.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */
#pragma once

#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/visitors/format/format.h"
#include "parser/visitors/typecheck/typecheck_ctx.h"
#include "parser/visitors/visitor.h"

namespace seq {
namespace ast {

class TypecheckVisitor : public CallbackASTVisitor<ExprPtr, StmtPtr> {
  shared_ptr<TypeContext> ctx;
  shared_ptr<vector<StmtPtr>> prependStmts;
  bool allowVoidExpr;

  ExprPtr resultExpr;
  StmtPtr resultStmt;

public:
  static StmtPtr apply(shared_ptr<Cache> cache, StmtPtr stmts);

public:
  explicit TypecheckVisitor(shared_ptr<TypeContext> ctx,
                            const shared_ptr<vector<StmtPtr>> &stmts = nullptr);

  /// All of these are non-const in TypeCheck visitor.
  ExprPtr transform(const ExprPtr &e) override;
  StmtPtr transform(const StmtPtr &s) override;
  ExprPtr transform(ExprPtr &e, bool allowTypes, bool allowVoid = false,
                    bool disableActivation = false);
  ExprPtr transformType(ExprPtr &expr, bool disableActivation = false);
  types::TypePtr realize(types::TypePtr typ);

private:
  void defaultVisit(Expr *e) override;
  void defaultVisit(Stmt *s) override;

public:
  /// Set type to bool.
  void visit(BoolExpr *) override;
  /// Set type to int.
  void visit(IntExpr *) override;
  /// Set type to float.
  void visit(FloatExpr *) override;
  /// Set type to str.
  void visit(StringExpr *) override;
  /// Get the type from dictionary. If static variable (e.g. N), evaluate it and
  /// transform the evaluated number to IntExpr.
  /// Correct the identifier with a realized identifier (e.g. replace Id("Ptr") with
  /// Id("Ptr[byte]")).
  /// Also generates stubs for Tuple.N, Callable.N and Function.N.
  void visit(IdExpr *) override;
  /// Transform a tuple (a1, ..., aN) to:
  ///   Tuple.N.__new__(a1, ..., aN).
  /// If Tuple.N has not been seen before, generate a stub class for it.
  void visit(TupleExpr *) override;
  /// Transform a tuple generator tuple(expr for i in tuple) to:
  ///   Tuple.N.__new__(expr...).
  void visit(GeneratorExpr *) override;
  /// Set type to the unification of both sides.
  /// Wrap a side with Optional.__new__() if other side is optional.
  /// Wrap conditional with .__bool__() if it is not a bool.
  /// Also evaluates static if expressions.
  void visit(IfExpr *) override;
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
  /// See transformDot() below.
  void visit(DotExpr *) override;
  /// See transformCall() below.
  void visit(CallExpr *) override;
  /// Type-checks __array__[T](...) with Array[T].
  void visit(StackAllocExpr *) override;
  /// Type-checks it with a new unbound type.
  void visit(EllipsisExpr *) override;
  /// Type-checks __ptr__(expr) with Ptr[typeof(T)].
  void visit(PtrExpr *) override;
  /// Unifies a function return type with a Generator[T] where T is a new unbound type.
  /// The expression itself will have type T.
  void visit(YieldExpr *) override;
  /// Use type of an inner expression.
  void visit(StmtExpr *) override;

  void visit(SuiteStmt *) override;
  void visit(BreakStmt *) override;
  void visit(ContinueStmt *) override;
  void visit(ExprStmt *) override;
  void visit(AssignStmt *) override;
  /// Transform an atomic or an in-place statement a += b to:
  ///   a.__iadd__(a, b)
  ///   typeof(a).__atomic_add__(__ptr__(a), b)
  /// Transform an atomic statement a = min(a, b) (and max(a, b)) to:
  ///   typeof(a).__atomic_min__(__ptr__(a), b).
  /// Transform an atomic update a = b to:
  ///   typeof(a).__atomic_xchg__(__ptr__(a), b).
  /// Transformations are performed only if the appropriate magics are available.
  void visit(UpdateStmt *) override;
  /// Transform a.b = c to:
  ///   unwrap(a).b = c
  /// if a is an Optional that does not have field b.
  void visit(AssignMemberStmt *) override;
  /// Wrap return a to:
  ///   return Optional(a)
  /// if a is of type T and return type is of type Optional[T]/
  void visit(ReturnStmt *) override;
  void visit(YieldStmt *) override;
  void visit(WhileStmt *) override;
  /// Unpack heterogeneous tuple iteration: for i in t: <suite> to:
  ///   i = t[0]; <suite>
  ///   i = t[1]; <suite> ...
  /// Transform for i in t to:
  ///   for i in t.__iter__()
  /// if t is not a generator.
  void visit(ForStmt *) override;
  /// Check if a condition is static expression. If it is and evaluates to zero,
  /// DO NOT parse the enclosed suite.
  /// Otherwise, transform if cond to:
  ///   if cond.__bool__()
  /// if cond is not of type bool (no pun intended).
  void visit(IfStmt *) override;
  void visit(TryStmt *) override;
  /// Transform raise Exception() to:
  ///   _e = Exception()
  ///   _e._hdr = ExcHeader("<func>", "<file>", <line>, <col>)
  ///   raise _e
  /// Also ensure that the raised type is a tuple whose first element is ExcHeader.
  void visit(ThrowStmt *) override;
  /// Parse a function stub and create a corresponding generic function type.
  /// Also realize built-ins and extern C functions.
  void visit(FunctionStmt *) override;
  /// Parse a type stub and create a corresponding generic type.
  void visit(ClassStmt *) override;

  using CallbackASTVisitor<ExprPtr, StmtPtr>::transform;

private:
  /// If a target type is Optional but the type of a given expression is not,
  /// replace the given expression with Optional(expr).
  void wrapOptionalIfNeeded(const types::TypePtr &targetType, ExprPtr &e);
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
  /// Transforms a DotExpr expr.member to:
  ///   string(realized type of expr) if member is __class__.
  ///   unwrap(expr).member if expr is of type Optional,
  ///   expr._getattr("member") if expr is of type pyobj,
  ///   DotExpr(expr, member) if a member is a class field,
  ///   IdExpr(method_name) if expr.member is a class method, and
  ///   member(expr, ...) partial call if expr.member is an object method.
  /// If args are set, this method will use it to pick the best overloaded method and
  /// return its IdExpr without making the call partial (the rest will be handled by
  /// CallExpr).
  /// If there are multiple valid overloaded methods, pick the first one
  ///   (TODO: improve this).
  /// Return nullptr if no transformation was made.
  ExprPtr transformDot(DotExpr *expr, vector<CallExpr::Arg> *args = nullptr);
  /// Deactivate any unbound that was activated during the instantiation of type t.
  void deactivateUnbounds(types::Type *t);
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
  pair<bool, ExprPtr> transformSpecialCall(CallExpr *expr);
  /// Find all generics on which a given function depends and add them to the context.
  void addFunctionGenerics(const types::FuncType *t);

  /// Generate a tuple class Tuple.N[T1,...,TN](a1: T1, ..., aN: TN).
  /// Also used to generate a named tuple class Name.N[T1,...,TN] with field names
  /// provided in names parameter.
  string generateTupleStub(int len, const string &name = "Tuple",
                           vector<string> names = vector<string>{},
                           bool hasSuffix = true);
  /// Generate a function type Function.N[TR, T1, ..., TN] as follows:
  ///   @internal @tuple @trait
  ///   class Function.N[TR, T1, ..., TN]:
  ///     def __new__(what: Ptr[byte]) -> Function.N[TR, T1, ..., TN]
  ///     def __raw__(self: Function.N[TR, T1, ..., TN]) -> Ptr[byte]
  ///     def __str__(self: Function.N[TR, T1, ..., TN]) -> str
  /// Return the canonical name of Function.N.
  string generateCallableStub(int n);
  string generateFunctionStub(int n);
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
  string generatePartialStub(const vector<char> &mask, types::FuncType *fn);
  void generateFnCall(int n);
  /// Make an empty partial call fn(...) for a function fn.
  ExprPtr partializeFunction(ExprPtr expr);

private:
  types::TypePtr unify(types::TypePtr &a, const types::TypePtr &b);
  types::TypePtr realizeType(types::ClassType *typ);
  types::TypePtr realizeFunc(types::FuncType *typ);
  std::pair<int, StmtPtr> inferTypes(StmtPtr stmt, bool keepLast, const string &name);
  seq::ir::types::Type *getLLVMType(const types::ClassType *t);

  bool wrapExpr(ExprPtr &expr, types::TypePtr expectedType,
                const types::FuncTypePtr &callee);
  int64_t translateIndex(int64_t idx, int64_t len, bool clamp = false);
  int64_t sliceAdjustIndices(int64_t length, int64_t *start, int64_t *stop,
                             int64_t step);

  friend struct Cache;
};

} // namespace ast
} // namespace seq
