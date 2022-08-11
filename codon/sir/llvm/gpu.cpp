#include "gpu.h"

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
  for (char c : name) {
    if (c == '.' || c == '@') {
      validNameStream << "_$_";
    } else {
      validNameStream << c;
    }
  }

  return validNameStream.str();
}

void linkLibdevice(llvm::Module *M, const std::string &path) {
  llvm::SMDiagnostic err;
  auto libdevice = llvm::parseIRFile(path, err, M->getContext());
  if (!libdevice)
    compilationError(err.getMessage().str(), err.getFilename().str(), err.getLineNo(),
                     err.getColumnNo());
  llvm::Linker::linkModules(*M, std::move(libdevice), 0);
}

void remapFunctions(llvm::Module *M) {
  std::vector<std::pair<std::string, std::string>> remapping = {
    {"llvm.sin.f64", "__nv_sin"},
  };

  for (auto &pair : remapping) {
    if (auto *F = M->getFunction(pair.first)) {
      auto *G = M->getFunction(pair.second);
      if (!G) {
        G = llvm::Function::Create(F->getFunctionType(),
                                   llvm::GlobalValue::ExternalLinkage, pair.second, *M);
      }
      F->replaceAllUsesWith(G);
      F->dropAllReferences();
      F->eraseFromParent();
    }
  }
}

void moduleToPTX(llvm::Module *M, const std::string &filename,
                 std::vector<llvm::GlobalValue *> &kernels,
                 const std::string &cpuStr = "sm_20",
                 const std::string &featuresStr = "") {
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
  remapFunctions(M);
  linkLibdevice(M, LIBDEVICE_PATH);

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

    pm->add(llvm::createGVExtractionPass(kernels));

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

  llvm::errs() << *M << "\n";

  // Clean up names.
  {
    static int x = 0;

    for (auto &G : M->globals()) {
      if (G.hasLocalLinkage())
        G.setName("x" + std::to_string(x++));
    }

    for (auto &F : M->functions()) {
      if (F.hasLocalLinkage())
        F.setName("x" + std::to_string(x++));
    }
  }

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
    auto dummyName = "dummy_" + F->getName();
    auto *dummy = M->getFunction(dummyName.str());
    if (!dummy) {
      dummy = llvm::Function::Create(F->getFunctionType(),
                                     llvm::GlobalValue::PrivateLinkage, dummyName, *M);
      auto *entry = llvm::BasicBlock::Create(context, "entry", dummy);
      llvm::IRBuilder<> B(entry);

      auto *retType = F->getReturnType();
      if (retType->isVoidTy()) {
        B.CreateRetVoid();
      } else {
        B.CreateRet(llvm::UndefValue::get(retType));
      }
    }

    F->replaceAllUsesWith(dummy);
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
    F.setPersonalityFn(nullptr);
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
