// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <gc.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "codon/cir/flow.h"
#include "codon/cir/func.h"
#include "codon/cir/module.h"
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

TEST(ASTFormattingTest, FloatValues) {
  const std::vector<std::pair<double, std::string>> cases = {
      {0.0, "0.0"},
      {-0.0, "-0.0"},
      {1.0, "1.0"},
      {-1.5, "-1.5"},
      {0.1, "0.1"},
      {1.2345678901234567, "1.2345678901234567"},
      {1e-5, "1e-05"},
      {1e-4, "0.0001"},
      {-1e-4, "-0.0001"},
      {1e15, "1000000000000000.0"},
      {-1e15, "-1000000000000000.0"},
      {1e16, "1e+16"},
      {1e20, "1e+20"},
      {std::numeric_limits<double>::min(), "2.2250738585072014e-308"},
      {std::numeric_limits<double>::max(), "1.7976931348623157e+308"},
      {std::numeric_limits<double>::denorm_min(), "5e-324"},
      {std::numeric_limits<double>::infinity(), "inf"},
      {-std::numeric_limits<double>::infinity(), "-inf"},
      {std::numeric_limits<double>::quiet_NaN(), "nan"},
  };
  for (const auto &[value, expected] : cases) {
    const codon::ast::FloatExpr expression(value);
    EXPECT_EQ(expression.toCodonString(false, -1),
              "FloatExpr(value='" + expression.getRawData().first +
                  "', float_value=" + expected + ")");
  }
}

TEST(TypeCoreTest, NewFunctionRealizationIsIncomplete) {
  auto realization =
      std::make_shared<codon::ast::Cache::Function::FunctionRealization>();
  EXPECT_EQ(realization->type, nullptr);
  EXPECT_EQ(realization->ast, nullptr);
  EXPECT_EQ(realization->ir, nullptr);
  EXPECT_TRUE(realization->captures.empty());
}

TEST(TypeCoreTest, ReplacingFunctionArguments) {
  using namespace codon::ast;
  using namespace codon::ast::types;
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  auto cache = compiler.getCache();
  FunctionStmt ast("replace_args", nullptr, {Param("x"), Param("y")});
  auto before = std::make_shared<ClassType>(cache, "Before");
  auto after = std::make_shared<ClassType>(cache, "After");
  auto args = std::make_shared<ClassType>(
      cache, "Tuple",
      std::vector<ClassType::Generic>{{"T0", before, 0, Runtime},
                                      {"T1", before, 1, Runtime}});
  ClassType base(cache, "Function",
                 {{"T", args, 2, Runtime}, {"TR", before, 3, Runtime}});
  FuncType function(&base, &ast), sibling(&base, &ast);
  // Issue 5: prime both names, then specialize without mutating a shared tuple.
  auto oldArgsName = args->realizedName();
  auto oldFunctionName = function.ClassType::realizedName();
  auto oldSignature = function.realizedName();
  function.replaceArgTypes({after, nullptr});
  EXPECT_EQ(function[0], after.get());
  EXPECT_EQ(function[1], before.get());
  EXPECT_NE(function.ClassType::realizedName(), oldFunctionName);
  EXPECT_NE(function.realizedName(), oldSignature);
  EXPECT_EQ(args->realizedName(), oldArgsName);
  EXPECT_EQ(sibling[0], before.get());
  EXPECT_EQ(sibling.ClassType::realizedName(), oldFunctionName);
  function.replaceArgTypes({before, after});
  EXPECT_EQ(function[0], before.get());
  EXPECT_EQ(function[1], after.get());
}

