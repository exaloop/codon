#pragma once

#include <memory>
#include <string>
#include <vector>

#include "codon/jit/engine.h"
#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/var.h"

namespace codon {
namespace jit {

class Status {
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
  explicit Status(Code code = Code::SUCCESS, const std::string &message = "",
                  const std::string &type = "", const SrcInfo &src = {})
      : code(code), message(message), type(type), src(src) {}

  operator bool() const { return code == Code::SUCCESS; }

  Code getCode() const { return code; }
  std::string getType() const { return type; }
  std::string getMessage() const { return message; }
  SrcInfo getSrcInfo() const { return src; }

  static const Status OK;
};

class JIT {
private:
  ir::Module *module;
  std::unique_ptr<ir::transform::PassManager> pm;
  std::unique_ptr<PluginManager> plm;
  std::unique_ptr<ir::LLVMVisitor> llvisitor;
  std::unique_ptr<Engine> engine;

public:
  JIT(ir::Module *module);
  ir::Module *getModule() const { return module; }
  Status init();
  Status run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals = {});
};

} // namespace jit
} // namespace codon
