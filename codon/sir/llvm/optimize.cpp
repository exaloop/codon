#include "optimize.h"

#include "codon/sir/llvm/coro/Coroutines.h"
#include "codon/util/common.h"
#include "llvm/CodeGen/CommandFlags.h"

static llvm::codegen::RegisterCodeGenFlags CFG;

namespace codon {
namespace ir {

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Triple triple, llvm::StringRef cpuStr,
                 llvm::StringRef featuresStr, const llvm::TargetOptions &options) {
  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), triple, err);

  if (!target)
    return nullptr;

  return std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
      triple.getTriple(), cpuStr, featuresStr, options,
      llvm::codegen::getExplicitRelocModel(), llvm::codegen::getExplicitCodeModel(),
      llvm::CodeGenOpt::Aggressive));
}

std::unique_ptr<llvm::TargetMachine> getTargetMachine(llvm::Module *module,
                                                      bool setFunctionAttributes) {
  llvm::Triple moduleTriple(module->getTargetTriple());
  std::string cpuStr, featuresStr;
  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(moduleTriple);
  llvm::TargetLibraryInfoImpl tlii(moduleTriple);

  auto pm = std::make_unique<llvm::legacy::PassManager>();
  auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module);
  pm->add(new llvm::TargetLibraryInfoWrapperPass(tlii));

  if (moduleTriple.getArch()) {
    cpuStr = llvm::codegen::getCPUStr();
    featuresStr = llvm::codegen::getFeaturesStr();
    auto machine = getTargetMachine(moduleTriple, cpuStr, featuresStr, options);
    if (setFunctionAttributes)
      llvm::codegen::setFunctionAttributes(cpuStr, featuresStr, *module);
    return machine;
  }
  return {};
}

namespace {
void applyDebugTransformations(llvm::Module *module, bool debug) {
  if (debug) {
    // remove tail calls and fix linkage for stack traces
    for (auto &f : *module) {
      f.setLinkage(llvm::GlobalValue::ExternalLinkage);
      if (f.hasFnAttribute(llvm::Attribute::AttrKind::AlwaysInline)) {
        f.removeFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
      }
      f.addFnAttr(llvm::Attribute::AttrKind::NoInline);
      f.setHasUWTable();
      f.addFnAttr("no-frame-pointer-elim", "true");
      f.addFnAttr("no-frame-pointer-elim-non-leaf");
      f.addFnAttr("no-jump-tables", "false");

      for (auto &block : f.getBasicBlockList()) {
        for (auto &inst : block) {
          if (auto *call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
            call->setTailCall(false);
          }
        }
      }
    }
  } else {
    llvm::StripDebugInfo(*module);
  }
}

/// Sometimes coroutine lowering produces hard-to-analyze loops involving
/// function pointer comparisons. This pass puts them into a somewhat
/// easier-to-analyze form.
struct CoroBranchSimplifier : public llvm::LoopPass {
  static char ID;
  CoroBranchSimplifier() : llvm::LoopPass(ID) {}

  static llvm::Value *getNonNullOperand(llvm::Value *op1, llvm::Value *op2) {
    auto *ptr = llvm::dyn_cast<llvm::PointerType>(op1->getType());
    if (!ptr || !ptr->getElementType()->isFunctionTy())
      return nullptr;

    auto *c1 = llvm::dyn_cast<llvm::Constant>(op1);
    auto *c2 = llvm::dyn_cast<llvm::Constant>(op2);
    const bool isNull1 = (c1 && c1->isNullValue());
    const bool isNull2 = (c2 && c2->isNullValue());
    if (!(isNull1 ^ isNull2))
      return nullptr;
    return isNull1 ? op2 : op1;
  }

