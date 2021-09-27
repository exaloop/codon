#include "dsl/plugins.h"
#include "parser/parser.h"
#include "seq/seq.h"
#include "sir/llvm/llvisitor.h"
#include "sir/transform/manager.h"
#include "sir/transform/pass.h"
#include "util/common.h"
#include "llvm/Support/CommandLine.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
void versMsg(llvm::raw_ostream &out) {
  out << "Seq " << SEQ_VERSION_MAJOR << "." << SEQ_VERSION_MINOR << "."
      << SEQ_VERSION_PATCH << "\n";
}

const std::vector<std::string> &supportedExtensions() {
  static const std::vector<std::string> extensions = {".seq", ".py"};
  return extensions;
}

bool hasExtension(const std::string &filename, const std::string &extension) {
  return filename.size() >= extension.size() &&
         filename.compare(filename.size() - extension.size(), extension.size(),
                          extension) == 0;
}

std::string trimExtension(const std::string &filename, const std::string &extension) {
  if (hasExtension(filename, extension)) {
    return filename.substr(0, filename.size() - extension.size());
  } else {
    return filename;
  }
}

std::string makeOutputFilename(const std::string &filename,
                               const std::string &extension) {
  for (const auto &ext : supportedExtensions()) {
    if (hasExtension(filename, ext))
      return trimExtension(filename, ext) + extension;
  }
  return filename + extension;
}

enum BuildKind { LLVM, Bitcode, Object, Executable, Detect };
enum OptMode { Debug, Release };
struct ProcessResult {
  std::unique_ptr<seq::ir::LLVMVisitor> visitor;
  std::string input;
};
} // namespace

int docMode(const std::vector<const char *> &args, const std::string &argv0) {
  llvm::cl::ParseCommandLineOptions(args.size(), args.data());
  seq::generateDocstr(argv0);
  return EXIT_SUCCESS;
}

ProcessResult processSource(const std::vector<const char *> &args) {
  llvm::cl::opt<std::string> input(llvm::cl::Positional, llvm::cl::desc("<input file>"),
                                   llvm::cl::init("-"));
  llvm::cl::opt<OptMode> optMode(
      llvm::cl::desc("optimization mode"),
      llvm::cl::values(
          clEnumValN(Debug, "debug",
                     "Turn off compiler optimizations and show backtraces"),
          clEnumValN(Release, "release",
                     "Turn on compiler optimizations and disable debug info")),
      llvm::cl::init(Debug));
  llvm::cl::list<std::string> defines(
      "D", llvm::cl::Prefix,
      llvm::cl::desc("Add static variable definitions. The syntax is <name>=<value>"));
  llvm::cl::list<std::string> disabledOpts(
      "disable-opt", llvm::cl::desc("Disable the specified IR optimization"));
  llvm::cl::list<std::string> dsls("dsl", llvm::cl::desc("Use specified DSL"));

  llvm::cl::ParseCommandLineOptions(args.size(), args.data());

  auto &exts = supportedExtensions();
  if (input != "-" && std::find_if(exts.begin(), exts.end(), [&](auto &ext) {
                        return hasExtension(input, ext);
                      }) == exts.end())
    seq::compilationError(
        "input file is expected to be a .seq/.py file, or '-' for stdin");

  std::unordered_map<std::string, std::string> defmap;
  for (const auto &define : defines) {
    auto eq = define.find('=');
    if (eq == std::string::npos || !eq) {
      seq::compilationWarning("ignoring malformed definition: " + define);
      continue;
    }

    auto name = define.substr(0, eq);
    auto value = define.substr(eq + 1);

    if (defmap.find(name) != defmap.end()) {
      seq::compilationWarning("ignoring duplicate definition: " + define);
      continue;
    }

    defmap.emplace(name, value);
  }

  auto *module = seq::parse(args[0], input.c_str(), /*code=*/"", /*isCode=*/false,
                            /*isTest=*/false, /*startLine=*/0, defmap);
  if (!module)
    return {{}, {}};

  const bool isDebug = (optMode == OptMode::Debug);
  auto t = std::chrono::high_resolution_clock::now();

  std::vector<std::string> disabledOptsVec(disabledOpts);
  seq::ir::transform::PassManager pm(isDebug, disabledOptsVec);
  seq::PluginManager plm(&pm, isDebug);

  // load Seq
  seq::Seq seqDSL;
  plm.load(&seqDSL);

  LOG_TIME("[T] ir-setup = {:.1f}",
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t)
                   .count() /
               1000.0);

  // load other plugins
  for (const auto &dsl : dsls) {
    auto result = plm.load(dsl);
    switch (result) {
    case seq::PluginManager::NONE:
      break;
    case seq::PluginManager::NOT_FOUND:
      seq::compilationError("DSL '" + dsl + "' not found");
      break;
    case seq::PluginManager::NO_ENTRYPOINT:
      seq::compilationError("DSL '" + dsl + "' has no entry point");
      break;
    case seq::PluginManager::UNSUPPORTED_VERSION:
      seq::compilationError("DSL '" + dsl + "' version incompatible");
      break;
    default:
      break;
    }
  }
  t = std::chrono::high_resolution_clock::now();
  pm.run(module);
  LOG_TIME("[T] ir-opt = {:.1f}", std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - t)
                                          .count() /
                                      1000.0);

  t = std::chrono::high_resolution_clock::now();
  auto visitor = std::make_unique<seq::ir::LLVMVisitor>(isDebug);
  visitor->visit(module);
  LOG_TIME("[T] ir-visitor = {:.1f}",
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t)
                   .count() /
               1000.0);
  if (_dbg_level)
    visitor->dump();
  return {std::move(visitor), input};
}

