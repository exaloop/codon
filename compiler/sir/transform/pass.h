#pragma once

#include "sir/module.h"
#include "sir/util/operator.h"

namespace seq {
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

  /// @return true if pass should repeat
  virtual bool shouldRepeat() const { return false; }

  /// Sets the manager.
  /// @param mng the new manager
  virtual void setManager(PassManager *mng) { manager = mng; }
  /// Returns the result of a given analysis.
  /// @param key the analysis key
  template <typename AnalysisType>
  const AnalysisType *getAnalysisResult(const std::string &key) {
    return static_cast<const AnalysisType *>(doGetAnalysis(key));
  }

private:
  const analyze::Result *doGetAnalysis(const std::string &key);
};

class PassGroup : public Pass {
private:
  std::vector<std::unique_ptr<Pass>> passes;

public:
  explicit PassGroup(std::vector<std::unique_ptr<Pass>> passes = {})
      : passes(std::move(passes)) {}

  virtual ~PassGroup() noexcept = default;

  void push_back(std::unique_ptr<Pass> p) { passes.push_back(std::move(p)); }

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
} // namespace seq
