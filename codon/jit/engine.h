#pragma once

#include <memory>
#include <vector>

#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/transform/manager.h"
#include "codon/sir/var.h"

namespace codon {
namespace jit {

class Engine;

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
  void init();
  void run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals = {});
};

} // namespace jit
} // namespace codon