TEST(TypeCoreTest, ChecksSharedTypeGraphsAndRestoresLevels) {
  using namespace codon::ast::types;
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  auto *cache = compiler.getCache();
  auto leaf = std::make_shared<LinkType>(cache, LinkType::Unbound,
                                         cache->unboundCount++, 5, nullptr);
  auto variable = std::make_shared<LinkType>(cache, LinkType::Unbound,
                                             cache->unboundCount++, 1, nullptr);
  TypePtr shared = leaf;
  // scratch.codon builds shared closure graphs. Occurs checks must visit shared
  // nodes once while still lowering free-variable levels and detecting cycles.
  for (int depth = 0; depth < 24; ++depth)
    shared = std::make_shared<ClassType>(
        cache, "Shared",
        std::vector<ClassType::Generic>{{"left", shared, 0, Runtime},
                                        {"right", shared, 1, Runtime}});
  Type::Unification changes;
  ASSERT_GE(variable->unify(shared.get(), &changes), 0);
  EXPECT_EQ(leaf->level, 1);
  EXPECT_EQ(changes.leveled.size(), 1);
  changes.undo();
  EXPECT_EQ(leaf->level, 5);
  EXPECT_EQ(variable->kind, LinkType::Unbound);

  auto recursive = std::make_shared<ClassType>(
      cache, "Recursive",
      std::vector<ClassType::Generic>{{"shared", shared, 0, Runtime},
                                      {"cycle", variable, 1, Runtime}});
  Type::Unification rejected;
  EXPECT_LT(variable->unify(recursive.get(), &rejected), 0);
  rejected.undo();
  EXPECT_EQ(variable->kind, LinkType::Unbound);
  EXPECT_EQ(leaf->level, 5);
  EXPECT_GE(variable->unify(shared.get(), nullptr), 0);
  EXPECT_EQ(leaf->level, 5);
}

// Missing or conditional writes are errors; initialization through aliases, generic
// helpers, and super() is recognized. Decorator opt-outs suppress field diagnostics.
TEST(FieldInitializationTest, RejectsUninitializedFields) {
  const std::vector<std::pair<std::string, std::string>> constructors = {
      {"Missing", "    pass\n"},
      {"Conditional", "    if flag:\n      self.values = [1]\n"},
      {"EarlyReturn", "    if flag:\n      return\n    self.values = [1]\n"},
  };
  for (const auto &[name, body] : constructors) {
    for (bool debug : {false, true}) {
      SCOPED_TRACE(name + (debug ? " debug" : " release"));
      auto options = codon::Options::getDefault("build/codon_test");
      options->debug = debug;
      codon::Compiler compiler(*options);
      auto error = compiler.parseCode("field_initialization.codon",
                                      "class " + name +
                                          ":\n  values: list[int]\n"
                                          "  def __init__(self, flag: bool):\n" +
                                          body + name + "(False)\n");
      ASSERT_TRUE(bool(error));
      EXPECT_NE(llvm::toString(std::move(error))
                    .find("field '" + name +
                          ".values' may be uninitialized when __init__ returns"),
                std::string::npos);
    }
  }
}

TEST(FieldInitializationTest, TracksHelpersAndSupportsOptOut) {
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  testing::internal::CaptureStderr();
  auto error = compiler.parseCode("field_initialization.codon", R"(
def initialize_values(Element: type, instance):
  target = instance
  target.values = [Element(1)]

class ThroughHelpers:
  values: list[int]
  def __init__(self):
    self.initialize()
  def initialize(self):
    initialize_values(int, self)

class Parent:
  value: int
  def __init__(self):
    self.value = 1

class Inherits(Parent):
  def __init__(self):
    super().__init__()

@dataclass(init=False)
class Unchecked:
  values: list[int]
  def __new__():
    return type._ref_new(Unchecked)

@tuple
class Value:
  number: int
  def __init__(self):
    pass

ThroughHelpers()
Inherits()
Unchecked()
Value(1).__init__()
)");
  auto warnings = testing::internal::GetCapturedStderr();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  EXPECT_EQ(warnings.find("field 'ThroughHelpers.values'"), std::string::npos)
      << warnings;
  EXPECT_EQ(warnings.find("field 'Unchecked.values'"), std::string::npos) << warnings;
  EXPECT_EQ(warnings.find("field 'Value.number'"), std::string::npos) << warnings;
  EXPECT_EQ(warnings.find("field 'Inherits.value'"), std::string::npos) << warnings;
  EXPECT_EQ(warnings.find("field 'Exception."), std::string::npos) << warnings;
  EXPECT_EQ(warnings.find("field 'ValueError."), std::string::npos) << warnings;
}

TEST(FieldInitializationTest, ChecksCooperativeSuperInitialization) {
  for (bool cooperative : {false, true}) {
    for (bool debug : {false, true}) {
      SCOPED_TRACE(cooperative ? "cooperative super" : "skipped right base");
      auto options = codon::Options::getDefault("build/codon_test");
      options->debug = debug;
      codon::Compiler compiler(*options);
      std::string source = R"(
class Base:
  value: int
  def __init__(self, value: int):
    self.value = value
class Left(Base):
  left: int
  def __init__(self, value: int):
)";
      source += cooperative ? "    super().__init__(value)\n"
                            : "    Base.__init__(self, value)\n";
      source += R"(
    self.left = value + 1
class Right(Base):
  right: int
  def __init__(self, value: int):
    super().__init__(value)
    self.right = value + 2
class Both(Left, Right):
  own: int
  def __init__(self, value: int):
    super().__init__(value)
    self.own = value + 3
instance = Both(10)
assert instance.value == 10
assert instance.left == 11
assert instance.right == 12
assert instance.own == 13
)";
      auto error = compiler.parseCode("field_initialization_super.codon", source);
      if (cooperative) {
        ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
      } else {
        ASSERT_TRUE(bool(error));
        EXPECT_NE(llvm::toString(std::move(error))
                      .find("field 'Both.right' may be uninitialized"),
                  std::string::npos);
      }
    }
  }
}

TEST(FieldInitializationTest, TracksVirtualInitialization) {
  for (bool baseWrites : {false, true}) {
    for (bool childWrites : {false, true}) {
      for (bool debug : {false, true}) {
        SCOPED_TRACE(baseWrites ? "base writes" : "base skips");
        SCOPED_TRACE(childWrites ? "child writes" : "child skips");
        auto options = codon::Options::getDefault("build/codon_test");
        options->debug = debug;
        codon::Compiler compiler(*options);
        std::string source = R"(
class Base:
  value: int
  def __init__(self):
    initialize_base(self)
  def initialize(self):
)";
        source += baseWrites ? "    self.value = 1\n" : "    pass\n";
        source += "class Child(Base):\n  def initialize(self):\n";
        source += childWrites ? "    self.value = 42\n" : "    pass\n";
        source += R"(
def initialize_base(instance: Base):
  instance.initialize()
Child()
)";
        auto error = compiler.parseCode("field_initialization_virtual.codon", source);
        if (childWrites) {
          ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
          if (!baseWrites) {
            auto *module = compiler.getCache()->module;
            auto *base = module->getOrRealizeType("Base.0");
            ASSERT_NE(base, nullptr);
            EXPECT_EQ(module->getOrRealizeMethod(base, "__init__", {base}), nullptr);
          }
        } else {
          ASSERT_TRUE(bool(error));
          EXPECT_NE(
              llvm::toString(std::move(error)).find(".value' may be uninitialized"),
              std::string::npos);
        }
      }
    }
  }
}

TEST(FieldInitializationTest, RejectsDeferredWrites) {
  for (bool asynchronous : {false, true}) {
    for (bool constructor : {false, true}) {
      for (bool debug : {false, true}) {
        SCOPED_TRACE(asynchronous ? "coroutine" : "generator");
        SCOPED_TRACE(constructor ? "constructor" : "helper");
        auto options = codon::Options::getDefault("build/codon_test");
        options->debug = debug;
        codon::Compiler compiler(*options);
        std::string source = "class Record:\n  value: int\n";
        if (!constructor)
          source += "  def __init__(self):\n    self.initialize()\n";
        source += asynchronous ? "  async def " : "  def ";
        source += constructor ? "__init__" : "initialize";
        source += "(self):\n    self.value = 42\n";
        if (!asynchronous)
          source += "    yield 1\n";
        source += "Record()\n";
        auto error = compiler.parseCode("field_initialization_deferred.codon", source);
        ASSERT_TRUE(bool(error));
        EXPECT_NE(llvm::toString(std::move(error))
                      .find("field 'Record.value' may be uninitialized"),
                  std::string::npos);
      }
    }
  }
}

