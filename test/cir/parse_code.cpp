#include "test.h"

#include "codon/cir/transform/lowering/pipeline.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/jit.h"
#include "codon/compiler/options.h"

using namespace codon;

namespace {
ir::SeriesFlow *mainBody(ir::Module *module) {
  return ir::cast<ir::SeriesFlow>(
      ir::cast<ir::BodiedFunc>(module->getMainFunc())->getBody());
}

// Exercise the plugin API during the pass pipeline, including new module globals
// and helper realization. The second parse must run after the first, before users.
class ParseImportPass : public ir::transform::Pass {
public:
  std::string getKey() const override { return "test-parse-import"; }
  void run(ir::Module *module) override {
    module->parseCode(R"(
import string
class ParsedBase:
    def value(self) -> int: return 0
class ParsedChild(ParsedBase):
    def value(self) -> int: return len(string.ascii_letters)
def __pc_dispatch(value: ParsedBase) -> int:
    return value.value()
def __pc_import(x: int) -> int:
    return len(string.ascii_letters) + x
print(__pc_dispatch(ParsedChild()))
print(string.ascii_lowercase)
)");
    ASSERT_TRUE(module->getOrRealizeFunc("__pc_import", {module->getIntType()}));
    module->parseCode("import string\nprint(__pc_import(7))\n");
  }
};
} // namespace

TEST(ParseCodeTest, DefinitionsAndNewImport) {
  auto options = Options::getDefault("build/codon_test");
  Compiler compiler(*options);
  llvm::cantFail(compiler.parseCode("parse_code_test.codon", "print(42)\n"));
  auto *module = compiler.getModule();
  auto *body = mainBody(module);
  auto before = std::distance(body->begin(), body->end());
  auto ctx = compiler.getCache()->codegenCtx;
  auto series = ctx->series;
  auto bases = ctx->bases;

  // Definitions alone must not insert executable code into main.
  module->parseCode("def __pc_plain(x: int) -> int:\n    return x + 1\n");
  ASSERT_TRUE(module->getOrRealizeFunc("__pc_plain", {module->getIntType()}));
  EXPECT_EQ(before, std::distance(body->begin(), body->end()));

  // A first import emits executable initialization, even for definitions-only
  // modules such as bisect. This used to emit into an empty statement stack.
  module->parseCode("import bisect\n"
                    "def __pc_bisect(x: int) -> int:\n"
                    "    return bisect.bisect_left([1, 5, 9], x)\n");
  ASSERT_TRUE(module->getOrRealizeFunc("__pc_bisect", {module->getIntType()}));
  EXPECT_GT(std::distance(body->begin(), body->end()), before);
  EXPECT_EQ(ctx->series, series);
  EXPECT_EQ(ctx->bases, bases);
}

TEST(ParseCodeTest, ImportsFromIRPassRunBeforeUserCode) {
  for (bool debug : {false, true}) {
    SCOPED_TRACE(debug);
    ASSERT_EXIT(
        {
          auto options = Options::getDefault("build/codon_test");
          options->debug = debug;
          options->capture = true;
          Compiler compiler(*options);
          llvm::cantFail(compiler.parseCode("parse_code_test.codon", "print(42)\n"));
          compiler.getPassManager()->registerPass(
              std::make_unique<ParseImportPass>(),
              ir::transform::lowering::PipelineLowering::KEY);
          llvm::cantFail(compiler.compile());
          compiler.getLLVMVisitor()->run();
          EXPECT_EQ(runtime::getCapturedOutput(),
                    "52\nabcdefghijklmnopqrstuvwxyz\n59\n42\n");
          std::exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
  }
}

TEST(ParseCodeTest, ImportsInitializeInCurrentJITCell) {
  for (bool debug : {false, true}) {
    SCOPED_TRACE(debug);
    ASSERT_EXIT(
        {
          auto options = Options::getDefault("build/codon_test");
          options->jit = true;
          options->debug = debug;
          options->capture = true;
          jit::JIT instance(*options);
          llvm::cantFail(instance.init());
          auto *module = instance.getCompiler()->getModule();
          // Pending functions follow user code in the JIT AST: treating the last
          // AST block as user code would place late initialization after print.
          auto *cell = llvm::cantFail(instance.compile(
              "def user_code(x: int):\n    return x * 2\nprint(user_code(21))\n"));
          ParseImportPass().run(module);
          EXPECT_EQ(llvm::cantFail(instance.run(cell)),
                    "52\nabcdefghijklmnopqrstuvwxyz\n59\n42\n");

          cell = llvm::cantFail(instance.compile("print(43)\n"));
          module->parseCode("import string\nprint(string.ascii_uppercase)\n");
          EXPECT_EQ(llvm::cantFail(instance.run(cell)),
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n43\n");
          std::exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
  }
}
