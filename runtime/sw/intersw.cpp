#include "intersw.h"
#include "ksw2.h"
#include "lib.h"
#include <cstdint>
#include <cstdlib>
#include <string>

// adapted from minimap2's KSW2 dispatch
// https://github.com/lh3/minimap2/blob/master/ksw2_dispatch.c
#define SIMD_SSE 0x1
#define SIMD_SSE2 0x2
#define SIMD_SSE3 0x4
#define SIMD_SSSE3 0x8
#define SIMD_SSE4_1 0x10
#define SIMD_SSE4_2 0x20
#define SIMD_AVX 0x40
#define SIMD_AVX2 0x80
#define SIMD_AVX512F 0x100

#ifndef _MSC_VER
// adapted from
// https://github.com/01org/linux-sgx/blob/master/common/inc/internal/linux/cpuid_gnu.h
void __cpuidex(int cpuid[4], int func_id, int subfunc_id) {
#if defined(__x86_64__)
  __asm__ volatile("cpuid"
                   : "=a"(cpuid[0]), "=b"(cpuid[1]), "=c"(cpuid[2]), "=d"(cpuid[3])
                   : "0"(func_id), "2"(subfunc_id));
#else // on 32bit, ebx can NOT be used as PIC code
  __asm__ volatile("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1"
                   : "=a"(cpuid[0]), "=r"(cpuid[1]), "=c"(cpuid[2]), "=d"(cpuid[3])
                   : "0"(func_id), "2"(subfunc_id));
#endif
}
#endif

static int intersw_simd = -1;

static int SEQ_MAXSIMD = (SIMD_AVX512F << 1) - 1;

static int x86_simd() {
  // char *env = getenv("SEQ_SWSIMD");
  // int SEQ_MAXSIMD = (SIMD_AVX512F << 1) - 1;
  // if (env && std::string(env) == "AVX2")
  //   SEQ_MAXSIMD = (SIMD_AVX2 << 1) - 1;
  // if (env && std::string(env) == "AVX")
  //   SEQ_MAXSIMD = (SIMD_AVX << 1) - 1;
  // if (env && std::string(env) == "AVX512")
  //   SEQ_MAXSIMD = (SIMD_AVX512F << 1) - 1;
  // if (env && std::string(env) == "SSE4_2")
  //   SEQ_MAXSIMD = (SIMD_SSE4_2 << 1) - 1;

  int flag = 0, cpuid[4], max_id;
  __cpuidex(cpuid, 0, 0);
  max_id = cpuid[0];
  if (max_id == 0)
    return 0;
  __cpuidex(cpuid, 1, 0);
  if (cpuid[3] >> 25 & 1)
    flag |= SIMD_SSE & SEQ_MAXSIMD;
  if (cpuid[3] >> 26 & 1)
    flag |= SIMD_SSE2 & SEQ_MAXSIMD;
  if (cpuid[2] >> 0 & 1)
    flag |= SIMD_SSE3 & SEQ_MAXSIMD;
  if (cpuid[2] >> 9 & 1)
    flag |= SIMD_SSSE3 & SEQ_MAXSIMD;
  if (cpuid[2] >> 19 & 1)
    flag |= SIMD_SSE4_1 & SEQ_MAXSIMD;
  if (cpuid[2] >> 20 & 1)
    flag |= SIMD_SSE4_2 & SEQ_MAXSIMD;
  if (cpuid[2] >> 28 & 1)
    flag |= SIMD_AVX & SEQ_MAXSIMD;
  if (max_id >= 7) {
    __cpuidex(cpuid, 7, 0);
    if (cpuid[1] >> 5 & 1)
      flag |= SIMD_AVX2 & SEQ_MAXSIMD;
    if (cpuid[1] >> 16 & 1)
      flag |= SIMD_AVX512F & SEQ_MAXSIMD;
  }
  return flag;
}

SEQ_FUNC seq_str_t seq_get_interaln_simd() {
  if (intersw_simd < 0)
    intersw_simd = x86_simd();
  if (intersw_simd & SIMD_AVX512F) {
    return string_conv("%s", 10, "AVX512");
  } else if (intersw_simd & SIMD_AVX2) {
    return string_conv("%s", 10, "AVX2");
  } else if (intersw_simd & SIMD_SSE4_1) {
    return string_conv("%s", 10, "SSE4_1");
  } else {
    return string_conv("%s", 10, "NONE");
  }
}

