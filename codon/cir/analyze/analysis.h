// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>

#include "codon/cir/module.h"
#include "codon/cir/transform/pass.h"

namespace codon {
namespace ir {
namespace analyze {

/// Analysis result base struct.
struct Result {
  virtual ~Result() noexcept = default;
};

/// Base class for IR analyses.
class Analysis {
private:
  transform::PassManager *manager = nullptr;

public:
  virtual ~Analysis() noexcept = default;

  /// @return a unique key for this pass
  virtual std::string getKey() const = 0;

  /// Execute the analysis.
  /// @param module the module
  virtual std::unique_ptr<Result> run(const Module *module) = 0;

  /// Sets the manager.
  /// @param mng the new manager
  void setManager(transform::PassManager *mng) { manager = mng; }
  /// Returns the result of a given analysis.
  /// @param key the analysis key
  template <typename AnalysisType>
  AnalysisType *getAnalysisResult(const std::string &key) {
    return static_cast<AnalysisType *>(doGetAnalysis(key));
  }

private:
  analyze::Result *doGetAnalysis(const std::string &key);
};

} // namespace analyze
} // namespace ir
} // namespace codon
