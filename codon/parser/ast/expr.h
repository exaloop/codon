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
  void accept(X &visitor) override

// Forward declarations
struct BinaryExpr;
struct CallExpr;
struct DotExpr;
struct EllipsisExpr;
struct IdExpr;
struct IfExpr;
struct IndexExpr;
struct IntExpr;
struct InstantiateExpr;
struct ListExpr;
struct NoneExpr;
struct StarExpr;
struct KeywordStarExpr;
struct StmtExpr;
struct StringExpr;
struct TupleExpr;
struct UnaryExpr;
struct Stmt;
struct SuiteStmt;

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

  /// Convenience virtual functions to avoid unnecessary dynamic_cast calls.
  virtual bool isId(const std::string &val) const { return false; }
  virtual BinaryExpr *getBinary() { return nullptr; }
  virtual CallExpr *getCall() { return nullptr; }
  virtual DotExpr *getDot() { return nullptr; }
  virtual EllipsisExpr *getEllipsis() { return nullptr; }
  virtual IdExpr *getId() { return nullptr; }
  virtual IfExpr *getIf() { return nullptr; }
  virtual IndexExpr *getIndex() { return nullptr; }
  virtual InstantiateExpr *getInstantiate() { return nullptr; }
  virtual IntExpr *getInt() { return nullptr; }
  virtual ListExpr *getList() { return nullptr; }
  virtual NoneExpr *getNone() { return nullptr; }
  virtual StarExpr *getStar() { return nullptr; }
  virtual KeywordStarExpr *getKwStar() { return nullptr; }
  virtual StmtExpr *getStmtExpr() { return nullptr; }
  virtual StringExpr *getString() { return nullptr; }
  virtual TupleExpr *getTuple() { return nullptr; }
  virtual UnaryExpr *getUnary() { return nullptr; }

  bool isDone() const { return done; }
  void setDone() { done = true; }

  /// @return Type name for IdExprs or instantiations.
  std::string getTypeName();

  SERIALIZE(Expr, BASE(ASTNode), done);

protected:
  /// Add a type to S-expression string.
  std::string wrapType(const std::string &sexpr) const;
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

  std::string toString(int) const;
  Param clone(bool) const;

  SERIALIZE(Param, name, type, defaultValue);
};

/// None expression.
/// @li None
struct NoneExpr : public AcceptorExtend<NoneExpr, Expr> {
  static const char NodeId;

  NoneExpr();
  NoneExpr(const NoneExpr &, bool);

  std::string toString(int) const override;

  using AcceptorExtend::getNone;
  NoneExpr *getNone() override { return this; }

  SERIALIZE(NoneExpr, BASE(Expr));
  ACCEPT(ASTVisitor);
};

/// Bool expression (value).
/// @li True
struct BoolExpr : public AcceptorExtend<BoolExpr, Expr> {
  static const char NodeId;

  explicit BoolExpr(bool value = false);
  BoolExpr(const BoolExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(BoolExpr, BASE(Expr), value);
  ACCEPT(ASTVisitor);

  bool getValue() const;

private:
  bool value;
};

/// Int expression (value.suffix).
/// @li 12
/// @li 13u
/// @li 000_010b
struct IntExpr : public AcceptorExtend<IntExpr, Expr> {
  static const char NodeId;

  explicit IntExpr(int64_t intValue = 0);
  explicit IntExpr(const std::string &value, std::string suffix = "");
  IntExpr(const IntExpr &, bool);

  std::string toString(int) const override;

  IntExpr *getInt() override { return this; }

  SERIALIZE(IntExpr, BASE(Expr), value, suffix, intValue);
  ACCEPT(ASTVisitor);

  std::pair<std::string, std::string> getRawData() const;
  bool hasStoredValue() const;
  int64_t getValue() const;

private:
  /// Expression value is stored as a string that is parsed during typechecking.
  std::string value;
  /// Number suffix (e.g. "u" for "123u").
  std::string suffix;

  /// Parsed value and sign for "normal" 64-bit integers.
  std::unique_ptr<int64_t> intValue;
};

/// Float expression (value.suffix).
/// @li 12.1
/// @li 13.15z
/// @li e-12
struct FloatExpr : public AcceptorExtend<FloatExpr, Expr> {
  static const char NodeId;

