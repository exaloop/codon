/*
 * stmt.h --- Seq AST statements.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "parser/ast/expr.h"
#include "parser/ast/types.h"
#include "parser/common.h"

namespace seq {
namespace ast {

#define ACCEPT(X)                                                                      \
  using Stmt::toString;                                                                \
  StmtPtr clone() const override;                                                      \
  void accept(X &visitor) override

// Forward declarations
struct ASTVisitor;
struct AssignStmt;
struct ClassStmt;
struct ExprStmt;
struct SuiteStmt;
struct FunctionStmt;

/**
 * A Seq AST statement.
 * Each AST statement is intended to be instantiated as a shared_ptr.
 */
struct Stmt : public seq::SrcObject {
  typedef Stmt base_type;

  /// Flag that indicates if all types in a statement are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;
  /// Statement age.
  int age;

public:
  Stmt();
  Stmt(const Stmt &s) = default;
  explicit Stmt(const seq::SrcInfo &s);

  /// Convert a node to an S-expression.
  string toString() const;
  virtual string toString(int indent) const = 0;
  /// Deep copy a node.
  virtual shared_ptr<Stmt> clone() const = 0;
  /// Accept an AST visitor.
  virtual void accept(ASTVisitor &) = 0;

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const Stmt &stmt) {
    return out << stmt.toString();
  }

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual const AssignStmt *getAssign() const { return nullptr; }
  virtual const ClassStmt *getClass() const { return nullptr; }
  virtual const ExprStmt *getExpr() const { return nullptr; }
  virtual const SuiteStmt *getSuite() const { return nullptr; }
  virtual const FunctionStmt *getFunction() const { return nullptr; }

  /// @return the first statement in a suite; if a statement is not a suite, returns the
  /// statement itself
  virtual const Stmt *firstInBlock() const { return this; }
};
using StmtPtr = shared_ptr<Stmt>;

/// Suite (block of statements) statement (stmt...).
/// @example a = 5; foo(1)
struct SuiteStmt : public Stmt {
  using Stmt::Stmt;

  vector<StmtPtr> stmts;
  /// True if a suite defines new variable-scoping block.
  bool ownBlock;

  /// These constructors flattens the provided statement vector (see flatten() below).
  explicit SuiteStmt(vector<StmtPtr> stmts = {}, bool ownBlock = false);
  /// Convenience constructor
  template <typename... Ts>
  SuiteStmt(StmtPtr stmt, Ts... stmts) : stmts({stmt, stmts...}), ownBlock(false) {}
  SuiteStmt(const SuiteStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  const SuiteStmt *getSuite() const override { return this; }
  const Stmt *firstInBlock() const override {
    return stmts.empty() ? nullptr : stmts[0]->firstInBlock();
  }

  /// Flatten all nested SuiteStmt objects that do not own a block in the statement
  /// vector. This is shallow flattening.
  static void flatten(StmtPtr s, vector<StmtPtr> &stmts);
};

/// Break statement.
/// @example break
struct BreakStmt : public Stmt {
  BreakStmt() = default;
  BreakStmt(const BreakStmt &stmt) = default;

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Continue statement.
/// @example continue
struct ContinueStmt : public Stmt {
  ContinueStmt() = default;
  ContinueStmt(const ContinueStmt &stmt) = default;

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Expression statement (expr).
/// @example 3 + foo()
struct ExprStmt : public Stmt {
  ExprPtr expr;

  explicit ExprStmt(ExprPtr expr);
  ExprStmt(const ExprStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  const ExprStmt *getExpr() const override { return this; }
};

/// Assignment statement (lhs: type = rhs).
/// @example a = 5
/// @example a: Optional[int] = 5
/// @example a, b, c = 5, *z
struct AssignStmt : public Stmt {
  ExprPtr lhs, rhs, type;
  /// True if assignment always shadows existing variables. For internal use (e.g.
  /// ForStmt).
  bool shadow;

  AssignStmt(ExprPtr lhs, ExprPtr rhs, ExprPtr type = nullptr, bool shadow = false);
  AssignStmt(const AssignStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  const AssignStmt *getAssign() const override { return this; }
};

/// Deletion statement (del expr).
/// @example del a
/// @example del a[5]
struct DelStmt : public Stmt {
  ExprPtr expr;

