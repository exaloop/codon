# Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "codon/compiler/jit_extern.h" namespace "codon::jit":
    cdef cppclass JIT
    cdef cppclass JITResult:
        void *result
        string message
        bint operator bool()

    JIT *jitInit(string)
    JITResult jitExecuteSafe(JIT*, string, string, int, char)
    JITResult jitExecutePython(JIT*, string, vector[string], string, vector[string], object, char)
    string getJITLibrary()
