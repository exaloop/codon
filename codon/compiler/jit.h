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
  bool is_error;

  JITResult():
    data(""), is_error(false) {}

  JITResult(const std::string &data, bool is_error):
    data(data), is_error(is_error) {}

  operator bool() {
    return !this->is_error;
  }

  static JITResult success(const std::string &output) {
    return JITResult(output, false);
  }

  static JITResult error(const std::string &error_info) {
    return JITResult(error_info, true);
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
  JITResult execute_safe(const std::string &code);
};

} // namespace jit
} // namespace codon
