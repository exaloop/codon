#include "ksw2.h"
#include <cassert>
#include <cstring>

#ifdef __SSE2__
#include <emmintrin.h>

#ifdef KSW_SSE2_ONLY
#undef __SSE4_1__
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef KSW_CPU_DISPATCH
#ifdef __SSE4_1__
void ksw_extz2_sse41(void *km, int qlen, const uint8_t *query, int tlen,
                     const uint8_t *target, int8_t m, const int8_t *mat, int8_t q,
                     int8_t e, int w, int zdrop, int end_bonus, int flag,
                     ksw_extz_t *ez)
#else
void ksw_extz2_sse2(void *km, int qlen, const uint8_t *query, int tlen,
                    const uint8_t *target, int8_t m, const int8_t *mat, int8_t q,
                    int8_t e, int w, int zdrop, int end_bonus, int flag, ksw_extz_t *ez)
#endif
#else
void ksw_extz2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                   const uint8_t *target, int8_t m, const int8_t *mat, int8_t q,
                   int8_t e, int w, int zdrop, int end_bonus, int flag, ksw_extz_t *ez)
#endif // ~KSW_CPU_DISPATCH
{
#define __dp_code_block1                                                               \
  z = _mm_add_epi8(_mm_load_si128(&s[t]), qe2_);                                       \
  xt1 = _mm_load_si128(&x[t]);                     /* xt1 <- x[r-1][t..t+15] */        \
  tmp = _mm_srli_si128(xt1, 15);                   /* tmp <- x[r-1][t+15] */           \
  xt1 = _mm_or_si128(_mm_slli_si128(xt1, 1), x1_); /* xt1 <- x[r-1][t-1..t+14] */      \
  x1_ = tmp;                                                                           \
  vt1 = _mm_load_si128(&v[t]);                     /* vt1 <- v[r-1][t..t+15] */        \
  tmp = _mm_srli_si128(vt1, 15);                   /* tmp <- v[r-1][t+15] */           \
  vt1 = _mm_or_si128(_mm_slli_si128(vt1, 1), v1_); /* vt1 <- v[r-1][t-1..t+14] */      \
  v1_ = tmp;                                                                           \
  a = _mm_add_epi8(xt1, vt1); /* a <- x[r-1][t-1..t+14] + v[r-1][t-1..t+14] */         \
  ut = _mm_load_si128(&u[t]); /* ut <- u[t..t+15] */                                   \
  b = _mm_add_epi8(_mm_load_si128(&y[t]),                                              \
                   ut); /* b <- y[r-1][t..t+15] + u[r-1][t..t+15] */

#define __dp_code_block2                                                               \
  z = _mm_max_epu8(z,                                                                  \
                   b); /* z = max(z, b); this works because both are non-negative */   \
  z = _mm_min_epu8(z, max_sc_);                                                        \
  _mm_store_si128(&u[t],                                                               \
                  _mm_sub_epi8(z, vt1)); /* u[r][t..t+15] <- z - v[r-1][t-1..t+14] */  \
  _mm_store_si128(&v[t],                                                               \
                  _mm_sub_epi8(z, ut)); /* v[r][t..t+15] <- z - u[r-1][t..t+15] */     \
  z = _mm_sub_epi8(z, q_);                                                             \
  a = _mm_sub_epi8(a, z);                                                              \
  b = _mm_sub_epi8(b, z);

  int r, t, qe = q + e, n_col_, *off = 0, *off_end = 0, tlen_, qlen_, last_st, last_en,
            wl, wr, max_sc, min_sc;
  int with_cigar = !(flag & KSW_EZ_SCORE_ONLY),
      approx_max = !!(flag & KSW_EZ_APPROX_MAX);
  int32_t *H = 0, H0 = 0, last_H0_t = 0;
  uint8_t *qr, *sf, *mem, *mem2 = 0;
  __m128i q_, qe2_, zero_, flag1_, flag2_, flag8_, flag16_, sc_mch_, sc_mis_, sc_N_,
      m1_, max_sc_;
  __m128i *u, *v, *x, *y, *s, *p = 0;

  ksw_reset_extz(ez);
  if (m <= 0 || qlen <= 0 || tlen <= 0)
    return;

  zero_ = _mm_set1_epi8(0);
  q_ = _mm_set1_epi8(q);
  qe2_ = _mm_set1_epi8((q + e) * 2);
  flag1_ = _mm_set1_epi8(1);
  flag2_ = _mm_set1_epi8(2);
  flag8_ = _mm_set1_epi8(0x08);
  flag16_ = _mm_set1_epi8(0x10);
  sc_mch_ = _mm_set1_epi8(mat[0]);
  sc_mis_ = _mm_set1_epi8(mat[1]);
  sc_N_ = mat[m * m - 1] == 0 ? _mm_set1_epi8(-e) : _mm_set1_epi8(mat[m * m - 1]);
  m1_ = _mm_set1_epi8(m - 1); // wildcard
  max_sc_ = _mm_set1_epi8(mat[0] + (q + e) * 2);

  if (w < 0)
    w = tlen > qlen ? tlen : qlen;
  wl = wr = w;
  tlen_ = (tlen + 15) / 16;
  n_col_ = qlen < tlen ? qlen : tlen;
  n_col_ = ((n_col_ < w + 1 ? n_col_ : w + 1) + 15) / 16 + 1;
  qlen_ = (qlen + 15) / 16;
  for (t = 1, max_sc = mat[0], min_sc = mat[1]; t < m * m; ++t) {
    max_sc = max_sc > mat[t] ? max_sc : mat[t];
    min_sc = min_sc < mat[t] ? min_sc : mat[t];
  }
  if (-min_sc > 2 * (q + e))
    return; // otherwise, we won't see any mismatches

  mem = (uint8_t *)kcalloc(km, tlen_ * 6 + qlen_ + 1, 16);
  u = (__m128i *)(((size_t)mem + 15) >> 4 << 4); // 16-byte aligned
  v = u + tlen_, x = v + tlen_, y = x + tlen_, s = y + tlen_,
  sf = (uint8_t *)(s + tlen_), qr = sf + tlen_ * 16;
  if (!approx_max) {
    H = (int32_t *)kmalloc(km, tlen_ * 16 * 4);
    for (t = 0; t < tlen_ * 16; ++t)
      H[t] = KSW_NEG_INF;
  }
  if (with_cigar) {
    mem2 = (uint8_t *)kmalloc(km, ((size_t)(qlen + tlen - 1) * n_col_ + 1) * 16);
    p = (__m128i *)(((size_t)mem2 + 15) >> 4 << 4);
    off = (int *)kmalloc(km, (qlen + tlen - 1) * sizeof(int) * 2);
    off_end = off + qlen + tlen - 1;
  }

  for (t = 0; t < qlen; ++t)
    qr[t] = query[qlen - 1 - t];
  memcpy(sf, target, tlen);

  for (r = 0, last_st = last_en = -1; r < qlen + tlen - 1; ++r) {
    int st = 0, en = tlen - 1, st0, en0, st_, en_;
    int8_t x1, v1;
    uint8_t *qrr = qr + (qlen - 1 - r), *u8 = (uint8_t *)u, *v8 = (uint8_t *)v;
    __m128i x1_, v1_;
    // find the boundaries
    if (st < r - qlen + 1)
      st = r - qlen + 1;
    if (en > r)
      en = r;
    if (st < ((r - wr + 1) >> 1))
      st = (r - wr + 1) >> 1; // take the ceil
    if (en > (r + wl) >> 1)
      en = (r + wl) >> 1; // take the floor
    if (st > en) {
      ez->zdropped = 1;
      break;
    }
    st0 = st, en0 = en;
    st = st / 16 * 16, en = (en + 16) / 16 * 16 - 1;
    // set boundary conditions
    if (st > 0) {
      if (st - 1 >= last_st && st - 1 <= last_en)
        x1 = ((uint8_t *)x)[st - 1],
        v1 = v8[st - 1]; // (r-1,s-1) calculated in the last round
      else
        x1 = v1 = 0; // not calculated; set to zeros
    } else
      x1 = 0, v1 = r ? q : 0;
    if (en >= r)
      ((uint8_t *)y)[r] = 0, u8[r] = r ? q : 0;
    // loop fission: set scores first
    if (!(flag & KSW_EZ_GENERIC_SC)) {
      for (t = st0; t <= en0; t += 16) {
        __m128i sq, st, tmp, mask;
        sq = _mm_loadu_si128((__m128i *)&sf[t]);
        st = _mm_loadu_si128((__m128i *)&qrr[t]);
        mask = _mm_or_si128(_mm_cmpeq_epi8(sq, m1_), _mm_cmpeq_epi8(st, m1_));
        tmp = _mm_cmpeq_epi8(sq, st);
#ifdef __SSE4_1__
        tmp = _mm_blendv_epi8(sc_mis_, sc_mch_, tmp);
        tmp = _mm_blendv_epi8(tmp, sc_N_, mask);
#else
        tmp = _mm_or_si128(_mm_andnot_si128(tmp, sc_mis_), _mm_and_si128(tmp, sc_mch_));
        tmp = _mm_or_si128(_mm_andnot_si128(mask, tmp), _mm_and_si128(mask, sc_N_));
#endif
        _mm_storeu_si128((__m128i *)((uint8_t *)s + t), tmp);
      }
    } else {
      for (t = st0; t <= en0; ++t)
        ((uint8_t *)s)[t] = mat[sf[t] * m + qrr[t]];
    }
    // core loop
    x1_ = _mm_cvtsi32_si128(x1);
    v1_ = _mm_cvtsi32_si128(v1);
    st_ = st / 16, en_ = en / 16;
    assert(en_ - st_ + 1 <= n_col_);
    if (!with_cigar) { // score only
      for (t = st_; t <= en_; ++t) {
        __m128i z, a, b, xt1, vt1, ut, tmp;
        __dp_code_block1;
#ifdef __SSE4_1__
        z = _mm_max_epi8(z, a); // z = z > a? z : a (signed)
#else                           // we need to emulate SSE4.1 intrinsics _mm_max_epi8()
        z = _mm_and_si128(z, _mm_cmpgt_epi8(z,
                                            zero_)); // z = z > 0? z : 0;
        z = _mm_max_epu8(z,
                         a); // z = max(z, a); this works because both are non-negative
#endif
        __dp_code_block2;
#ifdef __SSE4_1__
        _mm_store_si128(&x[t], _mm_max_epi8(a, zero_));
        _mm_store_si128(&y[t], _mm_max_epi8(b, zero_));
#else
        tmp = _mm_cmpgt_epi8(a, zero_);
        _mm_store_si128(&x[t], _mm_and_si128(a, tmp));
        tmp = _mm_cmpgt_epi8(b, zero_);
        _mm_store_si128(&y[t], _mm_and_si128(b, tmp));
#endif
      }
    } else if (!(flag & KSW_EZ_RIGHT)) { // gap left-alignment
      __m128i *pr = p + (size_t)r * n_col_ - st_;
      off[r] = st, off_end[r] = en;
      for (t = st_; t <= en_; ++t) {
        __m128i d, z, a, b, xt1, vt1, ut, tmp;
        __dp_code_block1;
        d = _mm_and_si128(_mm_cmpgt_epi8(a, z),
                          flag1_); // d = a > z? 1 : 0
#ifdef __SSE4_1__
        z = _mm_max_epi8(z, a); // z = z > a? z : a (signed)
        tmp = _mm_cmpgt_epi8(b, z);
        d = _mm_blendv_epi8(d, flag2_,
                            tmp); // d = b > z? 2 : d
#else // we need to emulate SSE4.1 intrinsics _mm_max_epi8() and
      // _mm_blendv_epi8()
        z = _mm_and_si128(z, _mm_cmpgt_epi8(z,
                                            zero_)); // z = z > 0? z : 0;
        z = _mm_max_epu8(z,
                         a); // z = max(z, a); this works because both are non-negative
        tmp = _mm_cmpgt_epi8(b, z);
        d = _mm_or_si128(_mm_andnot_si128(tmp, d),
                         _mm_and_si128(tmp,
                                       flag2_)); // d = b > z? 2 : d; emulating blendv
#endif
        __dp_code_block2;
        tmp = _mm_cmpgt_epi8(a, zero_);
        _mm_store_si128(&x[t], _mm_and_si128(tmp, a));
        d = _mm_or_si128(d, _mm_and_si128(tmp,
                                          flag8_)); // d = a > 0? 0x08 : 0
        tmp = _mm_cmpgt_epi8(b, zero_);
        _mm_store_si128(&y[t], _mm_and_si128(tmp, b));
        d = _mm_or_si128(d, _mm_and_si128(tmp,
                                          flag16_)); // d = b > 0? 0x10 : 0
        _mm_store_si128(&pr[t], d);
      }
    } else { // gap right-alignment
      __m128i *pr = p + (size_t)r * n_col_ - st_;
      off[r] = st, off_end[r] = en;
      for (t = st_; t <= en_; ++t) {
        __m128i d, z, a, b, xt1, vt1, ut, tmp;
        __dp_code_block1;
        d = _mm_andnot_si128(_mm_cmpgt_epi8(z, a),
                             flag1_); // d = z > a? 0 : 1
#ifdef __SSE4_1__
        z = _mm_max_epi8(z, a); // z = z > a? z : a (signed)
        tmp = _mm_cmpgt_epi8(z, b);
        d = _mm_blendv_epi8(flag2_, d,
                            tmp); // d = z > b? d : 2
#else // we need to emulate SSE4.1 intrinsics _mm_max_epi8() and
      // _mm_blendv_epi8()
        z = _mm_and_si128(z, _mm_cmpgt_epi8(z,
                                            zero_)); // z = z > 0? z : 0;
        z = _mm_max_epu8(z,
                         a); // z = max(z, a); this works because both are non-negative
        tmp = _mm_cmpgt_epi8(z, b);
        d = _mm_or_si128(_mm_andnot_si128(tmp, flag2_),
                         _mm_and_si128(tmp,
                                       d)); // d = z > b? d : 2; emulating blendv
#endif
        __dp_code_block2;
        tmp = _mm_cmpgt_epi8(zero_, a);
        _mm_store_si128(&x[t], _mm_andnot_si128(tmp, a));
        d = _mm_or_si128(d, _mm_andnot_si128(tmp,
                                             flag8_)); // d = 0 > a? 0 : 0x08
        tmp = _mm_cmpgt_epi8(zero_, b);
        _mm_store_si128(&y[t], _mm_andnot_si128(tmp, b));
        d = _mm_or_si128(d, _mm_andnot_si128(tmp,
                                             flag16_)); // d = 0 > b? 0 : 0x10
        _mm_store_si128(&pr[t], d);
      }
    }
    if (!approx_max) { // find the exact max with a 32-bit score array
      int32_t max_H, max_t;
      // compute H[], max_H and max_t
      if (r > 0) {
        int32_t HH[4], tt[4], en1 = st0 + (en0 - st0) / 4 * 4, i;
        __m128i max_H_, max_t_, qe_;
        max_H = H[en0] = en0 > 0
                             ? H[en0 - 1] + u8[en0] - qe
                             : H[en0] + v8[en0] - qe; // special casing the last element
        max_t = en0;
        max_H_ = _mm_set1_epi32(max_H);
        max_t_ = _mm_set1_epi32(max_t);
        qe_ = _mm_set1_epi32(q + e);
        for (t = st0; t < en1; t += 4) { // this implements: H[t]+=v8[t]-qe;
                                         // if(H[t]>max_H) max_H=H[t],max_t=t;
          __m128i H1, tmp, t_;
          H1 = _mm_loadu_si128((__m128i *)&H[t]);
          t_ = _mm_setr_epi32(v8[t], v8[t + 1], v8[t + 2], v8[t + 3]);
          H1 = _mm_add_epi32(H1, t_);
          H1 = _mm_sub_epi32(H1, qe_);
          _mm_storeu_si128((__m128i *)&H[t], H1);
          t_ = _mm_set1_epi32(t);
          tmp = _mm_cmpgt_epi32(H1, max_H_);
#ifdef __SSE4_1__
          max_H_ = _mm_blendv_epi8(max_H_, H1, tmp);
          max_t_ = _mm_blendv_epi8(max_t_, t_, tmp);
#else
          max_H_ = _mm_or_si128(_mm_and_si128(tmp, H1), _mm_andnot_si128(tmp, max_H_));
          max_t_ = _mm_or_si128(_mm_and_si128(tmp, t_), _mm_andnot_si128(tmp, max_t_));
#endif
        }
        _mm_storeu_si128((__m128i *)HH, max_H_);
        _mm_storeu_si128((__m128i *)tt, max_t_);
        for (i = 0; i < 4; ++i)
          if (max_H < HH[i])
            max_H = HH[i], max_t = tt[i] + i;
        for (; t < en0; ++t) { // for the rest of values that haven't been
                               // computed with SSE
          H[t] += (int32_t)v8[t] - qe;
          if (H[t] > max_H)
            max_H = H[t], max_t = t;
        }
      } else
        H[0] = v8[0] - qe - qe, max_H = H[0],
        max_t = 0; // special casing r==0
      // update ez
      if (en0 == tlen - 1 && H[en0] > ez->mte)
        ez->mte = H[en0], ez->mte_q = r - en;
      if (r - st0 == qlen - 1 && H[st0] > ez->mqe)
        ez->mqe = H[st0], ez->mqe_t = st0;
      if (ksw_apply_zdrop(ez, 1, max_H, r, max_t, zdrop, e))
        break;
      if (r == qlen + tlen - 2 && en0 == tlen - 1)
        ez->score = H[tlen - 1];
    } else { // find approximate max; Z-drop might be inaccurate, too.
      if (r > 0) {
        if (last_H0_t >= st0 && last_H0_t <= en0 && last_H0_t + 1 >= st0 &&
            last_H0_t + 1 <= en0) {
          int32_t d0 = v8[last_H0_t] - qe;
          int32_t d1 = u8[last_H0_t + 1] - qe;
          if (d0 > d1)
            H0 += d0;
          else
            H0 += d1, ++last_H0_t;
        } else if (last_H0_t >= st0 && last_H0_t <= en0) {
          H0 += v8[last_H0_t] - qe;
        } else {
          ++last_H0_t, H0 += u8[last_H0_t] - qe;
        }
        if ((flag & KSW_EZ_APPROX_DROP) &&
            ksw_apply_zdrop(ez, 1, H0, r, last_H0_t, zdrop, e))
          break;
      } else
        H0 = v8[0] - qe - qe, last_H0_t = 0;
      if (r == qlen + tlen - 2 && en0 == tlen - 1)
        ez->score = H0;
    }
    last_st = st, last_en = en;
  }
  kfree(km, mem);
  if (!approx_max)
    kfree(km, H);
  if (with_cigar) { // backtrack
    int rev_cigar = !!(flag & KSW_EZ_REV_CIGAR);
    if (!ez->zdropped && !(flag & KSW_EZ_EXTZ_ONLY)) {
      ksw_backtrack(km, 1, rev_cigar, 0, (uint8_t *)p, off, off_end, n_col_ * 16,
                    tlen - 1, qlen - 1, &ez->m_cigar, &ez->n_cigar, &ez->cigar);
    } else if (!ez->zdropped && (flag & KSW_EZ_EXTZ_ONLY) &&
               ez->mqe + end_bonus > (int)ez->max) {
      ez->reach_end = 1;
      ksw_backtrack(km, 1, rev_cigar, 0, (uint8_t *)p, off, off_end, n_col_ * 16,
                    ez->mqe_t, qlen - 1, &ez->m_cigar, &ez->n_cigar, &ez->cigar);
    } else if (ez->max_t >= 0 && ez->max_q >= 0) {
      ksw_backtrack(km, 1, rev_cigar, 0, (uint8_t *)p, off, off_end, n_col_ * 16,
                    ez->max_t, ez->max_q, &ez->m_cigar, &ez->n_cigar, &ez->cigar);
    }
    kfree(km, mem2);
    kfree(km, off);
  }
}
#endif // __SSE2__
