// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "codon/cir/llvm/llvm.h"

namespace codon {
namespace ir {

void addNativeLLVMPasses(llvm::PassBuilder *pb);

} // namespace ir
} // namespace codon
