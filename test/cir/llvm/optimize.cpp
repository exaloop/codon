#include "test.h"

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

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
} // namespace

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
