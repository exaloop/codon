// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <utility>

#include "codon/cir/analyze/analysis.h"
#include "codon/cir/analyze/dataflow/cfg.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

/// Single answer to a reaching definition query
struct ReachingDef {
  /// Assignment instruction, which can be a `AssignInstr` or
  /// e.g. a `SyntheticAssignInstr` to represent loop variable
  /// assignment etc.
  const Instr *assignment;
  /// The value being assigned, or null if unknown. The assigned
  /// value is unknown when, for example, assigning the next value
  /// of a loop variable.
  const Value *assignee;

  explicit ReachingDef(const Instr *assignment, const Value *assignee = nullptr)
      : assignment(assignment), assignee(assignee) {}

  bool known() const { return assignee != nullptr; }

  id_t getId() const { return known() ? assignee->getId() : assignment->getId(); }

  bool operator==(const ReachingDef &other) const {
    if (known() != other.known())
      return false;

    return known() ? (assignee->getId() == other.assignee->getId())
                   : (assignment->getId() == other.assignment->getId());
  }
};

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon

namespace std {
template <> struct hash<codon::ir::analyze::dataflow::ReachingDef> {
  size_t operator()(const codon::ir::analyze::dataflow::ReachingDef &d) const {
    return d.known() ? hash<id_t>{}(d.assignee->getId())
                     : hash<id_t>{}(d.assignment->getId());
  }
};
} // namespace std

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

/// Helper to query the reaching definitions of a particular function.
class RDInspector {
private:
  struct BlockData {
    std::unordered_map<id_t, std::unordered_set<ReachingDef>> in;
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
  /// @return a vector of reaching definitions
  std::vector<ReachingDef> getReachingDefinitions(const Var *var, const Value *loc);

  bool isInvalid(const Var *var) const { return invalid.count(var->getId()) != 0; }
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
} // namespace codon
