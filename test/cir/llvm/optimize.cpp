#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/llvm/native/native.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/cir/transform/manager.h"
#include "codon/cir/transform/numpy/numpy.h"
#include "codon/cir/util/irtools.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

#include <cstdlib>

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/SourceMgr.h>

using namespace codon;

namespace {
int countFixedAllocations(llvm::Module *module, uint64_t size,
                          bool inLoopOnly = false) {
  int count = 0;
  for (auto &function : *module) {
    if (function.isDeclaration())
      continue;
    llvm::DominatorTree dominators(function);
    llvm::LoopInfo loops(dominators);
    for (auto &block : function) {
      if (inLoopOnly && !loops.getLoopFor(&block))
        continue;
      for (auto &instruction : block) {
        auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
        if (!call || call->arg_empty())
          continue;
        auto *callee = call->getCalledFunction();
        if (!callee || callee->getName() != "seq_alloc_atomic")
          continue;
        auto *allocationSize =
            llvm::dyn_cast<llvm::ConstantInt>(call->getArgOperand(0));
        if (allocationSize && allocationSize->getZExtValue() == size)
          ++count;
      }
    }
  }
  return count;
}

int countLazyFixedAllocationCaches(llvm::Module *module, uint64_t size) {
  int count = 0;
  for (auto &function : *module) {
    if (function.isDeclaration())
      continue;
    llvm::DominatorTree dominators(function);
    llvm::LoopInfo loops(dominators);
    for (auto &block : function) {
      auto *allocationLoop = loops.getLoopFor(&block);
      if (!allocationLoop)
        continue;
      for (auto &instruction : block) {
        auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
        if (!call || call->arg_empty())
          continue;
        auto *callee = call->getCalledFunction();
        auto *allocationSize =
            llvm::dyn_cast<llvm::ConstantInt>(call->getArgOperand(0));
        if (!callee || callee->getName() != "seq_alloc_atomic" || !allocationSize ||
            allocationSize->getZExtValue() != size)
          continue;

        for (auto *user : call->users()) {
          auto *merge = llvm::dyn_cast<llvm::PHINode>(user);
          if (!merge)
            continue;
          for (auto &incoming : merge->incoming_values()) {
            auto *cache = llvm::dyn_cast<llvm::PHINode>(incoming.get());
            if (!cache || loops.getLoopFor(cache->getParent()) != allocationLoop)
              continue;
            bool startsNull = false;
            for (auto &cachedIncoming : cache->incoming_values())
              startsNull |= llvm::isa<llvm::ConstantPointerNull>(cachedIncoming.get());
            if (startsNull) {
              ++count;
              break;
            }
          }
        }
      }
    }
  }
  return count;
}

std::unique_ptr<Compiler> compileAndOptimize(const std::string &code) {
  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->standalone = true;
  auto compiler = std::make_unique<Compiler>(*options);
  llvm::cantFail(compiler->parseCode("allocation_phi_test.codon", code));
  llvm::cantFail(compiler->compile());
  ir::optimize(compiler->getLLVMVisitor()->getModule(), options.get());
  return compiler;
}

struct OptimizedModule {
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
};

std::string makeAllocationLoopIR(llvm::StringRef declaration,
                                 llvm::StringRef allocationUse) {
  std::string code = "declare noalias ptr @seq_alloc_atomic(i64)\n";
  code += declaration;
  code += R"(
define i64 @test(i64 %count) {
entry:
  br label %header

header:
  %index = phi i64 [ 0, %entry ], [ %next, %body ]
  %total = phi i64 [ 0, %entry ], [ %updated, %body ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body

body:
  %allocation = call ptr @seq_alloc_atomic(i64 65536)
)";
  code += allocationUse;
  code += R"(
  %extended = zext i8 %value to i64
  %updated = add i64 %total, %extended
  %next = add i64 %index, 1
  br label %header

exit:
  ret i64 %total
}
)";
  return code;
}

OptimizedModule compileAndOptimizeIR(const std::string &code) {
  OptimizedModule result{std::make_unique<llvm::LLVMContext>(), nullptr};
  llvm::SMDiagnostic diagnostic;
  result.module = llvm::parseAssemblyString(code, diagnostic, *result.context);
  if (!result.module) {
    std::string message;
    llvm::raw_string_ostream output(message);
    diagnostic.print("allocation_hoister_test", output);
    ADD_FAILURE() << output.str();
    return result;
  }

  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;
  options->standalone = true;
  ir::optimize(result.module.get(), options.get());
  EXPECT_FALSE(llvm::verifyModule(*result.module, &llvm::errs()));
  return result;
}

class GPUCodegenTest : public testing::Test {
  llvm::SmallString<128> libdevicePath;
  llvm::FileRemover libdeviceRemover;

protected:
  void SetUp() override {
    // These tests only emit PTX and need neither CUDA nor a GPU. Supply an empty
    // libdevice module rather than depending on a system CUDA installation.
    int fd;
    auto error =
        llvm::sys::fs::createTemporaryFile("codon-gpu-test", "ll", fd, libdevicePath);
    ASSERT_FALSE(error) << error.message();
    libdeviceRemover.setFile(libdevicePath);
    llvm::raw_fd_ostream output(fd, /*shouldClose=*/true);
    output << "; Empty libdevice for compile-only GPU tests.\n";
  }

  std::string compileToPTX(const std::string &code) {
    auto options = Options::getDefault("build/codon_test");
    options->debug = false;
    options->standalone = true;
    options->libdevice = libdevicePath.str().str();
    Compiler compiler(*options);
    llvm::cantFail(compiler.parseCode("gpu_codegen_test.codon", code));
    llvm::cantFail(compiler.compile());
    auto *module = compiler.getLLVMVisitor()->getModule();
    ir::optimize(module, options.get());
    auto *ptx = module->getNamedGlobal(".ptx");
    if (!ptx || !ptx->hasInitializer()) {
      ADD_FAILURE() << "No embedded PTX generated";
      return {};
    }
    return llvm::cast<llvm::ConstantDataArray>(ptx->getInitializer())
        ->getAsCString()
        .str();
  }
};
} // namespace