TEST(FieldInitializationTest, RejectsReboundAliases) {
  const std::vector<std::string> bodies = {
      "      for unused in range(1):\n        target = other\n"
      "      target.value = 42\n",
      "      active = True\n      while active:\n        target = other\n"
      "        active = False\n      target.value = 42\n",
      "      for index in range(2):\n        if index == 1:\n"
      "          target.value = 42\n          return\n        target = other\n"
      "      self.value = 99\n",
      "      for target in [other]:\n        target.value = 42\n        return\n"
      "      self.value = 99\n",
      "      try:\n        target = other\n        raise ValueError()\n"
      "      except ValueError:\n        target.value = 42\n",
      "      try:\n        fail(target := other)\n"
      "      except ValueError:\n        target.value = 42\n",
      "      try:\n        for target in instances(other):\n          pass\n"
      "      except ValueError:\n        target.value = 42\n",
  };
  for (const auto &body : bodies) {
    for (bool debug : {false, true}) {
      SCOPED_TRACE(body);
      auto options = codon::Options::getDefault("build/codon_test");
      options->debug = debug;
      codon::Compiler compiler(*options);
      auto error = compiler.parseCode("field_initialization_alias.codon", R"(
def fail(instance):
  raise ValueError()
def instances(instance):
  yield instance
  raise ValueError()
class Record:
  value: int
  def __init__(self, other=None):
    if other is None:
      self.value = 1
    else:
      target = self
)" + body + "Record(Record())\n");
      ASSERT_TRUE(bool(error));
      EXPECT_NE(llvm::toString(std::move(error))
                    .find("field 'Record.value' may be uninitialized"),
                std::string::npos);
    }
  }
}

TEST(FieldInitializationTest, ChecksFinallyExitOverrides) {
  const std::vector<std::pair<std::string, bool>> bodies = {
      {"    try:\n      raise ValueError()\n    finally:\n      return\n", false},
      {"    try:\n      fail()\n    finally:\n      return\n", false},
      {"    for unused in range(1):\n      try:\n        break\n"
       "      finally:\n        return\n    self.value = 42\n",
       false},
      {"    for unused in range(1):\n      try:\n        continue\n"
       "      finally:\n        return\n    self.value = 42\n",
       false},
      {"    try:\n      raise ValueError()\n    finally:\n"
       "      self.value = 42\n      return\n",
       true},
      {"    try:\n      return\n    finally:\n      self.value = 42\n", true},
      {"    try:\n      raise ValueError()\n    except:\n      self.value = 42\n"
       "    finally:\n      return\n",
       true},
      {"    try:\n      raise ValueError()\n    except BaseException:\n"
       "      self.value = 42\n    finally:\n      return\n",
       true},
  };
  for (const auto &[body, valid] : bodies) {
    for (bool debug : {false, true}) {
      SCOPED_TRACE(body);
      auto options = codon::Options::getDefault("build/codon_test");
      options->debug = debug;
      codon::Compiler compiler(*options);
      auto error = compiler.parseCode("field_initialization_finally.codon",
                                      "def fail():\n  raise ValueError()\n"
                                      "class Record:\n  value: int\n"
                                      "  def __init__(self):\n" +
                                          body + "Record()\n");
      if (valid) {
        ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
      } else {
        ASSERT_TRUE(bool(error));
        EXPECT_NE(llvm::toString(std::move(error))
                      .find("field 'Record.value' may be uninitialized"),
                  std::string::npos);
      }
    }
  }
}

TEST(FieldInitializationTest, ChecksLateRealizations) {
  for (bool pyext : {false, true}) {
    for (bool initialized : {false, true}) {
      SCOPED_TRACE(pyext ? "Python export" : "late SDK realization");
      auto options = codon::Options::getDefault("build/codon_test");
      options->pyext = pyext;
      codon::Compiler compiler(*options);
      std::string source = R"(
class Late:
  value: int
  def __init__(self):
)";
      source += initialized ? "    self.value = 42\n" : "    pass\n";
      source += R"(
def make() -> int:
  return Late().value
)";
      auto error = compiler.parseCode("field_initialization_late.codon", source);
      if (pyext && !initialized) {
        ASSERT_TRUE(bool(error));
        EXPECT_NE(llvm::toString(std::move(error))
                      .find("field 'Late.value' may be uninitialized"),
                  std::string::npos);
      } else {
        ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
        if (!pyext)
          EXPECT_EQ(compiler.getCache()->module->getOrRealizeFunc("make", {}) !=
                        nullptr,
                    initialized);
      }
    }
  }
}

