#pragma once

#include <memory>
#include <vector>

#include "codon/sir/llvm/llvm.h"

#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/TPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/TargetProcessControl.h"

namespace codon {
namespace jit {

class Engine {
private:
  std::unique_ptr<llvm::orc::TargetProcessControl> tpc;
  std::unique_ptr<llvm::orc::ExecutionSession> sess;
  std::unique_ptr<llvm::orc::TPCIndirectionUtils> tpciu;

  llvm::DataLayout layout;
  llvm::orc::MangleAndInterner mangle;

  llvm::orc::RTDyldObjectLinkingLayer objectLayer;
  llvm::orc::IRCompileLayer compileLayer;
  llvm::orc::IRTransformLayer optimizeLayer;
  llvm::orc::CompileOnDemandLayer codLayer;

  llvm::orc::JITDylib &mainJD;

  static void handleLazyCallThroughError();

  static llvm::Expected<llvm::orc::ThreadSafeModule>
  optimizeModule(llvm::orc::ThreadSafeModule module,
                 const llvm::orc::MaterializationResponsibility &R);

public:
  Engine(std::unique_ptr<llvm::orc::TargetProcessControl> tpc,
         std::unique_ptr<llvm::orc::ExecutionSession> sess,
         std::unique_ptr<llvm::orc::TPCIndirectionUtils> tpciu,
         llvm::orc::JITTargetMachineBuilder jtmb, llvm::DataLayout layout);

  ~Engine();

  static llvm::Expected<std::unique_ptr<Engine>> create();

  const llvm::DataLayout &getDataLayout() const { return layout; }

  llvm::orc::JITDylib &getMainJITDylib() { return mainJD; }

  llvm::Error addModule(llvm::orc::ThreadSafeModule module,
                        llvm::orc::ResourceTrackerSP rt = nullptr);

  llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef name);
};

} // namespace jit
} // namespace codon
