// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"
#include "hwy/contrib/sort/vqsort-inl.h"

#include <algorithm>
#include <type_traits>

namespace {
template <typename T> void sortPrimitive(T *data, int64_t size) {
#ifdef __APPLE__
  // macOS ARM64 benchmarks with 100M random/duplicate-heavy elements favored
  // libc++ for these 64-bit types and Highway for 16/32-bit types.
  if constexpr (std::is_same_v<T, double> || std::is_same_v<T, int64_t> ||
                std::is_same_v<T, uint64_t>) {
    if constexpr (std::is_same_v<T, double>) {
      // NaNs violate std::sort's default ordering; Highway places them last.
      // This O(n) scan retains libc++'s specialized default-comparator path.
      // Signed zeros compare equivalent and do not require the fallback.
      // libc++ sort is significantly faster even with the initial scan.
      if (std::any_of(data, data + size, [](T value) { return value != value; })) {
        hwy::VQSort(data, size, hwy::SortAscending());
        return;
      }
    }
    std::sort(data, data + size);
    return;
  }
#endif
  hwy::VQSort(data, size, hwy::SortAscending());
}
} // namespace

SEQ_FUNC void cnp_sort_int16(int16_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_uint16(uint16_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_int32(int32_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_uint32(uint32_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_int64(int64_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_uint64(uint64_t *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_float32(float *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_float64(double *data, int64_t n) { sortPrimitive(data, n); }

SEQ_FUNC void cnp_sort_uint128(hwy::uint128_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}
