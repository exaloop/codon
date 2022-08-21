#include "gpu.h"

#include <algorithm>
#include <memory>
#include <string>

#include "codon/util/common.h"
#include "llvm/CodeGen/CommandFlags.h"

namespace codon {
namespace ir {
namespace {
const std::string GPU_TRIPLE = "nvptx64-nvidia-cuda";
const std::string GPU_DL =
    "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-"
    "f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n16:32:64";
const std::string LIBDEVICE_PATH = "/usr/local/cuda/nvvm/libdevice/libdevice.10.bc";

std::string cleanUpName(llvm::StringRef name) {
  std::string validName;
  llvm::raw_string_ostream validNameStream(validName);

  auto valid = [](char c, bool first) {
    bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
    if (!first)
      ok = ok || ('0' <= c && c <= '9');
    return ok;
  };

  bool first = true;
  for (char c : name) {
    validNameStream << (valid(c, first) ? c : '_');
    first = false;
  }

  return validNameStream.str();
}

void linkLibdevice(llvm::Module *M, const std::string &path) {
  llvm::SMDiagnostic err;
  auto libdevice = llvm::parseIRFile(path, err, M->getContext());
  if (!libdevice)
    compilationError(err.getMessage().str(), err.getFilename().str(), err.getLineNo(),
                     err.getColumnNo());
  libdevice->setDataLayout(M->getDataLayout());
  libdevice->setTargetTriple(M->getTargetTriple());

  llvm::Linker L(*M);
  const bool fail = L.linkInModule(std::move(libdevice));
  seqassertn(!fail, "linking libdevice failed");
}

llvm::Function *copyPrototype(llvm::Function *F, const std::string &name) {
  auto *M = F->getParent();
  return llvm::Function::Create(F->getFunctionType(), llvm::GlobalValue::PrivateLinkage,
                                name.empty() ? F->getName() : name, *M);
}

llvm::Function *makeNoOp(llvm::Function *F) {
  auto *M = F->getParent();
  auto &context = M->getContext();
  auto dummyName = (".codon.gpu.dummy." + F->getName()).str();
  auto *dummy = M->getFunction(dummyName);
  if (!dummy) {
    dummy = copyPrototype(F, dummyName);
    auto *entry = llvm::BasicBlock::Create(context, "entry", dummy);
    llvm::IRBuilder<> B(entry);

    auto *retType = F->getReturnType();
    if (retType->isVoidTy()) {
      B.CreateRetVoid();
    } else {
      B.CreateRet(llvm::UndefValue::get(retType));
    }
  }
  return dummy;
}

using Codegen =
    std::function<void(llvm::IRBuilder<> &, const std::vector<llvm::Value *> &)>;

llvm::Function *makeFillIn(llvm::Function *F, Codegen codegen) {
  auto *M = F->getParent();
  auto &context = M->getContext();
  auto fillInName = (".codon.gpu.fillin." + F->getName()).str();
  auto *fillIn = M->getFunction(fillInName);
  if (!fillIn) {
    fillIn = copyPrototype(F, fillInName);
    std::vector<llvm::Value *> args;
    for (auto it = fillIn->arg_begin(); it != fillIn->arg_end(); ++it) {
      args.push_back(it);
    }
    auto *entry = llvm::BasicBlock::Create(context, "entry", fillIn);
    llvm::IRBuilder<> B(entry);
    codegen(B, args);
  }
  return fillIn;
}

llvm::Function *makeMalloc(llvm::Module *M) {
  auto &context = M->getContext();
  auto F = M->getOrInsertFunction("malloc", llvm::Type::getInt8PtrTy(context),
                                  llvm::Type::getInt64Ty(context));
  auto *G = llvm::cast<llvm::Function>(F.getCallee());
  G->setLinkage(llvm::GlobalValue::ExternalLinkage);
  G->setDoesNotThrow();
  G->setReturnDoesNotAlias();
  G->setOnlyAccessesInaccessibleMemory();
  G->setWillReturn();
  return G;
}

void remapFunctions(llvm::Module *M) {
  // simple name-to-name remappings
  static const std::vector<std::pair<std::string, std::string>> remapping = {
      // 64-bit float intrinsics
      {"llvm.ceil.f64", "__nv_ceil"},
      {"llvm.floor.f64", "__nv_floor"},
      {"llvm.fabs.f64", "__nv_fabs"},
      {"llvm.exp.f64", "__nv_exp"},
      {"llvm.log.f64", "__nv_log"},
      {"llvm.log2.f64", "__nv_log2"},
      {"llvm.log10.f64", "__nv_log10"},
      {"llvm.sqrt.f64", "__nv_sqrt"},
      {"llvm.pow.f64", "__nv_pow"},
      {"llvm.sin.f64", "__nv_sin"},
      {"llvm.cos.f64", "__nv_cos"},
      {"llvm.copysign.f64", "__nv_copysign"},
      {"llvm.trunc.f64", "__nv_trunc"},
      {"llvm.rint.f64", "__nv_rint"},
      {"llvm.nearbyint.f64", "__nv_nearbyint"},
      {"llvm.round.f64", "__nv_round"},
      {"llvm.minnum.f64", "__nv_fmin"},
      {"llvm.maxnum.f64", "__nv_fmax"},
      {"llvm.copysign.f64", "__nv_copysign"},
      {"llvm.fma.f64", "__nv_fma"},

      // 64-bit float math functions
      {"expm1", "__nv_expm1"},
      {"ldexp", "__nv_ldexp"},
      {"acos", "__nv_acos"},
      {"asin", "__nv_asin"},
      {"atan", "__nv_atan"},
      {"atan2", "__nv_atan2"},
      {"hypot", "__nv_hypot"},
      {"tan", "__nv_tan"},
      {"cosh", "__nv_cosh"},
      {"sinh", "__nv_sinh"},
      {"tanh", "__nv_tanh"},
      {"acosh", "__nv_acosh"},
      {"asinh", "__nv_asinh"},
      {"atanh", "__nv_atanh"},
      {"erf", "__nv_erf"},
      {"erfc", "__nv_erfc"},
      {"tgamma", "__nv_tgamma"},
      {"lgamma", "__nv_lgamma"},
      {"remainder", "__nv_remainder"},
      {"frexp", "__nv_frexp"},
      {"modf", "__nv_modf"},

      // 32-bit float intrinsics
      {"llvm.ceil.f32", "__nv_ceilf"},
      {"llvm.floor.f32", "__nv_floorf"},
      {"llvm.fabs.f32", "__nv_fabsf"},
      {"llvm.exp.f32", "__nv_expf"},
      {"llvm.log.f32", "__nv_logf"},
      {"llvm.log2.f32", "__nv_log2f"},
      {"llvm.log10.f32", "__nv_log10f"},
      {"llvm.sqrt.f32", "__nv_sqrtf"},
      {"llvm.pow.f32", "__nv_powf"},
      {"llvm.sin.f32", "__nv_sinf"},
      {"llvm.cos.f32", "__nv_cosf"},
      {"llvm.copysign.f32", "__nv_copysignf"},
      {"llvm.trunc.f32", "__nv_truncf"},
      {"llvm.rint.f32", "__nv_rintf"},
      {"llvm.nearbyint.f32", "__nv_nearbyintf"},
      {"llvm.round.f32", "__nv_roundf"},
      {"llvm.minnum.f32", "__nv_fminf"},
      {"llvm.maxnum.f32", "__nv_fmaxf"},
      {"llvm.copysign.f32", "__nv_copysignf"},
      {"llvm.fma.f32", "__nv_fmaf"},

      // 32-bit float math functions
      {"expm1f", "__nv_expm1f"},
      {"ldexpf", "__nv_ldexpf"},
      {"acosf", "__nv_acosf"},
      {"asinf", "__nv_asinf"},
      {"atanf", "__nv_atanf"},
      {"atan2f", "__nv_atan2f"},
      {"hypotf", "__nv_hypotf"},
      {"tanf", "__nv_tanf"},
      {"coshf", "__nv_coshf"},
      {"sinhf", "__nv_sinhf"},
      {"tanhf", "__nv_tanhf"},
      {"acoshf", "__nv_acoshf"},
      {"asinhf", "__nv_asinhf"},
      {"atanhf", "__nv_atanhf"},
      {"erff", "__nv_erff"},
      {"erfcf", "__nv_erfcf"},
      {"tgammaf", "__nv_tgammaf"},
      {"lgammaf", "__nv_lgammaf"},
      {"remainderf", "__nv_remainderf"},
      {"frexpf", "__nv_frexpf"},
      {"modff", "__nv_modff"},

      // runtime library functions
      {"seq_free", "free"},
      {"seq_register_finalizer", ""},
      {"seq_gc_add_roots", ""},
      {"seq_gc_remove_roots", ""},
      {"seq_gc_clear_roots", ""},
      {"seq_gc_exclude_static_roots", ""},
  };

  // functions that need to be generated as they're not available on GPU
  static const std::vector<std::pair<std::string, Codegen>> fillins = {
      {"seq_alloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_alloc_atomic",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[0]);
         B.CreateRet(mem);
       }},

