#include "test.h"

#include "codon/cir/transform/manager.h"
#include "codon/cir/transform/pass.h"

using namespace codon::ir;

std::string ANALYSIS_KEY = "**test_analysis**";
std::string PASS_KEY = "**test_pass**";

class DummyResult : public analyze::Result {};

class DummyAnalysis : public analyze::Analysis {
private:
  int &counter;

public:
  static int runCounter;

  explicit DummyAnalysis(int &counter) : counter(counter) {}

  std::string getKey() const override { return ANALYSIS_KEY; }

  std::unique_ptr<analyze::Result> run(const Module *) override {
    runCounter = counter++;
    return std::make_unique<DummyResult>();
  }
};

int DummyAnalysis::runCounter = 0;

class DummyPass : public transform::Pass {
private:
  int &counter;
  std::string required;

public:
  static int runCounter;

  explicit DummyPass(int &counter, std::string required)
      : counter(counter), required(std::move(required)) {}

  std::string getKey() const override { return PASS_KEY; }

  void run(Module *) override {
    runCounter = counter++;
    ASSERT_TRUE(getAnalysisResult<DummyResult>(required));
  }
};

int DummyPass::runCounter = 0;

TEST_F(CIRCoreTest, PassManagerNoInvalidations) {
  int counter = 0;

  auto manager =
      std::make_unique<transform::PassManager>(transform::PassManager::Init::EMPTY);
  manager->registerAnalysis(std::make_unique<DummyAnalysis>(counter));
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY});
  manager->run(module.get());

  ASSERT_EQ(0, DummyAnalysis::runCounter);
  ASSERT_EQ(1, DummyPass::runCounter);
}

TEST_F(CIRCoreTest, PassManagerInvalidations) {
  int counter = 0;

  auto manager =
      std::make_unique<transform::PassManager>(transform::PassManager::Init::EMPTY);
  manager->registerAnalysis(std::make_unique<DummyAnalysis>(counter));
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY}, {ANALYSIS_KEY});
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY});

  manager->run(module.get());

  ASSERT_EQ(2, DummyAnalysis::runCounter);
  ASSERT_EQ(3, DummyPass::runCounter);
}
