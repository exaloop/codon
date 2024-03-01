// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/transform/manager.h"
#include "codon/cir/var.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/engine.h"
#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/runtime/lib.h"

#include "codon/compiler/jit_extern.h"

namespace codon {
namespace jit {

class JIT {
public:
  struct PythonData {
    ir::types::Type *cobj;
    std::unordered_map<std::string, ir::Func *> cache;

    PythonData();
    ir::types::Type *getCObjType(ir::Module *M);
  };

private:
  std::unique_ptr<Compiler> compiler;
  std::unique_ptr<Engine> engine;
  std::unique_ptr<PythonData> pydata;
  std::string mode;

public:
  explicit JIT(const std::string &argv0, const std::string &mode = "");

  Compiler *getCompiler() const { return compiler.get(); }
  Engine *getEngine() const { return engine.get(); }

  // General
  llvm::Error init();
  llvm::Error compile(const ir::Func *input);
  llvm::Expected<ir::Func *> compile(const std::string &code,
                                     const std::string &file = "", int line = 0);
  llvm::Expected<void *> address(const ir::Func *input);
  llvm::Expected<std::string> run(const ir::Func *input);
  llvm::Expected<std::string> execute(const std::string &code,
                                      const std::string &file = "", int line = 0,
                                      bool debug = false);

  // Python
  llvm::Expected<void *> runPythonWrapper(const ir::Func *wrapper, void *arg);
  llvm::Expected<ir::Func *> getWrapperFunc(const std::string &name,
                                            const std::vector<std::string> &types);
  JITResult executePython(const std::string &name,
                          const std::vector<std::string> &types,
                          const std::string &pyModule,
                          const std::vector<std::string> &pyVars, void *arg,
                          bool debug);
  JITResult executeSafe(const std::string &code, const std::string &file, int line,
                        bool debug);

  // Errors
  llvm::Error handleJITError(const runtime::JITError &e);
};

} // namespace jit
} // namespace codon
