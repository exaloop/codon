// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "memory_manager.h"

#include "codon/runtime/lib.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"

#include <algorithm>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace codon {

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
  if (auto PageSize = llvm::sys::Process::getPageSize()) {
    if (!llvm::isPowerOf2_64((uint64_t)*PageSize))
      return llvm::make_error<llvm::StringError>("Page size is not a power of 2",
                                                 llvm::inconvertibleErrorCode());
    return std::make_unique<BoehmGCJITLinkMemoryManager>(*PageSize);
  } else {
    return PageSize.takeError();
  }
}

#ifdef _WIN32
/// Allocate a JIT slab in the 4GB window [handlerFloor, handler] so that COFF
/// .xdata Pointer32NB relocations (value = target - __ImageBase, which we anchor
/// at the 4GB-aligned floor below __C_specific_handler) stay within uint32 range
/// for both the handler reference and references into the slab itself. Searches
/// downward from just below the handler and falls back to an unconstrained
/// allocation if nothing in range is free.
static llvm::sys::MemoryBlock
allocateNearImage(size_t size, llvm::sys::Memory::ProtectionFlags prot,
                  std::error_code &ec) {
  uintptr_t handler = 0;
  if (HMODULE crt = GetModuleHandleW(L"vcruntime140.dll"))
    handler = reinterpret_cast<uintptr_t>(GetProcAddress(crt, "__C_specific_handler"));

  if (handler) {
    uintptr_t floor = handler & ~0xFFFFFFFFull;
    const uintptr_t step = 16ull * 1024 * 1024;
    uintptr_t top = handler - size;
    for (uintptr_t cand = top & ~(step - 1); cand >= floor; cand -= step) {
      llvm::sys::MemoryBlock hint(reinterpret_cast<void *>(cand), size);
      std::error_code localEC;
      auto block =
          llvm::sys::Memory::allocateMappedMemory(size, &hint, prot, localEC);
      if (!localEC && block.base()) {
        auto b = reinterpret_cast<uintptr_t>(block.base());
        if (b >= floor && (b + size) <= (floor + 0x100000000ull)) {
          ec = std::error_code();
          return block;
        }
        llvm::sys::Memory::releaseMappedMemory(block);
      }
      if (cand < floor + step)
        break;
    }
  }

  return llvm::sys::Memory::allocateMappedMemory(size, nullptr, prot, ec);
}
#endif

void BoehmGCJITLinkMemoryManager::allocate(const llvm::jitlink::JITLinkDylib *JD,
                                           llvm::jitlink::LinkGraph &G,
                                           OnAllocatedFunction OnAllocated) {
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
#ifdef _WIN32
    Slab = allocateNearImage(SegsSizes->total(), ReadWrite, EC);
#else
    Slab = llvm::sys::Memory::allocateMappedMemory(SegsSizes->total(), nullptr,
                                                   ReadWrite, EC);
#endif

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

    auto &SegAddr = (AG.getMemLifetime() == llvm::orc::MemLifetime::Standard)
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

#ifdef _WIN32
namespace {
/// JITLink plugin that registers the `.pdata` (RUNTIME_FUNCTION table) of each
/// JIT-compiled object with the OS unwinder via RtlAddFunctionTable. The RVAs in
/// `.pdata`/`.xdata` are emitted relative to our `__ImageBase` anchor (the
/// 4GB-aligned floor below __C_specific_handler), so that same value is used as
/// the table's base address. Without this, raised SEH exceptions cannot unwind
/// through JIT'd funclet scopes.
class Win64SEHRegistrationPlugin : public llvm::orc::ObjectLinkingLayer::Plugin {
  uint64_t imageBase;

public:
  Win64SEHRegistrationPlugin() {
    uintptr_t handler = 0;
    if (HMODULE crt = GetModuleHandleW(L"vcruntime140.dll"))
      handler =
          reinterpret_cast<uintptr_t>(GetProcAddress(crt, "__C_specific_handler"));
    imageBase = handler ? (handler & ~0xFFFFFFFFull)
                        : reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
  }

  void modifyPassConfig(llvm::orc::MaterializationResponsibility &,
                        llvm::jitlink::LinkGraph &,
                        llvm::jitlink::PassConfiguration &Config) override {
    uint64_t base = imageBase;
    Config.PostFixupPasses.push_back(
        [base](llvm::jitlink::LinkGraph &G) -> llvm::Error {
          auto *sec = G.findSectionByName(".pdata");
          if (!sec)
            return llvm::Error::success();
          uint64_t lo = ~uint64_t(0), hi = 0;
          for (auto *B : sec->blocks()) {
            uint64_t a = B->getAddress().getValue();
            if (a < lo)
              lo = a;
            if (a + B->getSize() > hi)
              hi = a + B->getSize();
          }
          if (lo >= hi)
            return llvm::Error::success();
          auto count = static_cast<DWORD>((hi - lo) / sizeof(RUNTIME_FUNCTION));
          if (count && !RtlAddFunctionTable(reinterpret_cast<PRUNTIME_FUNCTION>(lo),
                                            count, base))
            return llvm::make_error<llvm::StringError>(
                "RtlAddFunctionTable failed for JIT'd .pdata",
                llvm::inconvertibleErrorCode());
          return llvm::Error::success();
        });
  }

  llvm::Error notifyFailed(llvm::orc::MaterializationResponsibility &) override {
    return llvm::Error::success();
  }
  llvm::Error notifyRemovingResources(llvm::orc::JITDylib &,
                                      llvm::orc::ResourceKey) override {
    return llvm::Error::success();
  }
  void notifyTransferringResources(llvm::orc::JITDylib &, llvm::orc::ResourceKey,
                                   llvm::orc::ResourceKey) override {}
};
} // namespace

void addWin64SEHRegistration(llvm::orc::ObjectLinkingLayer &layer) {
  layer.addPlugin(std::make_unique<Win64SEHRegistrationPlugin>());
}
#endif

} // namespace codon
