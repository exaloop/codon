// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include "engine.h"

#include "codon/cir/llvm/optimize.h"
#include "codon/compiler/memory_manager.h"

namespace codon {
namespace jit {

void Engine::handleLazyCallThroughError() {
  llvm::errs() << "LazyCallThrough error: Could not find function body";
  exit(1);
}

llvm::Expected<llvm::orc::ThreadSafeModule>
Engine::optimizeModule(llvm::orc::ThreadSafeModule module,
                       const llvm::orc::MaterializationResponsibility &R) {
  module.withModuleDo([](llvm::Module &module) {
    ir::optimize(&module, /*debug=*/false, /*jit=*/true);
  });
  return std::move(module);
}

Engine::Engine(std::unique_ptr<llvm::orc::ExecutionSession> sess,
               std::unique_ptr<llvm::orc::EPCIndirectionUtils> epciu,
               llvm::orc::JITTargetMachineBuilder jtmb, llvm::DataLayout layout)
    : sess(std::move(sess)), epciu(std::move(epciu)), layout(std::move(layout)),
      mangle(*this->sess, this->layout),
      objectLayer(*this->sess,
                  []() { return std::make_unique<BoehmGCMemoryManager>(); }),
      compileLayer(*this->sess, objectLayer,
                   std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb))),
      optimizeLayer(*this->sess, compileLayer, optimizeModule),
      codLayer(*this->sess, optimizeLayer, this->epciu->getLazyCallThroughManager(),
               [this] { return this->epciu->createIndirectStubsManager(); }),
      mainJD(this->sess->createBareJITDylib("<main>")),
      dbListener(std::make_unique<DebugListener>()) {
  mainJD.addGenerator(
      llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          layout.getGlobalPrefix())));
  objectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
  objectLayer.registerJITEventListener(*dbListener);
}

Engine::~Engine() {
  if (auto err = sess->endSession())
    sess->reportError(std::move(err));
  if (auto err = epciu->cleanup())
    sess->reportError(std::move(err));
}

llvm::Expected<std::unique_ptr<Engine>> Engine::create() {
  auto epc = llvm::orc::SelfExecutorProcessControl::Create();
  if (!epc)
    return epc.takeError();

  auto sess = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));

  auto epciu =
      llvm::orc::EPCIndirectionUtils::Create(sess->getExecutorProcessControl());
  if (!epciu)
    return epciu.takeError();

  (*epciu)->createLazyCallThroughManager(
      *sess, llvm::pointerToJITTargetAddress(&handleLazyCallThroughError));

  if (auto err = llvm::orc::setUpInProcessLCTMReentryViaEPCIU(**epciu))
    return std::move(err);

  llvm::orc::JITTargetMachineBuilder jtmb(
      sess->getExecutorProcessControl().getTargetTriple());

  auto layout = jtmb.getDefaultDataLayoutForTarget();
  if (!layout)
    return layout.takeError();

  return std::make_unique<Engine>(std::move(sess), std::move(*epciu), std::move(jtmb),
                                  std::move(*layout));
}

llvm::Error Engine::addModule(llvm::orc::ThreadSafeModule module,
                              llvm::orc::ResourceTrackerSP rt) {
  if (!rt)
    rt = mainJD.getDefaultResourceTracker();

  return optimizeLayer.add(rt, std::move(module));
}

llvm::Expected<llvm::JITEvaluatedSymbol> Engine::lookup(llvm::StringRef name) {
  return sess->lookup({&mainJD}, mangle(name.str()));
}

} // namespace jit
} // namespace codon
