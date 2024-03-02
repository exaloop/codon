// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"
#include "codon/cir/transform/rewrite.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {

/// Canonicalization pass that flattens nested series
/// flows, puts operands in a predefined order, etc.
class CanonicalizationPass : public OperatorPass, public Rewriter {
private:
  std::string sideEffectsKey;

public:
  /// Constructs a canonicalization pass
  /// @param sideEffectsKey the side effect analysis' key
  CanonicalizationPass(const std::string &sideEffectsKey)
      : OperatorPass(/*childrenFirst=*/true), sideEffectsKey(sideEffectsKey) {}

  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  void run(Module *m) override;
  void handle(CallInstr *) override;
  void handle(SeriesFlow *) override;

private:
  void registerStandardRules(Module *m);
};

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
