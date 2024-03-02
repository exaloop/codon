// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/module.h"
#include "codon/cir/util/operator.h"

namespace codon {
namespace ir {

namespace analyze {
struct Result;
}

namespace transform {

class PassManager;

/// General pass base class.
class Pass {
private:
  PassManager *manager = nullptr;

public:
  virtual ~Pass() noexcept = default;

  /// @return a unique key for this pass
  virtual std::string getKey() const = 0;

  /// Execute the pass.
  /// @param module the module
  virtual void run(Module *module) = 0;

  /// Determine if pass should repeat.
  /// @param num how many times this pass has already run
  /// @return true if pass should repeat
  virtual bool shouldRepeat(int num) const { return false; }

  /// Sets the manager.
  /// @param mng the new manager
  virtual void setManager(PassManager *mng) { manager = mng; }
  /// Returns the result of a given analysis.
  /// @param key the analysis key
  /// @return the analysis result
  template <typename AnalysisType>
  AnalysisType *getAnalysisResult(const std::string &key) {
    return static_cast<AnalysisType *>(doGetAnalysis(key));
  }

private:
  analyze::Result *doGetAnalysis(const std::string &key);
};

class PassGroup : public Pass {
private:
  int repeat;
  std::vector<std::unique_ptr<Pass>> passes;

public:
  explicit PassGroup(int repeat = 0, std::vector<std::unique_ptr<Pass>> passes = {})
      : Pass(), repeat(repeat), passes(std::move(passes)) {}

  virtual ~PassGroup() noexcept = default;

  void push_back(std::unique_ptr<Pass> p) { passes.push_back(std::move(p)); }

  /// @return default number of times pass should repeat
  int getRepeat() const { return repeat; }

  /// Sets the default number of times pass should repeat.
  /// @param r number of repeats
  void setRepeat(int r) { repeat = r; }

  bool shouldRepeat(int num) const override { return num < repeat; }

  void run(Module *module) override;

  void setManager(PassManager *mng) override;
};

/// Pass that runs a single Operator.
class OperatorPass : public Pass, public util::Operator {
public:
  /// Constructs an operator pass.
  /// @param childrenFirst true if children should be iterated first
  explicit OperatorPass(bool childrenFirst = false) : util::Operator(childrenFirst) {}

  void run(Module *module) override {
    reset();
    process(module);
  }
};

} // namespace transform
} // namespace ir
} // namespace codon
