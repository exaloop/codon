# distutils: language=c++
# cython: c_string_type=unicode, c_string_encoding=ascii

from cython.operator import dereference as dref
from libcpp.string cimport string

from jit cimport JIT, JITResult


class JitError(Exception):
    pass


cdef class Jit:
    cdef JIT* jit

    def __cinit__(self):
        self.jit = new JIT("codon jit")
        dref(self.jit).init()

    def __dealloc__(self):
        del self.jit

    def execute(self, code: object) -> object:
        result = dref(self.jit).executeSafe(code)
        if <bint>result:
            return result.data
        else:
            raise JitError(result.data)
