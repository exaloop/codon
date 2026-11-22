#include "test.h"

#include "codon/cir/util/irtools.h"
#include "codon/cir/util/operator.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/options.h"

using namespace codon;

namespace {
class FormattingCallCounter : public ir::util::Operator {
public:
  int parsed = 0;
  int dynamic = 0;

  void handle(ir::CallInstr *call) override {
    if (getParentFunc()->getUnmangledName().find("format_optimization_") != 0)
      return;

    auto *func = ir::util::getFunc(call->getCallee());
    if (!func)
      return;

    if (func->getUnmangledName() == "_format_parsed")
      ++parsed;
    else if (func->getUnmangledName() == "format" ||
             func->getUnmangledName() == "__format__")
      ++dynamic;
  }
};
} // namespace

TEST(FormattingOptimizationTest, LowersLiteralFormattingCalls) {
  auto options = Options::getDefault("build/codon_test");
  options->debug = false;
  options->native = false;

  Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("format_optimization_test.codon",
                                    readCIRSource("format_optimization_test.codon")));
  llvm::cantFail(compiler.compile());

  FormattingCallCounter counter;
  counter.process(compiler.getModule());
  EXPECT_EQ(5, counter.parsed);
  EXPECT_EQ(6, counter.dynamic);
}
