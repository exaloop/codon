// Inter-sequence alignment kernel adapted from BWA-MEM2
// https://github.com/bwa-mem2/bwa-mem2/blob/master/src/bandedSWA.cpp
#pragma once

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(always_inline)
#define ALWAYS_INLINE __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif

#include "ksw2.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>

extern "C" void *seq_alloc_atomic(size_t n);
extern "C" void *seq_realloc(void *p, size_t n);
extern "C" void seq_free(void *p);

void pv(const char *tag, __m256i var) {
  int16_t *val = (int16_t *)&var;
  printf("%s: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", tag, val[0], val[1],
         val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9], val[10],
         val[11], val[12], val[13], val[14], val[15]);
}

struct SeqPair {
  int32_t id;
  int32_t len1, len2;
  int32_t score;
  uint32_t *cigar;
  int32_t n_cigar;
  int32_t flags;
};

#define min_(x, y) ((x) > (y) ? (y) : (x))
#define max_(x, y) ((x) > (y) ? (x) : (y))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#if defined(__clang__) || defined(__GNUC__)
#define __mmask8 uint8_t
#define __mmask16 uint16_t
#define __mmask32 uint32_t
#endif

#define AMBIG 4
#define DUMMY1 99
#define DUMMY2 100

template <unsigned W, unsigned N> struct SIMD {};

template <> struct SIMD<128, 8> {
  static constexpr int MAX_SEQ_LEN = 128;
  static constexpr int PFD = -1;
  static constexpr __mmask16 DMASK = 0xFFFF;
  using int_t = int8_t;
  using uint_t = uint8_t;
  using Vec = __m128i;
  using Cmp = Vec;

  static ALWAYS_INLINE Vec zero() { return _mm_setzero_si128(); }

  static ALWAYS_INLINE Cmp ff() { return _mm_set1_epi8(0xFF); }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) { return a; }

  static ALWAYS_INLINE Vec set(int_t n) { return _mm_set1_epi8(n); }

  static ALWAYS_INLINE Vec load(Vec *p) { return _mm_load_si128(p); }

  static ALWAYS_INLINE void store(Vec *p, Vec v) { _mm_store_si128(p, v); }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) { return _mm_cmpeq_epi8(a, b); }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) { return _mm_cmpgt_epi8(a, b); }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) { return _mm_add_epi8(a, b); }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) { return _mm_sub_epi8(a, b); }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) __attribute__((target("sse4.1"))) {
    return _mm_min_epi8(a, b);
  }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) __attribute__((target("sse4.1"))) {
    return _mm_max_epi8(a, b);
  }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) { return _mm_min_epu8(a, b); }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) { return _mm_max_epu8(a, b); }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) { return _mm_and_si128(a, b); }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) { return _mm_or_si128(a, b); }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) { return _mm_xor_si128(a, b); }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) { return _mm_and_si128(a, b); }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) { return _mm_or_si128(a, b); }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) { return _mm_xor_si128(a, b); }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("sse4.1"))) {
    return _mm_abs_epi8(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c)
      __attribute__((target("sse4.1"))) {
    return _mm_blendv_epi8(a, b, c);
  }

  static ALWAYS_INLINE bool all(Cmp c) {
    return (_mm_movemask_epi8(c) & DMASK) == DMASK;
  }

  static ALWAYS_INLINE bool none(Cmp c) { return (_mm_movemask_epi8(c) & DMASK) == 0; }
};

template <> struct SIMD<256, 8> {
  static constexpr int MAX_SEQ_LEN = 128;
  static constexpr int PFD = 5;
  static constexpr __mmask32 DMASK = 0xFFFFFFFF;
  using int_t = int8_t;
  using uint_t = uint8_t;
  using Vec = __m256i;
  using Cmp = Vec;

  static ALWAYS_INLINE Vec zero() __attribute__((target("avx2"))) {
    return _mm256_setzero_si256();
  }

  static ALWAYS_INLINE Cmp ff() __attribute__((target("avx2"))) {
    return _mm256_set1_epi8(0xFF);
  }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) { return a; }

  static ALWAYS_INLINE Vec set(int_t n) __attribute__((target("avx2"))) {
    return _mm256_set1_epi8(n);
  }

  static ALWAYS_INLINE Vec load(Vec *p) __attribute__((target("avx2"))) {
    return _mm256_load_si256(p);
  }

  static ALWAYS_INLINE void store(Vec *p, Vec v) __attribute__((target("avx2"))) {
    _mm256_store_si256(p, v);
  }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_cmpeq_epi8(a, b);
  }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_cmpgt_epi8(a, b);
  }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_add_epi8(a, b);
  }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_sub_epi8(a, b);
  }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_min_epi8(a, b);
  }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_max_epi8(a, b);
  }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_min_epu8(a, b);
  }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_max_epu8(a, b);
  }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_and_si256(a, b);
  }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_or_si256(a, b);
  }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_xor_si256(a, b);
  }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_and_si256(a, b);
  }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_or_si256(a, b);
  }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_xor_si256(a, b);
  }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("avx2"))) {
    return _mm256_abs_epi8(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c) __attribute__((target("avx2"))) {
    return _mm256_blendv_epi8(a, b, c);
  }

  static ALWAYS_INLINE bool all(Cmp c) __attribute__((target("avx2"))) {
    return (_mm256_movemask_epi8(c) & DMASK) == DMASK;
  }

  static ALWAYS_INLINE bool none(Cmp c) __attribute__((target("avx2"))) {
    return (_mm256_movemask_epi8(c) & DMASK) == 0;
  }
};

template <> struct SIMD<512, 8> {
  static constexpr int MAX_SEQ_LEN = 128;
  static constexpr int PFD = 5;
  static constexpr __mmask64 DMASK = 0xFFFFFFFFFFFFFFFF;
  using int_t = int8_t;
  using uint_t = uint8_t;
  using Vec = __m512i;
  using Cmp = __mmask64;

  static ALWAYS_INLINE Vec zero() __attribute__((target("avx512bw"))) {
    return _mm512_setzero_si512();
  }

  static ALWAYS_INLINE Cmp ff() { return DMASK; }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) __attribute__((target("avx512bw"))) {
    return _mm512_movepi8_mask(a);
  }

  static ALWAYS_INLINE Vec set(int_t n) __attribute__((target("avx512bw"))) {
    return _mm512_set1_epi8(n);
  }

  static ALWAYS_INLINE Vec load(Vec *p) __attribute__((target("avx512bw"))) {
    return _mm512_load_si512(p);
  }

  static ALWAYS_INLINE void store(Vec *p, Vec v) __attribute__((target("avx512bw"))) {
    _mm512_store_si512(p, v);
  }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_cmpeq_epi8_mask(a, b);
  }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_cmpgt_epi8_mask(a, b);
  }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_add_epi8(a, b);
  }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_sub_epi8(a, b);
  }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_min_epi8(a, b);
  }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_max_epi8(a, b);
  }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_min_epu8(a, b);
  }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_max_epu8(a, b);
  }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_and_si512(a, b);
  }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_or_si512(a, b);
  }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_xor_si512(a, b);
  }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) { return a & b; }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) { return a | b; }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) { return a ^ b; }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("avx512bw"))) {
    return _mm512_abs_epi8(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c)
      __attribute__((target("avx512bw"))) {
    return _mm512_mask_blend_epi8(c, a, b);
  }

  static ALWAYS_INLINE bool all(Cmp c) { return c == DMASK; }

  static ALWAYS_INLINE bool none(Cmp c) { return c == 0; }
};

template <> struct SIMD<128, 16> {
  static constexpr int MAX_SEQ_LEN = 32768;
  static constexpr int PFD = 2;
  static constexpr __mmask16 DMASK = 0xAAAA;
  using int_t = int16_t;
  using uint_t = uint16_t;
  using Vec = __m128i;
  using Cmp = Vec;

  static ALWAYS_INLINE Vec zero() { return _mm_setzero_si128(); }

  static ALWAYS_INLINE Cmp ff() { return _mm_set1_epi8(0xFF); }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) { return a; }

  static ALWAYS_INLINE Vec set(int_t n) { return _mm_set1_epi16(n); }

  static ALWAYS_INLINE Vec load(Vec *p) { return _mm_load_si128(p); }

  static ALWAYS_INLINE void store(Vec *p, Vec v) { _mm_store_si128(p, v); }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) { return _mm_cmpeq_epi16(a, b); }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) { return _mm_cmpgt_epi16(a, b); }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) { return _mm_add_epi16(a, b); }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) { return _mm_sub_epi16(a, b); }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) { return _mm_min_epi16(a, b); }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) { return _mm_max_epi16(a, b); }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) __attribute__((target("sse4.1"))) {
    return _mm_min_epu16(a, b);
  }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) __attribute__((target("sse4.1"))) {
    return _mm_max_epu16(a, b);
  }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) { return _mm_and_si128(a, b); }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) { return _mm_or_si128(a, b); }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) { return _mm_xor_si128(a, b); }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) { return _mm_and_si128(a, b); }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) { return _mm_or_si128(a, b); }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) { return _mm_xor_si128(a, b); }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("sse4.1"))) {
    return _mm_abs_epi16(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c)
      __attribute__((target("sse4.1"))) {
    return _mm_blendv_epi8(a, b, c);
  }

  static ALWAYS_INLINE bool all(Cmp c) {
    return (_mm_movemask_epi8(c) & DMASK) == DMASK;
  }

  static ALWAYS_INLINE bool none(Cmp c) { return (_mm_movemask_epi8(c) & DMASK) == 0; }
};

template <> struct SIMD<256, 16> {
  static constexpr int MAX_SEQ_LEN = 32768;
  static constexpr int PFD = 2;
  static constexpr __mmask32 DMASK = 0xAAAAAAAA;
  using int_t = int16_t;
  using uint_t = uint16_t;
  using Vec = __m256i;
  using Cmp = Vec;

  static ALWAYS_INLINE Vec zero() __attribute__((target("avx2"))) {
    return _mm256_setzero_si256();
  }

  static ALWAYS_INLINE Cmp ff() __attribute__((target("avx2"))) {
    return _mm256_set1_epi8(0xFF);
  }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) { return a; }

  static ALWAYS_INLINE Vec set(int_t n) __attribute__((target("avx2"))) {
    return _mm256_set1_epi16(n);
  }

  static ALWAYS_INLINE Vec load(Vec *p) __attribute__((target("avx2"))) {
    return _mm256_load_si256(p);
  }

  static ALWAYS_INLINE void store(Vec *p, Vec v) __attribute__((target("avx2"))) {
    _mm256_store_si256(p, v);
  }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_cmpeq_epi16(a, b);
  }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_cmpgt_epi16(a, b);
  }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_add_epi16(a, b);
  }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_sub_epi16(a, b);
  }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_min_epi16(a, b);
  }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_max_epi16(a, b);
  }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_min_epu16(a, b);
  }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_max_epu16(a, b);
  }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_and_si256(a, b);
  }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_or_si256(a, b);
  }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) __attribute__((target("avx2"))) {
    return _mm256_xor_si256(a, b);
  }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_and_si256(a, b);
  }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_or_si256(a, b);
  }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) __attribute__((target("avx2"))) {
    return _mm256_xor_si256(a, b);
  }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("avx2"))) {
    return _mm256_abs_epi16(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c) __attribute__((target("avx2"))) {
    return _mm256_blendv_epi8(a, b, c);
  }

  static ALWAYS_INLINE bool all(Cmp c) __attribute__((target("avx2"))) {
    return (_mm256_movemask_epi8(c) & DMASK) == DMASK;
  }

  static ALWAYS_INLINE bool none(Cmp c) __attribute__((target("avx2"))) {
    return (_mm256_movemask_epi8(c) & DMASK) == 0;
  }
};

