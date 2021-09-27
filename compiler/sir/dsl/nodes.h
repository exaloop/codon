#pragma once

#include <memory>

#include "sir/base.h"
#include "sir/const.h"
#include "sir/instr.h"

namespace seq {
namespace ir {

namespace util {
class CloneVisitor;
} // namespace util

namespace dsl {

namespace codegen {
struct CFBuilder;
struct TypeBuilder;
struct ValueBuilder;
} // namespace codegen

namespace types {

/// DSL type.
class CustomType : public AcceptorExtend<CustomType, ir::types::Type> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return the type builder
  virtual std::unique_ptr<codegen::TypeBuilder> getBuilder() const = 0;

  /// Compares DSL nodes.
  /// @param v the other node
  /// @return true if they match
  virtual bool match(const Type *v) const = 0;

  /// Format the DSL node.
  /// @param os the output stream
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

} // namespace types

/// DSL constant.
class CustomConst : public AcceptorExtend<CustomConst, Const> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return the value builder
  virtual std::unique_ptr<codegen::ValueBuilder> getBuilder() const = 0;
  /// Compares DSL nodes.
  /// @param v the other node
  /// @return true if they match
  virtual bool match(const Value *v) const = 0;
  /// Clones the value.
  /// @param cv the clone visitor
  /// @return a clone of the object
  virtual Value *doClone(util::CloneVisitor &cv) const = 0;

  /// Format the DSL node.
  /// @param os the output stream
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

/// DSL flow.
class CustomFlow : public AcceptorExtend<CustomFlow, Flow> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return the value builder
  virtual std::unique_ptr<codegen::ValueBuilder> getBuilder() const = 0;
  /// Compares DSL nodes.
  /// @param v the other node
  /// @return true if they match
  virtual bool match(const Value *v) const = 0;
  /// Clones the value.
  /// @param cv the clone visitor
  /// @return a clone of the object
  virtual Value *doClone(util::CloneVisitor &cv) const = 0;
  /// @return the control-flow builder
  virtual std::unique_ptr<codegen::CFBuilder> getCFBuilder() const = 0;
  /// @return true if this flow has side effects
  virtual bool hasSideEffect() const { return true; }

  /// Format the DSL node.
  /// @param os the output stream
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

/// DSL instruction.
class CustomInstr : public AcceptorExtend<CustomInstr, Instr> {
public:
  static const char NodeId;

  using AcceptorExtend::AcceptorExtend;

  /// @return the value builder
  virtual std::unique_ptr<codegen::ValueBuilder> getBuilder() const = 0;
  /// Compares DSL nodes.
  /// @param v the other node
  /// @return true if they match
  virtual bool match(const Value *v) const = 0;
  /// Clones the value.
  /// @param cv the clone visitor
  /// @return a clone of the object
  virtual Value *doClone(util::CloneVisitor &cv) const = 0;
  /// @return the control-flow builder
  virtual std::unique_ptr<codegen::CFBuilder> getCFBuilder() const = 0;
  /// @return true if this instruction has side effects
  virtual bool hasSideEffect() const { return true; }

  /// Format the DSL node.
  /// @param os the output stream
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

} // namespace dsl
} // namespace ir
} // namespace seq