TEST_F(GPUCodegenTest, FoldsNumpyArrayOrderChecks) {
  // Keep compiler state isolated, as in the source-file test harness.
  ASSERT_EXIT(
      {
        auto ptx = compileToPTX(R"(
import numpy as np

values = np.ones(16)
@par(gpu=True)
for i in range(len(values)):
    values[i] = np.exp(values[i])
)");

        EXPECT_NE(std::string::npos, ptx.find(".visible .entry"));
        EXPECT_NE(std::string::npos, ptx.find("__nv_exp"));
        EXPECT_EQ(std::string::npos, ptx.find("memcmp"));
        EXPECT_EQ(std::string::npos, ptx.find("_str_"));
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST_F(GPUCodegenTest, LowersRuntimeStringEquality) {
  ASSERT_EXIT(
      {
        auto ptx = compileToPTX(R"(
import gpu

@gpu.kernel
def compare(a, b, result):
    result[0] = a == b

result = [False]
compare('hello', 'world', result, grid=1, block=1)
)");

        EXPECT_NE(std::string::npos, ptx.find(".visible .entry"));
        // A device-side helper is fine; an external libc symbol is not.
        EXPECT_EQ(std::string::npos, ptx.find(") memcmp\n"));
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, PreservesLinuxArm64UnwindFramePointers) {
  auto options = Options::getDefault("build/codon_test");
  Compiler compiler(*options);
  for (const auto &triple : {"aarch64-unknown-linux-gnu", "arm64-apple-darwin",
                             "x86_64-unknown-linux-gnu"}) {
    SCOPED_TRACE(triple);
    llvm::LLVMContext context;
    llvm::SMDiagnostic diagnostic;
    auto module = llvm::parseAssemblyString(R"(
declare void @may_throw()
define void @caller() {
  call void @may_throw()
  ret void
}
)",
                                            diagnostic, context);
    ASSERT_NE(nullptr, module);
    module->setTargetTriple(triple);

    llvm::LoopAnalysisManager loops;
    llvm::FunctionAnalysisManager functions;
    llvm::CGSCCAnalysisManager callGraph;
    llvm::ModuleAnalysisManager modules;
    llvm::PassBuilder builder;
    builder.registerModuleAnalyses(modules);
    builder.registerCGSCCAnalyses(callGraph);
    builder.registerFunctionAnalyses(functions);
    builder.registerLoopAnalyses(loops);
    builder.crossRegisterProxies(loops, functions, callGraph, modules);
    ir::addNativeLLVMPasses(&builder);
    auto pipeline = builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
    pipeline.run(*module, modules);

    auto *caller = module->getFunction("caller");
    ASSERT_NE(nullptr, caller);
    EXPECT_EQ(llvm::StringRef(triple) == "aarch64-unknown-linux-gnu" ? "non-leaf"
                                                                     : "none",
              caller->getFnAttribute("frame-pointer").getValueAsString().str());
  }
}

TEST(LLVMOptimizationTest, RemovesUnusedStandardStreamInitialization) {
  auto compiler = compileAndOptimize("print(\"hello world\")\n");
  auto *module = compiler->getLLVMVisitor()->getModule();

  EXPECT_EQ(nullptr, module->getFunction("seq_alloc"));
  EXPECT_EQ(nullptr, module->getFunction("seq_alloc_atomic"));
  EXPECT_EQ(nullptr, module->getFunction("seq_env"));
  EXPECT_EQ(nullptr, module->getFunction("seq_stdin"));
  EXPECT_EQ(nullptr, module->getFunction("seq_stderr"));
  EXPECT_NE(nullptr, module->getFunction("seq_stdout"));

  unsigned definitions = 0;
  for (const auto &function : *module) {
    EXPECT_FALSE(function.getName().contains("std.internal.format"));
    EXPECT_FALSE(function.getName().contains("std.internal.types.str"));
    definitions += !function.isDeclaration();
  }
  EXPECT_EQ(1, definitions);
}

TEST(LLVMOptimizationTest, RequiresKnownNumpyOwnership) {
  using ir::transform::numpy::hasOwnedResult;
  using ir::transform::numpy::NumPyExpr;
  using ir::transform::numpy::NumPyType;
  auto owns = [](NumPyExpr::Op op, bool array = true) {
    NumPyType type(array ? NumPyType::NP_TYPE_ARR_F64 : NumPyType::NP_TYPE_F64,
                   array ? 1 : 0);
    NumPyExpr expression(type, nullptr, op, std::make_unique<NumPyExpr>(type, nullptr));
    return hasOwnedResult(expression);
  };
  for (auto op : {NumPyExpr::NP_OP_NEG, NumPyExpr::NP_OP_ADD, NumPyExpr::NP_OP_EXP,
                  NumPyExpr::NP_OP_MATMUL, NumPyExpr::NP_OP_ZEROS_LIKE,
                  NumPyExpr::NP_OP_ONES_LIKE, NumPyExpr::NP_OP_SUM,
                  NumPyExpr::NP_OP_PROD, NumPyExpr::NP_OP_ANY, NumPyExpr::NP_OP_ALL,
                  NumPyExpr::NP_OP_AMIN, NumPyExpr::NP_OP_AMAX}) {
    EXPECT_TRUE(owns(op));
    EXPECT_FALSE(owns(op, false));
  }
  for (auto op : {NumPyExpr::NP_OP_NONE, NumPyExpr::NP_OP_POS, NumPyExpr::NP_OP_CONJ,
                  NumPyExpr::NP_OP_TRANSPOSE, NumPyExpr::NP_OP_CAST,
                  static_cast<NumPyExpr::Op>(NumPyExpr::NP_OP_AMAX + 1)})
    EXPECT_FALSE(owns(op));
  NumPyExpr leaf(NumPyType(NumPyType::NP_TYPE_ARR_F64, 1), nullptr);
  EXPECT_FALSE(hasOwnedResult(leaf));
}

TEST(LLVMOptimizationTest, ReleasesNumpyUpdateTemporaries) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def release_update(values: np.ndarray[float, 1]):
    values += 2.0 * values[::-1]
    return values

@export
def release_fused_update(values: np.ndarray[float, 1]):
  values[:] = values * values + 1.0

@export
def release_named_update(values: np.ndarray[float, 1]):
  temporary = values * 2.0
  values[:] = temporary
  values[:] = temporary

@export
def retain_view(values: np.ndarray[float, 1]):
    temporary = values * 2.0
    view = temporary[::-1]
    values[:] = temporary
    return view

@noinline
def keep_array(values):
    return values

@export
def retain_unknown(values: np.ndarray[float, 1]):
    temporary = values * 2.0
    alias = keep_array(temporary)
    values[:] = temporary
    return alias

@export
def retain_borrowed(values: np.ndarray[float, 1]):
    temporary = values.astype(float, copy=False)
    values[:] = temporary
    return temporary

@export
def retain_loop_carried(values: np.ndarray[float, 1], count: int):
    temporary = values * 2.0
    for iteration in range(count):
        values[:] = temporary
        temporary = values * 3.0
    return temporary

@export
def retain_container(values: np.ndarray[float, 1]):
    temporary = values * 2.0
    saved = [temporary]
    values[:] = temporary
    return saved

@export
def retain_inplace_result(values: np.ndarray[float, 1]):
    temporary = values * 2.0
    alias = temporary.__iadd__(values)
    return alias

@export
def retain_pointer(values: np.ndarray[float, 1]):
  temporary = values * 2.0
  pointer = temporary.data
  values[:] = temporary
  return pointer

@export
def retain_branch(values: np.ndarray[float, 1], condition: bool):
  temporary = values * 2.0
  if condition:
    values[:] = temporary
  return temporary

@export
def release_loop_update(values: np.ndarray[float, 1], count: int):
  for iteration in range(count):
    temporary = values * values + 1.0
    values[:] = temporary
    values[:] = temporary

@export
def release_reassigned_update(values: np.ndarray[float, 1]):
  temporary = values * 2.0
  values[:] = temporary
  temporary = temporary * 3.0
  values[:] = temporary

@export
def retain_conjugate(values: np.ndarray[float, 1]):
    temporary = np.conj(values)
    values[:] = temporary
    return temporary

@export
def retain_transferred(values: np.ndarray[float, 1]):
    temporary = np.exp(values)
    total = temporary.sum()
    return temporary / total

@export
def release_masked_update(values: np.ndarray[complex, 1], mask: np.ndarray[bool, 1]):
  values[mask] = values[mask] ** 2 + values[mask]

@export
def release_gather(values: np.ndarray[float, 1], mask: np.ndarray[bool, 1], output: np.ndarray[float, 1]):
  temporary = values[mask]
  output[:] = temporary

@export
def retain_gather_view(values: np.ndarray[float, 1], mask: np.ndarray[bool, 1], output: np.ndarray[float, 1]):
  temporary = values[mask]
  view = temporary[:]
  output[:] = temporary
  return view

@export
def replace_filtered(values: np.ndarray[float, 1], count: int):
  for iteration in range(count):
    if not len(values):
      break
    np.ones(values.shape)
    np.multiply(values, 1., values)
    mask = values > iteration
    np.logical_not(mask, mask)
    np.logical_not(mask, mask)
    values = values[mask]
  return values

@export
def retain_filtered_alias(values: np.ndarray[float, 1], count: int):
  saved = []
  for iteration in range(count):
    saved.append(values[:])
    values = values[values > iteration]
  return saved

@export
def replace_filtered_pair(left: np.ndarray[float, 1], right: np.ndarray[float, 1], mask: np.ndarray[bool, 1], count: int):
  for iteration in range(count):
    left, right = left[mask], right[mask]
  return left, right

@export
def retain_filtered_tuple(values: np.ndarray[float, 1], mask: np.ndarray[bool, 1], count: int):
  saved = []
  for iteration in range(count):
    pair = values[mask], values[mask]
    values = pair[0]
    saved.append(pair)
  return saved
)");
        auto countReleases = [&](llvm::StringRef name) {
          int releases = 0;
          for (auto &function : *compiler->getLLVMVisitor()->getModule()) {
            if (!function.getName().contains(name))
              continue;
            for (auto &block : function) {
              for (auto &instruction : block) {
                auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
                auto *callee = call ? call->getCalledFunction() : nullptr;
                if (callee && callee->getName() == "seq_free")
                  ++releases;
              }
            }
          }
          return releases;
        };
        EXPECT_EQ(countReleases("release_update"), 1);
        EXPECT_EQ(countReleases("release_fused_update"), 1);
        EXPECT_EQ(countReleases("release_named_update"), 1);
        EXPECT_EQ(countReleases("retain_view"), 0);
        EXPECT_EQ(countReleases("retain_unknown"), 0);
        EXPECT_EQ(countReleases("retain_borrowed"), 0);
        EXPECT_GE(countReleases("retain_loop_carried"), 1);
        EXPECT_EQ(countReleases("retain_container"), 0);
        EXPECT_EQ(countReleases("retain_inplace_result"), 0);
        EXPECT_EQ(countReleases("retain_pointer"), 0);
        EXPECT_EQ(countReleases("retain_branch"), 0);
        EXPECT_GE(countReleases("release_loop_update"), 1);
        EXPECT_EQ(countReleases("release_reassigned_update"), 2);
        EXPECT_EQ(countReleases("retain_conjugate"), 0);
        EXPECT_EQ(countReleases("retain_transferred"), 0);
        EXPECT_GE(countReleases("release_masked_update"), 2);
        EXPECT_EQ(countReleases("release_gather"), 1);
        EXPECT_EQ(countReleases("retain_gather_view"), 0);
        EXPECT_GE(countReleases("replace_filtered"), 1);
        EXPECT_EQ(countReleases("retain_filtered_alias"), 0);
        if (countReleases("replace_filtered_pair") < 2)
          llvm::errs() << "Filtered pair releases: "
                       << countReleases("replace_filtered_pair") << '\n';
        EXPECT_GE(countReleases("replace_filtered_pair"), 2);
        EXPECT_EQ(countReleases("retain_filtered_tuple"), 0);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, ThresholdsNumpyReplacementCleanup) {
  for (unsigned threshold : {512u, 0u, 1024u}) {
    SCOPED_TRACE(threshold);
    ASSERT_EXIT(
        {
          if (threshold != 512) {
            auto *option = llvm::cl::getRegisteredOptions().lookup("npfree-threshold");
            ASSERT_NE(nullptr, option);
            ASSERT_FALSE(option->addOccurrence(0, "npfree-threshold",
                                               std::to_string(threshold)));
          }
          auto compiler = compileAndOptimize(R"(
import numpy as np
from numpy.fusion import _free, _free_replacement

@export
def below_threshold(data: Ptr[byte]):
    _free_replacement(np.ndarray[np.uint8, 1]((511,), data), 512)

@export
def at_threshold(data: Ptr[byte]):
    _free_replacement(np.ndarray[np.uint8, 1]((512,), data), 512)

@export
def above_threshold(data: Ptr[byte]):
    _free_replacement(np.ndarray[np.uint8, 1]((513,), data), 512)

@export
def matrix_at_threshold(data: Ptr[float]):
    _free_replacement(np.ndarray[float, 2]((8, 8), data), 512)

@export
def ordinary_small_free(data: Ptr[byte]):
    _free(np.ndarray[np.uint8, 1]((511,), data))

@export
def replace_small(data: Ptr[float], count: int):
  values = np.ndarray[float, 1]((63,), data)
  for iteration in range(count):
    temporary = values * values + 1.0
    values[:] = temporary
    values[:] = temporary

@export
def replace_large(data: Ptr[float], count: int):
  values = np.ndarray[float, 1]((64,), data)
  for iteration in range(count):
    temporary = values * values + 1.0
    values[:] = temporary
    values[:] = temporary
)");
          auto *module = compiler->getLLVMVisitor()->getModule();
          for (auto name : {"below_threshold", "at_threshold", "above_threshold",
                            "matrix_at_threshold", "ordinary_small_free",
                            "replace_small", "replace_large"}) {
            auto *function = module->getFunction(name);
            ASSERT_NE(nullptr, function);
            unsigned releases = 0;
            for (auto &block : *function) {
              for (auto &instruction : block) {
                auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
                auto *callee = call ? call->getCalledFunction() : nullptr;
                if (!callee)
                  continue;
                EXPECT_FALSE(callee->getName().contains("_free_replacement"));
                releases += callee->getName() == "seq_free";
              }
            }
            unsigned expected = llvm::StringRef(name) != "below_threshold";
            if (llvm::StringRef(name) == "replace_small")
              expected = threshold <= 63 * sizeof(double);
            else if (llvm::StringRef(name) == "replace_large")
              expected = threshold <= 64 * sizeof(double);
            if (releases != expected)
              llvm::errs() << name << ": releases=" << releases
                           << ", expected=" << expected << '\n'
                           << *function;
            EXPECT_EQ(expected, releases) << name;
          }
          std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
        },
        testing::ExitedWithCode(EXIT_SUCCESS), "");
  }
}

TEST(LLVMOptimizationTest, PacksAndReleasesReversedDotOperands) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def dot_reversed(left: np.ndarray[float, 1], right: np.ndarray[float, 1]):
    return np.dot(left[::-1], right[::-1])
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        auto *function = module->getFunction("dot_reversed");
        ASSERT_NE(nullptr, function);
        auto countCalls = [&](llvm::StringRef prefix) {
          std::unordered_set<llvm::Function *> visited;
          std::function<unsigned(llvm::Function *)> count =
              [&](llvm::Function *current) {
                if (!visited.insert(current).second)
                  return 0u;
                unsigned calls = 0;
                for (auto &block : *current) {
                  for (auto &instruction : block) {
                    auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
                    auto *callee = call ? call->getCalledFunction() : nullptr;
                    if (callee)
                      calls +=
                          callee->getName().starts_with(prefix) ? 1 : count(callee);
                  }
                }
                return calls;
              };
          return count(function);
        };
        auto dots = countCalls("cblas_ddot");
        auto releases = countCalls("seq_free");
        // Dynamic strides retain both the common and packed BLAS call sites.
        if (dots != 2 || releases != 2)
          llvm::errs() << "Reversed dot: BLAS calls=" << dots
                       << ", scratch releases=" << releases << '\n';
        EXPECT_EQ(2, dots);
        EXPECT_EQ(2, releases);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, InlinesOnlyCoveredNumpyHelpers) {
  ASSERT_EXIT(
      {
        auto options = Options::getDefault("build/codon_test");
        options->debug = false;
        options->standalone = true;
        Compiler compiler(*options);
        std::string code = R"(
import numpy as np

def helper_chain(values):
  squared = values * values
  shifted = squared + 1
  return shifted

def helper_rebind(values):
  values = values + 1
  return values * 2

def helper_mean(values):
  squared = values * values
  total = np.mean(squared)
  return total

def helper_where(values):
  condition = values > 0
  selected = np.where(condition, values * 2, values + 1)
  return selected

def helper_shared(values):
  temporary = values + 1
  return temporary * temporary

def helper_uncovered(values, other):
  unused = values + other
  return values * 2

def helper_effect(values):
  temporary = values + 1
  print('retained')
  return temporary * 2

def helper_reduction(values):
  total = values.mean()
  return values - total

def helper_alias(values):
  alias = values
  return alias + 1

@noinline
def helper_kept(values):
  temporary = values + 1
  return temporary * 2

def helper_branch(values, condition):
  if condition:
    return values + 1
  return values * 2

def helper_large(values):
  return values + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9

def helper_nested(values):
  return helper_chain(values) + 2

def helper_reordered(values):
  first = values + 1
  second = values * 2
  return second + first

def helper_reordered_matmul(left, right, other_left, other_right):
  first = left + right
  second = other_left @ other_right
  return second + first

def helper_recursive(values: np.ndarray[float, 1]) -> np.ndarray[float, 1]:
  return helper_recursive(values) + 1

def helper_mutual(values: np.ndarray[float, 1]) -> np.ndarray[float, 1]:
  return helper_mutual_other(values) + 1

def helper_mutual_other(values: np.ndarray[float, 1]) -> np.ndarray[float, 1]:
  return helper_mutual(values) + 1

def helper_diamond(values):
  return helper_chain(values) + helper_chain(values)

def helper_nested_where(values):
  return helper_where(values) + 1

def helper_nested_reordered(values):
  return helper_reordered(values) + 1

def helper_staged_siblings(values):
  first = helper_chain(values)
  second = helper_chain(values)
  return second + first

def helper_nested_shared(values):
  temporary = values + 1
  return helper_chain(temporary) + 2

def helper_nested_reduction(values):
  return values - helper_mean(values)

def helper_hidden_reduction(values):
  return values - float(helper_mean(values))

def helper_nested_actual(values):
  return helper_chain(values + 1) + 2

def helper_fanout(values):
  return helper_diamond(values) + helper_diamond(values)

def helper_nodes_limit(values):
  return np.sin(values + 1 + 2 + 3 + 4 + 5 + 6 + 7)

def helper_nodes_over(values):
  return np.cos(helper_nodes_limit(values))
)";
        auto accepted = std::vector<std::string>(
            {"chain", "rebind", "mean", "where", "nested", "reordered",
             "reordered_matmul", "diamond", "nested_where", "nested_reordered",
             "staged_siblings", "depth_2", "depth_3", "nodes_limit"});
        auto rejected = std::vector<std::string>(
            {"shared", "uncovered", "effect", "reduction", "alias", "kept", "branch",
             "large", "recursive", "mutual", "nested_shared", "nested_reduction",
             "hidden_reduction", "nested_actual", "fanout", "depth_4", "nodes_over"});
        code += "\ndef helper_depth_0(values):\n    return values + 1\n";
        for (int depth = 1; depth <= 20; ++depth)
          code += "\ndef helper_depth_" + std::to_string(depth) +
                  "(values):\n    return helper_depth_" + std::to_string(depth - 1) +
                  "(values) + 1\n";
        std::vector<std::string> cases = accepted;
        cases.insert(cases.end(), rejected.begin(), rejected.end());
        for (const auto &name : cases) {
          auto arguments = name == "reordered_matmul" ? "values, values, values, values"
                           : name == "uncovered"      ? "values, values"
                           : name == "branch"         ? "values, condition"
                                                      : "values";
          auto rank = name == "reordered_matmul" ? "2" : "1";
          code += "\n@export\ndef probe_" + name + "(values: np.ndarray[float, " +
                  rank + "], condition: bool):\n    return helper_" + name + "(" +
                  arguments + ") + 2\n";
        }
        code += "\n@export\ndef probe_budget(values: np.ndarray[float, 1]):\n"
                "    return helper_depth_20(values)\n";
        // Five depth-four calls cost twenty expansions even with a cached
        // summary. Four must disappear; the fifth must retain its helper call.
        code += "\n@export\ndef probe_occurrences(values: np.ndarray[float, 1]):\n";
        for (int index = 0; index < 5; ++index)
          code += "    print(helper_depth_3(values))\n";
        // Validation preludes count toward the IR budget. These templates must
        // exhaust it before the independent sixteen-expansion caller limit.
        code += "\n@export\ndef probe_ir_budget(values: np.ndarray[float, 1]):\n";
        for (int index = 0; index < 20; ++index)
          code += "    print(helper_reordered(values))\n";
        llvm::cantFail(compiler.parseCode("numpy_inline_coverage.codon", code));
        struct Inspect : ir::transform::OperatorPass {
          std::unordered_set<std::string> callers;
          std::unordered_set<std::string> retained;
          unsigned occurrenceCalls = 0;
          unsigned irBudgetCalls = 0;
          std::string getKey() const override { return "test-numpy-inline-coverage"; }
          void handle(ir::CallInstr *call) override {
            auto *parent = getParentFunc();
            auto *callee = ir::util::getFunc(call->getCallee());
            if (!parent || !callee ||
                parent->getUnmangledName().rfind("probe_", 0) != 0)
              return;
            callers.insert(parent->getUnmangledName());
            if (callee->getUnmangledName().rfind("helper_", 0) == 0)
              retained.insert(parent->getUnmangledName());
            if (parent->getUnmangledName() == "probe_occurrences" &&
                callee->getUnmangledName() == "helper_depth_3")
              ++occurrenceCalls;
            if (parent->getUnmangledName() == "probe_ir_budget" &&
                callee->getUnmangledName() == "helper_reordered")
              ++irBudgetCalls;
          }
        };
        auto inspector = std::make_unique<Inspect>();
        auto *inspection = inspector.get();
        compiler.getPassManager()->registerPass(std::move(inspector),
                                                "core-numpy-fusion");
        llvm::cantFail(compiler.compile());
        for (const auto &name : cases) {
          auto caller = "probe_" + name;
          bool expected =
              std::find(rejected.begin(), rejected.end(), name) != rejected.end();
          if (!inspection->callers.count(caller) ||
              bool(inspection->retained.count(caller)) != expected)
            llvm::errs() << caller
                         << ": retained=" << inspection->retained.count(caller)
                         << ", expected=" << expected << '\n';
          EXPECT_EQ(1, inspection->callers.count(caller));
          EXPECT_EQ(expected, bool(inspection->retained.count(caller)));
        }
        EXPECT_EQ(1, inspection->retained.count("probe_budget"));
        EXPECT_EQ(1, inspection->occurrenceCalls);
        EXPECT_GT(inspection->irBudgetCalls, 4);
        EXPECT_LT(inspection->irBudgetCalls, 20);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, InlinesLayoutSensitiveFusionCallbacks) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def layout_stencil(steps: int, left: np.ndarray[float, 3], right: np.ndarray[float, 3]):
  for step in range(1, steps):
    right[1:-1, 1:-1, 1:-1] = (
      0.125 * (left[2:, 1:-1, 1:-1] - 2.0 * left[1:-1, 1:-1, 1:-1] + left[:-2, 1:-1, 1:-1])
      + 0.125 * (left[1:-1, 2:, 1:-1] - 2.0 * left[1:-1, 1:-1, 1:-1] + left[1:-1, :-2, 1:-1])
      + 0.125 * (left[1:-1, 1:-1, 2:] - 2.0 * left[1:-1, 1:-1, 1:-1] + left[1:-1, 1:-1, :-2])
      + left[1:-1, 1:-1, 1:-1])
    left[1:-1, 1:-1, 1:-1] = (
      0.125 * (right[2:, 1:-1, 1:-1] - 2.0 * right[1:-1, 1:-1, 1:-1] + right[:-2, 1:-1, 1:-1])
      + 0.125 * (right[1:-1, 2:, 1:-1] - 2.0 * right[1:-1, 1:-1, 1:-1] + right[1:-1, :-2, 1:-1])
      + 0.125 * (right[1:-1, 1:-1, 2:] - 2.0 * right[1:-1, 1:-1, 1:-1] + right[1:-1, 1:-1, :-2])
      + right[1:-1, 1:-1, 1:-1])
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        EXPECT_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        auto *stencil = module->getFunction("layout_stencil");
        ASSERT_NE(stencil, nullptr);
        EXPECT_FALSE(stencil->isDeclaration());
        for (auto &function : *module) {
          if (function.getName().contains("_loop_alloc_layout") ||
              function.getName().starts_with("__numpy_fusion_scalar_fn"))
            llvm::errs() << "Outlined fusion helper: " << function.getName() << '\n';
          EXPECT_FALSE(function.getName().contains("_loop_alloc_layout"))
              << function.getName().str();
          EXPECT_FALSE(function.getName().starts_with("__numpy_fusion_scalar_fn"))
              << function.getName().str();
        }
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, VectorizesNumpyUfuncOuter) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def outer_sum(left: np.ndarray[np.int32, 1], right: np.ndarray[np.int32, 1]):
  return np.add.outer(left, right)
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        EXPECT_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        auto *outer = module->getFunction("outer_sum");
        ASSERT_NE(outer, nullptr);
        bool vectorAdd = false;
        for (auto &block : *outer) {
          for (auto &instruction : block) {
            if (instruction.getOpcode() == llvm::Instruction::Add &&
                instruction.getType()->isVectorTy() &&
                instruction.getType()->getScalarType()->isIntegerTy(32))
              vectorAdd = true;
          }
        }
        if (!vectorAdd)
          llvm::errs() << "Missing vector int32 addition in ufunc outer\n";
        EXPECT_TRUE(vectorAdd);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, VectorizesNumpyUfuncAxisLoops) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def reduce_int(values: np.ndarray[np.int32, 2]):
  return np.add.reduce(values, axis=0, initial=np.int32(0))

@export
def scan_int(values: np.ndarray[np.int32, 2]):
  return np.add.accumulate(values, axis=0)

@export
def reduce_float(values: np.ndarray[float, 2]):
  return np.add.reduce(values, axis=0, initial=0.)

@export
def scan_float(values: np.ndarray[float, 2]):
  return np.add.accumulate(values, axis=0)
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        EXPECT_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        for (auto name : {"reduce_int", "scan_int", "reduce_float", "scan_float"}) {
          auto *function = module->getFunction(name);
          ASSERT_NE(function, nullptr);
          bool vectorAdd = false;
          for (auto &block : *function) {
            for (auto &instruction : block) {
              if (instruction.getOpcode() == llvm::Instruction::FAdd) {
                EXPECT_FALSE(instruction.getFastMathFlags().allowReassoc());
                vectorAdd |= instruction.getType()->isVectorTy();
              } else if (instruction.getOpcode() == llvm::Instruction::Add &&
                         instruction.getType()->isVectorTy() &&
                         instruction.getType()->getScalarType()->isIntegerTy(32)) {
                vectorAdd = true;
              }
            }
          }
          if (!vectorAdd)
            llvm::errs() << "Missing vector addition in " << name << '\n';
          EXPECT_TRUE(vectorAdd);
        }
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, FusesSingleExpressionArrayHelpers) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

