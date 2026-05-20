// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <unwind.h>

#define SEQ_FLAG_DEBUG (1 << 0)          // compiled/running in debug mode
#define SEQ_FLAG_CAPTURE_OUTPUT (1 << 1) // capture writes to stdout/stderr
#define SEQ_FLAG_STANDALONE (1 << 2)     // compiled as a standalone object/binary

#define SEQ_EXCEPTION_CLASS 0x6f626a0073657100

#define SEQ_FUNC extern "C"

typedef int64_t seq_int_t;

using CodonJITAddSymbolFunc = void (*)(void *, const char *, void *);

namespace re2 {
class RE2;
}

namespace hwy {
struct uint128_t;
}

using Regex = re2::RE2;

struct seq_str_t {
  seq_int_t len;
  char *str;
};

struct seq_time_t {
  int16_t year;
  int16_t yday;
  int8_t sec;
  int8_t min;
  int8_t hour;
  int8_t mday;
  int8_t mon;
  int8_t wday;
  int8_t isdst;
};

struct Span {
  seq_int_t start;
  seq_int_t end;
};

SEQ_FUNC int seq_flags;

SEQ_FUNC void seq_init(int flags);

SEQ_FUNC seq_int_t seq_pid();
SEQ_FUNC seq_int_t seq_time();
SEQ_FUNC seq_int_t seq_time_monotonic();
SEQ_FUNC seq_int_t seq_time_highres();
SEQ_FUNC bool seq_localtime(seq_int_t secs, seq_time_t *output);
SEQ_FUNC bool seq_gmtime(seq_int_t secs, seq_time_t *output);
SEQ_FUNC seq_int_t seq_mktime(seq_time_t *time);
SEQ_FUNC void seq_sleep(double secs);
SEQ_FUNC char **seq_env();
SEQ_FUNC void seq_assert_failed(seq_str_t file, seq_int_t line);

SEQ_FUNC void *seq_alloc(size_t n);
SEQ_FUNC void *seq_alloc_atomic(size_t n);
SEQ_FUNC void *seq_alloc_uncollectable(size_t n);
SEQ_FUNC void *seq_alloc_atomic_uncollectable(size_t n);
SEQ_FUNC void *seq_realloc(void *p, size_t newsize, size_t oldsize);
SEQ_FUNC void seq_free(void *p);
SEQ_FUNC void seq_register_finalizer(void *p, void (*f)(void *obj, void *data));

SEQ_FUNC void seq_gc_add_roots(void *start, void *end);
SEQ_FUNC void seq_gc_remove_roots(void *start, void *end);
SEQ_FUNC void seq_gc_clear_roots();
SEQ_FUNC void seq_gc_exclude_static_roots(void *start, void *end);

SEQ_FUNC void *seq_alloc_exc(void *obj);
SEQ_FUNC void seq_terminate(void *exc);
SEQ_FUNC void seq_throw(void *exc);
SEQ_FUNC _Unwind_Reason_Code seq_personality(int version, _Unwind_Action actions,
                                             uint64_t exceptionClass,
                                             _Unwind_Exception *exceptionObject,
                                             _Unwind_Context *context);
SEQ_FUNC int64_t seq_exc_offset();

SEQ_FUNC seq_str_t seq_str_int(seq_int_t n, seq_str_t format, bool *error);
SEQ_FUNC seq_str_t seq_str_uint(seq_int_t n, seq_str_t format, bool *error);
SEQ_FUNC seq_str_t seq_str_float(double f, seq_str_t format, bool *error);
SEQ_FUNC seq_str_t seq_str_ptr(void *p, seq_str_t format, bool *error);
SEQ_FUNC seq_str_t seq_str_str(seq_str_t s, seq_str_t format, bool *error);
SEQ_FUNC double seq_float_from_str(seq_str_t s, const char **e);
SEQ_FUNC seq_str_t seq_check_errno();

SEQ_FUNC void *seq_stdin();
SEQ_FUNC void *seq_stdout();
SEQ_FUNC void *seq_stderr();

SEQ_FUNC void seq_print(seq_str_t str);
SEQ_FUNC void seq_print_full(seq_str_t str, FILE *fo);

SEQ_FUNC void *seq_lock_new();
SEQ_FUNC bool seq_lock_acquire(void *lock, bool block, double timeout);
SEQ_FUNC void seq_lock_release(void *lock);
SEQ_FUNC void *seq_rlock_new();
SEQ_FUNC bool seq_rlock_acquire(void *lock, bool block, double timeout);
SEQ_FUNC void seq_rlock_release(void *lock);

SEQ_FUNC void cnp_acos_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_acos_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_acosh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_acosh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_asin_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_asin_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_asinh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_asinh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_atan_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_atan_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_atanh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_atanh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_atan2_float32(const float *in1, size_t is1, const float *in2,
                                size_t is2, float *out, size_t os, size_t n);
SEQ_FUNC void cnp_atan2_float64(const double *in1, size_t is1, const double *in2,
                                size_t is2, double *out, size_t os, size_t n);
