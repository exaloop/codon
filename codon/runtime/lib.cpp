// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <cassert>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <unwind.h>
#include <vector>

#define GC_THREADS
#include "codon/runtime/lib.h"
#include <dlfcn.h>
#include <gc.h>

#define FASTFLOAT_ALLOWS_LEADING_PLUS
#define FASTFLOAT_SKIP_WHITE_SPACE
#include "fast_float/fast_float.h"

/*
 * General
 */

#define USE_STANDARD_MALLOC 0

// OpenMP patch with GC callbacks
typedef int (*gc_setup_callback)(GC_stack_base *);
typedef void (*gc_roots_callback)(void *, void *);
extern "C" void __kmpc_set_gc_callbacks(gc_setup_callback get_stack_base,
                                        gc_setup_callback register_thread,
                                        gc_roots_callback add_roots,
                                        gc_roots_callback del_roots);

void seq_exc_init(int flags);

int seq_flags;

SEQ_FUNC void seq_init(int flags) {
#if !USE_STANDARD_MALLOC
  GC_INIT();
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_allow_register_threads();
  __kmpc_set_gc_callbacks(GC_get_stack_base, (gc_setup_callback)GC_register_my_thread,
                          GC_add_roots, GC_remove_roots);
#endif

  seq_exc_init(flags);
  seq_flags = flags;
}

SEQ_FUNC seq_int_t seq_pid() { return (seq_int_t)getpid(); }

SEQ_FUNC seq_int_t seq_time() {
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  seq_int_t nanos =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  return nanos;
}

SEQ_FUNC seq_int_t seq_time_monotonic() {
  auto duration = std::chrono::steady_clock::now().time_since_epoch();
  seq_int_t nanos =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  return nanos;
}

SEQ_FUNC seq_int_t seq_time_highres() {
  auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
  seq_int_t nanos =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  return nanos;
}

static void copy_time_c_to_seq(struct tm *x, seq_time_t *output) {
  output->year = x->tm_year;
  output->yday = x->tm_yday;
  output->sec = x->tm_sec;
  output->min = x->tm_min;
  output->hour = x->tm_hour;
  output->mday = x->tm_mday;
  output->mon = x->tm_mon;
  output->wday = x->tm_wday;
  output->isdst = x->tm_isdst;
}

static void copy_time_seq_to_c(seq_time_t *x, struct tm *output) {
  output->tm_year = x->year;
  output->tm_yday = x->yday;
  output->tm_sec = x->sec;
  output->tm_min = x->min;
  output->tm_hour = x->hour;
  output->tm_mday = x->mday;
  output->tm_mon = x->mon;
  output->tm_wday = x->wday;
  output->tm_isdst = x->isdst;
}

SEQ_FUNC bool seq_localtime(seq_int_t secs, seq_time_t *output) {
  struct tm result;
  time_t now = (secs >= 0 ? secs : time(nullptr));
  if (now == (time_t)-1 || !localtime_r(&now, &result))
    return false;
  copy_time_c_to_seq(&result, output);
  return true;
}

SEQ_FUNC bool seq_gmtime(seq_int_t secs, seq_time_t *output) {
  struct tm result;
  time_t now = (secs >= 0 ? secs : time(nullptr));
  if (now == (time_t)-1 || !gmtime_r(&now, &result))
    return false;
  copy_time_c_to_seq(&result, output);
  return true;
}

SEQ_FUNC seq_int_t seq_mktime(seq_time_t *time) {
  struct tm result;
  copy_time_seq_to_c(time, &result);
  return mktime(&result);
}

SEQ_FUNC void seq_sleep(double secs) {
  std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1>>(secs));
}

extern char **environ;
SEQ_FUNC char **seq_env() { return environ; }

/*
 * GC
 */

SEQ_FUNC void *seq_alloc(size_t n) {
#if USE_STANDARD_MALLOC
  return malloc(n);
#else
  return GC_MALLOC(n);
#endif
}

SEQ_FUNC void *seq_alloc_atomic(size_t n) {
#if USE_STANDARD_MALLOC
  return malloc(n);
#else
  return GC_MALLOC_ATOMIC(n);
#endif
}

SEQ_FUNC void *seq_alloc_uncollectable(size_t n) {
#if USE_STANDARD_MALLOC
  return malloc(n);
#else
  return GC_MALLOC_UNCOLLECTABLE(n);
#endif
}

SEQ_FUNC void *seq_alloc_atomic_uncollectable(size_t n) {
#if USE_STANDARD_MALLOC
  return malloc(n);
#else
  return GC_MALLOC_ATOMIC_UNCOLLECTABLE(n);
#endif
}

