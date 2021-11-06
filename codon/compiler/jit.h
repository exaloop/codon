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

class JIT {
private:
  std::unique_ptr<Compiler> compiler;
  std::unique_ptr<Engine> engine;

public:
  explicit JIT(const std::string &argv0);
  llvm::Error init();
  llvm::Expected<std::string> run(const ir::Func *input,
                                  const std::vector<ir::Var *> &newGlobals = {});
  llvm::Expected<std::string> exec(const std::string &code);

private:
  std::pair<ir::Func *, std::vector<ir::Var *>>
  transformSimplified(const ast::StmtPtr &simplified);
};

} // namespace jit
} // namespace codon
