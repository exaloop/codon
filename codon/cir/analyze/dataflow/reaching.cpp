// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "reaching.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <tuple>

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {
namespace {

id_t getKilled(const Value *val) {
  if (auto *assign = cast<AssignInstr>(val)) {
    return assign->getLhs()->getId();
  } else if (auto *synthAssign = cast<analyze::dataflow::SyntheticAssignInstr>(val)) {
    return synthAssign->getLhs()->getId();
  }
  return -1;
}

std::pair<id_t, ReachingDef> getGenerated(const Value *val) {
  if (auto *assign = cast<AssignInstr>(val)) {
    return std::make_pair(assign->getLhs()->getId(),
                          ReachingDef(assign, assign->getRhs()));
  } else if (auto *synthAssign = cast<analyze::dataflow::SyntheticAssignInstr>(val)) {
    if (synthAssign->getKind() == analyze::dataflow::SyntheticAssignInstr::KNOWN)
      return std::make_pair(synthAssign->getLhs()->getId(),
                            ReachingDef(synthAssign, synthAssign->getArg()));
    else
      return std::make_pair(synthAssign->getLhs()->getId(), ReachingDef(synthAssign));
  }
  return std::make_pair(-1, ReachingDef(nullptr));
}

template <typename T> struct WorkList {
  std::unordered_set<id_t> have;
  std::deque<T *> queue;

  void push(T *a) {
    auto id = a->getId();
    if (have.count(id))
      return;
    have.insert(id);
    queue.push_back(a);
  }

  T *pop() {
    if (queue.empty())
      return nullptr;
    auto *a = queue.front();
    queue.pop_front();
    have.erase(a->getId());
    return a;
  }

  template <typename S> WorkList(S *x) : have(), queue() {
    for (T *a : *x) {
      push(a);
    }
  }
};

struct BitSet {
  static constexpr unsigned B = 64;
  static unsigned allocSize(unsigned size) { return (size + B - 1) / B; }

  std::vector<uint64_t> words;

  explicit BitSet(unsigned size) : words(allocSize(size), 0) {}

  BitSet copy(unsigned size) const {
    auto res = BitSet(size);
    std::memcpy(res.words.data(), words.data(), allocSize(size) * (B / 8));
    return res;
  }

  void set(unsigned bit) { words.data()[bit / B] |= (1UL << (bit % B)); }

  bool get(unsigned bit) const {
    return (words.data()[bit / B] & (1UL << (bit % B))) != 0;
  }

  bool equals(const BitSet &other, unsigned size) {
    return std::memcmp(words.data(), other.words.data(), allocSize(size) * (B / 8)) ==
           0;
  }

  void clear(unsigned size) { std::memset(words.data(), 0, allocSize(size) * (B / 8)); }

  void setAll(unsigned size) {
    std::memset(words.data(), 0xff, allocSize(size) * (B / 8));
  }

  void overwrite(const BitSet &other, unsigned size) {
    std::memcpy(words.data(), other.words.data(), allocSize(size) * (B / 8));
  }

  bool applyTransfer(const BitSet &in, const BitSet &kill, const BitSet &gen,
                     unsigned size) {
    bool changed = false;
    auto *out = words.data();
    auto *input = in.words.data();
    auto *killed = kill.words.data();
    auto *generated = gen.words.data();
    auto n = allocSize(size);
    for (unsigned i = 0; i < n; i++) {
      auto next = (input[i] & ~killed[i]) | generated[i];
      changed |= out[i] != next;
      out[i] = next;
    }
    return changed;
  }

  void update(const BitSet &other, unsigned size) {
    auto *p = words.data();
    auto *q = other.words.data();
    auto n = allocSize(size);
    for (unsigned i = 0; i < n; i++) {
      p[i] |= q[i];
    }
  }

  void subtract(const BitSet &other, unsigned size) {
    auto *p = words.data();
    auto *q = other.words.data();
    auto n = allocSize(size);
    for (unsigned i = 0; i < n; i++) {
      p[i] &= ~q[i];
    }
  }
};

template <typename T> struct BlockBitSets {
  T *blk;
  BitSet gen;
  BitSet kill;
  BitSet in;
  BitSet out;

  BlockBitSets(T *blk, BitSet gen, BitSet kill, BitSet in, BitSet out)
      : blk(blk), gen(std::move(gen)), kill(std::move(kill)), in(std::move(in)),
        out(std::move(out)) {}
};
} // namespace

void RDInspector::analyze() {
  std::vector<const Value *> ordering;
  std::unordered_map<id_t, unsigned> lookup;
  std::unordered_map<id_t, std::vector<const Value *>> varToAssignments;

  for (auto *blk : *cfg) {
    for (auto *val : *blk) {
      auto k = getKilled(val);
      if (k != -1) {
        lookup.emplace(val->getId(), ordering.size());
        ordering.push_back(val);
        varToAssignments[k].push_back(val);
      }
    }
  }

  unsigned n = ordering.size();
  std::unordered_map<id_t, BlockBitSets<CFBlock>> bitsets;

  // construct initial gen and kill sets
  for (auto *blk : *cfg) {
    auto gen = BitSet(n);
    auto kill = BitSet(n);

    std::unordered_map<id_t, id_t> generated;
    for (auto *val : *blk) {
      // vars that are used by pointer may change at any time, so don't track them
      if (auto *ptr = cast<PointerValue>(val)) {
        invalid.insert(ptr->getVar()->getId());
        continue;
      }

      auto g = getGenerated(val);
      if (g.first != -1) {
        // generated map will store latest generated assignment, as desired
        generated[g.first] = val->getId();
      }

      auto k = getKilled(val);
      if (k != -1) {
        // all other assignments that use the var are killed
        for (auto *assign : varToAssignments[k]) {
          if (assign->getId() != val->getId())
            kill.set(lookup[assign->getId()]);
        }
      }
    }

    // set gen for the last assignment of each var in the block
    for (auto &entry : generated) {
      gen.set(lookup[entry.second]);
    }

    auto in = BitSet(n);
    auto out = gen.copy(n); // out = gen is an optimization over out = {}
    bitsets.emplace(std::piecewise_construct, std::forward_as_tuple(blk->getId()),
                    std::forward_as_tuple(blk, std::move(gen), std::move(kill),
                                          std::move(in), std::move(out)));
  }

  WorkList<CFBlock> worklist(cfg);
  while (auto *blk = worklist.pop()) {
    auto &data = bitsets.find(blk->getId())->second;

    // IN[blk] = U OUT[pred], for all predecessors pred
    data.in.clear(n);
    for (auto it = blk->predecessors_begin(); it != blk->predecessors_end(); ++it) {
      data.in.update(bitsets.find((*it)->getId())->second.out, n);
    }

    // OUT[blk] = GEN[blk] U (IN[blk] - KILL[blk])
    // Apply OUT = GEN U (IN - KILL) in place.  The former temporary BitSets
    // allocated and copied two vectors for every worklist visit.
    auto changed = data.out.applyTransfer(data.in, data.kill, data.gen, n);

    // if OUT changed, add all successors to worklist
    if (changed) {
      for (auto it = blk->successors_begin(); it != blk->successors_end(); ++it) {
        worklist.push(*it);
      }
    }
  }

  // reconstruct final sets in more convenient format
  for (auto &elem : bitsets) {
    auto &data = elem.second;
    auto &entry = sets[data.blk->getId()];

    for (unsigned i = 0; i < n; i++) {
      if (data.in.get(i)) {
        auto g = getGenerated(ordering[i]);
        entry.in[g.first].insert(g.second);
      }
    }
  }
}

std::vector<ReachingDef> RDInspector::getReachingDefinitions(const Var *var,
                                                             const Value *loc) {
  if (invalid.find(var->getId()) != invalid.end() || var->isGlobal())
    return {};

  auto *blk = cfg->getBlock(loc);
  if (!blk)
    return {};
  auto &entry = sets[blk->getId()];
  if (!entry.indexed) {
    size_t position = 0;
    for (auto *value : *blk) {
      entry.positions.emplace(value->getId(), position);
      auto variable = getKilled(value);
      if (variable != -1)
        entry.assignments[variable].emplace_back(position, value);
      ++position;
    }
    entry.indexed = true;
  }

  auto position = entry.positions.find(loc->getId());
  auto limit = position == entry.positions.end() ? std::numeric_limits<size_t>::max()
                                                 : position->second;
  auto assignments = entry.assignments.find(var->getId());
  if (assignments != entry.assignments.end()) {
    auto &values = assignments->second;
    auto found = std::upper_bound(values.begin(), values.end(), limit,
                                  [](size_t position, const auto &assignment) {
                                    return position < assignment.first;
                                  });
    if (found != values.begin())
      return {getGenerated(std::prev(found)->second).second};
  }

  if (blk == cfg->getEntryBlock())
    return {};

  auto incoming = entry.in.find(var->getId());
  if (incoming == entry.in.end())
    return {};
  return std::vector<ReachingDef>(incoming->second.begin(), incoming->second.end());
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
} // namespace codon