TEST(FieldInitializationTest, ReusesSharedHelperSummaries) {
  // Each level has two edges to the same helper, but the graph has only 25 helpers.
  // Without completed summaries, analysis repeats exponentially despite having no
  // recursion. This depth exercises the scaling regression without a timing threshold.
  std::string source =
      "def initialize_0(instance, seed: int):\n    instance.value = seed\n";
  for (int level = 1; level <= 24; ++level) {
    auto previous = "initialize_" + std::to_string(level - 1);
    source += "def initialize_" + std::to_string(level) +
              "(instance, seed: int):\n"
              "    if seed % 2 == 0:\n        " +
              previous + "(instance, seed + 1)\n    else:\n        " + previous +
              "(instance, seed + 2)\n";
  }
  // The same receiver in both slots also requires parameter-specific summaries.
  source += R"(
class Record:
    value: int
    def __init__(self, seed: int):
        initialize_24(self, seed)

def initialize_pair(first, second):
    first.first = 1
    second.second = 2

class Pair:
    first: int
    second: int
    def __init__(self):
        initialize_pair(self, self)

assert Record(1).value == 49
assert Pair().first == 1 and Pair().second == 2
)";
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  testing::internal::CaptureStderr();
  auto error = compiler.parseCode("field_initialization_shared.codon", source);
  auto warnings = testing::internal::GetCapturedStderr();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  EXPECT_EQ(warnings.find("may be uninitialized"), std::string::npos) << warnings;
}

TEST(FieldInitializationTest, RejectsTupleInitOption) {
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  auto error = compiler.parseCode("tuple_init_option.codon", R"(
@tuple(check_init=False)
class Value:
  number: int
)");
  ASSERT_TRUE(bool(error));
  EXPECT_NE(
      llvm::toString(std::move(error)).find("class decorator got unexpected argument"),
      std::string::npos);
}

TEST(EnumConstructionTest, BoundsHelperRealizations) {
  std::string source = "from enum import Enum, auto\nclass Explicit(Enum):\n";
  for (int index = 0; index < 32; ++index)
    source +=
        "    MEMBER_" + std::to_string(index) + " = " + std::to_string(index) + "\n";
  source += "class Automatic(Enum):\n";
  for (int index = 0; index < 32; ++index)
    source += "    MEMBER_" + std::to_string(index) + " = auto()\n";
  source += "assert Explicit(31) is Explicit.MEMBER_31\n"
            "assert Automatic(32) is Automatic.MEMBER_31\n";

  for (bool unordered : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->debug = false;
    options->unordereddict = unordered;
    codon::Compiler compiler(*options);
    auto error = compiler.parseCode("enum_construction.codon", source);
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));

    auto countRealizations = [&](const std::string &helper) {
      size_t count = 0;
      auto prefix = codon::ast::getMangledFunc("std.enum", helper);
      for (const auto &[name, function] : compiler.getCache()->functions)
        if (name.starts_with(prefix))
          count += function.realizations.size();
      return count;
    };
    EXPECT_EQ(countRealizations("_make_member"), 2u);
    EXPECT_EQ(countRealizations("_next_value"), 1u);
    EXPECT_EQ(countRealizations("_auto_maximum"), 1u);
  }
}

TEST(EnumConstructionTest, SkipsLookupInitialization) {
  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  testing::internal::CaptureStderr();
  auto error = compiler.parseCode("enum_lookup_initialization.codon", R"(
from enum import Enum
class Color(Enum):
    RED = 1
    ALIAS = 1
assert Color(1) is Color.RED
assert Color(Color.RED) is Color.RED
assert Color.ALIAS is Color.RED

class ExplicitEmpty:
    @staticmethod
    def __new__(value: int):
        return type._ref_new(ExplicitEmpty)
    def __init__(self, value: int):
        pass
ExplicitEmpty(1)
)");
  auto warnings = testing::internal::GetCapturedStderr();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  EXPECT_EQ(warnings.find("field 'Enum."), std::string::npos) << warnings;
  auto enumInit = codon::ast::getMangledClass("std.enum", "Enum") + ".__init__:";
  bool realizedExplicitEmpty = false;
  for (const auto &[name, function] : compiler.getCache()->functions) {
    EXPECT_FALSE(name.starts_with(enumInit)) << name;
    if (name.find("ExplicitEmpty") != std::string::npos &&
        name.find(".__init__:") != std::string::npos &&
        !function.ast->hasAttribute(codon::ast::Attr::AutoGenerated))
      realizedExplicitEmpty |= !function.realizations.empty();
  }
  EXPECT_TRUE(realizedExplicitEmpty);
}

