// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "codon/parser/ast/expr.h"
#include "codon/parser/ast/types.h"
#include "codon/parser/common.h"

namespace codon::ast {

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
struct Stmt : public codon::SrcObject {
  using base_type = Stmt;

  /// Flag that indicates if all types in a statement are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;
  /// Statement age.
  int age;

public:
  Stmt();
  Stmt(const Stmt &s) = default;
  explicit Stmt(const codon::SrcInfo &s);

  /// Convert a node to an S-expression.
  std::string toString() const;
  virtual std::string toString(int indent) const = 0;
  /// Validate a node. Throw ParseASTException if a node is not valid.
  void validate() const;
  /// Deep copy a node.
  virtual std::shared_ptr<Stmt> clone() const = 0;
  /// Accept an AST visitor.
  virtual void accept(ASTVisitor &) = 0;

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const Stmt &stmt) {
    return out << stmt.toString();
  }

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual AssignStmt *getAssign() { return nullptr; }
  virtual ClassStmt *getClass() { return nullptr; }
  virtual ExprStmt *getExpr() { return nullptr; }
  virtual SuiteStmt *getSuite() { return nullptr; }
  virtual FunctionStmt *getFunction() { return nullptr; }

  /// @return the first statement in a suite; if a statement is not a suite, returns the
  /// statement itself
  virtual Stmt *firstInBlock() { return this; }

  bool isDone() const { return done; }
  void setDone() { done = true; }
};
using StmtPtr = std::shared_ptr<Stmt>;

/// Suite (block of statements) statement (stmt...).
/// @li a = 5; foo(1)
struct SuiteStmt : public Stmt {
  using Stmt::Stmt;

  std::vector<StmtPtr> stmts;

  /// These constructors flattens the provided statement vector (see flatten() below).
  explicit SuiteStmt(std::vector<StmtPtr> stmts = {});
  /// Convenience constructor
  template <typename... Ts>
  SuiteStmt(StmtPtr stmt, Ts... stmts) : stmts({stmt, stmts...}) {}
  SuiteStmt(const SuiteStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  SuiteStmt *getSuite() override { return this; }
  Stmt *firstInBlock() override {
    return stmts.empty() ? nullptr : stmts[0]->firstInBlock();
  }
  StmtPtr *lastInBlock();

  /// Flatten all nested SuiteStmt objects that do not own a block in the statement
  /// vector. This is shallow flattening.
  static void flatten(const StmtPtr &s, std::vector<StmtPtr> &stmts);
};

/// Break statement.
/// @li break
struct BreakStmt : public Stmt {
  BreakStmt() = default;
  BreakStmt(const BreakStmt &stmt) = default;

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Continue statement.
/// @li continue
struct ContinueStmt : public Stmt {
  ContinueStmt() = default;
  ContinueStmt(const ContinueStmt &stmt) = default;

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Expression statement (expr).
/// @li 3 + foo()
struct ExprStmt : public Stmt {
  ExprPtr expr;

  explicit ExprStmt(ExprPtr expr);
  ExprStmt(const ExprStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  ExprStmt *getExpr() override { return this; }
};

/// Assignment statement (lhs: type = rhs).
/// @li a = 5
/// @li a: Optional[int] = 5
/// @li a, b, c = 5, *z
struct AssignStmt : public Stmt {
  ExprPtr lhs, rhs, type;

  AssignStmt(ExprPtr lhs, ExprPtr rhs, ExprPtr type = nullptr);
  AssignStmt(const AssignStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  AssignStmt *getAssign() override { return this; }

  bool isUpdate() const { return update != Assign; }
  bool isAtomicUpdate() const { return update == UpdateAtomic; }
  void setUpdate() { update = Update; }
  void setAtomicUpdate() { update = UpdateAtomic; }

private:
  enum { Assign, Update, UpdateAtomic } update;
};

/// Deletion statement (del expr).
/// @li del a
/// @li del a[5]
struct DelStmt : public Stmt {
  ExprPtr expr;

