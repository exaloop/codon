# Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

# distutils: language=c++
# cython: language_level=3
# cython: c_string_type=unicode
# cython: c_string_encoding=utf8

from libcpp.string cimport string
from libcpp.vector cimport vector
cimport codon.jit


class JITError(Exception):
    pass


cdef class JITWrapper:
    cdef codon.jit.JIT* jit

    def __cinit__(self):
        self.jit = codon.jit.jitInit(b"codon jit")

    def __dealloc__(self):
        del self.jit

    def execute(self, code: str, filename: str, fileno: int, debug: char) -> str:
        result = codon.jit.jitExecuteSafe(self.jit, code, filename, fileno, <char>debug)
        if <bint>result:
            return None
        else:
            raise JITError(result.message)

    def run_wrapper(self, name: str, types: list[str], module: str, pyvars: list[str], args, debug: char) -> object:
        cdef vector[string] types_vec = types
        cdef vector[string] pyvars_vec = pyvars
        result = codon.jit.jitExecutePython(
            self.jit, name, types_vec, module, pyvars_vec, <object>args, <char>debug
        )
        if <bint>result:
            return <object>result.result
        else:
            raise JITError(result.message)

def codon_library():
    return codon.jit.getJITLibrary()
