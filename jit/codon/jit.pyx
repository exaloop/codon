# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

# distutils: language=c
# cython: language_level=3
# cython: c_string_type=unicode
# cython: c_string_encoding=utf8

cimport codon.jit
from libc.stddef cimport size_t
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


cdef class JITClassInstanceHandle:
    """Owns the opaque native jitclass control block returned by C++."""
    cdef void* instance

    def __cinit__(self):
        self.instance = NULL

    cdef void set_instance(self, void* instance):
        self.instance = instance

    cdef void* get_instance(self):
        return self.instance

    cdef void* require_instance(self) except NULL:
        if self.instance is NULL:
            raise JITError("jitclass object has been released")
        return self.instance

    def close(self):
        cdef codon.jit.CJITResult result
        cdef void* instance
        if self.instance is NULL:
            return

        # Invalidate first so re-entrant cleanup cannot release the same block twice.
        instance = self.instance
        self.instance = NULL
        result = codon.jit.c_jitclass_release(instance)
        if result.error is not NULL:
            msg = get_free_str(result.error)
            raise JITError(msg)

    @property
    def closed(self):
        return self.instance is NULL

    cdef object _call_with_jit(self, void* jit, str class_name, str method_name,
                              list types, args, debug):
        cdef codon.jit.CJITResult result
        cdef size_t types_size = len(types)
        cdef size_t alloc_size = types_size if types_size > 0 else 1
        cdef char** c_types
        cdef void* instance = self.require_instance()
        cdef bytes encoded
        cdef str msg
        if jit is NULL:
            raise JITError("JIT context has been released")

        c_types = <char**>calloc(alloc_size, sizeof(char*))
        if not c_types:
            raise JITError("Cython allocation failed")
        try:
            for i, s in enumerate(types):
                encoded = s.encode('utf-8')
                c_types[i] = <char*>malloc(len(encoded) + 1)
                if not c_types[i]:
                    raise JITError("Cython allocation failed")
                strcpy(c_types[i], encoded)
            result = codon.jit.c_jitclass_call(
                jit, class_name.encode('utf-8'), instance,
                method_name.encode('utf-8'), c_types, types_size,
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

    def call_with_jit(self, JITWrapper jit_wrapper, class_name: str, method_name: str,
                      types: list[str], args, debug) -> object:
        """Call a native method through the current JIT wrapper."""
        if jit_wrapper is None:
            raise JITError("JIT context has been released")
        return self._call_with_jit(jit_wrapper.jit, class_name, method_name, types, args, debug)

    def __dealloc__(self):
        cdef void* instance
        if self.instance is not NULL:
            # Final safety net for paths that did not call close() explicitly.
            instance = self.instance
            self.instance = NULL
            codon.jit.c_jitclass_release(instance)


cdef class JITWrapper:
    cdef void* jit

    def __cinit__(self):
        self.jit = codon.jit.jit_init(b"codon jit")

    def __dealloc__(self):
        if self.jit is not NULL:
            codon.jit.jit_exit(self.jit)
            self.jit = NULL

    def execute(self, code: str, filename: str, fileno: int, debug) -> str:
        result = codon.jit.jit_execute_safe(
            self.jit, code.encode('utf-8'), filename.encode('utf-8'), fileno, <uint8_t>debug
        )
        if result.error is NULL:
            return None
        else:
            msg = get_free_str(result.error)
            raise JITError(msg)

    def run_wrapper(self, name: str, types: list[str], module: str,
                    pyvars: list[str], args, debug) -> object:
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

    def jitclass_new(self, class_name: str, native_class_name: str,
                     types: list[str], args, debug) -> JITClassInstanceHandle:
        """Create a native jitclass instance and wrap it in a Python-owned handle."""
        cdef size_t types_size = len(types)
        cdef size_t alloc_size = types_size if types_size > 0 else 1
        cdef char** c_types = <char**>calloc(alloc_size, sizeof(char*))
        cdef JITClassInstanceHandle handle
        if not c_types:
            raise JITError("Cython allocation failed")
        try:
            for i, s in enumerate(types):
                bytes = s.encode('utf-8')
                c_types[i] = <char*>malloc(len(bytes) + 1)
                strcpy(c_types[i], bytes)
            result = codon.jit.c_jitclass_new(
                self.jit, class_name.encode('utf-8'),
                native_class_name.encode('utf-8'),
                c_types, types_size, <void *>args, <uint8_t>debug
            )
            if result.error is NULL:
                handle = JITClassInstanceHandle()
                handle.set_instance(result.result)
                return handle
            else:
                msg = get_free_str(result.error)
                raise JITError(msg)
        finally:
            for i in range(len(types)):
                free(c_types[i])
            free(c_types)

def codon_library():
    cdef char* c = codon.jit.get_jit_library()
    return get_free_str(c)
