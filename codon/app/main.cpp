// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <vector>

#include "codon/app/cli.h"
#include "codon/util/jupyter.h"
#include "llvm/Support/CommandLine.h"

// Thin `codon` executable shim. The compile driver (run/build/doc/jit + all
// codon-specific flag parsing) lives in codonc via codon::cliMain(), so that
// `ParseCommandLineOptions` runs in the same binary as the `cl::opt` globals it
// reads. On Windows LLVM is statically linked into both this exe and codonc.dll,
// giving each its own llvm::cl registry; routing the driver through codonc keeps
// every compile flag in one registry (otherwise the exe rejects -release etc.
// with "Unknown command line argument").
//
// `jupyter` is the one mode handled here: it depends on codon_jupyter (not on
// codonc's compile flags) and only uses options local to this binary, so its
// self-contained parse in the exe registry is fine — and keeping it out of
// codonc avoids a codonc<->codon_jupyter link dependency.

namespace {
int jupyterMode(const std::vector<const char *> &args) {
  llvm::cl::list<std::string> plugins("plugin",
                                      llvm::cl::desc("Load specified plugin"));
  llvm::cl::opt<std::string> input(llvm::cl::Positional,
                                   llvm::cl::desc("<connection file>"),
                                   llvm::cl::init("connection.json"));
  llvm::cl::ParseCommandLineOptions(args.size(), args.data());
  return codon::startJupyterKernel(args[0], plugins, input);
}
} // namespace

int main(int argc, const char **argv) {
  if (argc > 1 && std::string(argv[1]) == "jupyter") {
    std::vector<const char *> args{argv[0]};
    for (int i = 2; i < argc; i++)
      args.push_back(argv[i]);
    std::string argv0 = std::string(argv[0]) + " jupyter";
    args[0] = argv0.data();
    return jupyterMode(args);
  }
  return codon::cliMain(argc, argv);
}
