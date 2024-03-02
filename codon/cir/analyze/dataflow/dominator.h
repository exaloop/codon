// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <set>
#include <unordered_map>
#include <utility>

#include "codon/cir/analyze/analysis.h"
#include "codon/cir/analyze/dataflow/cfg.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

/// Helper to query the dominators of a particular function.
class DominatorInspector {
private:
  std::unordered_map<id_t, std::set<id_t>> sets;
  CFGraph *cfg;

public:
  explicit DominatorInspector(CFGraph *cfg) : cfg(cfg) {}

  /// Do the analysis.
  void analyze();

  /// Checks if one value dominates another.
  /// @param v the value
  /// @param dominator the dominator value
  bool isDominated(const Value *v, const Value *dominator);
};

/// Result of a dominator analysis.
struct DominatorResult : public Result {
  /// the corresponding control flow result
  const CFResult *cfgResult;
  /// the dominator inspectors
  std::unordered_map<id_t, std::unique_ptr<DominatorInspector>> results;

  explicit DominatorResult(const CFResult *cfgResult) : cfgResult(cfgResult) {}
};

/// Dominator analysis. Must have control flow-graph available.
class DominatorAnalysis : public Analysis {
private:
  /// the control-flow analysis key
  std::string cfAnalysisKey;

public:
  static const std::string KEY;

  /// Initializes a dominator analysis.
  /// @param cfAnalysisKey the control-flow analysis key
  explicit DominatorAnalysis(std::string cfAnalysisKey)
      : cfAnalysisKey(std::move(cfAnalysisKey)) {}

  std::string getKey() const override { return KEY; }

  std::unique_ptr<Result> run(const Module *m) override;
};

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon
