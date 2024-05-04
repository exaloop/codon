// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {

/// Cleanup pass that removes dead code.
class DeadCodeCleanupPass : public OperatorPass {
private:
  std::string sideEffectsKey;
  int numReplacements;

public:
  static const std::string KEY;

  /// Constructs a dead code elimination pass
  /// @param sideEffectsKey the side effect analysis' key
  DeadCodeCleanupPass(std::string sideEffectsKey)
      : OperatorPass(), sideEffectsKey(std::move(sideEffectsKey)), numReplacements(0) {}

  std::string getKey() const override { return KEY; }

  void run(Module *m) override;

  void handle(SeriesFlow *v) override;
  void handle(IfFlow *v) override;
  void handle(WhileFlow *v) override;
  void handle(ImperativeForFlow *v) override;
  void handle(TernaryInstr *v) override;

  /// @return the number of replacements
  int getNumReplacements() const { return numReplacements; }

private:
  void doReplacement(Value *og, Value *v);
};

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
