#pragma once

#include <memory>
#include <string>
#include <vector>

#include "codon/compiler/compiler.h"
#include "codon/compiler/engine.h"
#include "codon/compiler/error.h"
#include "codon/parser/cache.h"
#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/var.h"

namespace codon {
namespace jit {

struct JITResult {
  std::string data;
  bool isError;

  JITResult():
    data(""), isError(false) {}

  JITResult(const std::string &data, bool isError):
    data(data), isError(isError) {}

  operator bool() {
    return !this->isError;
  }

  static JITResult success(const std::string &output) {
    return JITResult(output, false);
  }

  static JITResult error(const std::string &errorInfo) {
    return JITResult(errorInfo, true);
  }
};

class JIT {
private:
  std::unique_ptr<Compiler> compiler;
  std::unique_ptr<Engine> engine;
  std::string mode;

public:
  explicit JIT(const std::string &argv0, const std::string &mode = "");

  Compiler *getCompiler() const { return compiler.get(); }
  Engine *getEngine() const { return engine.get(); }

  llvm::Error init();
  llvm::Expected<std::string> run(const ir::Func *input);
  llvm::Expected<std::string> execute(const std::string &code);
  JITResult executeSafe(const std::string &code);
};

} // namespace jit
} // namespace codon
