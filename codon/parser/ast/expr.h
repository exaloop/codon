// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/cir/base.h"
#include "codon/parser/ast/types.h"
#include "codon/parser/common.h"
#include "codon/util/serialize.h"

namespace codon::ast {

const int INDENT_SIZE = 2;

struct ASTVisitor;

struct Attr {
  // Function attributes
  const static std::string Module;
  const static std::string ParentClass;
  const static std::string Bindings;
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
  const static std::string ClassDeduce;
  const static std::string ClassNoTuple;
  // Standard library attributes
  const static std::string Test;
  const static std::string Overload;
  const static std::string Export;
  // Expression-related attributes
  const static std::string ClassMagic;
  const static std::string ExprSequenceItem;
  const static std::string ExprStarSequenceItem;
  const static std::string ExprList;
  const static std::string ExprSet;
  const static std::string ExprDict;
  const static std::string ExprPartial;
  const static std::string ExprDominated;
  const static std::string ExprStarArgument;
  const static std::string ExprKwStarArgument;
  const static std::string ExprOrderedCall;
  const static std::string ExprExternVar;
  const static std::string ExprDominatedUndefCheck;
  const static std::string ExprDominatedUsed;
};
struct ASTNode : public ir::Node {
  static const char NodeId;
  using ir::Node::Node;

  /// See LLVM documentation.
  static const void *nodeId() { return &NodeId; }
  const void *dynamicNodeId() const override { return &NodeId; }
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const override {
    return other == nodeId() || ir::Node::isConvertible(other);
  }

  Cache *cache;

  ASTNode() = default;
  ASTNode(const ASTNode &);
  virtual ~ASTNode() = default;

  /// Convert a node to an S-expression.
  virtual std::string toString(int) const = 0;
  virtual std::string toString() const { return toString(-1); }

  /// Deep copy a node.
  virtual ASTNode *clone(bool clean) const = 0;
  ASTNode *clone() const { return clone(false); }

  /// Accept an AST visitor.
  virtual void accept(ASTVisitor &visitor) = 0;

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const ASTNode &expr) {
    return out << expr.toString();
  }

  void setAttribute(const std::string &key, std::unique_ptr<ir::Attribute> value) {
    attributes[key] = std::move(value);
  }
  void setAttribute(const std::string &key, const std::string &value) {
    attributes[key] = std::make_unique<ir::StringValueAttribute>(value);
  }
  void setAttribute(const std::string &key) {
    attributes[key] = std::make_unique<ir::Attribute>();
  }

  inline decltype(auto) members() {
    int a = 0;
    return std::tie(a);
  }
};
template <class... TA> void E(error::Error e, ASTNode *o, const TA &...args) {
  E(e, o->getSrcInfo(), args...);
}
template <class... TA> void E(error::Error e, const ASTNode &o, const TA &...args) {
  E(e, o.getSrcInfo(), args...);
}
using ir::cast;

template <typename Derived, typename Parent> class AcceptorExtend : public Parent {
public:
  using Parent::Parent;

  /// See LLVM documentation.
  static const void *nodeId() { return &Derived::NodeId; }
  const void *dynamicNodeId() const override { return &Derived::NodeId; }
  /// See LLVM documentation.
  virtual bool isConvertible(const void *other) const override {
    return other == nodeId() || Parent::isConvertible(other);
  }
};
} // namespace codon::ast

template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<std::is_base_of<codon::ast::ASTNode, T>::value, char>>
    : fmt::ostream_formatter {};

namespace codon::ast {

#define ACCEPT(X)                                                                      \
  using AcceptorExtend::clone;                                                         \
  using AcceptorExtend::accept;                                                        \
  ASTNode *clone(bool c) const override;                                               \
  void accept(X &visitor) override;                                                    \
  std::string toString(int) const override;                                            \
  friend class TypecheckVisitor;                                                       \
  template <typename TE, typename TS> friend struct CallbackASTVisitor;                \
  friend struct ReplacingCallbackASTVisitor

// Forward declarations
struct Stmt;

/**
 * A Seq AST expression.
 * Each AST expression is intended to be instantiated as a shared_ptr.
 */
struct Expr : public AcceptorExtend<Expr, ASTNode> {
  using base_type = Expr;
  static const char NodeId;

  // private:
  /// Type of the expression. nullptr by default.
  types::TypePtr type;
  /// Flag that indicates if all types in an expression are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;

  /// Original (pre-transformation) expression
  Expr *origExpr;

public:
  Expr();
  Expr(const Expr &);
  Expr(const Expr &, bool);

  /// Validate a node. Throw ParseASTException if a node is not valid.
  void validate() const;
  /// Get a node type.
  /// @return Type pointer or a nullptr if a type is not set.
  types::TypePtr getType() const;
  types::ClassTypePtr getClassType() const;
  /// Set a node type.
  void setType(types::TypePtr type);

  /// Allow pretty-printing to C++ streams.
  friend std::ostream &operator<<(std::ostream &out, const Expr &expr) {
    return out << expr.toString();
  }

  bool isDone() const { return done; }
  void setDone() { done = true; }

  SERIALIZE(Expr, BASE(ASTNode), done);

protected:
  /// Add a type to S-expression string.
  std::string wrapType(const std::string &sexpr) const;
};

template <class T> struct Items {
  Items(std::vector<T> items) : items(std::move(items)) {}
  const T &operator[](int i) const { return items[i]; }
  T &operator[](int i) { return items[i]; }
  auto begin() { return items.begin(); }
  auto end() { return items.end(); }
  auto begin() const { return items.begin(); }
  auto end() const { return items.end(); }
  auto size() const { return items.size(); }
  bool empty() const { return items.empty(); }

protected:
  std::vector<T> items;
};

/// Function signature parameter helper node (name: type = defaultValue).
struct Param : public codon::SrcObject {
  std::string name;
  Expr *type;
  Expr *defaultValue;
  enum {
    Normal,
    Generic,
    HiddenGeneric
  } status; // 1 for normal generic, 2 for hidden generic

  explicit Param(std::string name = "", Expr *type = nullptr,
                 Expr *defaultValue = nullptr, int generic = 0);
  explicit Param(const SrcInfo &info, std::string name = "", Expr *type = nullptr,
                 Expr *defaultValue = nullptr, int generic = 0);

  std::string getName() const { return name; }
  Expr *getType() const { return type; }
  Expr *getDefault() const { return defaultValue; }
  bool isGeneric() const { return status == Generic; }
  bool isHiddenGeneric() const { return status == HiddenGeneric; }

  SERIALIZE(Param, name, type, defaultValue);
  Param clone(bool) const;
  std::string toString(int) const;
};

/// None expression.
/// @li None
struct NoneExpr : public AcceptorExtend<NoneExpr, Expr> {
  NoneExpr();
  NoneExpr(const NoneExpr &, bool);

  static const char NodeId;
  SERIALIZE(NoneExpr, BASE(Expr));
  ACCEPT(ASTVisitor);
};

/// Bool expression (value).
/// @li True
struct BoolExpr : public AcceptorExtend<BoolExpr, Expr> {
  explicit BoolExpr(bool value = false);
  BoolExpr(const BoolExpr &, bool);

  bool getValue() const;

  static const char NodeId;
  SERIALIZE(BoolExpr, BASE(Expr), value);
  ACCEPT(ASTVisitor);

private:
  bool value;
};

/// Int expression (value.suffix).
/// @li 12
/// @li 13u
/// @li 000_010b
struct IntExpr : public AcceptorExtend<IntExpr, Expr> {
  explicit IntExpr(int64_t intValue = 0);
  explicit IntExpr(const std::string &value, std::string suffix = "");
  IntExpr(const IntExpr &, bool);

