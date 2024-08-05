// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "engine.h"

#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/memory_manager.h"

namespace codon {
namespace jit {

Engine::Engine() : jit(), debug(nullptr) {
  auto eb = llvm::EngineBuilder();
  eb.setMArch(llvm::codegen::getMArch());
  eb.setMCPU(llvm::codegen::getCPUStr());
  eb.setMAttrs(llvm::codegen::getFeatureList());

  auto target = eb.selectTarget();
  auto layout = target->createDataLayout();
  auto epc = llvm::cantFail(llvm::orc::SelfExecutorProcessControl::Create(
      std::make_shared<llvm::orc::SymbolStringPool>()));

  llvm::orc::LLJITBuilder builder;
  builder.setDataLayout(layout);
  builder.setObjectLinkingLayerCreator(
      [&](llvm::orc::ExecutionSession &es, const llvm::Triple &triple)
          -> llvm::Expected<std::unique_ptr<llvm::orc::ObjectLayer>> {
        auto L = std::make_unique<llvm::orc::ObjectLinkingLayer>(
            es, llvm::cantFail(BoehmGCJITLinkMemoryManager::Create()));
        L->addPlugin(std::make_unique<llvm::orc::EHFrameRegistrationPlugin>(
            es, llvm::cantFail(llvm::orc::EPCEHFrameRegistrar::Create(es))));
        L->addPlugin(std::make_unique<llvm::orc::DebugObjectManagerPlugin>(
            es, llvm::cantFail(llvm::orc::createJITLoaderGDBRegistrar(es))));
        auto dbPlugin = std::make_unique<DebugPlugin>();
        this->debug = dbPlugin.get();
        L->addPlugin(std::move(dbPlugin));
        L->setAutoClaimResponsibilityForObjectSymbols(true);
        return L;
      });
  builder.setJITTargetMachineBuilder(
      llvm::orc::JITTargetMachineBuilder(target->getTargetTriple()));
  jit = llvm::cantFail(builder.create());

  jit->getMainJITDylib().addGenerator(
      llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          layout.getGlobalPrefix())));

  jit->getIRTransformLayer().setTransform(
      [&](llvm::orc::ThreadSafeModule module,
          const llvm::orc::MaterializationResponsibility &R) {
        module.withModuleDo([](llvm::Module &module) {
          ir::optimize(&module, /*debug=*/false, /*jit=*/true);
        });
        return std::move(module);
      });
}

llvm::Error Engine::addModule(llvm::orc::ThreadSafeModule module,
                              llvm::orc::ResourceTrackerSP rt) {
  if (!rt)
    rt = jit->getMainJITDylib().getDefaultResourceTracker();

  return jit->addIRModule(rt, std::move(module));
}

llvm::Expected<llvm::orc::ExecutorAddr> Engine::lookup(llvm::StringRef name) {
  return jit->lookup(name);
}

} // namespace jit
} // namespace codon