TEST(ContextManagerLoweringTest, OmitsHandlersOnlyForLegacyNoneExits) {
  const std::vector<std::tuple<std::string, std::string, bool, bool>> cases = {
      {"context_legacy", "LegacyNone", false, true},
      {"context_bool", "LegacyBool", false, false},
      {"context_aware", "AwareNone", false, false},
      {"context_defaults", "DefaultNone", false, false},
      {"context_overloaded", "OverloadedNone", false, false},
      {"context_async_legacy", "AsyncNone", true, true},
      {"context_async_bool", "AsyncBool", true, false},
  };
  std::string source = R"(
class LegacyNone:
  def __enter__(self):
    return 7
  def __exit__(self):
    pass

class LegacyBool:
  def __enter__(self):
    return 7
  def __exit__(self):
    return True

class AwareNone:
  def __enter__(self):
    return 7
  def __exit__(self, exc_type, exc_value, traceback):
    pass

class DefaultNone:
  def __enter__(self):
    return 7
  def __exit__(self, exc_type=None, exc_value=None, traceback=None):
    pass

class OverloadedNone:
  def __enter__(self):
    return 7
  def __exit__(self):
    pass
  def __exit__(self, exc_type, exc_value, traceback):
    pass

class AsyncNone:
  async def __aenter__(self):
    return 7
  async def __aexit__(self):
    pass

class AsyncBool:
  async def __aenter__(self):
    return 7
  async def __aexit__(self):
    return True
)";
  for (const auto &[name, manager, isAsync, legacy] : cases)
    source += std::string(isAsync ? "async def " : "def ") + name + "():\n    " +
              (isAsync ? "async with " : "with ") + manager +
              "() as value:\n        return value\n" + name + "()\n";

  auto options = codon::Options::getDefault("build/codon_test");
  codon::Compiler compiler(*options);
  auto error = compiler.parseCode("context_manager_lowering.codon", source);
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));

  size_t checked = 0;
  for (auto *variable : *compiler.getModule()) {
    auto *function = codon::ir::cast<codon::ir::BodiedFunc>(variable);
    if (!function)
      continue;
    auto expected = std::ranges::find_if(cases, [&](const auto &entry) {
      return std::get<0>(entry) == function->getUnmangledName();
    });
    if (expected == cases.end())
      continue;
    SCOPED_TRACE(function->getUnmangledName());
    ++checked;
    int tries = 0;
    int catches = 0;
    int flags = 0;
    std::vector<codon::ir::Value *> pending{function->getBody()};
    while (!pending.empty()) {
      auto *value = pending.back();
      pending.pop_back();
      if (!value)
        continue;
      if (auto *flow = codon::ir::cast<codon::ir::TryCatchFlow>(value)) {
        ++tries;
        catches += std::distance(flow->begin(), flow->end());
        EXPECT_NE(flow->getFinally(), nullptr);
      }
      auto children = value->getUsedValues();
      pending.insert(pending.end(), children.begin(), children.end());
    }
    for (auto *local : *function)
      if (local->getName().find("with_exception") != std::string::npos)
        ++flags;
    EXPECT_EQ(tries, 1);
    EXPECT_EQ(catches, std::get<3>(*expected) ? 0 : 1);
    EXPECT_EQ(flags, std::get<3>(*expected) ? 0 : 1);
  }
  EXPECT_EQ(checked, cases.size());
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
                         "assert CellPlain.__elemsize__ == Ptr[int].__elemsize__\n"
                         "assert not CellPlain.__atomic__\n"
                         "assert CellPlain.__contents_atomic__\n"
                         "assert type(()).__elemsize__ == 0\n"
                         "@tuple\nclass CellMixed:\n"
                         "    owner: CellPlain\n    count: int\n"
                         "assert not CellMixed.__atomic__\n"
                         "assert CellMixed.__contents_atomic__\n"
                         "class CellOwner:\n    owner: CellPlain\n"
                         "assert not CellOwner.__contents_atomic__\n"
                         "def saved_size_query(): return CellMixed.__elemsize__\n"
                         "assert saved_size_query() == 2 * int.__elemsize__\n"
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
class CellPayload(CellBase):
  left: int
  right: int
assert CellPayload.__elemsize__ == Ptr[int].__elemsize__
assert type._ref_size(CellPayload) == CellPayload.__elemsize__ + 2 * int.__elemsize__
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
                         "assert CellPlain.__elemsize__ == Ptr[int].__elemsize__\n"
                         "assert CellPlain.__contents_atomic__\n"
                         "assert saved_size_query() == CellMixed.__elemsize__\n"
                         "assert not CellMixed.__atomic__\n"
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
class LayoutLater(LayoutChild, LayoutSide):
  padding: Tuple[int, int, int]
  def method(self, *values, **options) -> int:
    return 200 + len(values) + options['extra']
