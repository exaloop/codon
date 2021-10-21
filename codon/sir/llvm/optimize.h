#pragma once

#include <memory>

#include "codon/dsl/plugins.h"
#include "codon/sir/llvm/llvm.h"

namespace codon {
namespace ir {
std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Triple triple, llvm::StringRef cpuStr,
                 llvm::StringRef featuresStr, const llvm::TargetOptions &options);

std::unique_ptr<llvm::TargetMachine>
getTargetMachine(llvm::Module *module, bool setFunctionAttributes = false);

void optimize(llvm::Module *module, bool debug, PluginManager *plugins = nullptr);
} // namespace ir
} // namespace codon
