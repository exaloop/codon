// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "const_fold.h"

#include <cmath>
#include <utility>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

#define BINOP(o) [](auto x, auto y) -> auto { return x o y; }
#define UNOP(o) [](auto x) -> auto { return o x; }

namespace codon {
namespace ir {
namespace transform {
namespace folding {
namespace {
auto pyDivmod(int64_t self, int64_t other) {
  auto d = self / other;
  auto m = self - d * other;
  if (m && ((other ^ m) < 0)) {
    m += other;
    d -= 1;
  }
  return std::make_pair(d, m);
}

template <typename Func, typename Out> class IntFloatBinaryRule : public RewriteRule {
private:
  Func f;
  std::string magic;
  types::Type *out;
  bool excludeRHSZero;

public:
  IntFloatBinaryRule(Func f, std::string magic, types::Type *out,
                     bool excludeRHSZero = false)
      : f(std::move(f)), magic(std::move(magic)), out(out),
        excludeRHSZero(excludeRHSZero) {}

  virtual ~IntFloatBinaryRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, 2, /*output=*/nullptr, /*method=*/true))
      return;

    auto *leftConst = cast<Const>(v->front());
    auto *rightConst = cast<Const>(v->back());

    if (!leftConst || !rightConst)
      return;

    auto *M = v->getModule();
    if (isA<FloatConst>(leftConst) && isA<IntConst>(rightConst)) {
      auto left = cast<FloatConst>(leftConst)->getVal();
      auto right = cast<IntConst>(rightConst)->getVal();
      if (excludeRHSZero && right == 0)
        return;
      return setResult(M->template N<TemplatedConst<Out>>(v->getSrcInfo(),
                                                          f(left, (double)right), out));
    } else if (isA<IntConst>(leftConst) && isA<FloatConst>(rightConst)) {
      auto left = cast<IntConst>(leftConst)->getVal();
      auto right = cast<FloatConst>(rightConst)->getVal();
      if (excludeRHSZero && right == 0.0)
        return;
      return setResult(M->template N<TemplatedConst<Out>>(v->getSrcInfo(),
                                                          f((double)left, right), out));
    }
  }
};

/// Binary rule that requires two constants.
template <typename ConstantType, typename Func, typename OutputType = ConstantType>
class DoubleConstantBinaryRuleExcludeRHSZero
    : public DoubleConstantBinaryRule<ConstantType, Func, OutputType> {
public:
  DoubleConstantBinaryRuleExcludeRHSZero(Func f, std::string magic,
                                         types::Type *inputType,
                                         types::Type *resultType)
      : DoubleConstantBinaryRule<ConstantType, Func, OutputType>(f, magic, inputType,
                                                                 resultType) {}

  virtual ~DoubleConstantBinaryRuleExcludeRHSZero() noexcept = default;

  void visit(CallInstr *v) override {
    if (v->numArgs() == 2) {
      auto *rightConst = cast<TemplatedConst<ConstantType>>(v->back());
      if (rightConst && rightConst->getVal() == ConstantType())
        return;
    }
    DoubleConstantBinaryRule<ConstantType, Func, OutputType>::visit(v);
  }
};

auto id_val(Module *m) {
  return [=](Value *v) -> Value * {
    util::CloneVisitor cv(m);
    return cv.clone(v);
  };
}

int64_t int_pow(int64_t base, int64_t exp) {
  if (exp < 0)
    return 0;
  int64_t result = 1;
  while (true) {
    if (exp & 1) {
      result *= base;
    }
    exp = exp >> 1;
    if (!exp)
      break;
    base = base * base;
  }
  return result;
}

template <typename From, typename To> To convert(From x) { return To(x); }

template <typename... Args> auto intSingleRule(Module *m, Args &&...args) {
  return std::make_unique<SingleConstantCommutativeRule<int64_t>>(
      std::forward<Args>(args)..., m->getIntType());
}

auto intNoOp(Module *m, std::string magic) {
  return std::make_unique<NoOpRule>(std::move(magic), m->getIntType());
}

auto intDoubleApplyNoOp(Module *m, std::string magic) {
  return std::make_unique<DoubleApplicationNoOpRule>(std::move(magic), m->getIntType());
}

template <typename Func> auto intToIntBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRule<int64_t, Func, int64_t>>(
      std::move(f), std::move(magic), m->getIntType(), m->getIntType());
}

template <typename Func>
auto intToIntBinaryNoZeroRHS(Module *m, Func f, std::string magic) {
  return std::make_unique<
      DoubleConstantBinaryRuleExcludeRHSZero<int64_t, Func, int64_t>>(
      std::move(f), std::move(magic), m->getIntType(), m->getIntType());
}

template <typename Func> auto intToBoolBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRule<int64_t, Func, bool>>(
      std::move(f), std::move(magic), m->getIntType(), m->getBoolType());
}

