// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace codon {

struct Options {
  enum class GlobalCTORMode { No, Yes, Auto };

  /// `argv[0]` of executing program; used to load libraries
  std::string argv0;

  /// true if compiling in debug mode
  bool debug = true;

  /// true if running tests (internal use only)
  bool test = false;

  /// true if compiling in JIT mode
  bool jit = false;

  /// true if compiling to standalone object/executable
  bool standalone = false;

  /// true if compiling as a Python extension
  bool pyext = false;

  /// if true, don't register/run any Codon IR passes
  bool pmempty = false;

  /// true to capture program output during execution
  bool capture = false;

  /// true to enable target-native optimizations
  bool native = true;

  /// true if compiling with Python (vs. C) numerical semantics
  bool pynum = true;

  /// true to disable exceptions
  bool noexc = false;

  /// true if compiling with fast-math optimizations
  bool fastmath = false;

  /// true to enable automatic Python fallback of imports
  bool autopy = false;

  /// true to enable auto-free optimization
  bool autofree = false;

  /// true to use unordered dictionary implementation
  bool unordereddict = false;

  /// whether to generate main code in a global constructor
  GlobalCTORMode ctor = GlobalCTORMode::Auto;

  /// list of plugins to include during compilation
  std::vector<std::string> plugins;

  /// list "name=value" definitions to be included during compilation
  std::vector<std::string> defines;

  /// list of disabled IR optimizations (identified by pass key)
  std::vector<std::string> disabled;

  /// CUDA libdevice path
  std::string libdevice = "/usr/local/cuda/nvvm/libdevice/libdevice.10.bc";

  /// GPU name
  std::string gpuName = "sm_30";

  /// GPU features
  std::string gpuFeat = "+ptx42";

  /// PTX output file, or empty to not output intermediate PTX
  std::string gpuOutput;

  /// log flags (https://docs.exaloop.io/start/usage/#logging)
  std::string log;

  /// target architecture (e.g. x86_64, aarch64, etc.)
  std::string march;

  /// target CPU (e.g. skylake, cortex-a72, etc.)
  std::string mcpu;

  /// target features (e.g. +sse4.2, +neon, etc.)
  std::vector<std::string> mattrs;

  /// Get a default-options instance.
  /// @return default options instance
  static std::unique_ptr<Options> getDefault(const std::string &argv0);

  /// Get an options instance with values set from command-line flags.
  /// @return command-line options instance
  static std::unique_ptr<Options> getFromCommandLine(const std::string &argv0);
};

} // namespace codon
