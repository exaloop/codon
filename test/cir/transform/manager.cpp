#include "test.h"

#include <map>
#include <set>

#include "codon/cir/transform/lowering/imperative.h"
#include "codon/cir/transform/lowering/return.h"
#include "codon/cir/transform/manager.h"
#include "codon/cir/transform/pass.h"
#include "codon/cir/util/irtools.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

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

TEST(MissingReturnTest, GuardsOnlyReachableNonnullableFallthrough) {
  auto options = codon::Options::getDefault("build/codon_test");
  options->debug = false;
  codon::Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("missing_return.codon", R"(
def error():
  raise ValueError("original")
def error_wrapper():
  error()
@export
def required(flag: bool) -> int:
  if flag:
    return 17
@export
def complete(flag: bool) -> int:
  if flag:
    return 17
  else:
    return 18
@export
def nullable(flag: bool) -> Optional[int]:
  if flag:
    return 17
@export
def nothing(flag: bool):
  if flag:
    print(flag)
@export
def calls_error(flag: bool) -> int:
  if flag:
    return 17
  error_wrapper()
@export
def caught_error() -> int:
  try:
    error_wrapper()
  except ValueError:
    pass
@export
def finally_returns(flag: bool) -> int:
  try:
    if flag:
      return 17
  finally:
    return 18
@export
def finally_breaks() -> int:
  while True:
    try:
      return 17
    finally:
      break
@export
def forever() -> int:
  while True:
    continue
def throwing_generator():
  error()
  yield 17
@export
def creates_generator() -> int:
  throwing_generator()
@export
def protected_return(flag: bool) -> int:
  try:
    if flag:
      error()
    return 17
  except ValueError:
    return 18
@export
def nonempty_loop() -> int:
  for index in range(1):
    return index
@export
def possibly_empty_loop(count: int) -> int:
  for index in range(count):
    return index
@export
def caught_return_expression() -> int:
  try:
    return int("bad")
  except ValueError:
    pass
@export
def complete_else(flag: bool) -> int:
  try:
    if flag:
      error()
  except ValueError:
    return 17
  else:
    return 18
)"));
  transform::lowering::ImperativeForFlowLowering().run(compiler.getModule());
  transform::lowering::MissingReturnLowering pass;
  pass.run(compiler.getModule());
  struct Inspector : public util::Operator {
    std::map<std::string, int> guarded;
    std::set<std::string> fixture = {"required",
                                     "complete",
                                     "nullable",
                                     "nothing",
                                     "calls_error",
                                     "caught_error",
                                     "finally_returns",
                                     "finally_breaks",
                                     "forever",
                                     "creates_generator",
                                     "protected_return",
                                     "nonempty_loop",
                                     "possibly_empty_loop",
                                     "caught_return_expression",
                                     "complete_else"};
    void handle(CallInstr *call) override {
      auto *callee = util::getFunc(call->getCallee());
      if (callee && callee->getUnmangledName() == "_missing_return_error" &&
          fixture.count(getParentFunc()->getUnmangledName()))
        ++guarded[getParentFunc()->getUnmangledName()];
    }
  } inspector;
  inspector.process(compiler.getModule());
  std::map<std::string, int> expected = {{"required", 1},
                                         {"caught_error", 1},
                                         {"finally_breaks", 1},
                                         {"creates_generator", 1},
                                         {"possibly_empty_loop", 1},
                                         {"caught_return_expression", 1}};
  EXPECT_EQ(inspector.guarded, expected);
  pass.run(compiler.getModule());
  Inspector repeated;
  repeated.process(compiler.getModule());
  EXPECT_EQ(repeated.guarded, expected);
}

TEST_F(CIRCoreTest, PassManagerNoInvalidations) {
  int counter = 0;
  auto options = codon::Options::getDefault("");
  options->pmempty = true;
  auto manager = std::make_unique<transform::PassManager>(options.get());
  manager->registerAnalysis(std::make_unique<DummyAnalysis>(counter));
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY});
  manager->run(module.get());

  ASSERT_EQ(0, DummyAnalysis::runCounter);
  ASSERT_EQ(1, DummyPass::runCounter);
}

TEST_F(CIRCoreTest, PassManagerInvalidations) {
  int counter = 0;
  auto options = codon::Options::getDefault("");
  options->pmempty = true;
  auto manager = std::make_unique<transform::PassManager>(options.get());
  manager->registerAnalysis(std::make_unique<DummyAnalysis>(counter));
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY}, {ANALYSIS_KEY});
  manager->registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                        {ANALYSIS_KEY});

  manager->run(module.get());

  ASSERT_EQ(2, DummyAnalysis::runCounter);
  ASSERT_EQ(3, DummyPass::runCounter);
}

TEST(CIRFoldingTest, FoldsSideEffectFreeSequencesBeforeLLVM) {
  auto options = codon::Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;
  codon::Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("folding_pure_sequence.codon", R"(
@export
def fold_variable(value: int) -> int:
    return 1 + (0 * value) + 2

@export
def fold_right_variable(value: int) -> int:
    return 1 + (value * 0) + 2

def pure(value: int) -> int:
    return value + 1

@export
def fold_call(value: int) -> int:
    return 1 + (0 * pure(value)) + 2

def consume(value: int) -> int:
    print(value)
    return value

@export
def fold_argument(value: int) -> int:
    return consume(1 + (0 * value) + 2)

@export
def fold_assignment(value: int) -> int:
    result = 1 + (0 * value) + 2
    consume(result)
    return result

def record(counter: Ptr[int]) -> int:
    counter[0] += 1
    return counter[0]

@export
def fold_effect(counter: Ptr[int]) -> int:
    return 1 + (0 * record(counter)) + 2
)"));
  compiler.getPassManager()->run(compiler.getModule());

  struct Inspector : public util::Operator {
    int returns = 0;
    int arguments = 0;
    int effects = 0;

    void handle(ReturnInstr *instruction) override {
      auto name = getParentFunc()->getUnmangledName();
      if (name != "fold_variable" && name != "fold_right_variable" &&
          name != "fold_call" && name != "fold_assignment")
        return;
      ++returns;
      SCOPED_TRACE(name);
      auto *constant = cast<IntConst>(instruction->getValue());
      ASSERT_NE(constant, nullptr);
      EXPECT_EQ(constant->getVal(), 3);
    }

    void handle(CallInstr *instruction) override {
      auto *function = util::getFunc(instruction->getCallee());
      if (function && function->getUnmangledName() == "record")
        ++effects;
      if (!function || function->getUnmangledName() != "consume")
        return;
      ++arguments;
      auto *constant = cast<IntConst>(instruction->front());
      ASSERT_NE(constant, nullptr);
      EXPECT_EQ(constant->getVal(), 3);
    }
  } inspector;
  inspector.process(compiler.getModule());
  EXPECT_EQ(inspector.returns, 4);
  EXPECT_EQ(inspector.arguments, 2);
  EXPECT_EQ(inspector.effects, 1);
}
