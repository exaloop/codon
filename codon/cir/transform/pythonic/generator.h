// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {

/// Pass to optimize passing a generator to some built-in functions
/// like sum(), any() or all(), which will be converted to regular
/// for-loops.
class GeneratorArgumentOptimization : public OperatorPass {
public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void handle(CallInstr *v) override;
};

class GeneratorLoopFusion : public OperatorPass {
  std::unordered_set<id_t> wrappers;
  std::unordered_map<id_t, int> growth;

public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }
  void run(Module *module) override {
    wrappers.clear();
    growth.clear();
    OperatorPass::run(module);
  }
  void handle(CallInstr *call) override;
  void handle(ForFlow *loop) override;
};

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