SEQ_FUNC void *seq_realloc(void *p, size_t newsize, size_t oldsize) {
#if USE_STANDARD_MALLOC
  return realloc(p, newsize);
#else
  return GC_REALLOC(p, newsize);
#endif
}

SEQ_FUNC void seq_free(void *p) {
#if USE_STANDARD_MALLOC
  free(p);
#else
  GC_FREE(p);
#endif
}

SEQ_FUNC void seq_register_finalizer(void *p, void (*f)(void *obj, void *data)) {
#if !USE_STANDARD_MALLOC
  GC_REGISTER_FINALIZER(p, f, nullptr, nullptr, nullptr);
#endif
}

SEQ_FUNC void seq_gc_add_roots(void *start, void *end) {
#if !USE_STANDARD_MALLOC
  GC_add_roots(start, end);
#endif
}

SEQ_FUNC void seq_gc_remove_roots(void *start, void *end) {
#if !USE_STANDARD_MALLOC
  GC_remove_roots(start, end);
#endif
}

SEQ_FUNC void seq_gc_clear_roots() {
#if !USE_STANDARD_MALLOC
  GC_clear_roots();
#endif
}

SEQ_FUNC void seq_gc_exclude_static_roots(void *start, void *end) {
#if !USE_STANDARD_MALLOC
  GC_exclude_static_roots(start, end);
#endif
}

/*
 * String conversion
 */
static seq_str_t string_conv(const std::string &s) {
  auto n = s.size();
  auto *p = (char *)seq_alloc_atomic(n);
  memcpy(p, s.data(), n);
  return {(seq_int_t)n, p};
}

template <typename T> std::string default_format(T n) {
  return fmt::format(FMT_STRING("{}"), n);
}

template <> std::string default_format(double n) {
  return fmt::format(FMT_STRING("{:g}"), n);
}

template <typename T> seq_str_t fmt_conv(T n, seq_str_t format, bool *error) {
  *error = false;
  try {
    if (format.len == 0) {
      return string_conv(default_format(n));
    } else {
      auto locale = std::locale("en_US.UTF-8");
      std::string fstr(format.str, format.len);
      return string_conv(fmt::format(
          locale, fmt::runtime(fmt::format(FMT_STRING("{{:{}}}"), fstr)), n));
    }
  } catch (const std::runtime_error &f) {
    *error = true;
    return string_conv(f.what());
  }
}

SEQ_FUNC seq_str_t seq_str_int(seq_int_t n, seq_str_t format, bool *error) {
  return fmt_conv<seq_int_t>(n, format, error);
}

SEQ_FUNC seq_str_t seq_str_uint(seq_int_t n, seq_str_t format, bool *error) {
  return fmt_conv<uint64_t>(n, format, error);
}

SEQ_FUNC seq_str_t seq_str_float(double f, seq_str_t format, bool *error) {
  return fmt_conv<double>(f, format, error);
}

SEQ_FUNC seq_str_t seq_str_ptr(void *p, seq_str_t format, bool *error) {
  return fmt_conv(fmt::ptr(p), format, error);
}

SEQ_FUNC seq_str_t seq_str_str(seq_str_t s, seq_str_t format, bool *error) {
  std::string t(s.str, s.len);
  return fmt_conv(t, format, error);
}

SEQ_FUNC double seq_float_from_str(seq_str_t s, const char **e) {
  double result;
  auto r = fast_float::from_chars(s.str, s.str + s.len, result);
  *e = (r.ec == std::errc() || r.ec == std::errc::result_out_of_range) ? r.ptr : s.str;
  return result;
}

/*
 * General I/O
 */

SEQ_FUNC seq_str_t seq_check_errno() {
  if (errno) {
    std::string msg = strerror(errno);
    auto *buf = (char *)seq_alloc_atomic(msg.size());
    memcpy(buf, msg.data(), msg.size());
    return {(seq_int_t)msg.size(), buf};
  }
  return {0, nullptr};
}

SEQ_FUNC void seq_print(seq_str_t str) { seq_print_full(str, stdout); }

static std::ostringstream capture;
static std::mutex captureLock;

SEQ_FUNC void seq_print_full(seq_str_t str, FILE *fo) {
  if ((seq_flags & SEQ_FLAG_CAPTURE_OUTPUT) && (fo == stdout || fo == stderr)) {
    captureLock.lock();
    capture.write(str.str, str.len);
    captureLock.unlock();
  } else {
    fwrite(str.str, 1, (size_t)str.len, fo);
  }
}

std::string codon::runtime::getCapturedOutput() {
  std::string result = capture.str();
  capture.str("");
  return result;
}

SEQ_FUNC void *seq_stdin() { return stdin; }

SEQ_FUNC void *seq_stdout() { return stdout; }

SEQ_FUNC void *seq_stderr() { return stderr; }

/*
 * Threading
 */

