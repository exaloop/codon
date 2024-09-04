// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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

SEQ_FUNC int seq_flags;

SEQ_FUNC void seq_init(int flags);

SEQ_FUNC bool seq_is_macos();
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

SEQ_FUNC void *seq_alloc_exc(int type, void *obj);
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

SEQ_FUNC void *seq_stdin();
SEQ_FUNC void *seq_stdout();
SEQ_FUNC void *seq_stderr();

SEQ_FUNC void seq_print(seq_str_t str);
SEQ_FUNC void seq_print_full(seq_str_t str, FILE *fo);

SEQ_FUNC void *seq_lock_new();
SEQ_FUNC void *seq_lock_new();
SEQ_FUNC bool seq_lock_acquire(void *lock, bool block, double timeout);
SEQ_FUNC void seq_lock_release(void *lock);
SEQ_FUNC void *seq_rlock_new();
SEQ_FUNC bool seq_rlock_acquire(void *lock, bool block, double timeout);
SEQ_FUNC void seq_rlock_release(void *lock);

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
