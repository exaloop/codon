#include "test.h"

#include <map>
#include <set>

#include "codon/cir/analyze/module/side_effect.h"
#include "codon/cir/transform/cleanup/canonical.h"
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

class SkippedPass : public DummyPass {
public:
  using DummyPass::DummyPass;
  bool shouldRun(Module *) const override { return false; }
};

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
def nullable_union(flag: bool) -> Union[int, str, NoneType]:
  if flag:
    return "present"
@export
def optional_union(flag: bool) -> Optional[Union[int, str]]:
  if flag:
    return "present"
@export
def required_union(flag: bool) -> Union[int, str]:
  if flag:
    return "present"
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
                                     "nullable_union",
                                     "optional_union",
                                     "required_union",
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
                                         {"required_union", 1},
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

TEST(MissingReturnTest, ConstructsNullForNoncanonicalUnionLayouts) {
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("union_null.codon", "pass\n"));
  auto *module = compiler.getModule();
  auto *integer = module->getIntType();
  auto *optional = module->getOptionalType(module->getStringType());
  auto *reference = module->getOrRealizeType("List", {integer});
  ASSERT_NE(reference, nullptr);
  auto *nested = module->getUnionType({integer, module->getOptionalType(reference)});
  struct Case {
    std::string name;
    Type *type;
    std::vector<int> tags;
    BodiedFunc *function = nullptr;
  };
  std::vector<Case> cases = {
      {"null_first",
       module->getUnionType({module->getOptionalType(integer), optional}),
       {0}},
      {"null_last", module->getUnionType({integer, optional}), {1}},
      {"null_reference", nested, {1}},
      {"null_none", module->getUnionType({integer, module->getNoneType()}), {1}},
      {"null_nested", module->getUnionType({integer, nested}), {1, 1}}};
  for (auto &fixture : cases) {
    fixture.function = module->Nr<BodiedFunc>(fixture.name);
    fixture.function->realize(
        module->unsafeGetFuncType(fixture.name + ".signature", fixture.type, {}), {});
    fixture.function->setBody(module->Nr<SeriesFlow>());
  }

  transform::lowering::MissingReturnLowering pass;
  pass.run(module);
  auto functionCount = std::distance(module->begin(), module->end());
  pass.run(module);
  EXPECT_EQ(std::distance(module->begin(), module->end()), functionCount);
  for (const auto &fixture : cases) {
    SCOPED_TRACE(fixture.name);
    auto *body = cast<SeriesFlow>(fixture.function->getBody());
    ASSERT_NE(body, nullptr);
    ASSERT_EQ(std::distance(body->begin(), body->end()), 1);
    auto *returned = cast<ReturnInstr>(body->front());
    ASSERT_NE(returned, nullptr);
    auto *value = returned->getValue();
    auto *type = fixture.type;
    for (auto tag : fixture.tags) {
      auto *call = cast<CallInstr>(value);
      ASSERT_NE(call, nullptr);
      ASSERT_EQ(call->numArgs(), 2);
      EXPECT_EQ(call->getType(), type);
      auto *member = *std::next(cast<UnionType>(type)->begin(), tag);
      EXPECT_EQ(util::getFunc(call->getCallee()),
                module->getOrRealizeMethod(type, "_make", {integer, member}, {type}));
      auto *actualTag = cast<IntConst>(call->front());
      ASSERT_NE(actualTag, nullptr);
      EXPECT_EQ(actualTag->getVal(), tag);
      value = call->back();
      type = member;
    }
    auto *call = cast<CallInstr>(value);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->getType(), type);
    EXPECT_EQ(call->numArgs(), 0);
    EXPECT_EQ(util::getFunc(call->getCallee()),
              module->getOrRealizeMethod(type, "__new__", {}));
  }
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

