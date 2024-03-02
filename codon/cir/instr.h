// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>

#include "codon/cir/flow.h"
#include "codon/cir/types/types.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/value.h"
#include "codon/cir/var.h"

namespace codon {
namespace ir {

/// CIR object representing an "instruction," or discrete operation in the context of a
/// block.
class Instr : public AcceptorExtend<Instr, Value> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

private:
  types::Type *doGetType() const override;
};

/// Instr representing setting a memory location.
class AssignInstr : public AcceptorExtend<AssignInstr, Instr> {
private:
  /// the left-hand side
  Var *lhs;
  /// the right-hand side
  Value *rhs;

public:
  static const char NodeId;

  /// Constructs an assign instruction.
  /// @param lhs the left-hand side
  /// @param rhs the right-hand side
  /// @param field the field being set, may be empty
  /// @param name the instruction's name
  AssignInstr(Var *lhs, Value *rhs, std::string name = "")
      : AcceptorExtend(std::move(name)), lhs(lhs), rhs(rhs) {}

  /// @return the left-hand side
  Var *getLhs() { return lhs; }
  /// @return the left-hand side
  const Var *getLhs() const { return lhs; }
  /// Sets the left-hand side
  /// @param l the new value
  void setLhs(Var *v) { lhs = v; }

  /// @return the right-hand side
  Value *getRhs() { return rhs; }
  /// @return the right-hand side
  const Value *getRhs() const { return rhs; }
  /// Sets the right-hand side
  /// @param l the new value
  void setRhs(Value *v) { rhs = v; }

protected:
  std::vector<Value *> doGetUsedValues() const override { return {rhs}; }
  int doReplaceUsedValue(id_t id, Value *newValue) override;

  std::vector<Var *> doGetUsedVariables() const override { return {lhs}; }
  int doReplaceUsedVariable(id_t id, Var *newVar) override;
};

/// Instr representing loading the field of a value.
class ExtractInstr : public AcceptorExtend<ExtractInstr, Instr> {
private:
  /// the value being manipulated
  Value *val;
  /// the field
  std::string field;

public:
  static const char NodeId;

  /// Constructs a load instruction.
  /// @param val the value being manipulated
  /// @param field the field
  /// @param name the instruction's name
  explicit ExtractInstr(Value *val, std::string field, std::string name = "")
      : AcceptorExtend(std::move(name)), val(val), field(std::move(field)) {}

  /// @return the location
  Value *getVal() { return val; }
  /// @return the location
  const Value *getVal() const { return val; }
  /// Sets the location.
  /// @param p the new value
  void setVal(Value *p) { val = p; }

  /// @return the field
  const std::string &getField() const { return field; }
  /// Sets the field.
  /// @param f the new field
  void setField(std::string f) { field = std::move(f); }

protected:
  types::Type *doGetType() const override;
  std::vector<Value *> doGetUsedValues() const override { return {val}; }
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr representing setting the field of a value.
class InsertInstr : public AcceptorExtend<InsertInstr, Instr> {
private:
  /// the value being manipulated
  Value *lhs;
  /// the field
  std::string field;
  /// the value being inserted
  Value *rhs;

public:
  static const char NodeId;

  /// Constructs a load instruction.
  /// @param lhs the value being manipulated
  /// @param field the field
  /// @param rhs the new value
  /// @param name the instruction's name
  explicit InsertInstr(Value *lhs, std::string field, Value *rhs, std::string name = "")
      : AcceptorExtend(std::move(name)), lhs(lhs), field(std::move(field)), rhs(rhs) {}

  /// @return the left-hand side
  Value *getLhs() { return lhs; }
  /// @return the left-hand side
  const Value *getLhs() const { return lhs; }
  /// Sets the left-hand side.
  /// @param p the new value
  void setLhs(Value *p) { lhs = p; }

  /// @return the right-hand side
  Value *getRhs() { return rhs; }
  /// @return the right-hand side
  const Value *getRhs() const { return rhs; }
  /// Sets the right-hand side.
  /// @param p the new value
  void setRhs(Value *p) { rhs = p; }

