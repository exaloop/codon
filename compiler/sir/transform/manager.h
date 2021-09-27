#pragma once

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "pass.h"
#include "sir/analyze/analysis.h"
#include "sir/module.h"

namespace seq {
namespace ir {
namespace transform {

/// Utility class to run a series of passes.
class PassManager {
private:
  /// Manager for keys of passes.
  class KeyManager {
  private:
    /// mapping of raw key to number of occurences
    std::unordered_map<std::string, int> keys;

  public:
    KeyManager() = default;
    /// Returns a unique'd key for a given raw key.
    /// Does so by appending ":<number>" if the key
    /// has been seen.
    /// @param key the raw key
    /// @return the unique'd key
    std::string getUniqueKey(const std::string &key);
  };

  /// Container for pass metadata.
  struct PassMetadata {
    /// pointer to the pass instance
    std::unique_ptr<Pass> pass;
    /// vector of required analyses
    std::vector<std::string> reqs;
    /// vector of invalidated analyses
    std::vector<std::string> invalidates;

    PassMetadata() = default;
    PassMetadata(std::unique_ptr<Pass> pass, std::vector<std::string> reqs,
                 std::vector<std::string> invalidates)
        : pass(std::move(pass)), reqs(std::move(reqs)),
          invalidates(std::move(invalidates)) {}
    PassMetadata(PassMetadata &&) = default;

    PassMetadata &operator=(PassMetadata &&) = default;
  };

  /// Container for analysis metadata.
  struct AnalysisMetadata {
    /// pointer to the analysis instance
    std::unique_ptr<analyze::Analysis> analysis;
    /// vector of required analyses
    std::vector<std::string> reqs;
    /// vector of invalidated analyses
    std::vector<std::string> invalidates;

    AnalysisMetadata() = default;
    AnalysisMetadata(std::unique_ptr<analyze::Analysis> analysis,
                     std::vector<std::string> reqs)
        : analysis(std::move(analysis)), reqs(std::move(reqs)) {}
    AnalysisMetadata(AnalysisMetadata &&) = default;

    AnalysisMetadata &operator=(AnalysisMetadata &&) = default;
  };

  /// key manager to handle duplicate keys (i.e. passes being added twice)
  KeyManager km;

  /// map of keys to passes
  std::unordered_map<std::string, PassMetadata> passes;
  /// map of keys to analyses
  std::unordered_map<std::string, AnalysisMetadata> analyses;
  /// reverse dependency map
  std::unordered_map<std::string, std::vector<std::string>> deps;

  /// execution order of passes
  std::vector<std::string> executionOrder;
  /// map of valid analysis results
  std::unordered_map<std::string, std::unique_ptr<analyze::Result>> results;

  /// passes to avoid registering
  std::vector<std::string> disabled;

public:
  /// PassManager initialization mode.
  enum Init {
    EMPTY,
    DEBUG,
    RELEASE,
  };

  static const int PASS_IT_MAX;

  explicit PassManager(Init init, std::vector<std::string> disabled = {})
      : km(), passes(), analyses(), executionOrder(), results(),
        disabled(std::move(disabled)) {
    switch (init) {
    case Init::EMPTY:
      break;
    case Init::DEBUG:
      registerStandardPasses(true);
      break;
    case Init::RELEASE:
      registerStandardPasses(false);
      break;
    }
  }

  explicit PassManager(bool debug = false, std::vector<std::string> disabled = {})
      : PassManager(debug ? Init::DEBUG : Init::RELEASE, std::move(disabled)) {}

  /// Registers a pass and appends it to the execution order.
  /// @param pass the pass
  /// @param insertBefore insert pass before the pass with this given key
  /// @param reqs keys of passes that must be run before the current one
  /// @param invalidates keys of passes that are invalidated by the current one
  /// @return unique'd key for the added pass, or empty string if not added
  std::string registerPass(std::unique_ptr<Pass> pass,
                           const std::string &insertBefore = "",
                           std::vector<std::string> reqs = {},
                           std::vector<std::string> invalidates = {});

  /// Registers an analysis.
  /// @param analysis the analysis
  /// @param reqs keys of analyses that must be run before the current one
  /// @return unique'd key for the added analysis, or empty string if not added
  std::string registerAnalysis(std::unique_ptr<analyze::Analysis> analysis,
                               std::vector<std::string> reqs = {});

  /// Run all passes.
  /// @param module the module
  void run(Module *module);

  /// Gets the result of a given analysis.
  /// @param key the (unique'd) analysis key
  /// @return the result
  analyze::Result *getAnalysisResult(const std::string &key) {
    auto it = results.find(key);
    return it != results.end() ? it->second.get() : nullptr;
  }

  /// Returns whether a given pass or analysis is disabled.
  /// @param key the (unique'd) pass or analysis key
  /// @return true if the pass or analysis is disabled
  bool isDisabled(const std::string &key) {
    return std::find(disabled.begin(), disabled.end(), key) != disabled.end();
  }

private:
  void runPass(Module *module, const std::string &name);
  void registerStandardPasses(bool debug = false);
  void runAnalysis(Module *module, const std::string &name);
  void invalidate(const std::string &key);
};

} // namespace transform
} // namespace ir
} // namespace seq
