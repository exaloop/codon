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
#include <gc.h>

using namespace std;

/*
 * General
 */

// the following is for manually invoking OpenMP "parallel for"
typedef int32_t kmp_int32;
typedef struct {
  kmp_int32 reserved_1;
  kmp_int32 flags;
  kmp_int32 reserved_2;
  kmp_int32 reserved_3;
  char const *psource;
} ident_t;
typedef void (*kmpc_micro)(kmp_int32 *global_tid, kmp_int32 *bound_tid, ...);
static ident_t dummy_loc = {0, 2, 0, 0, ";unknown;unknown;0;0;;"};
extern "C" void __kmpc_fork_call(ident_t *, kmp_int32 nargs, kmpc_micro microtask, ...);
static void register_thread(kmp_int32 *global_tid, kmp_int32 *bound_tid) {
  GC_stack_base sb;
  GC_get_stack_base(&sb);
  GC_register_my_thread(&sb);
}

// OpenMP patch for registering GC roots
typedef void (*gc_roots_callback)(void *, void *);
extern "C" void __kmpc_set_gc_callbacks(gc_roots_callback add_roots, gc_roots_callback del_roots);

void seq_exc_init();

int seq_flags;

SEQ_FUNC void seq_init(int flags) {
  GC_INIT();
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_allow_register_threads();
  __kmpc_set_gc_callbacks(GC_add_roots, GC_remove_roots);
  // equivalent to: #pragma omp parallel { register_thread }
  __kmpc_fork_call(&dummy_loc, 0, (kmpc_micro)register_thread);
  seq_exc_init();
  seq_flags = flags;
}

SEQ_FUNC bool seq_is_macos() {
#ifdef __APPLE__
  return true;
#else
  return false;
#endif
}

SEQ_FUNC seq_int_t seq_pid() { return (seq_int_t)getpid(); }

SEQ_FUNC seq_int_t seq_time() {
  auto duration = chrono::system_clock::now().time_since_epoch();
  seq_int_t nanos = chrono::duration_cast<chrono::nanoseconds>(duration).count();
  return nanos;
}

SEQ_FUNC seq_int_t seq_time_monotonic() {
  auto duration = chrono::steady_clock::now().time_since_epoch();
  seq_int_t nanos = chrono::duration_cast<chrono::nanoseconds>(duration).count();
  return nanos;
}

SEQ_FUNC seq_int_t seq_time_highres() {
  auto duration = chrono::high_resolution_clock::now().time_since_epoch();
  seq_int_t nanos = chrono::duration_cast<chrono::nanoseconds>(duration).count();
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
#define USE_STANDARD_MALLOC 0

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

SEQ_FUNC void *seq_calloc(size_t m, size_t n) {
#if USE_STANDARD_MALLOC
  return calloc(m, n);
#else
  size_t s = m * n;
  void *p = GC_MALLOC(s);
  memset(p, 0, s);
  return p;
#endif
}

SEQ_FUNC void *seq_calloc_atomic(size_t m, size_t n) {
#if USE_STANDARD_MALLOC
  return calloc(m, n);
#else
  size_t s = m * n;
  void *p = GC_MALLOC_ATOMIC(s);
  memset(p, 0, s);
  return p;
#endif
}

SEQ_FUNC void *seq_realloc(void *p, size_t n) {
#if USE_STANDARD_MALLOC
  return realloc(p, n);
#else
  return GC_REALLOC(p, n);
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
template <typename T>
static seq_str_t string_conv(const char *fmt, const size_t size, T t) {
  auto *p = (char *)seq_alloc_atomic(size);
  int n = snprintf(p, size, fmt, t);
  if (n >= size) {
    auto n2 = (size_t)n + 1;
    p = (char *)seq_realloc((void *)p, n2);
    n = snprintf(p, n2, fmt, t);
  }
  return {(seq_int_t)n, p};
}

SEQ_FUNC seq_str_t seq_str_int(seq_int_t n) { return string_conv("%ld", 22, n); }

SEQ_FUNC seq_str_t seq_str_uint(seq_int_t n) { return string_conv("%lu", 22, n); }

SEQ_FUNC seq_str_t seq_str_float(double f) { return string_conv("%g", 16, f); }

SEQ_FUNC seq_str_t seq_str_bool(bool b) {
  return string_conv("%s", 6, b ? "True" : "False");
}

SEQ_FUNC seq_str_t seq_str_byte(char c) { return string_conv("%c", 5, c); }

SEQ_FUNC seq_str_t seq_str_ptr(void *p) { return string_conv("%p", 19, p); }

/*
 * General I/O
 */

SEQ_FUNC seq_str_t seq_check_errno() {
  if (errno) {
    string msg = strerror(errno);
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
  if ((seq_flags & SEQ_FLAG_JIT) && (fo == stdout || fo == stderr)) {
    captureLock.lock();
    capture.write(str.str, str.len);
    captureLock.unlock();
  } else {
    fwrite(str.str, 1, (size_t)str.len, fo);
  }
}

std::string codon::getCapturedOutput() {
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
  return (void *)new (seq_alloc_atomic(sizeof(timed_mutex))) timed_mutex();
}

SEQ_FUNC bool seq_lock_acquire(void *lock, bool block, double timeout) {
  auto *m = (timed_mutex *)lock;
  if (timeout < 0.0) {
    if (block) {
      m->lock();
      return true;
    } else {
      return m->try_lock();
    }
  } else {
    return m->try_lock_for(chrono::duration<double>(timeout));
  }
}

SEQ_FUNC void seq_lock_release(void *lock) {
  auto *m = (timed_mutex *)lock;
  m->unlock();
}

SEQ_FUNC void *seq_rlock_new() {
  return (void *)new (seq_alloc_atomic(sizeof(recursive_timed_mutex)))
      recursive_timed_mutex();
}

SEQ_FUNC bool seq_rlock_acquire(void *lock, bool block, double timeout) {
  auto *m = (recursive_timed_mutex *)lock;
  if (timeout < 0.0) {
    if (block) {
      m->lock();
      return true;
    } else {
      return m->try_lock();
    }
  } else {
    return m->try_lock_for(chrono::duration<double>(timeout));
  }
}

SEQ_FUNC void seq_rlock_release(void *lock) {
  auto *m = (recursive_timed_mutex *)lock;
  m->unlock();
}
