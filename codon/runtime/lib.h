#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unwind.h>

#define SEQ_FLAG_DEBUG (1 << 0)
#define SEQ_FLAG_JIT (1 << 1)

#define SEQ_FUNC extern "C"

typedef int64_t seq_int_t;

struct seq_t {
  seq_int_t len;
  char *seq;
};

struct seq_str_t {
  seq_int_t len;
  char *str;
};

extern int seq_flags;

SEQ_FUNC void seq_init(int flags);

SEQ_FUNC bool seq_is_macos();
SEQ_FUNC seq_int_t seq_pid();
SEQ_FUNC seq_int_t seq_time();
SEQ_FUNC seq_int_t seq_time_monotonic();
SEQ_FUNC char **seq_env();
SEQ_FUNC void seq_assert_failed(seq_str_t file, seq_int_t line);

SEQ_FUNC void *seq_alloc(size_t n);
SEQ_FUNC void *seq_alloc_atomic(size_t n);
SEQ_FUNC void *seq_calloc(size_t m, size_t n);
SEQ_FUNC void *seq_calloc_atomic(size_t m, size_t n);
SEQ_FUNC void *seq_realloc(void *p, size_t n);
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
SEQ_FUNC uint64_t seq_exc_class();

SEQ_FUNC seq_str_t seq_str_int(seq_int_t n);
SEQ_FUNC seq_str_t seq_str_uint(seq_int_t n);
SEQ_FUNC seq_str_t seq_str_float(double f);
SEQ_FUNC seq_str_t seq_str_bool(bool b);
SEQ_FUNC seq_str_t seq_str_byte(char c);
SEQ_FUNC seq_str_t seq_str_ptr(void *p);
SEQ_FUNC seq_str_t seq_str_tuple(seq_str_t *strs, seq_int_t n);

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

class seq_jit_error : public std::runtime_error {
private:
  std::string output;
  std::string type;
  std::string file;
  int line;
  int col;

public:
  explicit seq_jit_error(const std::string &output, const std::string &what,
                         const std::string &type, const std::string &file, int line,
                         int col)
      : std::runtime_error(what), output(output), type(type), file(file), line(line),
        col(col) {}

  std::string getOutput() const { return output; }
  std::string getType() const { return type; }
  std::string getFile() const { return file; }
  int getLine() const { return line; }
  int getCol() const { return col; }
};