template <> struct SIMD<512, 16> {
  static constexpr int MAX_SEQ_LEN = 32768;
  static constexpr int PFD = 2;
  static constexpr __mmask32 DMASK = 0xFFFFFFFF;
  using int_t = int16_t;
  using uint_t = uint16_t;
  using Vec = __m512i;
  using Cmp = __mmask32;

  static ALWAYS_INLINE Vec zero() __attribute__((target("avx512bw"))) {
    return _mm512_setzero_si512();
  }

  static ALWAYS_INLINE Cmp ff() { return DMASK; }

  static ALWAYS_INLINE Cmp vec2cmp(Vec a) __attribute__((target("avx512bw"))) {
    return _mm512_movepi16_mask(a);
  }

  static ALWAYS_INLINE Vec set(int_t n) __attribute__((target("avx512bw"))) {
    return _mm512_set1_epi16(n);
  }

  static ALWAYS_INLINE Vec load(Vec *p) __attribute__((target("avx512bw"))) {
    return _mm512_load_si512(p);
  }

  static ALWAYS_INLINE void store(Vec *p, Vec v) __attribute__((target("avx512bw"))) {
    _mm512_store_si512(p, v);
  }

  static ALWAYS_INLINE Cmp eq(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_cmpeq_epi16_mask(a, b);
  }

  static ALWAYS_INLINE Cmp gt(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_cmpgt_epi16_mask(a, b);
  }

  static ALWAYS_INLINE Vec add(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_add_epi16(a, b);
  }

  static ALWAYS_INLINE Vec sub(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_sub_epi16(a, b);
  }

  static ALWAYS_INLINE Vec min(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_min_epi16(a, b);
  }

  static ALWAYS_INLINE Vec max(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_max_epi16(a, b);
  }

  static ALWAYS_INLINE Vec umin(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_min_epu16(a, b);
  }

  static ALWAYS_INLINE Vec umax(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_max_epu16(a, b);
  }

  static ALWAYS_INLINE Vec and_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_and_si512(a, b);
  }

  static ALWAYS_INLINE Vec or_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_or_si512(a, b);
  }

  static ALWAYS_INLINE Vec xor_(Vec a, Vec b) __attribute__((target("avx512bw"))) {
    return _mm512_xor_si512(a, b);
  }

  static ALWAYS_INLINE Cmp andc_(Cmp a, Cmp b) { return a & b; }

  static ALWAYS_INLINE Cmp orc_(Cmp a, Cmp b) { return a | b; }

  static ALWAYS_INLINE Cmp xorc_(Cmp a, Cmp b) { return a ^ b; }

  static ALWAYS_INLINE Vec abs(Vec a) __attribute__((target("avx512bw"))) {
    return _mm512_abs_epi16(a);
  }

  static ALWAYS_INLINE Vec blend(Vec a, Vec b, Cmp c)
      __attribute__((target("avx512bw"))) {
    return _mm512_mask_blend_epi16(c, a, b);
  }

  static ALWAYS_INLINE bool all(Cmp c) { return c == DMASK; }

  static ALWAYS_INLINE bool none(Cmp c) { return c == 0; }
};

template <unsigned W, unsigned N, bool CIGAR = false> class InterSW {
public:
  static constexpr size_t LEN_LIMIT = 512;
  using int_t = typename SIMD<W, N>::int_t;
  using uint_t = typename SIMD<W, N>::uint_t;

  InterSW(int o_del, int e_del, int o_ins, int e_ins, int zdrop, int end_bonus,
          int8_t w_match, int8_t w_ambig, int8_t w_mismatch);
  ~InterSW();

  void SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer, int32_t numPairs,
          int bandwidth);

private:
  static ALWAYS_INLINE SeqPair getp(SeqPair *p, int idx, int numPairs) {
    static SeqPair empty = {0, 0, 0, -1, nullptr, 0};
    return idx < numPairs ? p[idx] : empty;
  }

  static ALWAYS_INLINE int64_t idr(const SeqPair &sp) {
    return (int64_t)sp.id * LEN_LIMIT;
  }

  static ALWAYS_INLINE int64_t idq(const SeqPair &sp) {
    return (int64_t)sp.id * LEN_LIMIT;
  }

  void ALWAYS_INLINE SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                            uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                            int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                            uint_t myband[], uint_t z[], uint_t off[]);

  void SWBacktrace(bool is_rot, bool is_rev, int min_intron_len, const uint_t *p,
                   const uint_t *off, const uint_t *off_end, size_t n_col, int_t i0,
                   int_t j0, int *m_cigar_, int *n_cigar_, uint32_t **cigar_,
                   int offset);

  int end_bonus, zdrop;
  int o_del, o_ins, e_del, e_ins;
  int8_t w_match;
  int8_t w_mismatch;
  int8_t w_ambig;

  int_t *F;
  int_t *H1, *H2;
};

template __attribute__((target("sse4.1"))) void
InterSW<128, 8, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);
template __attribute__((target("sse4.1"))) void
InterSW<128, 8, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                          int32_t numPairs, int bandwidth);
template __attribute__((target("sse4.1"))) void
InterSW<128, 16, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                            int32_t numPairs, int bandwidth);
template __attribute__((target("sse4.1"))) void
InterSW<128, 16, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);

template __attribute__((target("avx2"))) void
InterSW<256, 8, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);
template __attribute__((target("avx2"))) void
InterSW<256, 8, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                          int32_t numPairs, int bandwidth);
template __attribute__((target("avx2"))) void
InterSW<256, 16, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                            int32_t numPairs, int bandwidth);
template __attribute__((target("avx2"))) void
InterSW<256, 16, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);

template __attribute__((target("avx512bw"))) void
InterSW<512, 8, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);
template __attribute__((target("avx512bw"))) void
InterSW<512, 8, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                          int32_t numPairs, int bandwidth);
template __attribute__((target("avx512bw"))) void
InterSW<512, 16, false>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                            int32_t numPairs, int bandwidth);
template __attribute__((target("avx512bw"))) void
InterSW<512, 16, true>::SW(SeqPair *pairArray, uint8_t *seqBufRef, uint8_t *seqBufQer,
                           int32_t numPairs, int bandwidth);

template __attribute__((target("sse4.1"))) void
InterSW<128, 8, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("sse4.1"))) void
InterSW<128, 8, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                              uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                              int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                              uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("sse4.1"))) void
InterSW<128, 16, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                                uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                                int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                                uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("sse4.1"))) void
InterSW<128, 16, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);

template __attribute__((target("avx2"))) void
InterSW<256, 8, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx2"))) void
InterSW<256, 8, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                              uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                              int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                              uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx2"))) void
InterSW<256, 16, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                                uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                                int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                                uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx2"))) void
InterSW<256, 16, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);

template __attribute__((target("avx512bw"))) void
InterSW<512, 8, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx512bw"))) void
InterSW<512, 8, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                              uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                              int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                              uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx512bw"))) void
InterSW<512, 16, false>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                                uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                                int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                                uint_t myband[], uint_t z[], uint_t off[]);
template __attribute__((target("avx512bw"))) void
InterSW<512, 16, true>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                               uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                               int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                               uint_t myband[], uint_t z[], uint_t off[]);

