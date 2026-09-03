#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

#include <llvm/AsmParser/Parser.h>
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
} // namespace

TEST(LLVMOptimizationTest, RemovesUnusedStandardStreamInitialization) {
  auto compiler = compileAndOptimize("print(\"hello world\")\n");
  auto *module = compiler->getLLVMVisitor()->getModule();

  EXPECT_EQ(nullptr, module->getFunction("seq_alloc"));
  EXPECT_EQ(nullptr, module->getFunction("seq_stdin"));
  EXPECT_EQ(nullptr, module->getFunction("seq_stderr"));
  EXPECT_NE(nullptr, module->getFunction("seq_stdout"));

  unsigned definitions = 0;
  for (const auto &function : *module)
    definitions += !function.isDeclaration();
  EXPECT_EQ(1, definitions);
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