later = __magic__.new(LayoutLater)
later.padding = (7, 8, 9)
later.side = 99
later.balance = 20
later.accrued = 5
assert TypeInfo.cache(LayoutLater)._dist_to(LayoutBase) != old_info._dist_to(LayoutBase)
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

TEST(JITRTTITest, MatchesBaseExceptionAcrossCells) {
  for (bool debug : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->jit = true;
    options->debug = debug;
    options->capture = true;
    codon::jit::JIT instance(*options);
    auto error = instance.init();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
    auto first =
        instance.execute("@noinline\n"
                         "def catch_future(error: BaseException):\n"
                         "    try:\n"
                         "        raise error\n"
                         "    except TypeError:\n"
                         "        return 'wrong handler'\n"
                         "    except BaseException as caught:\n"
                         "        assert caught is error\n"
                         "        return caught.message\n"
                         "assert catch_future(KeyboardInterrupt('old')) == 'old'\n"
                         "print('first')\n");
    ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
    EXPECT_EQ(*first, "first\n");
    auto second = instance.execute(
        "class ExceptionPadding(object):\n"
        "    padding: tuple[int, int, int]\n"
        "class FutureError(ValueError, ExceptionPadding):\n"
        "    payload: int\n"
        "    def __init__(self):\n"
        "        super().__init__('shifted exception')\n"
        "        self.padding = (17, 19, 23)\n"
        "        self.payload = 29\n"
        "future = FutureError()\n"
        "assert TypeInfo.cache(FutureError)._dist_to(BaseException) != "
        "TypeInfo.cache(ValueError)._dist_to(BaseException)\n"
        "assert catch_future(future) == 'shifted exception'\n"
        "assert future.padding == (17, 19, 23) and future.payload == 29\n"
        "print('second')\n");
    ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
    EXPECT_EQ(*second, "second\n");
    auto unhandled =
        instance.execute("from C import seq_alloc_exc(cobj) -> cobj\n"
                         "from C import seq_terminate(cobj) -> None\n"
                         "seq_terminate(seq_alloc_exc(type._ref_raw(future)))\n");
    ASSERT_FALSE(bool(unhandled));
    auto message = llvm::toString(unhandled.takeError());
    EXPECT_NE(message.find("FutureError"), std::string::npos);
    EXPECT_NE(message.find("shifted exception"), std::string::npos);
    auto recovered = instance.execute(
        "assert catch_future(future) == 'shifted exception'\n"
        "assert future.padding == (17, 19, 23) and future.payload == 29\n"
        "print('recovered')\n");
    ASSERT_TRUE(bool(recovered)) << llvm::toString(recovered.takeError());
    EXPECT_EQ(*recovered, "recovered\n");
  }
}

