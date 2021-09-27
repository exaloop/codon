#include "reaching.h"

namespace seq {
namespace ir {
namespace {
id_t getKilled(const Value *val) {
  if (auto *assign = cast<AssignInstr>(val)) {
    return assign->getLhs()->getId();
  } else if (auto *synthAssign = cast<analyze::dataflow::SyntheticAssignInstr>(val)) {
    return synthAssign->getLhs()->getId();
  }
  return -1;
}

std::pair<id_t, id_t> getGenerated(const Value *val) {
  if (auto *assign = cast<AssignInstr>(val)) {
    return {assign->getLhs()->getId(), assign->getRhs()->getId()};
  } else if (auto *synthAssign = cast<analyze::dataflow::SyntheticAssignInstr>(val)) {
    if (synthAssign->getKind() == analyze::dataflow::SyntheticAssignInstr::KNOWN)
      return {synthAssign->getLhs()->getId(), synthAssign->getArg()->getId()};
    else
      return {synthAssign->getLhs()->getId(), -1};
  }
  return {-1, -1};
}
} // namespace

namespace analyze {
namespace dataflow {

void RDInspector::analyze() {
  std::unordered_set<CFBlock *> workset(cfg->begin(), cfg->end());
  while (!workset.empty()) {
    std::unordered_set<CFBlock *> newWorkset;
    for (auto *blk : workset) {
      initializeIfNecessary(blk);
      calculateIn(blk);
      if (!calculateOut(blk))
        newWorkset.insert(blk->successors_begin(), blk->successors_end());
    }
    workset = std::move(newWorkset);
  }
}

std::unordered_set<id_t> RDInspector::getReachingDefinitions(Var *var, Value *loc) {
  if (invalid.find(var->getId()) != invalid.end() || var->isGlobal())
    return std::unordered_set<id_t>();

  auto *blk = cfg->getBlock(loc);
  if (!blk)
    return std::unordered_set<id_t>();
  auto &entry = sets[blk->getId()];
  auto defs = entry.in[var->getId()];

  auto done = false;
  for (auto *val : *blk) {
    if (done)
      break;
    if (val->getId() == loc->getId())
      done = true;

    auto killed = getKilled(val);
    if (killed == var->getId())
      defs.clear();
    auto gen = getGenerated(val);
    if (gen.first == var->getId())
      defs.insert(gen.second);
  }

  if (defs.find(-1) != defs.end())
    return std::unordered_set<id_t>();

  return defs;
}

void RDInspector::initializeIfNecessary(CFBlock *blk) {
  auto &entry = sets[blk->getId()];
  if (entry.initialized)
    return;
  entry.initialized = true;
  for (auto *val : *blk) {
    if (auto *ptr = cast<PointerValue>(val))
      invalid.insert(ptr->getVar()->getId());

    auto killed = getKilled(val);
    if (killed != -1)
      entry.killed.insert(killed);
    auto gen = getGenerated(val);
    if (gen.first != -1)
      entry.generated[gen.first] = gen.second;
  }
}

void RDInspector::calculateIn(CFBlock *blk) {
  auto &curEntry = sets[blk->getId()];
  std::unordered_map<id_t, std::unordered_set<id_t>> newVal;

  if (blk->getId() == cfg->getEntryBlock()->getId()) {
    auto *fn = cfg->getFunc();
    for (auto *v : *fn)
      newVal[v->getId()] = {-1};
    for (auto it = fn->arg_begin(); it != fn->arg_end(); ++it)
      newVal[(*it)->getId()] = {-1};
  }

  for (auto it = blk->predecessors_begin(); it != blk->predecessors_end(); ++it) {
    auto *pred = *it;
    auto &predEntry = sets[pred->getId()];
    for (auto &it2 : predEntry.out) {
      auto &loc = newVal[it2.first];
      loc.insert(it2.second.begin(), it2.second.end());
    }
  }
  curEntry.in = std::move(newVal);
}

bool RDInspector::calculateOut(CFBlock *blk) {
  auto &entry = sets[blk->getId()];
  std::unordered_map<id_t, std::unordered_set<id_t>> newOut;
  for (auto &gen : entry.generated) {
    newOut[gen.first] = {gen.second};
  }

  for (auto it = entry.in.begin(); it != entry.in.end(); ++it) {
    if (entry.killed.find(it->first) == entry.killed.end()) {
      newOut[it->first].insert(it->second.begin(), it->second.end());
    }
  }

  auto res = entry.out == newOut;
  entry.out = std::move(newOut);
  return res;
}

const std::string RDAnalysis::KEY = "core-analyses-rd";

std::unique_ptr<Result> RDAnalysis::run(const Module *m) {
  auto *cfgResult = getAnalysisResult<CFResult>(cfAnalysisKey);
  auto ret = std::make_unique<RDResult>(cfgResult);
  for (const auto &graph : cfgResult->graphs) {
    auto inspector = std::make_unique<RDInspector>(graph.second.get());
    inspector->analyze();
    ret->results[graph.first] = std::move(inspector);
  }
  return ret;
}

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace seq
