// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>

#include "codon/cir/base.h"
#include "codon/cir/const.h"
#include "codon/cir/instr.h"
#include "codon/cir/util/side_effect.h"

namespace codon {
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
  /// Query this custom node for its side effect properties. If "local"
  /// is true, then the return value should reflect this node and this
  /// node alone, otherwise the value should reflect functions containing
  /// this node in their bodies. For example, a "break" instruction has
  /// side effects locally, but functions containing "break" might still
  /// be side effect free, hence the distinction.
  /// @param local true if result should reflect only this node
  /// @return this node's side effect status
  virtual util::SideEffectStatus getSideEffectStatus(bool local = true) const {
    return util::SideEffectStatus::UNKNOWN;
  }

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
  /// Query this custom node for its side effect properties. If "local"
  /// is true, then the return value should reflect this node and this
  /// node alone, otherwise the value should reflect functions containing
  /// this node in their bodies. For example, a "break" instruction has
  /// side effects locally, but functions containing "break" might still
  /// be side effect free, hence the distinction.
  /// @param local true if result should reflect only this node
  /// @return this node's side effect status
  virtual util::SideEffectStatus getSideEffectStatus(bool local = true) const {
    return util::SideEffectStatus::UNKNOWN;
  }

  /// Format the DSL node.
  /// @param os the output stream
  virtual std::ostream &doFormat(std::ostream &os) const = 0;
};

} // namespace dsl
} // namespace ir
} // namespace codon
