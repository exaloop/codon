// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "engine.h"

#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/memory_manager.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace codon {
namespace jit {

Engine::Engine(Options *options) : jit(), debug(nullptr), options(options) {
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
        if (auto regOrErr = llvm::orc::createJITLoaderGDBRegistrar(es)) {
          L->addPlugin(std::make_unique<llvm::orc::DebugObjectManagerPlugin>(
              es, std::move(*regOrErr)));
        }
        auto dbPlugin = std::make_unique<DebugPlugin>();
        this->debug = dbPlugin.get();
        L->addPlugin(std::move(dbPlugin));
        L->setAutoClaimResponsibilityForObjectSymbols(true);
#ifdef _WIN32
        addWin64SEHRegistration(*L);
#endif
        return L;
      });
  builder.setJITTargetMachineBuilder(
      llvm::orc::JITTargetMachineBuilder(target->getTargetTriple()));
  jit = llvm::cantFail(builder.create());
#ifdef _WIN32
  defineImageBase();
#endif

  jit->getMainJITDylib().addGenerator(
      llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          layout.getGlobalPrefix())));

  jit->getIRTransformLayer().setTransform(
      [&](llvm::orc::ThreadSafeModule module,
          const llvm::orc::MaterializationResponsibility &R) {
        module.withModuleDo(
            [this](llvm::Module &module) { ir::optimize(&module, this->options); });
        return std::move(module);
      });
}

#ifdef _WIN32
void Engine::defineImageBase() {
  uintptr_t handler = 0;
  if (HMODULE crt = ::GetModuleHandleW(L"vcruntime140.dll"))
    handler =
        reinterpret_cast<uintptr_t>(::GetProcAddress(crt, "__C_specific_handler"));
  // 3.5GB below the handler — MUST match memory_manager.cpp (allocateNearImage /
  // Win64SEHRegistrationPlugin) and llvisitor.cpp, so .xdata Pointer32NB relocs fit.
  uintptr_t imageBase = handler
                            ? (handler - 0xE0000000ull)
                            : reinterpret_cast<uintptr_t>(::GetModuleHandleW(nullptr));
  auto &jd = jit->getMainJITDylib();
  llvm::orc::MangleAndInterner mangle(jit->getExecutionSession(),
                                      jit->getDataLayout());
  llvm::orc::SymbolMap symbols;
  symbols[mangle("__ImageBase")] = {
      llvm::orc::ExecutorAddr(imageBase),
      llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Absolute};
  llvm::cantFail(jd.define(llvm::orc::absoluteSymbols(std::move(symbols))));
}
#endif

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
