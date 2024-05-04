// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <algorithm>
#include <utility>

#include "codon/cir/transform/pass.h"
#include "codon/cir/transform/rewrite.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace folding {

/// Commutative, binary rule that requires a single constant.
template <typename ConstantType>
class SingleConstantCommutativeRule : public RewriteRule {
public:
  using Calculator = std::function<Value *(Value *)>;
  enum Kind { LEFT, RIGHT, COMMUTATIVE };

private:
  /// the value being matched against
  ConstantType val;
  /// the type being matched
  types::Type *type;
  /// the magic method name
  std::string magic;
  /// the calculator
  Calculator calc;
  /// left, right or commutative
  Kind kind;

public:
  /// Constructs a commutative rule.
  /// @param val the matched value
  /// @param newVal the result
  /// @param magic the magic name
  /// @param kind left, right, or commutative
  /// @param type the matched type
  SingleConstantCommutativeRule(ConstantType val, ConstantType newVal,
                                std::string magic, Kind kind, types::Type *type)
      : val(val), type(type), magic(std::move(magic)), kind(kind) {
    calc = [=](Value *v) -> Value * {
      return v->getModule()->N<TemplatedConst<ConstantType>>(v->getSrcInfo(), val,
                                                             type);
    };
  }
  /// Constructs a commutative rule.
  /// @param val the matched value
  /// @param newVal the result
  /// @param magic the magic name
  /// @param calc the calculator
  /// @param kind left, right, or commutative
  /// @param type the matched type
  SingleConstantCommutativeRule(ConstantType val, Calculator calc, std::string magic,
                                Kind kind, types::Type *type)
      : val(val), type(type), magic(std::move(magic)), calc(std::move(calc)),
        kind(kind) {}

  virtual ~SingleConstantCommutativeRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {type, type}, type, /*method=*/true))
      return;

    auto *left = v->front();
    auto *right = v->back();
    auto *leftConst = cast<TemplatedConst<ConstantType>>(left);
    auto *rightConst = cast<TemplatedConst<ConstantType>>(right);

    if ((kind == Kind::COMMUTATIVE || kind == Kind::LEFT) && leftConst &&
        leftConst->getVal() == val)
      return setResult(calc(right));
    if ((kind == Kind::COMMUTATIVE || kind == Kind::RIGHT) && rightConst &&
        rightConst->getVal() == val)
      return setResult(calc(left));
  }
};

/// Binary rule that requires two constants.
template <typename ConstantType, typename Func, typename OutputType = ConstantType>
class DoubleConstantBinaryRule : public RewriteRule {
private:
  /// the calculator
  Func f;
  /// the input type
  types::Type *inputType;
  /// the output type
  types::Type *resultType;
  /// the magic method name
  std::string magic;

public:
  /// Constructs a binary rule.
  /// @param f the calculator
  /// @param magic the magic method name
  /// @param inputType the input type
  /// @param resultType the output type
  DoubleConstantBinaryRule(Func f, std::string magic, types::Type *inputType,
                           types::Type *resultType)
      : f(std::move(f)), inputType(inputType), resultType(resultType),
        magic(std::move(magic)) {}

  virtual ~DoubleConstantBinaryRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {inputType, inputType}, resultType, /*method=*/true))
      return;

    auto *left = v->front();
    auto *right = v->back();
    auto *leftConst = cast<TemplatedConst<ConstantType>>(left);
    auto *rightConst = cast<TemplatedConst<ConstantType>>(right);

    if (leftConst && rightConst)
      return setResult(toValue(v, f(leftConst->getVal(), rightConst->getVal())));
  }

private:
  Value *toValue(Value *, Value *v) { return v; }

  Value *toValue(Value *og, OutputType v) {
    return og->getModule()->template N<TemplatedConst<OutputType>>(og->getSrcInfo(), v,
                                                                   resultType);
  }
};

/// Unary rule that requires one constant.
template <typename ConstantType, typename Func>
class SingleConstantUnaryRule : public RewriteRule {
private:
  /// the calculator
  Func f;
  /// the input type
  types::Type *inputType;
  /// the output type
  types::Type *resultType;
  /// the magic method name
  std::string magic;

public:
  /// Constructs a unary rule.
  /// @param f the calculator
  /// @param magic the magic method name
  /// @param inputType the input type
  /// @param resultType the output type
  SingleConstantUnaryRule(Func f, std::string magic, types::Type *inputType,
                          types::Type *resultType)
      : f(std::move(f)), inputType(inputType), resultType(resultType),
        magic(std::move(magic)) {}

  virtual ~SingleConstantUnaryRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {inputType}, resultType, /*method=*/true))
      return;

    auto *arg = v->front();
    auto *argConst = cast<TemplatedConst<ConstantType>>(arg);
    if (argConst)
      return setResult(toValue(v, f(argConst->getVal())));
  }

private:
  Value *toValue(Value *, Value *v) { return v; }

  template <typename NewType> Value *toValue(Value *og, NewType v) {
    return og->getModule()->template N<TemplatedConst<NewType>>(og->getSrcInfo(), v,
                                                                resultType);
  }
};

/// Unary rule that requires no constant.
template <typename Func> class UnaryRule : public RewriteRule {
private:
  /// the calculator
  Func f;
  /// the input type
  types::Type *inputType;
  /// the magic method name
  std::string magic;

public:
  /// Constructs a unary rule.
  /// @param f the calculator
  /// @param magic the magic method name
  /// @param inputType the input type
  UnaryRule(Func f, std::string magic, types::Type *inputType)
      : f(std::move(f)), inputType(inputType), magic(std::move(magic)) {}

  virtual ~UnaryRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {inputType}, inputType, /*method=*/true))
      return;

    auto *arg = v->front();
    return setResult(f(arg));
  }
};

/// Rule that eliminates an operation, like "+x".
class NoOpRule : public RewriteRule {
private:
  /// the input type
  types::Type *inputType;
  /// the magic method name
  std::string magic;

public:
  /// Constructs a no-op rule.
  /// @param magic the magic method name
  /// @param inputType the input type
  NoOpRule(std::string magic, types::Type *inputType)
      : inputType(inputType), magic(std::move(magic)) {}

  virtual ~NoOpRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {inputType}, inputType, /*method=*/true))
      return;

    auto *arg = v->front();
    return setResult(arg);
  }
};

/// Rule that eliminates a double-application of an operation, like "-(-x)".
class DoubleApplicationNoOpRule : public RewriteRule {
private:
  /// the input type
  types::Type *inputType;
  /// the magic method name
  std::string magic;

public:
  /// Constructs a double-application no-op rule.
  /// @param magic the magic method name
  /// @param inputType the input type
  DoubleApplicationNoOpRule(std::string magic, types::Type *inputType)
      : inputType(inputType), magic(std::move(magic)) {}

  virtual ~DoubleApplicationNoOpRule() noexcept = default;

  void visit(CallInstr *v) override {
    if (!util::isCallOf(v, magic, {inputType}, inputType, /*method=*/true))
      return;

    if (!util::isCallOf(v->front(), magic, {inputType}, inputType, /*method=*/true))
      return;

    auto *arg = v->front();
    return setResult(cast<CallInstr>(arg)->front());
  }
};

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