  explicit FloatExpr(double floatValue = 0.0);
  explicit FloatExpr(const std::string &value, std::string suffix = "");
  FloatExpr(const FloatExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(FloatExpr, BASE(Expr), value, suffix, floatValue);
  ACCEPT(ASTVisitor);

  std::pair<std::string, std::string> getRawData() const;
  bool hasStoredValue() const;
  double getValue() const;

private:
  /// Expression value is stored as a string that is parsed during typechecking.
  std::string value;
  /// Number suffix (e.g. "u" for "123u").
  std::string suffix;
  /// Parsed value for 64-bit floats.
  std::unique_ptr<double> floatValue;
};

/// String expression (prefix"value").
/// @li s'ACGT'
/// @li "fff"
struct StringExpr : public AcceptorExtend<StringExpr, Expr> {
  static const char NodeId;

  // Vector of {value, prefix} strings.
  struct String : public SrcObject {
    std::string value;
    std::string prefix;
    Expr *expr;

    String(std::string v, std::string p = "", Expr *e = nullptr)
        : value(std::move(v)), prefix(std::move(p)), expr(e) {}

    SERIALIZE(String, value, prefix, expr);
  };
  std::vector<String> strings;

  explicit StringExpr(std::string value = "", std::string prefix = "");
  explicit StringExpr(std::vector<String> strings);
  StringExpr(const StringExpr &, bool);

  std::string toString(int) const override;

  StringExpr *getString() override { return this; }
  std::string getValue() const;

  void unpack();
  std::vector<String> unpackFString(const std::string &) const;

  SERIALIZE(StringExpr, BASE(Expr), strings);
  ACCEPT(ASTVisitor);
};

/// Identifier expression (value).
struct IdExpr : public AcceptorExtend<IdExpr, Expr> {
  static const char NodeId;

  std::string value;

  explicit IdExpr(std::string value);
  IdExpr(const IdExpr &, bool);

  std::string toString(int) const override;

  bool isId(const std::string &val) const override { return this->value == val; }
  IdExpr *getId() override { return this; }

  SERIALIZE(IdExpr, BASE(Expr), value);
  ACCEPT(ASTVisitor);
};

/// Star (unpacking) expression (*what).
/// @li *args
struct StarExpr : public AcceptorExtend<StarExpr, Expr> {
  static const char NodeId;

  Expr *what;

  explicit StarExpr(Expr *what);
  StarExpr(const StarExpr &, bool);

  std::string toString(int) const override;

  StarExpr *getStar() override { return this; }

  SERIALIZE(StarExpr, BASE(Expr), what);
  ACCEPT(ASTVisitor);
};

/// KeywordStar (unpacking) expression (**what).
/// @li **kwargs
struct KeywordStarExpr : public AcceptorExtend<KeywordStarExpr, Expr> {
  static const char NodeId;

  Expr *what;

  explicit KeywordStarExpr(Expr *what);
  KeywordStarExpr(const KeywordStarExpr &, bool);

  std::string toString(int) const override;

  KeywordStarExpr *getKwStar() override { return this; }

  SERIALIZE(KeywordStarExpr, BASE(Expr), what);
  ACCEPT(ASTVisitor);
};

/// Tuple expression ((items...)).
/// @li (1, a)
struct TupleExpr : public AcceptorExtend<TupleExpr, Expr> {
  static const char NodeId;

  std::vector<Expr *> items;

  explicit TupleExpr(std::vector<Expr *> items = {});
  TupleExpr(const TupleExpr &, bool);

  std::string toString(int) const override;

  TupleExpr *getTuple() override { return this; }