int runMode(const std::vector<const char *> &args) {
  llvm::cl::list<std::string> libs(
      "l", llvm::cl::desc("Load and link the specified library"));
  llvm::cl::list<std::string> seqArgs(llvm::cl::ConsumeAfter,
                                      llvm::cl::desc("<program arguments>..."));
  auto start_t = std::chrono::high_resolution_clock::now();
  auto result = processSource(args);
  if (!result.visitor)
    return EXIT_FAILURE;
  std::vector<std::string> libsVec(libs);
  std::vector<std::string> argsVec(seqArgs);
  argsVec.insert(argsVec.begin(), result.input);
  LOG_USER("compiler took: {:.2f} seconds",
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - start_t)
                   .count() /
               1000.0);
  result.visitor->run(argsVec, libsVec);
  return EXIT_SUCCESS;
}

int buildMode(const std::vector<const char *> &args) {
  llvm::cl::list<std::string> libs(
      "l", llvm::cl::desc("Link the specified library (only for executables)"));
  llvm::cl::opt<BuildKind> buildKind(
      llvm::cl::desc("output type"),
      llvm::cl::values(clEnumValN(LLVM, "llvm", "Generate LLVM IR"),
                       clEnumValN(Bitcode, "bc", "Generate LLVM bitcode"),
                       clEnumValN(Object, "obj", "Generate native object file"),
                       clEnumValN(Executable, "exe", "Generate executable"),
                       clEnumValN(Detect, "detect",
                                  "Detect output type based on output file extension")),
      llvm::cl::init(Detect));
  llvm::cl::opt<std::string> output(
      "o",
      llvm::cl::desc(
          "Write compiled output to specified file. Supported extensions: "
          "none (executable), .o (object file), .ll (LLVM IR), .bc (LLVM bitcode)"));

  auto result = processSource(args);
  if (!result.visitor)
    return EXIT_FAILURE;
  std::vector<std::string> libsVec(libs);

  if (output.empty() && result.input == "-")
    seq::compilationError("output file must be specified when reading from stdin");
  std::string extension;
  switch (buildKind) {
  case BuildKind::LLVM:
    extension = ".ll";
    break;
  case BuildKind::Bitcode:
    extension = ".bc";
    break;
  case BuildKind::Object:
    extension = ".o";
    break;
  case BuildKind::Executable:
  case BuildKind::Detect:
    extension = "";
    break;
  default:
    assert(0);
  }
  const std::string filename =
      output.empty() ? makeOutputFilename(result.input, extension) : output;
  switch (buildKind) {
  case BuildKind::LLVM:
    result.visitor->writeToLLFile(filename);
    break;
  case BuildKind::Bitcode:
    result.visitor->writeToBitcodeFile(filename);
    break;
  case BuildKind::Object:
    result.visitor->writeToObjectFile(filename);
    break;
  case BuildKind::Executable:
    result.visitor->writeToExecutable(filename, libsVec);
    break;
  case BuildKind::Detect:
    result.visitor->compile(filename, libsVec);
    break;
  default:
    assert(0);
  }

  return EXIT_SUCCESS;
}

void showCommandsAndExit() {
  seq::compilationError("Available commands: seqc <run|build|doc>");
}

int otherMode(const std::vector<const char *> &args) {
  llvm::cl::opt<std::string> input(llvm::cl::Positional, llvm::cl::desc("<mode>"));
  llvm::cl::extrahelp("\nMODES:\n\n"
                      "  run   - run a program interactively\n"
                      "  build - build a program\n"
                      "  doc   - generate program documentation\n");
  llvm::cl::ParseCommandLineOptions(args.size(), args.data());

  if (!input.empty())
    showCommandsAndExit();
  return EXIT_SUCCESS;
}

int main(int argc, const char **argv) {
  if (argc < 2)
    showCommandsAndExit();

  llvm::cl::SetVersionPrinter(versMsg);
  std::vector<const char *> args{argv[0]};
  for (int i = 2; i < argc; i++)
    args.push_back(argv[i]);

  std::string mode(argv[1]);
  std::string argv0 = std::string(args[0]) + " " + mode;
  if (mode == "run") {
    args[0] = argv0.data();
    return runMode(args);
  }
  if (mode == "build") {
    args[0] = argv0.data();
    return buildMode(args);
  }
  if (mode == "doc") {
    const char *oldArgv0 = args[0];
    args[0] = argv0.data();
    return docMode(args, oldArgv0);
  }
  return otherMode({argv, argv + argc});
}
