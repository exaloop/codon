// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "codon/cir/analyze/analysis.h"
#include "codon/cir/analyze/dataflow/dominator.h"
#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/cir.h"

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
  /// @param type the argument's type
  /// @return an instance denoting unknown capture status
  static CaptureInfo unknown(const Func *func, types::Type *type);
};

/// Capture analysis result.
struct CaptureResult : public Result {
  /// the corresponding reaching definitions result
  RDResult *rdResult = nullptr;

  /// the corresponding dominator result
  DominatorResult *domResult = nullptr;

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
  /// the dominator analysis key
  std::string domAnalysisKey;

public:
  static const std::string KEY;
  std::string getKey() const override { return KEY; }

  /// Initializes a capture analysis.
  /// @param rdAnalysisKey the reaching definitions analysis key
  /// @param domAnalysisKey the dominator analysis key
  explicit CaptureAnalysis(std::string rdAnalysisKey, std::string domAnalysisKey)
      : rdAnalysisKey(std::move(rdAnalysisKey)),
        domAnalysisKey(std::move(domAnalysisKey)) {}

  std::unique_ptr<Result> run(const Module *m) override;
};

CaptureInfo escapes(const BodiedFunc *func, const Value *value, CaptureResult *cr);

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon
