/*
 * expr.h --- Seq AST expressions.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "parser/ast/types.h"
#include "parser/common.h"

namespace seq {
namespace ast {

#define ACCEPT(X)                                                                      \
  ExprPtr clone() const override;                                                      \
  void accept(X &visitor) override

// Forward declarations
struct ASTVisitor;
struct BinaryExpr;
struct CallExpr;
struct DotExpr;
struct EllipsisExpr;
struct IdExpr;
struct IfExpr;
struct IndexExpr;
struct IntExpr;
struct ListExpr;
struct NoneExpr;
struct StarExpr;
struct StmtExpr;
struct StringExpr;
struct TupleExpr;
struct UnaryExpr;
struct Stmt;

struct StaticValue {
  std::variant<int64_t, string> value;
  enum Type { NOT_STATIC = 0, STRING = 1, INT = 2 } type;
  bool evaluated;

  explicit StaticValue(Type);
  // Static(bool);
  explicit StaticValue(int64_t);
  explicit StaticValue(string);
  bool operator==(const StaticValue &s) const;
  string toString() const;
  int64_t getInt() const;
  string getString() const;
};

/**
 * A Seq AST expression.
 * Each AST expression is intended to be instantiated as a shared_ptr.
 */
struct Expr : public seq::SrcObject {
  typedef Expr base_type;

  // private:
  /// Type of the expression. nullptr by default.
  types::TypePtr type;
  /// Flag that indicates if an expression describes a type (e.g. int or list[T]).
  /// Used by transformation and type-checking stages.
  bool isTypeExpr;
  /// Flag that indicates if an expression is a compile-time static expression.
  /// Such expression is of a form:
  ///   an integer (IntExpr) without any suffix that is within i64 range
  ///   a static generic
  ///   [-,not] a
  ///   a [+,-,*,//,%,and,or,==,!=,<,<=,>,>=] b
  ///     (note: and/or will NOT short-circuit)
  ///   a if cond else b
  ///     (note: cond is static, and is true if non-zero, false otherwise).
  ///     (note: both branches will be evaluated).
  StaticValue staticValue;
  /// Flag that indicates if all types in an expression are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;

public:
  Expr();
  Expr(const Expr &expr) = default;

  /// Convert a node to an S-expression.
  virtual string toString() const = 0;
  /// Deep copy a node.
  virtual shared_ptr<Expr> clone() const = 0;
  /// Accept an AST visitor.
  virtual void accept(ASTVisitor &visitor) = 0;

  /// Get a node type.
  /// @return Type pointer or a nullptr if a type is not set.
  types::TypePtr getType() const;
  /// Set a node type.
  void setType(types::TypePtr type);
  /// @return true if a node describes a type expression.
  bool isType() const;
  /// Marks a node as a type expression.
  void markType();
  /// True if a node is static expression.
  bool isStatic() const;

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const Expr &expr) {
    return out << expr.toString();
  }

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual bool isId(const string &val) const { return false; }
  virtual const BinaryExpr *getBinary() const { return nullptr; }
  virtual const CallExpr *getCall() const { return nullptr; }
  virtual const DotExpr *getDot() const { return nullptr; }
  virtual const EllipsisExpr *getEllipsis() const { return nullptr; }
  virtual const IdExpr *getId() const { return nullptr; }
  virtual const IfExpr *getIf() const { return nullptr; }
  virtual const IndexExpr *getIndex() const { return nullptr; }
  virtual const IntExpr *getInt() const { return nullptr; }
  virtual const ListExpr *getList() const { return nullptr; }
  virtual const NoneExpr *getNone() const { return nullptr; }
  virtual const StarExpr *getStar() const { return nullptr; }
  virtual const StmtExpr *getStmtExpr() const { return nullptr; }
  virtual const StringExpr *getString() const { return nullptr; }
  virtual const TupleExpr *getTuple() const { return nullptr; }
  virtual const UnaryExpr *getUnary() const { return nullptr; }

protected:
  /// Add a type to S-expression string.
  string wrapType(const string &sexpr) const;
};
using ExprPtr = shared_ptr<Expr>;

/// Function signature parameter helper node (name: type = deflt).
struct Param : public seq::SrcObject {
  string name;
  ExprPtr type;
  ExprPtr deflt;
  bool generic;

  explicit Param(string name = "", ExprPtr type = nullptr, ExprPtr deflt = nullptr,
                 bool generic = false);

  string toString() const;
  Param clone() const;
};

/// None expression.
/// @example None
struct NoneExpr : public Expr {
  NoneExpr();
  NoneExpr(const NoneExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);

  const NoneExpr *getNone() const override { return this; }
};

/// Bool expression (value).
/// @example True
struct BoolExpr : public Expr {
  bool value;

  explicit BoolExpr(bool value);
  BoolExpr(const BoolExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Int expression (value.suffix).
/// @example 12
/// @example 13u
/// @example 000_010b
struct IntExpr : public Expr {
  /// Expression value is stored as a string that is parsed during the simplify stage.
  string value;
  /// Number suffix (e.g. "u" for "123u").
  string suffix;

  /// Parsed value and sign for "normal" 64-bit integers.
  int64_t intValue;

  explicit IntExpr(int64_t intValue);
  explicit IntExpr(const string &value, string suffix = "");
  IntExpr(const IntExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);

  const IntExpr *getInt() const override { return this; }
};

/// Float expression (value.suffix).
/// @example 12.1
/// @example 13.15z
/// @example e-12
struct FloatExpr : public Expr {
  /// Expression value is stored as a string that is parsed during the simplify stage.
  string value;
  /// Number suffix (e.g. "u" for "123u").
  string suffix;

  /// Parsed value for 64-bit floats.
  double floatValue;

  explicit FloatExpr(double floatValue);
  explicit FloatExpr(const string &value, string suffix = "");
  FloatExpr(const FloatExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// String expression (prefix"value").
/// @example s'ACGT'
/// @example "fff"
struct StringExpr : public Expr {
  // Vector of {value, prefix} strings.
  vector<pair<string, string>> strings;

  explicit StringExpr(string value, string prefix = "");
  explicit StringExpr(vector<pair<string, string>> strings);
  StringExpr(const StringExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);

  const StringExpr *getString() const override { return this; }
  string getValue() const;
};

/// Identifier expression (value).
struct IdExpr : public Expr {
  string value;

  explicit IdExpr(string value);
  IdExpr(const IdExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);

  bool isId(const string &val) const override { return this->value == val; }
  const IdExpr *getId() const override { return this; }
};

/// Star (unpacking) expression (*what).
/// @example *args
struct StarExpr : public Expr {
  ExprPtr what;

  explicit StarExpr(ExprPtr what);
  StarExpr(const StarExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const StarExpr *getStar() const override { return this; }
};

/// KeywordStar (unpacking) expression (**what).
/// @example **kwargs
struct KeywordStarExpr : public Expr {
  ExprPtr what;

  explicit KeywordStarExpr(ExprPtr what);
  KeywordStarExpr(const KeywordStarExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Tuple expression ((items...)).
/// @example (1, a)
struct TupleExpr : public Expr {
  vector<ExprPtr> items;

  explicit TupleExpr(vector<ExprPtr> items);
  TupleExpr(const TupleExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const TupleExpr *getTuple() const override { return this; }
};

/// List expression ([items...]).
/// @example [1, 2]
struct ListExpr : public Expr {
  vector<ExprPtr> items;

  explicit ListExpr(vector<ExprPtr> items);
  ListExpr(const ListExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const ListExpr *getList() const override { return this; }
};

/// Set expression ({items...}).
/// @example {1, 2}
struct SetExpr : public Expr {
  vector<ExprPtr> items;

  explicit SetExpr(vector<ExprPtr> items);
  SetExpr(const SetExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Dictionary expression ({(key: value)...}).
/// @example {'s': 1, 't': 2}
struct DictExpr : public Expr {
  struct DictItem {
    ExprPtr key, value;

    DictItem clone() const;
  };
  vector<DictItem> items;

  explicit DictExpr(vector<DictItem> items);
  DictExpr(const DictExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Generator body node helper [for vars in gen (if conds)...].
/// @example for i in lst if a if b
struct GeneratorBody {
  ExprPtr vars;
  ExprPtr gen;
  vector<ExprPtr> conds;

  GeneratorBody clone() const;
};

/// Generator or comprehension expression [(expr (loops...))].
/// @example [i for i in j]
/// @example (f + 1 for j in k if j for f in j)
struct GeneratorExpr : public Expr {
  /// Generator kind: normal generator, list comprehension, set comprehension.
  enum GeneratorKind { Generator, ListGenerator, SetGenerator };

  GeneratorKind kind;
  ExprPtr expr;
  vector<GeneratorBody> loops;

  GeneratorExpr(GeneratorKind kind, ExprPtr expr, vector<GeneratorBody> loops);
  GeneratorExpr(const GeneratorExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Dictionary comprehension expression [{key: expr (loops...)}].
/// @example {i: j for i, j in z.items()}
struct DictGeneratorExpr : public Expr {
  ExprPtr key, expr;
  vector<GeneratorBody> loops;

  DictGeneratorExpr(ExprPtr key, ExprPtr expr, vector<GeneratorBody> loops);
  DictGeneratorExpr(const DictGeneratorExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Conditional expression [cond if ifexpr else elsexpr].
/// @example 1 if a else 2
struct IfExpr : public Expr {
  ExprPtr cond, ifexpr, elsexpr;

  IfExpr(ExprPtr cond, ExprPtr ifexpr, ExprPtr elsexpr);
  IfExpr(const IfExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const IfExpr *getIf() const override { return this; }
};

/// Unary expression [op expr].
/// @example -56
struct UnaryExpr : public Expr {
  string op;
  ExprPtr expr;

  UnaryExpr(string op, ExprPtr expr);
  UnaryExpr(const UnaryExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const UnaryExpr *getUnary() const override { return this; }
};

/// Binary expression [lexpr op rexpr].
/// @example 1 + 2
/// @example 3 or 4
struct BinaryExpr : public Expr {
  string op;
  ExprPtr lexpr, rexpr;

  /// True if an expression modifies lhs in-place (e.g. a += b).
  bool inPlace;

  BinaryExpr(ExprPtr lexpr, string op, ExprPtr rexpr, bool inPlace = false);
  BinaryExpr(const BinaryExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const BinaryExpr *getBinary() const override { return this; }
};

/// Chained binary expression.
/// @example 1 <= x <= 2
struct ChainBinaryExpr : public Expr {
  vector<std::pair<string, ExprPtr>> exprs;

  ChainBinaryExpr(vector<std::pair<string, ExprPtr>> exprs);
  ChainBinaryExpr(const ChainBinaryExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Pipe expression [(op expr)...].
/// op is either "" (only the first item), "|>" or "||>".
/// @example a |> b ||> c
struct PipeExpr : public Expr {
  struct Pipe {
    string op;
    ExprPtr expr;

    Pipe clone() const;
  };

  vector<Pipe> items;
  /// Output type of a "prefix" pipe ending at the index position.
  /// Example: for a |> b |> c, inTypes[1] is typeof(a |> b).
  vector<types::TypePtr> inTypes;

  explicit PipeExpr(vector<Pipe> items);
  PipeExpr(const PipeExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Index expression (expr[index]).
/// @example a[5]
struct IndexExpr : public Expr {
  ExprPtr expr, index;

  IndexExpr(ExprPtr expr, ExprPtr index);
  IndexExpr(const IndexExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const IndexExpr *getIndex() const override { return this; }
};

/// Call expression (expr((name=value)...)).
/// @example a(1, b=2)
struct CallExpr : public Expr {
  /// Each argument can have a name (e.g. foo(1, b=5))
  struct Arg {
    string name;
    ExprPtr value;

    Arg clone() const;
  };

  ExprPtr expr;
  vector<Arg> args;
  /// True if type-checker has processed and re-ordered args.
  bool ordered;

  CallExpr(ExprPtr expr, vector<Arg> args = {});
  /// Convenience constructors
  CallExpr(ExprPtr expr, vector<ExprPtr> args);
  template <typename... Ts>
  CallExpr(ExprPtr expr, ExprPtr arg, Ts... args)
      : CallExpr(expr, vector<ExprPtr>{arg, args...}) {}
  CallExpr(const CallExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const CallExpr *getCall() const override { return this; }
};

/// Dot (access) expression (expr.member).
/// @example a.b
struct DotExpr : public Expr {
  ExprPtr expr;
  string member;

  DotExpr(ExprPtr expr, string member);
  /// Convenience constructor.
  DotExpr(string left, string member);
  DotExpr(const DotExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const DotExpr *getDot() const override { return this; }
};

/// Slice expression (st:stop:step).
/// @example 1:10:3
/// @example s::-1
/// @example :::
struct SliceExpr : public Expr {
  /// Any of these can be nullptr to account for partial slices.
  ExprPtr start, stop, step;

  SliceExpr(ExprPtr start, ExprPtr stop, ExprPtr step);
  SliceExpr(const SliceExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Ellipsis expression.
/// @example ...
struct EllipsisExpr : public Expr {
  /// True if this is a target partial argument within a PipeExpr.
  /// If true, this node will be handled differently during the type-checking stage.
  bool isPipeArg;

  explicit EllipsisExpr(bool isPipeArg = false);
  EllipsisExpr(const EllipsisExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);

  const EllipsisExpr *getEllipsis() const override { return this; }
};

/// Lambda expression (lambda (vars)...: expr).
/// @example lambda a, b: a + b
struct LambdaExpr : public Expr {
  vector<string> vars;
  ExprPtr expr;

  LambdaExpr(vector<string> vars, ExprPtr expr);
  LambdaExpr(const LambdaExpr &);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Yield (send to generator) expression.
/// @example (yield)
struct YieldExpr : public Expr {
  YieldExpr();
  YieldExpr(const YieldExpr &expr) = default;

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Assignment (walrus) expression (var := expr).
/// @example a := 5 + 3
struct AssignExpr : public Expr {
  ExprPtr var, expr;

  AssignExpr(ExprPtr var, ExprPtr expr);
  AssignExpr(const AssignExpr &);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Range expression (start ... end).
/// Used only in match-case statements.
/// @example 1 ... 2
struct RangeExpr : public Expr {
  ExprPtr start, stop;

  RangeExpr(ExprPtr start, ExprPtr stop);
  RangeExpr(const RangeExpr &);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// The following nodes are created after the simplify stage.

/// Statement expression (stmts...; expr).
/// Statements are evaluated only if the expression is evaluated
/// (to support short-circuiting).
/// @example (a = 1; b = 2; a + b)
struct StmtExpr : public Expr {
  vector<shared_ptr<Stmt>> stmts;
  ExprPtr expr;

  StmtExpr(vector<shared_ptr<Stmt>> stmts, ExprPtr expr);
  StmtExpr(shared_ptr<Stmt> stmt, ExprPtr expr);
  StmtExpr(shared_ptr<Stmt> stmt, shared_ptr<Stmt> stmt2, ExprPtr expr);
  StmtExpr(const StmtExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);

  const StmtExpr *getStmtExpr() const override { return this; }
};

/// Pointer expression (__ptr__(expr)).
/// @example __ptr__(a)
struct PtrExpr : public Expr {
  ExprPtr expr;

  explicit PtrExpr(ExprPtr expr);
  PtrExpr(const PtrExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Static tuple indexing expression (expr[index]).
/// @example (1, 2, 3)[2]
struct TupleIndexExpr : Expr {
  ExprPtr expr;
  int index;

  TupleIndexExpr(ExprPtr expr, int index);
  TupleIndexExpr(const TupleIndexExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Static tuple indexing expression (expr[index]).
/// @example (1, 2, 3)[2]
struct InstantiateExpr : Expr {
  ExprPtr typeExpr;
  vector<ExprPtr> typeParams;

  InstantiateExpr(ExprPtr typeExpr, vector<ExprPtr> typeParams);
  /// Convenience constructor for a single type parameter.
  InstantiateExpr(ExprPtr typeExpr, ExprPtr typeParam);
  InstantiateExpr(const InstantiateExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

/// Stack allocation expression (__array__[type](expr)).
/// @example __array__[int](5)
struct StackAllocExpr : Expr {
  ExprPtr typeExpr, expr;

  StackAllocExpr(ExprPtr typeExpr, ExprPtr expr);
  StackAllocExpr(const StackAllocExpr &expr);

  string toString() const override;
  ACCEPT(ASTVisitor);
};

#undef ACCEPT

} // namespace ast
} // namespace seq
