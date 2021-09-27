#ifndef SEQ_LIB_H
#define SEQ_LIB_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unwind.h>

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

template <typename T = void> struct seq_arr_t {
  seq_int_t len;
  T *arr;
};

extern int debug;

SEQ_FUNC void seq_init(int debug);
SEQ_FUNC void seq_assert_failed(seq_str_t file, seq_int_t line);

SEQ_FUNC void *seq_alloc(size_t n);
SEQ_FUNC void *seq_alloc_atomic(size_t n);
SEQ_FUNC void *seq_realloc(void *p, size_t n);
SEQ_FUNC void seq_free(void *p);
SEQ_FUNC void seq_register_finalizer(void *p, void (*f)(void *obj, void *data));

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

SEQ_FUNC void seq_print(seq_str_t str);
SEQ_FUNC void seq_print_full(seq_str_t str, FILE *fo);

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

#endif /* SEQ_LIB_H */