  /// @return the field
  const std::string &getField() const { return field; }
  /// Sets the field.
  /// @param f the new field
  void setField(std::string f) { field = std::move(f); }

protected:
  types::Type *doGetType() const override { return lhs->getType(); }
  std::vector<Value *> doGetUsedValues() const override { return {lhs, rhs}; }
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr representing calling a function.
class CallInstr : public AcceptorExtend<CallInstr, Instr> {
private:
  /// the function
  Value *callee;
  /// the arguments
  std::vector<Value *> args;

public:
  static const char NodeId;

  /// Constructs a call instruction.
  /// @param callee the function
  /// @param args the arguments
  /// @param name the instruction's name
  CallInstr(Value *callee, std::vector<Value *> args, std::string name = "")
      : AcceptorExtend(std::move(name)), callee(callee), args(std::move(args)) {}

  /// Constructs a call instruction with no arguments.
  /// @param callee the function
  /// @param name the instruction's name
  explicit CallInstr(Value *callee, std::string name = "")
      : CallInstr(callee, {}, std::move(name)) {}

  /// @return the callee
  Value *getCallee() { return callee; }
  /// @return the callee
  const Value *getCallee() const { return callee; }
  /// Sets the callee.
  /// @param c the new value
  void setCallee(Value *c) { callee = c; }

  /// @return an iterator to the first argument
  auto begin() { return args.begin(); }
  /// @return an iterator beyond the last argument
  auto end() { return args.end(); }
  /// @return an iterator to the first argument
  auto begin() const { return args.begin(); }
  /// @return an iterator beyond the last argument
  auto end() const { return args.end(); }

  /// @return a pointer to the first argument
  Value *front() { return args.front(); }
  /// @return a pointer to the last argument
  Value *back() { return args.back(); }
  /// @return a pointer to the first argument
  const Value *front() const { return args.front(); }
  /// @return a pointer to the last argument
  const Value *back() const { return args.back(); }

  /// Inserts an argument at the given position.
  /// @param pos the position
  /// @param v the argument
  /// @return an iterator to the newly added argument
  template <typename It> auto insert(It pos, Value *v) { return args.insert(pos, v); }
  /// Appends an argument.
  /// @param v the argument
  void push_back(Value *v) { args.push_back(v); }

  /// Sets the args.
  /// @param v the new args vector
  void setArgs(std::vector<Value *> v) { args = std::move(v); }

  /// @return the number of arguments
  int numArgs() const { return args.size(); }

protected:
  types::Type *doGetType() const override;
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr representing allocating an array on the stack.
class StackAllocInstr : public AcceptorExtend<StackAllocInstr, Instr> {
private:
  /// the array type
  types::Type *arrayType;
  /// number of elements to allocate
  int64_t count;

public:
  static const char NodeId;

  /// Constructs a stack allocation instruction.
  /// @param arrayType the type of the array
  /// @param count the number of elements
  /// @param name the name
  StackAllocInstr(types::Type *arrayType, int64_t count, std::string name = "")
      : AcceptorExtend(std::move(name)), arrayType(arrayType), count(count) {}

  /// @return the count
  int64_t getCount() const { return count; }
  /// Sets the count.
  /// @param c the new value
  void setCount(int64_t c) { count = c; }

  /// @return the array type
  types::Type *getArrayType() { return arrayType; }
  /// @return the array type
  types::Type *getArrayType() const { return arrayType; }
  /// Sets the array type.
  /// @param t the new type
  void setArrayType(types::Type *t) { arrayType = t; }

protected:
  types::Type *doGetType() const override { return arrayType; }
  std::vector<types::Type *> doGetUsedTypes() const override { return {arrayType}; }
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;
};

/// Instr representing getting information about a type.
class TypePropertyInstr : public AcceptorExtend<TypePropertyInstr, Instr> {
public:
  enum Property { IS_ATOMIC, IS_CONTENT_ATOMIC, SIZEOF };

private:
  /// the type being inspected
  types::Type *inspectType;
  /// the property being checked
  Property property;

public:
  static const char NodeId;

  /// Constructs a type property instruction.
  /// @param type the type being inspected
  /// @param name the name
  explicit TypePropertyInstr(types::Type *type, Property property,
                             std::string name = "")
      : AcceptorExtend(std::move(name)), inspectType(type), property(property) {}

