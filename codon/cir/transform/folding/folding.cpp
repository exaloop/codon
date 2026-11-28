// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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
                                   bool runGlobalDemotion)
    : PassGroup(repeat) {
  auto gdUnique = runGlobalDemotion ? std::make_unique<cleanup::GlobalDemotionPass>()
                                    : std::unique_ptr<cleanup::GlobalDemotionPass>();
  auto canonUnique = std::make_unique<cleanup::CanonicalizationPass>(sideEffectsPass);
  auto fpUnique = std::make_unique<FoldingPass>();
  auto dceUnique = std::make_unique<cleanup::DeadCodeCleanupPass>(sideEffectsPass);

  gd = gdUnique.get();
  canon = canonUnique.get();
  fp = fpUnique.get();
  dce = dceUnique.get();

  if (runGlobalDemotion)
    push_back(std::move(gdUnique));
  auto propUnique = std::make_unique<ConstPropPass>(reachingDefPass, globalVarPass);
  prop = propUnique.get();
  push_back(std::move(propUnique));
  push_back(std::move(canonUnique));
  push_back(std::move(fpUnique));
  push_back(std::move(dceUnique));
}

void FoldingPassGroup::run(Module *m) {
  // Even a rejected rewrite can realize a new operator while checking its type.
  auto lastVar = [&]() { return m->begin() == m->end() ? nullptr : m->back(); };
  auto *before = lastVar();
  PassGroup::run(m);
  addedVars = before != lastVar();
}

bool FoldingPassGroup::shouldRepeat(int num) const {
  LOG_TIME("[T] folding round {}: demoted={}, propagated={}, canonicalized={}, "
           "folded={}, removed={}",
           num, gd ? gd->getNumDemotions() : 0, prop->getNumReplacements(),
           canon->getNumChanges(), fp->getNumReplacements(), dce->getNumReplacements());
  return PassGroup::shouldRepeat(num) && changed();
}

bool FoldingPassGroup::changed() const {
  return addedVars || (gd && gd->getNumDemotions()) || prop->getNumReplacements() ||
         canon->getNumChanges() || fp->getNumReplacements() ||
         dce->getNumReplacements();
}

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
