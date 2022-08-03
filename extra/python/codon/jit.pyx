# distutils: language=c++
# cython: language_level=3
# cython: c_string_type=unicode
# cython: c_string_encoding=ascii

from cython.operator import dereference as dref
from libcpp.string cimport string
from libcpp.vector cimport vector

from codon.jit cimport JIT, JITResult


class JITError(Exception):
    pass


cdef class JITWrapper:
    cdef JIT* jit

    def __cinit__(self):
        self.jit = new JIT(b"codon jit")
        dref(self.jit).init()

    def __dealloc__(self):
        del self.jit

    def execute(self, code: str, debug: char) -> str:
        result = dref(self.jit).executeSafe(code, <char>debug)
        if <bint>result:
            return None
        else:
            raise JITError(result.message)

    def run_wrapper(self, name: str, types: list[str], pyvars: list[str], args, debug: char) -> object:
        cdef vector[string] types_vec = types
        cdef vector[string] pyvars_vec = pyvars
        result = dref(self.jit).executePython(name, types_vec, pyvars_vec, <object>args, <char>debug)
        if <bint>result:
            return <object>result.result
        else:
            raise JITError(result.message)
