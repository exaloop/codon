// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "codon/parser/ast/expr.h"
#include "codon/parser/ast/types.h"
#include "codon/parser/common.h"
#include "codon/util/serialize.h"

namespace codon::ast {

#define ACCEPT(X)                                                                      \
  using Stmt::toString;                                                                \
  using AcceptorExtend::clone;                                                         \
  using AcceptorExtend::accept;                                                        \
  ASTNode *clone(bool) const override;                                                 \
  void accept(X &visitor) override

// Forward declarations
struct ASTVisitor;
struct AssignStmt;
struct ClassStmt;
struct ExprStmt;
struct SuiteStmt;
struct FunctionStmt;
struct ForStmt;
struct IfStmt;
struct TryStmt;

/**
 * A Seq AST statement.
 * Each AST statement is intended to be instantiated as a shared_ptr.
 */
struct Stmt : public AcceptorExtend<Stmt, ASTNode> {
  using base_type = Stmt;
  static const char NodeId;

  /// Flag that indicates if all types in a statement are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;

public:
  Stmt();
  Stmt(const Stmt &s);
  Stmt(const Stmt &, bool);
  explicit Stmt(const codon::SrcInfo &s);

  /// Validate a node. Throw ParseASTException if a node is not valid.
  void validate() const;

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual AssignStmt *getAssign() { return nullptr; }
  virtual ClassStmt *getClass() { return nullptr; }
  virtual ExprStmt *getExpr() { return nullptr; }
  virtual SuiteStmt *getSuite() { return nullptr; }
  virtual FunctionStmt *getFunction() { return nullptr; }
  virtual TryStmt *getTry() { return nullptr; }
  virtual IfStmt *getIf() { return nullptr; }
  virtual ForStmt *getFor() { return nullptr; }

  /// @return the first statement in a suite; if a statement is not a suite, returns the
  /// statement itself
  virtual Stmt *firstInBlock() { return this; }

  bool isDone() const { return done; }
  void setDone() { done = true; }

  SERIALIZE(Stmt, BASE(ASTNode));
};

/// Suite (block of statements) statement (stmt...).
/// @li a = 5; foo(1)
struct SuiteStmt : public AcceptorExtend<SuiteStmt, Stmt> {
  static const char NodeId;
  std::vector<Stmt *> stmts;

  explicit SuiteStmt(std::vector<Stmt *> stmts = {});
  /// Convenience constructor
  template <typename... Ts>
  SuiteStmt(Stmt *stmt, Ts... stmts) : stmts({stmt, stmts...}) {}
  SuiteStmt(const SuiteStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  SuiteStmt *getSuite() override { return this; }
  Stmt *firstInBlock() override {
    return stmts.empty() ? nullptr : stmts[0]->firstInBlock();
  }
  void shallow_flatten();
  Stmt **lastInBlock();
  static SuiteStmt *wrap(Stmt *);
};

/// Break statement.
/// @li break
struct BreakStmt : public AcceptorExtend<BreakStmt, Stmt> {
  static const char NodeId;

