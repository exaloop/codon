// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_map>

#include "codon/cir/llvm/llvm.h"
#include "codon/cir/types/types.h"

namespace codon {
namespace ir {

namespace analyze {
namespace dataflow {
class CFVisitor;
} // namespace dataflow
} // namespace analyze

class LLVMVisitor;

namespace dsl {
namespace codegen {

/// Builder for LLVM types.
struct TypeBuilder {
  virtual ~TypeBuilder() noexcept = default;

  /// Construct the LLVM type.
  /// @param the LLVM visitor
  /// @return the LLVM type
  virtual llvm::Type *buildType(LLVMVisitor *visitor) = 0;
  /// Construct the LLVM debug type.
  /// @param the LLVM visitor
  /// @return the LLVM debug type
  virtual llvm::DIType *buildDebugType(LLVMVisitor *visitor) = 0;
};

/// Builder for LLVM values.
struct ValueBuilder {
  virtual ~ValueBuilder() noexcept = default;

  /// Construct the LLVM value.
  /// @param the LLVM visitor
  /// @return the LLVM value
  virtual llvm::Value *buildValue(LLVMVisitor *visitor) = 0;
};

/// Builder for control flow graphs.
struct CFBuilder {
  virtual ~CFBuilder() noexcept = default;

  /// Construct the control-flow nodes.
  /// @param graph the graph
  virtual void buildCFNodes(analyze::dataflow::CFVisitor *visitor) = 0;
};

} // namespace codegen
} // namespace dsl
} // namespace ir
} // namespace codon