template <unsigned W, unsigned N, bool CIGAR>
InterSW<W, N, CIGAR>::InterSW(const int o_del, const int e_del, const int o_ins,
                              const int e_ins, const int zdrop, const int end_bonus,
                              const int8_t w_match, const int8_t w_mismatch,
                              const int8_t w_ambig) {
  this->end_bonus = end_bonus;
  this->zdrop = zdrop;
  this->o_del = o_del;
  this->o_ins = o_ins;
  this->e_del = e_del;
  this->e_ins = e_ins;

  this->w_match = w_match;
  this->w_mismatch = -w_mismatch;
  this->w_ambig = (w_ambig == 0 ? this->w_mismatch : -w_ambig);
  this->F = this->H1 = this->H2 = nullptr;

  constexpr int MAX_SEQ_LEN = SIMD<W, N>::MAX_SEQ_LEN;
  constexpr int SIMD_WIDTH = W / N;

  F = H1 = H2 = nullptr;
  F = (int_t *)_mm_malloc(MAX_SEQ_LEN * SIMD_WIDTH * sizeof(int_t), 64);
  H1 = (int_t *)_mm_malloc(MAX_SEQ_LEN * SIMD_WIDTH * sizeof(int_t), 64);
  H2 = (int_t *)_mm_malloc(MAX_SEQ_LEN * SIMD_WIDTH * sizeof(int_t), 64);

  if (F == nullptr || H1 == nullptr || H2 == nullptr) {
    fprintf(stderr, "failed to allocate memory for inter-sequence alignment\n");
    exit(EXIT_FAILURE);
  }
}

template <unsigned W, unsigned N, bool CIGAR> InterSW<W, N, CIGAR>::~InterSW() {
  _mm_free(F);
  _mm_free(H1);
  _mm_free(H2);
}

template <unsigned W, unsigned N, bool CIGAR>
void InterSW<W, N, CIGAR>::SW(SeqPair *pairArray, uint8_t *seqBufRef,
                              uint8_t *seqBufQer, int32_t numPairs, int bandwidth) {
  using S = SIMD<W, N>;
  using Vec = typename S::Vec;
  using Cmp = typename S::Cmp;

  constexpr int MAX_SEQ_LEN = S::MAX_SEQ_LEN;
  constexpr int SIMD_WIDTH = W / N;
  constexpr uint_t FF = (1 << N) - 1;

  const uint_t w = (bandwidth > (int)FF || bandwidth < 0) ? FF : (uint_t)bandwidth;

  uint_t *seq1SoA = (uint_t *)_mm_malloc(MAX_SEQ_LEN * SIMD_WIDTH * sizeof(uint_t), 64);
  uint_t *seq2SoA = (uint_t *)_mm_malloc(MAX_SEQ_LEN * SIMD_WIDTH * sizeof(uint_t), 64);

  uint_t *z = nullptr;
  uint_t *off = nullptr;
  size_t offlen = 0;
  if (CIGAR) {
    // TODO: make alloc smaller based on bandwidth and lengths
    z = (uint_t *)_mm_malloc(LEN_LIMIT * LEN_LIMIT * SIMD_WIDTH * sizeof(uint_t), 64);
    offlen = LEN_LIMIT * SIMD_WIDTH * sizeof(uint_t);
    off = (uint_t *)_mm_malloc(offlen, 64);
  }

  if (seq1SoA == nullptr || seq2SoA == nullptr) {
    fprintf(stderr, "failed to allocate memory for inter-sequence alignment (SoA)\n");
    exit(EXIT_FAILURE);
  }

  int32_t roundNumPairs = ((numPairs + SIMD_WIDTH - 1) / SIMD_WIDTH) * SIMD_WIDTH;

  int eb = end_bonus;
  {
    int32_t i;
    uint_t *mySeq1SoA = seq1SoA;
    uint_t *mySeq2SoA = seq2SoA;
    assert(mySeq1SoA != nullptr && mySeq2SoA != nullptr);
    uint8_t *seq1;
    uint8_t *seq2;
    uint_t h0[SIMD_WIDTH] __attribute__((aligned(64)));
    uint_t band[SIMD_WIDTH];
    uint_t qlen[SIMD_WIDTH] __attribute__((aligned(64)));
    bool ext[SIMD_WIDTH];
    uint_t bsize = 0;

    Vec zero128 = S::zero();
    Vec o_ins128 = S::set(o_ins);
    Vec e_ins128 = S::set(e_ins);
    Vec oe_ins128 = S::set(o_ins + e_ins);
    Vec o_del128 = S::set(o_del);
    Vec e_del128 = S::set(e_del);
    Vec eb_ins128 = S::set(eb - o_ins);
    Vec eb_del128 = S::set(eb - o_del);

    int_t max = 0;
    if (max < w_match)
      max = w_match;
    if (max < w_mismatch)
      max = w_mismatch;
    if (max < w_ambig)
      max = w_ambig;

    int nstart = 0, nend = numPairs;

    for (i = nstart; i < nend; i += SIMD_WIDTH) {
      int32_t j, k;
      uint_t maxLen1 = 0;
      uint_t maxLen2 = 0;
      bsize = w;

      uint64_t tim;
      for (j = 0; j < SIMD_WIDTH; j++) {
        if (S::PFD >= 0) { // prefetch block
          SeqPair spf = getp(pairArray, i + j + S::PFD, numPairs);
          _mm_prefetch((const char *)seqBufRef + idr(spf), _MM_HINT_NTA);
          _mm_prefetch((const char *)seqBufRef + idr(spf) + 64, _MM_HINT_NTA);
        }
        SeqPair sp = getp(pairArray, i + j, numPairs);
        h0[j] = 0;
        ext[j] = (sp.flags & KSW_EZ_EXTZ_ONLY) != 0;
        seq1 = seqBufRef + idr(sp);

        for (k = 0; k < sp.len1; k++) {
          mySeq1SoA[k * SIMD_WIDTH + j] = (seq1[k] == AMBIG ? FF : seq1[k]);
          H2[k * SIMD_WIDTH + j] = 0;
        }
        qlen[j] = sp.len2 * max;
        if (maxLen1 < sp.len1)
          maxLen1 = sp.len1;
      }

      for (j = 0; j < SIMD_WIDTH; j++) {
        SeqPair sp = getp(pairArray, i + j, numPairs);
        for (k = sp.len1; k <= maxLen1; k++) {
          mySeq1SoA[k * SIMD_WIDTH + j] = DUMMY1;
          H2[k * SIMD_WIDTH + j] = DUMMY1;
        }
      }

      Vec h0_128 = S::load((Vec *)h0);
      S::store((Vec *)H2, h0_128);
      Vec tmp128 = S::sub(h0_128, o_del128);

      for (k = 1; k < maxLen1; k++) {
        tmp128 = S::sub(tmp128, e_del128);
        S::store((Vec *)(H2 + k * SIMD_WIDTH), tmp128);
      }

      for (j = 0; j < SIMD_WIDTH; j++) {
        if (S::PFD >= 0) { // prefetch block
          SeqPair spf = getp(pairArray, i + j + S::PFD, numPairs);
          _mm_prefetch((const char *)seqBufQer + idq(spf), _MM_HINT_NTA);
          _mm_prefetch((const char *)seqBufQer + idq(spf) + 64, _MM_HINT_NTA);
        }
        SeqPair sp = getp(pairArray, i + j, numPairs);
        seq2 = seqBufQer + idq(sp);
        for (k = 0; k < sp.len2; k++) {
          mySeq2SoA[k * SIMD_WIDTH + j] = (seq2[k] == AMBIG ? FF : seq2[k]);
          H1[k * SIMD_WIDTH + j] = 0;
        }
        if (maxLen2 < sp.len2)
          maxLen2 = sp.len2;
      }

      for (j = 0; j < SIMD_WIDTH; j++) {
        SeqPair sp = getp(pairArray, i + j, numPairs);
        for (k = sp.len2; k <= maxLen2; k++) {
          mySeq2SoA[k * SIMD_WIDTH + j] = DUMMY2;
          H1[k * SIMD_WIDTH + j] = 0;
        }
      }

      S::store((Vec *)H1, h0_128);
      Cmp cmp128;
      tmp128 = S::sub(h0_128, oe_ins128);
      S::store((Vec *)(H1 + SIMD_WIDTH), tmp128);
      for (k = 2; k < maxLen2; k++) {
        Vec h1_128 = tmp128;
        tmp128 = S::sub(h1_128, e_ins128);
        S::store((Vec *)(H1 + k * SIMD_WIDTH), tmp128);
      }

      uint_t myband[SIMD_WIDTH] __attribute__((aligned(64)));
      uint_t temp[SIMD_WIDTH] __attribute__((aligned(64)));
      {
        Vec qlen128 = S::load((Vec *)qlen);
        Vec sum128 = S::add(qlen128, eb_ins128);
        S::store((Vec *)temp, sum128);
        for (int l = 0; l < SIMD_WIDTH; l++) {
          double val = temp[l] / e_ins + 1.0;
          int max_ins = (int)val;
          max_ins = max_ins > 1 ? max_ins : 1;
          myband[l] = min_(bsize, max_ins);
        }
        sum128 = S::add(qlen128, eb_del128);
        S::store((Vec *)temp, sum128);
        for (int l = 0; l < SIMD_WIDTH; l++) {
          double val = temp[l] / e_del + 1.0;
          int max_ins = (int)val;
          max_ins = max_ins > 1 ? max_ins : 1;
          myband[l] = min_(myband[l], max_ins);
          myband[l] = ext[l] ? myband[l] : w;
          bsize = bsize < myband[l] ? myband[l] : bsize;
        }
      }

      if (CIGAR)
        memset(off, '\0', offlen);
      SWCore(mySeq1SoA, mySeq2SoA, maxLen1, maxLen2, pairArray + i,
             pairArray + numPairs, h0, numPairs, zdrop, bsize, qlen, myband, z, off);
    }
  }

  _mm_free(seq1SoA);
  _mm_free(seq2SoA);
  if (CIGAR) {
    _mm_free(z);
    _mm_free(off);
  }
}