  explicit DelStmt(ExprPtr expr);
  DelStmt(const DelStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Print statement (print expr).
/// @example print a, b
struct PrintStmt : public Stmt {
  vector<ExprPtr> items;
  /// True if there is a dangling comma after print: print a,
  bool isInline;

  explicit PrintStmt(vector<ExprPtr> items, bool isInline);
  PrintStmt(const PrintStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Return statement (return expr).
/// @example return
/// @example return a
struct ReturnStmt : public Stmt {
  /// nullptr if this is an empty return/yield statements.
  ExprPtr expr;

  explicit ReturnStmt(ExprPtr expr = nullptr);
  ReturnStmt(const ReturnStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Yield statement (yield expr).
/// @example yield
/// @example yield a
struct YieldStmt : public Stmt {
  /// nullptr if this is an empty return/yield statements.
  ExprPtr expr;

  explicit YieldStmt(ExprPtr expr = nullptr);
  YieldStmt(const YieldStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Assert statement (assert expr).
/// @example assert a
/// @example assert a, "Message"
struct AssertStmt : public Stmt {
  ExprPtr expr;
  /// nullptr if there is no message.
  ExprPtr message;

  explicit AssertStmt(ExprPtr expr, ExprPtr message = nullptr);
  AssertStmt(const AssertStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// While loop statement (while cond: suite; else: elseSuite).
/// @example while True: print
/// @example while True: break
///          else: print
struct WhileStmt : public Stmt {
  ExprPtr cond;
  StmtPtr suite;
  /// nullptr if there is no else suite.
  StmtPtr elseSuite;

  WhileStmt(ExprPtr cond, StmtPtr suite, StmtPtr elseSuite = nullptr);
  WhileStmt(const WhileStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// For loop statement (for var in iter: suite; else elseSuite).
/// @example for a, b in c: print
/// @example for i in j: break
///          else: print
struct ForStmt : public Stmt {
  ExprPtr var;
  ExprPtr iter;
  StmtPtr suite;
  StmtPtr elseSuite;
  ExprPtr decorator;
  vector<CallExpr::Arg> ompArgs;

  /// Indicates if iter was wrapped with __iter__() call.
  bool wrapped;

  ForStmt(ExprPtr var, ExprPtr iter, StmtPtr suite, StmtPtr elseSuite = nullptr,
          ExprPtr decorator = nullptr, vector<CallExpr::Arg> ompArgs = {});
  ForStmt(const ForStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// If block statement (if cond: suite; (elif cond: suite)...).
/// @example if a: foo()
/// @example if a: foo()
///          elif b: bar()
/// @example if a: foo()
///          elif b: bar()
///          else: baz()
struct IfStmt : public Stmt {
  ExprPtr cond;
  /// elseSuite can be nullptr (if no else is found).
  StmtPtr ifSuite, elseSuite;

  IfStmt(ExprPtr cond, StmtPtr ifSuite, StmtPtr elseSuite = nullptr);
  IfStmt(const IfStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Match statement (match what: (case pattern: case)...).
/// @example match a:
///          case 1: print
///          case _: pass
struct MatchStmt : public Stmt {
  struct MatchCase {
    ExprPtr pattern;
    ExprPtr guard;
    StmtPtr suite;

    MatchCase clone() const;
  };
  ExprPtr what;
  vector<MatchCase> cases;

  MatchStmt(ExprPtr what, vector<MatchCase> cases);
  MatchStmt(const MatchStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Import statement.
/// This node describes various kinds of import statements:
///  - from from import what (as as)
///  - import what (as as)
///  - from c import what(args...) (-> ret) (as as)
///  - from .(dots...)from import what (as as)
/// @example import a
/// @example from b import a
/// @example from ...b import a as ai
/// @example from c import foo(int) -> int as bar
/// @example from python.numpy import array
/// @example from python import numpy.array(int) -> int as na
struct ImportStmt : public Stmt {
  ExprPtr from, what;
  string as;
  /// Number of dots in a relative import (e.g. dots is 3 for "from ...foo").
  int dots;
  /// Function argument types for C imports.
  vector<Param> args;
  /// Function return type for C imports.
  ExprPtr ret;

  ImportStmt(ExprPtr from, ExprPtr what, vector<Param> args = {}, ExprPtr ret = nullptr,
             string as = "", int dots = 0);
  ImportStmt(const ImportStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Try-catch statement (try: suite; (catch var (as exc): suite)...; finally: finally).
/// @example: try: a
///           catch e: pass
///           catch e as Exc: pass
///           catch: pass
///           finally: print
struct TryStmt : public Stmt {
  struct Catch {
    /// empty string if a catch is unnamed.
    string var;
    /// nullptr if there is no explicit exception type.
    ExprPtr exc;
    StmtPtr suite;

    Catch clone() const;
  };

  StmtPtr suite;
  vector<Catch> catches;
  /// nullptr if there is no finally block.
  StmtPtr finally;

  TryStmt(StmtPtr suite, vector<Catch> catches, StmtPtr finally = nullptr);
  TryStmt(const TryStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Throw statement (raise expr).
/// @example: raise a
struct ThrowStmt : public Stmt {
  ExprPtr expr;
  // True if a statement was transformed during type-checking stage
  // (to avoid setting up ExcHeader multuple times).
  bool transformed;

  explicit ThrowStmt(ExprPtr expr, bool transformed = false);
  ThrowStmt(const ThrowStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Global variable statement (global var).
/// @example: global a
struct GlobalStmt : public Stmt {
  string var;

  explicit GlobalStmt(string var);
  GlobalStmt(const GlobalStmt &stmt) = default;

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

struct Attr {
  // Toplevel attributes
  const static string LLVM;
  const static string Python;
  const static string Atomic;
  const static string Property;
  // Internal attributes
  const static string Internal;
  const static string ForceRealize;
  // Compiler-generated attributes
  const static string C;
  const static string CVarArg;
  const static string Method;
  const static string Capture;
  // Class attributes
  const static string Extend;
  const static string Tuple;
  // Standard library attributes
  const static string Test;
  // Function module
  string module;
  // Parent class (set for methods only)
  string parentClass;
  // True if a function is decorated with __attribute__
  bool isAttribute;
  // Set of attributes
  set<string> customAttr;

  Attr(const vector<string> &attrs = vector<string>());
  void set(const string &attr);
  void unset(const string &attr);
  bool has(const string &attr) const;
};

/// Function statement (@(attributes...) def name[funcs...](args...) -> ret: suite).
/// @example: @decorator
///           def foo[T=int, U: int](a, b: int = 0) -> list[T]: pass
struct FunctionStmt : public Stmt {
  string name;
  /// nullptr if return type is not specified.
  ExprPtr ret;
  vector<Param> args;
  StmtPtr suite;
  Attr attributes;
  vector<ExprPtr> decorators;

  FunctionStmt(string name, ExprPtr ret, vector<Param> args, StmtPtr suite,
               Attr attributes = Attr(), vector<ExprPtr> decorators = {});
  FunctionStmt(const FunctionStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  /// @return a function signature that consists of generics and arguments in a
  /// S-expression form.
  /// @example (T U (int 0))
  string signature() const;
  bool hasAttr(const string &attr) const;

  const FunctionStmt *getFunction() const override { return this; }
};

/// Class statement (@(attributes...) class name[generics...]: args... ; suite).
/// @example: @type
///           class F[T]:
///              m: T
///              def __new__() -> F[T]: ...
struct ClassStmt : public Stmt {
  string name;
  vector<Param> args;
  StmtPtr suite;
  Attr attributes;
  vector<ExprPtr> decorators;
  vector<ExprPtr> baseClasses;

  ClassStmt(string name, vector<Param> args, StmtPtr suite, Attr attributes = Attr(),
            vector<ExprPtr> decorators = {}, vector<ExprPtr> baseClasses = {});
  ClassStmt(const ClassStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  /// @return true if a class is a tuple-like record (e.g. has a "@tuple" attribute)
  bool isRecord() const;
  bool hasAttr(const string &attr) const;

  const ClassStmt *getClass() const override { return this; }
};

/// Yield-from statement (yield from expr).
/// @example: yield from it
struct YieldFromStmt : public Stmt {
  ExprPtr expr;

  explicit YieldFromStmt(ExprPtr expr);
  YieldFromStmt(const YieldFromStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// With statement (with (item as var)...: suite).
/// @example: with foo(), bar() as b: pass
struct WithStmt : public Stmt {
  vector<ExprPtr> items;
  /// empty string if a corresponding item is unnamed
  vector<string> vars;
  StmtPtr suite;

  WithStmt(vector<ExprPtr> items, vector<string> vars, StmtPtr suite);
  WithStmt(vector<pair<ExprPtr, ExprPtr>> items, StmtPtr suite);
  WithStmt(const WithStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Custom block statement (foo: ...).
/// @example: pt_tree: pass
struct CustomStmt : public Stmt {
  string keyword;
  ExprPtr expr;
  StmtPtr suite;

  CustomStmt(string keyword, ExprPtr expr, StmtPtr suite);
  CustomStmt(const CustomStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// The following nodes are created after the simplify stage.

/// Member assignment statement (lhs.member = rhs).
/// @example: a.x = b
struct AssignMemberStmt : public Stmt {
  ExprPtr lhs;
  string member;
  ExprPtr rhs;

  AssignMemberStmt(ExprPtr lhs, string member, ExprPtr rhs);
  AssignMemberStmt(const AssignMemberStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Update assignment statement (lhs = rhs).
/// Only valid if lhs exists.
/// @example: lhs = rhs
struct UpdateStmt : public Stmt {
  ExprPtr lhs, rhs;
  /// True if this is an atomic update.
  bool isAtomic;

  UpdateStmt(ExprPtr lhs, ExprPtr rhs, bool isAtomic = false);
  UpdateStmt(const UpdateStmt &stmt);

  string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

#undef ACCEPT

} // namespace ast
} // namespace seq