def square(values):
    return values * values

@noinline
def kept_square(values):
    return values * values

@export
def unfused_helper(values: np.ndarray[float, 1]):
    return kept_square(values) + 1.0

@export
def fused_helper(values: np.ndarray[float, 1]):
    return square(values) + 1.0

def staged_helper(values):
  squared = values * values
  shifted = squared + 1.0
  return shifted

@export
def fused_staged_helper(values: np.ndarray[float, 1]):
  return staged_helper(values) + 2.0

def nested_helper(values):
  shifted = staged_helper(values)
  return shifted + 2.0

@export
def fused_nested_helper(values: np.ndarray[float, 1]):
  return nested_helper(values) + 3.0

def nested_direct(values):
  return staged_helper(values) + 2.0

def nested_deep(values):
  return nested_direct(values) + 3.0

@export
def fused_nested_direct(values: np.ndarray[float, 1]):
  return nested_direct(values) + 3.0

@export
def fused_nested_deep(values: np.ndarray[float, 1]):
  return nested_deep(values) + 4.0

@export
def fused_nested_diamond(values: np.ndarray[float, 1]):
  return nested_diamond(values) + 3.0

def nested_diamond(values):
  return staged_helper(values) + staged_helper(values)

def nested_siblings(values):
  first = staged_helper(values)
  second = staged_helper(values)
  return second + first

@export
def fused_nested_siblings(values: np.ndarray[float, 1]):
  return nested_siblings(values) + 3.0

def staged_rebind(values):
  values = values + 1.0
  result = values * 2.0
  return result

@export
def fused_staged_rebind(values: np.ndarray[float, 1]):
  return staged_rebind(values) + 2.0

def staged_reordered(left, right):
  first = left + right
  second = left * right
  return second + first

@export
def fused_staged_reordered(left: np.ndarray[float, 1], right: np.ndarray[float, 1]):
  return staged_reordered(left, right) + 3.0

def nested_reordered(left, right):
  return staged_reordered(left, right) + 2.0

@export
def fused_nested_reordered(left: np.ndarray[float, 1], right: np.ndarray[float, 1]):
  return nested_reordered(left, right) + 3.0

def staged_mean(values):
  squared = values * values
  result = np.mean(squared + 1.0)
  return result

@export
def fused_staged_mean(values: np.ndarray[float, 1]):
  return staged_mean(values)

def affine_helper(left, right, bias):
  return left * right + bias

@export
def fused_affine_helper(left: np.ndarray[float, 2], right: np.ndarray[float, 2], bias: np.ndarray[float, 2]):
  return affine_helper(left, right, bias) * bias

@export
def direct_affine_helper(left: np.ndarray[float, 2], right: np.ndarray[float, 2], bias: np.ndarray[float, 2]):
  return (left * right + bias) * bias

@export
def fused_slices(values: np.ndarray[float, 2]):
  return (values[1:, :-1] + values[:-1, 1:]) * values[1:, 1:]

@export
def fused_indexed_slices(values: np.ndarray[float, 2], index: int):
  return (values[1:, index] + values[:-1, index]) * values[1:, index]

@export
def indexed_slices_reference(values: np.ndarray[float, 2], index: int):
  left = values[1:, index]
  right = values[:-1, index]
  factor = values[1:, index]
  return (left + right) * factor

@export
def fused_clip(values: np.ndarray[float, 2]):
  return (np.clip(values, 2., 10.) * 3. + values) * 2.

@export
def fused_clip_min(values: np.ndarray[float, 2]):
  return values.clip(min=2.) + 1.

@export
def fused_clip_max(values: np.ndarray[float, 2]):
  return np.clip(values, None, 10.) * 2.

@export
def fused_clip_bounds(values: np.ndarray[float, 2], lower: np.ndarray[float, 1], upper: np.ndarray[float, 2]):
  return np.clip(values + 1., lower, upper) * 2.

@export
def fused_clip_sum(values: np.ndarray[float, 2]):
  return np.clip(values, -2., 2.).sum()

@export
def fused_scalar_coefficients(values: np.ndarray[float, 2], dx: float, dy: float):
  return (values[1:, :-1] / (2 * dx) + values[:-1, 1:] * (1 / dy)) / (2 * (dx ** 2 + dy ** 2))

