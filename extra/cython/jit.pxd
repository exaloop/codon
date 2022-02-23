# distutils: language=c++
# cython: c_string_type=unicode, c_string_encoding=ascii

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
        JITResult executeSafe(string)