template <unsigned W, unsigned N, bool CIGAR>
void InterSW<W, N, CIGAR>::SWCore(uint_t seq1SoA[], uint_t seq2SoA[], uint_t nrow,
                                  uint_t ncol, SeqPair *p, SeqPair *endp, uint_t h0[],
                                  int32_t numPairs, int zdrop, uint_t w, uint_t qlen[],
                                  uint_t myband[], uint_t z[], uint_t off[]) {
  using S = SIMD<W, N>;
  using Vec = typename S::Vec;
  using Cmp = typename S::Cmp;

  constexpr int MAX_SEQ_LEN = S::MAX_SEQ_LEN;
  constexpr int SIMD_WIDTH = W / N;
  constexpr uint_t FF = (1 << N) - 1;
  constexpr int_t NEG_INF = -(1 << (N - 2));

  Vec match256 = S::set(this->w_match);
  Vec mismatch256 = S::set(this->w_mismatch);
  Vec w_ambig_256 = S::set(this->w_ambig);
  Vec five256 = S::set(5);
  Vec neg_inf256 = S::set(NEG_INF);
  Vec init256 = neg_inf256;

  Vec e_del256 = S::set(this->e_del);
  Vec oe_del256 = S::set(this->o_del + this->e_del);
  Vec e_ins256 = S::set(this->e_ins);
  Vec oe_ins256 = S::set(this->o_ins + this->e_ins);

  int_t *H_h = H1;
  int_t *H_v = H2;

  int_t i, j;

  uint_t tlen[SIMD_WIDTH] __attribute((aligned(64)));
  uint_t tail[SIMD_WIDTH] __attribute((aligned(64)));
  uint_t head[SIMD_WIDTH] __attribute((aligned(64)));

  int minq = 10000000;
  for (int l = 0; l < SIMD_WIDTH; l++) {
    tlen[l] = p[l].len1;
    qlen[l] = p[l].len2;
    if (p[l].len2 < minq)
      minq = p[l].len2;
  }
  minq -= 1; // for gscore

  Vec tlen256 = S::load((Vec *)tlen);
  Vec qlen256 = S::load((Vec *)qlen);
  Vec myband256 = S::load((Vec *)myband);
  Vec zero256 = S::zero();
  Vec one256 = S::set(1);
  Vec two256 = S::set(2);
  Vec max_ie256 = zero256;
  Vec ff256 = S::set(FF);

  Vec tail256 = qlen256, head256 = zero256;
  S::store((Vec *)head, head256);
  S::store((Vec *)tail, tail256);

  Vec mlen256 = S::add(qlen256, myband256);
  mlen256 = S::umin(mlen256, tlen256);

  uint_t temp[SIMD_WIDTH] __attribute((aligned(64)));
  uint_t temp1[SIMD_WIDTH] __attribute((aligned(64)));

  Vec s00 = S::load((Vec *)(seq1SoA));
  Vec hval = S::load((Vec *)(H_v));

  Vec maxScore256 = hval;
  for (j = 0; j < ncol; j++)
    S::store((Vec *)(F + j * SIMD_WIDTH), init256);

  Vec x256 = zero256;
  Vec y256 = zero256;
  Vec i256 = zero256;
  Vec gscore = S::set(-1);
  Vec max_off256 = zero256;
  Vec exit0 = S::set(FF);
  Vec zdrop256 = S::set(zdrop);

  int beg = 0, end = ncol;
  int nbeg = beg, nend = end;

  for (i = 0; i < nrow; i++) {
    Vec e11 = init256;
    Vec h00, h11, h10;
    Vec s10 = S::load((Vec *)(seq1SoA + (i + 0) * SIMD_WIDTH));

    beg = nbeg;
    end = nend;
    int pbeg = beg;
    if (beg < i - w)
      beg = i - w;
    if (end > i + w + 1)
      end = i + w + 1;
    if (end > ncol)
      end = ncol;

    h10 = init256;
    if (beg == 0)
      h10 = S::load((Vec *)(H_v + (i + 1) * SIMD_WIDTH));

    Vec j256 = zero256;
    Vec maxRS1 = init256;

    Vec i1_256 = S::set(i + 1);
    Vec y1_256 = zero256;

    Vec i256, cache256;
    Vec phead256 = head256, ptail256 = tail256;
    i256 = S::set(i);
    cache256 = S::sub(i256, myband256);
    head256 = S::max(head256, cache256);
    cache256 = S::add(i1_256, myband256);
    tail256 = S::umin(tail256, cache256);
    tail256 = S::umin(tail256, qlen256);

    Cmp cmph = S::eq(head256, phead256);
    Cmp cmpt = S::eq(tail256, ptail256);
    cmph = S::andc_(cmph, cmpt);

    if (!S::all(cmph)) {
      for (int l = beg; l < end; l++) {
        Vec h256 = S::load((Vec *)(H_h + l * SIMD_WIDTH));
        Vec f256 = S::load((Vec *)(F + l * SIMD_WIDTH));

        Vec pj256 = S::set(l);
        Vec j256 = S::set(l + 1);
        Cmp cmp1 = S::gt(head256, pj256);
        if (S::none(cmp1))
          break;

        Cmp cmp2 = S::gt(j256, tail256);
        cmp1 = S::orc_(cmp1, cmp2);
        h256 = S::blend(h256, init256, cmp1);
        f256 = S::blend(f256, init256, cmp1);

        S::store((Vec *)(F + l * SIMD_WIDTH), f256);
        S::store((Vec *)(H_h + l * SIMD_WIDTH), h256);
      }
    }

    Cmp cmp256_1 = S::gt(i1_256, tlen256);
    Cmp cmpim = S::gt(i1_256, mlen256);
    Cmp cmpht = S::eq(tail256, head256);
    cmpim = S::orc_(cmpim, cmpht);
    cmpht = S::gt(head256, tail256);
    cmpim = S::orc_(cmpim, cmpht);
    exit0 = /*GLOBAL ? zero256 :*/ S::blend(exit0, zero256, cmpim);

    j256 = S::set(beg);

    if (CIGAR) {
      // off[i] = beg
      Vec offi = S::blend(j256, zero256, cmpim);
      S::store((Vec *)(off + i * SIMD_WIDTH), offi);
    }

    for (j = beg; j < end; j++) {
      Vec f11, f21, s2;
      h00 = S::load((Vec *)(H_h + j * SIMD_WIDTH));
      f11 = S::load((Vec *)(F + j * SIMD_WIDTH));

      s2 = S::load((Vec *)(seq2SoA + j * SIMD_WIDTH));

      Vec pj256 = j256;
      j256 = S::add(j256, one256);

      // main code
      Vec d, dtmp;
      Cmp dcmp;
      Cmp cmp11 = S::eq(s10, s2);
      Vec sbt11 = S::blend(mismatch256, match256, cmp11);
      Vec tmp256 = S::umax(s10, s2);
      cmp11 = S::vec2cmp(tmp256);
      sbt11 = S::blend(sbt11, w_ambig_256, cmp11);
      Vec m11 = S::add(h00, sbt11);
      if (CIGAR) {
        dcmp = S::orc_(S::gt(m11, e11), S::eq(m11, e11));
        d = S::blend(two256, zero256, dcmp);
      }
      h11 = S::max(m11, e11);
      if (CIGAR) {
        dcmp = S::orc_(S::gt(h11, f11), S::eq(h11, f11));
        d = S::blend(one256, d, dcmp);
      }
      h11 = S::max(h11, f11);
      Vec temp256 = S::sub(m11, oe_ins256);
      Vec val256 = temp256;
      e11 = S::sub(e11, e_ins256);
      if (CIGAR) {
        dcmp = S::gt(e11, val256);
        dtmp = S::blend(zero256, S::set(0x10), dcmp);
        d = S::or_(d, dtmp);
      }
      e11 = S::max(val256, e11);
      temp256 = S::sub(m11, oe_del256);
      val256 = temp256;
      f21 = S::sub(f11, e_del256);
      if (CIGAR) {
        dcmp = S::gt(f21, val256);
        dtmp = S::blend(zero256, S::set(0x08), dcmp);
        d = S::or_(d, dtmp);
      }
      f21 = S::max(val256, f21);
      if (CIGAR) {
        // z[i * n_col + j - beg] = d
        S::store((Vec *)(z + (i * LEN_LIMIT + j - beg) * SIMD_WIDTH), d);
      }

      // Masked writing
      Cmp cmp2 = S::gt(head256, pj256);
      Cmp cmp1 = S::gt(pj256, tail256);
      cmp1 = S::orc_(cmp1, cmp2);
      h10 = S::blend(h10, init256, cmp1);
      f21 = S::blend(f21, init256, cmp1);

      Vec bmaxRS = maxRS1, blend256;
      maxRS1 = S::max(maxRS1, h11);
      Cmp cmpA = S::gt(maxRS1, bmaxRS);
      Cmp cmpB = S::eq(maxRS1, h11);
      cmpA = S::orc_(cmpA, cmpB);
      cmp1 = S::gt(j256, tail256);
      cmp1 = S::orc_(cmp1, cmp2);
      blend256 = S::blend(y1_256, j256, cmpA);
      y1_256 = S::blend(blend256, y1_256, cmp1);
      maxRS1 = S::blend(maxRS1, bmaxRS, cmp1);

      S::store((Vec *)(F + j * SIMD_WIDTH), f21);
      S::store((Vec *)(H_h + j * SIMD_WIDTH), h10);

      h10 = h11;

      // gscore calculations
      if (j >= minq) {
        Cmp cmp, cmp_gh;
        Vec max_gh, tmp256_1;
        Vec tlen256s1 = S::sub(tlen256, one256);
        cmp = S::eq(i256, tlen256s1);
        max_gh = h11;
        tmp256_1 = i1_256;
        Vec tmp256_t = S::blend(max_ie256, tmp256_1, cmp);

        /*
        if (!GLOBAL) {
          Cmp mex0 = S::vec2cmp(exit0);
          tmp256_1 = S::blend(max_ie256, tmp256_t, mex0);
          max_gh = S::blend(gscore, max_gh, mex0);
        }
        */

        max_gh = S::blend(gscore, max_gh, cmp);
        cmp = S::gt(j256, tail256);
        max_gh = S::blend(max_gh, gscore, cmp);
        max_ie256 = S::blend(tmp256_1, max_ie256, cmp);
        gscore = max_gh;
      }
    }
    S::store((Vec *)(H_h + j * SIMD_WIDTH), h10);
    S::store((Vec *)(F + j * SIMD_WIDTH), init256);

    /* exit due to zero score by a row */
    Vec bmaxScore256 = maxScore256;
    /*
    if (!GLOBAL) {
      Cmp tmp = S::eq(maxRS1, zero256);
      if (S::all(tmp))
        break;
      exit0 = S::blend(exit0, zero256, tmp);
    } else {
      exit0 = zero256;
    }
    */

    Vec score256 = S::max(maxScore256, maxRS1);
    maxScore256 = score256;
    /*
    if (GLOBAL) {
      maxScore256 = score256;
    } else {
      Cmp mex0 = S::vec2cmp(exit0);
      maxScore256 = S::blend(maxScore256, score256, mex0);
    }
    */

    Cmp cmp = S::gt(maxScore256, bmaxScore256);
    y256 = S::blend(y256, y1_256, cmp);
    x256 = S::blend(x256, i1_256, cmp);

    // max_off calculations
    Vec ind256 = S::sub(y1_256, i1_256);
    ind256 = S::abs(ind256);
    Vec bmax_off256 = max_off256;
    ind256 = S::max(max_off256, ind256);
    max_off256 = S::blend(bmax_off256, ind256, cmp);

    // Z-score
    Vec tmpi = S::sub(i1_256, x256);
    Vec tmpj = S::sub(y1_256, y256);
    cmp = S::gt(tmpi, tmpj);
    score256 = S::sub(maxScore256, maxRS1);
    Vec insdel = S::blend(e_ins256, e_del256, cmp);
    Vec sub_a256 = S::sub(tmpi, tmpj);
    Vec sub_b256 = S::sub(tmpj, tmpi);
    Vec tmp = S::blend(sub_b256, sub_a256, cmp);
    tmp = S::sub(score256, tmp);
    cmp = S::gt(tmp, zdrop256);
    exit0 = S::blend(exit0, zero256, cmp);
    gscore = S::blend(gscore, neg_inf256, cmp);

    /* Narrowing of the band */
    /* From beg */
    int l;
    for (l = beg; l < end; l++) {
      Vec f256 = S::load((Vec *)(F + l * SIMD_WIDTH));
      Vec h256 = S::load((Vec *)(H_h + l * SIMD_WIDTH));
      Vec tmp = S::or_(f256, h256);
      Cmp tmp2 = S::eq(tmp, zero256);
      if (S::all(tmp2))
        nbeg = l;
      else
        break;
    }

    /* From end */
    for (l = end; l >= beg; l--) {
      Vec f256 = S::load((Vec *)(F + l * SIMD_WIDTH));
      Vec h256 = S::load((Vec *)(H_h + l * SIMD_WIDTH));
      Vec tmp = S::or_(f256, h256);
      Cmp tmp2 = S::eq(tmp, zero256);
      if (!S::all(tmp2))
        break;
    }
    nend = l + 2 < ncol ? l + 2 : ncol;

    Vec tail256_ = S::sub(tail256, one256);
    Cmp tmpb = S::ff();

    Vec exit1 = S::xor_(exit0, ff256);
    Vec l256 = S::set(beg);
    for (l = beg; l < end; l++) {
      Vec f256 = S::load((Vec *)(F + l * SIMD_WIDTH));
      Vec h256 = S::load((Vec *)(H_h + l * SIMD_WIDTH));

      Vec tmp_ = S::or_(f256, h256);
      tmp_ = S::or_(tmp_, exit1);
      Cmp tmp = S::eq(tmp_, zero256);
      if (S::none(tmp))
        break;

      tmp = S::andc_(tmp, tmpb);
      l256 = S::add(l256, one256);

      head256 = S::blend(head256, l256, tmp);
      tmpb = tmp;
    }

    Vec index256 = tail256;
    tmpb = S::ff();

    l256 = S::set(end);
    for (l = end; l >= beg; l--) {
      Vec f256 = S::load((Vec *)(F + l * SIMD_WIDTH));
      Vec h256 = S::load((Vec *)(H_h + l * SIMD_WIDTH));

      Vec tmp_ = S::or_(f256, h256);
      tmp_ = S::or_(tmp_, exit1);
      Cmp tmp = S::eq(tmp_, zero256);
      if (S::none(tmp))
        break;

      tmp = S::andc_(tmp, tmpb);
      l256 = S::sub(l256, one256);
      index256 = S::blend(index256, l256, tmp);
      tmpb = tmp;
    }
    index256 = S::add(index256, two256);
    tail256 = S::min(index256, qlen256);
  }

  int_t score[SIMD_WIDTH] __attribute((aligned(64)));
  S::store((Vec *)score, maxScore256);

  int_t maxi[SIMD_WIDTH] __attribute((aligned(64)));
  S::store((Vec *)maxi, x256);

  int_t maxj[SIMD_WIDTH] __attribute((aligned(64)));
  S::store((Vec *)maxj, y256);

  // int_t max_off_ar[SIMD_WIDTH] __attribute((aligned(64)));
  // S::store((Vec *)max_off_ar, max_off256);

  int_t gscore_ar[SIMD_WIDTH] __attribute((aligned(64)));
  S::store((Vec *)gscore_ar, gscore);

  // int_t maxie_ar[SIMD_WIDTH] __attribute((aligned(64)));
  // S::store((Vec *)maxie_ar, max_ie256);

  for (i = 0; i < SIMD_WIDTH; i++) {
    if (p + i >= endp)
      break;
    const bool ext_only = (p[i].flags & KSW_EZ_EXTZ_ONLY) != 0;
    p[i].score = ext_only ? score[i] : gscore_ar[i];
    if (p[i].score == NEG_INF)
      p[i].score = KSW_NEG_INF;

    if (CIGAR) {
      static constexpr size_t CIGAR_INIT_CAP = 15;
      const bool is_rev = (p[i].flags & KSW_EZ_REV_CIGAR) != 0;
      uint32_t *cigar = nullptr;
      int n_cigar = 0, m_cigar = 0;
      int_t i0 = ext_only ? maxi[i] : p[i].len1;
      int_t j0 = ext_only ? maxj[i] : p[i].len2;
      if (i0 > 0 && j0 > 0) {
        m_cigar = CIGAR_INIT_CAP;
        cigar = (uint32_t *)seq_alloc_atomic(m_cigar * sizeof(uint32_t));
        SWBacktrace(false, false, 0, z, off, nullptr, LEN_LIMIT, i0 - 1, j0 - 1,
                    &m_cigar, &n_cigar, &cigar, i);
      }
      p[i].cigar = cigar;
      p[i].n_cigar = n_cigar;
    }
  }
}

