#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

#include <cstdlib>

#include <llvm/AsmParser/Parser.h>
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