template <typename Func> auto boolToBoolBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRule<bool, Func, bool>>(
      std::move(f), std::move(magic), m->getBoolType(), m->getBoolType());
}

template <typename Func> auto floatToFloatBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRule<double, Func, double>>(
      std::move(f), std::move(magic), m->getFloatType(), m->getFloatType());
}

template <typename Func>
auto floatToFloatBinaryNoZeroRHS(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRuleExcludeRHSZero<double, Func, double>>(
      std::move(f), std::move(magic), m->getFloatType(), m->getFloatType());
}

template <typename Func> auto floatToBoolBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<DoubleConstantBinaryRule<double, Func, bool>>(
      std::move(f), std::move(magic), m->getFloatType(), m->getBoolType());
}

template <typename Func>
auto intFloatToFloatBinary(Module *m, Func f, std::string magic,
                           bool excludeRHSZero = false) {
  return std::make_unique<IntFloatBinaryRule<Func, double>>(
      std::move(f), std::move(magic), m->getFloatType(), excludeRHSZero);
}

template <typename Func>
auto intFloatToBoolBinary(Module *m, Func f, std::string magic) {
  return std::make_unique<IntFloatBinaryRule<Func, bool>>(
      std::move(f), std::move(magic), m->getBoolType());
}

template <typename Func> auto intToIntUnary(Module *m, Func f, std::string magic) {
  return std::make_unique<SingleConstantUnaryRule<int64_t, Func>>(
      std::move(f), std::move(magic), m->getIntType(), m->getIntType());
}

template <typename Func> auto floatToFloatUnary(Module *m, Func f, std::string magic) {
  return std::make_unique<SingleConstantUnaryRule<double, Func>>(
      std::move(f), std::move(magic), m->getFloatType(), m->getFloatType());
}

template <typename Func> auto boolToBoolUnary(Module *m, Func f, std::string magic) {
  return std::make_unique<SingleConstantUnaryRule<bool, Func>>(
      std::move(f), std::move(magic), m->getBoolType(), m->getBoolType());
}

auto identityConvert(Module *m, std::string magic, types::Type *type) {
  return std::make_unique<UnaryRule<decltype(id_val(m))>>(id_val(m), std::move(magic),
                                                          type);
}

template <typename From, typename To>
auto typeConvert(Module *m, std::string magic, types::Type *fromType,
                 types::Type *toType) {
  return std::make_unique<
      SingleConstantUnaryRule<From, std::function<decltype(convert<From, To>)>>>(
      convert<From, To>, std::move(magic), fromType, toType);
}
} // namespace

const std::string FoldingPass::KEY = "core-folding-const-fold";

void FoldingPass::run(Module *m) {
  registerStandardRules(m);
  Rewriter::reset();
  OperatorPass::run(m);
}

void FoldingPass::handle(CallInstr *v) { rewrite(v); }

