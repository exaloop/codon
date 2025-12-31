// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "codon/parser/ast/expr.h"
#include "codon/parser/common.h"
#include "codon/util/serialize.h"

namespace codon::ast {

#define ACCEPT(CLASS, VISITOR, ...)                                                    \
  static const char NodeId;                                                            \
  using AcceptorExtend::clone;                                                         \
  using AcceptorExtend::accept;                                                        \
  ASTNode *clone(bool c) const override;                                               \
  void accept(VISITOR &visitor) override;                                              \
  std::string toString(int) const override;                                            \
  friend class TypecheckVisitor;                                                       \
  template <typename TE, typename TS> friend struct CallbackASTVisitor;                \
  friend struct ReplacingCallbackASTVisitor;                                           \
  inline decltype(auto) match_members() const { return std::tie(__VA_ARGS__); }        \
  SERIALIZE(CLASS, BASE(Stmt), ##__VA_ARGS__)

// Forward declarations
struct ASTVisitor;

/**
 * A Seq AST statement.
 * Each AST statement is intended to be instantiated as a shared_ptr.
 */
struct Stmt : public AcceptorExtend<Stmt, ASTNode> {
  using base_type = Stmt;

  Stmt();
  Stmt(const Stmt &s);
  Stmt(const Stmt &, bool);
  explicit Stmt(const codon::SrcInfo &s);

  bool isDone() const { return done; }
  void setDone() { done = true; }
  /// @return the first statement in a suite; if a statement is not a suite, returns the
  /// statement itself
  virtual Stmt *firstInBlock() { return this; }

  static const char NodeId;
  SERIALIZE(Stmt, BASE(ASTNode), done);

  virtual std::string wrapStmt(const std::string &) const;

protected:
  /// Flag that indicates if all types in a statement are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;
};

/// Suite (block of statements) statement (stmt...).
/// @li a = 5; foo(1)
struct SuiteStmt : public AcceptorExtend<SuiteStmt, Stmt>, Items<Stmt *> {
  explicit SuiteStmt(std::vector<Stmt *> stmts = {});
  /// Convenience constructor
  template <typename... Ts>
  explicit SuiteStmt(Stmt *stmt, Ts... stmts) : Items({stmt, stmts...}) {}
  SuiteStmt(const SuiteStmt &, bool);

  Stmt *firstInBlock() override {
    return items.empty() ? nullptr : items[0]->firstInBlock();
  }
  void flatten();
  void addStmt(Stmt *s);

  static SuiteStmt *wrap(Stmt *);

  ACCEPT(SuiteStmt, ASTVisitor, items);
};

/// Break statement.
/// @li break
struct BreakStmt : public AcceptorExtend<BreakStmt, Stmt> {
  BreakStmt() = default;
  BreakStmt(const BreakStmt &, bool);

  ACCEPT(BreakStmt, ASTVisitor);
};

/// Continue statement.
/// @li continue
struct ContinueStmt : public AcceptorExtend<ContinueStmt, Stmt> {
  ContinueStmt() = default;
  ContinueStmt(const ContinueStmt &, bool);

  ACCEPT(ContinueStmt, ASTVisitor);
};

/// Expression statement (expr).
/// @li 3 + foo()
struct ExprStmt : public AcceptorExtend<ExprStmt, Stmt> {
  explicit ExprStmt(Expr *expr = nullptr);
  ExprStmt(const ExprStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(ExprStmt, ASTVisitor, expr);

private:
  Expr *expr;
};

/// Assignment statement (lhs: type = rhs).
/// @li a = 5
/// @li a: Optional[int] = 5
/// @li a, b, c = 5, *z
struct AssignStmt : public AcceptorExtend<AssignStmt, Stmt> {
  enum UpdateMode { Assign, Update, UpdateAtomic, ThreadLocalAssign };

  AssignStmt()
      : lhs(nullptr), rhs(nullptr), type(nullptr), update(UpdateMode::Assign) {}
  AssignStmt(Expr *lhs, Expr *rhs, Expr *type = nullptr,
             UpdateMode update = UpdateMode::Assign);
  AssignStmt(const AssignStmt &, bool);

  Expr *getLhs() const { return lhs; }
  Expr *getRhs() const { return rhs; }
  Expr *getTypeExpr() const { return type; }

  void setLhs(Expr *expr) { lhs = expr; }
  void setRhs(Expr *expr) { rhs = expr; }

  bool isAssignment() const { return update == Assign; }
  bool isUpdate() const { return update == Update; }
  bool isAtomicUpdate() const { return update == UpdateAtomic; }
  bool isThreadLocal() { return update == ThreadLocalAssign; }
  void setUpdate() { update = Update; }
  void setAtomicUpdate() { update = UpdateAtomic; }
  void setThreadLocal() { update = ThreadLocalAssign; }

  ACCEPT(AssignStmt, ASTVisitor, lhs, rhs, type, update);

private:
  Expr *lhs, *rhs, *type;
  UpdateMode update;
};

/// Deletion statement (del expr).
/// @li del a
/// @li del a[5]
struct DelStmt : public AcceptorExtend<DelStmt, Stmt> {
  explicit DelStmt(Expr *expr = nullptr);
  DelStmt(const DelStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(DelStmt, ASTVisitor, expr);

private:
  Expr *expr;
};

/// Print statement (print expr).
/// @li print a, b
struct PrintStmt : public AcceptorExtend<PrintStmt, Stmt>, Items<Expr *> {
  explicit PrintStmt(std::vector<Expr *> items = {}, bool noNewline = false);
  PrintStmt(const PrintStmt &, bool);

  bool hasNewline() const { return !noNewline; }

  ACCEPT(PrintStmt, ASTVisitor, items, noNewline);

private:
  /// True if there is a dangling comma after print: print a,
  bool noNewline;
};

/// Return statement (return expr).
/// @li return
/// @li return a
struct ReturnStmt : public AcceptorExtend<ReturnStmt, Stmt> {
  explicit ReturnStmt(Expr *expr = nullptr);
  ReturnStmt(const ReturnStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(ReturnStmt, ASTVisitor, expr);

private:
  /// nullptr if this is an empty return/yield statements.
  Expr *expr;
};

/// Yield statement (yield expr).
/// @li yield
/// @li yield a
struct YieldStmt : public AcceptorExtend<YieldStmt, Stmt> {
  explicit YieldStmt(Expr *expr = nullptr);
  YieldStmt(const YieldStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(YieldStmt, ASTVisitor, expr);

private:
  /// nullptr if this is an empty return/yield statements.
  Expr *expr;
};

/// Assert statement (assert expr).
/// @li assert a
/// @li assert a, "Message"
struct AssertStmt : public AcceptorExtend<AssertStmt, Stmt> {
  explicit AssertStmt(Expr *expr = nullptr, Expr *message = nullptr);
  AssertStmt(const AssertStmt &, bool);

  Expr *getExpr() const { return expr; }
  Expr *getMessage() const { return message; }

  ACCEPT(AssertStmt, ASTVisitor, expr, message);

private:
  Expr *expr;
  /// nullptr if there is no message.
  Expr *message;
};

/// While loop statement (while cond: suite; else: elseSuite).
/// @li while True: print
/// @li while True: break
///          else: print
struct WhileStmt : public AcceptorExtend<WhileStmt, Stmt> {
  WhileStmt() : cond(nullptr), suite(nullptr), elseSuite(nullptr), gotoVar() {}
  WhileStmt(Expr *cond, Stmt *suite, Stmt *elseSuite = nullptr);
  WhileStmt(const WhileStmt &, bool);

  Expr *getCond() const { return cond; }
  SuiteStmt *getSuite() const { return suite; }
  SuiteStmt *getElse() const { return elseSuite; }

  ACCEPT(WhileStmt, ASTVisitor, cond, suite, elseSuite, gotoVar);

private:
  Expr *cond;
  SuiteStmt *suite;
  /// nullptr if there is no else suite.
  SuiteStmt *elseSuite;

  /// Set if a while loop is used to emulate goto statement
  /// (as `while gotoVar: ...`).
  std::string gotoVar;
};

/// For loop statement (for var in iter: suite; else elseSuite).
/// @li for a, b in c: print
/// @li for i in j: break
///          else: print
struct ForStmt : public AcceptorExtend<ForStmt, Stmt> {
  ForStmt()
      : var(nullptr), iter(nullptr), suite(nullptr), elseSuite(nullptr),
        decorator(nullptr), ompArgs(), async(false), wrapped(false), flat(false) {}
  ForStmt(Expr *var, Expr *iter, Stmt *suite, Stmt *elseSuite = nullptr,
          Expr *decorator = nullptr, std::vector<CallArg> ompArgs = {},
          bool async = false);
  ForStmt(const ForStmt &, bool);

  Expr *getVar() const { return var; }
  Expr *getIter() const { return iter; }
  SuiteStmt *getSuite() const { return suite; }
  SuiteStmt *getElse() const { return elseSuite; }
  Expr *getDecorator() const { return decorator; }
  void setDecorator(Expr *e) { decorator = e; }
  bool isAsync() const { return async; }
  void setAsync() { async = true; }
  bool isWrapped() const { return wrapped; }
  bool isFlat() const { return flat; }

  ACCEPT(ForStmt, ASTVisitor, var, iter, suite, elseSuite, decorator, ompArgs, async,
         wrapped, flat);

private:
  Expr *var;
  Expr *iter;
  SuiteStmt *suite;
  SuiteStmt *elseSuite;
  Expr *decorator;
  std::vector<CallArg> ompArgs;
  bool async;

  /// Indicates if iter was wrapped with __iter__() call.
  bool wrapped;
  /// True if there are no break/continue within the loop
  bool flat;

  friend struct GeneratorExpr;
  friend class ScopingVisitor;
};

/// If block statement (if cond: suite; (elif cond: suite)...).
/// @li if a: foo()
/// @li if a: foo()
///          elif b: bar()
/// @li if a: foo()
///          elif b: bar()
///          else: baz()
struct IfStmt : public AcceptorExtend<IfStmt, Stmt> {
  IfStmt(Expr *cond = nullptr, Stmt *ifSuite = nullptr, Stmt *elseSuite = nullptr);
  IfStmt(const IfStmt &, bool);

  Expr *getCond() const { return cond; }
  SuiteStmt *getIf() const { return ifSuite; }
  SuiteStmt *getElse() const { return elseSuite; }

  ACCEPT(IfStmt, ASTVisitor, cond, ifSuite, elseSuite);

private:
  Expr *cond;
  /// elseSuite can be nullptr (if no else is found).
  SuiteStmt *ifSuite, *elseSuite;

  friend struct GeneratorExpr;
};

struct MatchCase {
  MatchCase(Expr *pattern = nullptr, Expr *guard = nullptr, Stmt *suite = nullptr);

  Expr *getPattern() const { return pattern; }
  Expr *getGuard() const { return guard; }
  SuiteStmt *getSuite() const { return suite; }

  MatchCase clone(bool) const;
  SERIALIZE(MatchCase, pattern, guard, suite);

private:
  Expr *pattern;
  Expr *guard;
  SuiteStmt *suite;

  friend struct MatchStmt;
  friend class TypecheckVisitor;
  template <typename TE, typename TS> friend struct CallbackASTVisitor;
  friend struct ReplacingCallbackASTVisitor;
};

/// Match statement (match what: (case pattern: case)...).
/// @li match a:
///          case 1: print
///          case _: pass
struct MatchStmt : public AcceptorExtend<MatchStmt, Stmt>, Items<MatchCase> {
  MatchStmt(Expr *what = nullptr, std::vector<MatchCase> cases = {});
  MatchStmt(const MatchStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(MatchStmt, ASTVisitor, items, expr);

private:
  Expr *expr;
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
  ImportStmt(Expr *from = nullptr, Expr *what = nullptr, std::vector<Param> args = {},
             Expr *ret = nullptr, std::string as = "", size_t dots = 0,
             bool isFunction = true);
  ImportStmt(const ImportStmt &, bool);

  Expr *getFrom() const { return from; }
  Expr *getWhat() const { return what; }
  std::string getAs() const { return as; }
  size_t getDots() const { return dots; }
  Expr *getReturnType() const { return ret; }
  const std::vector<Param> &getArgs() const { return args; }
  bool isCVar() const { return !isFunction; }

  ACCEPT(ImportStmt, ASTVisitor, from, what, as, dots, args, ret, isFunction);

private:
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
};

struct ExceptStmt : public AcceptorExtend<ExceptStmt, Stmt> {
  ExceptStmt(const std::string &var = "", Expr *exc = nullptr, Stmt *suite = nullptr);
  ExceptStmt(const ExceptStmt &, bool);

  std::string getVar() const { return var; }
  Expr *getException() const { return exc; }
  SuiteStmt *getSuite() const { return suite; }

  ACCEPT(ExceptStmt, ASTVisitor, var, exc, suite);

private:
  /// empty string if an except is unnamed.
  std::string var;
  /// nullptr if there is no explicit exception type.
  Expr *exc;
  SuiteStmt *suite;

  friend class ScopingVisitor;
};

/// Try-except statement (try: suite; (except var (as exc): suite)...; finally:
/// finally).
/// @li: try: a
///           except e: pass
///           except e as Exc: pass
///           except: pass
///           finally: print
struct TryStmt : public AcceptorExtend<TryStmt, Stmt>, Items<ExceptStmt *> {
  TryStmt(Stmt *suite = nullptr, std::vector<ExceptStmt *> catches = {},
          Stmt *elseSuite = nullptr, Stmt *finally = nullptr);
  TryStmt(const TryStmt &, bool);

  SuiteStmt *getSuite() const { return suite; }
  SuiteStmt *getElse() const { return elseSuite; }
  SuiteStmt *getFinally() const { return finally; }

  ACCEPT(TryStmt, ASTVisitor, items, suite, elseSuite, finally);

private:
  SuiteStmt *suite;
  /// nullptr if there is no else block.
  SuiteStmt *elseSuite;
  /// nullptr if there is no finally block.
  SuiteStmt *finally;
};

/// Throw statement (raise expr).
/// @li: raise a
struct ThrowStmt : public AcceptorExtend<ThrowStmt, Stmt> {
  explicit ThrowStmt(Expr *expr = nullptr, Expr *from = nullptr,
                     bool transformed = false);
  ThrowStmt(const ThrowStmt &, bool);

  Expr *getExpr() const { return expr; }
  Expr *getFrom() const { return from; }
  bool isTransformed() const { return transformed; }

  ACCEPT(ThrowStmt, ASTVisitor, expr, from, transformed);

private:
  Expr *expr;
  Expr *from;
  // True if a statement was transformed during type-checking stage
  // (to avoid setting up ExcHeader multiple times).
  bool transformed;
};

/// Global variable statement (global var).
/// @li: global a
struct GlobalStmt : public AcceptorExtend<GlobalStmt, Stmt> {
  explicit GlobalStmt(std::string var = "", bool nonLocal = false);
  GlobalStmt(const GlobalStmt &, bool);

  std::string getVar() const { return var; }
  bool isNonLocal() const { return nonLocal; }

  ACCEPT(GlobalStmt, ASTVisitor, var, nonLocal);

private:
  std::string var;
  bool nonLocal;
};

/// Function statement (@(attributes...) def name[funcs...](args...) -> ret: suite).
/// @li: @decorator
///           def foo[T=int, U: int](a, b: int = 0) -> list[T]: pass
struct FunctionStmt : public AcceptorExtend<FunctionStmt, Stmt>, Items<Param> {
  FunctionStmt(std::string name = "", Expr *ret = nullptr, std::vector<Param> args = {},
               Stmt *suite = nullptr, std::vector<Expr *> decorators = {},
               bool async = false);
  FunctionStmt(const FunctionStmt &, bool);

  std::string getName() const { return name; }
  void setName(const std::string &n) { name = n; }
  Expr *getReturn() const { return ret; }
  SuiteStmt *getSuite() const { return suite; }
  void setSuite(SuiteStmt *s) { suite = s; }
  const std::vector<Expr *> &getDecorators() const { return decorators; }
  void setDecorators(const std::vector<Expr *> &d) { decorators = d; }
  bool isAsync() const { return async; }
  void setAsync() { async = true; }
  void addParam(const Param &p) { items.push_back(p); }

  /// @return a function signature that consists of generics and arguments in a
  /// S-expression form.
  /// @li (T U (int 0))
  std::string getSignature();
  size_t getStarArgs() const;
  size_t getKwStarArgs() const;
  std::string getDocstr() const;
  std::unordered_set<std::string> getNonInferrableGenerics() const;
  bool hasFunctionAttribute(const std::string &attr) const;

  ACCEPT(FunctionStmt, ASTVisitor, name, items, ret, suite, decorators, async);

private:
  std::string name;
  Expr *ret; /// nullptr if return type is not specified.
  SuiteStmt *suite;
  std::vector<Expr *> decorators;
  bool async;
  std::string signature;

  friend struct Cache;
};

/// Class statement (@(attributes...) class name[generics...]: args... ; suite).
/// @li: @type
///           class F[T]:
///              m: T
///              def __new__() -> F[T]: ...
struct ClassStmt : public AcceptorExtend<ClassStmt, Stmt>, Items<Param> {
  ClassStmt(std::string name = "", std::vector<Param> args = {}, Stmt *suite = nullptr,
            std::vector<Expr *> decorators = {},
            const std::vector<Expr *> &baseClasses = {},
            std::vector<Expr *> staticBaseClasses = {});
  ClassStmt(const ClassStmt &, bool);

  std::string getName() const { return name; }
  SuiteStmt *getSuite() const { return suite; }
  const std::vector<Expr *> &getDecorators() const { return decorators; }
  void setDecorators(const std::vector<Expr *> &d) { decorators = d; }
  const std::vector<Expr *> &getBaseClasses() const { return baseClasses; }
  const std::vector<Expr *> &getStaticBaseClasses() const { return staticBaseClasses; }

  /// @return true if a class is a tuple-like record (e.g. has a "@tuple" attribute)
  bool isRecord() const;
  std::string getDocstr() const;

  static bool isClassVar(const Param &p);

  ACCEPT(ClassStmt, ASTVisitor, name, suite, items, decorators, baseClasses,
         staticBaseClasses);

private:
  std::string name;
  SuiteStmt *suite;
  std::vector<Expr *> decorators;
  std::vector<Expr *> baseClasses;
  std::vector<Expr *> staticBaseClasses;
};

/// Yield-from statement (yield from expr).
/// @li: yield from it
struct YieldFromStmt : public AcceptorExtend<YieldFromStmt, Stmt> {
  explicit YieldFromStmt(Expr *expr = nullptr);
  YieldFromStmt(const YieldFromStmt &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(YieldFromStmt, ASTVisitor, expr);

private:
  Expr *expr;
};

/// With statement (with (item as var)...: suite).
/// @li: with foo(), bar() as b: pass
struct WithStmt : public AcceptorExtend<WithStmt, Stmt>, Items<Expr *> {
  WithStmt(std::vector<Expr *> items = {}, std::vector<std::string> vars = {},
           Stmt *suite = nullptr);
  WithStmt(std::vector<std::pair<Expr *, Expr *>> items, Stmt *suite);
  WithStmt(const WithStmt &, bool);

  const std::vector<std::string> &getVars() const { return vars; }
  SuiteStmt *getSuite() const { return suite; }

  ACCEPT(WithStmt, ASTVisitor, items, vars, suite);

private:
  /// empty string if a corresponding item is unnamed
  std::vector<std::string> vars;
  SuiteStmt *suite;
};

/// Custom block statement (foo: ...).
/// @li: pt_tree: pass
struct CustomStmt : public AcceptorExtend<CustomStmt, Stmt> {
  CustomStmt(std::string keyword = "", Expr *expr = nullptr, Stmt *suite = nullptr);
  CustomStmt(const CustomStmt &, bool);

  std::string getKeyword() const { return keyword; }
  Expr *getExpr() const { return expr; }
  SuiteStmt *getSuite() const { return suite; }

  ACCEPT(CustomStmt, ASTVisitor, keyword, expr, suite);

private:
  std::string keyword;
  Expr *expr;
  SuiteStmt *suite;
};

struct DirectiveStmt : public AcceptorExtend<DirectiveStmt, Stmt> {
  DirectiveStmt(std::string key = "", std::string value = "");
  DirectiveStmt(const DirectiveStmt &, bool);

  std::string getKey() const { return key; }
  std::string getValue() const { return value; }

  ACCEPT(DirectiveStmt, ASTVisitor, key, value);

private:
  std::string key, value;
};

/// The following nodes are created during typechecking.

/// Member assignment statement (lhs.member = rhs).
/// @li: a.x = b
struct AssignMemberStmt : public AcceptorExtend<AssignMemberStmt, Stmt> {
  AssignMemberStmt(Expr *lhs = nullptr, std::string member = "", Expr *rhs = nullptr,
                   Expr *type = nullptr);
  AssignMemberStmt(const AssignMemberStmt &, bool);

  Expr *getLhs() const { return lhs; }
  std::string getMember() const { return member; }
  Expr *getRhs() const { return rhs; }
  Expr *getTypeExpr() const { return type; }

  ACCEPT(AssignMemberStmt, ASTVisitor, lhs, member, rhs, type);

private:
  Expr *lhs;
  std::string member;
  Expr *rhs;
  Expr *type;
};

/// Comment statement (# comment).
/// Currently used only for pretty-printing.
struct CommentStmt : public AcceptorExtend<CommentStmt, Stmt> {
  explicit CommentStmt(std::string comment = "");
  CommentStmt(const CommentStmt &, bool);

  std::string getComment() const { return comment; }

  ACCEPT(CommentStmt, ASTVisitor, comment);

private:
  std::string comment;
};

#undef ACCEPT

} // namespace codon::ast

namespace tser {
static void operator<<(codon::ast::Stmt *t, Archive &a) {
  using S = codon::PolymorphicSerializer<Archive, codon::ast::Stmt>;
  a.save(t != nullptr);
  if (t) {
    auto typ = t->dynamicNodeId();
    auto key = S::_serializers[const_cast<void *>(typ)];
    a.save(key);
    S::save(key, t, a);
  }
}
static void operator>>(codon::ast::Stmt *&t, Archive &a) {
  using S = codon::PolymorphicSerializer<Archive, codon::ast::Stmt>;
  bool empty = a.load<bool>();
  if (!empty) {
    std::string key = a.load<std::string>();
    S::load(key, t, a);
  } else {
    t = nullptr;
  }
}
} // namespace tser