  SERIALIZE(TupleExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// List expression ([items...]).
/// @li [1, 2]
struct ListExpr : public AcceptorExtend<ListExpr, Expr> {
  static const char NodeId;

  std::vector<Expr *> items;

  explicit ListExpr(std::vector<Expr *> items = {});
  ListExpr(const ListExpr &, bool);

  std::string toString(int) const override;

  ListExpr *getList() override { return this; }

  SERIALIZE(ListExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Set expression ({items...}).
/// @li {1, 2}
struct SetExpr : public AcceptorExtend<SetExpr, Expr> {
  static const char NodeId;

  std::vector<Expr *> items;

  explicit SetExpr(std::vector<Expr *> items = {});
  SetExpr(const SetExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(SetExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Dictionary expression ({(key: value)...}).
/// Each (key, value) pair is stored as a TupleExpr.
/// @li {'s': 1, 't': 2}
struct DictExpr : public AcceptorExtend<DictExpr, Expr> {
  static const char NodeId;

  std::vector<Expr *> items;

  explicit DictExpr(std::vector<Expr *> items = {});
  DictExpr(const DictExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(DictExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Generator or comprehension expression [(expr (loops...))].
/// @li [i for i in j]
/// @li (f + 1 for j in k if j for f in j)
struct GeneratorExpr : public AcceptorExtend<GeneratorExpr, Expr> {
  static const char NodeId;

  /// Generator kind: normal generator, list comprehension, set comprehension.
  enum GeneratorKind {
    Generator,
    ListGenerator,
    SetGenerator,
    TupleGenerator,
    DictGenerator
  };

  GeneratorKind kind;
  Stmt *loops;

  GeneratorExpr(Cache *cache, GeneratorKind kind, Expr *expr,
                std::vector<Stmt *> loops);
  GeneratorExpr(Cache *cache, Expr *key, Expr *expr, std::vector<Stmt *> loops);
  GeneratorExpr(const GeneratorExpr &, bool);

  std::string toString(int) const override;

  int loopCount() const;
  Stmt *getFinalSuite() const;
  Expr *getFinalExpr();
  void setFinalExpr(Expr *);
  void setFinalStmt(Stmt *);

  SERIALIZE(GeneratorExpr, BASE(Expr), kind, loops);
  ACCEPT(ASTVisitor);

private:
  Stmt **getFinalStmt();
  void formCompleteStmt(const std::vector<Stmt *> &);
};

/// Conditional expression [cond if ifexpr else elsexpr].
/// @li 1 if a else 2
struct IfExpr : public AcceptorExtend<IfExpr, Expr> {
  static const char NodeId;

  Expr *cond, *ifexpr, *elsexpr;

  IfExpr(Expr *cond, Expr *ifexpr, Expr *elsexpr);
  IfExpr(const IfExpr &, bool);

  std::string toString(int) const override;

  IfExpr *getIf() override { return this; }

  SERIALIZE(IfExpr, BASE(Expr), cond, ifexpr, elsexpr);
  ACCEPT(ASTVisitor);
};

/// Unary expression [op expr].
/// @li -56
struct UnaryExpr : public AcceptorExtend<UnaryExpr, Expr> {
  static const char NodeId;

  std::string op;
  Expr *expr;

  UnaryExpr(std::string op, Expr *expr);
  UnaryExpr(const UnaryExpr &, bool);

  std::string toString(int) const override;

  UnaryExpr *getUnary() override { return this; }

  SERIALIZE(UnaryExpr, BASE(Expr), op, expr);
  ACCEPT(ASTVisitor);
};

/// Binary expression [lexpr op rexpr].
/// @li 1 + 2
/// @li 3 or 4
struct BinaryExpr : public AcceptorExtend<BinaryExpr, Expr> {
  static const char NodeId;

  std::string op;
  Expr *lexpr, *rexpr;

  /// True if an expression modifies lhs in-place (e.g. a += b).
  bool inPlace;

  BinaryExpr(Expr *lexpr, std::string op, Expr *rexpr, bool inPlace = false);
  BinaryExpr(const BinaryExpr &, bool);

  std::string toString(int) const override;

  BinaryExpr *getBinary() override { return this; }

  SERIALIZE(BinaryExpr, BASE(Expr), op, lexpr, rexpr);
  ACCEPT(ASTVisitor);
};

/// Chained binary expression.
/// @li 1 <= x <= 2
struct ChainBinaryExpr : public AcceptorExtend<ChainBinaryExpr, Expr> {
  static const char NodeId;

  std::vector<std::pair<std::string, Expr *>> exprs;

  ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs);
  ChainBinaryExpr(const ChainBinaryExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(ChainBinaryExpr, BASE(Expr), exprs);
  ACCEPT(ASTVisitor);
};

/// Pipe expression [(op expr)...].
/// op is either "" (only the first item), "|>" or "||>".
/// @li a |> b ||> c
struct PipeExpr : public AcceptorExtend<PipeExpr, Expr> {
  static const char NodeId;

  struct Pipe {
    std::string op;
    Expr *expr;

    SERIALIZE(Pipe, op, expr);
    Pipe clone(bool) const;
  };

  std::vector<Pipe> items;
  /// Output type of a "prefix" pipe ending at the index position.
  /// Example: for a |> b |> c, inTypes[1] is typeof(a |> b).
  std::vector<types::TypePtr> inTypes;

  explicit PipeExpr(std::vector<Pipe> items);
  PipeExpr(const PipeExpr &, bool);

  std::string toString(int) const override;
  void validate() const;

  SERIALIZE(PipeExpr, BASE(Expr), items);
  ACCEPT(ASTVisitor);
};

/// Index expression (expr[index]).
/// @li a[5]
struct IndexExpr : public AcceptorExtend<IndexExpr, Expr> {
  static const char NodeId;

  Expr *expr, *index;

  IndexExpr(Expr *expr, Expr *index);
  IndexExpr(const IndexExpr &, bool);

  std::string toString(int) const override;

  IndexExpr *getIndex() override { return this; }

  SERIALIZE(IndexExpr, BASE(Expr), expr, index);
  ACCEPT(ASTVisitor);
};

/// Call expression (expr((name=value)...)).
/// @li a(1, b=2)
struct CallExpr : public AcceptorExtend<CallExpr, Expr> {
  static const char NodeId;

  /// Each argument can have a name (e.g. foo(1, b=5))
  struct Arg : public codon::SrcObject {
    std::string name;
    Expr *value;


    Arg(const SrcInfo &info, const std::string &name, Expr *value);
    Arg(const std::string &name, Expr *value);
    Arg(Expr *value);

    SERIALIZE(Arg, name, value);
    Arg clone(bool) const;
  };

  Expr *expr;
  std::vector<Arg> args;
  /// True if type-checker has processed and re-ordered args.
  bool ordered;
  /// True if the call is partial
  bool partial = false;

  CallExpr(Expr *expr, std::vector<Arg> args = {});
  /// Convenience constructors
  CallExpr(Expr *expr, std::vector<Expr *> args);
  template <typename... Ts>
  CallExpr(Expr *expr, Expr *arg, Ts... args)
      : CallExpr(expr, std::vector<Expr *>{arg, args...}) {}
  CallExpr(const CallExpr &, bool);

  void validate() const;
  std::string toString(int) const override;

  CallExpr *getCall() override { return this; }

  SERIALIZE(CallExpr, BASE(Expr), expr, args, ordered, partial);
  ACCEPT(ASTVisitor);
};

/// Dot (access) expression (expr.member).
/// @li a.b
struct DotExpr : public AcceptorExtend<DotExpr, Expr> {
  static const char NodeId;

  Expr *expr;
  std::string member;

  DotExpr(Expr *expr, std::string member);
  DotExpr(const DotExpr &, bool);

  std::string toString(int) const override;

  DotExpr *getDot() override { return this; }

  SERIALIZE(DotExpr, BASE(Expr), expr, member);
  ACCEPT(ASTVisitor);
};

/// Slice expression (st:stop:step).
/// @li 1:10:3
/// @li s::-1
/// @li :::
struct SliceExpr : public AcceptorExtend<SliceExpr, Expr> {
  static const char NodeId;

  /// Any of these can be nullptr to account for partial slices.
  Expr *start, *stop, *step;

  SliceExpr(Expr *start, Expr *stop, Expr *step);
  SliceExpr(const SliceExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(SliceExpr, BASE(Expr), start, stop, step);
  ACCEPT(ASTVisitor);
};

/// Ellipsis expression.
/// @li ...
struct EllipsisExpr : public AcceptorExtend<EllipsisExpr, Expr> {
  static const char NodeId;

  /// True if this is a target partial argument within a PipeExpr.
  /// If true, this node will be handled differently during the type-checking stage.
  enum EllipsisType { PIPE, PARTIAL, STANDALONE } mode;

  explicit EllipsisExpr(EllipsisType mode = STANDALONE);
  EllipsisExpr(const EllipsisExpr &, bool);

  std::string toString(int) const override;

  EllipsisExpr *getEllipsis() override { return this; }

  SERIALIZE(EllipsisExpr, BASE(Expr), mode);
  ACCEPT(ASTVisitor);
};

/// Lambda expression (lambda (vars)...: expr).
/// @li lambda a, b: a + b
struct LambdaExpr : public AcceptorExtend<LambdaExpr, Expr> {
  static const char NodeId;

  std::vector<std::string> vars;
  Expr *expr;

  LambdaExpr(std::vector<std::string> vars, Expr *expr);
  LambdaExpr(const LambdaExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(LambdaExpr, BASE(Expr), vars, expr);
  ACCEPT(ASTVisitor);
};

/// Yield (send to generator) expression.
/// @li (yield)
struct YieldExpr : public AcceptorExtend<YieldExpr, Expr> {
  static const char NodeId;

  YieldExpr();
  YieldExpr(const YieldExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(YieldExpr, BASE(Expr));
  ACCEPT(ASTVisitor);
};

/// Assignment (walrus) expression (var := expr).
/// @li a := 5 + 3
struct AssignExpr : public AcceptorExtend<AssignExpr, Expr> {
  static const char NodeId;

  Expr *var, *expr;

  AssignExpr(Expr *var, Expr *expr);
  AssignExpr(const AssignExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(AssignExpr, BASE(Expr), var, expr);
  ACCEPT(ASTVisitor);
};

/// Range expression (start ... end).
/// Used only in match-case statements.
/// @li 1 ... 2
struct RangeExpr : public AcceptorExtend<RangeExpr, Expr> {
  static const char NodeId;

  Expr *start, *stop;

  RangeExpr(Expr *start, Expr *stop);
  RangeExpr(const RangeExpr &, bool);

  std::string toString(int) const override;

  SERIALIZE(RangeExpr, BASE(Expr), start, stop);
  ACCEPT(ASTVisitor);
};

/// The following nodes are created during typechecking.

/// Statement expression (stmts...; expr).
/// Statements are evaluated only if the expression is evaluated
/// (to support short-circuiting).
/// @li (a = 1; b = 2; a + b)
struct StmtExpr : public AcceptorExtend<StmtExpr, Expr> {
  static const char NodeId;

  std::vector<Stmt *> stmts;
  Expr *expr;

  StmtExpr(std::vector<Stmt *> stmts, Expr *expr);
  StmtExpr(Stmt *stmt, Expr *expr);
  StmtExpr(Stmt *stmt, Stmt *stmt2, Expr *expr);
  StmtExpr(const StmtExpr &, bool);

  std::string toString(int) const override;

  StmtExpr *getStmtExpr() override { return this; }

  SERIALIZE(StmtExpr, BASE(Expr), expr);
  ACCEPT(ASTVisitor);
};

/// Static tuple indexing expression (expr[index]).
/// @li (1, 2, 3)[2]
struct InstantiateExpr : public AcceptorExtend<InstantiateExpr, Expr> {
  static const char NodeId;

  Expr *typeExpr;
  std::vector<Expr *> typeParams;

  InstantiateExpr(Expr *typeExpr, std::vector<Expr *> typeParams);
  /// Convenience constructor for a single type parameter.
  InstantiateExpr(Expr *typeExpr, Expr *typeParam);
  InstantiateExpr(const InstantiateExpr &, bool);

  std::string toString(int) const override;

  InstantiateExpr *getInstantiate() override { return this; }

  SERIALIZE(InstantiateExpr, BASE(Expr), typeExpr, typeParams);
  ACCEPT(ASTVisitor);
};

#undef ACCEPT

char getStaticGeneric(Expr *e);

} // namespace codon::ast

template <>
struct fmt::formatter<codon::ast::CallExpr::Arg> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::CallExpr::Arg &p,
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
