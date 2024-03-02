// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/module.h"
#include "codon/cir/value.h"

namespace codon {
namespace ir {

/// CIR constant base. Once created, constants are immutable.
class Const : public AcceptorExtend<Const, Value> {
private:
  /// the type
  types::Type *type;

public:
  static const char NodeId;

  /// Constructs a constant.
  /// @param type the type
  /// @param name the name
  explicit Const(types::Type *type, std::string name = "")
      : AcceptorExtend(std::move(name)), type(type) {}

private:
  types::Type *doGetType() const override { return type; }

  std::vector<types::Type *> doGetUsedTypes() const override { return {type}; }
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;
};

template <typename ValueType>
class TemplatedConst : public AcceptorExtend<TemplatedConst<ValueType>, Const> {
private:
  ValueType val;

public:
  static const char NodeId;

  using AcceptorExtend<TemplatedConst<ValueType>, Const>::getModule;
  using AcceptorExtend<TemplatedConst<ValueType>, Const>::getSrcInfo;
  using AcceptorExtend<TemplatedConst<ValueType>, Const>::getType;

  TemplatedConst(ValueType v, types::Type *type, std::string name = "")
      : AcceptorExtend<TemplatedConst<ValueType>, Const>(type, std::move(name)),
        val(v) {}

  /// @return the internal value.
  ValueType getVal() const { return val; }
  /// Sets the value.
  /// @param v the value
  void setVal(ValueType v) { val = v; }
};

using IntConst = TemplatedConst<int64_t>;
using FloatConst = TemplatedConst<double>;
using BoolConst = TemplatedConst<bool>;
using StringConst = TemplatedConst<std::string>;

template <typename T> const char TemplatedConst<T>::NodeId = 0;

template <>
class TemplatedConst<std::string>
    : public AcceptorExtend<TemplatedConst<std::string>, Const> {
private:
  std::string val;

public:
  static const char NodeId;

  TemplatedConst(std::string v, types::Type *type, std::string name = "")
      : AcceptorExtend(type, std::move(name)), val(std::move(v)) {}

  /// @return the internal value.
  std::string getVal() const { return val; }
  /// Sets the value.
  /// @param v the value
  void setVal(std::string v) { val = std::move(v); }
};

} // namespace ir
} // namespace codon
