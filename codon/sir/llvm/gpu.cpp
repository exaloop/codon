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

std::unique_ptr<llvm::Module> createKernelModule(llvm::LLVMContext &context,
                                                 const std::string &filename) {
  auto M = std::make_unique<llvm::Module>("codon_gpu", context);
  M->setTargetTriple(GPU_TRIPLE);
  M->setDataLayout(GPU_DL);
  M->setSourceFileName(filename);
  return M;
}

void moduleToPTX(llvm::Module *M, const std::string &filename,
                 const std::string &cpuStr = "sm_20",
                 const std::string &featuresStr = "") {
  std::string err;
  llvm::Triple triple(GPU_TRIPLE);
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget("nvptx64", triple, err);
  seqassertn(target, "invalid target");

  const llvm::TargetOptions options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(triple);

  std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(
      triple.getTriple(), cpuStr, featuresStr, options,
      llvm::codegen::getExplicitRelocModel(), llvm::codegen::getExplicitCodeModel(),
      llvm::CodeGenOpt::Aggressive));

  std::error_code errcode;
  auto out =
      std::make_unique<llvm::ToolOutputFile>(filename, errcode, llvm::sys::fs::OF_Text);
  if (errcode)
    compilationError(errcode.message());
  llvm::raw_pwrite_stream *os = &out->os();

  auto &llvmtm = static_cast<llvm::LLVMTargetMachine &>(*machine);
  auto *mmiwp = new llvm::MachineModuleInfoWrapperPass(&llvmtm);
  llvm::legacy::PassManager pm;

  llvm::TargetLibraryInfoImpl tlii(triple);
  pm.add(new llvm::TargetLibraryInfoWrapperPass(tlii));
  seqassertn(!machine->addPassesToEmitFile(pm, *os, nullptr, llvm::CGFT_AssemblyFile,
                                           /*DisableVerify=*/false, mmiwp),
             "could not add passes");
  const_cast<llvm::TargetLoweringObjectFile *>(llvmtm.getObjFileLowering())
      ->Initialize(mmiwp->getMMI().getContext(), *machine);
  pm.run(*M);
  out->keep();
}

void addInitCall(llvm::Module *M, const std::string &filename) {
  llvm::LLVMContext &context = M->getContext();
  auto f = M->getOrInsertFunction("seq_nvptx_init", llvm::Type::getVoidTy(context),
                                  llvm::Type::getInt8PtrTy(context));
  auto *g = llvm::cast<llvm::Function>(f.getCallee());
  g->setDoesNotThrow();

  auto *init = M->getFunction("seq_init");
  seqassertn(init, "seq_init function not found in M");
  seqassertn(init->hasOneUse(), "seq_init used more than once");
  auto *use = llvm::dyn_cast<llvm::CallBase>(init->use_begin()->get());
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
} // namespace

void applyGPUTransformations(llvm::Module *M) {
  llvm::LLVMContext &context = M->getContext();
  auto kernelModule = createKernelModule(context, M->getSourceFileName());
  llvm::NamedMDNode *nvvmAnno =
      kernelModule->getOrInsertNamedMetadata("nvvm.annotations");
  llvm::ValueToValueMapTy vmap;

  for (auto &func : *M) {
    if (!func.hasFnAttribute("kernel"))
      continue;

    auto *clone = llvm::Function::Create(func.getFunctionType(), func.getLinkage(),
                                         func.getName(), *kernelModule);
    clone->copyAttributesFrom(&func);
    static int idx = 0;
    clone->setName("kernel_" + std::to_string(idx++));
    vmap[&func] = clone;

    auto cloneArg = clone->arg_begin();
    for (const auto &arg : func.args()) {
      cloneArg->setName(arg.getName());
      vmap[&arg] = &*cloneArg++;
    }

    llvm::SmallVector<llvm::ReturnInst *, 8> returns;
    llvm::CloneFunctionInto(clone, &func, vmap, /*ModuleLevelChanges=*/true, returns);
    clone->setPersonalityFn(nullptr);

    llvm::Metadata *nvvmElem[] = {
        llvm::ConstantAsMetadata::get(clone),
        llvm::MDString::get(context, "kernel"),
        llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1)),
    };

    nvvmAnno->addOperand(llvm::MDNode::get(context, nvvmElem));
  }

  const std::string filename = "kernel.ptx";
  llvm::errs() << *kernelModule << "\n";
  moduleToPTX(kernelModule.get(), filename);
  addInitCall(M, filename);
}

} // namespace ir
} // namespace codon
