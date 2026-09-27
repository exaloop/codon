// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"

#if (defined(__aarch64__) || defined(__arm64__)) && !defined(__ARM_FEATURE_SVE)
#define HWY_DISABLED_TARGETS HWY_ALL_SVE
#endif

#include "hwy/contrib/sort/vqsort.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "codon/runtime/numpy/sort.cpp"
#include "hwy/contrib/sort/vqsort-inl.h"
#include "hwy/foreach_target.h"

HWY_BEFORE_NAMESPACE();
namespace {
namespace HWY_NAMESPACE {
namespace hn = hwy::HWY_NAMESPACE;

template <typename T> void sortPreservingZeros(T *data, size_t size) {
  if (size < 2)
    return;
  const hn::CappedTag<T, 64 / sizeof(T)> lanes;
  const hn::RebindToUnsigned<decltype(lanes)> bits;
  const hn::detail::MakeTraits<T, hwy::SortAscending> traits;
  const size_t width = hn::Lanes(lanes);
  size_t negativeZeros = 0;
  size_t nanCount = 0;
  size_t index = 0;
  for (; index + width <= size; index += width) {
    const auto values = hn::LoadU(lanes, data + index);
    const auto nans = hn::IsNaN(values);
    const auto negatives = hn::Eq(hn::BitCast(bits, values), hn::SignBit(bits));
    negativeZeros += hn::CountTrue(bits, negatives);
    nanCount += hn::CountTrue(lanes, nans);
    const auto replace = hn::Or(nans, hn::RebindMask(lanes, negatives));
    hn::StoreU(hn::IfThenElse(replace,
                              hn::IfThenElse(nans, hn::Inf(lanes), hn::Zero(lanes)),
                              values),
               lanes, data + index);
  }
  for (; index < size; ++index) {
    const T value = data[index];
    if (value != value) {
      ++nanCount;
      data[index] = std::numeric_limits<T>::infinity();
    } else if (value == 0) {
      negativeZeros += std::signbit(value);
      data[index] = 0;
    }
  }
  HWY_ALIGN T buffer[hwy::SortConstants::BufBytes<T, 1>(HWY_MAX_BYTES) / sizeof(T)];
#if VQSORT_ENABLED
  if (!hn::detail::HandleSpecialCases(lanes, traits, data, size, buffer)) {
    hn::detail::Recurse<hn::detail::RecurseMode::kSort>(
        lanes, traits, data, size, buffer, hwy::detail::GetGeneratorStateStatic(), 50);
  }
#else
  hn::detail::HeapSort(traits, data, size);
#endif
  if (negativeZeros) {
    T *zeros = std::lower_bound(data, data + size - nanCount, T(0));
    hn::Fill(lanes, T(-0.0), negativeZeros, zeros);
  }
  if (nanCount)
    hn::Fill(lanes, hn::GetLane(hn::NaN(lanes)), nanCount, data + size - nanCount);
}

void sortListFloat32(float *data, size_t size) { sortPreservingZeros(data, size); }

#ifndef __APPLE__
void sortListFloat64(double *data, size_t size) { sortPreservingZeros(data, size); }
#endif
} // namespace HWY_NAMESPACE
} // namespace
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace {
HWY_EXPORT(sortListFloat32);
#ifndef __APPLE__
HWY_EXPORT(sortListFloat64);
#endif

template <typename T> void sortPrimitive(T *data, int64_t size) {
#ifdef __APPLE__
  // macOS ARM64 benchmarks with 100M random/duplicate-heavy elements favored
  // libc++ for these 64-bit types and Highway for 16/32-bit types.
  if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
    std::sort(data, data + size);
    return;
  }
#endif
  hwy::VQSort(data, size, hwy::SortAscending());
}

template <typename T> void sortNanLast(T *data, int64_t size) {
  if (size < 2)
    return;
#ifdef __APPLE__
  if constexpr (std::is_same_v<T, double>) {
    T *end = std::partition(data, data + size, [](T value) { return value == value; });
    std::sort(data, end);
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

SEQ_FUNC void cnp_sort_float32(float *data, int64_t n) {
  if (n > 1)
    HWY_DYNAMIC_DISPATCH(sortListFloat32)(data, n);
}

SEQ_FUNC void cnp_sort_float64(double *data, int64_t n) {
#ifdef __APPLE__
  sortNanLast(data, n);
#else
  if (n > 1)
    HWY_DYNAMIC_DISPATCH(sortListFloat64)(data, n);
#endif
}

SEQ_FUNC void cnp_sort_float32_nan_last(float *data, int64_t n) {
  sortNanLast(data, n);
}

SEQ_FUNC void cnp_sort_float64_nan_last(double *data, int64_t n) {
  sortNanLast(data, n);
}

SEQ_FUNC void cnp_sort_uint128(hwy::uint128_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}
#endif
