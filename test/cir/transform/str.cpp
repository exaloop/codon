#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/operator.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

using namespace codon;

namespace {
struct AppendCounter : ir::util::Operator {
  std::unordered_map<std::string, int> appends;
  std::unordered_map<std::string, int> additions;
  std::unordered_map<std::string, int> cats;

  void handle(ir::CallInstr *call) override {
    auto *function = ir::util::getFunc(call->getCallee());
    if (!function)
      return;
    const auto &parent = getParentFunc()->getUnmangledName();
    if (function->getUnmangledName() == "_str_append_private")
      ++appends[parent];
    if (function->getUnmangledName() == "cat")
      ++cats[parent];
    if (function->getUnmangledName() == "__add__" &&
        call->getType()->is(call->getModule()->getStringType()))
      ++additions[parent];
  }
};
} // namespace

TEST(StringAppendOptimizationTest, ProvesPrivateLoopStorage) {
  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;
  Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("string_append_test.codon", R"(
def private_loop(seed: str, token: str, count: int):
    result = seed
    for index in range(count):
        result += token
    return result
def compound_loop(seed: str, a: str, b: str, count: int):
    result = seed
    for index in range(count):
        result += a + b
    return result
def observed_compound_loop(seed: str, a: str, b: str, count: int):
    result = seed
    for index in range(count):
        print(result)
        result += a + b
    return result
def branch_loop(count: int):
    result = ''
    for index in range(count):
        if index % 2:
            result += 'a'
        else:
            result += 'b'
    return result
def snapshot_loop(count: int):
    result = ''
    saved = []
    for index in range(count):
        saved.append(result)
        result += 'a'
    return result
def self_loop(count: int):
    result = 'a'
    for index in range(count):
        result += result
    return result
def exception_loop(count: int):
    result = ''
    try:
        for index in range(count):
            result += 'a'
    finally:
        print(result)
    return result
def reset_loop(count: int):
    result = ''
    for index in range(count):
        if index % 2:
            result = 'reset'
        result += 'a'
    return result
def exit_loop(count: int):
    result = ''
    index = 0
    while index < count:
        index += 1
        if index == count:
            break
        if index % 2:
            continue
        result += 'a'
        if index == 4:
            return result
    return result
def slice_loop(count: int):
    result = 'seed'
    view = result[1:]
    for index in range(count):
        result += 'a'
    print(view)
    return result
def key_loop(count: int):
    result = 'seed'
    saved = {result: 1}
    for index in range(count):
        result += 'a'
    print(saved)
    return result
def pointer_loop(count: int):
    result = 'seed'
    pointer = __ptr__(result)
    for index in range(count):
        result += 'a'
    print(pointer[0])
    return result
def closure_loop(count: int):
    result = 'seed'
    observe = lambda: result
    for index in range(count):
        result += 'a'
    print(observe())
    return result
def argument_loop(result: str, count: int):
    for index in range(count):
        result += 'a'
    return result
def called_loop(count: int):
    result = 'seed'
    for index in range(count):
        print(result)
        result += 'a'
    return result
print(private_loop('seed', 'a', 3))
print(compound_loop('seed', 'a', 'b', 3))
print(observed_compound_loop('seed', 'a', 'b', 3))
print(branch_loop(3))
print(snapshot_loop(3))
print(self_loop(3))
print(exception_loop(3))
print(reset_loop(3))
print(exit_loop(8))
print(slice_loop(3))
print(key_loop(3))
print(pointer_loop(3))
print(closure_loop(3))
print(argument_loop('seed', 3))
print(called_loop(3))
)"));
  llvm::cantFail(compiler.compile());
  AppendCounter counter;
  counter.process(compiler.getModule());
  EXPECT_EQ(1, counter.appends["private_loop"]);
  EXPECT_EQ(0, counter.additions["private_loop"]);
  EXPECT_EQ(1, counter.appends["compound_loop"]);
  EXPECT_EQ(0, counter.cats["compound_loop"]);
  EXPECT_EQ(0, counter.appends["observed_compound_loop"]);
  EXPECT_EQ(1, counter.cats["observed_compound_loop"]);
  EXPECT_EQ(2, counter.appends["branch_loop"]);
  EXPECT_EQ(0, counter.additions["branch_loop"]);
  EXPECT_EQ(1, counter.appends["exit_loop"]);
  EXPECT_EQ(0, counter.additions["exit_loop"]);
  for (const auto &name :
       {"snapshot_loop", "self_loop", "exception_loop", "reset_loop", "slice_loop",
        "key_loop", "pointer_loop", "closure_loop", "argument_loop", "called_loop"}) {
    EXPECT_EQ(0, counter.appends[name]) << name;
    EXPECT_GE(counter.additions[name], 1) << name;
  }
}

TEST(StringAppendOptimizationTest, PreservesUserDefinedAddition) {
  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;
  Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("string_append_override.codon", R"(
@extend
class str:
        def __add__(self, other: str):
                print('custom addition')
                return other
def custom_loop(token: str, count: int):
        result = ''
        for index in range(count):
                result += token
        return result
print(custom_loop('token', 3))
)"));
  llvm::cantFail(compiler.compile());
  AppendCounter counter;
  counter.process(compiler.getModule());
  EXPECT_EQ(0, counter.appends["custom_loop"]);
  EXPECT_EQ(1, counter.additions["custom_loop"]);
}

TEST(StringAppendOptimizationTest, IsolatesPublicMethodOverrides) {
  ASSERT_EXIT(
      {
        auto options = Options::getDefault("build/codon_test");
        options->debug = false;
        options->native = false;
        Compiler compiler(*options);
        llvm::cantFail(compiler.parseCode("string_buffer_override.codon", R"(
from internal.types.strbuf import strbuf
@extend
class strbuf:
    def __init__(self, capacity: int = 16, kind: int = 0):
        raise ValueError('public constructor')
    def append(self, value: str) -> None:
        raise ValueError('public append')
    def __str__(self) -> str:
        raise ValueError('public conversion')
def buffer_override_loop(token: str, count: int):
    result = ''
    for index in range(count):
        result += token
    return result
assert buffer_override_loop('a', 65) == 'a' * 65
@extend
class str:
    def __len__(self, /) -> int:
        return 0
# The already-bound builtin addition must not acquire this override via the pass.
assert buffer_override_loop('ab', 2)._meta == 4
)"));
        llvm::cantFail(compiler.compile());
        AppendCounter counter;
        counter.process(compiler.getModule());
        EXPECT_EQ(1, counter.appends["buffer_override_loop"]);
        compiler.getLLVMVisitor()->run();
        std::exit(::testing::Test::HasFailure() ? 1 : 0);
      },
      ::testing::ExitedWithCode(0), "");
}
