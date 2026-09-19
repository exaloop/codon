#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

#include <cstdlib>

#include <llvm/AsmParser/Parser.h>
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
        EXPECT_EQ(countReleases("retain_loop_carried"), 0);
        EXPECT_EQ(countReleases("retain_container"), 0);
        EXPECT_EQ(countReleases("retain_inplace_result"), 0);
        EXPECT_EQ(countReleases("retain_pointer"), 0);
        EXPECT_EQ(countReleases("retain_branch"), 0);
        EXPECT_GE(countReleases("release_loop_update"), 1);
        EXPECT_EQ(countReleases("release_reassigned_update"), 2);
        EXPECT_EQ(countReleases("retain_conjugate"), 0);
        EXPECT_EQ(countReleases("retain_transferred"), 0);
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
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
        if (dots != 1 || releases != 2)
          llvm::errs() << "Reversed dot: BLAS calls=" << dots
                       << ", scratch releases=" << releases << '\n';
        EXPECT_EQ(1, dots);
        EXPECT_EQ(2, releases);
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
def fused_scalar_coefficients(values: np.ndarray[float, 2], dx: float, dy: float):
  return (values[1:, :-1] / (2 * dx) + values[:-1, 1:] * (1 / dy)) / (2 * (dx ** 2 + dy ** 2))

@export
def fused_sum(values: np.ndarray[float, 1]):
    return np.sum(values.astype(np.float32).copy() * 2 + 1.0)

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
        for (auto name : {"fused_helper", "fused_copy_array", "fused_cast_array",
                          "fused_filled_array", "fused_axis_sum", "fused_axis_any",
                          "fused_axis_min", "fused_axis_keepdims", "fused_slices",
                          "fused_scalar_coefficients"}) {
          auto *function = module->getFunction(name);
          ASSERT_NE(nullptr, function);
          auto allocations = allocationCount(function);
          if (allocations != 1)
            function->print(llvm::errs());
          EXPECT_EQ(1, allocations) << name;
        }
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
        ASSERT_NE(nullptr, clipped);
        EXPECT_EQ(2, reachableAllocations(clipped));
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
        for (auto name : {"fused_sum", "fused_prod", "fused_filled", "fused_sum_2d",
                          "fused_sum_transpose", "fused_sum_3d", "fused_prod_3d",
                          "fused_any", "fused_all", "fused_min", "fused_max",
                          "fused_amin", "fused_amax", "fused_producers_2d",
                          "fused_filled_3d", "fused_cast_4d", "fused_any_producer"}) {
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
  auto optimized = compileAndOptimizeIR(R"(
declare noalias ptr @seq_alloc_atomic(i64)
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
