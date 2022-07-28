#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/compiler/compiler.h"
#include "codon/compiler/engine.h"
#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/runtime/lib.h"
#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/var.h"

namespace codon {
namespace jit {

struct JITResult {
  void *result;
  std::string message;

  operator bool() const { return message.empty(); }

  static JITResult success(void *result) { return {result, ""}; }

  static JITResult error(const std::string &message) { return {nullptr, message}; }
};

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
  llvm::Expected<void *> getRawFunction(const ir::Func *input);
  llvm::Expected<std::string> run(const ir::Func *input);
  llvm::Expected<std::string> execute(const std::string &code);

  // Python
  llvm::Expected<void *> runPythonWrapper(const ir::Func *wrapper, void *arg);
  llvm::Expected<ir::Func *> getWrapperFunc(const std::string &name,
                                            const std::vector<std::string> &types);
  JITResult executePython(const std::string &name,
                          const std::vector<std::string> &types, void *arg);
  JITResult executeSafe(const std::string &code);

  // Errors
  llvm::Error handleJITError(const JITError &e);
};

} // namespace jit
} // namespace codon
