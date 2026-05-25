// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "codon/cir/llvm/llvm.h"
#include "codon/compiler/debug_listener.h"

namespace codon {
namespace jit {

class Engine {
private:
  constexpr static const char *kLibgcc_sName = "libgcc_s.so.1";
  constexpr static const char *kLibstdcxxName = "libstdc++.so.6";
  constexpr static const char *kRuntimeInitFnName = "__codon_jit_runtime_init";

  using RuntimeSymbolMap = std::map<std::string, void *>;
  using RuntimeAddSymbolFunc = void (*)(void *, const char *, void *);
  using RuntimeInitFunc = void (*)(RuntimeAddSymbolFunc, void *);

  std::unique_ptr<llvm::orc::LLJIT> jit;
  DebugPlugin *debug;
  char globalPrefix;
  std::vector<void *> runtimeHandles;

  /// Register symbols with this Engine.
  llvm::Error registerSymbols(
      llvm::function_ref<llvm::orc::SymbolMap(llvm::orc::MangleAndInterner)> symbolMap);

  /// Best-effort dynamic library search generator registration.
  void tryAddDynamicLibrarySearchGenerator(const char *path);

public:
  Engine();
  ~Engine();

  const llvm::DataLayout &getDataLayout() const { return jit->getDataLayout(); }

  llvm::orc::JITDylib &getMainJITDylib() { return jit->getMainJITDylib(); }

  DebugPlugin *getDebugListener() const { return debug; }

  llvm::Error addModule(llvm::orc::ThreadSafeModule module,
                        llvm::orc::ResourceTrackerSP rt = nullptr);

  llvm::Expected<llvm::orc::ExecutorAddr> lookup(llvm::StringRef name);

  /// Load the Codon runtime locally and register its JIT symbol map with ORC.
  /// @param path Path to the Codon runtime library file (.so/.dll/.dylib)
  /// @return llvm::Error::success() on success, error code on failure
  llvm::Error addRuntimeSymbolMap(const std::string &path);
};

} // namespace jit
} // namespace codon
