#include "jit.h"

#include "codon/runtime/lib.h"

namespace codon {
namespace jit {

typedef int MainFunc(int, char **);
typedef void InputFunc();

JIT::JIT(ir::Module *module)
    : module(module), pm(std::make_unique<ir::transform::PassManager>(/*debug=*/true)),
      plm(std::make_unique<PluginManager>()),
      llvisitor(std::make_unique<ir::LLVMVisitor>(/*debug=*/true, /*jit=*/true)) {
  if (auto e = Engine::create()) {
    engine = std::move(e.get());
  } else {
    engine = {};
    seqassert(false, "JIT engine creation error");
  }
  llvisitor->setPluginManager(plm.get());
}

void JIT::init() {
  module->accept(*llvisitor);
  auto pair = llvisitor->takeModule();
  // auto rt = engine->getMainJITDylib().createResourceTracker();
  llvm::cantFail(engine->addModule({std::move(pair.second), std::move(pair.first)}));
  auto func = llvm::cantFail(engine->lookup("main"));
  auto *main = (MainFunc *)func.getAddress();
  (*main)(0, nullptr);
  // llvm::cantFail(rt->remove());
}

void JIT::run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals) {
  const std::string name = ir::LLVMVisitor::getNameForFunction(input);
  llvisitor->registerGlobal(input);
  for (auto *var : newGlobals) {
    llvisitor->registerGlobal(var);
  }
  for (auto *var : newGlobals) {
    if (auto *func = ir::cast<ir::Func>(var))
      func->accept(*llvisitor);
  }
  input->accept(*llvisitor);
  auto pair = llvisitor->takeModule();
  // auto rt = engine->getMainJITDylib().createResourceTracker();
  llvm::StripDebugInfo(*pair.second); // TODO: needed?
  llvm::cantFail(engine->addModule({std::move(pair.second), std::move(pair.first)}));
  auto func = llvm::cantFail(engine->lookup(name));
  auto *repl = (InputFunc *)func.getAddress();
  try {
    (*repl)();
  } catch (const seq_jit_error &) {
    // nothing to do
  }
  // llvm::cantFail(rt->remove());
}

} // namespace jit
} // namespace codon
