# distutils: language=c++
# cython: language_level=3
# cython: c_string_type=unicode
# cython: c_string_encoding=ascii

from cython.operator import dereference as dref
from libcpp.string cimport string

from src.jit cimport JIT, JITResult


class JitError(Exception):
    pass


cdef class Jit:
    cdef JIT* jit

    def __cinit__(self):
        self.jit = new JIT(b"codon jit")
        dref(self.jit).init()

    def __dealloc__(self):
        del self.jit

    def execute(self, code: str) -> str:
        result = dref(self.jit).executeSafe(code)
        if <bint>result:
            return result.data
        else:
            raise JitError(result.data)
