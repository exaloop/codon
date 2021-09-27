#pragma once

#include <unordered_map>

#include "sir/analyze/analysis.h"

namespace seq {
namespace ir {
namespace analyze {
namespace module {

struct SideEffectResult : public Result {
  /// mapping of ID to bool indicating whether the node has side effects
  std::unordered_map<id_t, bool> result;

  SideEffectResult(std::unordered_map<id_t, bool> result) : result(std::move(result)) {}

  /// @param v the value to check
  /// @return true if the node has side effects (false positives allowed)
  bool hasSideEffect(Value *v) const;
};

class SideEffectAnalysis : public Analysis {
private:
  /// true if assigning to a global variable automatically has side effects
  bool globalAssignmentHasSideEffects;

public:
  static const std::string KEY;

  /// Constructs a side effect analysis.
  /// @param globalAssignmentHasSideEffects true if global variable assignment
  /// automatically has side effects
  explicit SideEffectAnalysis(bool globalAssignmentHasSideEffects = true)
      : Analysis(), globalAssignmentHasSideEffects(globalAssignmentHasSideEffects){};

  std::string getKey() const override { return KEY; }

  std::unique_ptr<Result> run(const Module *m) override;
};

} // namespace module
} // namespace analyze
} // namespace ir
} // namespace seq