  /// @return the type being inspected
  types::Type *getInspectType() { return inspectType; }
  /// @return the type being inspected
  types::Type *getInspectType() const { return inspectType; }
  /// Sets the type being inspected
  /// @param t the new type
  void setInspectType(types::Type *t) { inspectType = t; }

  /// @return the property being inspected
  Property getProperty() const { return property; }
  /// Sets the property.
  /// @param p the new value
  void setProperty(Property p) { property = p; }

protected:
  types::Type *doGetType() const override;
  std::vector<types::Type *> doGetUsedTypes() const override { return {inspectType}; }
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;
};

/// Instr representing a Python yield expression.
class YieldInInstr : public AcceptorExtend<YieldInInstr, Instr> {
private:
  /// the type of the value being yielded in.
  types::Type *type;
  /// whether or not to suspend
  bool suspend;

public:
  static const char NodeId;

  /// Constructs a yield in instruction.
  /// @param type the type of the value being yielded in
  /// @param suspend whether to suspend
  /// @param name the instruction's name
  explicit YieldInInstr(types::Type *type, bool suspend = true, std::string name = "")
      : AcceptorExtend(std::move(name)), type(type), suspend(suspend) {}

  /// @return true if the instruction suspends
  bool isSuspending() const { return suspend; }
  /// Sets the instruction suspending flag.
  /// @param v the new value
  void setSuspending(bool v = true) { suspend = v; }

  /// Sets the type being inspected
  /// @param t the new type
  void setType(types::Type *t) { type = t; }

protected:
  types::Type *doGetType() const override { return type; }
  std::vector<types::Type *> doGetUsedTypes() const override { return {type}; }
  int doReplaceUsedType(const std::string &name, types::Type *newType) override;
};

/// Instr representing a ternary operator.
class TernaryInstr : public AcceptorExtend<TernaryInstr, Instr> {
private:
  /// the condition
  Value *cond;
  /// the true value
  Value *trueValue;
  /// the false value
  Value *falseValue;

public:
  static const char NodeId;

  /// Constructs a ternary instruction.
  /// @param cond the condition
  /// @param trueValue the true value
  /// @param falseValue the false value
  /// @param name the instruction's name
  TernaryInstr(Value *cond, Value *trueValue, Value *falseValue, std::string name = "")
      : AcceptorExtend(std::move(name)), cond(cond), trueValue(trueValue),
        falseValue(falseValue) {}

  /// @return the condition
  Value *getCond() { return cond; }
  /// @return the condition
  const Value *getCond() const { return cond; }
  /// Sets the condition.
  /// @param v the new value
  void setCond(Value *v) { cond = v; }

  /// @return the condition
  Value *getTrueValue() { return trueValue; }
  /// @return the condition
  const Value *getTrueValue() const { return trueValue; }
  /// Sets the true value.
  /// @param v the new value
  void setTrueValue(Value *v) { trueValue = v; }

  /// @return the false value
  Value *getFalseValue() { return falseValue; }
  /// @return the false value
  const Value *getFalseValue() const { return falseValue; }
  /// Sets the value.
  /// @param v the new value
  void setFalseValue(Value *v) { falseValue = v; }

protected:
  types::Type *doGetType() const override { return trueValue->getType(); }
  std::vector<Value *> doGetUsedValues() const override {
    return {cond, trueValue, falseValue};
  }
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Base for control flow instructions
class ControlFlowInstr : public AcceptorExtend<ControlFlowInstr, Instr> {
public:
  static const char NodeId;
  using AcceptorExtend::AcceptorExtend;
};

/// Instr representing a break statement.
class BreakInstr : public AcceptorExtend<BreakInstr, ControlFlowInstr> {
private:
  /// the loop being broken, nullptr if the immediate ancestor
  Value *loop;

public:
  static const char NodeId;

  /// Constructs a break instruction.
  /// @param loop the loop being broken, nullptr if immediate ancestor
  /// @param name the instruction's name
  explicit BreakInstr(Value *loop = nullptr, std::string name = "")
      : AcceptorExtend(std::move(name)), loop(loop) {}