@export
def fused_complex_grid(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  return left + right * 1j

@export
def fused_masked_square(values: np.ndarray[complex, 1], other: np.ndarray[complex, 1], mask: np.ndarray[bool, 1]):
  return values[mask] ** 2 + other[mask]

@export
def masked_gather_reference(values: np.ndarray[complex, 1], mask: np.ndarray[bool, 1]):
  return values[mask]

@export
def fused_sum(values: np.ndarray[float, 1]):
    return np.sum(values.astype(np.float32).copy() * 2 + 1.0)

@export
def fused_mean(values: np.ndarray[float, 1]):
  return np.mean(values * values + 1)

@export
def fused_where(values: np.ndarray[float, 1]):
  return np.where(values > 0, values * 2, values + 1)

@export
def fused_where_mean(values: np.ndarray[float, 2]):
  return np.where(values > 0, values * 2, values + 1).mean()

@export
def fused_axis_mean(values: np.ndarray[float, 2]):
  return (values + 1).mean(axis=1)

@export
def fused_prod(values: np.ndarray[int, 1]):
    return (values + 1).prod()

@export
def fused_filled(values: np.ndarray[float, 1]):
    return np.sum(np.ones_like(values) + np.zeros_like(values))

@export
def fused_sum_2d(values: np.ndarray[float, 2]):
    return (values + 1.0).sum()

@export
def fused_sum_transpose(values: np.ndarray[float, 2]):
    return (values.T + 1.0).sum()

@export
def fused_sum_3d(values: np.ndarray[float, 3]):
    return (values + 1.0).sum()

@export
def fused_prod_3d(values: np.ndarray[float, 3]):
    return (values + 1.0).prod()

@export
def fused_any(values: np.ndarray[float, 2]):
    return (values < 1).any()

@export
def fused_all(values: np.ndarray[float, 3]):
    return np.all(values >= 1)

@export
def fused_min(values: np.ndarray[float, 2]):
    return (values + 1).min()

@export
def fused_max(values: np.ndarray[float, 3]):
    return np.max(values + 1, initial=0.)

@export
def fused_amin(values: np.ndarray[float, 2]):
    return np.amin(values * 2)

@export
def fused_amax(values: np.ndarray[float, 2]):
    return np.amax(values * 2)

@export
def fused_producers_2d(values: np.ndarray[float, 2]):
    return ((values + 1).astype(np.float32, order='F').copy(order='C') + np.ones_like(values, order='F')).sum()

@export
def fused_filled_3d(values: np.ndarray[float, 3]):
    return (np.ones_like(values, order='K') + np.zeros_like(values, order='F')).sum()

@export
def fused_cast_4d(values: np.ndarray[float, 4]):
    return values.astype(np.float32, order='F').sum()

@export
def fused_any_producer(values: np.ndarray[float, 2]):
    return (values.astype(np.int8, order='F') < 1).any()

@export
def fused_copy_array(values: np.ndarray[float, 2]):
    return (values + 1).copy(order='F')

@export
def fused_cast_array(values: np.ndarray[float, 2]):
    return (values + 1).astype(np.float32, order='F')

@export
def fused_filled_array(values: np.ndarray[float, 3]):
    return np.zeros_like(values + 1, order='F') + np.ones_like(values, order='C')

@export
def fused_axis_sum(values: np.ndarray[float, 2], axis: int):
  return (values + 1).sum(axis=axis)

@export
def fused_axis_any(values: np.ndarray[float, 3]):
  return (values < 1).any(axis=(0, 2), keepdims=True)

@export
def fused_axis_min(values: np.ndarray[float, 3], axis: int):
  return (values + 1).min(axis=axis)

@export
def fused_axis_keepdims(values: np.ndarray[float, 3], axis: int):
  return (values.copy(order='F') + 1).sum(axis=axis, keepdims=True)

@export
def forwarded_axis_sum(values: np.ndarray[np.float32, 2]):
    reduced = np.sum(values, axis=1)
    return reduced * np.float32(2)

@export
def axis_sum_reference(values: np.ndarray[np.float32, 2]):
    return np.sum(values, axis=1)

retained_sum = np.zeros(0, dtype=np.float32)

@export
def global_axis_sum(values: np.ndarray[np.float32, 2]):
  global retained_sum
  retained_sum = np.sum(values, axis=1)
  return retained_sum * np.float32(2)

@export
def get_retained_sum():
  return retained_sum

@export
def last_use_division(values: np.ndarray[np.float32, 4]):
  temporary = np.exp(values)
  total = temporary.sum()
  return temporary / total

@export
def last_use_reference(values: np.ndarray[np.float32, 4]):
  temporary = np.exp(values)
  total = temporary.sum()
  return temporary, total

@export
def last_use_alias_mutation(values: np.ndarray[np.float32, 4]):
  temporary = np.exp(values)
  alias = temporary
  total = temporary.sum()
  alias[0, 0, 0, 0] = np.float32(123.)
  return temporary / total

@export
def last_use_view_mutation(values: np.ndarray[np.float32, 4]):
  temporary = np.exp(values)
  view = temporary[::-1]
  total = temporary.sum()
  view[0, 0, 0, 0] = np.float32(123.)
  return temporary / total

@export
def destination_slice(values: np.ndarray[float, 2]):
  out = np.empty_like(values)
  out[:] = values * values + 1.0
  return out

@export
def destination_ufunc(values: np.ndarray[float, 2]):
  out = np.empty_like(values)
  return np.add(values * values, 1.0, out=out)

@export
def destination_reference(values: np.ndarray[float, 2]):
  return np.empty_like(values)
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        auto allocationCount = [](llvm::Function *function) {
          unsigned allocations = 0;
          for (auto &block : *function) {
            if (llvm::isa<llvm::UnreachableInst>(block.getTerminator()))
              continue;
            for (auto &instruction : block) {
              auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
              auto *callee = call ? call->getCalledFunction() : nullptr;
              if (callee && (callee->getName() == "seq_alloc_atomic" ||
                             callee->getName() == "seq_alloc"))
                ++allocations;
            }
          }
          return allocations;
        };
        for (auto name : {"fused_helper",
                          "fused_staged_helper",
                          "fused_nested_helper",
                          "fused_nested_direct",
                          "fused_nested_deep",
                          "fused_nested_diamond",
                          "fused_nested_reordered",
                          "fused_nested_siblings",
                          "fused_staged_rebind",
                          "fused_staged_reordered",
                          "fused_copy_array",
                          "fused_cast_array",
                          "fused_filled_array",
                          "fused_axis_sum",
                          "fused_axis_any",
                          "fused_axis_min",
                          "fused_axis_keepdims",
                          "fused_slices",
                          "fused_scalar_coefficients",
                          "fused_complex_grid"}) {
          auto *function = module->getFunction(name);
          ASSERT_NE(nullptr, function);
          auto allocations = allocationCount(function);
          if (allocations != 1)
            function->print(llvm::errs());
          EXPECT_EQ(1, allocations) << name;
        }
        auto *masked = module->getFunction("fused_masked_square");
        ASSERT_NE(nullptr, masked);
        auto *forwarded = module->getFunction("forwarded_axis_sum");
        auto *reference = module->getFunction("axis_sum_reference");
        ASSERT_NE(nullptr, forwarded);
        ASSERT_NE(nullptr, reference);
        std::vector<llvm::Function *> active;
        std::function<unsigned(llvm::Function *)> reachableAllocations =
            [&](llvm::Function *function) {
              if (std::find(active.begin(), active.end(), function) != active.end())
                return 0u;
              active.push_back(function);
              auto count = allocationCount(function);
              for (auto &block : *function) {
                if (llvm::isa<llvm::UnreachableInst>(block.getTerminator()))
                  continue;
                for (auto &instruction : block) {
                  auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
                  auto *callee = call ? call->getCalledFunction() : nullptr;
                  if (callee && !callee->isDeclaration())
                    count += reachableAllocations(callee);
                }
              }
              active.pop_back();
              return count;
            };
        auto *clipped = module->getFunction("fused_clip");
        auto *gather = module->getFunction("masked_gather_reference");
        ASSERT_NE(nullptr, gather);
        auto expectedMasked = 2 * reachableAllocations(gather) + 1;
        if (reachableAllocations(masked) != expectedMasked)
          llvm::errs() << "Masked allocations: " << reachableAllocations(masked)
                       << ", expected: " << expectedMasked << '\n';
        EXPECT_EQ(expectedMasked, reachableAllocations(masked));
        ASSERT_NE(nullptr, clipped);
        EXPECT_EQ(1, reachableAllocations(clipped));
        for (auto name : {"fused_clip_min", "fused_clip_max", "fused_clip_bounds"}) {
          auto *clip = module->getFunction(name);
          ASSERT_NE(nullptr, clip);
          EXPECT_EQ(1, reachableAllocations(clip)) << name;
        }
        auto *clipSum = module->getFunction("fused_clip_sum");
        ASSERT_NE(nullptr, clipSum);
        EXPECT_EQ(0, reachableAllocations(clipSum));
        auto *where = module->getFunction("fused_where");
        ASSERT_NE(nullptr, where);
        if (reachableAllocations(where) != 1)
          llvm::errs() << "Where allocations: " << reachableAllocations(where) << '\n';
        EXPECT_EQ(1, reachableAllocations(where));
        auto *mean = module->getFunction("fused_mean");
        auto *whereMean = module->getFunction("fused_where_mean");
        ASSERT_NE(nullptr, mean);
        ASSERT_NE(nullptr, whereMean);
        if (reachableAllocations(mean) || reachableAllocations(whereMean))
          llvm::errs() << "Mean allocations: " << reachableAllocations(mean)
                       << ", where mean allocations: "
                       << reachableAllocations(whereMean) << '\n';
        EXPECT_EQ(0, reachableAllocations(mean));
        EXPECT_EQ(0, reachableAllocations(whereMean));
        auto *axisMean = module->getFunction("fused_axis_mean");
        ASSERT_NE(nullptr, axisMean);
        if (reachableAllocations(axisMean) != 1)
          llvm::errs() << "Axis mean allocations: " << reachableAllocations(axisMean)
                       << '\n';
        EXPECT_EQ(1, reachableAllocations(axisMean));
        auto *indexed = module->getFunction("fused_indexed_slices");
        auto *indexedReference = module->getFunction("indexed_slices_reference");
        ASSERT_NE(nullptr, indexed);
        ASSERT_NE(nullptr, indexedReference);
        EXPECT_GT(allocationCount(indexedReference), 0);
        EXPECT_EQ(allocationCount(indexedReference), allocationCount(indexed));
        auto *destinationReference = module->getFunction("destination_reference");
        auto *affineHelper = module->getFunction("fused_affine_helper");
        auto *directAffine = module->getFunction("direct_affine_helper");
        ASSERT_NE(nullptr, affineHelper);
        ASSERT_NE(nullptr, directAffine);
        EXPECT_EQ(reachableAllocations(directAffine),
                  reachableAllocations(affineHelper));
        ASSERT_NE(nullptr, destinationReference);
        auto destinationAllocations = reachableAllocations(destinationReference);
        EXPECT_GT(destinationAllocations, 0);
        for (auto name : {"destination_slice", "destination_ufunc"}) {
          auto *destination = module->getFunction(name);
          ASSERT_NE(nullptr, destination);
          auto count = reachableAllocations(destination);
          if (count != destinationAllocations)
            llvm::errs() << name << ": " << count
                         << " allocation sites; reference: " << destinationAllocations
                         << '\n';
          EXPECT_EQ(destinationAllocations, count) << name;
        }
        auto referenceAllocations = reachableAllocations(reference);
        auto forwardedAllocations = reachableAllocations(forwarded);
        if (referenceAllocations == 0 || referenceAllocations != forwardedAllocations)
          llvm::errs() << "Reference allocations: " << referenceAllocations
                       << ", forwarded allocations: " << forwardedAllocations << '\n';
        EXPECT_GT(referenceAllocations, 0);
        EXPECT_EQ(referenceAllocations, forwardedAllocations);
        auto *global = module->getFunction("global_axis_sum");
        ASSERT_NE(nullptr, global);
        EXPECT_GT(reachableAllocations(global), referenceAllocations);
        auto *lastUse = module->getFunction("last_use_division");
        auto *lastUseReference = module->getFunction("last_use_reference");
        ASSERT_NE(nullptr, lastUse);
        ASSERT_NE(nullptr, lastUseReference);
        auto lastUseAllocations = reachableAllocations(lastUse);
        auto lastUseReferenceAllocations = reachableAllocations(lastUseReference);
        if (lastUseAllocations != lastUseReferenceAllocations) {
          llvm::errs() << "Last-use allocations: " << lastUseAllocations
                       << ", reference allocations: " << lastUseReferenceAllocations
                       << '\n';
          lastUse->print(llvm::errs());
        }
        EXPECT_GT(lastUseReferenceAllocations, 0);
        EXPECT_EQ(lastUseReferenceAllocations, lastUseAllocations);
        for (auto name : {"last_use_alias_mutation", "last_use_view_mutation"}) {
          auto *mutated = module->getFunction(name);
          ASSERT_NE(nullptr, mutated);
          auto allocations = reachableAllocations(mutated);
          if (allocations <= lastUseReferenceAllocations)
            llvm::errs() << name << ": " << allocations
                         << " allocation sites; reference: "
                         << lastUseReferenceAllocations << '\n';
          EXPECT_GT(allocations, lastUseReferenceAllocations) << name;
        }
        auto *unfused = module->getFunction("unfused_helper");
        ASSERT_NE(nullptr, unfused);
        bool keptCall = false;
        for (auto &block : *unfused) {
          for (auto &instruction : block) {
            auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
            auto *callee = call ? call->getCalledFunction() : nullptr;
            keptCall |= callee && callee->getName().contains("kept_square");
          }
        }
        EXPECT_TRUE(keptCall);
        for (auto name :
             {"fused_sum", "fused_staged_mean", "fused_prod", "fused_filled",
              "fused_sum_2d", "fused_sum_transpose", "fused_sum_3d", "fused_prod_3d",
              "fused_any", "fused_all", "fused_min", "fused_max", "fused_amin",
              "fused_amax", "fused_producers_2d", "fused_filled_3d", "fused_cast_4d",
              "fused_any_producer"}) {
          auto *reduction = module->getFunction(name);
          ASSERT_NE(nullptr, reduction);
          for (auto &block : *reduction) {
            if (llvm::isa<llvm::UnreachableInst>(block.getTerminator()))
              continue;
            for (auto &instruction : block) {
              auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
              auto *callee = call ? call->getCalledFunction() : nullptr;
              if (callee && callee->getName() == "seq_alloc_atomic") {
                llvm::errs() << "Unexpected array allocation in " << name << '\n';
                reduction->print(llvm::errs());
                ADD_FAILURE();
              }
            }
          }
        }
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, PreservesHighwayMathLoops) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def highway_sin(values: np.ndarray[float, 2], bias: np.ndarray[float, 2]):
    return np.sin(values) + bias

