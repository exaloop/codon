// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace {
// LLVM's emulated TLS control ABI: size, alignment, runtime index, initializer.
struct TLSControl {
  uintptr_t size;
  uintptr_t alignment;
  uintptr_t index;
  void *initializer;
};

struct TLSAllocation {
  void *storage;
  void *address;
};

struct TLSValues {
  std::unordered_map<uintptr_t, TLSAllocation> values;
  ~TLSValues() {
    for (auto &[index, allocation] : values)
      seq_free(allocation.storage);
  }
};

std::atomic<uintptr_t> nextTLSIndex{1};
thread_local TLSValues threadValues;
} // namespace

SEQ_FUNC void *__emutls_get_address(TLSControl *control) {
  std::atomic_ref<uintptr_t> slot(control->index);
  auto index = slot.load(std::memory_order_acquire);
  if (!index) {
    auto candidate = nextTLSIndex.fetch_add(1, std::memory_order_relaxed);
    if (slot.compare_exchange_strong(index, candidate, std::memory_order_release,
                                     std::memory_order_acquire))
      index = candidate;
  }
  if (auto found = threadValues.values.find(index); found != threadValues.values.end())
    return found->second.address;

  // Uncollectable GC memory keeps TLS values and the objects they reference alive
  // even though the C++ container itself is outside the GC heap.
  auto alignment = control->alignment ? control->alignment : uintptr_t(1);
  auto *storage = seq_alloc_uncollectable(control->size + alignment - 1);
  if (!storage)
    std::abort();
  auto address =
      (reinterpret_cast<uintptr_t>(storage) + alignment - 1) & ~(alignment - 1);
  auto *data = reinterpret_cast<void *>(address);
  if (control->initializer)
    std::memcpy(data, control->initializer, control->size);
  else
    std::memset(data, 0, control->size);
  threadValues.values.emplace(index, TLSAllocation{storage, data});
  return data;
}
