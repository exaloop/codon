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
void applyDebugTransformations(llvm::Module *module, bool debug, bool jit) {
  if (debug) {
    // remove tail calls and fix linkage for stack traces
    for (auto &f : *module) {
      if (!jit)
        f.setLinkage(llvm::GlobalValue::ExternalLinkage);
      if (!f.hasFnAttribute(llvm::Attribute::AttrKind::AlwaysInline))
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

/// Lowers allocations of known, small size to alloca when possible.
/// Also removes unused allocations.
struct AllocationRemover : public llvm::FunctionPass {
  std::string alloc;
  std::string allocAtomic;
  std::string realloc;
  std::string free;

  static char ID;
  AllocationRemover(const std::string &alloc = "seq_alloc",
                    const std::string &allocAtomic = "seq_alloc_atomic",
                    const std::string &realloc = "seq_realloc",
                    const std::string &free = "seq_free")
      : llvm::FunctionPass(ID), alloc(alloc), allocAtomic(allocAtomic),
        realloc(realloc), free(free) {}

  static const llvm::Function *getCalledFunction(const llvm::Value *value) {
    // Don't care about intrinsics in this case.
    if (llvm::isa<llvm::IntrinsicInst>(value))
      return nullptr;

    const auto *cb = llvm::dyn_cast<llvm::CallBase>(value);
    if (!cb)
      return nullptr;

    if (const llvm::Function *callee = cb->getCalledFunction())
      return callee;
    return nullptr;
  }

  bool isAlloc(const llvm::Value *value) {
    if (auto *func = getCalledFunction(value)) {
      return func->arg_size() == 1 &&
             (func->getName() == alloc || func->getName() == allocAtomic);
    }
    return false;
  }

  bool isRealloc(const llvm::Value *value) {
    if (auto *func = getCalledFunction(value)) {
      return func->arg_size() == 2 && func->getName() == realloc;
    }
    return false;
  }

  bool isFree(const llvm::Value *value) {
    if (auto *func = getCalledFunction(value)) {
      return func->arg_size() == 1 && func->getName() == free;
    }
    return false;
  }

  static bool getFixedArg(llvm::CallBase &cb, uint64_t &size) {
    if (cb.arg_empty())
      return false;

    if (auto *ci = llvm::dyn_cast<llvm::ConstantInt>(*cb.arg_begin())) {
      size = ci->getZExtValue();
      return true;
    }

    return false;
  }

  bool isNeverEqualToUnescapedAlloc(llvm::Value *value, llvm::Instruction *ai) {
    using namespace llvm;

    if (isa<ConstantPointerNull>(value))
      return true;
    if (auto *li = dyn_cast<LoadInst>(value))
      return isa<GlobalVariable>(li->getPointerOperand());
    // Two distinct allocations will never be equal.
    return isAlloc(value) && value != ai;
  }

  bool isAllocSiteRemovable(llvm::Instruction *ai,
                            llvm::SmallVectorImpl<llvm::WeakTrackingVH> &users) {
    using namespace llvm;

    // Should never be an invoke, so just check right away.
    if (isa<InvokeInst>(ai))
      return false;

    SmallVector<Instruction *, 4> worklist;
    worklist.push_back(ai);

    do {
      Instruction *pi = worklist.pop_back_val();
      for (User *u : pi->users()) {
        Instruction *instr = cast<Instruction>(u);
        switch (instr->getOpcode()) {
        default:
          // Give up the moment we see something we can't handle.
          return false;

        case Instruction::AddrSpaceCast:
        case Instruction::BitCast:
        case Instruction::GetElementPtr:
          users.emplace_back(instr);
          worklist.push_back(instr);
          continue;

        case Instruction::ICmp: {
          ICmpInst *cmp = cast<ICmpInst>(instr);
          // We can fold eq/ne comparisons with null to false/true, respectively.
          // We also fold comparisons in some conditions provided the alloc has
          // not escaped (see isNeverEqualToUnescapedAlloc).
          if (!cmp->isEquality())
            return false;
          unsigned otherIndex = (cmp->getOperand(0) == pi) ? 1 : 0;
          if (!isNeverEqualToUnescapedAlloc(cmp->getOperand(otherIndex), ai))
            return false;
          users.emplace_back(instr);
          continue;
        }

        case Instruction::Call:
          // Ignore no-op and store intrinsics.
          if (IntrinsicInst *intrinsic = dyn_cast<IntrinsicInst>(instr)) {
            switch (intrinsic->getIntrinsicID()) {
            default:
              return false;

            case Intrinsic::memmove:
            case Intrinsic::memcpy:
            case Intrinsic::memset: {
              MemIntrinsic *MI = cast<MemIntrinsic>(intrinsic);
              if (MI->isVolatile() || MI->getRawDest() != pi)
                return false;
              LLVM_FALLTHROUGH;
            }
            case Intrinsic::assume:
            case Intrinsic::invariant_start:
            case Intrinsic::invariant_end:
            case Intrinsic::lifetime_start:
            case Intrinsic::lifetime_end:
              users.emplace_back(instr);
              continue;
            case Intrinsic::launder_invariant_group:
            case Intrinsic::strip_invariant_group:
              users.emplace_back(instr);
              worklist.push_back(instr);
              continue;
            }
          }

          if (isFree(instr)) {
            users.emplace_back(instr);
            continue;
          }

          if (isRealloc(instr)) {
            users.emplace_back(instr);
            worklist.push_back(instr);
            continue;
          }

          return false;

        case Instruction::Store: {
          StoreInst *si = cast<StoreInst>(instr);
          if (si->isVolatile() || si->getPointerOperand() != pi)
            return false;
          users.emplace_back(instr);
          continue;
        }
        }
        seqassert(false, "missing a return?");
      }
    } while (!worklist.empty());
    return true;
  }

  bool isAllocSiteDemotable(llvm::Instruction *ai, uint64_t &size,
                            llvm::SmallVectorImpl<llvm::WeakTrackingVH> &frees) {
    using namespace llvm;

    // Should never be an invoke, so just check right away.
    if (isa<InvokeInst>(ai))
      return false;

    if (!(getFixedArg(*dyn_cast<CallBase>(&*ai), size) && size <= 1024))
      return false;

    SmallVector<Instruction *, 4> worklist;
    worklist.push_back(ai);

    do {
      Instruction *pi = worklist.pop_back_val();
      for (User *u : pi->users()) {
        Instruction *instr = cast<Instruction>(u);
        switch (instr->getOpcode()) {
        default:
          // Give up the moment we see something we can't handle.
          return false;

        case Instruction::AddrSpaceCast:
        case Instruction::BitCast:
        case Instruction::GetElementPtr:
          worklist.push_back(instr);
          continue;

        case Instruction::ICmp: {
          ICmpInst *cmp = cast<ICmpInst>(instr);
          // We can fold eq/ne comparisons with null to false/true, respectively.
          // We also fold comparisons in some conditions provided the alloc has
          // not escaped (see isNeverEqualToUnescapedAlloc).
          if (!cmp->isEquality())
            return false;
          unsigned otherIndex = (cmp->getOperand(0) == pi) ? 1 : 0;
          if (!isNeverEqualToUnescapedAlloc(cmp->getOperand(otherIndex), ai))
            return false;
          continue;
        }

        case Instruction::Call:
          // Ignore no-op and store intrinsics.
          if (IntrinsicInst *intrinsic = dyn_cast<IntrinsicInst>(instr)) {
            switch (intrinsic->getIntrinsicID()) {
            default:
              return false;

            case Intrinsic::memmove:
            case Intrinsic::memcpy:
            case Intrinsic::memset: {
              MemIntrinsic *MI = cast<MemIntrinsic>(intrinsic);
              if (MI->isVolatile())
                return false;
              LLVM_FALLTHROUGH;
            }
            case Intrinsic::assume:
            case Intrinsic::invariant_start:
            case Intrinsic::invariant_end:
            case Intrinsic::lifetime_start:
            case Intrinsic::lifetime_end:
              continue;
            case Intrinsic::launder_invariant_group:
            case Intrinsic::strip_invariant_group:
              worklist.push_back(instr);
              continue;
            }
          }

          if (isFree(instr)) {
            frees.emplace_back(instr);
            continue;
          }

          return false;

        case Instruction::Store: {
          StoreInst *si = cast<StoreInst>(instr);
          if (si->isVolatile() || si->getPointerOperand() != pi)
            return false;
          continue;
        }

        case Instruction::Load: {
          LoadInst *li = cast<LoadInst>(instr);
          if (li->isVolatile())
            return false;
          continue;
        }
        }
        seqassert(false, "missing a return?");
      }
    } while (!worklist.empty());
    return true;
  }

  void getErasesAndReplacementsForAlloc(
      llvm::Instruction &mi, llvm::SmallVectorImpl<llvm::Instruction *> &erase,
      llvm::SmallVectorImpl<std::pair<llvm::Instruction *, llvm::Value *>> &replace,
      llvm::SmallVectorImpl<llvm::AllocaInst *> &alloca) {
    using namespace llvm;

    uint64_t size = 0;
    SmallVector<WeakTrackingVH, 64> users;
    SmallVector<WeakTrackingVH, 64> frees;

    if (isAllocSiteRemovable(&mi, users)) {
      for (unsigned i = 0, e = users.size(); i != e; ++i) {
        if (!users[i])
          continue;

        Instruction *instr = cast<Instruction>(&*users[i]);
        if (ICmpInst *cmp = dyn_cast<ICmpInst>(instr)) {
          replace.emplace_back(cmp, ConstantInt::get(Type::getInt1Ty(cmp->getContext()),
                                                     cmp->isFalseWhenEqual()));
        } else if (!isa<StoreInst>(instr)) {
          // Casts, GEP, or anything else: we're about to delete this instruction,
          // so it can not have any valid uses.
          replace.emplace_back(instr, PoisonValue::get(instr->getType()));
        }
        erase.push_back(instr);
      }
      erase.push_back(&mi);
    } else if (isAllocSiteDemotable(&mi, size, frees)) {
      auto *replacement = new AllocaInst(
          Type::getInt8Ty(mi.getContext()), 0,
          ConstantInt::get(Type::getInt64Ty(mi.getContext()), size), Align());
      alloca.push_back(replacement);
      replace.emplace_back(&mi, replacement);
      erase.push_back(&mi);

      for (unsigned i = 0, e = frees.size(); i != e; ++i) {
        if (!frees[i])
          continue;

        Instruction *instr = cast<Instruction>(&*frees[i]);
        erase.push_back(instr);
      }
    }
  }

  bool runOnFunction(llvm::Function &func) override {
    using namespace llvm;

    llvm::SmallVector<Instruction *, 32> erase;
    llvm::SmallVector<std::pair<Instruction *, llvm::Value *>, 32> replace;
    llvm::SmallVector<AllocaInst *, 32> alloca;

    for (inst_iterator instr = inst_begin(func), end = inst_end(func); instr != end;
         ++instr) {
      auto *cb = dyn_cast<CallBase>(&*instr);
      if (!cb || !isAlloc(cb))
        continue;

      getErasesAndReplacementsForAlloc(*cb, erase, replace, alloca);
    }

    for (auto *A : alloca) {
      A->insertBefore(func.getEntryBlock().getFirstNonPHI());
    }

    for (auto &P : replace) {
      P.first->replaceAllUsesWith(P.second);
    }

    for (auto *I : erase) {
      I->eraseFromParent();
    }

    return !erase.empty() || !replace.empty() || !alloca.empty();
  }
};

void addAllocationRemover(const llvm::PassManagerBuilder &builder,
                          llvm::legacy::PassManagerBase &pm) {
  pm.add(new AllocationRemover());
}

char AllocationRemover::ID = 0;
llvm::RegisterPass<AllocationRemover> X1("alloc-remove", "Allocation Remover");

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
llvm::RegisterPass<CoroBranchSimplifier> X2("coro-br-simpl",
                                            "Coroutine Branch Simplifier");

void runLLVMOptimizationPasses(llvm::Module *module, bool debug, bool jit,
                               PluginManager *plugins) {
  applyDebugTransformations(module, debug, jit);

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
    pmb.addExtension(llvm::PassManagerBuilder::EP_LoopOptimizerEnd,
                     addAllocationRemover);
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
  applyDebugTransformations(module, debug, jit);
}

void verify(llvm::Module *module) {
  const bool broken = llvm::verifyModule(*module, &llvm::errs());
  seqassert(!broken, "module broken");
}

} // namespace

void optimize(llvm::Module *module, bool debug, bool jit, PluginManager *plugins) {
  verify(module);
  {
    TIME("llvm/opt");
    runLLVMOptimizationPasses(module, debug, jit, plugins);
  }
  if (!debug) {
    TIME("llvm/opt2");
    runLLVMOptimizationPasses(module, debug, jit, plugins);
  }
  verify(module);
}

} // namespace ir
} // namespace codon
