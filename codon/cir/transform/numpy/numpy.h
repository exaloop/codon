// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/analyze/module/global_vars.h"
#include "codon/cir/analyze/module/side_effect.h"
#include "codon/cir/transform/pass.h"
#include "codon/cir/types/types.h"

#include <functional>
#include <memory>
#include <vector>

namespace codon {
namespace ir {
namespace transform {
namespace numpy {
extern const std::string FUSION_MODULE;

/// NumPy operator fusion pass.
class NumPyFusionPass : public OperatorPass {
private:
  /// Key of the reaching definition analysis
  std::string reachingDefKey;
  /// Key of the side effect analysis
  std::string sideEffectsKey;

public:
  static const std::string KEY;

  /// Constructs a NumPy fusion pass.
  /// @param reachingDefKey the reaching definition analysis' key
  /// @param sideEffectsKey side effect analysis' key
  NumPyFusionPass(const std::string &reachingDefKey, const std::string &sideEffectsKey)
      : OperatorPass(), reachingDefKey(reachingDefKey), sideEffectsKey(sideEffectsKey) {
  }

  std::string getKey() const override { return KEY; }
  void visit(BodiedFunc *f) override;
};

struct NumPyPrimitiveTypes {
  types::Type *none;
  types::Type *optnone;
  types::Type *bool_;
  types::Type *i8;
  types::Type *u8;
  types::Type *i16;
  types::Type *u16;
  types::Type *i32;
  types::Type *u32;
  types::Type *i64;
  types::Type *u64;
  types::Type *f16;
  types::Type *f32;
  types::Type *f64;
  types::Type *c64;
  types::Type *c128;

  explicit NumPyPrimitiveTypes(Module *M);
};

struct NumPyType {
  enum Type {
    NP_TYPE_NONE = -1,
    NP_TYPE_BOOL,
    NP_TYPE_I8,
    NP_TYPE_U8,
    NP_TYPE_I16,
    NP_TYPE_U16,
    NP_TYPE_I32,
    NP_TYPE_U32,
    NP_TYPE_I64,
    NP_TYPE_U64,
    NP_TYPE_F16,
    NP_TYPE_F32,
    NP_TYPE_F64,
    NP_TYPE_C64,
    NP_TYPE_C128,
    NP_TYPE_SCALAR_END, // separator value
    NP_TYPE_ARR_BOOL,
    NP_TYPE_ARR_I8,
    NP_TYPE_ARR_U8,
    NP_TYPE_ARR_I16,
    NP_TYPE_ARR_U16,
    NP_TYPE_ARR_I32,
    NP_TYPE_ARR_U32,
    NP_TYPE_ARR_I64,
    NP_TYPE_ARR_U64,
    NP_TYPE_ARR_F16,
    NP_TYPE_ARR_F32,
    NP_TYPE_ARR_F64,
    NP_TYPE_ARR_C64,
    NP_TYPE_ARR_C128,
  } dtype;
  int64_t ndim;

  NumPyType(Type dtype, int64_t ndim = 0);
  NumPyType();

  static NumPyType get(types::Type *t, NumPyPrimitiveTypes &T);

  types::Type *getIRBaseType(NumPyPrimitiveTypes &T) const;

  operator bool() const { return dtype != NP_TYPE_NONE; }
  bool isArray() const { return dtype > NP_TYPE_SCALAR_END; }

  friend std::ostream &operator<<(std::ostream &os, NumPyType const &type);

  std::string str() const;
};

struct NumPyExpr;

struct CodegenContext {
  Module *M;
  SeriesFlow *series;
  BodiedFunc *func;
  std::unordered_map<NumPyExpr *, Var *> vars;
  NumPyPrimitiveTypes &T;

  CodegenContext(Module *M, SeriesFlow *series, BodiedFunc *func,
                 NumPyPrimitiveTypes &T);
};

enum BroadcastInfo {
  UNKNOWN,
  YES,
  NO,
  MAYBE,
};

struct NumPyExpr {
  NumPyType type;
  Value *val;
  enum Op {
    NP_OP_NONE,
    NP_OP_POS,
    NP_OP_NEG,
    NP_OP_INVERT,
    NP_OP_ABS,
    NP_OP_TRANSPOSE,
    NP_OP_ADD,
    NP_OP_SUB,
    NP_OP_MUL,
    NP_OP_MATMUL,
    NP_OP_TRUE_DIV,
    NP_OP_FLOOR_DIV,
    NP_OP_MOD,
    NP_OP_FMOD,
    NP_OP_POW,
    NP_OP_LSHIFT,
    NP_OP_RSHIFT,
    NP_OP_AND,
    NP_OP_OR,
    NP_OP_XOR,
    NP_OP_LOGICAL_AND,
    NP_OP_LOGICAL_OR,
    NP_OP_LOGICAL_XOR,
    NP_OP_EQ,
    NP_OP_NE,
    NP_OP_LT,
    NP_OP_LE,
    NP_OP_GT,
    NP_OP_GE,
    NP_OP_MIN,
    NP_OP_MAX,
    NP_OP_FMIN,
    NP_OP_FMAX,
    NP_OP_SIN,
    NP_OP_COS,
    NP_OP_TAN,
    NP_OP_ARCSIN,
    NP_OP_ARCCOS,
    NP_OP_ARCTAN,
    NP_OP_ARCTAN2,
    NP_OP_HYPOT,
    NP_OP_SINH,
    NP_OP_COSH,
    NP_OP_TANH,
    NP_OP_ARCSINH,
    NP_OP_ARCCOSH,
    NP_OP_ARCTANH,
    NP_OP_CONJ,
    NP_OP_EXP,
    NP_OP_EXP2,
    NP_OP_LOG,
    NP_OP_LOG2,
    NP_OP_LOG10,
    NP_OP_EXPM1,
    NP_OP_LOG1P,
    NP_OP_SQRT,
    NP_OP_SQUARE,
    NP_OP_CBRT,
    NP_OP_LOGADDEXP,
    NP_OP_LOGADDEXP2,
    NP_OP_RECIPROCAL,
    NP_OP_RINT,
    NP_OP_FLOOR,
    NP_OP_CEIL,
    NP_OP_TRUNC,
    NP_OP_ISNAN,
    NP_OP_ISINF,
    NP_OP_ISFINITE,
    NP_OP_SIGN,
    NP_OP_SIGNBIT,
    NP_OP_COPYSIGN,
    NP_OP_SPACING,
    NP_OP_NEXTAFTER,
    NP_OP_DEG2RAD,
    NP_OP_RAD2DEG,
    NP_OP_HEAVISIDE,
  } op;
  std::unique_ptr<NumPyExpr> lhs;
  std::unique_ptr<NumPyExpr> rhs;
  bool freeable;

