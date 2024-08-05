// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <vector>

#include "codon/cir/llvm/llvm.h"
#include "codon/compiler/debug_listener.h"

namespace codon {
namespace jit {

class Engine {
private:
  std::unique_ptr<llvm::orc::LLJIT> jit;
  DebugPlugin *debug;

public:
  Engine();

  const llvm::DataLayout &getDataLayout() const { return jit->getDataLayout(); }

  llvm::orc::JITDylib &getMainJITDylib() { return jit->getMainJITDylib(); }

  DebugPlugin *getDebugListener() const { return debug; }

  llvm::Error addModule(llvm::orc::ThreadSafeModule module,
                        llvm::orc::ResourceTrackerSP rt = nullptr);

  llvm::Expected<llvm::orc::ExecutorAddr> lookup(llvm::StringRef name);
};

} // namespace jit
} // namespace codon