      {"seq_realloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *mem = B.CreateCall(makeMalloc(M), args[1]);

         {
           auto F = llvm::Intrinsic::getDeclaration(
               M, llvm::Intrinsic::memcpy,
               {B.getInt8PtrTy(), B.getInt8PtrTy(), B.getInt64Ty()});
           B.CreateCall(F, {mem, args[0], args[1], B.getFalse()});
         }

         {
           auto F = M->getOrInsertFunction("free", B.getVoidTy(), B.getInt8PtrTy());
           auto *G = llvm::cast<llvm::Function>(F.getCallee());
           G->setLinkage(llvm::GlobalValue::ExternalLinkage);
           B.CreateCall(G, args[0]);
         }

         B.CreateRet(mem);
       }},

      {"seq_calloc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *size = B.CreateMul(args[0], args[1]);
         llvm::Value *mem = B.CreateCall(makeMalloc(M), size);
         auto F = llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::memset,
                                                  {B.getInt8PtrTy(), B.getInt64Ty()});
         B.CreateCall(F, {mem, B.getInt8(0), size, B.getFalse()});
         B.CreateRet(mem);
       }},

      {"seq_calloc_atomic",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         auto *M = B.GetInsertBlock()->getModule();
         llvm::Value *size = B.CreateMul(args[0], args[1]);
         llvm::Value *mem = B.CreateCall(makeMalloc(M), size);
         auto F = llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::memset,
                                                  {B.getInt8PtrTy(), B.getInt64Ty()});
         B.CreateCall(F, {mem, B.getInt8(0), size, B.getFalse()});
         B.CreateRet(mem);
       }},

      {"seq_alloc_exc",
       [](llvm::IRBuilder<> &B, const std::vector<llvm::Value *> &args) {
         // TODO: print error message and abort if in debug mode
         B.CreateUnreachable();
       }},

      {"seq_throw",
       [](llvm::IRBuilder<> &B,
          const std::vector<llvm::Value *> &args) { B.CreateUnreachable(); }},
  };

  for (auto &pair : remapping) {
    if (auto *F = M->getFunction(pair.first)) {
      llvm::Function *G = nullptr;
      if (pair.second.empty()) {
        G = makeNoOp(F);
      } else {
        G = M->getFunction(pair.second);
        if (!G)
          G = copyPrototype(F, pair.second);
      }

      G->setWillReturn();
      F->replaceAllUsesWith(G);
      F->dropAllReferences();
      F->eraseFromParent();
    }
  }

  for (auto &pair : fillins) {
    if (auto *F = M->getFunction(pair.first)) {
      llvm::Function *G = makeFillIn(F, pair.second);
      F->replaceAllUsesWith(G);
      F->dropAllReferences();
      F->eraseFromParent();
    }
  }
}

