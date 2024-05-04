// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/transform/cleanup/canonical.h"
#include "codon/cir/transform/cleanup/dead_code.h"
#include "codon/cir/transform/cleanup/global_demote.h"
#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace transform {
namespace folding {

class FoldingPass;

/// Group of constant folding passes.
class FoldingPassGroup : public PassGroup {
private:
  cleanup::GlobalDemotionPass *gd;
  cleanup::CanonicalizationPass *canon;
  FoldingPass *fp;
  cleanup::DeadCodeCleanupPass *dce;

public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  /// @param sideEffectsPass the key of the side effects pass
  /// @param reachingDefPass the key of the reaching definitions pass
  /// @param globalVarPass the key of the global variables pass
  /// @param repeat default number of times to repeat the pass
  /// @param runGlobalDemotion whether to demote globals if possible
  /// @param pyNumerics whether to use Python (vs. C) semantics when folding
  FoldingPassGroup(const std::string &sideEffectsPass,
                   const std::string &reachingDefPass, const std::string &globalVarPass,
                   int repeat = 5, bool runGlobalDemotion = true,
                   bool pyNumerics = false);

  bool shouldRepeat(int num) const override;
};

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
