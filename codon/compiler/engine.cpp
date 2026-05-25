// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "engine.h"

#include <dlfcn.h>

#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/memory_manager.h"

namespace codon {
namespace jit {

namespace {

llvm::Error makeRuntimeLoadError(const std::string &message) {
  return llvm::make_error<llvm::StringError>(message, llvm::inconvertibleErrorCode());
}

} // namespace

Engine::Engine() : jit(), debug(nullptr), globalPrefix('\0') {
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
        return L;
      });
  builder.setJITTargetMachineBuilder(
      llvm::orc::JITTargetMachineBuilder(target->getTargetTriple()));
  jit = llvm::cantFail(builder.create());
  globalPrefix = layout.getGlobalPrefix();

  jit->getMainJITDylib().addGenerator(llvm::cantFail(
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(globalPrefix)));
  jit->getMainJITDylib().addGenerator(llvm::cantFail(
      llvm::orc::DynamicLibrarySearchGenerator::Load(kLibgcc_sName, globalPrefix)));
  tryAddDynamicLibrarySearchGenerator(kLibstdcxxName);

  jit->getIRTransformLayer().setTransform(
      [&](llvm::orc::ThreadSafeModule module,
          const llvm::orc::MaterializationResponsibility &R) {
        module.withModuleDo([](llvm::Module &module) {
          ir::optimize(&module, /*debug=*/false, /*jit=*/true);
        });
        return std::move(module);
      });
}

Engine::~Engine() {
  jit.reset();
  for (auto *handle : runtimeHandles)
    dlclose(handle);
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

llvm::Error Engine::registerSymbols(
    llvm::function_ref<llvm::orc::SymbolMap(llvm::orc::MangleAndInterner)> symbolMap) {
  auto &mainJitDylib = jit->getMainJITDylib();
  return mainJitDylib.define(
      llvm::orc::absoluteSymbols(symbolMap(llvm::orc::MangleAndInterner(
          mainJitDylib.getExecutionSession(), jit->getDataLayout()))));
}

void Engine::tryAddDynamicLibrarySearchGenerator(const char *path) {
  auto gen = llvm::orc::DynamicLibrarySearchGenerator::Load(path, globalPrefix);
  if (!gen) {
    llvm::consumeError(gen.takeError());
    return;
  }
  jit->getMainJITDylib().addGenerator(std::move(*gen));
}

llvm::Error Engine::addRuntimeSymbolMap(const std::string &path) {
  void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    const char *err = dlerror();
    return makeRuntimeLoadError("cannot load codon runtime '" + path +
                                "': " + (err ? err : "unknown error"));
  }

  llvm::sys::DynamicLibrary lib(handle);
  void *initSym = lib.getAddressOfSymbol(kRuntimeInitFnName);

  if (!initSym) {
    const char *err = dlerror();
    dlclose(handle);
    return makeRuntimeLoadError("cannot find " + std::string(kRuntimeInitFnName) +
                                " in '" + path + "': " + (err ? err : "unknown error"));
  }

  RuntimeSymbolMap runtimeSymbols;
  auto initFn = reinterpret_cast<RuntimeInitFunc>(initSym);
  initFn(
      [](void *ctx, const char *name, void *address) {
        if (!ctx || !name || !address)
          return;
        auto *symbols = static_cast<RuntimeSymbolMap *>(ctx);
        (*symbols)[name] = address;
      },
      &runtimeSymbols);

  // Build a runtime symbol map from the exported symbols and register them.
  auto runtimeSymbolMap = [&](llvm::orc::MangleAndInterner interner) {
    auto symbolMap = llvm::orc::SymbolMap();
    for (auto &[name, address] : runtimeSymbols)
      symbolMap[interner(name)] = {llvm::orc::ExecutorAddr::fromPtr(address),
                                   llvm::JITSymbolFlags::Exported};
    return symbolMap;
  };
  if (auto err = registerSymbols(runtimeSymbolMap)) {
    dlclose(handle);
    return err;
  }

  runtimeHandles.push_back(handle);
  return llvm::Error::success();
}

} // namespace jit
} // namespace codon
