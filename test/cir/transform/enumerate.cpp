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
  llvm::cantFail(compiler.parseCode("enumerate_optimization_test.codon", R"(
import numpy as np

def values():
    yield 1

def enum_generator(items: Generator[int]):
    for index, value in enumerate(items, 7):
        print(index, value)

class EnumBase(object):
  def __iter__(self) -> Generator[int]:
    yield 1

class EnumDerived(EnumBase):
  def __iter__(self) -> Generator[int]:
    yield 2

def enum_virtual(items: EnumBase):
  for index, value in enumerate(items):
    print(index, value)

class EnumDefault:
  def __iter__(self, start: int = 3):
    yield start

def enum_default(items: EnumDefault):
  for index, value in enumerate(items):
    print(index, value)

global_pair = (-1, -1)

def observe_global_pair():
  print(global_pair)

def enum_global(items: List[int]):
  global global_pair
  for global_pair in enumerate(items):
    index = global_pair[0]
    value = global_pair[1]
    print(index, value)
    observe_global_pair()

def enum_list(items: List[int]):
    for index, value in enumerate(items):
        print(index, value)

def enum_array(items: np.ndarray[int, 1]):
    for index, value in enumerate(items):
        print(index, value)

def enum_matrix(items: np.ndarray[int, 2]):
    for index, row in enumerate(items):
        print(index, row)

def enum_tuple(items: List[int]):
    for pair in enumerate(items):
        index = pair[0]
        value = pair[1]
        print(index, value, pair)

def enum_escaping(items: List[int]):
    iterator = enumerate(items)
    for index, value in iterator:
        print(index, value)

def enum_parallel(items: List[int]):
    for index, value in enumerate(items):
        print(index, value)

def enum_async(items: List[int]):
    for index, value in enumerate(items):
        print(index, value)

def enum_shadowed(items: List[int]):
    def enumerate(items):
        for value in items:
            yield (42, value)
    for index, value in enumerate(items):
        print(index, value)

enum_generator(values())
enum_virtual(EnumDerived())
enum_default(EnumDefault())
enum_global([1])
enum_list([1])
enum_array(np.arange(2))
enum_matrix(np.arange(4).reshape(2, 2))
enum_tuple([1])
enum_escaping([1])
enum_parallel([1])
enum_async([1])
enum_shadowed([1])
)"));

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
