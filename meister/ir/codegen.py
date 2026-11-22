# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Meister's source-to-LLVM and run entry points."""

import argparse
import sys
import time
from contextlib import contextmanager, nullcontext
from pathlib import Path

from ..ast.nodes import Node
from ..cache import MAX_REALIZATION_DEPTH, Cache
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from ..test import find_library
from . import llvm
from .llvisitor import LLVMVisitor
from .transform import PassManager


def peak_rss():
    if sys.platform == "win32":
        import ctypes
        from ctypes import wintypes

        class ProcessMemoryCounters(ctypes.Structure):
            _fields_ = [
                ("cb", wintypes.DWORD),
                ("PageFaultCount", wintypes.DWORD),
                ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t),
            ]

        counters = ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.GetCurrentProcess.restype = wintypes.HANDLE
        query = ctypes.WinDLL("psapi", use_last_error=True).GetProcessMemoryInfo
        query.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessMemoryCounters), wintypes.DWORD]
        query.restype = wintypes.BOOL
        if not query(kernel.GetCurrentProcess(), ctypes.byref(counters), counters.cb):
            raise ctypes.WinError(ctypes.get_last_error())
        return counters.PeakWorkingSetSize

    import resource

    # ru_maxrss is bytes on macOS and KiB on Linux.
    value = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return value if sys.platform == "darwin" else value * 1024


class StageTimings:
    """Accumulate exclusive wall time and process high-water RSS by stage."""

    def __init__(self):
        self.times = {}
        self.peaks = {}
        self.stack = []
        self.last = time.perf_counter()

    def peak(self):
        return peak_rss()

    def switch(self, name):
        now = time.perf_counter()
        if self.stack:
            current = self.stack[-1]
            self.times[current] = self.times.get(current, 0) + now - self.last
            self.peaks[current] = max(self.peaks.get(current, 0), self.peak())
        self.last = now
        if name:
            self.stack.append(name)

    @contextmanager
    def stage(self, name):
        self.switch(name)
        try:
            yield
        finally:
            self.switch(None)
            current = self.stack.pop()
            # Imported files are parsed/scoped while the typechecker is active;
            # their totals are final only when that enclosing stage completes.
            if current == "typecheck":
                for child in ("parse", "scope"):
                    if child in self.times:
                        self.report(child)
            if "typecheck" not in self.stack:
                self.report(current)

    def report(self, stage):
        print(
            f"  {stage:<15} {self.times[stage]:9.3f} s   "
            f"peak RSS {self.peaks[stage] / 1024**2:8.1f} MiB (cumulative)",
            file=sys.stderr,
            flush=True,
        )

    def transition(self, name):
        self.switch(None)
        current = self.stack.pop()
        self.report(current)
        self.switch(name)


def emit_module(cc, library, file, defines, *, standalone=False):
    """Emit and verify an LLVM module. The caller owns the returned module."""
    module = llvm.Module(library, "codon", file)
    try:
        version = ".".join(
            defines[f"__codon_version_{part}__"] for part in ("major", "minor", "micro")
        )
        LLVMVisitor(
            module, debug=cc.options.debug, standalone=standalone, version=version
        ).process(cc.module)
        module.debug_builder.finalize()
        module.verify()
        return module
    except BaseException:
        module.close()
        raise


def main(mode, argv):
    parser = argparse.ArgumentParser(prog=f"python -m meister {mode}")
    parser.add_argument("-release", "--release", action="store_true", help="enable release optimizations")
    parser.add_argument("--library", help="path to libcodonc from the Exaloop LLVM build")
    if mode == "run":
        parser.add_argument("-time", action="store_true", help="report timings and peak memory")
    if mode == "llvm":
        parser.add_argument("-o", "--output", type=Path, help="LLVM IR output (default: stdout)")
    parser.add_argument("file")
    if mode == "run":
        parser.add_argument("args", nargs=argparse.REMAINDER, help="arguments to the Codon program")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parents[2]
    native, library = find_library(root, args.library)
    library_api = llvm.Library(library)
    file = str(Path(args.file).resolve())
    sys.setrecursionlimit(max(sys.getrecursionlimit(), MAX_REALIZATION_DEPTH * 100))
    # Resolve stdlib files relative to the native executable so the source and
    # generated tables have the same filenames as those used by Codon.
    cc = Cache(str(library.parent / "codon"))
    cc.options.debug = not args.release
    cc.fs.set_module0(file)
    timing = StageTimings() if mode == "run" and args.time else None
    if timing:
        print("Meister timings (exclusive wall time):", file=sys.stderr, flush=True)
        # Typechecking loads and scopes imports too. Attribute their work to the
        # actual stage without pre-scoping or changing compilation semantics.
        parse, scope = cc.parse, cc.scope

        def timed_parse(*args, **kwargs):
            with timing.stage("parse"):
                return parse(*args, **kwargs)

        def timed_scope(*args, **kwargs):
            with timing.stage("scope"):
                return scope(*args, **kwargs)

        cc.parse, cc.scope = timed_parse, timed_scope
    if timing:
        with timing.stage("parse main"):
            node = parse(file=file)
    else:
        node = cc.parse(file=file)
    # Use the native build's define order; it is observable in CIR/LLVM dumps.
    defines = native.get_early_defines() or cc.options.get_early_defines()
    defines["__debug__"] = str(int(cc.options.debug))
    if timing:
        with timing.stage("typecheck"):
            node = typecheck_program(cc, node, file=file, early_defines=defines)
        with timing.stage("translate"):
            TranslateVisitor.apply(cc, node)
            cc.module.info = Node.SrcInfo(file)
        with timing.stage("ir pass"):
            PassManager(cc.options).run(cc.module)
    else:
        node = typecheck_program(cc, node, file=file, early_defines=defines)
        TranslateVisitor.apply(cc, node)
        cc.module.info = Node.SrcInfo(file)
        PassManager(cc.options).run(cc.module)

    construct = timing.stage("llvm construct") if timing else nullcontext()
    with construct:
        module = emit_module(cc, library_api, file, defines)
    with module:
        if mode == "run":
            if timing:

                def boundary(stage):
                    timing.transition("jit link" if stage == 0 else "run")

                with timing.stage("llvm pass"):
                    module.run([args.file, *args.args], debug=cc.options.debug, on_stage=boundary)
            else:
                module.run([args.file, *args.args], debug=cc.options.debug)
        elif args.output:
            args.output.write_text(str(module), encoding="utf-8")
        else:
            sys.stdout.write(str(module))