void FoldingPass::registerStandardRules(Module *m) {
  // binary, single constant, int->int
  using Kind = SingleConstantCommutativeRule<int64_t>::Kind;
  registerRule("int-multiply-by-zero",
               intSingleRule(m, 0, 0, Module::MUL_MAGIC_NAME, Kind::COMMUTATIVE));
  registerRule(
      "int-multiply-by-one",
      intSingleRule(m, 1, id_val(m), Module::MUL_MAGIC_NAME, Kind::COMMUTATIVE));
  registerRule("int-subtract-zero",
               intSingleRule(m, 0, id_val(m), Module::SUB_MAGIC_NAME, Kind::RIGHT));
  registerRule("int-add-zero", intSingleRule(m, 0, id_val(m), Module::ADD_MAGIC_NAME,
                                             Kind::COMMUTATIVE));
  registerRule(
      "int-floor-div-by-one",
      intSingleRule(m, 1, id_val(m), Module::FLOOR_DIV_MAGIC_NAME, Kind::RIGHT));
  registerRule("int-zero-floor-div",
               intSingleRule(m, 0, 0, Module::FLOOR_DIV_MAGIC_NAME, Kind::LEFT));
  registerRule("int-pos", intNoOp(m, Module::POS_MAGIC_NAME));
  registerRule("int-double-neg", intDoubleApplyNoOp(m, Module::NEG_MAGIC_NAME));
  registerRule("int-double-inv", intDoubleApplyNoOp(m, Module::INVERT_MAGIC_NAME));

  // binary, double constant, int->int
  registerRule("int-constant-addition",
               intToIntBinary(m, BINOP(+), Module::ADD_MAGIC_NAME));
  registerRule("int-constant-subtraction",
               intToIntBinary(m, BINOP(-), Module::SUB_MAGIC_NAME));
  if (pyNumerics) {
    registerRule("int-constant-floor-div",
                 intToIntBinaryNoZeroRHS(
                     m, [](auto x, auto y) -> auto { return pyDivmod(x, y).first; },
                     Module::FLOOR_DIV_MAGIC_NAME));
  } else {
    registerRule("int-constant-floor-div",
                 intToIntBinaryNoZeroRHS(m, BINOP(/), Module::FLOOR_DIV_MAGIC_NAME));
  }
  registerRule("int-constant-mul", intToIntBinary(m, BINOP(*), Module::MUL_MAGIC_NAME));
  registerRule("int-constant-lshift",
               intToIntBinary(m, BINOP(<<), Module::LSHIFT_MAGIC_NAME));
  registerRule("int-constant-rshift",
               intToIntBinary(m, BINOP(>>), Module::RSHIFT_MAGIC_NAME));
  registerRule("int-constant-pow", intToIntBinary(m, int_pow, Module::POW_MAGIC_NAME));
  registerRule("int-constant-xor", intToIntBinary(m, BINOP(^), Module::XOR_MAGIC_NAME));
  registerRule("int-constant-or", intToIntBinary(m, BINOP(|), Module::OR_MAGIC_NAME));
  registerRule("int-constant-and", intToIntBinary(m, BINOP(&), Module::AND_MAGIC_NAME));
  if (pyNumerics) {
    registerRule("int-constant-mod",
                 intToIntBinaryNoZeroRHS(
                     m, [](auto x, auto y) -> auto { return pyDivmod(x, y).second; },
                     Module::MOD_MAGIC_NAME));
  } else {
    registerRule("int-constant-mod",
                 intToIntBinaryNoZeroRHS(m, BINOP(%), Module::MOD_MAGIC_NAME));
  }

  // binary, double constant, int->bool
  registerRule("int-constant-eq", intToBoolBinary(m, BINOP(==), Module::EQ_MAGIC_NAME));
  registerRule("int-constant-ne", intToBoolBinary(m, BINOP(!=), Module::NE_MAGIC_NAME));
  registerRule("int-constant-gt", intToBoolBinary(m, BINOP(>), Module::GT_MAGIC_NAME));
  registerRule("int-constant-ge", intToBoolBinary(m, BINOP(>=), Module::GE_MAGIC_NAME));
  registerRule("int-constant-lt", intToBoolBinary(m, BINOP(<), Module::LT_MAGIC_NAME));
  registerRule("int-constant-le", intToBoolBinary(m, BINOP(<=), Module::LE_MAGIC_NAME));

  // binary, double constant, bool->bool
  registerRule("bool-constant-xor",
               boolToBoolBinary(m, BINOP(^), Module::XOR_MAGIC_NAME));
  registerRule("bool-constant-or",
               boolToBoolBinary(m, BINOP(|), Module::OR_MAGIC_NAME));
  registerRule("bool-constant-and",
               boolToBoolBinary(m, BINOP(&), Module::AND_MAGIC_NAME));

  // unary, single constant, int->int
  registerRule("int-constant-pos", intToIntUnary(m, UNOP(+), Module::POS_MAGIC_NAME));
  registerRule("int-constant-neg", intToIntUnary(m, UNOP(-), Module::NEG_MAGIC_NAME));
  registerRule("int-constant-inv",
               intToIntUnary(m, UNOP(~), Module::INVERT_MAGIC_NAME));

  // unary, singe constant, float->float
  registerRule("float-constant-pos",
               floatToFloatUnary(m, UNOP(+), Module::POS_MAGIC_NAME));
  registerRule("float-constant-neg",
               floatToFloatUnary(m, UNOP(-), Module::NEG_MAGIC_NAME));

  // unary, single constant, bool->bool
  registerRule("bool-constant-inv",
               boolToBoolUnary(m, UNOP(!), Module::INVERT_MAGIC_NAME));

  // binary, double constant, float->float
  registerRule("float-constant-addition",
               floatToFloatBinary(m, BINOP(+), Module::ADD_MAGIC_NAME));
  registerRule("float-constant-subtraction",
               floatToFloatBinary(m, BINOP(-), Module::SUB_MAGIC_NAME));
  if (pyNumerics) {
    registerRule("float-constant-floor-div",
                 floatToFloatBinaryNoZeroRHS(m, BINOP(/), Module::TRUE_DIV_MAGIC_NAME));
  } else {
    registerRule("float-constant-floor-div",
                 floatToFloatBinary(m, BINOP(/), Module::TRUE_DIV_MAGIC_NAME));
  }
  registerRule("float-constant-mul",
               floatToFloatBinary(m, BINOP(*), Module::MUL_MAGIC_NAME));
  registerRule(
      "float-constant-pow",
      floatToFloatBinary(
          m, [](auto a, auto b) { return std::pow(a, b); }, Module::POW_MAGIC_NAME));

  // binary, double constant, float->bool
  registerRule("float-constant-eq",
               floatToBoolBinary(m, BINOP(==), Module::EQ_MAGIC_NAME));
  registerRule("float-constant-ne",
               floatToBoolBinary(m, BINOP(!=), Module::NE_MAGIC_NAME));
  registerRule("float-constant-gt",
               floatToBoolBinary(m, BINOP(>), Module::GT_MAGIC_NAME));
  registerRule("float-constant-ge",
               floatToBoolBinary(m, BINOP(>=), Module::GE_MAGIC_NAME));
  registerRule("float-constant-lt",
               floatToBoolBinary(m, BINOP(<), Module::LT_MAGIC_NAME));
  registerRule("float-constant-le",
               floatToBoolBinary(m, BINOP(<=), Module::LE_MAGIC_NAME));

  // binary, double constant, int,float->float
  registerRule("int-float-constant-addition",
               intFloatToFloatBinary(m, BINOP(+), Module::ADD_MAGIC_NAME));
  registerRule("int-float-constant-subtraction",
               intFloatToFloatBinary(m, BINOP(-), Module::SUB_MAGIC_NAME));
  registerRule(
      "int-float-constant-floor-div",
      intFloatToFloatBinary(m, BINOP(/), Module::TRUE_DIV_MAGIC_NAME, pyNumerics));
  registerRule("int-float-constant-mul",
               intFloatToFloatBinary(m, BINOP(*), Module::MUL_MAGIC_NAME));

  // binary, double constant, int,float->bool
  registerRule("int-float-constant-eq",
               intFloatToBoolBinary(m, BINOP(==), Module::EQ_MAGIC_NAME));
  registerRule("int-float-constant-ne",
               intFloatToBoolBinary(m, BINOP(!=), Module::NE_MAGIC_NAME));
  registerRule("int-float-constant-gt",
               intFloatToBoolBinary(m, BINOP(>), Module::GT_MAGIC_NAME));
  registerRule("int-float-constant-ge",
               intFloatToBoolBinary(m, BINOP(>=), Module::GE_MAGIC_NAME));
  registerRule("int-float-constant-lt",
               intFloatToBoolBinary(m, BINOP(<), Module::LT_MAGIC_NAME));
  registerRule("int-float-constant-le",
               intFloatToBoolBinary(m, BINOP(<=), Module::LE_MAGIC_NAME));

  // type conversions, identity
  registerRule("int-constant-int",
               identityConvert(m, Module::INT_MAGIC_NAME, m->getIntType()));
  registerRule("float-constant-float",
               identityConvert(m, Module::FLOAT_MAGIC_NAME, m->getFloatType()));
  registerRule("bool-constant-bool",
               identityConvert(m, Module::BOOL_MAGIC_NAME, m->getBoolType()));

  // type conversions, distinct
  registerRule("float-constant-int",
               typeConvert<double, int64_t>(m, Module::INT_MAGIC_NAME,
                                            m->getFloatType(), m->getIntType()));
  registerRule("bool-constant-int",
               typeConvert<bool, int64_t>(m, Module::INT_MAGIC_NAME, m->getBoolType(),
                                          m->getIntType()));
  registerRule("int-constant-float",
               typeConvert<int64_t, double>(m, Module::FLOAT_MAGIC_NAME,
                                            m->getIntType(), m->getFloatType()));
  registerRule("bool-constant-float",
               typeConvert<bool, double>(m, Module::FLOAT_MAGIC_NAME, m->getBoolType(),
                                         m->getFloatType()));
  registerRule("int-constant-bool",
               typeConvert<int64_t, bool>(m, Module::BOOL_MAGIC_NAME, m->getIntType(),
                                          m->getBoolType()));
  registerRule("float-constant-bool",
               typeConvert<double, bool>(m, Module::BOOL_MAGIC_NAME, m->getFloatType(),
                                         m->getBoolType()));
}

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon

#undef BINOP
#undef UNOP
