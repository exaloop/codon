#include "jit.h"

#include "codon/runtime/lib.h"

namespace codon {
namespace jit {
namespace {
typedef int MainFunc(int, char **);
typedef void InputFunc();

Status statusFromError(llvm::Error err) {
  return Status(Status::Code::LLVM_ERROR, llvm::toString(std::move(err)));
}
} // namespace

const Status Status::OK = Status(Status::Code::SUCCESS);

JIT::JIT(const std::string &argv0)
    : cache(std::make_shared<ast::Cache>(argv0)), module(nullptr),
      pm(std::make_unique<ir::transform::PassManager>(/*debug=*/true)),
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

Status JIT::init() {
  module->accept(*llvisitor);
  auto pair = llvisitor->takeModule();

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return statusFromError(std::move(err));

  auto func = engine->lookup("main");
  if (auto err = func.takeError())
    return statusFromError(std::move(err));

  auto *main = (MainFunc *)func->getAddress();
  (*main)(0, nullptr);
  return Status::OK;
}

Status JIT::run(const ir::Func *input, const std::vector<ir::Var *> &newGlobals) {
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
  llvm::StripDebugInfo(*pair.first); // TODO: needed?

  if (auto err = engine->addModule({std::move(pair.first), std::move(pair.second)}))
    return statusFromError(std::move(err));

  auto func = engine->lookup(name);
  if (auto err = func.takeError())
    return statusFromError(std::move(err));

  auto *repl = (InputFunc *)func->getAddress();
  try {
    (*repl)();
  } catch (const seq_jit_error &err) {
    return Status(Status::Code::RUNTIME_ERROR, err.what(), err.getType(),
                  SrcInfo(err.getFile(), err.getLine(), err.getCol(), /*len=*/0));
  }
  return Status::OK;
}

} // namespace jit
} // namespace codon
