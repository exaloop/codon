// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <sstream>
#include <string>

#include "codon/cir/llvm/llvm.h"

namespace codon {
namespace ir {

class Target {
public:
  virtual ~Target() {}
  virtual std::string getCPU(const llvm::Triple &triple) const = 0;
  virtual std::string getFeatures(const llvm::Triple &triple) const = 0;
};

} // namespace ir
} // namespace codon
