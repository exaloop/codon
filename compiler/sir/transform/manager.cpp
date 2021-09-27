#include "manager.h"

#include <cassert>
#include <unordered_set>

#include "pass.h"

#include "sir/analyze/analysis.h"
#include "sir/analyze/dataflow/cfg.h"
#include "sir/analyze/dataflow/reaching.h"
#include "sir/analyze/module/global_vars.h"
#include "sir/analyze/module/side_effect.h"

#include "sir/transform/folding/folding.h"
#include "sir/transform/lowering/imperative.h"
#include "sir/transform/lowering/pipeline.h"
#include "sir/transform/manager.h"
#include "sir/transform/parallel/openmp.h"
#include "sir/transform/pythonic/dict.h"
#include "sir/transform/pythonic/io.h"
#include "sir/transform/pythonic/str.h"
#include "util/common.h"

namespace seq {
namespace ir {
namespace transform {

const int PassManager::PASS_IT_MAX = 5;

std::string PassManager::KeyManager::getUniqueKey(const std::string &key) {
  // make sure we can't ever produce duplicate "unique'd" keys
  seqassert(key.find(':') == std::string::npos,
            "pass key '{}' contains invalid character ':'", key);
  auto it = keys.find(key);
  if (it == keys.end()) {
    keys.emplace(key, 1);
    return key;
  } else {
    auto id = ++(it->second);
    return key + ":" + std::to_string(id);
  }
}

std::string PassManager::registerPass(std::unique_ptr<Pass> pass,
                                      const std::string &insertBefore,
                                      std::vector<std::string> reqs,
                                      std::vector<std::string> invalidates) {
  std::string key = pass->getKey();
  if (isDisabled(key))
    return "";
  key = km.getUniqueKey(key);

  for (const auto &req : reqs) {
    assert(deps.find(req) != deps.end());
    deps[req].push_back(key);
  }

  passes.insert(std::make_pair(
      key, PassMetadata(std::move(pass), std::move(reqs), std::move(invalidates))));
  passes[key].pass->setManager(this);
  if (insertBefore.empty()) {
    executionOrder.push_back(key);
  } else {
    auto it = std::find(executionOrder.begin(), executionOrder.end(), insertBefore);
    seqassert(it != executionOrder.end(), "pass with key '{}' not found in manager",
              insertBefore);
    executionOrder.insert(it, key);
  }
  return key;
}

std::string PassManager::registerAnalysis(std::unique_ptr<analyze::Analysis> analysis,
                                          std::vector<std::string> reqs) {

  std::string key = analysis->getKey();
  if (isDisabled(key))
    return "";
  key = km.getUniqueKey(key);

  for (const auto &req : reqs) {
    assert(deps.find(req) != deps.end());
    deps[req].push_back(key);
  }

  analyses.insert(
      std::make_pair(key, AnalysisMetadata(std::move(analysis), std::move(reqs))));
  analyses[key].analysis->setManager(this);
  deps[key] = {};
  return key;
}

void PassManager::run(Module *module) {
  for (auto &p : executionOrder) {
    runPass(module, p);
  }
}

void PassManager::runPass(Module *module, const std::string &name) {
  auto &meta = passes[name];

  auto run = true;
  auto it = 0;

  while (run && it < PASS_IT_MAX) {
    for (auto &dep : meta.reqs) {
      runAnalysis(module, dep);
    }

    meta.pass->run(module);

    for (auto &inv : meta.invalidates)
      invalidate(inv);

    ++it;
    run = meta.pass->shouldRepeat();
  }
}

void PassManager::runAnalysis(Module *module, const std::string &name) {
  if (results.find(name) != results.end())
    return;

  auto &meta = analyses[name];
  for (auto &dep : meta.reqs) {
    runAnalysis(module, dep);
  }
  results[name] = meta.analysis->run(module);
}

void PassManager::invalidate(const std::string &key) {
  std::unordered_set<std::string> open = {key};

  while (!open.empty()) {
    std::unordered_set<std::string> newOpen;
    for (const auto &k : open) {
      if (results.find(k) != results.end()) {
        results.erase(k);
        newOpen.insert(deps[k].begin(), deps[k].end());
      }
    }
    open = std::move(newOpen);
  }
}

void PassManager::registerStandardPasses(bool debug) {
  if (debug) {
    registerPass(std::make_unique<lowering::PipelineLowering>());
    registerPass(std::make_unique<lowering::ImperativeForFlowLowering>());
    registerPass(std::make_unique<parallel::OpenMPPass>());
  } else {
    // Pythonic
    registerPass(std::make_unique<pythonic::DictArithmeticOptimization>());
    registerPass(std::make_unique<pythonic::StrAdditionOptimization>());
    registerPass(std::make_unique<pythonic::IOCatOptimization>());

    // lowering
    registerPass(std::make_unique<lowering::PipelineLowering>());
    registerPass(std::make_unique<lowering::ImperativeForFlowLowering>());

    // folding
    auto seKey1 =
        registerAnalysis(std::make_unique<analyze::module::SideEffectAnalysis>(
            /*globalAssignmentHasSideEffects=*/true));
    auto seKey2 =
        registerAnalysis(std::make_unique<analyze::module::SideEffectAnalysis>(
            /*globalAssignmentHasSideEffects=*/false));
    auto cfgKey = registerAnalysis(std::make_unique<analyze::dataflow::CFAnalysis>());
    auto rdKey = registerAnalysis(
        std::make_unique<analyze::dataflow::RDAnalysis>(cfgKey), {cfgKey});
    auto globalKey =
        registerAnalysis(std::make_unique<analyze::module::GlobalVarsAnalyses>());
    registerPass(std::make_unique<folding::FoldingPassGroup>(
                     seKey1, rdKey, globalKey, /*runGlobalDemoton=*/false),
                 /*insertBefore=*/"", {seKey1, rdKey, globalKey},
                 {seKey1, rdKey, cfgKey, globalKey});

    // parallel
    registerPass(std::make_unique<parallel::OpenMPPass>());

    registerPass(std::make_unique<folding::FoldingPassGroup>(seKey2, rdKey, globalKey,
                                                             /*runGlobalDemoton=*/true),
                 /*insertBefore=*/"", {seKey2, rdKey, globalKey},
                 {seKey2, rdKey, cfgKey, globalKey});
  }
}

} // namespace transform
} // namespace ir
} // namespace seq