@export
def highway_exp(values: np.ndarray[np.float32, 2], bias: np.ndarray[np.float32, 2]):
    return np.exp(values) + bias

@export
def highway_log(values: np.ndarray[float, 2], bias: np.ndarray[float, 2]):
    return np.log(values) + bias

@export
def highway_hypot(left: np.ndarray[np.float32, 2], right: np.ndarray[np.float32, 2]):
    return np.hypot(left, right) + right
)");
        auto *module = compiler->getLLVMVisitor()->getModule();
        for (auto name :
             {"highway_sin", "highway_exp", "highway_log", "highway_hypot"}) {
          auto *function = module->getFunction(name);
          ASSERT_NE(nullptr, function);
          std::string expected = "cnp_" + std::string(name).substr(8) + "_float";
          std::unordered_set<llvm::Function *> visited;
          std::function<bool(llvm::Function *)> hasHighwayCall =
              [&](llvm::Function *current) {
                if (!visited.insert(current).second)
                  return false;
                if (current->getName().starts_with(expected))
                  return true;
                for (auto &block : *current) {
                  for (auto &instruction : block) {
                    auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
                    auto *callee = call ? call->getCalledFunction() : nullptr;
                    if (callee && hasHighwayCall(callee))
                      return true;
                  }
                }
                return false;
              };
          EXPECT_TRUE(hasHighwayCall(function)) << name;
        }
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

