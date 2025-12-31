// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "codon/parser/ast/node.h"
#include "codon/parser/ast/types.h"
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
  SERIALIZE(CLASS, BASE(Expr), ##__VA_ARGS__)

// Forward declarations
struct Stmt;

/**
 * A Seq AST expression.
 * Each AST expression is intended to be instantiated as a shared_ptr.
 */
struct Expr : public AcceptorExtend<Expr, ASTNode> {
  using base_type = Expr;

  Expr();
  Expr(const Expr &) = default;
  Expr(const Expr &, bool);

  /// Get a node type.
  /// @return Type pointer or a nullptr if a type is not set.
  types::Type *getType() const { return type.get(); }
  void setType(const types::TypePtr &t) { type = t; }
  types::ClassType *getClassType() const;
  bool isDone() const { return done; }
  void setDone() { done = true; }
  Expr *getOrigExpr() const { return origExpr; }
  void setOrigExpr(Expr *orig) { origExpr = orig; }

  static const char NodeId;
  SERIALIZE(Expr, BASE(ASTNode), /*type,*/ done, origExpr);

  Expr *operator<<(types::Type *t);

protected:
  /// Add a type to S-expression string.
  std::string wrapType(const std::string &sexpr) const;

private:
  /// Type of the expression. nullptr by default.
  types::TypePtr type;
  /// Flag that indicates if all types in an expression are inferred (i.e. if a
  /// type-checking procedure was successful).
  bool done;
  /// Original (pre-transformation) expression
  Expr *origExpr;
};

/// Function signature parameter helper node (name: type = defaultValue).
struct Param : public codon::SrcObject {
  std::string name;
  Expr *type;
  Expr *defaultValue;
  enum {
    Value,
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
  bool isValue() const { return status == Value; }
  bool isGeneric() const { return status == Generic; }
  bool isHiddenGeneric() const { return status == HiddenGeneric; }
  std::pair<int, std::string> getNameWithStars() const;

  SERIALIZE(Param, name, type, defaultValue);
  Param clone(bool) const;
  std::string toString(int) const;
};

/// None expression.
/// @li None
struct NoneExpr : public AcceptorExtend<NoneExpr, Expr> {
  NoneExpr();
  NoneExpr(const NoneExpr &, bool);

  ACCEPT(NoneExpr, ASTVisitor);
};

/// Bool expression (value).
/// @li True
struct BoolExpr : public AcceptorExtend<BoolExpr, Expr> {
  explicit BoolExpr(bool value = false);
  BoolExpr(const BoolExpr &, bool);

  bool getValue() const;

  ACCEPT(BoolExpr, ASTVisitor, value);

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

  ACCEPT(IntExpr, ASTVisitor, value, suffix, intValue);

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

  ACCEPT(FloatExpr, ASTVisitor, value, suffix, floatValue);

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
  struct FormatSpec {
    std::string text;
    std::string conversion;
    std::string spec;

    SERIALIZE(FormatSpec, text, conversion, spec);
  };

  // Vector of {value, prefix} strings.
  struct String : public SrcObject {
    std::string value;
    std::string prefix;
    Expr *expr;
    FormatSpec format;

    explicit String(std::string v, std::string p = "", Expr *e = nullptr)
        : value(std::move(v)), prefix(std::move(p)), expr(e), format() {}

    SERIALIZE(String, value, prefix, expr, format);
  };

  explicit StringExpr(std::string value = "", std::string prefix = "");
  explicit StringExpr(std::vector<String> strings);
  StringExpr(const StringExpr &, bool);

  std::string getValue() const;
  bool isSimple() const;

  ACCEPT(StringExpr, ASTVisitor, strings);

private:
  std::vector<String> strings;

  auto begin() { return strings.begin(); }
  auto end() { return strings.end(); }

  friend class ScopingVisitor;
};

/// Identifier expression (value).
struct IdExpr : public AcceptorExtend<IdExpr, Expr> {
  explicit IdExpr(std::string value = "");
  IdExpr(const IdExpr &, bool);

  std::string getValue() const { return value; }

  ACCEPT(IdExpr, ASTVisitor, value);

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

  ACCEPT(StarExpr, ASTVisitor, expr);

private:
  Expr *expr;
};

/// KeywordStar (unpacking) expression (**what).
/// @li **kwargs
struct KeywordStarExpr : public AcceptorExtend<KeywordStarExpr, Expr> {
  explicit KeywordStarExpr(Expr *what = nullptr);
  KeywordStarExpr(const KeywordStarExpr &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(KeywordStarExpr, ASTVisitor, expr);

private:
  Expr *expr;
};

/// Tuple expression ((items...)).
/// @li (1, a)
struct TupleExpr : public AcceptorExtend<TupleExpr, Expr>, Items<Expr *> {
  explicit TupleExpr(std::vector<Expr *> items = {});
  TupleExpr(const TupleExpr &, bool);

  ACCEPT(TupleExpr, ASTVisitor, items);
};

/// List expression ([items...]).
/// @li [1, 2]
struct ListExpr : public AcceptorExtend<ListExpr, Expr>, Items<Expr *> {
  explicit ListExpr(std::vector<Expr *> items = {});
  ListExpr(const ListExpr &, bool);

  ACCEPT(ListExpr, ASTVisitor, items);
};

/// Set expression ({items...}).
/// @li {1, 2}
struct SetExpr : public AcceptorExtend<SetExpr, Expr>, Items<Expr *> {
  explicit SetExpr(std::vector<Expr *> items = {});
  SetExpr(const SetExpr &, bool);

  ACCEPT(SetExpr, ASTVisitor, items);
};

/// Dictionary expression ({(key: value)...}).
/// Each (key, value) pair is stored as a TupleExpr.
/// @li {'s': 1, 't': 2}
struct DictExpr : public AcceptorExtend<DictExpr, Expr>, Items<Expr *> {
  explicit DictExpr(std::vector<Expr *> items = {});
  DictExpr(const DictExpr &, bool);

  ACCEPT(DictExpr, ASTVisitor, items);
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

  ACCEPT(GeneratorExpr, ASTVisitor, kind, loops);

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
  explicit IfExpr(Expr *cond = nullptr, Expr *ifexpr = nullptr,
                  Expr *elsexpr = nullptr);
  IfExpr(const IfExpr &, bool);

  Expr *getCond() const { return cond; }
  Expr *getIf() const { return ifexpr; }
  Expr *getElse() const { return elsexpr; }

  ACCEPT(IfExpr, ASTVisitor, cond, ifexpr, elsexpr);

private:
  Expr *cond, *ifexpr, *elsexpr;
};

/// Unary expression [op expr].
/// @li -56
struct UnaryExpr : public AcceptorExtend<UnaryExpr, Expr> {
  explicit UnaryExpr(std::string op = "", Expr *expr = nullptr);
  UnaryExpr(const UnaryExpr &, bool);

  std::string getOp() const { return op; }
  Expr *getExpr() const { return expr; }

  ACCEPT(UnaryExpr, ASTVisitor, op, expr);

private:
  std::string op;
  Expr *expr;
};

/// Binary expression [lexpr op rexpr].
/// @li 1 + 2
/// @li 3 or 4
struct BinaryExpr : public AcceptorExtend<BinaryExpr, Expr> {
  explicit BinaryExpr(Expr *lexpr = nullptr, std::string op = "", Expr *rexpr = nullptr,
                      bool inPlace = false);
  BinaryExpr(const BinaryExpr &, bool);

  std::string getOp() const { return op; }
  void setOp(const std::string &o) { op = o; }
  Expr *getLhs() const { return lexpr; }
  Expr *getRhs() const { return rexpr; }
  bool isInPlace() const { return inPlace; }

  ACCEPT(BinaryExpr, ASTVisitor, op, lexpr, rexpr);

private:
  std::string op;
  Expr *lexpr, *rexpr;

  /// True if an expression modifies lhs in-place (e.g. a += b).
  bool inPlace;
};

/// Chained binary expression.
/// @li 1 <= x <= 2
struct ChainBinaryExpr : public AcceptorExtend<ChainBinaryExpr, Expr> {
  explicit ChainBinaryExpr(std::vector<std::pair<std::string, Expr *>> exprs = {});
  ChainBinaryExpr(const ChainBinaryExpr &, bool);

  ACCEPT(ChainBinaryExpr, ASTVisitor, exprs);

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

  ACCEPT(PipeExpr, ASTVisitor, items);

private:
  /// Output type of a "prefix" pipe ending at the index position.
  /// Example: for a |> b |> c, inTypes[1] is typeof(a |> b).
  std::vector<types::TypePtr> inTypes;
};

/// Index expression (expr[index]).
/// @li a[5]
struct IndexExpr : public AcceptorExtend<IndexExpr, Expr> {
  explicit IndexExpr(Expr *expr = nullptr, Expr *index = nullptr);
  IndexExpr(const IndexExpr &, bool);

  Expr *getExpr() const { return expr; }
  Expr *getIndex() const { return index; }

  ACCEPT(IndexExpr, ASTVisitor, expr, index);

private:
  Expr *expr, *index;
};

struct CallArg : public codon::SrcObject {
  std::string name;
  Expr *value;

  explicit CallArg(std::string name = "", Expr *value = nullptr);
  CallArg(const SrcInfo &info, std::string name, Expr *value);
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
  explicit CallExpr(Expr *expr = nullptr, std::vector<CallArg> args = {});
  /// Convenience constructors
  CallExpr(Expr *expr, const std::vector<Expr *> &args);
  template <typename... Ts>
  CallExpr(Expr *expr, Expr *arg, Ts... args)
      : CallExpr(expr, std::vector<Expr *>{arg, args...}) {}
  CallExpr(const CallExpr &, bool);

  Expr *getExpr() const { return expr; }
  bool isOrdered() const { return ordered; }
  bool isPartial() const { return partial; }

  ACCEPT(CallExpr, ASTVisitor, expr, items, ordered, partial);

private:
  Expr *expr;
  /// True if type-checker has processed and re-ordered args.
  bool ordered;
  /// True if the call is partial
  bool partial = false;
};

/// Dot (access) expression (expr.member).
/// @li a.b
struct DotExpr : public AcceptorExtend<DotExpr, Expr> {
  DotExpr() : expr(nullptr), member() {}
  DotExpr(Expr *expr, std::string member);
  DotExpr(const DotExpr &, bool);

  Expr *getExpr() const { return expr; }
  std::string getMember() const { return member; }

  ACCEPT(DotExpr, ASTVisitor, expr, member);

private:
  Expr *expr;
  std::string member;
};

/// Slice expression (st:stop:step).
/// @li 1:10:3
/// @li s::-1
/// @li :::
struct SliceExpr : public AcceptorExtend<SliceExpr, Expr> {
  explicit SliceExpr(Expr *start = nullptr, Expr *stop = nullptr, Expr *step = nullptr);
  SliceExpr(const SliceExpr &, bool);

  Expr *getStart() const { return start; }
  Expr *getStop() const { return stop; }
  Expr *getStep() const { return step; }

  ACCEPT(SliceExpr, ASTVisitor, start, stop, step);

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
  bool isStandalone() const { return mode == STANDALONE; }
  bool isPipe() const { return mode == PIPE; }
  bool isPartial() const { return mode == PARTIAL; }

  ACCEPT(EllipsisExpr, ASTVisitor, mode);

private:
  EllipsisType mode;

  friend struct PipeExpr;
};

/// Lambda expression (lambda (vars)...: expr).
/// @li lambda a, b: a + b
struct LambdaExpr : public AcceptorExtend<LambdaExpr, Expr>, Items<Param> {
  explicit LambdaExpr(std::vector<Param> vars = {}, Expr *expr = nullptr);
  LambdaExpr(const LambdaExpr &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(LambdaExpr, ASTVisitor, expr, items);

private:
  Expr *expr;
};

/// Yield (send to generator) expression.
/// @li (yield)
struct YieldExpr : public AcceptorExtend<YieldExpr, Expr> {
  YieldExpr();
  YieldExpr(const YieldExpr &, bool);

  ACCEPT(YieldExpr, ASTVisitor);
};

/// Await expression (await expr).
/// @li await a
struct AwaitExpr : public AcceptorExtend<AwaitExpr, Expr> {
  explicit AwaitExpr(Expr *expr);
  AwaitExpr(const AwaitExpr &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(AwaitExpr, ASTVisitor, expr);

private:
  Expr *expr;
};

/// Assignment (walrus) expression (var := expr).
/// @li a := 5 + 3
struct AssignExpr : public AcceptorExtend<AssignExpr, Expr> {
  explicit AssignExpr(Expr *var = nullptr, Expr *expr = nullptr);
  AssignExpr(const AssignExpr &, bool);

  Expr *getVar() const { return var; }
  Expr *getExpr() const { return expr; }

  ACCEPT(AssignExpr, ASTVisitor, var, expr);

private:
  Expr *var, *expr;
};

/// Range expression (start ... end).
/// Used only in match-case statements.
/// @li 1 ... 2
struct RangeExpr : public AcceptorExtend<RangeExpr, Expr> {
  explicit RangeExpr(Expr *start = nullptr, Expr *stop = nullptr);
  RangeExpr(const RangeExpr &, bool);

  Expr *getStart() const { return start; }
  Expr *getStop() const { return stop; }

  ACCEPT(RangeExpr, ASTVisitor, start, stop);

private:
  Expr *start, *stop;
};

/// The following nodes are created during typechecking.

/// Statement expression (stmts...; expr).
/// Statements are evaluated only if the expression is evaluated
/// (to support short-circuiting).
/// @li (a = 1; b = 2; a + b)
struct StmtExpr : public AcceptorExtend<StmtExpr, Expr>, Items<Stmt *> {
  explicit StmtExpr(Stmt *stmt = nullptr, Expr *expr = nullptr);
  StmtExpr(std::vector<Stmt *> stmts, Expr *expr);
  StmtExpr(Stmt *stmt, Stmt *stmt2, Expr *expr);
  StmtExpr(const StmtExpr &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(StmtExpr, ASTVisitor, expr, items);

private:
  Expr *expr;
};

/// Static tuple indexing expression (expr[index]).
/// @li (1, 2, 3)[2]
struct InstantiateExpr : public AcceptorExtend<InstantiateExpr, Expr>, Items<Expr *> {
  explicit InstantiateExpr(Expr *expr = nullptr, std::vector<Expr *> typeParams = {});
  /// Convenience constructor for a single type parameter.
  InstantiateExpr(Expr *expr, Expr *typeParam);
  InstantiateExpr(const InstantiateExpr &, bool);

  Expr *getExpr() const { return expr; }

  ACCEPT(InstantiateExpr, ASTVisitor, expr, items);

private:
  Expr *expr;
};

#undef ACCEPT

bool isId(Expr *e, const std::string &s);
types::LiteralKind getStaticGeneric(Expr *e);

} // namespace codon::ast

template <>
struct fmt::formatter<codon::ast::CallArg> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::CallArg &p, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "({}{})",
                          p.name.empty() ? "" : fmt::format("{} = ", p.name),
                          p.value ? p.value->toString(0) : "-");
  }
};

template <>
struct fmt::formatter<codon::ast::Param> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const codon::ast::Param &p, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", p.toString(0));
  }
};

namespace tser {
using Archive = BinaryArchive;
static void operator<<(codon::ast::Expr *t, Archive &a) {
  using S = codon::PolymorphicSerializer<Archive, codon::ast::Expr>;
  a.save(t != nullptr);
  if (t) {
    void *typ = const_cast<void *>(t->dynamicNodeId());
    auto key = S::_serializers[typ];
    a.save(key);
    S::save(key, t, a);
  }
}
static void operator>>(codon::ast::Expr *&t, Archive &a) {
  using S = codon::PolymorphicSerializer<Archive, codon::ast::Expr>;
  bool empty = a.load<bool>();
  if (!empty) {
    const auto key = a.load<std::string>();
    S::load(key, t, a);
  } else {
    t = nullptr;
  }
}
} // namespace tser
