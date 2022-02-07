# distutils: language=c++
# cython: c_string_type=unicode, c_string_encoding=ascii

from cython.operator import dereference as dref
from libcpp.string cimport string


cdef extern from "llvm/Support/Error.h" namespace "llvm":
    cdef cppclass Error
    cdef cppclass Expected[T]:
        T& get()


cdef extern from "../build/include/codon/compiler/jit.h" namespace "codon::jit":
    cdef cppclass JIT:
        JIT(string)
        Error init()
        Expected[string] execute(string)


cdef class Jit:
    cdef JIT* jit

    def __cinit__(self):
        self.jit = new JIT("codon jit")
        dref(self.jit).init()

    def __dealloc__(self):
        del self.jit

    def execute(self, code: object) -> object:
        return dref(self.jit).execute(code).get()
