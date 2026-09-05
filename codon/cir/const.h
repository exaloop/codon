// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/module.h"
#include "codon/cir/value.h"

namespace codon {
namespace ir {

/// CIR constant base. Once created, constants are immutable.
class Const : public AcceptorExtend<Const, Value> {
private:
  /// the type
  Type *type;

public:
  static const char NodeId;

  /// Constructs a constant.
  /// @param type the type
  /// @param name the name
  explicit Const(Type *type, std::string name = "")
      : AcceptorExtend(std::move(name)), type(type) {}

private:
  Type *doGetType() const override { return type; }

  std::vector<Type *> doGetUsedTypes() const override { return {type}; }
  int doReplaceUsedType(const std::string &name, Type *newType) override;
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

  TemplatedConst(ValueType v, Type *type, std::string name = "")
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

template <typename T> const char TemplatedConst<T>::NodeId = 0;

template <> class TemplatedConst<std::string> : public Const {
private:
  std::string val;

public:
  static const char NodeId;

  TemplatedConst(std::string v, Type *type, std::string name = "")
      : Const(type, std::move(name)), val(std::move(v)) {}

  /// @return the internal value.
  std::string getVal() const { return val; }
  /// Sets the value.
  /// @param v the value
  void setVal(std::string v) { val = std::move(v); }
};

class StringConst : public AcceptorExtend<StringConst, TemplatedConst<std::string>> {
public:
  static const char NodeId;

  using AcceptorExtend<StringConst, TemplatedConst<std::string>>::AcceptorExtend;
};

class BytesConst : public AcceptorExtend<BytesConst, TemplatedConst<std::string>> {
public:
  static const char NodeId;

  using AcceptorExtend<BytesConst, TemplatedConst<std::string>>::AcceptorExtend;
};

} // namespace ir
} // namespace codon
