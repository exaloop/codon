// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <vector>

#include "codon/cir/llvm/llvm.h"
#include "codon/compiler/debug_listener.h"

namespace codon {
namespace jit {

class Engine {
private:
  std::unique_ptr<llvm::orc::ExecutionSession> sess;
  std::unique_ptr<llvm::orc::EPCIndirectionUtils> epciu;

  llvm::DataLayout layout;
  llvm::orc::MangleAndInterner mangle;

  llvm::orc::RTDyldObjectLinkingLayer objectLayer;
  llvm::orc::IRCompileLayer compileLayer;
  llvm::orc::IRTransformLayer optimizeLayer;
  llvm::orc::CompileOnDemandLayer codLayer;

  llvm::orc::JITDylib &mainJD;

  std::unique_ptr<DebugListener> dbListener;

  static void handleLazyCallThroughError();

  static llvm::Expected<llvm::orc::ThreadSafeModule>
  optimizeModule(llvm::orc::ThreadSafeModule module,
                 const llvm::orc::MaterializationResponsibility &R);

public:
  Engine(std::unique_ptr<llvm::orc::ExecutionSession> sess,
         std::unique_ptr<llvm::orc::EPCIndirectionUtils> epciu,
         llvm::orc::JITTargetMachineBuilder jtmb, llvm::DataLayout layout);

  ~Engine();

  static llvm::Expected<std::unique_ptr<Engine>> create();

  const llvm::DataLayout &getDataLayout() const { return layout; }

  llvm::orc::JITDylib &getMainJITDylib() { return mainJD; }

  DebugListener *getDebugListener() const { return dbListener.get(); }

  llvm::Error addModule(llvm::orc::ThreadSafeModule module,
                        llvm::orc::ResourceTrackerSP rt = nullptr);

  llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef name);
};

} // namespace jit
} // namespace codon