  bool hasStoredValue() const;
  int64_t getValue() const;
  std::pair<std::string, std::string> getRawData() const;

  static const char NodeId;
  SERIALIZE(IntExpr, BASE(Expr), value, suffix, intValue);
  ACCEPT(ASTVisitor);

private:
  /// Expression value is stored as a string that is parsed during typechecking.
  std::string value;
  /// Number suffix (e.g. "u" for "123u").
  std::string suffix;
  /// Parsed value and sign for "normal" 64-bit integers.
  std::optional<int64_t> intValue;
};

/// Float expression (value.suffix).
/// @li 12.1
/// @li 13.15z
/// @li e-12
struct FloatExpr : public AcceptorExtend<FloatExpr, Expr> {
  explicit FloatExpr(double floatValue = 0.0);
  explicit FloatExpr(const std::string &value, std::string suffix = "");
  FloatExpr(const FloatExpr &, bool);

  bool hasStoredValue() const;
  double getValue() const;
  std::pair<std::string, std::string> getRawData() const;

  static const char NodeId;
  SERIALIZE(FloatExpr, BASE(Expr), value, suffix, floatValue);
  ACCEPT(ASTVisitor);

private:
  /// Expression value is stored as a string that is parsed during typechecking.
  std::string value;
  /// Number suffix (e.g. "u" for "123u").
  std::string suffix;
  /// Parsed value for 64-bit floats.
  std::optional<double> floatValue;
};

/// String expression (prefix"value").
/// @li s'ACGT'
/// @li "fff"
struct StringExpr : public AcceptorExtend<StringExpr, Expr> {
  // Vector of {value, prefix} strings.
  struct String : public SrcObject {
    std::string value;
    std::string prefix;
    Expr *expr;

    String(std::string v, std::string p = "", Expr *e = nullptr)
        : value(std::move(v)), prefix(std::move(p)), expr(e) {}

    SERIALIZE(String, value, prefix, expr);
  };

  explicit StringExpr(std::string value = "", std::string prefix = "");
  explicit StringExpr(std::vector<String> strings);
  StringExpr(const StringExpr &, bool);

  std::string getValue() const;
  bool isSimple() const;

  static const char NodeId;
  SERIALIZE(StringExpr, BASE(Expr), strings);
  ACCEPT(ASTVisitor);

private:
  std::vector<String> strings;

  void unpack();
  std::vector<String> unpackFString(const std::string &) const;
  auto begin() { return strings.begin(); }
  auto end() { return strings.end(); }

  friend class ScopingVisitor;
};

/// Identifier expression (value).
struct IdExpr : public AcceptorExtend<IdExpr, Expr> {
  explicit IdExpr(std::string value = "");
  IdExpr(const IdExpr &, bool);

  std::string getValue() const { return value; }

  static const char NodeId;
  SERIALIZE(IdExpr, BASE(Expr), value);
  ACCEPT(ASTVisitor);

private:
  std::string value;

  void setValue(const std::string &s) { value = s; }

  friend class ScopingVisitor;
};

/// Star (unpacking) expression (*what).
/// @li *args
struct StarExpr : public AcceptorExtend<StarExpr, Expr> {
  explicit StarExpr(Expr *what = nullptr);
  StarExpr(const StarExpr &, bool);

  Expr *getExpr() const { return expr; }

  static const char NodeId;
  SERIALIZE(StarExpr, BASE(Expr), expr);
  ACCEPT(ASTVisitor);

private:
  Expr *expr;
};

/// KeywordStar (unpacking) expression (**what).
/// @li **kwargs
struct KeywordStarExpr : public AcceptorExtend<KeywordStarExpr, Expr> {
  explicit KeywordStarExpr(Expr *what = nullptr);
  KeywordStarExpr(const KeywordStarExpr &, bool);

  Expr *getExpr() const { return expr; }

