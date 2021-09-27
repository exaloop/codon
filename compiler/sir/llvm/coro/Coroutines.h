#pragma once

namespace llvm {
class Pass;
class PassManagerBuilder;
} // namespace llvm

namespace seq {
namespace coro {

/// Add all coroutine passes to appropriate extension points.
void addCoroutinePassesToExtensionPoints(llvm::PassManagerBuilder &Builder);

/// Lower coroutine intrinsics that are not needed by later passes.
llvm::Pass *createCoroEarlyLegacyPass();

/// Split up coroutines into multiple functions driving their state machines.
llvm::Pass *createCoroSplitLegacyPass(bool IsOptimizing = false);

/// Analyze coroutines use sites, devirtualize resume/destroy calls and elide
/// heap allocation for coroutine frame where possible.
llvm::Pass *createCoroElideLegacyPass();

/// Lower all remaining coroutine intrinsics.
llvm::Pass *createCoroCleanupLegacyPass();

} // namespace coro
} // namespace seq
