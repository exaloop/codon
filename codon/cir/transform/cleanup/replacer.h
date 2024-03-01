// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {

/// Cleanup pass that physically replaces nodes.
class ReplaceCleanupPass : public Pass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void run(Module *module) override;
};

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
