#pragma once

#include <memory>
#include <string>
#include <vector>

#include "codon/compiler/compiler.h"
#include "codon/compiler/engine.h"
#include "codon/parser/cache.h"
#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/var.h"

namespace codon {
namespace jit {

class Error {
public:
  enum Code {
    SUCCESS = 0,
    PARSER_ERROR,
    LLVM_ERROR,
    RUNTIME_ERROR,
  };

private:
  Code code;
  std::string message;
  std::string type;
  SrcInfo src;

public:
  explicit Error(Code code = Code::SUCCESS, const std::string &message = "",
                 const std::string &type = "", const SrcInfo &src = {})
      : code(code), message(message), type(type), src(src) {}

  operator bool() const { return code != Code::SUCCESS; }

  Code getCode() const { return code; }
  std::string getType() const { return type; }
  std::string getMessage() const { return message; }
  SrcInfo getSrcInfo() const { return src; }

  static const Error NONE;
};

class JIT {
private:
  std::unique_ptr<Compiler> compiler;
  std::unique_ptr<Engine> engine;

public:
  explicit JIT(const std::string &argv0);
  Error init();
  Error run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals = {});
  Error exec(const std::string &code);
};

} // namespace jit
} // namespace codon