SEQ_FUNC void cnp_cos_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_cos_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_exp_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_exp_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_exp2_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_exp2_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_expm1_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_expm1_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_log_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_log_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_log10_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_log10_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_log1p_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_log1p_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n);
SEQ_FUNC void cnp_log2_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_log2_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_sin_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_sin_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n);
SEQ_FUNC void cnp_sinh_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_sinh_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_tanh_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_tanh_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n);
SEQ_FUNC void cnp_hypot_float32(const float *in1, size_t is1, const float *in2,
                                size_t is2, float *out, size_t os, size_t n);
SEQ_FUNC void cnp_hypot_float64(const double *in1, size_t is1, const double *in2,
                                size_t is2, double *out, size_t os, size_t n);

SEQ_FUNC void cnp_sort_int16(int16_t *data, int64_t n);
SEQ_FUNC void cnp_sort_uint16(uint16_t *data, int64_t n);
SEQ_FUNC void cnp_sort_int32(int32_t *data, int64_t n);
SEQ_FUNC void cnp_sort_uint32(uint32_t *data, int64_t n);
SEQ_FUNC void cnp_sort_int64(int64_t *data, int64_t n);
SEQ_FUNC void cnp_sort_uint64(uint64_t *data, int64_t n);
SEQ_FUNC void cnp_sort_uint128(hwy::uint128_t *data, int64_t n);
SEQ_FUNC void cnp_sort_float32(float *data, int64_t n);
SEQ_FUNC void cnp_sort_float64(double *data, int64_t n);

SEQ_FUNC void cnp_cexpf(float r, float i, float *z);
SEQ_FUNC void cnp_clogf(float r, float i, float *z);
SEQ_FUNC void cnp_csqrtf(float r, float i, float *z);
SEQ_FUNC void cnp_ccoshf(float r, float i, float *z);
SEQ_FUNC void cnp_csinhf(float r, float i, float *z);
SEQ_FUNC void cnp_ctanhf(float r, float i, float *z);
SEQ_FUNC void cnp_cacoshf(float r, float i, float *z);
SEQ_FUNC void cnp_casinhf(float r, float i, float *z);
SEQ_FUNC void cnp_catanhf(float r, float i, float *z);
SEQ_FUNC void cnp_ccosf(float r, float i, float *z);
SEQ_FUNC void cnp_csinf(float r, float i, float *z);
SEQ_FUNC void cnp_ctanf(float r, float i, float *z);
SEQ_FUNC void cnp_cacosf(float r, float i, float *z);
SEQ_FUNC void cnp_casinf(float r, float i, float *z);
SEQ_FUNC void cnp_catanf(float r, float i, float *z);

SEQ_FUNC Span *seq_re_match(Regex *re, seq_int_t anchor, seq_str_t s, seq_int_t pos,
                            seq_int_t endpos);
SEQ_FUNC Span seq_re_match_one(Regex *re, seq_int_t anchor, seq_str_t s, seq_int_t pos,
                               seq_int_t endpos);
SEQ_FUNC seq_str_t seq_re_escape(seq_str_t p);
SEQ_FUNC Regex *seq_re_compile(seq_str_t p, seq_int_t flags);
SEQ_FUNC void seq_re_purge();
SEQ_FUNC seq_int_t seq_re_pattern_groups(Regex *pattern);
SEQ_FUNC seq_int_t seq_re_group_name_to_index(Regex *pattern, seq_str_t name);
SEQ_FUNC seq_str_t seq_re_group_index_to_name(Regex *pattern, seq_int_t index);
SEQ_FUNC bool seq_re_check_rewrite_string(Regex *pattern, seq_str_t rewrite,
                                          seq_str_t *error);
SEQ_FUNC seq_str_t seq_re_pattern_error(Regex *pattern);

SEQ_FUNC void __codon_jit_runtime_init(CodonJITAddSymbolFunc addSymbol, void *ctx);

namespace codon {
namespace runtime {
class JITError : public std::runtime_error {
private:
  std::string output;
  std::string type;
  std::string file;
  int line;
  int col;
  std::vector<uintptr_t> backtrace;

public:
  JITError(const std::string &output, const std::string &what, const std::string &type,
           const std::string &file, int line, int col,
           std::vector<uintptr_t> backtrace = {})
      : std::runtime_error(what), output(output), type(type), file(file), line(line),
        col(col), backtrace(std::move(backtrace)) {}

  std::string getOutput() const { return output; }
  std::string getType() const { return type; }
  std::string getFile() const { return file; }
  int getLine() const { return line; }
  int getCol() const { return col; }
  std::vector<uintptr_t> getBacktrace() const { return backtrace; }
};

std::string makeBacktraceFrameString(uintptr_t pc, const std::string &func = "",
                                     const std::string &file = "", int line = 0,
                                     int col = 0);

std::string getCapturedOutput();

void setJITErrorCallback(std::function<void(const JITError &)> callback);

} // namespace runtime
} // namespace codon