SEQ_FUNC void *seq_lock_new() {
  return (void *)new (seq_alloc_atomic(sizeof(std::timed_mutex))) std::timed_mutex();
}

SEQ_FUNC bool seq_lock_acquire(void *lock, bool block, double timeout) {
  auto *m = (std::timed_mutex *)lock;
  if (timeout < 0.0) {
    if (block) {
      m->lock();
      return true;
    } else {
      return m->try_lock();
    }
  } else {
    return m->try_lock_for(std::chrono::duration<double>(timeout));
  }
}

SEQ_FUNC void seq_lock_release(void *lock) {
  auto *m = (std::timed_mutex *)lock;
  m->unlock();
}

SEQ_FUNC void *seq_rlock_new() {
  return (void *)new (seq_alloc_atomic(sizeof(std::recursive_timed_mutex)))
      std::recursive_timed_mutex();
}

SEQ_FUNC bool seq_rlock_acquire(void *lock, bool block, double timeout) {
  auto *m = (std::recursive_timed_mutex *)lock;
  if (timeout < 0.0) {
    if (block) {
      m->lock();
      return true;
    } else {
      return m->try_lock();
    }
  } else {
    return m->try_lock_for(std::chrono::duration<double>(timeout));
  }
}

SEQ_FUNC void seq_rlock_release(void *lock) {
  auto *m = (std::recursive_timed_mutex *)lock;
  m->unlock();
}

#define CODON_JIT_SYMBOL(name) addSymbol(ctx, #name, reinterpret_cast<void *>(&name))