void exploreGV(llvm::GlobalValue *G, llvm::SmallPtrSetImpl<llvm::GlobalValue *> &keep) {
  if (keep.contains(G))
    return;

  keep.insert(G);
  if (auto *F = llvm::dyn_cast<llvm::Function>(G)) {
    for (auto I = llvm::inst_begin(F), E = inst_end(F); I != E; ++I) {
      for (auto &U : I->operands()) {
        if (auto *G2 = llvm::dyn_cast<llvm::GlobalValue>(U.get()))
          exploreGV(G2, keep);
      }
    }
  }
}

std::vector<llvm::GlobalValue *>
getRequiredGVs(const std::vector<llvm::GlobalValue *> &kernels) {
  llvm::SmallPtrSet<llvm::GlobalValue *, 32> keep;
  for (auto *G : kernels) {
    exploreGV(G, keep);
  }
  return std::vector<llvm::GlobalValue *>(keep.begin(), keep.end());
}

void moduleToPTX(llvm::Module *M, const std::string &filename,
                 std::vector<llvm::GlobalValue *> &kernels,
                 const std::string &cpuStr = "sm_20",
                 const std::string &featuresStr = "+ptx42") {
  llvm::Triple triple(llvm::Triple::normalize(GPU_TRIPLE));
  llvm::TargetLibraryInfoImpl tlii(triple);

  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget("nvptx64", triple, err);
  seqassertn(target, "couldn't lookup target: {}", err);

  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(triple);

  std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(
      triple.getTriple(), cpuStr, featuresStr, options,
      llvm::codegen::getExplicitRelocModel(), llvm::codegen::getExplicitCodeModel(),
      llvm::CodeGenOpt::Aggressive));

  M->setDataLayout(machine->createDataLayout());
  auto keep = getRequiredGVs(kernels);

  auto prune = [&](std::vector<llvm::GlobalValue *> keep) {
    auto pm = std::make_unique<llvm::legacy::PassManager>();
    pm->add(new llvm::TargetLibraryInfoWrapperPass(tlii));
    // Delete everything but kernel functions.
    pm->add(llvm::createGVExtractionPass(keep));
    // Delete unreachable globals.
    pm->add(llvm::createGlobalDCEPass());
    // Remove dead debug info.
    pm->add(llvm::createStripDeadDebugInfoPass());
    // Remove dead func decls.
    pm->add(llvm::createStripDeadPrototypesPass());
    pm->run(*M);
  };

  // Remove non-kernel functions.
  prune(keep);

  // Link libdevice and other cleanup.
  linkLibdevice(M, LIBDEVICE_PATH);
  remapFunctions(M);

  // Run NVPTX passes and general opt pipeline.
  {
    auto pm = std::make_unique<llvm::legacy::PassManager>();
    auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(M);
    pm->add(new llvm::TargetLibraryInfoWrapperPass(tlii));

    pm->add(llvm::createTargetTransformInfoWrapperPass(
        machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));
    fpm->add(llvm::createTargetTransformInfoWrapperPass(
        machine ? machine->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));

    if (machine) {
      auto &ltm = dynamic_cast<llvm::LLVMTargetMachine &>(*machine);
      llvm::Pass *tpc = ltm.createPassConfig(*pm);
      pm->add(tpc);
    }

    pm->add(llvm::createInternalizePass([&](const llvm::GlobalValue &gv) {
      return std::find(keep.begin(), keep.end(), &gv) != keep.end();
    }));

    llvm::PassManagerBuilder pmb;
    unsigned optLevel = 3, sizeLevel = 0;
    pmb.OptLevel = optLevel;
    pmb.SizeLevel = sizeLevel;
    pmb.Inliner = llvm::createFunctionInliningPass(optLevel, sizeLevel, false);
    pmb.DisableUnrollLoops = false;
    pmb.LoopVectorize = true;
    pmb.SLPVectorize = true;

    if (machine) {
      machine->adjustPassManager(pmb);
    }

    pmb.populateModulePassManager(*pm);
    pmb.populateFunctionPassManager(*fpm);

    fpm->doInitialization();
    for (llvm::Function &f : *M) {
      fpm->run(f);
    }
    fpm->doFinalization();
    pm->run(*M);
  }

  // Prune again after optimizations.
  keep = getRequiredGVs(kernels);
  prune(keep);

  // Clean up names.
  {
    for (auto &G : M->globals()) {
      if (G.hasLocalLinkage())
        G.setName(cleanUpName(G.getName()));
    }

    for (auto &F : M->functions()) {
      if (F.getInstructionCount() > 0)
        F.setName(cleanUpName(F.getName()));
    }

    for (auto *S : M->getIdentifiedStructTypes()) {
      S->setName(cleanUpName(S->getName()));
    }
  }

  llvm::errs() << *M << "\n";

  // Generate PTX file.
  {
    std::error_code errcode;
    auto out = std::make_unique<llvm::ToolOutputFile>(filename, errcode,
                                                      llvm::sys::fs::OF_Text);
    if (errcode)
      compilationError(errcode.message());
    llvm::raw_pwrite_stream *os = &out->os();

    auto &llvmtm = static_cast<llvm::LLVMTargetMachine &>(*machine);
    auto *mmiwp = new llvm::MachineModuleInfoWrapperPass(&llvmtm);
    llvm::legacy::PassManager pm;

    pm.add(new llvm::TargetLibraryInfoWrapperPass(tlii));
    seqassertn(!machine->addPassesToEmitFile(pm, *os, nullptr, llvm::CGFT_AssemblyFile,
                                             /*DisableVerify=*/false, mmiwp),
               "could not add passes");
    const_cast<llvm::TargetLoweringObjectFile *>(llvmtm.getObjFileLowering())
        ->Initialize(mmiwp->getMMI().getContext(), *machine);
    pm.run(*M);
    out->keep();
  }
}

