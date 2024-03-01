// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <mutex>
#include <utility>
#include <vector>

#include "codon/cir/llvm/llvm.h"

namespace codon {

/// Simple extension of LLVM's SectionMemoryManager which catches data section
/// allocations and registers them with the GC. This allows the GC to know not
/// to collect globals even in JIT mode.
class BoehmGCMemoryManager : public llvm::SectionMemoryManager {
private:
  /// Vector of (start, end) address pairs registered with GC.
  std::vector<std::pair<void *, void *>> roots;
  uint8_t *allocateDataSection(uintptr_t size, unsigned alignment, unsigned sectionID,
                               llvm::StringRef sectionName, bool isReadOnly) override;

public:
  BoehmGCMemoryManager();
  ~BoehmGCMemoryManager() override;
};

/// Basically a copy of LLVM's jitlink::InProcessMemoryManager that registers
/// relevant allocated sections with the GC. TODO: Avoid copying this entire
/// class if/when there's an API to perform the registration externally.
class BoehmGCJITLinkMemoryManager : public llvm::jitlink::JITLinkMemoryManager {
public:
  class IPInFlightAlloc;

  /// Attempts to auto-detect the host page size.
  static llvm::Expected<std::unique_ptr<BoehmGCJITLinkMemoryManager>> Create();

  /// Create an instance using the given page size.
  BoehmGCJITLinkMemoryManager(uint64_t PageSize) : PageSize(PageSize) {}

  void allocate(const llvm::jitlink::JITLinkDylib *JD, llvm::jitlink::LinkGraph &G,
                OnAllocatedFunction OnAllocated) override;

  // Use overloads from base class.
  using llvm::jitlink::JITLinkMemoryManager::allocate;

  void deallocate(std::vector<FinalizedAlloc> Alloc,
                  OnDeallocatedFunction OnDeallocated) override;

  // Use overloads from base class.
  using llvm::jitlink::JITLinkMemoryManager::deallocate;

private:
  // FIXME: Use an in-place array instead of a vector for DeallocActions.
  //        There shouldn't need to be a heap alloc for this.
  struct FinalizedAllocInfo {
    llvm::sys::MemoryBlock StandardSegments;
    std::vector<llvm::orc::shared::WrapperFunctionCall> DeallocActions;
  };

  FinalizedAlloc createFinalizedAlloc(
      llvm::sys::MemoryBlock StandardSegments,
      std::vector<llvm::orc::shared::WrapperFunctionCall> DeallocActions);

  uint64_t PageSize;
  std::mutex FinalizedAllocsMutex;
  llvm::RecyclingAllocator<llvm::BumpPtrAllocator, FinalizedAllocInfo>
      FinalizedAllocInfos;
};

} // namespace codon
