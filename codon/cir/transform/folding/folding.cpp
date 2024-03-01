// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "folding.h"

#include "codon/cir/transform/folding/const_fold.h"
#include "codon/cir/transform/folding/const_prop.h"

namespace codon {
namespace ir {
namespace transform {
namespace folding {

const std::string FoldingPassGroup::KEY = "core-folding-pass-group";

FoldingPassGroup::FoldingPassGroup(const std::string &sideEffectsPass,
                                   const std::string &reachingDefPass,
                                   const std::string &globalVarPass, int repeat,
                                   bool runGlobalDemotion, bool pyNumerics)
    : PassGroup(repeat) {
  auto gdUnique = runGlobalDemotion ? std::make_unique<cleanup::GlobalDemotionPass>()
                                    : std::unique_ptr<cleanup::GlobalDemotionPass>();
  auto canonUnique = std::make_unique<cleanup::CanonicalizationPass>(sideEffectsPass);
  auto fpUnique = std::make_unique<FoldingPass>(pyNumerics);
  auto dceUnique = std::make_unique<cleanup::DeadCodeCleanupPass>(sideEffectsPass);

  gd = gdUnique.get();
  canon = canonUnique.get();
  fp = fpUnique.get();
  dce = dceUnique.get();

  if (runGlobalDemotion)
    push_back(std::move(gdUnique));
  push_back(std::make_unique<ConstPropPass>(reachingDefPass, globalVarPass));
  push_back(std::move(canonUnique));
  push_back(std::move(fpUnique));
  push_back(std::move(dceUnique));
}

bool FoldingPassGroup::shouldRepeat(int num) const {
  return PassGroup::shouldRepeat(num) &&
         ((gd && gd->getNumDemotions() != 0) || canon->getNumReplacements() != 0 ||
          fp->getNumReplacements() != 0 || dce->getNumReplacements() != 0);
}

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