void addInitCall(llvm::Module *M, const std::string &filename) {
  llvm::LLVMContext &context = M->getContext();
  auto f = M->getOrInsertFunction("seq_nvptx_init", llvm::Type::getVoidTy(context),
                                  llvm::Type::getInt8PtrTy(context));
  auto *g = llvm::cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();

  auto *init = M->getFunction("seq_init");
  seqassertn(init, "seq_init function not found in module");
  seqassertn(init->hasOneUse(), "seq_init used more than once");
  auto *use = llvm::dyn_cast<llvm::CallBase>(init->use_begin()->getUser());
  seqassertn(use, "seq_init use was not a call");

  auto *filenameVar = new llvm::GlobalVariable(
      *M, llvm::ArrayType::get(llvm::Type::getInt8Ty(context), filename.length() + 1),
      /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
      llvm::ConstantDataArray::getString(context, filename), ".nvptx.filename");
  filenameVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  llvm::IRBuilder<> B(context);
  B.SetInsertPoint(use->getNextNode());
  B.CreateCall(g, B.CreateBitCast(filenameVar, B.getInt8PtrTy()));
}

void cleanUpIntrinsics(llvm::Module *M) {
  llvm::LLVMContext &context = M->getContext();
  llvm::SmallVector<llvm::Function *, 16> remove;
  for (auto &F : *M) {
    if (F.getIntrinsicID() != llvm::Intrinsic::not_intrinsic &&
        F.getName().startswith("llvm.nvvm"))
      remove.push_back(&F);
  }

  for (auto *F : remove) {
    F->replaceAllUsesWith(makeNoOp(F));
    F->dropAllReferences();
    F->eraseFromParent();
  }
}
} // namespace

void applyGPUTransformations(llvm::Module *M) {
  llvm::LLVMContext &context = M->getContext();
  std::unique_ptr<llvm::Module> clone = llvm::CloneModule(*M);
  clone->setTargetTriple(llvm::Triple::normalize(GPU_TRIPLE));
  clone->setDataLayout(GPU_DL);

  llvm::NamedMDNode *nvvmAnno = clone->getOrInsertNamedMetadata("nvvm.annotations");

  unsigned idx = 0;
  std::vector<llvm::GlobalValue *> kernels;

  for (auto &F : *clone) {
    if (!F.hasFnAttribute("kernel"))
      continue;

    F.setName("kernel_" + std::to_string(idx++));

    llvm::Metadata *nvvmElem[] = {
        llvm::ConstantAsMetadata::get(&F),
        llvm::MDString::get(context, "kernel"),
        llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)),
    };

    nvvmAnno->addOperand(llvm::MDNode::get(context, nvvmElem));
    kernels.push_back(&F);
  }

  if (kernels.empty())
    return;

  const std::string filename = "kernel.ptx";
  moduleToPTX(clone.get(), filename, kernels);
  cleanUpIntrinsics(M);
  addInitCall(M, filename);
}

} // namespace ir
} // namespace codon