  static const char NodeId;
  SERIALIZE(KeywordStarExpr, BASE(Expr), expr);
  ACCEPT(ASTVisitor);

private:
  Expr *expr;
};

/// Tuple expression ((items...)).
/// @li (1, a)
struct TupleExpr : public AcceptorExtend<TupleExpr, Expr>, Items<Expr *> {
  explicit TupleExpr(std::vector<Expr *> items = {});
  TupleExpr(const TupleExpr &, bool);

  static const char NodeId;
  SERIALIZE(TupleExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// List expression ([items...]).
/// @li [1, 2]
struct ListExpr : public AcceptorExtend<ListExpr, Expr>, Items<Expr *> {
  explicit ListExpr(std::vector<Expr *> items = {});
  ListExpr(const ListExpr &, bool);

  static const char NodeId;
  SERIALIZE(ListExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Set expression ({items...}).
/// @li {1, 2}
struct SetExpr : public AcceptorExtend<SetExpr, Expr>, Items<Expr *> {
  explicit SetExpr(std::vector<Expr *> items = {});
  SetExpr(const SetExpr &, bool);

  static const char NodeId;
  SERIALIZE(SetExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Dictionary expression ({(key: value)...}).
/// Each (key, value) pair is stored as a TupleExpr.
/// @li {'s': 1, 't': 2}
struct DictExpr : public AcceptorExtend<DictExpr, Expr>, Items<Expr *> {
  explicit DictExpr(std::vector<Expr *> items = {});
  DictExpr(const DictExpr &, bool);

  static const char NodeId;
  SERIALIZE(DictExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Generator or comprehension expression [(expr (loops...))].
/// @li [i for i in j]
/// @li (f + 1 for j in k if j for f in j)
struct GeneratorExpr : public AcceptorExtend<GeneratorExpr, Expr> {
  /// Generator kind: normal generator, list comprehension, set comprehension.
  enum GeneratorKind {
    Generator,
    ListGenerator,
    SetGenerator,
    TupleGenerator,
    DictGenerator
  };

  GeneratorExpr() : kind(Generator), loops(nullptr) {}
  GeneratorExpr(Cache *cache, GeneratorKind kind, Expr *expr,
                std::vector<Stmt *> loops);
  GeneratorExpr(Cache *cache, Expr *key, Expr *expr, std::vector<Stmt *> loops);
  GeneratorExpr(const GeneratorExpr &, bool);

  int loopCount() const;
  Stmt *getFinalSuite() const;
  Expr *getFinalExpr();

  static const char NodeId;
  SERIALIZE(GeneratorExpr, BASE(Expr), kind, loops);
  ACCEPT(ASTVisitor);

private:
  GeneratorKind kind;
  Stmt *loops;

  Stmt **getFinalStmt();
  void setFinalExpr(Expr *);
  void setFinalStmt(Stmt *);
  void formCompleteStmt(const std::vector<Stmt *> &);

  friend class TranslateVisitor;
};

/// Conditional expression [cond if ifexpr else elsexpr].
/// @li 1 if a else 2
struct IfExpr : public AcceptorExtend<IfExpr, Expr> {
  IfExpr(Expr *cond = nullptr, Expr *ifexpr = nullptr, Expr *elsexpr = nullptr);
  IfExpr(const IfExpr &, bool);

  Expr *getCond() const { return cond; }
  Expr *getIf() const { return ifexpr; }
  Expr *getElse() const { return elsexpr; }

  static const char NodeId;
  SERIALIZE(IfExpr, BASE(Expr), cond, ifexpr, elsexpr);
  ACCEPT(ASTVisitor);

private:
  Expr *cond, *ifexpr, *elsexpr;
};

/// Unary expression [op expr].
/// @li -56
struct UnaryExpr : public AcceptorExtend<UnaryExpr, Expr> {
  UnaryExpr(std::string op = "", Expr *expr = nullptr);
  UnaryExpr(const UnaryExpr &, bool);

  std::string getOp() const { return op; }
  Expr *getExpr() const { return expr; }

  static const char NodeId;
  SERIALIZE(UnaryExpr, BASE(Expr), op, expr);
  ACCEPT(ASTVisitor);

private:
  std::string op;
  Expr *expr;
};

/// Binary expression [lexpr op rexpr].
/// @li 1 + 2
/// @li 3 or 4
struct BinaryExpr : public AcceptorExtend<BinaryExpr, Expr> {
  BinaryExpr(Expr *lexpr = nullptr, std::string op = "", Expr *rexpr = nullptr,
             bool inPlace = false);
  BinaryExpr(const BinaryExpr &, bool);

  std::string getOp() const { return op; }
  Expr *getLhs() const { return lexpr; }
  Expr *getRhs() const { return rexpr; }
  bool isInPlace() const { return inPlace; }

  static const char NodeId;
  SERIALIZE(BinaryExpr, BASE(Expr), op, lexpr, rexpr);
  ACCEPT(ASTVisitor);

private:
  std::string op;
  Expr *lexpr, *rexpr;

  /// True if an expression modifies lhs in-place (e.g. a += b).
  bool inPlace;
};

/// Chained binary expression.
/// @li 1 <= x <= 2
struct ChainBinaryExpr : public AcceptorExtend<ChainBinaryExpr, Expr> {
  ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs = {});
  ChainBinaryExpr(const ChainBinaryExpr &, bool);

  static const char NodeId;
  SERIALIZE(ChainBinaryExpr, BASE(Expr), exprs);
  ACCEPT(ASTVisitor);

private:
  std::vector<std::pair<std::string, Expr *>> exprs;
};

struct Pipe {
  std::string op;
  Expr *expr;

  SERIALIZE(Pipe, op, expr);
  Pipe clone(bool) const;
};

/// Pipe expression [(op expr)...].
/// op is either "" (only the first item), "|>" or "||>".
/// @li a |> b ||> c
struct PipeExpr : public AcceptorExtend<PipeExpr, Expr>, Items<Pipe> {
  explicit PipeExpr(std::vector<Pipe> items = {});
  PipeExpr(const PipeExpr &, bool);

  static const char NodeId;
  SERIALIZE(PipeExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);

private:
  /// Output type of a "prefix" pipe ending at the index position.
  /// Example: for a |> b |> c, inTypes[1] is typeof(a |> b).
  std::vector<types::TypePtr> inTypes;

  void validate() const;
};

/// Index expression (expr[index]).
/// @li a[5]
struct IndexExpr : public AcceptorExtend<IndexExpr, Expr> {
  IndexExpr(Expr *expr = nullptr, Expr *index = nullptr);
  IndexExpr(const IndexExpr &, bool);

  Expr *getExpr() const { return expr; }
  Expr *getIndex() const { return index; }

  static const char NodeId;
  SERIALIZE(IndexExpr, BASE(Expr), expr, index);
  ACCEPT(ASTVisitor);

private:
  Expr *expr, *index;
};

struct CallArg : public codon::SrcObject {
  std::string name;
  Expr *value;

  CallArg(const std::string &name = "", Expr *value = nullptr);
  CallArg(const SrcInfo &info, const std::string &name, Expr *value);
  CallArg(Expr *value);

  std::string getName() const { return name; }
  Expr *getExpr() const { return value; }
  operator Expr *() const { return value; }

  SERIALIZE(CallArg, name, value);
  CallArg clone(bool) const;
};

/// Call expression (expr((name=value)...)).
/// @li a(1, b=2)
struct CallExpr : public AcceptorExtend<CallExpr, Expr>, Items<CallArg> {
  /// Each argument can have a name (e.g. foo(1, b=5))
  CallExpr(Expr *expr = nullptr, std::vector<CallArg> args = {});
  /// Convenience constructors
  CallExpr(Expr *expr, std::vector<Expr *> args);
  template <typename... Ts>
  CallExpr(Expr *expr, Expr *arg, Ts... args)
      : CallExpr(expr, std::vector<Expr *>{arg, args...}) {}
  CallExpr(const CallExpr &, bool);

  Expr *getExpr() const { return expr; }
  bool isOrdered() const { return ordered; }
  bool isPartial() const { return partial; }

  static const char NodeId;
  SERIALIZE(CallExpr, BASE(Expr), expr, items, ordered, partial);
  ACCEPT(ASTVisitor);

private:
  Expr *expr;
  /// True if type-checker has processed and re-ordered args.
  bool ordered;
  /// True if the call is partial
  bool partial = false;

  void validate() const;
};

/// Dot (access) expression (expr.member).
/// @li a.b
struct DotExpr : public AcceptorExtend<DotExpr, Expr> {
  DotExpr() : expr(nullptr), member() {}
  DotExpr(Expr *expr, std::string member);
  DotExpr(const DotExpr &, bool);

  Expr *getExpr() const { return expr; }
  std::string getMember() const { return member; }

  static const char NodeId;
  SERIALIZE(DotExpr, BASE(Expr), expr, member);
  ACCEPT(ASTVisitor);

private:
  Expr *expr;
  std::string member;
};

/// Slice expression (st:stop:step).
/// @li 1:10:3
/// @li s::-1
/// @li :::
struct SliceExpr : public AcceptorExtend<SliceExpr, Expr> {
  SliceExpr(Expr *start = nullptr, Expr *stop = nullptr, Expr *step = nullptr);
  SliceExpr(const SliceExpr &, bool);

  Expr *getStart() const { return start; }
  Expr *getStop() const { return stop; }
  Expr *getStep() const { return step; }

  static const char NodeId;
  SERIALIZE(SliceExpr, BASE(Expr), start, stop, step);
  ACCEPT(ASTVisitor);

private:
  /// Any of these can be nullptr to account for partial slices.
  Expr *start, *stop, *step;
};

/// Ellipsis expression.
/// @li ...
struct EllipsisExpr : public AcceptorExtend<EllipsisExpr, Expr> {
  /// True if this is a target partial argument within a PipeExpr.
  /// If true, this node will be handled differently during the type-checking stage.
  enum EllipsisType { PIPE, PARTIAL, STANDALONE };

  explicit EllipsisExpr(EllipsisType mode = STANDALONE);
  EllipsisExpr(const EllipsisExpr &, bool);

  EllipsisType getMode() const { return mode; }

  static const char NodeId;
  SERIALIZE(EllipsisExpr, BASE(Expr), mode);
  ACCEPT(ASTVisitor);

private:
  EllipsisType mode;

  friend class PipeExpr;
};

/// Lambda expression (lambda (vars)...: expr).
/// @li lambda a, b: a + b
struct LambdaExpr : public AcceptorExtend<LambdaExpr, Expr> {
  LambdaExpr(std::vector<std::string> vars = {}, Expr *expr = nullptr);
  LambdaExpr(const LambdaExpr &, bool);

  Expr *getExpr() const { return expr; }
  auto begin() { return vars.begin(); }
  auto end() { return vars.end(); }
  auto size() const { return vars.size(); }

  static const char NodeId;
  SERIALIZE(LambdaExpr, BASE(Expr), vars, expr);
  ACCEPT(ASTVisitor);

private:
  std::vector<std::string> vars;
  Expr *expr;
};

/// Yield (send to generator) expression.
/// @li (yield)
struct YieldExpr : public AcceptorExtend<YieldExpr, Expr> {
  YieldExpr();
  YieldExpr(const YieldExpr &, bool);

  static const char NodeId;
  SERIALIZE(YieldExpr, BASE(Expr));
  ACCEPT(ASTVisitor);
};

/// Assignment (walrus) expression (var := expr).
/// @li a := 5 + 3
struct AssignExpr : public AcceptorExtend<AssignExpr, Expr> {
  AssignExpr(Expr *var = nullptr, Expr *expr = nullptr);
  AssignExpr(const AssignExpr &, bool);

  Expr *getVar() const { return var; }
  Expr *getExpr() const { return expr; }

  static const char NodeId;
  SERIALIZE(AssignExpr, BASE(Expr), var, expr);
  ACCEPT(ASTVisitor);

private:
  Expr *var, *expr;
};

/// Range expression (start ... end).
/// Used only in match-case statements.
/// @li 1 ... 2
struct RangeExpr : public AcceptorExtend<RangeExpr, Expr> {
  RangeExpr(Expr *start = nullptr, Expr *stop = nullptr);
  RangeExpr(const RangeExpr &, bool);

  Expr *getStart() const { return start; }
  Expr *getStop() const { return stop; }

  static const char NodeId;
  SERIALIZE(RangeExpr, BASE(Expr), start, stop);
  ACCEPT(ASTVisitor);

private:
  Expr *start, *stop;
};

/// The following nodes are created during typechecking.

/// Statement expression (stmts...; expr).
/// Statements are evaluated only if the expression is evaluated
/// (to support short-circuiting).
/// @li (a = 1; b = 2; a + b)
struct StmtExpr : public AcceptorExtend<StmtExpr, Expr> {
  StmtExpr(Stmt *stmt = nullptr, Expr *expr = nullptr);
  StmtExpr(std::vector<Stmt *> stmts, Expr *expr);
  StmtExpr(Stmt *stmt, Stmt *stmt2, Expr *expr);
  StmtExpr(const StmtExpr &, bool);

  Expr *getExpr() const { return expr; }
  auto begin() { return stmts.begin(); }
  auto end() { return stmts.end(); }
  auto size() const { return stmts.size(); }

  static const char NodeId;
  SERIALIZE(StmtExpr, BASE(Expr), expr);
  ACCEPT(ASTVisitor);

private:
  std::vector<Stmt *> stmts;
  Expr *expr;
};

/// Static tuple indexing expression (expr[index]).
/// @li (1, 2, 3)[2]
struct InstantiateExpr : public AcceptorExtend<InstantiateExpr, Expr>, Items<Expr*> {
  InstantiateExpr(Expr *expr = nullptr, std::vector<Expr *> typeParams = {});
  /// Convenience constructor for a single type parameter.
  InstantiateExpr(Expr *expr, Expr *typeParam);
  InstantiateExpr(const InstantiateExpr &, bool);

  Expr *getExpr() const { return expr; }

  static const char NodeId;
  SERIALIZE(InstantiateExpr, BASE(Expr), expr, items);
  ACCEPT(ASTVisitor);

private:
  Expr *expr;
};

#undef ACCEPT

bool isId(Expr *e, const std::string &s);
char getStaticGeneric(Expr *e);

} // namespace codon::ast

template <>
struct fmt::formatter<codon::ast::CallArg> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::CallArg &p,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "({}{})",
                          p.name.empty() ? "" : fmt::format("{} = ", p.name),
                          p.value ? p.value->toString(0) : "-");
  }
};

template <>
struct fmt::formatter<codon::ast::Param> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::Param &p,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", p.toString(0));
  }
};

namespace tser {
using Archive = BinaryArchive;
using S = codon::PolymorphicSerializer<Archive, codon::ast::Expr>;
static void operator<<(codon::ast::Expr *t, Archive &a) {
  a.save(t != nullptr);
  if (t) {
    auto typ = t->dynamicNodeId();
    auto key = S::_serializers[(void *)typ];
    a.save(key);
    S::save(key, t, a);
  }
}
static void operator>>(codon::ast::Expr *&t, Archive &a) {
  bool empty = a.load<bool>();
  if (!empty) {
    std::string key = a.load<std::string>();
    S::load(key, t, a);
  } else {
    t = nullptr;
  }
}
} // namespace tser
