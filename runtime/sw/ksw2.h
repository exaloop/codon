#ifndef KSW2_H_
#define KSW2_H_

#include <cstddef>
#include <cstdint>

#define KSW_NEG_INF (-0x40000000)

#define KSW_EZ_SCORE_ONLY 0x01 // don't record alignment path/cigar
#define KSW_EZ_RIGHT 0x02      // right-align gaps
#define KSW_EZ_GENERIC_SC                                                              \
  0x04 // without this flag: match/mismatch only; last symbol is a wildcard
#define KSW_EZ_APPROX_MAX 0x08  // approximate max; this is faster with sse
#define KSW_EZ_APPROX_DROP 0x10 // approximate Z-drop; faster with sse
#define KSW_EZ_EXTZ_ONLY 0x40   // only perform extension
#define KSW_EZ_REV_CIGAR 0x80   // reverse CIGAR in the output
#define KSW_EZ_SPLICE_FOR 0x100
#define KSW_EZ_SPLICE_REV 0x200
#define KSW_EZ_SPLICE_FLANK 0x400

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t max : 31, zdropped : 1;
  int max_q, max_t; // max extension coordinate
  int mqe, mqe_t;   // max score when reaching the end of query
  int mte, mte_q;   // max score when reaching the end of target
  int score;        // max score reaching both ends; may be KSW_NEG_INF
  int m_cigar, n_cigar;
  int reach_end;
  uint32_t *cigar;
} ksw_extz_t;

/**
 * NW-like extension
 *
 * @param km        memory pool, when used with kalloc
 * @param qlen      query length
 * @param query     query sequence with 0 <= query[i] < m
 * @param tlen      target length
 * @param target    target sequence with 0 <= target[i] < m
 * @param m         number of residue types
 * @param mat       m*m scoring mattrix in one-dimension array
 * @param gapo      gap open penalty; a gap of length l cost "-(gapo+l*gape)"
 * @param gape      gap extension penalty
 * @param w         band width (<0 to disable)
 * @param zdrop     off-diagonal drop-off to stop extension (positive; <0 to
 * disable)
 * @param flag      flag (see KSW_EZ_* macros)
 * @param ez        (out) scores and cigar
 */
void ksw_extz(void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target,
              int8_t m, const int8_t *mat, int8_t q, int8_t e, int w, int zdrop,
              int flag, ksw_extz_t *ez);

void ksw_extz2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                   const uint8_t *target, int8_t m, const int8_t *mat, int8_t q,
                   int8_t e, int w, int zdrop, int end_bonus, int flag, ksw_extz_t *ez);

void ksw_extd(void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target,
              int8_t m, const int8_t *mat, int8_t gapo, int8_t gape, int8_t gapo2,
              int8_t gape2, int w, int zdrop, int flag, ksw_extz_t *ez);

void ksw_extd2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                   const uint8_t *target, int8_t m, const int8_t *mat, int8_t gapo,
                   int8_t gape, int8_t gapo2, int8_t gape2, int w, int zdrop,
                   int end_bonus, int flag, ksw_extz_t *ez);

void ksw_exts2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                   const uint8_t *target, int8_t m, const int8_t *mat, int8_t gapo,
                   int8_t gape, int8_t gapo2, int8_t noncan, int zdrop, int flag,
                   ksw_extz_t *ez);

void ksw_extf2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                   const uint8_t *target, int8_t mch, int8_t mis, int8_t e, int w,
                   int xdrop, ksw_extz_t *ez);

/**
 * Global alignment
 *
 * (first 10 parameters identical to ksw_extz_sse())
 * @param m_cigar   (modified) max CIGAR length; feed 0 if cigar==0
 * @param n_cigar   (out) number of CIGAR elements
 * @param cigar     (out) BAM-encoded CIGAR; caller need to deallocate with
 * kfree(km, )
 *
 * @return          score of the alignment
 */
int ksw_gg(void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target,
           int8_t m, const int8_t *mat, int8_t gapo, int8_t gape, int w, int *m_cigar_,
           int *n_cigar_, uint32_t **cigar_);
int ksw_gg2(void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target,
            int8_t m, const int8_t *mat, int8_t gapo, int8_t gape, int w, int *m_cigar_,
            int *n_cigar_, uint32_t **cigar_);
int ksw_gg2_sse(void *km, int qlen, const uint8_t *query, int tlen,
                const uint8_t *target, int8_t m, const int8_t *mat, int8_t gapo,
                int8_t gape, int w, int *m_cigar_, int *n_cigar_, uint32_t **cigar_);

void *ksw_ll_qinit(void *km, int size, int qlen, const uint8_t *query, int m,
                   const int8_t *mat);
int ksw_ll_i16(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe,
               int *te);

#ifdef __cplusplus
}
#endif

/************************************
 *** Private macros and functions ***
 ************************************/

extern "C" void *seq_alloc_atomic(size_t n);
extern "C" void *seq_calloc_atomic(size_t m, size_t n);
extern "C" void *seq_realloc(void *p, size_t n);
extern "C" void seq_free(void *p);
#define kmalloc(km, size) seq_alloc_atomic((size))
#define kcalloc(km, count, size) seq_calloc_atomic((count), (size))
#define krealloc(km, ptr, size) seq_realloc((ptr), (size))
#define kfree(km, ptr) seq_free((ptr))

static inline uint32_t *ksw_push_cigar(void *km, int *n_cigar, int *m_cigar,
                                       uint32_t *cigar, uint32_t op, int len) {
  if (*n_cigar == 0 || op != (cigar[(*n_cigar) - 1] & 0xf)) {
    if (*n_cigar == *m_cigar) {
      *m_cigar = *m_cigar ? (*m_cigar) << 1 : 4;
      cigar = (uint32_t *)((*n_cigar) ? krealloc(km, cigar, (*m_cigar) << 2)
                                      : kmalloc(km, (*m_cigar) << 2));
    }
    cigar[(*n_cigar)++] = len << 4 | op;
  } else
    cigar[(*n_cigar) - 1] += len << 4;
  return cigar;
}

// In the backtrack matrix, value p[] has the following structure:
//   bit 0-2: which type gets the max - 0 for H, 1 for E, 2 for F, 3 for
//   \tilde{E} and 4 for \tilde{F} bit 3/0x08: 1 if a continuation on the E
//   state (bit 5/0x20 for a continuation on \tilde{E}) bit 4/0x10: 1 if a
//   continuation on the F state (bit 6/0x40 for a continuation on \tilde{F})
static inline void
ksw_backtrack(void *km, int is_rot, int is_rev, int min_intron_len, const uint8_t *p,
              const int *off, const int *off_end, int n_col, int i0, int j0,
              int *m_cigar_, int *n_cigar_,
              uint32_t **cigar_) { // p[] - lower 3 bits: which type gets the max; bit
  int n_cigar = 0, m_cigar = *m_cigar_, i = i0, j = j0, r, state = 0;
  uint32_t *cigar = *cigar_, tmp;
  while (i >= 0 && j >= 0) { // at the beginning of the loop, _state_ tells us
                             // which state to check
    int force_state = -1;
    if (is_rot) {
      r = i + j;
      if (i < off[r])
        force_state = 2;
      if (off_end && i > off_end[r])
        force_state = 1;
      tmp = force_state < 0 ? p[(size_t)r * n_col + i - off[r]] : 0;
    } else {
      if (j < off[i])
        force_state = 2;
      if (off_end && j > off_end[i])
        force_state = 1;
      tmp = force_state < 0 ? p[(size_t)i * n_col + j - off[i]] : 0;
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
      cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar, 0, 1), --i,
      --j; // match
    else if (state == 1 || (state == 3 && min_intron_len <= 0))
      cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar, 2, 1),
      --i; // deletion
    else if (state == 3 && min_intron_len > 0)
      cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar, 3, 1),
      --i; // intron
    else
      cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar, 1, 1),
      --j; // insertion
  }
  if (i >= 0)
    cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar,
                           min_intron_len > 0 && i >= min_intron_len ? 3 : 2,
                           i + 1); // first deletion
  if (j >= 0)
    cigar = ksw_push_cigar(km, &n_cigar, &m_cigar, cigar, 1,
                           j + 1); // first insertion
  if (!is_rev)
    for (i = 0; i < (n_cigar >> 1); ++i) // reverse CIGAR
      tmp = cigar[i], cigar[i] = cigar[n_cigar - 1 - i], cigar[n_cigar - 1 - i] = tmp;
  *m_cigar_ = m_cigar, *n_cigar_ = n_cigar, *cigar_ = cigar;
}

static inline void ksw_reset_extz(ksw_extz_t *ez) {
  ez->max_q = ez->max_t = ez->mqe_t = ez->mte_q = -1;
  ez->max = 0;
  ez->score = ez->mqe = ez->mte = KSW_NEG_INF;
  ez->cigar = nullptr;
  ez->n_cigar = ez->m_cigar = 0;
  ez->zdropped = 0;
  ez->reach_end = 0;
}

static inline int ksw_apply_zdrop(ksw_extz_t *ez, int is_rot, int32_t H, int a, int b,
                                  int zdrop, int8_t e) {
  int r, t;
  if (is_rot)
    r = a, t = b;
  else
    r = a + b, t = a;
  if (H > (int32_t)ez->max) {
    ez->max = H, ez->max_t = t, ez->max_q = r - t;
  } else if (t >= ez->max_t && r - t >= ez->max_q) {
    int tl = t - ez->max_t, ql = (r - t) - ez->max_q, l;
    l = tl > ql ? tl - ql : ql - tl;
    if (zdrop >= 0 && ez->max - H > zdrop + l * e) {
      ez->zdropped = 1;
      return 1;
    }
  }
  return 0;
}
#endif
