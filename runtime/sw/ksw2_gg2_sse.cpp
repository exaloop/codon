#include "ksw2.h"
#include <cstddef>

#ifdef __SSE2__
#include <emmintrin.h>

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

int ksw_gg2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                const uint8_t *target, int8_t m, const int8_t *mat, int8_t q, int8_t e,
                int w, int *m_cigar_, int *n_cigar_, uint32_t **cigar_) {
  int r, t, n_col, n_col_, *off, tlen_, last_st, last_en, H0 = 0, last_H0_t = 0;
  uint8_t *qr, *mem, *mem2;
  __m128i *u, *v, *x, *y, *s, *p;
  __m128i q_, qe2_, zero_, flag1_, flag2_, flag8_, flag16_;

  zero_ = _mm_set1_epi8(0);
  q_ = _mm_set1_epi8(q);
  qe2_ = _mm_set1_epi8((q + e) * 2);
  flag1_ = _mm_set1_epi8(1);
  flag2_ = _mm_set1_epi8(2);
  flag8_ = _mm_set1_epi8(0x08);
  flag16_ = _mm_set1_epi8(0x10);

  if (w < 0)
    w = tlen > qlen ? tlen : qlen;
  n_col = w + 1 < tlen ? w + 1 : tlen; // number of columns in the backtrack matrix
  tlen_ = (tlen + 15) / 16;
  n_col_ = (n_col + 15) / 16 + 1;
  n_col = n_col_ * 16;

  mem = (uint8_t *)kcalloc(km, tlen_ * 5 + 1, 16);
  u = (__m128i *)(((size_t)mem + 15) >> 4 << 4); // 16-byte aligned
  v = u + tlen_, x = v + tlen_, y = x + tlen_, s = y + tlen_;
  qr = (uint8_t *)kcalloc(km, qlen, 1);
  mem2 = (uint8_t *)kmalloc(km, ((qlen + tlen - 1) * n_col_ + 1) * 16);
  p = (__m128i *)(((size_t)mem2 + 15) >> 4 << 4);
  off = (int *)kmalloc(km, (qlen + tlen - 1) * sizeof(int));

  for (t = 0; t < qlen; ++t)
    qr[t] = query[qlen - 1 - t];

  for (r = 0, last_st = last_en = -1; r < qlen + tlen - 1; ++r) {
    int st = 0, en = tlen - 1, st0, en0, st_, en_;
    int8_t x1, v1;
    __m128i x1_, v1_, *pr;
    // find the boundaries
    if (st < r - qlen + 1)
      st = r - qlen + 1;
    if (en > r)
      en = r;
    if (st < ((r - w + 1) >> 1))
      st = (r - w + 1) >> 1; // take the ceil
    if (en > (r + w) >> 1)
      en = (r + w) >> 1; // take the floor
    st0 = st, en0 = en;
    st = st / 16 * 16, en = (en + 16) / 16 * 16 - 1;
    off[r] = st;
    // set boundary conditions
    if (st > 0) {
      if (st - 1 >= last_st && st - 1 <= last_en)
        x1 = ((uint8_t *)x)[st - 1],
        v1 = ((uint8_t *)v)[st - 1]; // (r-1,s-1) calculated in the last round
      else
        x1 = v1 = 0; // not calculated; set to zeros
    } else
      x1 = 0, v1 = r ? q : 0;
    if (en >= r)
      ((uint8_t *)y)[r] = 0, ((uint8_t *)u)[r] = r ? q : 0;
    // loop fission: set scores first
    for (t = st0; t <= en0; ++t)
      ((uint8_t *)s)[t] = mat[target[t] * m + qr[t + qlen - 1 - r]];
    // core loop
    x1_ = _mm_cvtsi32_si128(x1);
    v1_ = _mm_cvtsi32_si128(v1);
    st_ = st >> 4, en_ = en >> 4;
    pr = p + r * n_col_ - st_;
    for (t = st_; t <= en_; ++t) {
      __m128i d, z, a, b, xt1, vt1, ut, tmp;

      z = _mm_add_epi8(_mm_load_si128(&s[t]), qe2_);

      xt1 = _mm_load_si128(&x[t]);   // xt1 <- x[r-1][t..t+15]
      tmp = _mm_srli_si128(xt1, 15); // tmp <- x[r-1][t+15]
      xt1 = _mm_or_si128(_mm_slli_si128(xt1, 1),
                         x1_); // xt1 <- x[r-1][t-1..t+14]
      x1_ = tmp;
      vt1 = _mm_load_si128(&v[t]);   // vt1 <- v[r-1][t..t+15]
      tmp = _mm_srli_si128(vt1, 15); // tmp <- v[r-1][t+15]
      vt1 = _mm_or_si128(_mm_slli_si128(vt1, 1),
                         v1_); // vt1 <- v[r-1][t-1..t+14]
      v1_ = tmp;
      a = _mm_add_epi8(xt1,
                       vt1); // a <- x[r-1][t-1..t+14] + v[r-1][t-1..t+14]

      ut = _mm_load_si128(&u[t]); // ut <- u[t..t+15]
      b = _mm_add_epi8(_mm_load_si128(&y[t]),
                       ut); // b <- y[r-1][t..t+15] + u[r-1][t..t+15]

      d = _mm_and_si128(_mm_cmpgt_epi8(a, z),
                        flag1_); // d = a > z? 1 : 0
#ifdef __SSE4_1__
      z = _mm_max_epi8(z, a); // z = z > a? z : a (signed)
      tmp = _mm_cmpgt_epi8(b, z);
      d = _mm_blendv_epi8(d, flag2_,
                          tmp); // d = b > z? 2 : d
#else // we need to emulate SSE4.1 intrinsics _mm_max_epi8() and
      // _mm_blendv_epi8()
      z = _mm_and_si128(z, _mm_cmpgt_epi8(z, zero_)); // z = z > 0? z : 0;
      z = _mm_max_epu8(z,
                       a); // z = max(z, a); this works because both are non-negative
      tmp = _mm_cmpgt_epi8(b, z);
      d = _mm_or_si128(_mm_andnot_si128(tmp, d),
                       _mm_and_si128(tmp,
                                     flag2_)); // d = b > z? 2 : d; emulating blendv
#endif
      z = _mm_max_epu8(z,
                       b); // z = max(z, b); this works because both are non-negative
      _mm_store_si128(&u[t],
                      _mm_sub_epi8(z,
                                   vt1)); // u[r][t..t+15] <- z - v[r-1][t-1..t+14]
      _mm_store_si128(&v[t], _mm_sub_epi8(z,
                                          ut)); // v[r][t..t+15] <- z - u[r-1][t..t+15]

      z = _mm_sub_epi8(z, q_);
      a = _mm_sub_epi8(a, z);
      b = _mm_sub_epi8(b, z);
      tmp = _mm_cmpgt_epi8(a, zero_);
      d = _mm_or_si128(d, _mm_and_si128(flag8_, tmp));
      _mm_store_si128(&x[t], _mm_and_si128(a, tmp));
      tmp = _mm_cmpgt_epi8(b, zero_);
      d = _mm_or_si128(d, _mm_and_si128(flag16_, tmp));
      _mm_store_si128(&y[t], _mm_and_si128(b, tmp));
      _mm_store_si128(&pr[t], d);
    }
    if (r > 0) {
      if (last_H0_t >= st0 && last_H0_t <= en0)
        H0 += ((uint8_t *)v)[last_H0_t] - (q + e);
      else
        ++last_H0_t, H0 += ((uint8_t *)u)[last_H0_t] - (q + e);
    } else
      H0 = ((uint8_t *)v)[0] - 2 * (q + e), last_H0_t = 0;
    last_st = st, last_en = en;
  }
  kfree(km, mem);
  kfree(km, qr);
  ksw_backtrack(km, 1, 0, 0, (uint8_t *)p, off, 0, n_col, tlen - 1, qlen - 1, m_cigar_,
                n_cigar_, cigar_);
  kfree(km, mem2);
  kfree(km, off);
  return H0;
}
#endif // __SSE2__
