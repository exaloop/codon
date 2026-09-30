// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <gc.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "codon/cir/transform/manager.h"
#include "codon/compiler/compiler.h"
#include "codon/compiler/jit.h"
#include "codon/compiler/jit_extern.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/util/common.h"
#include "gtest/gtest.h"

TEST(TypeCoreTest, NewFunctionRealizationIsIncomplete) {
  auto realization =
      std::make_shared<codon::ast::Cache::Function::FunctionRealization>();
  EXPECT_EQ(realization->type, nullptr);
  EXPECT_EQ(realization->ast, nullptr);
  EXPECT_EQ(realization->ir, nullptr);
  EXPECT_TRUE(realization->captures.empty());
}

TEST(PassManagerTest, HonorsDisabledPassesAtRegistration) {
  auto options = codon::Options::getDefault("build/codon_test");
  options->debug = false;
  codon::ir::transform::PassManager enabled(options.get());
  EXPECT_TRUE(enabled.hasPass("core-numpy-fusion"));
  EXPECT_TRUE(enabled.hasPass("core-numpy-lifetime"));
  EXPECT_TRUE(enabled.hasPass("core-numpy-inline"));
  EXPECT_FALSE(enabled.isDisabled("core-numpy-fusion"));

  options->disabled = {"core-numpy-fusion", "core-numpy-lifetime", "not-a-pass"};
  EXPECT_TRUE(enabled.isDisabled("core-numpy-fusion"));
  EXPECT_TRUE(enabled.hasPass("core-numpy-fusion"));
  codon::ir::transform::PassManager disabled(options.get());
  EXPECT_FALSE(disabled.hasPass("core-numpy-fusion"));
  EXPECT_FALSE(disabled.hasPass("core-numpy-lifetime"));
  EXPECT_TRUE(disabled.hasPass("core-numpy-inline"));
  EXPECT_TRUE(disabled.isDisabled("not-a-pass"));

  options->disabled.clear();
  EXPECT_FALSE(disabled.isDisabled("core-numpy-fusion"));
  EXPECT_FALSE(disabled.hasPass("core-numpy-fusion"));
}

TEST(JITOptionsTest, RejectsInvalidOptions) {
  for (const auto *settings :
       {"[]", "{", R"({"fastmath": 1})", R"({"native": null})", R"({"mcpu": true})",
        R"({"disabled": "folding"})", R"({"mattrs": [1]})", R"({"jit": false})",
        R"({"standalone": true})", R"({"unknown": true})",
        R"({"defines": ["missing-value"]})", R"({"defines": ["name=1", "name=2"]})"}) {
    auto result = jit_validate_options(settings);
    EXPECT_EQ(result.result, nullptr);
    EXPECT_NE(result.error, nullptr) << settings;
    free(result.error);
  }
}

TEST(JITOptionsTest, AppliesOptionsBeforeInitialization) {
  auto result =
      jit_init_with_options("build/codon_test",
                            R"({"pynum": false, "fastmath": true, "capture": true,
           "native": false, "gpuName": "sm_80", "mattrs": [],
         "defines": ["JIT_SCALE=7", "JIT_LABEL=str:configured"],
           "disabled": ["not-a-pass"]})");
  ASSERT_EQ(result.error, nullptr) << (result.error ? result.error : "");
  std::unique_ptr<codon::jit::JIT> jit(static_cast<codon::jit::JIT *>(result.result));
  auto *options = jit->getCompiler()->getOptions();
  EXPECT_TRUE(options->jit);
  EXPECT_FALSE(options->debug);
  EXPECT_FALSE(options->pynum);
  EXPECT_TRUE(options->fastmath);
  EXPECT_FALSE(options->native);
  EXPECT_EQ(options->gpuName, "sm_80");
  EXPECT_EQ(options->disabled, std::vector<std::string>{"not-a-pass"});
  auto executed = jit->execute("print(__py_numerics__)\nprint(-3 // 2)\n"
                               "print(JIT_SCALE)\nprint(JIT_LABEL)\n");
  ASSERT_TRUE(bool(executed)) << llvm::toString(executed.takeError());
  EXPECT_EQ(*executed, "0\n-1\n7\nconfigured\n");
}

TEST(JITOptionsTest, ReportsPluginInitializationErrors) {
  auto result = jit_init_with_options(
      "build/codon_test", R"({"plugins": ["/__codon_missing_jit_plugin__"]})");
  EXPECT_EQ(result.result, nullptr);
  ASSERT_NE(result.error, nullptr);
  EXPECT_NE(std::string(result.error).find("plugin"), std::string::npos);
  free(result.error);
}

TEST(JITOptionsTest, CollectsAfterJITTeardown) {
  ASSERT_EXIT(
      {
        auto options = codon::Options::getDefault("build/codon_test");
        options->capture = true;
        {
          codon::jit::JIT jit(*options);
          auto error = jit.init();
          ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
          auto result = jit.execute("values = [str(index) for index in range(100)]\n"
                                    "print(values[-1])\n");
          ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
          EXPECT_EQ(*result, "99\n");
        }
        GC_gcollect();
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}
