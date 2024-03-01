// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "dominator.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

void DominatorInspector::analyze() {
  auto changed = true;
  while (changed) {
    changed = false;
    for (auto *blk : *cfg) {
      auto init = false;
      std::set<id_t> old = sets[blk->getId()];
      std::set<id_t> working;

      for (auto it = blk->predecessors_begin(); it != blk->predecessors_end(); ++it) {
        auto &predDoms = sets[(*it)->getId()];
        if (!init) {
          init = true;
          working = std::set<id_t>(predDoms.begin(), predDoms.end());
        }

        std::set<id_t> newWorking;
        std::set_intersection(working.begin(), working.end(), predDoms.begin(),
                              predDoms.end(),
                              std::inserter(newWorking, newWorking.begin()));
        working = newWorking;
      }

      working.insert(blk->getId());

      if (working != old) {
        changed = true;
        sets[blk->getId()] = working;
      }
    }
  }
}

bool DominatorInspector::isDominated(const Value *v, const Value *dominator) {
  auto *vBlock = cfg->getBlock(v);
  auto *dBlock = cfg->getBlock(dominator);

  if (vBlock->getId() == dBlock->getId()) {
    auto vDist =
        std::distance(vBlock->begin(), std::find(vBlock->begin(), vBlock->end(), v));
    auto dDist = std::distance(vBlock->begin(),
                               std::find(vBlock->begin(), vBlock->end(), dominator));
    return dDist <= vDist;
  }

  return sets[vBlock->getId()].find(dBlock->getId()) != sets[vBlock->getId()].end();
}

const std::string DominatorAnalysis::KEY = "core-analyses-dominator";

std::unique_ptr<Result> DominatorAnalysis::run(const Module *m) {
  auto *cfgResult = getAnalysisResult<CFResult>(cfAnalysisKey);
  auto ret = std::make_unique<DominatorResult>(cfgResult);
  for (const auto &graph : cfgResult->graphs) {
    auto inspector = std::make_unique<DominatorInspector>(graph.second.get());
    inspector->analyze();
    ret->results[graph.first] = std::move(inspector);
  }
  return ret;
}

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon
