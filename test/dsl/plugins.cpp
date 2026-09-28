#include "codon/dsl/plugins.h"
#include "codon/compiler/jit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

namespace codon {
namespace {

TEST(PluginInfoTest, LegacyLibraryDefaultsToCompilerAndRuntime) {
  DSL::Info info = {"name", "description", "1.0.0",  "url",
                    "*",    "stdlib",      "legacy", {"-lextra"}};
  EXPECT_EQ(info.getCompilerDylibPath(), "legacy");
  EXPECT_EQ(info.getRuntimeDylibPath(), "legacy");

  info.compilerDylibPath = "compiler";
  EXPECT_EQ(info.getCompilerDylibPath(), "compiler");
  EXPECT_EQ(info.getRuntimeDylibPath(), "legacy");
  info.runtimeDylibPath = "runtime";
  EXPECT_EQ(info.getRuntimeDylibPath(), "runtime");

  info.runtimeDylibPath = "";
  EXPECT_TRUE(info.getRuntimeDylibPath().empty());
  EXPECT_EQ(info.getCompilerDylibPath(), "compiler");
  info.compilerDylibPath = "";
  EXPECT_TRUE(info.getCompilerDylibPath().empty());
  info.compilerDylibPath.reset();
  info.runtimeDylibPath.reset();
  EXPECT_EQ(info.getCompilerDylibPath(), "legacy");
  EXPECT_EQ(info.getRuntimeDylibPath(), "legacy");
}

class PluginLibrariesTest : public ::testing::Test {
protected:
  llvm::SmallString<128> directory;
  PluginManager manager{""};
  std::string compilerStem;
  std::string runtimeStem;

  void SetUp() override {
    auto error =
        llvm::sys::fs::createUniqueDirectory("codon-plugin-libraries", directory);
    ASSERT_FALSE(error) << error.message();
    compilerStem = "compiler/" + llvm::sys::path::stem(TEST_PLUGIN_COMPILER).str();
    runtimeStem = "runtime/" + llvm::sys::path::stem(TEST_PLUGIN_RUNTIME).str();
    for (const auto &entry : {std::make_pair(TEST_PLUGIN_COMPILER, compilerStem),
                              std::make_pair(TEST_PLUGIN_RUNTIME, runtimeStem)}) {
      auto destination = libraryPath(entry.second);
      error =
          llvm::sys::fs::create_directories(llvm::sys::path::parent_path(destination));
      ASSERT_FALSE(error) << error.message();
      error = llvm::sys::fs::copy_file(entry.first, destination);
      ASSERT_FALSE(error) << error.message();
    }
  }

  void TearDown() override { llvm::sys::fs::remove_directories(directory); }

  void manifest(const std::string &libraries) {
    llvm::SmallString<128> filename(directory);
    llvm::sys::path::append(filename, "plugin.toml");
    std::error_code error;
    llvm::raw_fd_ostream output(filename, error);
    ASSERT_FALSE(error) << error.message();
    output << "[about]\nname = \"test\"\nsupported = \">=0.0.0\"\n[library]\n"
           << libraries << '\n';
  }

  std::string libraryPath(const std::string &name) {
    llvm::SmallString<128> filename(directory);
#ifdef __APPLE__
    llvm::sys::path::append(filename, name + ".dylib");
#else
    llvm::sys::path::append(filename, name + ".so");
#endif
    return std::string(filename.str());
  }

  std::string execute(const std::vector<std::string> &arguments) {
    std::vector<llvm::StringRef> references(arguments.begin(), arguments.end());
    std::string outputPath = std::string(directory.str()) + "/stdout.txt";
    std::string errorPath = std::string(directory.str()) + "/stderr.txt";
    std::vector<std::optional<llvm::StringRef>> redirects = {
        std::nullopt, llvm::StringRef(outputPath), llvm::StringRef(errorPath)};
    std::string message;
    int result = llvm::sys::ExecuteAndWait(arguments.front(), references, std::nullopt,
                                           redirects, 60, 0, &message);
    auto output = llvm::MemoryBuffer::getFile(outputPath);
    auto errors = llvm::MemoryBuffer::getFile(errorPath);
    EXPECT_EQ(result, 0) << message << (errors ? (*errors)->getBuffer().str() : "");
    return output ? (*output)->getBuffer().str() : "";
  }

  void checkExecution(const std::string &libraries, const std::string &code,
                      const std::string &expected, bool linksCompiler,
                      bool linksRuntime) {
    manifest(libraries);
    std::string root(directory.str());
    std::string source = root + "/program.codon";
    std::string executable = root + "/program";
    std::error_code error;
    {
      llvm::raw_fd_ostream output(source, error);
      ASSERT_FALSE(error) << error.message();
      output << code;
    }
    execute(
        {TEST_CODON, "build", "-release", "-plugin", root, "-o", executable, source});
    ASSERT_FALSE(HasFailure());
    EXPECT_EQ(execute({executable}), expected);
    EXPECT_EQ(execute({TEST_CODON, "run", "-release", "-plugin", root, source}),
              expected);
#ifdef __APPLE__
    auto dependencies = execute({"/usr/bin/otool", "-L", executable});
    EXPECT_EQ(
        dependencies.find(llvm::sys::path::filename(TEST_PLUGIN_COMPILER).str()) !=
            std::string::npos,
        linksCompiler);
    EXPECT_EQ(dependencies.find(llvm::sys::path::filename(TEST_PLUGIN_RUNTIME).str()) !=
                  std::string::npos,
              linksRuntime);
    if (!linksCompiler) {
      auto commands = execute({"/usr/bin/otool", "-l", executable});
      EXPECT_EQ(commands.find(root + "/compiler"), std::string::npos);
    }
#endif
  }
};

TEST_F(PluginLibrariesTest, RuntimeOnlyLibraryIsNotLoadedIntoCompiler) {
  manifest("runtime = \"missing-runtime\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  EXPECT_TRUE((*result)->info.getCompilerDylibPath().empty());
  EXPECT_EQ((*result)->info.getRuntimeDylibPath(), libraryPath("missing-runtime"));
  auto error = manager.loadRuntimeLibraries();
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("missing-runtime"),
            std::string::npos);
}

TEST_F(PluginLibrariesTest, EmptyCompilerOverridesLegacyAndRuntimeInheritsIt) {
  manifest("cpp = \"missing-legacy\"\ncompiler = \"\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  EXPECT_EQ((*result)->info.dylibPath, libraryPath("missing-legacy"));
  EXPECT_TRUE((*result)->info.getCompilerDylibPath().empty());
  EXPECT_EQ((*result)->info.getRuntimeDylibPath(), (*result)->info.dylibPath);
}

TEST_F(PluginLibrariesTest, RuntimeLibraryLoadFailuresAreRetried) {
  manifest("runtime = \"retry-runtime\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  for (int attempt = 0; attempt < 2; ++attempt) {
    auto error = manager.loadRuntimeLibraries();
    ASSERT_TRUE(bool(error));
    EXPECT_NE(llvm::toString(std::move(error)).find("retry-runtime"),
              std::string::npos);
  }
  auto copyError =
      llvm::sys::fs::copy_file(TEST_PLUGIN_RUNTIME, libraryPath("retry-runtime"));
  ASSERT_FALSE(copyError) << copyError.message();
  for (int attempt = 0; attempt < 2; ++attempt) {
    auto error = manager.loadRuntimeLibraries();
    ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  }
}

TEST_F(PluginLibrariesTest, LoadsNewRuntimeLibrariesAfterPreviousSuccess) {
  manifest("runtime = \"" + runtimeStem + "\"");
  auto first = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(first)) << llvm::toString(first.takeError());
  auto error = manager.loadRuntimeLibraries();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));

  manifest("runtime = \"late-runtime\"");
  auto second = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(second)) << llvm::toString(second.takeError());
  error = manager.loadRuntimeLibraries();
  ASSERT_TRUE(bool(error));
  EXPECT_NE(llvm::toString(std::move(error)).find("late-runtime"), std::string::npos);

  auto copyError =
      llvm::sys::fs::copy_file(TEST_PLUGIN_RUNTIME, libraryPath("late-runtime"));
  ASSERT_FALSE(copyError) << copyError.message();
  error = manager.loadRuntimeLibraries();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST_F(PluginLibrariesTest, EmptyRuntimeOverridesLegacy) {
  manifest("cpp = \"missing-legacy\"\ncompiler = \"\"\nruntime = \"\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  EXPECT_TRUE((*result)->info.getRuntimeDylibPath().empty());
  auto error = manager.loadRuntimeLibraries();
  EXPECT_FALSE(bool(error)) << llvm::toString(std::move(error));
}

TEST_F(PluginLibrariesTest, CompilerOverrideSelectsTheLoadedLibrary) {
  manifest("cpp = \"missing-legacy\"\ncompiler = \"missing-compiler\"\nruntime = \"\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_FALSE(bool(result));
  auto message = llvm::toString(result.takeError());
  EXPECT_NE(message.find("missing-compiler"), std::string::npos);
  EXPECT_EQ(message.find("missing-legacy"), std::string::npos);
}

TEST_F(PluginLibrariesTest, EmptyRuntimeDoesNotDisableLegacyCompilerLoading) {
  manifest("cpp = \"missing-legacy\"\nruntime = \"\"");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_FALSE(bool(result));
  EXPECT_NE(llvm::toString(result.takeError()).find("missing-legacy"),
            std::string::npos);
}

TEST_F(PluginLibrariesTest, LegacyLinkFalseMeansEmptyRuntime) {
  manifest("cpp = \"missing-legacy\"\ncompiler = \"\"\nlink = false");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  EXPECT_TRUE((*result)->info.getRuntimeDylibPath().empty());
}

TEST_F(PluginLibrariesTest, ExplicitLinkArgumentsRemainIndependent) {
  manifest("runtime = \"\"\nlink = [\"-lstandalone\", \"{root}/custom.a\"]");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  ASSERT_EQ((*result)->info.linkArgs.size(), 2);
  EXPECT_EQ((*result)->info.linkArgs[0], "-lstandalone");
  EXPECT_EQ((*result)->info.linkArgs[1], std::string(directory.str()) + "/custom.a");
}

TEST_F(PluginLibrariesTest, RejectsNonStringLibraryPaths) {
  manifest("runtime = false");
  auto result = manager.load(std::string(directory.str()));
  ASSERT_FALSE(bool(result));
  EXPECT_NE(llvm::toString(result.takeError()).find("library.runtime must be a string"),
            std::string::npos);
}

TEST_F(PluginLibrariesTest, ExecutesWithSeparateCompilerAndRuntimeLibraries) {
  checkExecution("compiler = \"" + compilerStem + "\"\nruntime = \"" + runtimeStem +
                     "\"",
                 "from C import codon_test_runtime_value() -> int\n"
                 "print(codon_test_runtime_value())\n",
                 "42\n", false, true);
}

TEST_F(PluginLibrariesTest, ExecutesWithLegacyCombinedLibrary) {
  checkExecution("cpp = \"" + compilerStem + "\"",
                 "from C import codon_test_compiler_value() -> int\n"
                 "print(codon_test_compiler_value())\n",
                 "17\n", true, false);
}

TEST_F(PluginLibrariesTest, ExecutesWithOnlyRuntimeLibrary) {
  checkExecution("runtime = \"" + runtimeStem + "\"",
                 "from C import codon_test_runtime_value() -> int\n"
                 "print(codon_test_runtime_value())\n",
                 "42\n", false, true);
}

TEST_F(PluginLibrariesTest, ExecutesWithCompilerOnlyAndEmptyRuntime) {
  checkExecution("cpp = \"" + compilerStem + "\"\nruntime = \"\"", "print(7)\n", "7\n",
                 false, false);
}

TEST_F(PluginLibrariesTest, ExecutesWithExplicitRuntimeLinkArguments) {
  checkExecution("compiler = \"" + compilerStem + "\"\nruntime = \"" + runtimeStem +
                     "\"\nlink = [\"{root}/" + runtimeStem +
#ifdef __APPLE__
                     ".dylib\"]",
#else
                     ".so\"]",
#endif
                 "from C import codon_test_runtime_value() -> int\n"
                 "print(codon_test_runtime_value())\n",
                 "42\n", false, true);
}

TEST_F(PluginLibrariesTest, InteractiveJITLoadsRuntimeForLatePlugin) {
  Options options;
  options.argv0 = TEST_CODON;
  jit::JIT instance(options, "", std::string(TEST_DIR) + "/../stdlib");
  auto error = instance.init();
  ASSERT_FALSE(bool(error)) << llvm::toString(std::move(error));
  manifest("runtime = \"" + runtimeStem + "\"");
  auto plugin =
      instance.getCompiler()->getPluginManager()->load(std::string(directory.str()));
  ASSERT_TRUE(bool(plugin)) << llvm::toString(plugin.takeError());
  for (int cell = 0; cell < 2; ++cell) {
    auto result = instance.execute("from C import codon_test_runtime_value() -> int\n"
                                   "assert codon_test_runtime_value() == 42\n");
    ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  }
}

} // namespace
} // namespace codon
