// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "optimize.h"

#include <algorithm>

#include "codon/cir/llvm/gpu.h"
#include "codon/util/common.h"

static llvm::codegen::RegisterCodeGenFlags CFG;

namespace codon {
namespace ir {

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Triple triple, llvm::StringRef cpuStr,
                 llvm::StringRef featuresStr, const llvm::TargetOptions &options,
                 bool pic) {
  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), triple, err);

  if (!target)
    return nullptr;

  return std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
      triple.getTriple(), cpuStr, featuresStr, options,
      pic ? llvm::Reloc::Model::PIC_ : llvm::codegen::getExplicitRelocModel(),
      llvm::codegen::getExplicitCodeModel(), llvm::CodeGenOpt::Aggressive));
}

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Module *module, bool setFunctionAttributes, bool pic) {
  llvm::Triple moduleTriple(module->getTargetTriple());
  std::string cpuStr, featuresStr;
  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(moduleTriple);
  llvm::TargetLibraryInfoImpl tlii(moduleTriple);

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
#ifdef __APPLE__
      // needed for debug symbols
      if (!jit)
        f.setLinkage(llvm::GlobalValue::ExternalLinkage);
#endif
      if (!f.hasFnAttribute(llvm::Attribute::AttrKind::AlwaysInline))
        f.addFnAttr(llvm::Attribute::AttrKind::NoInline);
      f.setUWTableKind(llvm::UWTableKind::Default);
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
struct AllocationRemover : public llvm::PassInfoMixin<AllocationRemover> {
  std::string alloc;
  std::string allocAtomic;
  std::string realloc;
  std::string free;

  AllocationRemover(const std::string &alloc = "seq_alloc",
                    const std::string &allocAtomic = "seq_alloc_atomic",
                    const std::string &realloc = "seq_realloc",
                    const std::string &free = "seq_free")
      : alloc(alloc), allocAtomic(allocAtomic), realloc(realloc), free(free) {}

  static bool sizeOkToDemote(uint64_t size) { return 0 < size && size <= 1024; }

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

  static bool getFixedArg(llvm::CallBase &cb, uint64_t &size, unsigned idx = 0) {
    if (cb.arg_empty())
      return false;

    if (auto *ci = llvm::dyn_cast<llvm::ConstantInt>(cb.getArgOperand(idx))) {
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
        seqassertn(false, "missing a return?");
      }
    } while (!worklist.empty());
    return true;
  }

  bool isAllocSiteDemotable(llvm::Instruction *ai, uint64_t &size,
                            llvm::SmallVectorImpl<llvm::WeakTrackingVH> &users) {
    using namespace llvm;

    // Should never be an invoke, so just check right away.
    if (isa<InvokeInst>(ai))
      return false;

    if (!(getFixedArg(*dyn_cast<CallBase>(&*ai), size) && sizeOkToDemote(size)))
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
            // If the realloc also has constant small size,
            // then we can just update the assumed size to be
            // max of original alloc's and this realloc's.
            uint64_t newSize = 0;
            if (getFixedArg(*dyn_cast<CallBase>(instr), newSize, 1) &&
                sizeOkToDemote(newSize)) {
              size = std::max(size, newSize);
            } else {
              return false;
            }

            users.emplace_back(instr);
            worklist.push_back(instr);
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
        seqassertn(false, "missing a return?");
      }
    } while (!worklist.empty());
    return true;
  }

  void getErasesAndReplacementsForAlloc(
      llvm::Instruction &mi, llvm::SmallPtrSetImpl<llvm::Instruction *> &erase,
      llvm::SmallVectorImpl<std::pair<llvm::Instruction *, llvm::Value *>> &replace,
      llvm::SmallVectorImpl<llvm::AllocaInst *> &alloca,
      llvm::SmallVectorImpl<llvm::CallInst *> &untail) {
    using namespace llvm;

    uint64_t size = 0;
    SmallVector<WeakTrackingVH, 64> users;

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
        erase.insert(instr);
      }
      erase.insert(&mi);
      return;
    } else {
      users.clear();
    }

    if (isAllocSiteDemotable(&mi, size, users)) {
      auto *replacement = new AllocaInst(
          Type::getInt8Ty(mi.getContext()), 0,
          ConstantInt::get(Type::getInt64Ty(mi.getContext()), size), Align());
      alloca.push_back(replacement);
      replace.emplace_back(&mi, replacement);
      erase.insert(&mi);

      for (unsigned i = 0, e = users.size(); i != e; ++i) {
        if (!users[i])
          continue;

        Instruction *instr = cast<Instruction>(&*users[i]);
        if (isFree(instr)) {
          erase.insert(instr);
        } else if (isRealloc(instr)) {
          replace.emplace_back(instr, replacement);
          erase.insert(instr);
        } else if (auto *ci = dyn_cast<CallInst>(&*instr)) {
          if (ci->isTailCall() || ci->isMustTailCall())
            untail.push_back(ci);
        }
      }
    }
  }

  llvm::PreservedAnalyses run(llvm::Function &func, llvm::FunctionAnalysisManager &am) {
    using namespace llvm;

    SmallSet<Instruction *, 32> erase;
    SmallVector<std::pair<Instruction *, llvm::Value *>, 32> replace;
    SmallVector<AllocaInst *, 32> alloca;
    SmallVector<CallInst *, 32> untail;

    for (inst_iterator instr = inst_begin(func), end = inst_end(func); instr != end;
         ++instr) {
      auto *cb = dyn_cast<CallBase>(&*instr);
      if (!cb || !isAlloc(cb))
        continue;

      getErasesAndReplacementsForAlloc(*cb, erase, replace, alloca, untail);
    }

    for (auto *A : alloca) {
      A->insertBefore(func.getEntryBlock().getFirstNonPHI());
    }

    for (auto *C : untail) {
      C->setTailCall(false);
    }

    for (auto &P : replace) {
      P.first->replaceAllUsesWith(P.second);
    }

    for (auto *I : erase) {
      I->dropAllReferences();
    }

    for (auto *I : erase) {
      I->eraseFromParent();
    }

    if (!erase.empty() || !replace.empty() || !alloca.empty() || !untail.empty())
      return PreservedAnalyses::none();
    else
      return PreservedAnalyses::all();
  }
};

