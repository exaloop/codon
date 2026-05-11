// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "codon/cir/llvm/llvm.h"
#include "codon/compiler/debug_listener.h"

namespace codon {
namespace jit {

class Engine {
private:
  std::unique_ptr<llvm::orc::LLJIT> jit;
  DebugPlugin *debug;
  char globalPrefix;

public:
  Engine();

  const llvm::DataLayout &getDataLayout() const { return jit->getDataLayout(); }

  llvm::orc::JITDylib &getMainJITDylib() { return jit->getMainJITDylib(); }

  DebugPlugin *getDebugListener() const { return debug; }

  llvm::Error addModule(llvm::orc::ThreadSafeModule module,
                        llvm::orc::ResourceTrackerSP rt = nullptr);

  llvm::Expected<llvm::orc::ExecutorAddr> lookup(llvm::StringRef name);

  /// Load a dynamic library and register it with the JIT for symbol resolution.
  /// @param path Path to the dynamic library file (.so/.dll/.dylib)
  /// @return llvm::Error::success() on success, error code on failure
  llvm::Error addDynamicLibrary(const std::string &path);
};

} // namespace jit
} // namespace codon
