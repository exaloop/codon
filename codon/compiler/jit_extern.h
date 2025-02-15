// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CJITResult {
  void *result;
  char *error;
};

void *jit_init(char *name);
void jit_exit(void *jit);

struct CJITResult jit_execute_python(void *jit, char *name, char **types,
                                     size_t types_size, char *pyModule, char **py_vars,
                                     size_t py_vars_size, void *arg, uint8_t debug);

struct CJITResult jit_execute_safe(void *jit, char *code, char *file, int32_t line,
                                   uint8_t debug);

char *get_jit_library();

#ifdef __cplusplus
}
#endif