TEST_F(CIRCoreTest, PassManagerSkipsInapplicablePasses) {
  int counter = 0;
  auto options = codon::Options::getDefault("");
  options->pmempty = true;
  transform::PassManager manager(options.get());
  manager.registerAnalysis(std::make_unique<DummyAnalysis>(counter));
  manager.registerPass(std::make_unique<SkippedPass>(counter, ANALYSIS_KEY), "",
                       {ANALYSIS_KEY}, {ANALYSIS_KEY});
  manager.run(module.get());
  EXPECT_EQ(counter, 0);

  manager.registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                       {ANALYSIS_KEY});
  manager.registerPass(std::make_unique<SkippedPass>(counter, ANALYSIS_KEY), "",
                       {ANALYSIS_KEY}, {ANALYSIS_KEY});
  manager.registerPass(std::make_unique<DummyPass>(counter, ANALYSIS_KEY), "",
                       {ANALYSIS_KEY});
  manager.run(module.get());
  EXPECT_EQ(counter, 3);
  EXPECT_EQ(DummyAnalysis::runCounter, 0);
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

TEST_F(CIRCoreTest, CanonicalizationKeepsUnchangedCalls) {
  struct PureAnalysis : analyze::Analysis {
    std::string getKey() const override { return ANALYSIS_KEY; }
    std::unique_ptr<analyze::Result> run(const Module *module) override {
      std::unordered_map<codon::ir::id_t, util::SideEffectStatus> result;
      for (auto it = module->values_begin(); it != module->values_end(); ++it)
        result[(*it)->getId()] = util::PURE;
      return std::make_unique<analyze::module::SideEffectResult>(std::move(result));
    }
  };
  auto *function = module->Nr<BodiedFunc>("int.combine");
  function->setUnmangledName("combine");
  function->setParentType(module->getIntType());
  function->realize(
      module->unsafeGetFuncType("combine", module->getIntType(),
                                {module->getIntType(), module->getIntType()}),
      {"left", "right"});
  auto *call = util::call(function, {module->getInt(1), module->getInt(2)});
  auto *body = module->Nr<SeriesFlow>();
  body->push_back(call);
  cast<BodiedFunc>(module->getMainFunc())->setBody(body);
  auto options = codon::Options::getDefault("");
  options->pmempty = true;
  transform::PassManager manager(options.get());
  manager.registerAnalysis(std::make_unique<PureAnalysis>());
  manager.registerPass(
      std::make_unique<transform::cleanup::CanonicalizationPass>(ANALYSIS_KEY), "",
      {ANALYSIS_KEY}, {ANALYSIS_KEY});
  auto originalSize = std::distance(module->values_begin(), module->values_end());
  manager.run(module.get());
  EXPECT_EQ(std::distance(module->values_begin(), module->values_end()), originalSize);
  manager.run(module.get());
  EXPECT_EQ(std::distance(module->values_begin(), module->values_end()), originalSize);
  EXPECT_EQ(body->front(), call);

  auto attributes = std::make_unique<KeyValueAttribute>();
  attributes->attributes[codon::ast::getMangledFunc("std.internal.attributes",
                                                    "associative")] = "";
  function->setAttribute(std::move(attributes));
  auto *third = module->getInt(3);
  auto *leftChain = util::call(function, {call, third});
  body->push_back(leftChain);
  originalSize = std::distance(module->values_begin(), module->values_end());
  manager.run(module.get());
  EXPECT_EQ(std::distance(module->values_begin(), module->values_end()), originalSize);

  auto *rightChain = util::call(
      function, {call->front(), util::call(function, {call->back(), third})});
  body->push_back(rightChain);
  manager.run(module.get());
  auto *canonical = cast<CallInstr>(rightChain);
  ASSERT_NE(canonical, rightChain);
  auto *left = cast<CallInstr>(canonical->front());
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->front(), call->front());
  EXPECT_EQ(left->back(), call->back());
  EXPECT_EQ(canonical->back(), third);

  auto *named = util::call(function, {call->front(), call->back()});
  named->setName("named-call");
  body->push_back(named);
  manager.run(module.get());
  EXPECT_TRUE(named->getName().empty());
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
