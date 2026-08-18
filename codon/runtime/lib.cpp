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
  __kmpc_set_gc_callbacks(GC_get_stack_base, (gc_setup_callback)GC_register_my_thread,
                          GC_add_roots, GC_remove_roots);
#endif

  seq_exc_init(flags);
  seq_flags = flags;
}

SEQ_FUNC void seq_allow_register_threads() {
  static bool called = false;
  if (!called) {
    GC_allow_register_threads();
    called = true;
  }
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

#define UTF8_BUFFER_SIZE 256

typedef struct {
  uint8_t *ptr;
  size_t len;
} seq_utf8_t;

static inline size_t utf8_size(uint32_t c) {
  if (c <= 0x7f)
    return 1;
  if (c <= 0x7ff)
    return 2;
  if (c <= 0xffff)
    return 3;
  return 4;
}

static inline uint8_t *utf8_write(uint8_t *p, uint32_t c) {
  if (c <= 0x7f) {
    *p++ = (uint8_t)c;
  } else if (c <= 0x7ff) {
    *p++ = (uint8_t)(0xc0 | (c >> 6));
    *p++ = (uint8_t)(0x80 | (c & 0x3f));
  } else if (c <= 0xffff) {
    *p++ = (uint8_t)(0xe0 | (c >> 12));
    *p++ = (uint8_t)(0x80 | ((c >> 6) & 0x3f));
    *p++ = (uint8_t)(0x80 | (c & 0x3f));
  } else {
    *p++ = (uint8_t)(0xf0 | (c >> 18));
    *p++ = (uint8_t)(0x80 | ((c >> 12) & 0x3f));
    *p++ = (uint8_t)(0x80 | ((c >> 6) & 0x3f));
    *p++ = (uint8_t)(0x80 | (c & 0x3f));
  }

  return p;
}

static seq_utf8_t encode(const seq_str_t *s, uint8_t *tmp) {
  size_t n = SEQ_STR_LEN(*s);
  unsigned kind = SEQ_STR_KIND(*s);

  // ASCII is already UTF-8, so no allocation or copy is necessary.
  if (kind == SEQ_STR_KIND_ASCII || n == 0) {
    return {s->ptr, n};
  }

  // Determine the exact UTF-8 size first.
  size_t out_len = 0;

  switch (kind) {
  case SEQ_STR_KIND_LATIN1: {
    const uint8_t *p = s->ptr;

    size_t i = 0;
    while (i < n && p[i] < 0x80) {
      out_len += utf8_size(p[i]);
      ++i;
    }

    // If the string is pure ASCII, we can return early.
    if (i == n) {
      return {s->ptr, n};
    }

    for (; i < n; ++i)
      out_len += utf8_size(p[i]);

    break;
  }

  case SEQ_STR_KIND_UCS2: {
    for (size_t i = 0; i < n; ++i) {
      out_len += utf8_size(((uint16_t *)s->ptr)[i]);
    }
    break;
  }

  case SEQ_STR_KIND_UCS4: {
    for (size_t i = 0; i < n; ++i) {
      out_len += utf8_size(((uint32_t *)s->ptr)[i]);
    }
    break;
  }

  default:
    abort();
  }

  uint8_t *out =
      (out_len <= UTF8_BUFFER_SIZE) ? tmp : (uint8_t *)seq_alloc_atomic(out_len);
  uint8_t *dst = out;

  switch (kind) {
  case SEQ_STR_KIND_LATIN1:
    for (size_t i = 0; i < n; ++i)
      dst = utf8_write(dst, s->ptr[i]);
    break;

  case SEQ_STR_KIND_UCS2:
    for (size_t i = 0; i < n; ++i) {
      dst = utf8_write(dst, ((uint16_t *)s->ptr)[i]);
    }
    break;

  case SEQ_STR_KIND_UCS4:
    for (size_t i = 0; i < n; ++i) {
      dst = utf8_write(dst, ((uint32_t *)s->ptr)[i]);
    }
    break;
  }

  return {out, out_len};
}

static bool utf8_read(const uint8_t *data, size_t length, size_t *pos,
                      uint32_t *codepoint) {
  if (*pos == length)
    return false;

  uint8_t byte = data[(*pos)++];
  if (byte < 0x80) {
    *codepoint = byte;
    return true;
  }

  size_t width = 0;
  uint32_t value = 0;
  if (byte >= 0xC2 && byte <= 0xDF) {
    width = 1;
    value = byte & 0x1F;
  } else if (byte >= 0xE0 && byte <= 0xEF) {
    width = 2;
    value = byte & 0x0F;
  } else if (byte >= 0xF0 && byte <= 0xF4) {
    width = 3;
    value = byte & 0x07;
  } else {
    abort();
  }

  if (*pos + width > length)
    abort();
  for (size_t i = 0; i < width; ++i) {
    uint8_t continuation = data[(*pos)++];
    if ((continuation & 0xC0) != 0x80)
      abort();
    value = (value << 6) | (continuation & 0x3F);
  }

  if ((width == 1 && value < 0x80) || (width == 2 && value < 0x800) ||
      (width == 3 && value < 0x10000) || value > 0x10FFFF ||
      (value >= 0xD800 && value <= 0xDFFF))
    abort();

  *codepoint = value;
  return true;
}

static seq_str_t string_conv(const std::string &s) {
  if (s.empty())
    return {nullptr, 0};

  const auto *data = reinterpret_cast<const uint8_t *>(s.data());
  size_t length = 0;
  uint32_t maxchar = 0;
  size_t pos = 0;
  uint32_t codepoint;
  while (utf8_read(data, s.size(), &pos, &codepoint)) {
    ++length;
    maxchar = std::max(maxchar, codepoint);
  }

  unsigned kind = SEQ_STR_KIND_ASCII;
  size_t width = 1;
  if (maxchar > 0xFFFF) {
    kind = SEQ_STR_KIND_UCS4;
    width = sizeof(uint32_t);
  } else if (maxchar > 0xFF) {
    kind = SEQ_STR_KIND_UCS2;
    width = sizeof(uint16_t);
  } else if (maxchar > 0x7F) {
    kind = SEQ_STR_KIND_LATIN1;
  }

  auto *out = (uint8_t *)seq_alloc_atomic(length * width);
  pos = 0;
  for (size_t i = 0; utf8_read(data, s.size(), &pos, &codepoint); ++i) {
    if (kind == SEQ_STR_KIND_UCS4)
      reinterpret_cast<uint32_t *>(out)[i] = codepoint;
    else if (kind == SEQ_STR_KIND_UCS2)
      reinterpret_cast<uint16_t *>(out)[i] = codepoint;
    else
      out[i] = codepoint;
  }

  auto meta =
      (uint64_t(length) & SEQ_STR_LEN_MASK) | (uint64_t(kind) << SEQ_STR_KIND_SHIFT);
  return {out, static_cast<seq_int_t>(meta)};
}

static std::string string_encode(const seq_str_t &s) {
  uint8_t utf8_buf[UTF8_BUFFER_SIZE];
  auto utf8 = encode(&s, utf8_buf);
  std::string result(reinterpret_cast<char *>(utf8.ptr), utf8.len);
  if (utf8.ptr != utf8_buf && utf8.ptr != s.ptr)
    seq_free(utf8.ptr);
  return result;
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
    if (SEQ_STR_LEN(format) == 0) {
      return string_conv(default_format(n));
    } else {
      auto locale = std::locale("en_US.UTF-8");
      std::string fstr = string_encode(format);
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
  std::string t = string_encode(s);
  return fmt_conv(t, format, error);
}

SEQ_FUNC double seq_float_from_str(seq_str_t s, const char **e) {
  if (SEQ_STR_KIND(s) != SEQ_STR_KIND_ASCII) {
    *e = reinterpret_cast<char *>(s.ptr);
    return 0.0;
  }
  double result;
  auto r =
      fast_float::from_chars((char *)s.ptr, (char *)s.ptr + SEQ_STR_LEN(s), result);
  *e = (r.ec == std::errc() || r.ec == std::errc::result_out_of_range) ? r.ptr
                                                                       : (char *)s.ptr;
  return result;
}

/*
 * General I/O
 */

SEQ_FUNC seq_str_t seq_check_errno() {
  if (errno)
    return string_conv(strerror(errno));
  return {nullptr, 0};
}

SEQ_FUNC void seq_print(seq_str_t str) { seq_print_full(str, stdout); }

static std::ostringstream capture;
static std::mutex captureLock;

SEQ_FUNC void seq_print_full(seq_str_t str, FILE *fo) {
  uint8_t utf8_buf[UTF8_BUFFER_SIZE];
  auto utf8 = encode(&str, utf8_buf);

  if ((seq_flags & SEQ_FLAG_CAPTURE_OUTPUT) && (fo == stdout || fo == stderr)) {
    captureLock.lock();
    capture.write((char *)utf8.ptr, utf8.len);
    captureLock.unlock();
  } else {
    fwrite(utf8.ptr, 1, utf8.len, fo);
  }

  if (utf8.ptr != utf8_buf && utf8.ptr != str.ptr)
    seq_free(utf8.ptr);
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