SEQ_FUNC void __codon_jit_runtime_init(CodonJITAddSymbolFunc addSymbol, void *ctx) {
  if (!addSymbol)
    return;

  CODON_JIT_SYMBOL(seq_alloc_exc);
  CODON_JIT_SYMBOL(seq_terminate);
  CODON_JIT_SYMBOL(seq_throw);
  CODON_JIT_SYMBOL(seq_personality);
  CODON_JIT_SYMBOL(seq_exc_offset);
  CODON_JIT_SYMBOL(seq_init);
  CODON_JIT_SYMBOL(seq_pid);
  CODON_JIT_SYMBOL(seq_time);
  CODON_JIT_SYMBOL(seq_time_monotonic);
  CODON_JIT_SYMBOL(seq_time_highres);
  CODON_JIT_SYMBOL(seq_localtime);
  CODON_JIT_SYMBOL(seq_gmtime);
  CODON_JIT_SYMBOL(seq_mktime);
  CODON_JIT_SYMBOL(seq_sleep);
  CODON_JIT_SYMBOL(seq_env);
  CODON_JIT_SYMBOL(seq_alloc);
  CODON_JIT_SYMBOL(seq_alloc_atomic);
  CODON_JIT_SYMBOL(seq_alloc_uncollectable);
  CODON_JIT_SYMBOL(seq_alloc_atomic_uncollectable);
  CODON_JIT_SYMBOL(seq_realloc);
  CODON_JIT_SYMBOL(seq_free);
  CODON_JIT_SYMBOL(seq_register_finalizer);
  CODON_JIT_SYMBOL(seq_gc_add_roots);
  CODON_JIT_SYMBOL(seq_gc_remove_roots);
  CODON_JIT_SYMBOL(seq_gc_clear_roots);
  CODON_JIT_SYMBOL(seq_gc_exclude_static_roots);
  CODON_JIT_SYMBOL(seq_str_int);
  CODON_JIT_SYMBOL(seq_str_uint);
  CODON_JIT_SYMBOL(seq_str_float);
  CODON_JIT_SYMBOL(seq_str_ptr);
  CODON_JIT_SYMBOL(seq_str_str);
  CODON_JIT_SYMBOL(seq_float_from_str);
  CODON_JIT_SYMBOL(seq_check_errno);
  CODON_JIT_SYMBOL(seq_print);
  CODON_JIT_SYMBOL(seq_print_full);
  CODON_JIT_SYMBOL(seq_stdin);
  CODON_JIT_SYMBOL(seq_stdout);
  CODON_JIT_SYMBOL(seq_stderr);
  CODON_JIT_SYMBOL(seq_lock_new);
  CODON_JIT_SYMBOL(seq_lock_acquire);
  CODON_JIT_SYMBOL(seq_lock_release);
  CODON_JIT_SYMBOL(seq_rlock_new);
  CODON_JIT_SYMBOL(seq_rlock_acquire);
  CODON_JIT_SYMBOL(seq_rlock_release);

  CODON_JIT_SYMBOL(cnp_acos_float32);
  CODON_JIT_SYMBOL(cnp_acos_float64);
  CODON_JIT_SYMBOL(cnp_acosh_float32);
  CODON_JIT_SYMBOL(cnp_acosh_float64);
  CODON_JIT_SYMBOL(cnp_asin_float32);
  CODON_JIT_SYMBOL(cnp_asin_float64);
  CODON_JIT_SYMBOL(cnp_asinh_float32);
  CODON_JIT_SYMBOL(cnp_asinh_float64);
  CODON_JIT_SYMBOL(cnp_atan_float32);
  CODON_JIT_SYMBOL(cnp_atan_float64);
  CODON_JIT_SYMBOL(cnp_atanh_float32);
  CODON_JIT_SYMBOL(cnp_atanh_float64);
  CODON_JIT_SYMBOL(cnp_atan2_float32);
  CODON_JIT_SYMBOL(cnp_atan2_float64);
  CODON_JIT_SYMBOL(cnp_cos_float32);
  CODON_JIT_SYMBOL(cnp_cos_float64);
  CODON_JIT_SYMBOL(cnp_exp_float32);
  CODON_JIT_SYMBOL(cnp_exp_float64);
  CODON_JIT_SYMBOL(cnp_exp2_float32);
  CODON_JIT_SYMBOL(cnp_exp2_float64);
  CODON_JIT_SYMBOL(cnp_expm1_float32);
  CODON_JIT_SYMBOL(cnp_expm1_float64);
  CODON_JIT_SYMBOL(cnp_log_float32);
  CODON_JIT_SYMBOL(cnp_log_float64);
  CODON_JIT_SYMBOL(cnp_log10_float32);
  CODON_JIT_SYMBOL(cnp_log10_float64);
  CODON_JIT_SYMBOL(cnp_log1p_float32);
  CODON_JIT_SYMBOL(cnp_log1p_float64);
  CODON_JIT_SYMBOL(cnp_log2_float32);
  CODON_JIT_SYMBOL(cnp_log2_float64);
  CODON_JIT_SYMBOL(cnp_sin_float32);
  CODON_JIT_SYMBOL(cnp_sin_float64);
  CODON_JIT_SYMBOL(cnp_sinh_float32);
  CODON_JIT_SYMBOL(cnp_sinh_float64);
  CODON_JIT_SYMBOL(cnp_tanh_float32);
  CODON_JIT_SYMBOL(cnp_tanh_float64);
  CODON_JIT_SYMBOL(cnp_hypot_float32);
  CODON_JIT_SYMBOL(cnp_hypot_float64);

  CODON_JIT_SYMBOL(cnp_sort_int16);
  CODON_JIT_SYMBOL(cnp_sort_uint16);
  CODON_JIT_SYMBOL(cnp_sort_int32);
  CODON_JIT_SYMBOL(cnp_sort_uint32);
  CODON_JIT_SYMBOL(cnp_sort_int64);
  CODON_JIT_SYMBOL(cnp_sort_uint64);
  CODON_JIT_SYMBOL(cnp_sort_uint128);
  CODON_JIT_SYMBOL(cnp_sort_float32);
  CODON_JIT_SYMBOL(cnp_sort_float64);

  CODON_JIT_SYMBOL(cnp_cexpf);
  CODON_JIT_SYMBOL(cnp_clogf);
  CODON_JIT_SYMBOL(cnp_csqrtf);
  CODON_JIT_SYMBOL(cnp_ccoshf);
  CODON_JIT_SYMBOL(cnp_csinhf);
  CODON_JIT_SYMBOL(cnp_ctanhf);
  CODON_JIT_SYMBOL(cnp_cacoshf);
  CODON_JIT_SYMBOL(cnp_casinhf);
  CODON_JIT_SYMBOL(cnp_catanhf);
  CODON_JIT_SYMBOL(cnp_ccosf);
  CODON_JIT_SYMBOL(cnp_csinf);
  CODON_JIT_SYMBOL(cnp_ctanf);
  CODON_JIT_SYMBOL(cnp_cacosf);
  CODON_JIT_SYMBOL(cnp_casinf);
  CODON_JIT_SYMBOL(cnp_catanf);

  CODON_JIT_SYMBOL(seq_re_match);
  CODON_JIT_SYMBOL(seq_re_match_one);
  CODON_JIT_SYMBOL(seq_re_escape);
  CODON_JIT_SYMBOL(seq_re_compile);
  CODON_JIT_SYMBOL(seq_re_purge);
  CODON_JIT_SYMBOL(seq_re_pattern_groups);
  CODON_JIT_SYMBOL(seq_re_group_name_to_index);
  CODON_JIT_SYMBOL(seq_re_group_index_to_name);
  CODON_JIT_SYMBOL(seq_re_check_rewrite_string);
  CODON_JIT_SYMBOL(seq_re_pattern_error);
}

#undef CODON_JIT_SYMBOL
