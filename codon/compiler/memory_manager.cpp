// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "memory_manager.h"

#include "codon/runtime/lib.h"

namespace codon {

BoehmGCMemoryManager::BoehmGCMemoryManager() : SectionMemoryManager(), roots() {}

uint8_t *BoehmGCMemoryManager::allocateDataSection(uintptr_t size, unsigned alignment,
                                                   unsigned sectionID,
                                                   llvm::StringRef sectionName,
                                                   bool isReadOnly) {
  auto *result = SectionMemoryManager::allocateDataSection(size, alignment, sectionID,
                                                           sectionName, isReadOnly);
  void *start = result;
  void *end = result + size;
  seq_gc_add_roots(start, end);
  roots.emplace_back(start, end);
  return result;
}

BoehmGCMemoryManager::~BoehmGCMemoryManager() {
  for (const auto &root : roots) {
    seq_gc_remove_roots(root.first, root.second);
  }
}

class BoehmGCJITLinkMemoryManager::IPInFlightAlloc
    : public llvm::jitlink::JITLinkMemoryManager::InFlightAlloc {
public:
  IPInFlightAlloc(BoehmGCJITLinkMemoryManager &MemMgr, llvm::jitlink::LinkGraph &G,
                  llvm::jitlink::BasicLayout BL,
                  llvm::sys::MemoryBlock StandardSegments,
                  llvm::sys::MemoryBlock FinalizationSegments)
      : MemMgr(MemMgr), G(G), BL(std::move(BL)),
        StandardSegments(std::move(StandardSegments)),
        FinalizationSegments(std::move(FinalizationSegments)) {}

  void finalize(OnFinalizedFunction OnFinalized) override {

    // Apply memory protections to all segments.
    if (auto Err = applyProtections()) {
      OnFinalized(std::move(Err));
      return;
    }

    // Run finalization actions.
    auto DeallocActions = runFinalizeActions(G.allocActions());
    if (!DeallocActions) {
      OnFinalized(DeallocActions.takeError());
      return;
    }

    // Release the finalize segments slab.
    if (auto EC = llvm::sys::Memory::releaseMappedMemory(FinalizationSegments)) {
      OnFinalized(llvm::errorCodeToError(EC));
      return;
    }

    // Continue with finalized allocation.
    OnFinalized(MemMgr.createFinalizedAlloc(std::move(StandardSegments),
                                            std::move(*DeallocActions)));
  }

  void abandon(OnAbandonedFunction OnAbandoned) override {
    llvm::Error Err = llvm::Error::success();
    if (auto EC = llvm::sys::Memory::releaseMappedMemory(FinalizationSegments))
      Err = llvm::joinErrors(std::move(Err), llvm::errorCodeToError(EC));
    if (auto EC = llvm::sys::Memory::releaseMappedMemory(StandardSegments))
      Err = llvm::joinErrors(std::move(Err), llvm::errorCodeToError(EC));
    OnAbandoned(std::move(Err));
  }

private:
  llvm::Error applyProtections() {
    for (auto &KV : BL.segments()) {
      const auto &AG = KV.first;
      auto &Seg = KV.second;

      auto Prot = toSysMemoryProtectionFlags(AG.getMemProt());

      uint64_t SegSize =
          llvm::alignTo(Seg.ContentSize + Seg.ZeroFillSize, MemMgr.PageSize);
      llvm::sys::MemoryBlock MB(Seg.WorkingMem, SegSize);
      if (auto EC = llvm::sys::Memory::protectMappedMemory(MB, Prot))
        return llvm::errorCodeToError(EC);
      if (Prot & llvm::sys::Memory::MF_EXEC)
        llvm::sys::Memory::InvalidateInstructionCache(MB.base(), MB.allocatedSize());
    }
    return llvm::Error::success();
  }

  BoehmGCJITLinkMemoryManager &MemMgr;
  llvm::jitlink::LinkGraph &G;
  llvm::jitlink::BasicLayout BL;
  llvm::sys::MemoryBlock StandardSegments;
  llvm::sys::MemoryBlock FinalizationSegments;
};

llvm::Expected<std::unique_ptr<BoehmGCJITLinkMemoryManager>>
BoehmGCJITLinkMemoryManager::Create() {
  if (auto PageSize = llvm::sys::Process::getPageSize())
    return std::make_unique<BoehmGCJITLinkMemoryManager>(*PageSize);
  else
    return PageSize.takeError();
}

void BoehmGCJITLinkMemoryManager::allocate(const llvm::jitlink::JITLinkDylib *JD,
                                           llvm::jitlink::LinkGraph &G,
                                           OnAllocatedFunction OnAllocated) {

  // FIXME: Just check this once on startup.
  if (!llvm::isPowerOf2_64((uint64_t)PageSize)) {
    OnAllocated(llvm::make_error<llvm::StringError>("Page size is not a power of 2",
                                                    llvm::inconvertibleErrorCode()));
    return;
  }

  llvm::jitlink::BasicLayout BL(G);

  /// Scan the request and calculate the group and total sizes.
  /// Check that segment size is no larger than a page.
  auto SegsSizes = BL.getContiguousPageBasedLayoutSizes(PageSize);
  if (!SegsSizes) {
    OnAllocated(SegsSizes.takeError());
    return;
  }

  /// Check that the total size requested (including zero fill) is not larger
  /// than a size_t.
  if (SegsSizes->total() > std::numeric_limits<size_t>::max()) {
    OnAllocated(llvm::make_error<llvm::jitlink::JITLinkError>(
        "Total requested size " + llvm::formatv("{0:x}", SegsSizes->total()) +
        " for graph " + G.getName() + " exceeds address space"));
    return;
  }

  // Allocate one slab for the whole thing (to make sure everything is
  // in-range), then partition into standard and finalization blocks.
  //
  // FIXME: Make two separate allocations in the future to reduce
  // fragmentation: finalization segments will usually be a single page, and
  // standard segments are likely to be more than one page. Where multiple
  // allocations are in-flight at once (likely) the current approach will leave
  // a lot of single-page holes.
  llvm::sys::MemoryBlock Slab;
  llvm::sys::MemoryBlock StandardSegsMem;
  llvm::sys::MemoryBlock FinalizeSegsMem;
  {
    const llvm::sys::Memory::ProtectionFlags ReadWrite =
        static_cast<llvm::sys::Memory::ProtectionFlags>(llvm::sys::Memory::MF_READ |
                                                        llvm::sys::Memory::MF_WRITE);

    std::error_code EC;
    Slab = llvm::sys::Memory::allocateMappedMemory(SegsSizes->total(), nullptr,
                                                   ReadWrite, EC);

    if (EC) {
      OnAllocated(llvm::errorCodeToError(EC));
      return;
    }

    // Zero-fill the whole slab up-front.
    memset(Slab.base(), 0, Slab.allocatedSize());

    StandardSegsMem = {Slab.base(), static_cast<size_t>(SegsSizes->StandardSegs)};
    FinalizeSegsMem = {(void *)((char *)Slab.base() + SegsSizes->StandardSegs),
                       static_cast<size_t>(SegsSizes->FinalizeSegs)};
  }

  auto NextStandardSegAddr = llvm::orc::ExecutorAddr::fromPtr(StandardSegsMem.base());
  auto NextFinalizeSegAddr = llvm::orc::ExecutorAddr::fromPtr(FinalizeSegsMem.base());

  // Build ProtMap, assign addresses.
  for (auto &KV : BL.segments()) {
    auto &AG = KV.first;
    auto &Seg = KV.second;

    auto &SegAddr =
        (AG.getMemLifetimePolicy() == llvm::orc::MemLifetimePolicy::Standard)
            ? NextStandardSegAddr
            : NextFinalizeSegAddr;

    Seg.WorkingMem = SegAddr.toPtr<char *>();
    Seg.Addr = SegAddr;

    SegAddr += llvm::alignTo(Seg.ContentSize + Seg.ZeroFillSize, PageSize);

    if (static_cast<int>(AG.getMemProt()) &
        static_cast<int>(llvm::orc::MemProt::Write)) {
      seq_gc_add_roots((void *)Seg.Addr.getValue(), (void *)SegAddr.getValue());
    }
  }

  if (auto Err = BL.apply()) {
    OnAllocated(std::move(Err));
    return;
  }

  OnAllocated(std::make_unique<IPInFlightAlloc>(
      *this, G, std::move(BL), std::move(StandardSegsMem), std::move(FinalizeSegsMem)));
}

void BoehmGCJITLinkMemoryManager::deallocate(std::vector<FinalizedAlloc> Allocs,
                                             OnDeallocatedFunction OnDeallocated) {
  std::vector<llvm::sys::MemoryBlock> StandardSegmentsList;
  std::vector<std::vector<llvm::orc::shared::WrapperFunctionCall>> DeallocActionsList;

  {
    std::lock_guard<std::mutex> Lock(FinalizedAllocsMutex);
    for (auto &Alloc : Allocs) {
      auto *FA = Alloc.release().toPtr<FinalizedAllocInfo *>();
      StandardSegmentsList.push_back(std::move(FA->StandardSegments));
      if (!FA->DeallocActions.empty())
        DeallocActionsList.push_back(std::move(FA->DeallocActions));
      FA->~FinalizedAllocInfo();
      FinalizedAllocInfos.Deallocate(FA);
    }
  }

  llvm::Error DeallocErr = llvm::Error::success();

  while (!DeallocActionsList.empty()) {
    auto &DeallocActions = DeallocActionsList.back();
    auto &StandardSegments = StandardSegmentsList.back();

    /// Run any deallocate calls.
    while (!DeallocActions.empty()) {
      if (auto Err = DeallocActions.back().runWithSPSRetErrorMerged())
        DeallocErr = llvm::joinErrors(std::move(DeallocErr), std::move(Err));
      DeallocActions.pop_back();
    }

    /// Release the standard segments slab.
    if (auto EC = llvm::sys::Memory::releaseMappedMemory(StandardSegments))
      DeallocErr = llvm::joinErrors(std::move(DeallocErr), llvm::errorCodeToError(EC));

    DeallocActionsList.pop_back();
    StandardSegmentsList.pop_back();
  }

  OnDeallocated(std::move(DeallocErr));
}

llvm::jitlink::JITLinkMemoryManager::FinalizedAlloc
BoehmGCJITLinkMemoryManager::createFinalizedAlloc(
    llvm::sys::MemoryBlock StandardSegments,
    std::vector<llvm::orc::shared::WrapperFunctionCall> DeallocActions) {
  std::lock_guard<std::mutex> Lock(FinalizedAllocsMutex);
  auto *FA = FinalizedAllocInfos.Allocate<FinalizedAllocInfo>();
  new (FA) FinalizedAllocInfo({std::move(StandardSegments), std::move(DeallocActions)});
  return FinalizedAlloc(llvm::orc::ExecutorAddr::fromPtr(FA));
}

} // namespace codon