  BreakStmt() = default;
  BreakStmt(const BreakStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Continue statement.
/// @li continue
struct ContinueStmt : public AcceptorExtend<ContinueStmt, Stmt> {
  static const char NodeId;

  ContinueStmt() = default;
  ContinueStmt(const ContinueStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Expression statement (expr).
/// @li 3 + foo()
struct ExprStmt : public AcceptorExtend<ExprStmt, Stmt> {
  static const char NodeId;

  Expr *expr;

  explicit ExprStmt(Expr *expr);
  ExprStmt(const ExprStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  ExprStmt *getExpr() override { return this; }
};

/// Assignment statement (lhs: type = rhs).
/// @li a = 5
/// @li a: Optional[int] = 5
/// @li a, b, c = 5, *z
struct AssignStmt : public AcceptorExtend<AssignStmt, Stmt> {
  static const char NodeId;

  enum UpdateMode { Assign, Update, UpdateAtomic };

  Expr *lhs, *rhs, *type;
  Stmt *preamble = nullptr;

  AssignStmt(Expr *lhs, Expr *rhs, Expr *type = nullptr,
             UpdateMode update = UpdateMode::Assign);
  AssignStmt(const AssignStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);

  AssignStmt *getAssign() override { return this; }

  bool isUpdate() const { return update != Assign; }
  bool isAtomicUpdate() const { return update == UpdateAtomic; }
  void setUpdate() { update = Update; }
  void setAtomicUpdate() { update = UpdateAtomic; }

  Stmt *unpack() const;

private:
  UpdateMode update;
};

/// Deletion statement (del expr).
/// @li del a
/// @li del a[5]
struct DelStmt : public AcceptorExtend<DelStmt, Stmt> {
  static const char NodeId;

  Expr *expr;

  explicit DelStmt(Expr *expr);
  DelStmt(const DelStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Print statement (print expr).
/// @li print a, b
struct PrintStmt : public AcceptorExtend<PrintStmt, Stmt> {
  static const char NodeId;

  std::vector<Expr *> items;
  /// True if there is a dangling comma after print: print a,
  bool isInline;

  explicit PrintStmt(std::vector<Expr *> items, bool isInline);
  PrintStmt(const PrintStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Return statement (return expr).
/// @li return
/// @li return a
struct ReturnStmt : public AcceptorExtend<ReturnStmt, Stmt> {
  static const char NodeId;

  /// nullptr if this is an empty return/yield statements.
  Expr *expr;

  explicit ReturnStmt(Expr *expr = nullptr);
  ReturnStmt(const ReturnStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Yield statement (yield expr).
/// @li yield
/// @li yield a
struct YieldStmt : public AcceptorExtend<YieldStmt, Stmt> {
  static const char NodeId;

  /// nullptr if this is an empty return/yield statements.
  Expr *expr;

  explicit YieldStmt(Expr *expr = nullptr);
  YieldStmt(const YieldStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Assert statement (assert expr).
/// @li assert a
/// @li assert a, "Message"
struct AssertStmt : public AcceptorExtend<AssertStmt, Stmt> {
  static const char NodeId;

  Expr *expr;
  /// nullptr if there is no message.
  Expr *message;

  explicit AssertStmt(Expr *expr, Expr *message = nullptr);
  AssertStmt(const AssertStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// While loop statement (while cond: suite; else: elseSuite).
/// @li while True: print
/// @li while True: break
///          else: print
struct WhileStmt : public AcceptorExtend<WhileStmt, Stmt> {
  static const char NodeId;

  Expr *cond;
  SuiteStmt *suite;
  /// nullptr if there is no else suite.
  SuiteStmt *elseSuite;
  /// Set if a while loop is used to emulate goto statement
  /// (as `while gotoVar: ...`).
  std::string gotoVar = "";

  WhileStmt(Expr *cond, Stmt *suite, Stmt *elseSuite = nullptr);
  WhileStmt(const WhileStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// For loop statement (for var in iter: suite; else elseSuite).
/// @li for a, b in c: print
/// @li for i in j: break
///          else: print
struct ForStmt : public AcceptorExtend<ForStmt, Stmt> {
  static const char NodeId;

  Expr *var;
  Expr *iter;
  SuiteStmt *suite;
  SuiteStmt *elseSuite;
  Expr *decorator;
  std::vector<CallExpr::Arg> ompArgs;

  /// Indicates if iter was wrapped with __iter__() call.
  bool wrapped;
  /// True if there are no break/continue within the loop
  bool flat;

  ForStmt(Expr *var, Expr *iter, Stmt *suite, Stmt *elseSuite = nullptr,
          Expr *decorator = nullptr, std::vector<CallExpr::Arg> ompArgs = {});
  ForStmt(const ForStmt &, bool);

  ForStmt *getFor() override { return this; }

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
struct IfStmt : public AcceptorExtend<IfStmt, Stmt> {
  static const char NodeId;

  Expr *cond;
  /// elseSuite can be nullptr (if no else is found).
  SuiteStmt *ifSuite, *elseSuite;

  IfStmt(Expr *cond, Stmt *ifSuite, Stmt *elseSuite = nullptr);
  IfStmt(const IfStmt &, bool);

  IfStmt *getIf() override { return this; }

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Match statement (match what: (case pattern: case)...).
/// @li match a:
///          case 1: print
///          case _: pass
struct MatchStmt : public AcceptorExtend<MatchStmt, Stmt> {
  static const char NodeId;

  struct MatchCase {
    Expr *pattern;
    Expr *guard;
    SuiteStmt *suite;

    MatchCase(Expr *, Expr *, Stmt *);
    MatchCase clone(bool) const;
  };
  Expr *what;
  std::vector<MatchCase> cases;

  MatchStmt(Expr *what, std::vector<MatchCase> cases);
  MatchStmt(const MatchStmt &, bool);

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
struct ImportStmt : public AcceptorExtend<ImportStmt, Stmt> {
  static const char NodeId;

  Expr *from, *what;
  std::string as;
  /// Number of dots in a relative import (e.g. dots is 3 for "from ...foo").
  size_t dots;
  /// Function argument types for C imports.
  std::vector<Param> args;
  /// Function return type for C imports.
  Expr *ret;
  /// Set if this is a function C import (not variable import)
  bool isFunction;

  ImportStmt(Expr *from, Expr *what, std::vector<Param> args = {}, Expr *ret = nullptr,
             std::string as = "", size_t dots = 0, bool isFunction = true);
  ImportStmt(const ImportStmt &, bool);

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
struct TryStmt : public AcceptorExtend<TryStmt, Stmt> {
  static const char NodeId;

  struct Catch : public AcceptorExtend<Catch, Stmt> {
    static const char NodeId;

    /// empty string if a catch is unnamed.
    std::string var;
    /// nullptr if there is no explicit exception type.
    Expr *exc;
    SuiteStmt *suite;

    Catch(const std::string &, Expr *, Stmt *);
    Catch(const Catch &, bool);

    std::string toString(int indent) const override;
    ACCEPT(ASTVisitor);
  };

  SuiteStmt *suite;
  std::vector<Catch *> catches;
  /// nullptr if there is no finally block.
  SuiteStmt *finally;

  TryStmt(Stmt *suite, std::vector<Catch *> catches, Stmt *finally = nullptr);
  TryStmt(const TryStmt &, bool);

  TryStmt *getTry() override { return this; }

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Throw statement (raise expr).
/// @li: raise a
struct ThrowStmt : public AcceptorExtend<ThrowStmt, Stmt> {
  static const char NodeId;

  Expr *expr;
  // True if a statement was transformed during type-checking stage
  // (to avoid setting up ExcHeader multiple times).
  bool transformed;

  explicit ThrowStmt(Expr *expr, bool transformed = false);
  ThrowStmt(const ThrowStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Global variable statement (global var).
/// @li: global a
struct GlobalStmt : public AcceptorExtend<GlobalStmt, Stmt> {
  static const char NodeId;

  std::string var;
  bool nonLocal;

  explicit GlobalStmt(std::string var, bool nonLocal = false);
  GlobalStmt(const GlobalStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Function statement (@(attributes...) def name[funcs...](args...) -> ret: suite).
/// @li: @decorator
///           def foo[T=int, U: int](a, b: int = 0) -> list[T]: pass
struct FunctionStmt : public AcceptorExtend<FunctionStmt, Stmt> {
  static const char NodeId;

  std::string name;
  /// nullptr if return type is not specified.
  Expr *ret;
  std::vector<Param> args;
  SuiteStmt *suite;
  std::vector<Expr *> decorators;

  FunctionStmt(std::string name, Expr *ret, std::vector<Param> args, Stmt *suite,
               std::vector<Expr *> decorators = {});
  FunctionStmt(const FunctionStmt &, bool);

  std::string toString(int indent) const override;
  void validate() const;
  ACCEPT(ASTVisitor);

  /// @return a function signature that consists of generics and arguments in a
  /// S-expression form.
  /// @li (T U (int 0))
  std::string signature() const;
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
struct ClassStmt : public AcceptorExtend<ClassStmt, Stmt> {
  static const char NodeId;

  std::string name;
  std::vector<Param> args;
  SuiteStmt *suite;
  std::vector<Expr *> decorators;
  std::vector<Expr *> baseClasses;
  std::vector<Expr *> staticBaseClasses;

  ClassStmt(std::string name, std::vector<Param> args, Stmt *suite,
            std::vector<Expr *> decorators = {}, std::vector<Expr *> baseClasses = {},
            std::vector<Expr *> staticBaseClasses = {});
  ClassStmt(const ClassStmt &, bool);

  std::string toString(int indent) const override;
  void validate() const;
  ACCEPT(ASTVisitor);

  /// @return true if a class is a tuple-like record (e.g. has a "@tuple" attribute)
  bool isRecord() const;

  ClassStmt *getClass() override { return this; }

  void parseDecorators();
  static bool isClassVar(const Param &p);
  std::string getDocstr();
};

/// Yield-from statement (yield from expr).
/// @li: yield from it
struct YieldFromStmt : public AcceptorExtend<YieldFromStmt, Stmt> {
  static const char NodeId;

  Expr *expr;

  explicit YieldFromStmt(Expr *expr);
  YieldFromStmt(const YieldFromStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// With statement (with (item as var)...: suite).
/// @li: with foo(), bar() as b: pass
struct WithStmt : public AcceptorExtend<WithStmt, Stmt> {
  static const char NodeId;

  std::vector<Expr *> items;
  /// empty string if a corresponding item is unnamed
  std::vector<std::string> vars;
  SuiteStmt *suite;

  WithStmt(std::vector<Expr *> items, std::vector<std::string> vars, Stmt *suite);
  WithStmt(std::vector<std::pair<Expr *, Expr *>> items, Stmt *suite);
  WithStmt(const WithStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Custom block statement (foo: ...).
/// @li: pt_tree: pass
struct CustomStmt : public AcceptorExtend<CustomStmt, Stmt> {
  static const char NodeId;

  std::string keyword;
  Expr *expr;
  SuiteStmt *suite;

  CustomStmt(std::string keyword, Expr *expr, Stmt *suite);
  CustomStmt(const CustomStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// The following nodes are created during typechecking.

/// Member assignment statement (lhs.member = rhs).
/// @li: a.x = b
struct AssignMemberStmt : public AcceptorExtend<AssignMemberStmt, Stmt> {
  static const char NodeId;

  Expr *lhs;
  std::string member;
  Expr *rhs;

  AssignMemberStmt(Expr *lhs, std::string member, Expr *rhs);
  AssignMemberStmt(const AssignMemberStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

/// Comment statement (# comment).
/// Currently used only for pretty-printing.
struct CommentStmt : public AcceptorExtend<CommentStmt, Stmt> {
  static const char NodeId;

  std::string comment;

  explicit CommentStmt(std::string comment);
  CommentStmt(const CommentStmt &, bool);

  std::string toString(int indent) const override;
  ACCEPT(ASTVisitor);
};

#undef ACCEPT

} // namespace codon::ast
