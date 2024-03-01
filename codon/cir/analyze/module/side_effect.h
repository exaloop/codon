// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <unordered_map>

#include "codon/cir/analyze/analysis.h"
#include "codon/cir/util/side_effect.h"

namespace codon {
namespace ir {
namespace analyze {
namespace module {

struct SideEffectResult : public Result {
  /// mapping of ID to corresponding node's side effect status
  std::unordered_map<id_t, util::SideEffectStatus> result;

  SideEffectResult(std::unordered_map<id_t, util::SideEffectStatus> result)
      : result(std::move(result)) {}

  /// @param v the value to check
  /// @return true if the node has side effects (false positives allowed)
  bool hasSideEffect(const Value *v) const;
};

class SideEffectAnalysis : public Analysis {
private:
  /// the capture analysis key
  std::string capAnalysisKey;
  /// true if assigning to a global variable automatically has side effects
  bool globalAssignmentHasSideEffects;

public:
  static const std::string KEY;

  /// Constructs a side effect analysis.
  /// @param globalAssignmentHasSideEffects true if global variable assignment
  /// automatically has side effects
  explicit SideEffectAnalysis(const std::string &capAnalysisKey,
                              bool globalAssignmentHasSideEffects = true)
      : Analysis(), capAnalysisKey(capAnalysisKey),
        globalAssignmentHasSideEffects(globalAssignmentHasSideEffects) {}

  std::string getKey() const override { return KEY; }

  std::unique_ptr<Result> run(const Module *m) override;
};

} // namespace module
} // namespace analyze
} // namespace ir
} // namespace codon
