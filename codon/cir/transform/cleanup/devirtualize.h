// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {

/// Proves exact runtime types with reaching definitions and bounded interprocedural
/// specialization, then annotates field accesses and resolves AOT virtual calls.
/// Preserves dynamic JIT dispatch and leaves inlining to LLVM.
class DevirtualizationPass : public Pass {
  std::string reachingKey;

public:
  static const std::string KEY;

  /// @param reachingKey the module's reaching-definition analysis key
  explicit DevirtualizationPass(std::string reachingKey)
      : reachingKey(std::move(reachingKey)) {}

  std::string getKey() const override { return KEY; }
  void run(Module *module) override;
};

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
