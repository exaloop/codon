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

TEST(JITRTTITest, RefreshesDescriptorsAndLookupAcrossCells) {
  for (bool debug : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->jit = true;
    options->debug = debug;
    options->capture = true;
    codon::jit::JIT instance(*options);
    auto error = instance.init();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
    auto first =
        instance.execute("class CellPlain:\n"
                         "    value: int\n"
                         "plain = CellPlain(41)\n"
                         "assert not TypeInfo.cache(CellPlain).rtti\n"
                         "assert type._ref_size(CellPlain) == int.__elemsize__\n"
                         "def increment_plain(value: CellPlain):\n"
                         "    value.value += 1\n"
                         "    return value.value\n"
                         "assert increment_plain(plain) == 42\n"
                         R"(
class CellBase:
    def method(self): return 1
class CellChild(CellBase):
    def method(self): return 42
)"
                         "def dispatch_old(value: CellBase):\n"
                         "    return value.method()\n"
                         R"(
old_object = CellChild()
base: CellBase = old_object
assert dispatch_old(base) == 42
old_info = TypeInfo.cache(CellChild)
def lookup_id(type_id: int):
    return TypeInfo.cache(type_id).id
assert lookup_id(CellChild.__id__) == CellChild.__id__
print("first")
)");
    ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
    EXPECT_EQ(*first, "first\n");
    auto second =
        instance.execute("assert not TypeInfo.cache(CellPlain).rtti\n"
                         "assert type._ref_size(CellPlain) == int.__elemsize__\n"
                         "assert __ptr__(plain.value) == "
                         "type._force_cast(__magic__.raw(plain), Ptr[int])\n"
                         "assert increment_plain(plain) == 43\n"
                         R"(
assert TypeInfo.cache(CellChild).__raw__() == old_info.__raw__()
assert get_typeinfo(base).__raw__() == old_info.__raw__()
assert base.method() == 42
class CellLater(CellBase):
    def method(self): return 43
later: CellBase = CellLater()
assert later.method() == 43
assert dispatch_old(later) == 43
assert dispatch_old(base) == 42
assert lookup_id(CellLater.__id__) == CellLater.__id__
assert base.method() == 42
print("second")
)");
    ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
    EXPECT_EQ(*second, "second\n");
    auto rejected = instance.execute("class CellBadChild(CellPlain):\n    pass\n");
    ASSERT_FALSE(bool(rejected));
    EXPECT_NE(llvm::toString(rejected.takeError())
                  .find("cannot inherit from a non-RTTI class"),
              std::string::npos);
  }
}

TEST(JITRTTITest, PreservesLayoutsWhileRefreshingMethods) {
  for (bool debug : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->jit = true;
    options->debug = debug;
    options->capture = true;
    codon::jit::JIT instance(*options);
    auto error = instance.init();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
    auto first = instance.execute(R"(
class LayoutBase(object):
  balance: int
  accrued: int
  def method(self, *values, **options) -> int:
    return len(values) + options['extra']
class LayoutChild(LayoutBase):
  def method(self, *values, **options) -> int:
    return 100 + len(values) + options['extra']
@noinline
def update_old(value: LayoutBase, delta: int):
  value.balance += delta
  value.accrued += value.balance
  return value.balance + value.accrued
@noinline
def dispatch_old(value: LayoutBase):
  return value.method(1, extra=2)
old: LayoutBase = LayoutChild(10, 1)
assert update_old(old, 2) == 25
assert dispatch_old(old) == 103
old_info = TypeInfo.cache(LayoutChild)
old_mro = old_info.mro
old_offsets = old_info.offsets
old_methods = old_info.methods
print('first')
)");
    ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
    EXPECT_EQ(*first, "first\n");

    // A new variadic signature refreshes an already published method table.
    // Old callers must retain valid layout loads but see newly installed thunks.
    auto second = instance.execute(R"(
@noinline
def dispatch_new(value: LayoutBase):
  return value.method(1, 'two', extra=3)
assert dispatch_new(old) == 105
assert dispatch_old(old) == 103
assert TypeInfo.cache(LayoutChild).__raw__() == old_info.__raw__()
assert old_info.mro == old_mro
assert old_info.offsets == old_offsets
assert old_info.methods != old_methods
assert update_old(old, 3) == 43
class LayoutSide(object):
  side: int
class LayoutLater(LayoutSide, LayoutChild):
  padding: Tuple[int, int, int]
  def method(self, *values, **options) -> int:
    return 200 + len(values) + options['extra']
later = __magic__.new(LayoutLater)
later.padding = (7, 8, 9)
later.side = 99
later.balance = 20
later.accrued = 5
assert update_old(later, 4) == 53
assert later.balance == 24 and later.accrued == 29
assert later.padding == (7, 8, 9) and later.side == 99
assert dispatch_old(later) == 203
assert dispatch_new(later) == 205
pointer = __ptr__(later.balance)
pointer[0] = 30
assert update_old(later, 1) == 91
assert update_old(old, 1) == 60
print('second')
)");
    ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
    EXPECT_EQ(*second, "second\n");

    auto rejected = instance.execute(R"(
class LayoutBad(LayoutBase):
  def method(self, *values, **options) -> str:
    return 'incompatible'
bad: LayoutBase = LayoutBad(0, 0)
dispatch_old(bad)
)");
    ASSERT_FALSE(bool(rejected));
    EXPECT_NE(llvm::toString(rejected.takeError()).find("returns str, expected int"),
              std::string::npos);
    auto recovered = instance.execute("assert dispatch_old(old) == 103\n"
                                      "assert dispatch_new(later) == 205\n"
                                      "assert update_old(later, 1) == 124\n"
                                      "print('recovered')\n");
    ASSERT_TRUE(bool(recovered)) << llvm::toString(recovered.takeError());
    EXPECT_EQ(*recovered, "recovered\n");
  }
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

TEST(JITFinalizerTest, EarlierCodeTriggersLaterFinalizers) {
  ASSERT_EXIT(
      {
        auto options = codon::Options::getDefault("build/codon_test");
        options->capture = true;
        options->jit = true;
        codon::jit::JIT jit(*options);
        auto error = jit.init();
        ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
        auto first = jit.execute("finalized = 0\n"
                                 "@noinline\n"
                                 "def old_tick():\n"
                                 "    from C import GC_gcollect()\n"
                                 "    before = finalized\n"
                                 "    GC_gcollect()\n"
                                 "    return finalized > before\n"
                                 "old_tick()\n");
        ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
        auto second =
            jit.execute("class LaterFinalizer:\n"
                        "    value: int\n"
                        "    def __del__(self):\n"
                        "        global finalized\n"
                        "        finalized += 1\n"
                        "        if finalized == 1:\n"
                        "            raise BaseException('late finalizer warning')\n"
                        "values = [LaterFinalizer(index) for index in range(10000)]\n");
        ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
        auto third = jit.execute("values.clear()\n"
                                 "progress = False\n"
                                 "for attempt in range(4):\n"
                                 "    progress = old_tick() or progress\n"
                                 "assert progress\n"
                                 "print('late finalizers ok')\n");
        ASSERT_TRUE(bool(third)) << llvm::toString(third.takeError());
        EXPECT_EQ(*third,
                  "warning: exception in LaterFinalizer.__del__: BaseException: "
                  "late finalizer warning\nlate finalizers ok\n");
        std::_Exit(HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
      },
      testing::ExitedWithCode(EXIT_SUCCESS), "");
}
