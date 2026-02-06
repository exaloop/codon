// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"
#include "hwy/contrib/sort/vqsort-inl.h"

SEQ_FUNC void cnp_sort_int16(int16_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_uint16(uint16_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_int32(int32_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_uint32(uint32_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_int64(int64_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_uint64(uint64_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_uint128(hwy::uint128_t *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_float32(float *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}

SEQ_FUNC void cnp_sort_float64(double *data, int64_t n) {
  hwy::VQSort(data, n, hwy::SortAscending());
}