SEQ_FUNC void seq_set_sw_maxsimd(int max) {
  SEQ_MAXSIMD = (max << 1) - 1;
  intersw_simd = x86_simd();
}

struct InterAlignParams { // must be consistent with bio/align.seq
  int8_t a;
  int8_t b;
  int8_t ambig;
  int8_t gapo;
  int8_t gape;
  int8_t score_only;
  int32_t bandwidth;
  int32_t zdrop;
  int32_t end_bonus;
};

template <typename SW8, typename SWbt8>
static inline void seq_inter_align128_generic(InterAlignParams *paramsx,
                                              SeqPair *seqPairArray, uint8_t *seqBufRef,
                                              uint8_t *seqBufQer, int numPairs) {
  InterAlignParams params = *paramsx;
  const int8_t bandwidth =
      (0 <= params.bandwidth && params.bandwidth < 0xff) ? params.bandwidth : 0x7f;
  const int8_t zdrop = (0 <= params.zdrop && params.zdrop < 0xff) ? params.zdrop : 0x7f;
  if (params.score_only) {
    SW8 bsw(params.gapo, params.gape, params.gapo, params.gape, zdrop, params.end_bonus,
            params.a, params.b, params.ambig);
    bsw.SW(seqPairArray, seqBufRef, seqBufQer, numPairs, bandwidth);
  } else {
    SWbt8 bsw(params.gapo, params.gape, params.gapo, params.gape, zdrop,
              params.end_bonus, params.a, params.b, params.ambig);
    bsw.SW(seqPairArray, seqBufRef, seqBufQer, numPairs, bandwidth);
  }
}

template <typename SW16, typename SWbt16>
static inline void seq_inter_align16_generic(InterAlignParams *paramsx,
                                             SeqPair *seqPairArray, uint8_t *seqBufRef,
                                             uint8_t *seqBufQer, int numPairs) {
  InterAlignParams params = *paramsx;
  const int16_t bandwidth =
      (0 <= params.bandwidth && params.bandwidth < 0xffff) ? params.bandwidth : 0x7fff;
  const int16_t zdrop =
      (0 <= params.zdrop && params.zdrop < 0xffff) ? params.zdrop : 0x7fff;
  if (params.score_only) {
    SW16 bsw(params.gapo, params.gape, params.gapo, params.gape, zdrop,
             params.end_bonus, params.a, params.b, params.ambig);
    bsw.SW(seqPairArray, seqBufRef, seqBufQer, numPairs, bandwidth);
  } else {
    SWbt16 bsw(params.gapo, params.gape, params.gapo, params.gape, zdrop,
               params.end_bonus, params.a, params.b, params.ambig);
    bsw.SW(seqPairArray, seqBufRef, seqBufQer, numPairs, bandwidth);
  }
}

SEQ_FUNC void seq_inter_align1(InterAlignParams *paramsx, SeqPair *seqPairArray,
                               uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<128, 8, /*CIGAR=*/false> SW8;
  InterAlignParams params = *paramsx;
  int8_t a = params.a > 0 ? params.a : -params.a;
  int8_t b = params.b > 0 ? -params.b : params.b;
  int8_t ambig = params.ambig > 0 ? -params.ambig : params.ambig;
  int8_t mat[] = {a, b,     b, b, ambig, b, a,     b,     b,     ambig, b,     b,    a,
                  b, ambig, b, b, b,     a, ambig, ambig, ambig, ambig, ambig, ambig};
  ksw_extz_t ez;
  int flags = params.score_only ? KSW_EZ_SCORE_ONLY : 0;
  for (int i = 0; i < numPairs; i++) {
    SeqPair *sp = &seqPairArray[i];
    int myflags = flags | sp->flags;
    ksw_reset_extz(&ez);
    ksw_extz2_sse(nullptr, sp->len2, seqBufQer + SW8::LEN_LIMIT * sp->id, sp->len1,
                  seqBufRef + SW8::LEN_LIMIT * sp->id, /*m=*/5, mat, params.gapo,
                  params.gape, params.bandwidth, params.zdrop, params.end_bonus,
                  myflags, &ez);
    sp->score = (myflags & KSW_EZ_EXTZ_ONLY) ? ez.max : ez.score;
    sp->cigar = ez.cigar;
    sp->n_cigar = ez.n_cigar;
  }
}

