// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>

#include "codon/cir/llvm/llvm.h"
#include "codon/dsl/plugins.h"

namespace codon {
namespace ir {
std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Triple triple, llvm::StringRef cpuStr,
                 llvm::StringRef featuresStr, const llvm::TargetOptions &options,
                 bool pic = false);

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Module *module, bool setFunctionAttributes = false,
                 bool pic = false);

void optimize(llvm::Module *module, bool debug, bool jit = false,
              PluginManager *plugins = nullptr);
} // namespace ir
} // namespace codon
