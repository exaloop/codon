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
  llvm::cantFail(compiler.parseCode(
      "format_optimization_test.codon",
      "def format_optimization_probe(value: int):\n"
      "    return 'abc{:04d}xyz'.format(value) + value.__format__('04d')\n"
      "class FormatOptimizationValue:\n"
      "    value: int\n"
      "    def __init__(self, value: int):\n"
      "        self.value = value\n"
      "def format_optimization_compound(value: FormatOptimizationValue, "
      "items: List[int], mapping: Dict[str, str]):\n"
      "    return '{0.value}:{1[0]}:{2[key]}'.format(value, items, mapping)\n"
      "def format_optimization_bad_member(value: FormatOptimizationValue):\n"
      "    return '{0.value}:{0.missing}'.format(value)\n"
      "def format_optimization_bad_element(value: int):\n"
      "    return '{0[0]}'.format(value)\n"
      "def format_optimization_dynamic(format: str, value: int):\n"
      "    return format.format(value)\n"
      "def format_optimization_malformed(value: int):\n"
      "    return '{'.format(value)\n"
      "format_optimization_probe(42)\n"
      "format_optimization_compound(FormatOptimizationValue(42), [7], "
      "{'key': 'value'})\n"
      "format_optimization_bad_member(FormatOptimizationValue(42))\n"
      "format_optimization_bad_element(42)\n"
      "format_optimization_dynamic('{}', 42)\n"
      "format_optimization_malformed(42)\n"));
  llvm::cantFail(compiler.compile());

  FormattingCallCounter counter;
  counter.process(compiler.getModule());
  EXPECT_EQ(5, counter.parsed);
  EXPECT_EQ(4, counter.dynamic);
}
