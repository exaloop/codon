// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "value.h"

#include "codon/cir/instr.h"
#include "codon/cir/module.h"

namespace codon {
namespace ir {

const char Value::NodeId = 0;

Value *Value::operator==(Value &other) {
  return doBinaryOp(Module::EQ_MAGIC_NAME, other);
}

Value *Value::operator!=(Value &other) {
  return doBinaryOp(Module::NE_MAGIC_NAME, other);
}

Value *Value::operator<(Value &other) {
  return doBinaryOp(Module::LT_MAGIC_NAME, other);
}

Value *Value::operator>(Value &other) {
  return doBinaryOp(Module::GT_MAGIC_NAME, other);
}

Value *Value::operator<=(Value &other) {
  return doBinaryOp(Module::LE_MAGIC_NAME, other);
}

Value *Value::operator>=(Value &other) {
  return doBinaryOp(Module::GE_MAGIC_NAME, other);
}

Value *Value::operator+() { return doUnaryOp(Module::POS_MAGIC_NAME); }

Value *Value::operator-() { return doUnaryOp(Module::NEG_MAGIC_NAME); }

Value *Value::operator~() { return doUnaryOp(Module::INVERT_MAGIC_NAME); }

Value *Value::operator+(Value &other) {
  return doBinaryOp(Module::ADD_MAGIC_NAME, other);
}

Value *Value::operator-(Value &other) {
  return doBinaryOp(Module::SUB_MAGIC_NAME, other);
}

Value *Value::operator*(Value &other) {
  return doBinaryOp(Module::MUL_MAGIC_NAME, other);
}

Value *Value::matMul(Value &other) {
  return doBinaryOp(Module::MATMUL_MAGIC_NAME, other);
}

Value *Value::trueDiv(Value &other) {
  return doBinaryOp(Module::TRUE_DIV_MAGIC_NAME, other);
}

Value *Value::operator/(Value &other) {
  return doBinaryOp(Module::FLOOR_DIV_MAGIC_NAME, other);
}

Value *Value::operator%(Value &other) {
  return doBinaryOp(Module::MOD_MAGIC_NAME, other);
}

Value *Value::pow(Value &other) { return doBinaryOp(Module::POW_MAGIC_NAME, other); }

Value *Value::operator<<(Value &other) {
  return doBinaryOp(Module::LSHIFT_MAGIC_NAME, other);
}

Value *Value::operator>>(Value &other) {
  return doBinaryOp(Module::RSHIFT_MAGIC_NAME, other);
}

Value *Value::operator&(Value &other) {
  return doBinaryOp(Module::AND_MAGIC_NAME, other);
}

Value *Value::operator|(Value &other) {
  return doBinaryOp(Module::OR_MAGIC_NAME, other);
}

Value *Value::operator^(Value &other) {
  return doBinaryOp(Module::XOR_MAGIC_NAME, other);
}

Value *Value::operator||(Value &other) {
  auto *module = getModule();
  return module->Nr<TernaryInstr>(toBool(), module->getBool(true), other.toBool());
}

Value *Value::operator&&(Value &other) {
  auto *module = getModule();
  return module->Nr<TernaryInstr>(toBool(), other.toBool(), module->getBool(false));
}

Value *Value::operator[](Value &other) {
  return doBinaryOp(Module::GETITEM_MAGIC_NAME, other);
}

Value *Value::toInt() { return doUnaryOp(Module::INT_MAGIC_NAME); }

Value *Value::toFloat() { return doUnaryOp(Module::FLOAT_MAGIC_NAME); }

Value *Value::toBool() { return doUnaryOp(Module::BOOL_MAGIC_NAME); }

Value *Value::toStr() { return doUnaryOp(Module::REPR_MAGIC_NAME); }

Value *Value::len() { return doUnaryOp(Module::LEN_MAGIC_NAME); }

Value *Value::iter() { return doUnaryOp(Module::ITER_MAGIC_NAME); }

Value *Value::doUnaryOp(const std::string &name) {
  auto *module = getModule();
  auto *fn = module->getOrRealizeMethod(getType(), name,
                                        std::vector<types::Type *>{getType()});

  if (!fn)
    return nullptr;

  auto *fnVal = module->Nr<VarValue>(fn);
  return (*fnVal)(*this);
}

Value *Value::doBinaryOp(const std::string &name, Value &other) {
  auto *module = getModule();
  auto *fn = module->getOrRealizeMethod(
      getType(), name, std::vector<types::Type *>{getType(), other.getType()});

  if (!fn)
    return nullptr;

  auto *fnVal = module->Nr<VarValue>(fn);
  return (*fnVal)(*this, other);
}

Value *Value::doCall(const std::vector<Value *> &args) {
  auto *module = getModule();
  return module->Nr<CallInstr>(this, args);
}

} // namespace ir
} // namespace codon