namespace {
void checkMatmulAddFusion(bool enabled) {
  if (!enabled) {
    auto *option = llvm::cl::getRegisteredOptions().lookup("npfuse-matmul");
    ASSERT_NE(nullptr, option);
    ASSERT_FALSE(option->addOccurrence(0, "npfuse-matmul", "false"));
  }
  auto compiler = compileAndOptimize(R"(
import numpy as np

@export
def fused_gemv(left: np.ndarray[float, 2], right: np.ndarray[float, 1],
               bias: np.ndarray[float, 1]):
    return left @ right + bias

@export
def fused_gemm(left: np.ndarray[np.float32, 2], right: np.ndarray[np.float32, 2],
               bias: np.ndarray[np.float32, 2]):
    return left @ right + bias

@export
def fused_gemm_into(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
    output = np.ones((left.shape[0], right.shape[1]))
    output[:] = left @ right + output
    return output

@export
def fused_gemm_out(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
    output = np.ones((left.shape[0], right.shape[1]))
    return np.add(left @ right, output, out=output)

@export
def reversed_gemv(left: np.ndarray[float, 2], right: np.ndarray[float, 1],
          bias: np.ndarray[float, 1]):
  return bias + left @ right

@export
def reversed_gemm(left: np.ndarray[np.float32, 2], right: np.ndarray[np.float32, 2],
          bias: np.ndarray[np.float32, 2]):
  return bias + left @ right

@export
def reversed_gemm_into(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  output[:] = output + left @ right
  return output

@export
def reversed_gemm_out(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  return np.add(output, left @ right, out=output)

@export
def subtract_gemv(left: np.ndarray[float, 2], right: np.ndarray[float, 1],
          bias: np.ndarray[float, 1]):
  return left @ right - bias

@export
def reverse_subtract_gemv(left: np.ndarray[float, 2], right: np.ndarray[float, 1],
              bias: np.ndarray[float, 1]):
  return bias - left @ right

@export
def subtract_gemm(left: np.ndarray[np.float32, 2], right: np.ndarray[np.float32, 2],
          bias: np.ndarray[np.float32, 2]):
  return left @ right - bias

@export
def reverse_subtract_gemm(left: np.ndarray[np.float32, 2], right: np.ndarray[np.float32, 2],
              bias: np.ndarray[np.float32, 2]):
  return bias - left @ right

@export
def subtract_gemm_into(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  output[:] = left @ right - output
  return output

@export
def reverse_subtract_gemm_into(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  output[:] = output - left @ right
  return output

@export
def subtract_gemm_out(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  return np.subtract(left @ right, output, out=output)

@export
def reverse_subtract_gemm_out(left: np.ndarray[float, 2], right: np.ndarray[float, 2]):
  output = np.ones((left.shape[0], right.shape[1]))
  return np.subtract(output, left @ right, out=output)
)");
  auto *module = compiler->getLLVMVisitor()->getModule();
  std::vector<llvm::Function *> active;
  std::function<unsigned(llvm::Function *, double, double)> accumulatingCalls =
      [&](llvm::Function *function, double expectedAlpha, double expectedBeta) {
        if (std::find(active.begin(), active.end(), function) != active.end())
          return 0u;
        active.push_back(function);
        unsigned count = 0;
        for (auto &block : *function) {
          for (auto &instruction : block) {
            auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
            auto *callee = call ? call->getCalledFunction() : nullptr;
            if (!callee)
              continue;
            auto name = callee->getName();
            bool gemm = name.contains("cblas_sgemm") || name.contains("cblas_dgemm");
            bool gemv = name.contains("cblas_sgemv") || name.contains("cblas_dgemv");
            if (gemm || gemv) {
              auto *alpha =
                  llvm::dyn_cast<llvm::ConstantFP>(call->getArgOperand(gemm ? 6 : 4));
              auto *beta =
                  llvm::dyn_cast<llvm::ConstantFP>(call->getArgOperand(gemm ? 11 : 9));
              count += alpha && beta && alpha->isExactlyValue(expectedAlpha) &&
                       beta->isExactlyValue(expectedBeta);
            } else if (!callee->isDeclaration()) {
              count += accumulatingCalls(callee, expectedAlpha, expectedBeta);
            }
          }
        }
        active.pop_back();
        return count;
      };
  struct {
    const char *name;
    double alpha;
    double beta;
  } cases[] = {{"fused_gemv", 1, 1},          {"fused_gemm", 1, 1},
               {"fused_gemm_into", 1, 1},     {"fused_gemm_out", 1, 1},
               {"reversed_gemv", 1, 1},       {"reversed_gemm", 1, 1},
               {"reversed_gemm_into", 1, 1},  {"reversed_gemm_out", 1, 1},
               {"subtract_gemv", 1, -1},      {"reverse_subtract_gemv", -1, 1},
               {"subtract_gemm", 1, -1},      {"reverse_subtract_gemm", -1, 1},
               {"subtract_gemm_into", 1, -1}, {"reverse_subtract_gemm_into", -1, 1},
               {"subtract_gemm_out", 1, -1},  {"reverse_subtract_gemm_out", -1, 1}};
  for (auto &test : cases) {
    auto *function = module->getFunction(test.name);
    ASSERT_NE(nullptr, function);
    auto count = accumulatingCalls(function, test.alpha, test.beta);
    if ((count > 0) != enabled)
      llvm::errs() << test.name << ": accumulating BLAS calls=" << count
                   << ", enabled=" << enabled << '\n';
    EXPECT_EQ(enabled, count > 0) << test.name;
  }
}
} // namespace

TEST(LLVMOptimizationTest, FusesMatmulAddByDefault) {
  ASSERT_EXIT(
      {
        checkMatmulAddFusion(true);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, DisablesMatmulAddFusion) {
  ASSERT_EXIT(
      {
        checkMatmulAddFusion(false);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, ReusesOpenMPThreadIds) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
import openmp as omp

@export
def static_ids(data: Ptr[int], count: int):
    @par(num_threads=4)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()
    data[0] = omp.get_thread_num()

@export
def chunked_ids(data: Ptr[int], count: int):
    @par(schedule='static', chunk_size=7)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()

@export
def dynamic_ids(data: Ptr[int], count: int):
    @par(schedule='dynamic', chunk_size=7)
    for index in range(count):
        data[index] = omp._get_gtid() + omp.get_thread_num()

def items(count):
  for index in range(count):
    yield index

@export
def task_ids(data: Ptr[int], count: int):
  @par
  for index in items(count):
    data[index] = omp._get_gtid()
)");
        unsigned outlines = 0;
        unsigned outsideQueries = 0;
        for (auto &function : *compiler->getLLVMVisitor()->getModule()) {
          if (function.isDeclaration())
            continue;
          bool outlined = function.getName().contains("_loop_outline_template");
          outlines += outlined;
          for (auto &block : function) {
            for (auto &instruction : block) {
              auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
              auto *callee = call ? call->getCalledFunction() : nullptr;
              if (callee && (callee->getName() == "__kmpc_global_thread_num" ||
                             callee->getName() == "omp_get_thread_num")) {
                EXPECT_FALSE(outlined) << function.getName().str();
                outsideQueries += !outlined;
              }
            }
          }
        }
        EXPECT_EQ(5, outlines);
        EXPECT_GT(outsideQueries, 0);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, KeepsOpenMPAccumulatorsInRegisters) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
@export
def sum_int(data: Ptr[int], count: int):
    total = 0
    @par
    for index in range(count):
        total += data[index]
    return total

@export
def sum_float(data: Ptr[float], count: int):
    total = 0.0
    @par
    for index in range(count):
        total += data[index]
    return total
)");
        unsigned outlines = 0;
        unsigned loopBlocks = 0;
        for (auto &function : *compiler->getLLVMVisitor()->getModule()) {
          if (!function.getName().contains("_loop_outline_template"))
            continue;
          ++outlines;
          llvm::DominatorTree dominators(function);
          llvm::LoopInfo loops(dominators);
          for (auto &block : function) {
            if (!loops.getLoopFor(&block))
              continue;
            ++loopBlocks;
            for (auto &instruction : block) {
              if (auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
                EXPECT_FALSE(llvm::isa<llvm::AllocaInst>(store->getPointerOperand()));
            }
          }
        }
        EXPECT_EQ(2, outlines);
        EXPECT_GT(loopBlocks, 0);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, KeepsOpenMPQueriesWithoutStableContext) {
  auto optimized = compileAndOptimizeIR(R"(
declare void @__kmpc_fork_call(ptr, i32, ptr, ...)
declare ptr @__kmpc_omp_task_alloc(ptr, i32, i32, i64, i64, ptr, ptr)
declare i32 @__kmpc_global_thread_num(ptr)
declare i32 @omp_get_thread_num()
declare void @escape(ptr)

define private void @escaped(ptr %gtid, ptr %btid, ptr %result) noinline {
  %global = call i32 @__kmpc_global_thread_num(ptr null)
  %team = call i32 @omp_get_thread_num()
  %total = add i32 %global, %team
  store i32 %total, ptr %result
  ret void
}

define private i32 @untied(i32 %gtid, ptr %task) noinline {
  %global = call i32 @__kmpc_global_thread_num(ptr null)
  ret i32 %global
}

define void @launch(ptr %result) {
  call void (ptr, i32, ptr, ...) @__kmpc_fork_call(ptr null, i32 1,
                                                ptr @escaped, ptr %result)
  call void @escape(ptr @escaped)
  %task = call ptr @__kmpc_omp_task_alloc(ptr null, i32 0, i32 0, i64 64,
                                        i64 0, ptr @untied, ptr null)
  call void @escape(ptr %task)
  ret void
}
)");
  ASSERT_NE(nullptr, optimized.module);
  unsigned queries = 0;
  for (auto &function : *optimized.module) {
    for (auto &block : function) {
      for (auto &instruction : block) {
        auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
        auto *callee = call ? call->getCalledFunction() : nullptr;
        queries += callee && (callee->getName() == "__kmpc_global_thread_num" ||
                              callee->getName() == "omp_get_thread_num");
      }
    }
  }
  EXPECT_EQ(3, queries);
}

TEST(LLVMOptimizationTest, HoistsNonescapingPointerThroughAggregatePhi) {
  auto compiler =
      compileAndOptimize("PATH = 'allocation_phi_test.txt'\n"
                         "total = 0\n"
                         "with open(PATH, 'r', encoding='utf-8') as stream:\n"
                         "    while True:\n"
                         "        value = stream.read(65536)\n"
                         "        if not value:\n"
                         "            break\n"
                         "        total += len(value)\n"
                         "print(total)\n");

  auto *module = compiler->getLLVMVisitor()->getModule();
  EXPECT_GT(countFixedAllocations(module, 65536), 0);
  EXPECT_EQ(1, countLazyFixedAllocationCaches(module, 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistEscapingPointerThroughAggregatePhi) {
  auto compiler =
      compileAndOptimize("PATH = 'allocation_phi_test.txt'\n"
                         "chunks = List[str]()\n"
                         "with open(PATH, 'r', encoding='utf-8') as stream:\n"
                         "    while True:\n"
                         "        value = stream.read(65536)\n"
                         "        if not value:\n"
                         "            break\n"
                         "        chunks.append(value)\n"
                         "print(len(chunks))\n");

  auto *module = compiler->getLLVMVisitor()->getModule();
  EXPECT_GT(countFixedAllocations(module, 65536, /*inLoopOnly=*/true), 0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(module, 65536));
}

TEST(LLVMOptimizationTest, ResetsLazyAllocationCacheForEachOuterIteration) {
  for (bool freed : {false, true}) {
    SCOPED_TRACE(freed);
    auto optimized = compileAndOptimizeIR(std::string(R"(
declare noalias ptr @seq_alloc_atomic(i64)
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree memory(read)

define i64 @test(i64 %limit, i64 %count) {
entry:
  br label %outer
outer:
  %size = phi i64 [ 2, %entry ], [ %next.size, %outer.latch ]
  %total = phi i64 [ 0, %entry ], [ %subtotal, %outer.latch ]
  br label %inner
inner:
  %index = phi i64 [ 0, %outer ], [ %next.index, %inner.latch ]
  %subtotal = phi i64 [ %total, %outer ], [ %updated, %inner.latch ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %outer.latch, label %body
body:
  %parity = and i64 %index, 1
  %allocate = icmp eq i64 %parity, 0
  br i1 %allocate, label %allocation, label %inner.latch
allocation:
  %buffer = call ptr @seq_alloc_atomic(i64 %size)
  store i8 42, ptr %buffer
  %value = call i8 @read(ptr %buffer)
)") + (freed ? "  call void @seq_free(ptr %buffer)\n" : "") +
                                          R"(
  %extended = zext i8 %value to i64
  %sum = add i64 %subtotal, %extended
  br label %inner.latch
inner.latch:
  %updated = phi i64 [ %subtotal, %body ], [ %sum, %allocation ]
  %next.index = add i64 %index, 1
  br label %inner
outer.latch:
  %next.size = add i64 %size, 1
  %finished = icmp eq i64 %next.size, %limit
  br i1 %finished, label %exit, label %outer
exit:
  ret i64 %subtotal
}
)");
    ASSERT_NE(nullptr, optimized.module);
    auto *function = optimized.module->getFunction("test");
    ASSERT_NE(nullptr, function);
    llvm::DominatorTree dominators(*function);
    llvm::LoopInfo loops(dominators);
    unsigned caches = 0;
    for (auto &block : *function) {
      auto *loop = loops.getLoopFor(&block);
      if (!loop || loop->getLoopDepth() != 2 || loop->getHeader() != &block)
        continue;
      for (auto &phi : block.phis()) {
        if (!phi.getType()->isPointerTy())
          continue;
        ++caches;
        auto *initial = phi.getIncomingValueForBlock(loop->getLoopPreheader());
        EXPECT_TRUE(llvm::isa<llvm::ConstantPointerNull>(initial));
      }
    }
    EXPECT_EQ(1, caches);
    unsigned releases = 0;
    for (auto &block : *function) {
      for (auto &instruction : block) {
        auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
        auto *callee = call ? call->getCalledFunction() : nullptr;
        if (callee && callee->getName() == "seq_free") {
          // Loop unswitching may retain a free(NULL) on the zero-trip path.
          releases += !llvm::isa<llvm::ConstantPointerNull>(call->getArgOperand(0));
          auto *loop = loops.getLoopFor(&block);
          ASSERT_NE(nullptr, loop);
          EXPECT_EQ(1, loop->getLoopDepth());
        }
      }
    }
    EXPECT_EQ(freed ? 1u : 0u, releases);
  }
}

TEST(LLVMOptimizationTest, HoistsAndReleasesFreedLoopAllocation) {
  auto optimized = compileAndOptimizeIR(
      makeAllocationLoopIR("declare {} @seq_free(ptr) nounwind\n"
                           "declare i8 @read(ptr nocapture) nofree memory(read)\n",
                           "  store i8 42, ptr %allocation\n"
                           "  %value = call i8 @read(ptr %allocation)\n"
                           "  call {} @seq_free(ptr %allocation)\n"));

  ASSERT_NE(nullptr, optimized.module);
  auto *function = optimized.module->getFunction("test");
  ASSERT_NE(nullptr, function);
  llvm::DominatorTree dominators(*function);
  llvm::LoopInfo loops(dominators);
  unsigned releases = 0;
  for (auto &block : *function) {
    for (auto &instruction : block) {
      auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
      auto *callee = call ? call->getCalledFunction() : nullptr;
      if (callee && callee->getName() == "seq_free") {
        ++releases;
        EXPECT_EQ(nullptr, loops.getLoopFor(&block));
      }
    }
  }
  EXPECT_EQ(1, releases);
  EXPECT_EQ(1, countFixedAllocations(optimized.module.get(), 65536));
  EXPECT_TRUE(countFixedAllocations(optimized.module.get(), 65536, true) == 0 ||
              countLazyFixedAllocationCaches(optimized.module.get(), 65536) == 1);
}

TEST(LLVMOptimizationTest, HoistsFreedAllocationWithConditionalUseAndEarlyExit) {
  auto optimized = compileAndOptimizeIR(R"(
declare noalias ptr @seq_alloc_atomic(i64)
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree memory(read)
declare void @early_exit()

define i64 @test(i64 %count, i64 %stop) {
entry:
  br label %header
header:
  %index = phi i64 [ 0, %entry ], [ %next, %latch ]
  %total = phi i64 [ 0, %entry ], [ %updated, %latch ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body
body:
  %parity = and i64 %index, 1
  %allocate = icmp eq i64 %parity, 1
  br i1 %allocate, label %allocation, label %latch
allocation:
  %buffer = call ptr @seq_alloc_atomic(i64 65536)
  store i8 42, ptr %buffer
  %value = call i8 @read(ptr %buffer)
  call void @seq_free(ptr nonnull %buffer)
  %extended = zext i8 %value to i64
  %sum = add i64 %total, %extended
  %finish = icmp eq i64 %index, %stop
  br i1 %finish, label %early, label %latch
latch:
  %updated = phi i64 [ %total, %body ], [ %sum, %allocation ]
  %next = add i64 %index, 1
  br label %header
early:
  call void @early_exit()
  ret i64 %sum
exit:
  ret i64 %total
}
)");
  ASSERT_NE(nullptr, optimized.module);
  auto *function = optimized.module->getFunction("test");
  ASSERT_NE(nullptr, function);
  llvm::DominatorTree dominators(*function);
  llvm::LoopInfo loops(dominators);
  unsigned releases = 0;
  for (auto &block : *function) {
    for (auto &instruction : block) {
      auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction);
      auto *callee = call ? call->getCalledFunction() : nullptr;
      if (callee && callee->getName() == "seq_free") {
        ++releases;
        EXPECT_EQ(nullptr, loops.getLoopFor(&block));
      }
    }
  }
  EXPECT_GE(releases, 1u);
  EXPECT_EQ(1, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistMixedOwnerFree) {
  auto optimized = compileAndOptimizeIR(
      makeAllocationLoopIR("declare void @seq_free(ptr) nounwind\n"
                           "declare ptr @other()\n"
                           "declare i1 @choose()\n"
                           "declare i8 @read(ptr nocapture) nofree memory(read)\n",
                           "  store i8 42, ptr %allocation\n"
                           "  %value = call i8 @read(ptr %allocation)\n"
                           "  %other = call ptr @other()\n"
                           "  %choose = call i1 @choose()\n"
                           "  %owner = select i1 %choose, ptr %allocation, ptr %other\n"
                           "  call void @seq_free(ptr %owner)\n"));
  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536, true), 0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistEscapingFreedAllocation) {
  auto optimized = compileAndOptimizeIR(
      makeAllocationLoopIR("@escaped = global ptr null\n"
                           "declare void @seq_free(ptr) nounwind\n"
                           "declare i8 @read(ptr nocapture) nofree memory(read)\n",
                           "  store ptr %allocation, ptr @escaped\n"
                           "  %value = call i8 @read(ptr %allocation)\n"
                           "  call void @seq_free(ptr %allocation)\n"));
  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536, true), 0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistFreeAcrossExceptionalExit) {
  auto optimized = compileAndOptimizeIR(R"(
declare noalias ptr @seq_alloc_atomic(i64)
declare void @seq_free(ptr) nounwind
declare i8 @read(ptr nocapture) nofree
declare void @cleanup() nounwind
declare i32 @__gxx_personality_v0(...)

define i64 @test(i64 %count) personality ptr @__gxx_personality_v0 {
entry:
  br label %header
header:
  %index = phi i64 [ 0, %entry ], [ %next, %normal ]
  %total = phi i64 [ 0, %entry ], [ %updated, %normal ]
  %done = icmp eq i64 %index, %count
  br i1 %done, label %exit, label %body
body:
  %allocation = call ptr @seq_alloc_atomic(i64 65536)
  %value = invoke i8 @read(ptr %allocation) to label %normal unwind label %unwind
normal:
  call void @seq_free(ptr %allocation)
  %extended = zext i8 %value to i64
  %updated = add i64 %total, %extended
  %next = add i64 %index, 1
  br label %header
unwind:
  %exception = landingpad { ptr, i32 } cleanup
  call void @cleanup()
  resume { ptr, i32 } %exception
exit:
  ret i64 %total
}
)");
  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536, true), 0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, ExecutesHoistedFreedLoopAllocations) {
  ASSERT_EXIT(
      {
        auto compiler = compileAndOptimize(R"(
from internal.gc import free

@C
def GC_get_total_bytes() -> int:
  pass

@noinline
def touch(buffer: Ptr[byte], size: int, value: int):
  for offset in range(size):
    buffer[offset] = byte(value + offset)
  return int(buffer[0]) + int(buffer[size - 1])

@noinline
def exercise(limit: int, count: int, stop: int, skip: bool):
  total = 0
  for outer in range(limit):
    size = (outer + 1) * 4096
    for inner in range(count):
      if skip or inner % 2 == 0:
        continue
      buffer = Ptr[byte](size)
      total += touch(buffer, size, outer + inner)
      free(buffer.as_byte())
      if inner == stop:
        return total
  return total

def expected(limit: int, count: int, stop: int, skip: bool):
  total = 0
  for outer in range(limit):
    for inner in range(count):
      if skip or inner % 2 == 0:
        continue
      total += ((outer + inner) & 255) + ((outer + inner - 1) & 255)
      if inner == stop:
        return total
  return total

for count in (0, 1, 8):
  for stop in (-1, 3):
    for skip in (False, True):
      assert exercise(8, count, stop, skip) == expected(8, count, stop, skip)

before = GC_get_total_bytes()
result = exercise(8, 128, -1, False)
allocated = GC_get_total_bytes() - before
assert result == expected(8, 128, -1, False)
assert 0 < allocated < 1024 * 1024
)");
        compiler->getLLVMVisitor()->run({"allocation_free_test.codon"});
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(LLVMOptimizationTest, HoistsAllocationPassedToMetadataOnlyHelper) {
  auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(R"(
declare i8 @observe_size(i64)
declare i8 @read(ptr nocapture) nofree

define i8 @metadata({ { ptr, i64 }, i64 } %descriptor) noinline {
  %nested = extractvalue { { ptr, i64 }, i64 } %descriptor, 0
  %size = extractvalue { ptr, i64 } %nested, 1
  %result = call i8 @observe_size(i64 %size)
  ret i8 %result
}
)",
                                                             R"(
  %descriptor = insertvalue { { ptr, i64 }, i64 } zeroinitializer, ptr %allocation, 0, 0
  %sized = insertvalue { { ptr, i64 }, i64 } %descriptor, i64 %index, 0, 1
  %metadata = call i8 @metadata({ { ptr, i64 }, i64 } %sized)
  %byte = call i8 @read(ptr %allocation)
  %value = add i8 %metadata, %byte
)"));
  ASSERT_NE(nullptr, optimized.module);
  EXPECT_EQ(1, countFixedAllocations(optimized.module.get(), 65536));
  EXPECT_TRUE(countFixedAllocations(optimized.module.get(), 65536,
                                    /*inLoopOnly=*/true) == 0 ||
              countLazyFixedAllocationCaches(optimized.module.get(), 65536) == 1);
}

TEST(LLVMOptimizationTest, HoistsMetadataFieldsThroughAggregateHelpers) {
  auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(R"(
declare i8 @observe_size(i64)
declare i8 @read(ptr nocapture) nofree
declare void @escape_descriptor({ ptr, i64 })

define i8 @metadata_leaf({ ptr, i64 } %descriptor) noinline optnone {
  %cleared = insertvalue { ptr, i64 } %descriptor, ptr null, 0
  call void @escape_descriptor({ ptr, i64 } %cleared)
  %size = extractvalue { ptr, i64 } %descriptor, 1
  %result = call i8 @observe_size(i64 %size)
  ret i8 %result
}

define i8 @metadata({ { ptr, i64 }, i64 } %descriptor, i1 %choose) noinline optnone {
entry:
  %frozen = freeze { { ptr, i64 }, i64 } %descriptor
  %selected = select i1 %choose, { { ptr, i64 }, i64 } %frozen,
                                { { ptr, i64 }, i64 } zeroinitializer
  br i1 %choose, label %first, label %second
first:
  br label %merge
second:
  br label %merge
merge:
  %merged = phi { { ptr, i64 }, i64 } [ %selected, %first ], [ %descriptor, %second ]
  %nested = extractvalue { { ptr, i64 }, i64 } %merged, 0
  %result = call i8 @metadata_leaf({ ptr, i64 } %nested)
  ret i8 %result
}
)",
                                                             R"(
  %descriptor = insertvalue { { ptr, i64 }, i64 } zeroinitializer, ptr %allocation, 0, 0
  %sized = insertvalue { { ptr, i64 }, i64 } %descriptor, i64 %index, 0, 1
  %choose = icmp eq i64 %index, 0
  %metadata = call i8 @metadata({ { ptr, i64 }, i64 } %sized, i1 %choose)
  %byte = call i8 @read(ptr %allocation)
  %value = add i8 %metadata, %byte
)"));
  ASSERT_NE(nullptr, optimized.module);
  EXPECT_EQ(1, countFixedAllocations(optimized.module.get(), 65536));
  EXPECT_TRUE(countFixedAllocations(optimized.module.get(), 65536,
                                    /*inLoopOnly=*/true) == 0 ||
              countLazyFixedAllocationCaches(optimized.module.get(), 65536) == 1);
}

TEST(LLVMOptimizationTest, DoesNotHoistUsedAggregateFields) {
  const char *bodies[] = {
      "store { ptr, i64 } %descriptor, ptr @escaped_descriptor\n",
      "%pointer = extractvalue { ptr, i64 } %descriptor, 0\n"
      "store ptr %pointer, ptr @escaped\n",
      "%pointer = extractvalue { ptr, i64 } %descriptor, 0\n"
      "call void @seq_free(ptr %pointer)\n",
      "call void @unknown({ ptr, i64 } %descriptor)\n",
      "%returned = call { ptr, i64 } @identity({ ptr, i64 } %descriptor)\n"
      "call void @unknown({ ptr, i64 } %returned)\n",
      "%indirect = load ptr, ptr @callee\n"
      "call void %indirect({ ptr, i64 } %descriptor)\n",
      "%again = call i1 @choose()\n"
      "br i1 %again, label %recurse, label %capture\n"
      "recurse:\n"
      "%recursive = call i8 @metadata({ ptr, i64 } %descriptor)\n"
      "ret i8 %recursive\n"
      "capture:\n"
      "call void @unknown({ ptr, i64 } %descriptor)\n",
      "call void @variadic(i64 0, { ptr, i64 } %descriptor)\n",
      "%weak = call i8 @replaceable({ ptr, i64 } %descriptor)\n",
      "call void @bundle() [ \"unknown\"({ ptr, i64 } %descriptor) ]\n"};
  for (auto *body : bodies) {
    SCOPED_TRACE(body);
    auto declarations = std::string(R"(
@escaped = global ptr null
@escaped_descriptor = global { ptr, i64 } zeroinitializer
@callee = external global ptr
declare void @seq_free(ptr)
declare void @unknown({ ptr, i64 })
declare void @variadic(i64, ...)
declare void @bundle()
declare i1 @choose()
declare i8 @read(ptr nocapture) nofree
define { ptr, i64 } @identity({ ptr, i64 } %descriptor) noinline optnone {
  ret { ptr, i64 } %descriptor
}
define weak i8 @replaceable({ ptr, i64 } %descriptor) noinline optnone {
  ret i8 0
}
define i8 @metadata({ ptr, i64 } %descriptor) noinline optnone {
)") + body + "ret i8 1\n}\n";
    auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(declarations, R"(
  %descriptor = insertvalue { ptr, i64 } zeroinitializer, ptr %allocation, 0
  %sized = insertvalue { ptr, i64 } %descriptor, i64 %index, 1
  %metadata = call i8 @metadata({ ptr, i64 } %sized)
  %byte = call i8 @read(ptr %allocation)
  %value = add i8 %metadata, %byte
)"));
    ASSERT_NE(nullptr, optimized.module);
    EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536,
                                    /*inLoopOnly=*/true),
              0);
    EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
  }
}

TEST(LLVMOptimizationTest, DoesNotHoistReadonlyCallWithoutNoCapture) {
  auto optimized = compileAndOptimizeIR(
      makeAllocationLoopIR("declare i8 @read_and_capture(ptr) nofree memory(read)\n",
                           "  %value = call i8 @read_and_capture(ptr %allocation)\n"));

  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536,
                                  /*inLoopOnly=*/true),
            0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistCallWithoutNoFree) {
  auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(
      "declare i8 @read_and_maybe_free(ptr nocapture)\n",
      "  %value = call i8 @read_and_maybe_free(ptr %allocation)\n"));

  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536,
                                  /*inLoopOnly=*/true),
            0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistPointerReturnedThroughAggregate) {
  auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(
      "@escaped = global ptr null\n"
      "declare { ptr, i8 } @return_and_read(ptr) nofree memory(read)\n",
      "  %result = call { ptr, i8 } @return_and_read(ptr %allocation)\n"
      "  %returned = extractvalue { ptr, i8 } %result, 0\n"
      "  store ptr %returned, ptr @escaped\n"
      "  %value = extractvalue { ptr, i8 } %result, 1\n"));

  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536,
                                  /*inLoopOnly=*/true),
            0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}

TEST(LLVMOptimizationTest, DoesNotHoistReallocatedPointer) {
  auto optimized = compileAndOptimizeIR(makeAllocationLoopIR(
      "declare ptr @seq_realloc(ptr, i64, i64)\n"
      "declare i8 @read(ptr nocapture) nofree memory(read)\n",
      "  %resized = call ptr @seq_realloc(ptr %allocation, i64 131072, i64 65536)\n"
      "  %value = call i8 @read(ptr %resized)\n"));

  ASSERT_NE(nullptr, optimized.module);
  EXPECT_GT(countFixedAllocations(optimized.module.get(), 65536,
                                  /*inLoopOnly=*/true),
            0);
  EXPECT_EQ(0, countLazyFixedAllocationCaches(optimized.module.get(), 65536));
}
