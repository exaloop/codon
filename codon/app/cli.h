// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

namespace codon {

// Main Codon CLI driver. Implemented in codon/app/cli.cpp, which is compiled into
// codonc so that command-line parsing shares the same llvm::cl registry as the
// `cl::opt` flag globals (options.cpp, numpy.cpp). The `codon` executable
// (codon/app/main.cpp) forwards here for every mode except `jupyter`.
int cliMain(int argc, const char **argv);

} // namespace codon
