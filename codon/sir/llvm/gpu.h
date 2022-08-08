#pragma once

#include "codon/sir/llvm/llvm.h"

namespace codon {
namespace ir {

void applyGPUTransformations(llvm::Module *module);

} // namespace ir
} // namespace codon
