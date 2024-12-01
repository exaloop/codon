// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <unordered_map>

#include "codon/cir/transform/folding/rule.hpp"
#include "codon/cir/transform/pass.hpp"

namespace codon {
namespace ir {
namespace transform {
namespace folding {

class FoldingPass : public OperatorPass, public Rewriter {
private:
  bool pyNumerics;

  void registerStandardRules(Module *m);

public:
  /// Constructs a folding pass.
  FoldingPass(bool pyNumerics = false)
      : OperatorPass(/*childrenFirst=*/true), pyNumerics(pyNumerics) {}

  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void run(Module *m) override;
  void handle(CallInstr *v) override;
};

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
