// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>
#include <vector>

namespace codon {
namespace jit {

class JIT;

struct JITResult {
  void *result;
  std::string message;

  operator bool() const { return message.empty(); }
  static JITResult success(void *result) { return {result, ""}; }
  static JITResult error(const std::string &message) { return {nullptr, message}; }
};

JIT *jitInit(const std::string &name);

JITResult jitExecutePython(JIT *jit, const std::string &name,
                           const std::vector<std::string> &types,
                           const std::string &pyModule,
                           const std::vector<std::string> &pyVars, void *arg,
                           bool debug);

JITResult jitExecuteSafe(JIT *jit, const std::string &code, const std::string &file,
                         int line, bool debug);

std::string getJITLibrary();

} // namespace jit
} // namespace codon
