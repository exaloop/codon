# Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

# distutils: language=c
# cython: language_level=3
# cython: c_string_type=unicode
# cython: c_string_encoding=utf8

cimport codon.jit
from libc.stdlib cimport malloc, calloc, free
from libc.string cimport strcpy
from libc.stdint cimport int32_t, uint8_t


class JITError(Exception):
    pass


cdef str get_free_str(char *s):
    cdef bytes py_s
    try:
        py_s = s
        return py_s.decode('utf-8')
    finally:
        free(s)


cdef class JITWrapper:
    cdef void* jit

    def __cinit__(self):
        self.jit = codon.jit.jit_init(b"codon jit")

    def __dealloc__(self):
        codon.jit.jit_exit(self.jit)

    def execute(self, code: str, filename: str, fileno: int, debug) -> str:
        result = codon.jit.jit_execute_safe(
            self.jit, code.encode('utf-8'), filename.encode('utf-8'), fileno, <uint8_t>debug
        )
        if result.error is NULL:
            return None
        else:
            msg = get_free_str(result.error)
            raise JITError(msg)

    def run_wrapper(self, name: str, types: list[str], module: str, pyvars: list[str], args, debug) -> object:
        cdef char** c_types = <char**>calloc(len(types), sizeof(char*))
        cdef char** c_pyvars = <char**>calloc(len(pyvars), sizeof(char*))
        if not c_types or not c_pyvars:
            raise JITError("Cython allocation failed")
        try:
            for i, s in enumerate(types):
                bytes = s.encode('utf-8')
                c_types[i] = <char*>malloc(len(bytes) + 1)
                strcpy(c_types[i], bytes)
            for i, s in enumerate(pyvars):
                bytes = s.encode('utf-8')
                c_pyvars[i] = <char*>malloc(len(bytes) + 1)
                strcpy(c_pyvars[i], bytes)

            result = codon.jit.jit_execute_python(
                self.jit, name.encode('utf-8'), c_types, len(types),
                module.encode('utf-8'), c_pyvars, len(pyvars),
                <void *>args, <uint8_t>debug
            )
            if result.error is NULL:
                return <object>result.result
            else:
                msg = get_free_str(result.error)
                raise JITError(msg)
        finally:
            for i in range(len(types)):
                free(c_types[i])
            free(c_types)
            for i in range(len(pyvars)):
                free(c_pyvars[i])
            free(c_pyvars)


def codon_library():
    cdef char* c = codon.jit.get_jit_library()
    return get_free_str(c)