  explicit DelStmt(ExprPtr expr);
  DelStmt(const DelStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Print statement (print expr).
/// @li print a, b
struct PrintStmt : public Stmt {
  std::vector<ExprPtr> items;
  /// True if there is a dangling comma after print: print a,
  bool isInline;

  explicit PrintStmt(std::vector<ExprPtr> items, bool isInline);
  PrintStmt(const PrintStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Return statement (return expr).
/// @li return
/// @li return a
struct ReturnStmt : public Stmt {
  /// nullptr if this is an empty return/yield statements.
  ExprPtr expr;

  explicit ReturnStmt(ExprPtr expr = nullptr);
  ReturnStmt(const ReturnStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Yield statement (yield expr).
/// @li yield
/// @li yield a
struct YieldStmt : public Stmt {
  /// nullptr if this is an empty return/yield statements.
  ExprPtr expr;

  explicit YieldStmt(ExprPtr expr = nullptr);
  YieldStmt(const YieldStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Assert statement (assert expr).
/// @li assert a
/// @li assert a, "Message"
struct AssertStmt : public Stmt {
  ExprPtr expr;
  /// nullptr if there is no message.
  ExprPtr message;

  explicit AssertStmt(ExprPtr expr, ExprPtr message = nullptr);
  AssertStmt(const AssertStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// While loop statement (while cond: suite; else: elseSuite).
/// @li while True: print
/// @li while True: break
///          else: print
struct WhileStmt : public Stmt {
  ExprPtr cond;
  StmtPtr suite;
  /// nullptr if there is no else suite.
  StmtPtr elseSuite;
  /// Set if a while loop is used to emulate goto statement
  /// (as `while gotoVar: ...`).
  std::string gotoVar = "";

  WhileStmt(ExprPtr cond, StmtPtr suite, StmtPtr elseSuite = nullptr);
  WhileStmt(const WhileStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// For loop statement (for var in iter: suite; else elseSuite).
/// @li for a, b in c: print
/// @li for i in j: break
///          else: print
struct ForStmt : public Stmt {
  ExprPtr var;
  ExprPtr iter;
  StmtPtr suite;
  StmtPtr elseSuite;
  ExprPtr decorator;
  std::vector<CallExpr::Arg> ompArgs;

  /// Indicates if iter was wrapped with __iter__() call.
  bool wrapped;
  /// True if there are no break/continue within the loop
  bool flat;

  ForStmt(ExprPtr var, ExprPtr iter, StmtPtr suite, StmtPtr elseSuite = nullptr,
          ExprPtr decorator = nullptr, std::vector<CallExpr::Arg> ompArgs = {});
  ForStmt(const ForStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// If block statement (if cond: suite; (elif cond: suite)...).
/// @li if a: foo()
/// @li if a: foo()
///          elif b: bar()
/// @li if a: foo()
///          elif b: bar()
///          else: baz()
struct IfStmt : public Stmt {
  ExprPtr cond;
  /// elseSuite can be nullptr (if no else is found).
  StmtPtr ifSuite, elseSuite;

  IfStmt(ExprPtr cond, StmtPtr ifSuite, StmtPtr elseSuite = nullptr);
  IfStmt(const IfStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Match statement (match what: (case pattern: case)...).
/// @li match a:
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
  std::vector<MatchCase> cases;

  MatchStmt(ExprPtr what, std::vector<MatchCase> cases);
  MatchStmt(const MatchStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Import statement.
/// This node describes various kinds of import statements:
///  - from from import what (as as)
///  - import what (as as)
///  - from c import what(args...) (-> ret) (as as)
///  - from .(dots...)from import what (as as)
/// @li import a
/// @li from b import a
/// @li from ...b import a as ai
/// @li from c import foo(int) -> int as bar
/// @li from python.numpy import array
/// @li from python import numpy.array(int) -> int as na
struct ImportStmt : public Stmt {
  ExprPtr from, what;
  std::string as;
  /// Number of dots in a relative import (e.g. dots is 3 for "from ...foo").
  size_t dots;
  /// Function argument types for C imports.
  std::vector<Param> args;
  /// Function return type for C imports.
  ExprPtr ret;
  /// Set if this is a function C import (not variable import)
  bool isFunction;

  ImportStmt(ExprPtr from, ExprPtr what, std::vector<Param> args = {},
             ExprPtr ret = nullptr, std::string as = "", size_t dots = 0,
             bool isFunction = true);
  ImportStmt(const ImportStmt &stmt);

  std::string toString(int indent) const override;
  void validate() const;
  ACCEPT(ASTVisitor);
};

/// Try-catch statement (try: suite; (catch var (as exc): suite)...; finally: finally).
/// @li: try: a
///           catch e: pass
///           catch e as Exc: pass
///           catch: pass
///           finally: print
struct TryStmt : public Stmt {
  struct Catch {
    /// empty string if a catch is unnamed.
    std::string var;
    /// nullptr if there is no explicit exception type.
    ExprPtr exc;
    StmtPtr suite;

    Catch clone() const;
  };

  StmtPtr suite;
  std::vector<Catch> catches;
  /// nullptr if there is no finally block.
  StmtPtr finally;

  TryStmt(StmtPtr suite, std::vector<Catch> catches, StmtPtr finally = nullptr);
  TryStmt(const TryStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Throw statement (raise expr).
/// @li: raise a
struct ThrowStmt : public Stmt {
  ExprPtr expr;
  // True if a statement was transformed during type-checking stage
  // (to avoid setting up ExcHeader multiple times).
  bool transformed;

  explicit ThrowStmt(ExprPtr expr, bool transformed = false);
  ThrowStmt(const ThrowStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Global variable statement (global var).
/// @li: global a
struct GlobalStmt : public Stmt {
  std::string var;
  bool nonLocal;

  explicit GlobalStmt(std::string var, bool nonLocal = false);
  GlobalStmt(const GlobalStmt &stmt) = default;

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

struct Attr {
  // Toplevel attributes
  const static std::string LLVM;
  const static std::string Python;
  const static std::string Atomic;
  const static std::string Property;
  const static std::string StaticMethod;
  const static std::string Attribute;
  const static std::string C;
  // Internal attributes
  const static std::string Internal;
  const static std::string HiddenFromUser;
  const static std::string ForceRealize;
  const static std::string RealizeWithoutSelf; // not internal
  // Compiler-generated attributes
  const static std::string CVarArg;
  const static std::string Method;
  const static std::string Capture;
  const static std::string HasSelf;
  const static std::string IsGenerator;
  // Class attributes
  const static std::string Extend;
  const static std::string Tuple;
  // Standard library attributes
  const static std::string Test;
  const static std::string Overload;
  const static std::string Export;
  // Function module
  std::string module;
  // Parent class (set for methods only)
  std::string parentClass;
  // True if a function is decorated with __attribute__
  bool isAttribute;

  std::set<std::string> magics;

  // Set of attributes
  std::set<std::string> customAttr;

  explicit Attr(const std::vector<std::string> &attrs = std::vector<std::string>());
  void set(const std::string &attr);
  void unset(const std::string &attr);
  bool has(const std::string &attr) const;
};

/// Function statement (@(attributes...) def name[funcs...](args...) -> ret: suite).
/// @li: @decorator
///           def foo[T=int, U: int](a, b: int = 0) -> list[T]: pass
struct FunctionStmt : public Stmt {
  std::string name;
  /// nullptr if return type is not specified.
  ExprPtr ret;
  std::vector<Param> args;
  StmtPtr suite;
  Attr attributes;
  std::vector<ExprPtr> decorators;

  FunctionStmt(std::string name, ExprPtr ret, std::vector<Param> args, StmtPtr suite,
               Attr attributes = Attr(), std::vector<ExprPtr> decorators = {});
  FunctionStmt(const FunctionStmt &stmt);

  std::string toString(int indent) const override;
  void validate() const;
  ACCEPT(ASTVisitor);

  /// @return a function signature that consists of generics and arguments in a
  /// S-expression form.
  /// @li (T U (int 0))
  std::string signature() const;
  bool hasAttr(const std::string &attr) const;
  void parseDecorators();

  size_t getStarArgs() const;
  size_t getKwStarArgs() const;

  FunctionStmt *getFunction() override { return this; }
  std::string getDocstr();
  std::unordered_set<std::string> getNonInferrableGenerics();
};

/// Class statement (@(attributes...) class name[generics...]: args... ; suite).
/// @li: @type
///           class F[T]:
///              m: T
///              def __new__() -> F[T]: ...
struct ClassStmt : public Stmt {
  std::string name;
  std::vector<Param> args;
  StmtPtr suite;
  Attr attributes;
  std::vector<ExprPtr> decorators;
  std::vector<ExprPtr> baseClasses;
  std::vector<ExprPtr> staticBaseClasses;

  ClassStmt(std::string name, std::vector<Param> args, StmtPtr suite,
            std::vector<ExprPtr> decorators = {}, std::vector<ExprPtr> baseClasses = {},
            std::vector<ExprPtr> staticBaseClasses = {});
  ClassStmt(std::string name, std::vector<Param> args, StmtPtr suite, Attr attr);
  ClassStmt(const ClassStmt &stmt);

  std::string toString(int indent) const override;
  void validate() const;
  ACCEPT(ASTVisitor);

  /// @return true if a class is a tuple-like record (e.g. has a "@tuple" attribute)
  bool isRecord() const;
  bool hasAttr(const std::string &attr) const;

  ClassStmt *getClass() override { return this; }

  void parseDecorators();
  static bool isClassVar(const Param &p);
  std::string getDocstr();
};

/// Yield-from statement (yield from expr).
/// @li: yield from it
struct YieldFromStmt : public Stmt {
  ExprPtr expr;

  explicit YieldFromStmt(ExprPtr expr);
  YieldFromStmt(const YieldFromStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// With statement (with (item as var)...: suite).
/// @li: with foo(), bar() as b: pass
struct WithStmt : public Stmt {
  std::vector<ExprPtr> items;
  /// empty string if a corresponding item is unnamed
  std::vector<std::string> vars;
  StmtPtr suite;

  WithStmt(std::vector<ExprPtr> items, std::vector<std::string> vars, StmtPtr suite);
  WithStmt(std::vector<std::pair<ExprPtr, ExprPtr>> items, StmtPtr suite);
  WithStmt(const WithStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Custom block statement (foo: ...).
/// @li: pt_tree: pass
struct CustomStmt : public Stmt {
  std::string keyword;
  ExprPtr expr;
  StmtPtr suite;

  CustomStmt(std::string keyword, ExprPtr expr, StmtPtr suite);
  CustomStmt(const CustomStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// The following nodes are created after the simplify stage.

/// Member assignment statement (lhs.member = rhs).
/// @li: a.x = b
struct AssignMemberStmt : public Stmt {
  ExprPtr lhs;
  std::string member;
  ExprPtr rhs;

  AssignMemberStmt(ExprPtr lhs, std::string member, ExprPtr rhs);
  AssignMemberStmt(const AssignMemberStmt &stmt);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Comment statement (# comment).
/// Currently used only for pretty-printing.
struct CommentStmt : public Stmt {
  std::string comment;

  explicit CommentStmt(std::string comment);
  CommentStmt(const CommentStmt &stmt) = default;

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

#undef ACCEPT

} // namespace codon::ast

template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<std::is_base_of<codon::ast::Stmt, T>::value, char>>
    : fmt::ostream_formatter {};

template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<
           std::is_convertible<T, std::shared_ptr<codon::ast::Stmt>>::value, char>>
    : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const T &p, FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", p ? p->toString() : "<nullptr>");
  }
};
