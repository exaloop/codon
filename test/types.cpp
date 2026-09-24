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

namespace {
const std::string recursiveVirtualCode = R"codon(
import internal.static as static

class Root:
	def value(self):
		return 0

class Leaf(Root):
	cached: int
	def __init__(self):
		self.cached = self.value()
	def value(self):
		return build_value(42, False)

def build_value[T](item: T, recurse: bool) -> T:
	if isinstance(item, int):
		if recurse:
			return Leaf().cached
	return item

method = static.function.realized(Leaf.value, type._force_cast(cobj(), Leaf))
leaf = Leaf()
base: Root = leaf
assert method(leaf) == 42 and base.value() == 42
assert build_value(42, True) == 42 and build_value(1.25, True) == 1.25

@__force__
def forced_entry(value: Leaf):
	return value.value()
)codon";

void expectCompleteRealizations(codon::ast::Cache *cache) {
  for (const auto &[name, function] : cache->functions) {
    for (const auto &[key, realization] : function.realizations) {
      SCOPED_TRACE(name + ": " + key);
      ASSERT_NE(realization, nullptr);
      ASSERT_NE(realization->type, nullptr);
      EXPECT_NE(realization->ast, nullptr);
      EXPECT_NE(realization->ir, nullptr);
      EXPECT_TRUE(realization->getType()->getRetType()->canRealize());
      if (realization->ast)
        EXPECT_EQ(realization->ast->getName(), realization->type->realizedName());
    }
  }
  for (const auto &[name, imported] : cache->imports) {
    if (!imported.ctx)
      continue;
    SCOPED_TRACE(name);
    EXPECT_EQ(imported.ctx->getRealizationDepth(), 1);
    for (const auto &base : imported.ctx->bases)
      for (const auto &dependency : base.recursiveDependencies)
        EXPECT_TRUE(dependency->getFunc()->getRetType()->canRealize());
  }
}
} // namespace

TEST(TypeCoreTest, RecursiveRealizationsRemainCompleteWhenForced) {
  auto options = codon::Options::getDefault("build/codon_test");
  options->pyext = true;
  codon::Compiler compiler(*options);
  auto *cache = compiler.getCache();
  auto parsed =
      codon::ast::parseCode(cache, "recursive_realization.codon", recursiveVirtualCode);
  ASSERT_TRUE(bool(parsed)) << llvm::toString(parsed.takeError());
  auto *typechecked = codon::ast::TypecheckVisitor::apply(
      cache, *parsed, "recursive_realization.codon", {}, compiler.getEarlyDefines());
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));

  auto name = codon::ast::getMangledFunc(MAIN_IMPORT, "forced_entry");
  auto found = cache->functions.find(name);
  ASSERT_NE(found, cache->functions.end());
  ASSERT_EQ(found->second.realizations.size(), 1);
  auto realization = found->second.realizations.begin()->second;
  auto *originalIR = realization->ir;

  auto thunkName =
      codon::ast::getMangledMethod("std.internal.core", "RTTIType", "_get_thunk_id");
  auto thunks = cache->functions.at(thunkName).realizations;
  ASSERT_FALSE(thunks.empty());
  for (auto &[name, function] : cache->functions)
    function.isToplevel = false;
  codon::ast::TranslateVisitor::apply(cache, typechecked);
  for (const auto &[key, previous] : thunks) {
    auto current = cache->functions.at(thunkName).realizations.at(key);
    EXPECT_NE(current, previous);
    EXPECT_EQ(current->ir, previous->ir);
    EXPECT_NE(current->ast, nullptr);
  }
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  EXPECT_TRUE(cache->pendingRealizations.empty());

  const auto &specializations =
      cache->functions.at(codon::ast::getMangledFunc(MAIN_IMPORT, "build_value"))
          .realizations;
  ASSERT_EQ(specializations.size(), 2);
  auto specialization = specializations.begin();
  auto firstType = specialization++->second;
  auto secondType = specialization->second;
  EXPECT_NE(firstType->type, secondType->type);
  EXPECT_NE(firstType->ir, secondType->ir);
  EXPECT_NE(firstType->type->getRetType()->realizedName(),
            secondType->type->getRetType()->realizedName());

  codon::ast::TypecheckVisitor visitor(cache->typeCtx);
  for (int attempt = 0; attempt < 2; ++attempt) {
    auto *type = visitor.realize(realization->getType());
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type, realization->getType());
    realization = cache->functions.at(name).realizations.at(type->realizedName());
    EXPECT_EQ(realization->ir, originalIR);
    ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  }
}

TEST(TypeCoreTest, RecursiveRealizationsSurviveJITRollback) {
  auto options = codon::Options::getDefault("build/codon_test");
  options->capture = true;
  codon::jit::JIT jit(*options);
  auto error = jit.init();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  auto *cache = jit.getCompiler()->getCache();
  auto first = jit.execute(R"codon(
def recursive_first[T](depth: int, item: T) -> T:
	if depth:
		return recursive_second(depth - 1, item)
	return item

def recursive_second(depth: int, item):
	return recursive_first(depth, item)

print(recursive_first(2, 42))
)codon");
  ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
  EXPECT_EQ(*first, "42\n");
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  EXPECT_TRUE(cache->pendingRealizations.empty());

  auto virtualCell = jit.compile(recursiveVirtualCode);
  ASSERT_TRUE(bool(virtualCell)) << llvm::toString(virtualCell.takeError());
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  EXPECT_TRUE(cache->pendingRealizations.empty());

  auto before = cache->functions;
  auto cell = cache->jitCell;
  auto failed = jit.compile(R"codon(
class BrokenRoot:
	def value(self):
		return 0

class BrokenLeaf(BrokenRoot):
	def value(self):
		return unconstrained(self)

def unconstrained(item: BrokenLeaf):
	return item.value()

static.function.realized(BrokenLeaf.value, type._force_cast(cobj(), BrokenLeaf))
)codon");
  ASSERT_FALSE(bool(failed));
  auto message = llvm::toString(failed.takeError());
  EXPECT_NE(message.find("cannot typecheck"), std::string::npos) << message;
  EXPECT_EQ(cache->jitCell, cell);
  ASSERT_EQ(cache->functions.size(), before.size());
  for (const auto &[name, function] : before) {
    const auto &current = cache->functions.at(name);
    EXPECT_EQ(current.realizations, function.realizations) << name;
  }
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  EXPECT_TRUE(cache->pendingRealizations.empty());

  auto next = jit.execute("print(recursive_first(3, 42))\n"
                          "print(recursive_first(2, 1.25))\n");
  ASSERT_TRUE(bool(next)) << llvm::toString(next.takeError());
  EXPECT_EQ(*next, "42\n1.25\n");
  EXPECT_EQ(cache->jitCell, cell + 1);
  ASSERT_NO_FATAL_FAILURE(expectCompleteRealizations(cache));
  EXPECT_TRUE(cache->pendingRealizations.empty());
}
