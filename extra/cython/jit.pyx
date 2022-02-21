# distutils: language=c++
# cython: c_string_type=unicode, c_string_encoding=ascii

from cython.operator import dereference as dref
from libcpp.string cimport string


cdef extern from "llvm/Support/Error.h" namespace "llvm":
    cdef cppclass Error


cdef extern from "codon/compiler/jit.h" namespace "codon::jit":
    cdef cppclass JITResult:
        string data
        bint operator bool()

    cdef cppclass JIT:
        JIT(string)
        Error init()
        JITResult execute_safe(string)


cdef class Jit:
    cdef JIT* jit

    def __cinit__(self):
        self.jit = new JIT("codon jit")
        dref(self.jit).init()

    def __dealloc__(self):
        del self.jit

    def execute(self, code: object) -> object:
        result = dref(self.jit).execute_safe(code)
        if <bint>result:
            return result.data
        else:
            raise RuntimeError(result.data)
