#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/sir/analyze/analysis.h"
#include "codon/sir/analyze/dataflow/reaching.h"
#include "codon/sir/sir.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {

/// Information about how a function argument is captured.
struct CaptureInfo {
  /// vector of other argument indices capturing this one
  std::vector<unsigned> argCaptures;
  /// true if the return value of the function captures this argument
  bool returnCaptures = false;
  /// true if this argument is externally captured e.g. by assignment to global
  bool externCaptures = false;
  /// true if this argument is modified
  bool modified = false;

  /// @return true if anything captures
  operator bool() const {
    return !argCaptures.empty() || returnCaptures || externCaptures;
  }

  /// Returns an instance denoting no captures.
  /// @return an instance denoting no captures
  static CaptureInfo nothing() { return {}; }

  /// Returns an instance denoting unknown capture status.
  /// @param func the function containing this argument
  /// @param arg the argument itself
  /// @return an instance denoting unknown capture status
  static CaptureInfo unknown(Func *func, Var *arg);
};

/// Capture analysis result.
struct CaptureResult : public Result {
  /// the corresponding reaching definitions result
  RDResult *rdResult = nullptr;

  /// map from function id to capture information, where
  /// each element of the value vector corresponds to an
  /// argument of the function
  std::unordered_map<id_t, std::vector<CaptureInfo>> results;
};

/// Capture analysis that runs on all functions.
class CaptureAnalysis : public Analysis {
private:
  /// the reaching definitions analysis key
  std::string rdAnalysisKey;

public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  /// Initializes a capture analysis.
  /// @param rdAnalysisKey the reaching definitions analysis key
  explicit CaptureAnalysis(std::string rdAnalysisKey)
      : rdAnalysisKey(std::move(rdAnalysisKey)) {}

  std::unique_ptr<Result> run(const Module *m) override;
};

CaptureInfo escapes(BodiedFunc *func, Value *value, CaptureResult *cr);

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon
