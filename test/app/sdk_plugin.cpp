// Build this against installed headers/import libraries, never compiler sources.
#include "codon/dsl/dsl.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/cache.h"

class SDKPass : public codon::ir::transform::OperatorPass {
  std::string getKey() const override { return "sdk-rtti-smoke"; }
  void handle(codon::ir::CallInstr *call) override {
    using namespace codon::ir;
    auto *target = cast<BodiedFunc>(util::getFunc(call->getCallee()));
    if (isA<BodiedFunc>(getParentFunc()) && target &&
        !Module::ADD_MAGIC_NAME.empty() &&
        !codon::ast::StdlibTypes::Int.empty() &&
        !codon::ast::StdlibTypes::String.empty() &&
        !codon::ast::StdlibTypes::Function.empty() &&
        !codon::ast::VAR_ARGV.empty() && !codon::ast::VAR_ARGC.empty() &&
        !codon::ast::FN_OPTIONAL_UNWRAP.empty() &&
        target->getUnmangledName() == "sdk_probe")
      call->replaceAll(call->getModule()->getInt(73));
  }
};

class SDKPlugin : public codon::DSL {
  void addIRPasses(codon::ir::transform::PassManager *manager, bool) override {
    manager->registerPass(std::make_unique<SDKPass>());
  }
};

extern "C" __declspec(dllexport) std::unique_ptr<codon::DSL> load() {
  return std::make_unique<SDKPlugin>();
}