  /// @return the loop, nullptr if immediate ancestor
  Value *getLoop() const { return loop; }
  /// Sets the loop id.
  /// @param v the new loop, nullptr if immediate ancestor
  void setLoop(Value *v) { loop = v; }

  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr representing a continue statement.
class ContinueInstr : public AcceptorExtend<ContinueInstr, ControlFlowInstr> {
private:
  /// the loop being continued, nullptr if the immediate ancestor
  Value *loop;

public:
  static const char NodeId;

  /// Constructs a continue instruction.
  /// @param loop the loop being continued, nullptr if immediate ancestor
  /// @param name the instruction's name
  explicit ContinueInstr(Value *loop = nullptr, std::string name = "")
      : AcceptorExtend(std::move(name)), loop(loop) {}

  /// @return the loop, nullptr if immediate ancestor
  Value *getLoop() const { return loop; }
  /// Sets the loop id.
  /// @param v the new loop, -1 if immediate ancestor
  void setLoop(Value *v) { loop = v; }

  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr representing a return statement.
class ReturnInstr : public AcceptorExtend<ReturnInstr, ControlFlowInstr> {
private:
  /// the value
  Value *value;

public:
  static const char NodeId;

  explicit ReturnInstr(Value *value = nullptr, std::string name = "")
      : AcceptorExtend(std::move(name)), value(value) {}

  /// @return the value
  Value *getValue() { return value; }
  /// @return the value
  const Value *getValue() const { return value; }
  /// Sets the value.
  /// @param v the new value
  void setValue(Value *v) { value = v; }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

class YieldInstr : public AcceptorExtend<YieldInstr, Instr> {
private:
  /// the value
  Value *value;
  /// whether this yield is final
  bool final;

public:
  static const char NodeId;

  explicit YieldInstr(Value *value = nullptr, bool final = false, std::string name = "")
      : AcceptorExtend(std::move(name)), value(value), final(final) {}

  /// @return the value
  Value *getValue() { return value; }
  /// @return the value
  const Value *getValue() const { return value; }
  /// Sets the value.
  /// @param v the new value
  void setValue(Value *v) { value = v; }

  /// @return if this yield is final
  bool isFinal() const { return final; }
  /// Sets whether this yield is final.
  /// @param f true if final
  void setFinal(bool f = true) { final = f; }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

class ThrowInstr : public AcceptorExtend<ThrowInstr, Instr> {
private:
  /// the value
  Value *value;

public:
  static const char NodeId;

  explicit ThrowInstr(Value *value = nullptr, std::string name = "")
      : AcceptorExtend(std::move(name)), value(value) {}

  /// @return the value
  Value *getValue() { return value; }
  /// @return the value
  const Value *getValue() const { return value; }
  /// Sets the value.
  /// @param v the new value
  void setValue(Value *v) { value = v; }

protected:
  std::vector<Value *> doGetUsedValues() const override;
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

/// Instr that contains a flow and value.
class FlowInstr : public AcceptorExtend<FlowInstr, Instr> {
private:
  /// the flow
  Value *flow;
  /// the output value
  Value *val;

public:
  static const char NodeId;

  /// Constructs a flow value.
  /// @param flow the flow
  /// @param val the output value
  /// @param name the name
  explicit FlowInstr(Flow *flow, Value *val, std::string name = "")
      : AcceptorExtend(std::move(name)), flow(flow), val(val) {}

  /// @return the flow
  Flow *getFlow() { return cast<Flow>(flow); }
  /// @return the flow
  const Flow *getFlow() const { return cast<Flow>(flow); }
  /// Sets the flow.
  /// @param f the new flow
  void setFlow(Flow *f) { flow = f; }

  /// @return the value
  Value *getValue() { return val; }
  /// @return the value
  const Value *getValue() const { return val; }
  /// Sets the value.
  /// @param v the new value
  void setValue(Value *v) { val = v; }

protected:
  types::Type *doGetType() const override { return val->getType(); }
  std::vector<Value *> doGetUsedValues() const override { return {flow, val}; }
  int doReplaceUsedValue(id_t id, Value *newValue) override;
};

} // namespace ir
} // namespace codon