TEST(JITRTTITest, RefreshesAnyValueCallbacksAcrossCells) {
  for (bool debug : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->jit = true;
    options->debug = debug;
    options->capture = true;
    codon::jit::JIT instance(*options);
    auto error = instance.init();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
    auto first = instance.execute(R"(
class ValueBase:
  value: int
  def __eq__(self, other: ValueBase): return self.value == other.value
  def __hash__(self): return hash(self.value)
  def marker(self): return self.value
class ValueChild(ValueBase):
  def __hash__(self): return hash(self.value + 100)
  def marker(self): return self.value + 10
def box_base(value: ValueBase) -> Any:
  return Any(value)
def marker_base(value: ValueBase): return value.marker()
child = ValueChild(17)
assert marker_base(child) == 27
saved = box_base(child)
number = Any(17)
number_info = TypeInfo.cache(int)
number_storage = Ptr[int](1)
number_storage[0] = 17
copied_number = Any(number_info, number_storage.as_byte())
assert number._is_inline() and copied_number._is_inline()
assert number_info.inline_value
assert number._typeinfo == number_info
assert number.__unwrap__(int) == 17
old_info = TypeInfo.cache(ValueChild)
hash_method_id = TypeInfo._value_method_id('_hash_value')
equal_method_id = TypeInfo._value_method_id('_equal_values')
assert hash_method_id > 0 and equal_method_id > 0
assert hash_method_id != equal_method_id
class ValueMissing:
  value: int
unsupported = Any([ValueMissing(5)])
print('boxed')
)");
    ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
    EXPECT_EQ(*first, "boxed\n");

    auto second = instance.execute(R"(
def hash_any(value: Any): return hash(value)
assert hash_any(saved) == hash(child)
assert hash_any(number) == hash(17)
assert hash_any(copied_number) == hash_any(number)
assert number._typeinfo.__raw__() == number_info.__raw__()
assert TypeInfo._value_method_id('_hash_value') == hash_method_id
hash_target = old_info._lookup_method(hash_method_id)
assert Function[[Ptr[byte]], int](hash_target)(saved._data) == hash(child)
assert marker_base(child) == 27
try:
  hash_any(unsupported)
  assert False
except TypeError:
  pass
assert TypeInfo.cache(ValueChild).__raw__() == old_info.__raw__()
print('hashed')
)");
    ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
    EXPECT_EQ(*second, "hashed\n");

    auto third = instance.execute(R"(
def equal_any(left: Any, right: Any): return left == right
assert TypeInfo._value_method_id('_equal_values') == equal_method_id
refreshed_hash = old_info._lookup_method(hash_method_id)
assert Function[[Ptr[byte]], int](refreshed_hash)(saved._data) == hash(child)
assert equal_any(saved, box_base(ValueChild(17)))
assert not equal_any(saved, box_base(ValueBase(17)))
assert equal_any(number, Any(17))
assert not equal_any(number, Any(17.0))
assert equal_any(number, copied_number) and equal_any(copied_number, number)
number_storage[0] = 23
assert equal_any(number, copied_number)
assert copied_number.__unwrap__(int) == 17 and number.__unwrap__(int) == 17
assert Any(number_info, number_storage.as_byte()).__unwrap__(int) == 23
from C import GC_gcollect()
GC_gcollect()
assert hash_any(number) == hash(17)
assert number._typeinfo.__raw__() == number_info.__raw__()
try:
  equal_any(unsupported, unsupported)
  assert False
except TypeError:
  pass
class ValueLater(ValueBase):
  def __hash__(self): return hash(self.value + 200)
  def marker(self): return self.value + 20
later = ValueLater(19)
assert marker_base(later) == 39
assert marker_base(child) == 27
assert equal_any(box_base(later), box_base(ValueLater(19)))
assert not equal_any(saved, box_base(later))
assert hash_any(box_base(later)) == hash(later)
assert hash_any(saved) == hash(child)
assert TypeInfo.cache(ValueChild).__raw__() == old_info.__raw__()
print('compared')
)");
    ASSERT_TRUE(bool(third)) << llvm::toString(third.takeError());
    EXPECT_EQ(*third, "compared\n");
  }
}

TEST(JITRTTITest, RefreshesAnyDictionaryAdaptersAcrossCells) {
  for (bool debug : {false, true}) {
    auto options = codon::Options::getDefault("build/codon_test");
    options->jit = true;
    options->debug = debug;
    options->capture = true;
    codon::jit::JIT instance(*options);
    auto error = instance.init();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
    auto first = instance.execute(R"(
source = {'second': 2, 'first': 1}
saved = Any(source)
old_info = TypeInfo.cache(type(source))
dict_method_id = TypeInfo._value_method_id('_dict_value')
print('boxed')
)");
    ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
    EXPECT_EQ(*first, "boxed\n");

    auto second = instance.execute(R"(
def convert_any(value: Any): return value.__cast__(Dict[Any, Any])
converted = convert_any(saved)
assert [(key.__unwrap__(str), value.__unwrap__(int))
        for key, value in converted.items()] == list(source.items())
assert TypeInfo._value_method_id('_dict_value') == dict_method_id
assert TypeInfo.cache(type(source)).__raw__() == old_info.__raw__()
print('converted')
)");
    ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
    EXPECT_EQ(*second, "converted\n");

    auto third = instance.execute(R"(
later = convert_any(Any({3: 1.5}))
assert later[Any(3)].__unwrap__(float) == 1.5
source['third'] = 3
assert [key.__unwrap__(str) for key in convert_any(saved)] == ['second', 'first', 'third']
assert [value.__unwrap__(int) for value in converted.values()] == [2, 1]
assert TypeInfo._value_method_id('_dict_value') == dict_method_id
assert TypeInfo.cache(type(source)).__raw__() == old_info.__raw__()
print('refreshed')
)");
    ASSERT_TRUE(bool(third)) << llvm::toString(third.takeError());
    EXPECT_EQ(*third, "refreshed\n");
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
