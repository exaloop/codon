// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "native.h"

#include "codon/cir/llvm/llvm.h"
#include "codon/cir/llvm/native/targets/aarch64.h"
#include "codon/cir/llvm/native/targets/x86.h"

namespace codon {
namespace ir {
namespace {
std::unique_ptr<Target> getNativeTarget(const llvm::Triple &triple) {
  std::unique_ptr<Target> result = std::unique_ptr<Target>();
  switch (triple.getArch()) {
  default:
    break;
  case llvm::Triple::mips:
  case llvm::Triple::mipsel:
  case llvm::Triple::mips64:
  case llvm::Triple::mips64el:
    // nothing
    break;

  case llvm::Triple::arm:
  case llvm::Triple::armeb:
  case llvm::Triple::thumb:
  case llvm::Triple::thumbeb:
    // nothing
    break;

  case llvm::Triple::ppc:
  case llvm::Triple::ppcle:
  case llvm::Triple::ppc64:
  case llvm::Triple::ppc64le:
    // nothing
    break;
  case llvm::Triple::riscv32:
  case llvm::Triple::riscv64:
    // nothing
    break;
  case llvm::Triple::systemz:
    // nothing
    break;
  case llvm::Triple::aarch64:
  case llvm::Triple::aarch64_32:
  case llvm::Triple::aarch64_be:
    result = std::make_unique<Aarch64>();
    break;
  case llvm::Triple::x86:
  case llvm::Triple::x86_64:
    result = std::make_unique<X86>();
    break;
  case llvm::Triple::hexagon:
    // nothing
    break;
  case llvm::Triple::wasm32:
  case llvm::Triple::wasm64:
    // nothing
    break;
  case llvm::Triple::sparc:
  case llvm::Triple::sparcel:
  case llvm::Triple::sparcv9:
    // nothing
    break;
  case llvm::Triple::r600:
  case llvm::Triple::amdgcn:
    // nothing
    break;
  case llvm::Triple::msp430:
    // nothing
    break;
  case llvm::Triple::ve:
    // nothing
    break;
  }
  return result;
}

class ArchNativePass : public llvm::PassInfoMixin<ArchNativePass> {
private:
  std::string cpu;
  std::string features;

public:
  explicit ArchNativePass(const std::string &cpu = "", const std::string &features = "")
      : cpu(cpu), features(features) {}

  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &) {
    if (!cpu.empty())
      F.addFnAttr("target-cpu", cpu);
    if (!features.empty())
      F.addFnAttr("target-features", features);
    F.addFnAttr("frame-pointer", "none");
    return llvm::PreservedAnalyses::all();
  }
};
} // namespace

void addNativeLLVMPasses(llvm::PassBuilder *pb) {
  llvm::Triple triple = llvm::EngineBuilder().selectTarget()->getTargetTriple();
  auto target = getNativeTarget(triple);
  if (!target)
    return;
  std::string cpu = target->getCPU(triple);
  std::string features = target->getFeatures(triple);

  pb->registerPipelineEarlySimplificationEPCallback(
      [cpu, features](llvm::ModulePassManager &pm, llvm::OptimizationLevel opt,
                      llvm::ThinOrFullLTOPhase lto) {
        pm.addPass(
            llvm::createModuleToFunctionPassAdaptor(ArchNativePass(cpu, features)));
      });
}

} // namespace ir
} // namespace codon