/// Sometimes coroutine lowering produces hard-to-analyze loops involving
/// function pointer comparisons. This pass puts them into a somewhat
/// easier-to-analyze form.
struct CoroBranchSimplifier : public llvm::PassInfoMixin<CoroBranchSimplifier> {
  static llvm::Value *getNonNullOperand(llvm::Value *op1, llvm::Value *op2) {
    auto *ptr = llvm::dyn_cast<llvm::PointerType>(op1->getType());
    if (!ptr)
      return nullptr;

    auto *c1 = llvm::dyn_cast<llvm::Constant>(op1);
    auto *c2 = llvm::dyn_cast<llvm::Constant>(op2);
    const bool isNull1 = (c1 && c1->isNullValue());
    const bool isNull2 = (c2 && c2->isNullValue());
    if (!(isNull1 ^ isNull2))
      return nullptr;
    return isNull1 ? op2 : op1;
  }

  llvm::PreservedAnalyses run(llvm::Loop &loop, llvm::LoopAnalysisManager &am,
                              llvm::LoopStandardAnalysisResults &ar,
                              llvm::LPMUpdater &u) {
    if (auto *exit = loop.getExitingBlock()) {
      if (auto *br = llvm::dyn_cast<llvm::BranchInst>(exit->getTerminator())) {
        if (!br->isConditional() || br->getNumSuccessors() != 2 ||
            loop.contains(br->getSuccessor(0)) || !loop.contains(br->getSuccessor(1)))
          return llvm::PreservedAnalyses::all();

        auto *cond = br->getCondition();
        if (auto *cmp = llvm::dyn_cast<llvm::CmpInst>(cond)) {
          if (cmp->getPredicate() != llvm::CmpInst::Predicate::ICMP_EQ)
            return llvm::PreservedAnalyses::all();

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
                    return llvm::PreservedAnalyses::all();

                  br->setCondition(sel->getCondition());
                  return llvm::PreservedAnalyses::none();
                }
              }
            }
          }
        }
      }
    }
    return llvm::PreservedAnalyses::all();
  }
};

void runLLVMOptimizationPasses(llvm::Module *module, bool debug, bool jit,
                               PluginManager *plugins) {
  applyDebugTransformations(module, debug, jit);

  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;
  auto machine = getTargetMachine(module, /*setFunctionAttributes=*/true);
  llvm::PassBuilder pb(machine.get());

  llvm::Triple moduleTriple(module->getTargetTriple());
  llvm::TargetLibraryInfoImpl tlii(moduleTriple);
  fam.registerPass([&] { return llvm::TargetLibraryAnalysis(tlii); });

  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  pb.registerLateLoopOptimizationsEPCallback(
      [&](llvm::LoopPassManager &pm, llvm::OptimizationLevel opt) {
        if (opt.isOptimizingForSpeed())
          pm.addPass(CoroBranchSimplifier());
      });

  pb.registerPeepholeEPCallback(
      [&](llvm::FunctionPassManager &pm, llvm::OptimizationLevel opt) {
        if (opt.isOptimizingForSpeed())
          pm.addPass(AllocationRemover());
      });

  if (plugins) {
    for (auto *plugin : *plugins) {
      plugin->dsl->addLLVMPasses(&pb, debug);
    }
  }

  if (debug) {
    llvm::ModulePassManager mpm =
        pb.buildO0DefaultPipeline(llvm::OptimizationLevel::O0);
    mpm.run(*module, mam);
  } else {
    llvm::ModulePassManager mpm =
        pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    mpm.run(*module, mam);
  }

  applyDebugTransformations(module, debug, jit);
}

void verify(llvm::Module *module) {
  const bool broken = llvm::verifyModule(*module, &llvm::errs());
  seqassertn(!broken, "module broken");
}

} // namespace

void optimize(llvm::Module *module, bool debug, bool jit, PluginManager *plugins) {
  verify(module);
  {
    TIME("llvm/opt1");
    runLLVMOptimizationPasses(module, debug, jit, plugins);
  }
  if (!debug) {
    TIME("llvm/opt2");
    runLLVMOptimizationPasses(module, debug, jit, plugins);
  }
  {
    TIME("llvm/gpu");
    applyGPUTransformations(module);
  }
  verify(module);
}

} // namespace ir
} // namespace codon