  bool runOnLoop(llvm::Loop *loop, llvm::LPPassManager &lpm) override {
    if (auto *exit = loop->getExitingBlock()) {
      if (auto *br = llvm::dyn_cast<llvm::BranchInst>(exit->getTerminator())) {
        if (!br->isConditional() || br->getNumSuccessors() != 2 ||
            loop->contains(br->getSuccessor(0)) || !loop->contains(br->getSuccessor(1)))
          return false;

        auto *cond = br->getCondition();
        if (auto *cmp = llvm::dyn_cast<llvm::CmpInst>(cond)) {
          if (cmp->getPredicate() != llvm::CmpInst::Predicate::ICMP_EQ)
            return false;

          if (auto *f = getNonNullOperand(cmp->getOperand(0), cmp->getOperand(1))) {
            if (auto *sel = llvm::dyn_cast<llvm::SelectInst>(f)) {
              if (auto *g =
                      getNonNullOperand(sel->getTrueValue(), sel->getFalseValue())) {
                // If we can deduce that g is not null, we can replace the condition.
                if (auto *phi = llvm::dyn_cast<llvm::PHINode>(g)) {
                  bool ok = true;
                  for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
                    auto *phiBlock = phi->getIncomingBlock(i);
                    auto *phiValue = phi->getIncomingValue(i);

                    if (auto *c = llvm::dyn_cast<llvm::Constant>(phiValue)) {
                      if (c->isNullValue()) {
                        ok = false;
                        break;
                      }
                    } else {
                      // There is no way for the value to be null if the incoming phi
                      // value is predicated on this exit condition, which checks for a
                      // non-null function pointer.

                      if (phiBlock != exit || phiValue != f) {
                        ok = false;
                        break;
                      }
                    }
                  }
                  if (!ok)
                    return false;

                  br->setCondition(sel->getCondition());
                  return true;
                }
              }
            }
          }
        }
      }
    }
    return false;
  }
};

void addCoroutineBranchSimplifier(const llvm::PassManagerBuilder &builder,
                                  llvm::legacy::PassManagerBase &pm) {
  pm.add(new CoroBranchSimplifier());
}

char CoroBranchSimplifier::ID = 0;
llvm::RegisterPass<CoroBranchSimplifier> X("coro-br-simpl",
                                           "Coroutine Branch Simplifier");

void runLLVMOptimizationPasses(llvm::Module *module, bool debug,
                               PluginManager *plugins) {
  applyDebugTransformations(module, debug);

  llvm::Triple moduleTriple(module->getTargetTriple());
  llvm::TargetLibraryInfoImpl tlii(moduleTriple);

  auto pm = std::make_unique<llvm::legacy::PassManager>();
  auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module);
  pm->add(new llvm::TargetLibraryInfoWrapperPass(tlii));

  auto machine = getTargetMachine(module, /*setFunctionAttributes=*/true);
  pm->add(llvm::createTargetTransformInfoWrapperPass(
      machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));
  fpm->add(llvm::createTargetTransformInfoWrapperPass(
      machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));

  if (machine) {
    auto &ltm = dynamic_cast<llvm::LLVMTargetMachine &>(*machine);
    llvm::Pass *tpc = ltm.createPassConfig(*pm);
    pm->add(tpc);
  }

  unsigned optLevel = 3;
  unsigned sizeLevel = 0;
  llvm::PassManagerBuilder pmb;

  if (!debug) {
    pmb.OptLevel = optLevel;
    pmb.SizeLevel = sizeLevel;
    pmb.Inliner = llvm::createFunctionInliningPass(optLevel, sizeLevel, false);
    pmb.DisableUnrollLoops = false;
    pmb.LoopVectorize = true;
    pmb.SLPVectorize = true;
    // pmb.MergeFunctions = true;
  } else {
    pmb.OptLevel = 0;
  }

  if (machine) {
    machine->adjustPassManager(pmb);
  }

  coro::addCoroutinePassesToExtensionPoints(pmb);
  if (!debug) {
    pmb.addExtension(llvm::PassManagerBuilder::EP_LateLoopOptimizations,
                     addCoroutineBranchSimplifier);
  }

  if (plugins) {
    for (auto *plugin : *plugins) {
      plugin->dsl->addLLVMPasses(&pmb, debug);
    }
  }

  pmb.populateModulePassManager(*pm);
  pmb.populateFunctionPassManager(*fpm);

  fpm->doInitialization();
  for (llvm::Function &f : *module) {
    fpm->run(f);
  }
  fpm->doFinalization();
  pm->run(*module);
  applyDebugTransformations(module, debug);
}

void verify(llvm::Module *module) {
  const bool broken = llvm::verifyModule(*module, &llvm::errs());
  seqassert(!broken, "module broken");
}

} // namespace

void optimize(llvm::Module *module, bool debug, PluginManager *plugins) {
  verify(module);
  {
    TIME("llvm/opt");
    runLLVMOptimizationPasses(module, debug, plugins);
  }
  if (!debug) {
    TIME("llvm/opt2");
    runLLVMOptimizationPasses(module, debug, plugins);
  }
  verify(module);
}

} // namespace ir
} // namespace codon
