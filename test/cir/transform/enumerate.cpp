#include "test.h"

#include <unordered_map>

#include "codon/cir/transform/lowering/imperative.h"
#include "codon/cir/transform/pythonic/enumerate.h"
#include "codon/cir/util/irtools.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

using namespace codon;

namespace {
struct LoopCounts {
  int generators = 0;
  int indexed = 0;
  int enumerates = 0;
  int globalTargets = 0;
  int virtualLookups = 0;
};

class EnumerateInspector : public ir::util::Operator {
public:
  std::unordered_map<std::string, LoopCounts> counts;
  bool markSchedules = false;

  void handle(ir::ForFlow *loop) override {
    auto name = getParentFunc()->getUnmangledName();
    ++counts[name].generators;
    if (loop->getVar()->isGlobal())
      ++counts[name].globalTargets;
    if (markSchedules && name == "enum_parallel")
      loop->setParallel();
    if (markSchedules && name == "enum_async")
      loop->setAsync();
  }

  void handle(ir::ImperativeForFlow *loop) override {
    ++counts[getParentFunc()->getUnmangledName()].indexed;
  }

  void handle(ir::CallInstr *call) override {
    auto *func = ir::util::getFunc(call->getCallee());
    if (auto *intrinsic = ir::cast<ir::InternalFunc>(func);
        intrinsic &&
        intrinsic->getIntrinsic() == ir::InternalFunc::Intrinsic::VIRTUAL_LOOKUP)
      ++counts[getParentFunc()->getUnmangledName()].virtualLookups;
    if (func && func->getName().rfind(
                    ast::getMangledFunc("std.internal.builtin", "enumerate"), 0) == 0)
      ++counts[getParentFunc()->getUnmangledName()].enumerates;
  }
};
} // namespace

TEST(EnumerateOptimizationTest, RemovesGeneratorsAndPreservesFallbacks) {
  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;
  Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("enumerate_optimization_test.codon",
                                    readCIRSource("enumerate_optimization_test.codon")));

  EnumerateInspector before;
  before.markSchedules = true;
  before.process(compiler.getModule());
  EXPECT_EQ(1, before.counts["enum_generator"].enumerates);
  EXPECT_EQ(1, before.counts["enum_list"].enumerates);
  EXPECT_EQ(1, before.counts["enum_array"].enumerates);
  EXPECT_EQ(1, before.counts["enum_matrix"].enumerates);
  EXPECT_EQ(1, before.counts["enum_global"].globalTargets);
  EXPECT_EQ(1, before.counts["enum_virtual"].enumerates);
  EXPECT_EQ(1, before.counts["enum_default"].enumerates);

  ir::transform::pythonic::EnumerateOptimization enumerate;
  enumerate.run(compiler.getModule());
  ir::transform::lowering::ImperativeForFlowLowering lowering;
  lowering.run(compiler.getModule());

  EnumerateInspector after;
  after.process(compiler.getModule());
  EXPECT_EQ(1, after.counts["enum_virtual"].virtualLookups);
  for (const auto &name : {"enum_generator", "enum_virtual", "enum_default"}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(0, after.counts[name].enumerates);
    EXPECT_EQ(1, after.counts[name].generators);
  }
  EXPECT_EQ(1, after.counts["enum_global"].globalTargets);
  for (const auto &name : {"enum_list", "enum_array", "enum_matrix"}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(0, after.counts[name].enumerates);
    EXPECT_EQ(0, after.counts[name].generators);
    EXPECT_EQ(1, after.counts[name].indexed);
  }
  for (const auto &name :
       {"enum_tuple", "enum_escaping", "enum_parallel", "enum_async", "enum_global"}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(1, after.counts[name].enumerates);
    EXPECT_EQ(1, after.counts[name].generators);
    EXPECT_EQ(0, after.counts[name].indexed);
  }
  EXPECT_EQ(0, after.counts["enum_shadowed"].enumerates);
  EXPECT_EQ(1, after.counts["enum_shadowed"].generators);
}
