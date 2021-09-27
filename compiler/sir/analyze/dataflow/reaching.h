#pragma once

#include <utility>

#include "sir/analyze/analysis.h"
#include "sir/analyze/dataflow/cfg.h"

namespace seq {
namespace ir {
namespace analyze {
namespace dataflow {

/// Helper to query the reaching definitions of a particular function.
class RDInspector {
private:
  struct BlockData {
    std::unordered_map<id_t, std::unordered_set<id_t>> in;
    std::unordered_map<id_t, std::unordered_set<id_t>> out;
    std::unordered_set<id_t> killed;
    std::unordered_map<id_t, id_t> generated;
    bool initialized = false;

    BlockData() = default;
  };
  std::unordered_set<id_t> invalid;
  std::unordered_map<id_t, BlockData> sets;
  CFGraph *cfg;

public:
  explicit RDInspector(CFGraph *cfg) : cfg(cfg) {}

  /// Do the analysis.
  void analyze();

  /// Gets the reaching definitions at a particular location.
  /// @param var the variable being inspected
  /// @param loc the location
  /// @return an unordered set of value ids
  std::unordered_set<id_t> getReachingDefinitions(Var *var, Value *loc);

private:
  void initializeIfNecessary(CFBlock *blk);

  void calculateIn(CFBlock *blk);
  bool calculateOut(CFBlock *blk);
};

/// Result of a reaching definition analysis.
struct RDResult : public Result {
  /// the corresponding control flow result
  const CFResult *cfgResult;
  /// the reaching definition inspectors
  std::unordered_map<id_t, std::unique_ptr<RDInspector>> results;

  explicit RDResult(const CFResult *cfgResult) : cfgResult(cfgResult) {}
};

/// Reaching definition analysis. Must have control flow-graph available.
class RDAnalysis : public Analysis {
private:
  /// the control-flow analysis key
  std::string cfAnalysisKey;

public:
  static const std::string KEY;

  /// Initializes a reaching definition analysis.
  /// @param cfAnalysisKey the control-flow analysis key
  explicit RDAnalysis(std::string cfAnalysisKey)
      : cfAnalysisKey(std::move(cfAnalysisKey)) {}

  std::string getKey() const override { return KEY; }

  std::unique_ptr<Result> run(const Module *m) override;
};

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace seq
