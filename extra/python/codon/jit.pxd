from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "llvm/Support/Error.h" namespace "llvm":
    cdef cppclass Error


cdef extern from "codon/compiler/jit.h" namespace "codon::jit":
    cdef cppclass JITResult:
        void *result
        string message
        bint operator bool()

    cdef cppclass JIT:
        JIT(string)
        Error init()
        JITResult executeSafe(string, char)
        JITResult executePython(string, vector[string], vector[string], object, char)
