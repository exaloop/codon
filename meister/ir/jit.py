# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Execute a compiled module with the LLVM-C ORC LLJIT API."""

from contextlib import ExitStack
from ctypes import CDLL, POINTER, PYFUNCTYPE, Structure, byref
from os import fsencode
import sys

from .jit_debug import error_callback
from .jit_memory import JITMemoryManager

from .llvm import A, I, P, Q, S, Linkage


class Root(Structure):
    _fields_ = [("start", P), ("size", Q)]


def define_roots(module):
    """Expose the addresses of writable globals to the GC after JIT linking."""
    ptr, size = module.pointer_type(), module.int_type(64)
    entries = []
    for value in module.globals():
        if (
            not value._call("IsDeclaration", I, ())
            and not value._call("IsGlobalConstant", I, ())
            and not value._call("IsThreadLocal", I, ())
            and value._call("GetLinkage", I, ()) != Linkage.AVAILABLE_EXTERNALLY
            and not value.name.startswith("llvm.")
        ):
            length = module.data_layout.alloc_size(value.value_type)
            if length:
                entries.append(module.const_struct((value, size.const_int(length))))
    if not entries:
        return None, 0
    row = module.struct_type((ptr, size))
    name = ".meister.gc.roots"
    roots = module.add_global(name, row.array(len(entries)), row.const_array(entries), constant=True)
    return roots.name, len(entries)


def root_ranges(roots, alignment):
    # Boehm works at word granularity. Merge before registering so removing
    # one range cannot leave part of a combined registration behind.
    ranges = []
    for root in sorted(roots, key=lambda root: root.start):
        start = root.start // alignment * alignment
        end = (root.start + root.size + alignment - 1) // alignment * alignment
        if ranges and start <= ranges[-1][1]:
            ranges[-1] = ranges[-1][0], max(end, ranges[-1][1])
        else:
            ranges.append((start, end))
    return ranges


def run(module, args, *, debug, fast_math=False, native=True, auto_free=False, on_stage=None):
    if module.debug_builder:
        module.debug_builder.finalize()
    module.optimize(debug=debug, fast_math=fast_math, native=native, auto_free=auto_free)
    lib = module.lib
    use_rtdyld = sys.platform == "linux" and module.target.startswith("x86_64-")
    manager = JITMemoryManager(lib, debug) if use_rtdyld else None
    root_name, root_count = (None, 0) if manager else define_roots(module)
    word_size = module.data_layout.pointer_size()
    if on_stage:
        on_stage(0)

    jit = P()
    builder = lib.function("OrcCreateLLJITBuilder", P)()
    if manager:
        manager.attach(builder)
    lib.check_error(lib.function("OrcCreateLLJIT", P, A, P)(byref(jit), builder))

    def lookup(name):
        address = Q()
        lib.check_error(lib.function("OrcLLJITLookup", P, P, POINTER(Q), S)(
            jit, byref(address), name.encode()
        ))
        return address.value

    try:
        if manager:
            manager.capture(jit)
        dylib = lib.function("OrcLLJITGetMainJITDylib", P, P)(jit)
        add_module = lib.function("OrcLLJITAddLLVMIRModule", P, P, P, P)
        lib.check_error(add_module(jit, dylib, module.transfer()))
        address = lookup("main")
        with ExitStack() as roots:
            if root_count:
                add_roots = lib.runtime_function("seq_gc_add_roots", None, P, P)
                remove_roots = lib.runtime_function("seq_gc_remove_roots", None, P, P)
                table = (Root * root_count).from_address(lookup(root_name))
                for start, end in root_ranges(table, word_size):
                    add_roots(start, end)
                    roots.callback(remove_roots, start, end)
            if on_stage:
                on_stage(1)
            # Programs can call CPython through the runtime on this thread.
            # Keep the GIL and supply the C ABI's argv[argc] sentinel.
            main = PYFUNCTYPE(I, I, POINTER(S))(address)
            argv = (S * (len(args) + 1))(*map(fsencode, args))
            with error_callback(lib, manager, debug):
                result = main(len(args), argv)
            CDLL(None).fflush(None)
            return result
    finally:
        lib.check_error(lib.function("OrcDisposeLLJIT", P, P)(jit))