// Backtrace code adapted from KSW2
// https://github.com/lh3/ksw2

static ALWAYS_INLINE uint32_t *push_cigar(int *n_cigar, int *m_cigar, uint32_t *cigar,
                                          uint32_t op, int len) {
  if (*n_cigar == 0 || op != (cigar[(*n_cigar) - 1] & 0xf)) {
    if (*n_cigar == *m_cigar) {
      *m_cigar = *m_cigar ? (*m_cigar) << 1 : 4;
      cigar = (uint32_t *)seq_realloc(cigar, (*m_cigar) << 2);
    }
    cigar[(*n_cigar)++] = len << 4 | op;
  } else
    cigar[(*n_cigar) - 1] += len << 4;
  return cigar;
}

template <unsigned W, unsigned N, bool CIGAR>
void InterSW<W, N, CIGAR>::SWBacktrace(bool is_rot, bool is_rev, int min_intron_len,
                                       const uint_t *p, const uint_t *off,
                                       const uint_t *off_end, size_t n_col, int_t i0,
                                       int_t j0, int *m_cigar_, int *n_cigar_,
                                       uint32_t **cigar_, int offset) {
  constexpr int SIMD_WIDTH = W / N;
  int n_cigar = 0, m_cigar = *m_cigar_, i = i0, j = j0, r, state = 0;
  uint32_t *cigar = *cigar_, tmp;
  while (i >= 0 && j >= 0) { // at the beginning of the loop, _state_ tells us
                             // which state to check
    int force_state = -1;
    if (is_rot) {
      r = i + j;
      uint_t off_val = off[r * SIMD_WIDTH + offset];
      if (i < off_val)
        force_state = 2;
      if (off_end && i > off_end[r])
        force_state = 1;
      tmp = force_state < 0 ? p[((size_t)r * n_col + i - off_val) * SIMD_WIDTH + offset]
                            : 0;
    } else {
      uint_t off_val = off[i * SIMD_WIDTH + offset];
      if (j < off_val)
        force_state = 2;
      if (off_end && j > off_end[i])
        force_state = 1;
      tmp = force_state < 0 ? p[((size_t)i * n_col + j - off_val) * SIMD_WIDTH + offset]
                            : 0;
    }
    if (state == 0)
      state = tmp & 7; // if requesting the H state, find state one maximizes it.
    else if (!(tmp >> (state + 2) & 1))
      state = 0; // if requesting other states, _state_ stays the same if it is
                 // a continuation; otherwise, set to H
    if (state == 0)
      state = tmp & 7; // TODO: probably this line can be merged into the "else
                       // if" line right above; not 100% sure
    if (force_state >= 0)
      state = force_state;
    if (state == 0)
      cigar = push_cigar(&n_cigar, &m_cigar, cigar, 0, 1), --i, --j; // match
    else if (state == 1 || (state == 3 && min_intron_len <= 0))
      cigar = push_cigar(&n_cigar, &m_cigar, cigar, 2, 1), --i; // deletion
    else if (state == 3 && min_intron_len > 0)
      cigar = push_cigar(&n_cigar, &m_cigar, cigar, 3, 1), --i; // intron
    else
      cigar = push_cigar(&n_cigar, &m_cigar, cigar, 1, 1), --j; // insertion
  }
  if (i >= 0)
    cigar = push_cigar(&n_cigar, &m_cigar, cigar,
                       min_intron_len > 0 && i >= min_intron_len ? 3 : 2,
                       i + 1); // first deletion
  if (j >= 0)
    cigar = push_cigar(&n_cigar, &m_cigar, cigar, 1, j + 1); // first insertion
  if (!is_rev)
    for (i = 0; i < (n_cigar >> 1); ++i) // reverse CIGAR
      tmp = cigar[i], cigar[i] = cigar[n_cigar - 1 - i], cigar[n_cigar - 1 - i] = tmp;
  *m_cigar_ = m_cigar, *n_cigar_ = n_cigar, *cigar_ = cigar;
}
