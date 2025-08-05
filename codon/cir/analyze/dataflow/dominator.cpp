// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "dominator.h"

#include "codon/cir/llvm/llvm.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

void DominatorInspector::analyze() {
  const auto numBlocks = cfg->size();
  std::unordered_map<id_t, size_t> mapping;        // id -> sequential id
  std::vector<id_t> mappingInv(numBlocks);         // sequential id -> id
  std::vector<llvm::BitVector> bitvecs(numBlocks); // sequential id -> bitvector

  mapping.reserve(numBlocks);
  size_t next = 0;
  for (auto *blk : *cfg) {
    auto id = blk->getId();
    mapping[id] = next;
    mappingInv[next] = id;
    ++next;
  }

  // Initialize: all blocks dominate themselves; others start with universal set
  for (auto *blk : *cfg) {
    auto id = mapping[blk->getId()];
    llvm::BitVector &bv = bitvecs[id];
    bv.resize(numBlocks, true);
    if (blk == cfg->getEntryBlock()) { // entry block only dominated by itself
      bv.reset();
      bv.set(id);
    }
  }

  // Run simple domination algorithm
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto *blk : *cfg) {
      auto id = mapping[blk->getId()];
      llvm::BitVector old = bitvecs[id];
      llvm::BitVector newSet;

      if (blk->predecessors_begin() == blk->predecessors_end())
        newSet.resize(numBlocks);

      bool first = true;
      for (auto it = blk->predecessors_begin(); it != blk->predecessors_end(); ++it) {
        auto predId = mapping[(*it)->getId()];
        const auto &predSet = bitvecs[predId];
        if (first) {
          newSet = predSet;
          first = false;
        } else {
          newSet &= predSet;
        }
      }

      newSet.set(id); // a block always dominates itself

      if (newSet != old) {
        bitvecs[id] = newSet;
        changed = true;
      }
    }
  }

  // Map back to canonical id
  sets.reserve(numBlocks);
  for (unsigned id = 0; id < numBlocks; id++) {
    auto &bv = bitvecs[id];
    auto &set = sets[mappingInv[id]];
    for (auto n = bv.find_first(); n != -1; n = bv.find_next(n)) {
      set.insert(mappingInv[n]);
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

  auto &set = sets[vBlock->getId()];
  return set.find(dBlock->getId()) != set.end();
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