  NumPyExpr(NumPyType type, Value *val)
      : type(std::move(type)), val(val), op(NP_OP_NONE), lhs(), rhs(), freeable(false) {
  }
  NumPyExpr(NumPyType type, Value *val, NumPyExpr::Op op,
            std::unique_ptr<NumPyExpr> lhs)
      : type(std::move(type)), val(val), op(op), lhs(std::move(lhs)), rhs(),
        freeable(false) {}
  NumPyExpr(NumPyType type, Value *val, NumPyExpr::Op op,
            std::unique_ptr<NumPyExpr> lhs, std::unique_ptr<NumPyExpr> rhs)
      : type(std::move(type)), val(val), op(op), lhs(std::move(lhs)),
        rhs(std::move(rhs)), freeable(false) {}

  static std::unique_ptr<NumPyExpr>
  parse(Value *v, std::vector<std::pair<NumPyExpr *, Value *>> &leaves,
        NumPyPrimitiveTypes &T);

  void replace(NumPyExpr &e);
  bool haveVectorizedLoop() const;

  int64_t opcost() const;
  int64_t cost() const;

  std::string opstring() const;
  void dump(std::ostream &os, int level, int &leafId) const;
  friend std::ostream &operator<<(std::ostream &os, NumPyExpr const &expr);
  std::string str() const;

  bool isLeaf() const { return !lhs && !rhs; }

  int depth() const {
    return std::max(lhs ? lhs->depth() : 0, rhs ? rhs->depth() : 0) + 1;
  }

  int nodes() const { return (lhs ? lhs->nodes() : 0) + (rhs ? rhs->nodes() : 0) + 1; }

  void apply(std::function<void(NumPyExpr &)> f);

  Value *codegenBroadcasts(CodegenContext &C);

  Var *codegenFusedEval(CodegenContext &C);

  Var *codegenSequentialEval(CodegenContext &C);

  BroadcastInfo getBroadcastInfo();

  Value *codegenScalarExpr(CodegenContext &C,
                           const std::unordered_map<NumPyExpr *, Var *> &args,
                           const std::unordered_map<NumPyExpr *, unsigned> &scalarMap,
                           Var *scalars);
};

std::unique_ptr<NumPyExpr> parse(Value *v,
                                 std::vector<std::pair<NumPyExpr *, Value *>> &leaves,
                                 NumPyPrimitiveTypes &T);

struct NumPyOptimizationUnit {
  /// Original IR value being corresponding to expression
  Value *value;
  /// Function in which the value exists
  BodiedFunc *func;
  /// Root expression
  std::unique_ptr<NumPyExpr> expr;
  /// Leaves ordered by execution in original expression
  std::vector<std::pair<NumPyExpr *, Value *>> leaves;
  /// AssignInstr in which RHS is represented by this expression, or null if none
  AssignInstr *assign;

  bool optimize(NumPyPrimitiveTypes &T);
};

struct Forwarding {
  NumPyOptimizationUnit *dst;
  NumPyOptimizationUnit *src;
  Var *var;
  NumPyExpr *dstLeaf;
  int64_t dstId;
  int64_t srcId;
};

using ForwardingDAG =
    std::unordered_map<NumPyOptimizationUnit *, std::vector<Forwarding>>;

NumPyOptimizationUnit *doForwarding(ForwardingDAG &dag,
                                    std::vector<AssignInstr *> &assignsToDelete);

std::vector<ForwardingDAG> getForwardingDAGs(BodiedFunc *func,
                                             analyze::dataflow::RDInspector *rd,
                                             analyze::dataflow::CFGraph *cfg,
                                             analyze::module::SideEffectResult *se,
                                             std::vector<NumPyOptimizationUnit> &exprs);

} // namespace numpy
} // namespace transform
} // namespace ir
} // namespace codon
