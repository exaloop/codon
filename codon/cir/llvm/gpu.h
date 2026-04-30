// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>

#include "codon/cir/llvm/llvm.h"

namespace codon {
namespace ir {

/// Applies GPU-specific transformations to the prepared GPU module, generates
/// PTX for kernel functions, and patches the original module accordingly.
/// @param module Original LLVM module to patch with generated GPU artifacts.
/// @param clone GPU-targeted module produced by prepareGPUmodule(); ownership is
/// consumed.
/// @param ptxFilename Filename for output PTX code; empty to derive it from the module.

void applyGPUTransformations(llvm::Module *module, std::unique_ptr<llvm::Module> clone,
                             const std::string &ptxFilename = "");

/// Creates a GPU-targeted clone of the given LLVM module.
/// The cloned module is configured with the GPU target triple and data layout
/// so it can go through the GPU-specific optimization pipeline.
/// @param module LLVM module containing both host code and GPU kernel functions.
/// @return A cloned module configured for GPU lowering.
std::unique_ptr<llvm::Module> prepareGPUmodule(llvm::Module *module);
} // namespace ir
} // namespace codon
