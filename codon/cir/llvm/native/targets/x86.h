// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/llvm/native/targets/target.h"

namespace codon {
namespace ir {

class X86 : public Target {
public:
  std::string getCPU(const llvm::Triple &triple) const override;
  std::string getFeatures(const llvm::Triple &triple) const override;
};

} // namespace ir
} // namespace codon