void seq_inter_align128_scalar(InterAlignParams *paramsx, SeqPair *seqPairArray,
                               uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  seq_inter_align1(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
}

void seq_inter_align128_sse2(InterAlignParams *paramsx, SeqPair *seqPairArray,
                             uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<128, 8, /*CIGAR=*/false> SW8;
  typedef InterSW<128, 8, /*CIGAR=*/true> SWbt8;
  seq_inter_align128_generic<SW8, SWbt8>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                         numPairs);
}

void seq_inter_align128_avx2(InterAlignParams *paramsx, SeqPair *seqPairArray,
                             uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<256, 8, /*CIGAR=*/false> SW8;
  typedef InterSW<256, 8, /*CIGAR=*/true> SWbt8;
  seq_inter_align128_generic<SW8, SWbt8>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                         numPairs);
}

void seq_inter_align128_avx512(InterAlignParams *paramsx, SeqPair *seqPairArray,
                               uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<512, 8, /*CIGAR=*/false> SW8;
  typedef InterSW<512, 8, /*CIGAR=*/true> SWbt8;
  seq_inter_align128_generic<SW8, SWbt8>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                         numPairs);
}

SEQ_FUNC void seq_inter_align128(InterAlignParams *paramsx, SeqPair *seqPairArray,
                                 uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  if (intersw_simd < 0)
    intersw_simd = x86_simd();
  if (intersw_simd & SIMD_AVX512F) {
    seq_inter_align128_avx512(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else if (intersw_simd & SIMD_AVX2) {
    seq_inter_align128_avx2(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else if (intersw_simd & SIMD_SSE4_1) {
    seq_inter_align128_sse2(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else {
    seq_inter_align128_scalar(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  }
}

void seq_inter_align16_scalar(InterAlignParams *paramsx, SeqPair *seqPairArray,
                              uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  seq_inter_align1(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
}

void seq_inter_align16_sse2(InterAlignParams *paramsx, SeqPair *seqPairArray,
                            uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<128, 16, /*CIGAR=*/false> SW16;
  typedef InterSW<128, 16, /*CIGAR=*/true> SWbt16;
  seq_inter_align16_generic<SW16, SWbt16>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                          numPairs);
}

void seq_inter_align16_avx2(InterAlignParams *paramsx, SeqPair *seqPairArray,
                            uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<256, 16, /*CIGAR=*/false> SW16;
  typedef InterSW<256, 16, /*CIGAR=*/true> SWbt16;
  seq_inter_align16_generic<SW16, SWbt16>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                          numPairs);
}

void seq_inter_align16_avx512(InterAlignParams *paramsx, SeqPair *seqPairArray,
                              uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  typedef InterSW<512, 16, /*CIGAR=*/false> SW16;
  typedef InterSW<512, 16, /*CIGAR=*/true> SWbt16;
  seq_inter_align16_generic<SW16, SWbt16>(paramsx, seqPairArray, seqBufRef, seqBufQer,
                                          numPairs);
}

SEQ_FUNC void seq_inter_align16(InterAlignParams *paramsx, SeqPair *seqPairArray,
                                uint8_t *seqBufRef, uint8_t *seqBufQer, int numPairs) {
  if (intersw_simd < 0)
    intersw_simd = x86_simd();
  if (intersw_simd & SIMD_AVX512F) {
    seq_inter_align16_avx512(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else if (intersw_simd & SIMD_AVX2) {
    seq_inter_align16_avx2(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else if (intersw_simd & SIMD_SSE4_1) {
    seq_inter_align16_sse2(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  } else {
    seq_inter_align16_scalar(paramsx, seqPairArray, seqBufRef, seqBufQer, numPairs);
  }
}
