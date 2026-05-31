// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "options.h"

#include "llvm/Support/CommandLine.h"

namespace codon {
namespace {
enum OptModeEnum { Debug, Release };
enum NumericsEnum { C, Python };

llvm::cl::opt<OptModeEnum>
    OptMode(llvm::cl::desc("optimization mode"),
            llvm::cl::values(
                clEnumValN(Debug, "debug",
                           "Turn off compiler optimizations and show backtraces"),
                clEnumValN(Release, "release",
                           "Turn on compiler optimizations and disable debug info")),
            llvm::cl::init(Debug));

llvm::cl::opt<bool> DisableExceptions("disable-exceptions",
                                      llvm::cl::desc("Disable exception handling"),
                                      llvm::cl::init(false));

llvm::cl::opt<bool>
    AutoFree("auto-free",
             llvm::cl::desc("Insert free() calls on allocated memory automatically"),
             llvm::cl::init(false), llvm::cl::Hidden);

llvm::cl::opt<bool> FastMath("fast-math",
                             llvm::cl::desc("Apply fastmath optimizations"),
                             llvm::cl::init(false));

llvm::cl::opt<bool>
    DisableNative("disable-native",
                  llvm::cl::desc("Disable architecture-specific optimizations"),
                  llvm::cl::init(false));

llvm::cl::opt<bool> AutoPython(
    "auto-python",
    llvm::cl::desc("Automatically fall back to Python when importing modules"),
    llvm::cl::init(false));

llvm::cl::list<std::string> Defines(
    "D", llvm::cl::Prefix,
    llvm::cl::desc("Add static variable definitions. The syntax is <name>=<value>"));

llvm::cl::list<std::string>
    DisabledOpts("disable-opt",
                 llvm::cl::desc("Disable the specified IR optimization"));

llvm::cl::list<std::string> Plugins("plugin", llvm::cl::desc("Load specified plugin"));

llvm::cl::opt<std::string> Log("log", llvm::cl::desc("Enable given log streams"));

llvm::cl::opt<NumericsEnum> Numerics(
    "numerics", llvm::cl::desc("numerical semantics"),
    llvm::cl::values(
        clEnumValN(C, "c", "C semantics: best performance but deviates from Python"),
        clEnumValN(Python, "py",
                   "Python semantics: mirrors Python but might disable optimizations "
                   "like vectorization")),
    llvm::cl::init(Python));

llvm::cl::opt<std::string>
    LibDevice("libdevice", llvm::cl::desc("libdevice path for GPU kernels"),
              llvm::cl::init("/usr/local/cuda/nvvm/libdevice/libdevice.10.bc"));

llvm::cl::opt<std::string> GpuName(
    "gpu-name",
    llvm::cl::desc(
        "Target GPU architecture or compute capability (e.g. sm_70, sm_80, etc.)"),
    llvm::cl::init("sm_30"));

llvm::cl::opt<std::string> GpuFeatures(
    "gpu-features",
    llvm::cl::desc("GPU feature flags passed (e.g. +ptx42 to enable PTX 4.2 features)"),
    llvm::cl::init("+ptx42"));

llvm::cl::opt<std::string> PTXOutput("ptx",
                                     llvm::cl::desc("Output PTX to specified file"));

llvm::cl::opt<bool>
    UnorderedDict("unordered-dict",
                  llvm::cl::desc("Use unordered dictionary implementation"),
                  llvm::cl::init(false));

llvm::cl::opt<Options::GlobalCTORMode> GlobalCTOR(
    "global-ctor", llvm::cl::desc("generate global constructor with main code"),
    llvm::cl::values(clEnumValN(Options::GlobalCTORMode::No, "no",
                                "Keep main code in main() function"),
                     clEnumValN(Options::GlobalCTORMode::Yes, "yes",
                                "Put main code in global constructor"),
                     clEnumValN(Options::GlobalCTORMode::Auto, "auto",
                                "'yes' if shared library output, 'no' otherwise")),
    llvm::cl::init(Options::GlobalCTORMode::Auto));

void apply(Options *opt) {
  opt->debug = (OptMode == Debug);
  opt->native = !DisableNative;
  opt->pynum = (Numerics == Python);
  opt->noexc = DisableExceptions;
  opt->fastmath = FastMath;
  opt->autofree = AutoFree;
  opt->autopy = AutoPython;
  opt->unordereddict = UnorderedDict;
  opt->ctor = GlobalCTOR;
  opt->plugins.assign(Plugins.begin(), Plugins.end());
  opt->defines.assign(Defines.begin(), Defines.end());
  opt->disabled.assign(DisabledOpts.begin(), DisabledOpts.end());
  opt->libdevice = LibDevice;
  opt->gpuName = GpuName;
  opt->gpuFeat = GpuFeatures;
  opt->gpuOutput = PTXOutput;
  opt->log = Log;
}
} // namespace

std::unique_ptr<Options> Options::getDefault(const std::string &argv0) {
  auto opt = std::make_unique<Options>();
  opt->argv0 = argv0;
  return std::move(opt);
}

std::unique_ptr<Options> Options::getFromCommandLine(const std::string &argv0) {
  auto opt = getDefault(argv0);
  apply(opt.get());
  return std::move(opt);
}

} // namespace codon
