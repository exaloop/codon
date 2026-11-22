# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Meister's source-to-LLVM and run entry points."""

import argparse
import os
import subprocess
import sysconfig
import sys
import time
from contextlib import contextmanager, nullcontext
from pathlib import Path

from ..ast.nodes import Node, NodeError
from ..parser.pegen import CodonSyntaxError
from ..parser.tokenize import TokenError
from ..cache import MAX_REALIZATION_DEPTH, Cache, FrontendOptions
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from ..error import ParserError
from . import build, llvm
from .toolchain import BUILD_NAMES, ROOT, llvm_path, runtime_path
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
        visitor = LLVMVisitor(
            module, debug=cc.options.debug, standalone=standalone, version=version
        )
        visitor.process(cc.module)
        if cc.options.pyext:
            from .pyext import PythonExtension

            PythonExtension(visitor, cc.py_module).emit()
        module.debug_builder.finalize()
        module.verify()
        return module
    except BaseException:
        module.close()
        raise


def prepare_compiler(runtime, options):
    sys.setrecursionlimit(max(sys.getrecursionlimit(), MAX_REALIZATION_DEPTH * 100))
    cache = Cache(str(runtime.parent / "meister"), options=options)
    paths = (ROOT / "stdlib", *(ROOT / name / "stdlib" for name in BUILD_NAMES))
    for path in paths:
        if path.is_dir() and path not in cache.fs.get_stdlib_paths():
            cache.fs.add_search_path(str(path))
    return cache, options.get_early_defines()


def configure_python():
    """Make the host Python library available to the runtime's Python bridge."""
    if sys.platform == "win32":
        name = f"python{sys.version_info.major}{sys.version_info.minor}.dll"
        library = Path(sys.base_prefix) / name
    else:
        directory = sysconfig.get_config_var("LIBDIR")
        name = sysconfig.get_config_var("LDLIBRARY")
        if not directory or not name:
            return
        library = Path(directory) / name
    if library.is_file():
        os.environ.setdefault("CODON_PYTHON", str(library))


def parse_args(mode, argv):
    parser = argparse.ArgumentParser(prog=f"python -m meister {mode}")
    optimization = parser.add_mutually_exclusive_group()
    for opt in ("debug", "release"):
        optimization.add_argument(f"-{opt}", f"--{opt}", dest="release",
                                  action="store_const", const=opt == "release")
    parser.add_argument("--library", "--llvm-library", help="path to the shared LLVM library")
    parser.add_argument("--runtime", type=Path, help="path to libcodonrt or its directory")
    parser.add_argument("-D", action="append", default=[], metavar="NAME=VALUE")
    parser.add_argument("-numerics", "--numerics", choices=("c", "py"), default="py")
    parser.add_argument("-auto-python", "--auto-python", action="store_true")
    parser.add_argument("-unordered-dict", "--unordered-dict", action="store_true")
    parser.add_argument("-disable-opt", "--disable-opt", action="append", default=[])
    parser.add_argument("-disable-native", "--disable-native", action="store_true")
    parser.add_argument("-fast-math", "--fast-math", action="store_true")
    if mode == "run":
        parser.add_argument(
            "-time", "--time", action="store_true", help="report timings and peak memory"
        )
        parser.add_argument("-l", action="append", default=[], metavar="LIBRARY")
    else:
        parser.add_argument("-o", "--output", type=Path)
    if mode == "build":
        kinds = parser.add_mutually_exclusive_group()
        for kind in (*build.EXTENSIONS, "detect"):
            kinds.add_argument(f"-{kind}", f"--{kind}", dest="kind", action="store_const", const=kind)
        parser.set_defaults(kind="detect")
        parser.add_argument("-l", action="append", default=[], metavar="LIBRARY")
        parser.add_argument("-linker-flags", "--linker-flags", default="")
        parser.add_argument("-pie", "--pie", action="store_true")
        parser.add_argument("-module", "--module", help="Python extension module name")
        parser.add_argument("-relocation-model", "--relocation-model", choices=("pic", "static"))
        parser.add_argument(
            "-global-ctor", "--global-ctor", choices=("auto", "no", "yes"), default="auto"
        )
    parser.add_argument("file", nargs="?", default="-")
    if mode == "run":
        parser.add_argument("args", nargs=argparse.REMAINDER, help="arguments to the Codon program")
    args = parser.parse_args(argv)
    if mode == "build":
        if args.pie:
            if args.kind not in {"detect", "exe"}:
                parser.error("--pie requires executable output")
            args.kind = "exe"
        if args.output is None:
            if args.file == "-":
                parser.error("output file must be specified when reading from stdin")
            args.output = build.output_path(args.file, args.kind)
        if args.kind == "detect":
            args.kind = build.output_kind(args.output)
        if args.file != "-" and args.output.resolve() == Path(args.file).resolve():
            parser.error("output file must differ from the source file")
    return args


def user_defines(definitions):
    result = {}
    for definition in definitions:
        name, sep, value = definition.partition("=")
        if not name or not sep:
            print(f"warning: ignoring malformed definition: {definition}", file=sys.stderr)
        elif name in result:
            print(f"warning: ignoring duplicate definition: {definition}", file=sys.stderr)
        else:
            result[name] = value
    return result


def compile_source(cc, defines, file, source, definitions, timing):
    if timing:
        # Typechecking also parses and scopes imports.
        parse, scope = cc.parse, cc.scope

        def timed_parse(*args, **kwargs):
            with timing.stage("parse"):
                return parse(*args, **kwargs)

        def timed_scope(*args, **kwargs):
            with timing.stage("scope"):
                return scope(*args, **kwargs)

        cc.parse, cc.scope = timed_parse, timed_scope

    def stage(name):
        return timing.stage(name) if timing else nullcontext()

    with stage("parse main"):
        node = cc.parse(file=file, code=source)
    with stage("typecheck"):
        node = typecheck_program(cc, node, file=file, defines=definitions, early_defines=defines)
    with stage("translate"):
        TranslateVisitor.apply(cc, node)
        cc.module.info = Node.SrcInfo(file)
    with stage("ir pass"):
        PassManager(cc.options).run(cc.module)


def execute(mode, args):
    runtime = runtime_path(args.runtime)
    options = FrontendOptions(
        debug=not args.release,
        pyext=mode == "build" and args.kind == "pyext",
        pynum=args.numerics == "py",
        autopy=args.auto_python,
        unordereddict=args.unordered_dict,
        fast_math=args.fast_math,
        native=not args.disable_native,
        disabled=tuple(args.disable_opt),
    )
    file = str(Path(args.file).resolve()) if args.file != "-" else "-"
    source = sys.stdin.read() if args.file == "-" else None
    cc, defines = prepare_compiler(runtime, options)
    cc.fs.set_module0(file)
    timing = StageTimings() if mode == "run" and args.time else None
    if timing:
        print("Meister timings (exclusive wall time):", file=sys.stderr, flush=True)
    compile_source(cc, defines, file, source, user_defines(args.D), timing)

    if mode == "build" and args.kind == "cir":
        from .util.format import format_ir

        args.output.write_text(
            format_ir(cc.module, include_attributes=True, indent=2, types=True),
            encoding="utf-8",
        )
        return 0

    if options.pyext:
        cc.py_module.name = args.module or Path(args.file).stem
    library = llvm.Library(llvm_path(build=runtime.parent, requested=args.library), runtime)
    construct = timing.stage("llvm construct") if timing else nullcontext()
    with construct:
        # The runtime's standalone exception handler keeps a C++ exception
        # from unwinding across the ctypes entry point.
        module = emit_module(cc, library, file, defines, standalone=mode != "llvm")
    with module:
        if mode == "run":
            from ctypes import CDLL, RTLD_GLOBAL

            configure_python()
            library.load_runtime()
            for library_path in args.l:
                CDLL(library_path, mode=RTLD_GLOBAL)

            def boundary(stage):
                timing.transition("jit link" if stage == 0 else "run")

            with timing.stage("llvm pass") if timing else nullcontext():
                return module.run(
                    [args.file, *args.args], debug=options.debug, fast_math=options.fast_math,
                    native=options.native,
                    on_stage=boundary if timing else None,
                )
        if mode == "build":
            build.write(module, args.output, args.kind, options, libraries=args.l,
                        linker_flags=args.linker_flags, pie=args.pie, ctor=args.global_ctor,
                        relocation=args.relocation_model)
        elif args.output:
            module.write(args.output, "llvm")
        else:
            sys.stdout.write(str(module))
    return 0


def main(mode, argv):
    args = parse_args(mode, argv)
    try:
        return execute(mode, args)
    except ParserError as error:
        for group in error.errors:
            for message in group:
                print(message, file=sys.stderr)
    except (NodeError, CodonSyntaxError, TokenError) as error:
        print(error, file=sys.stderr)
    except (llvm.LLVMError, OSError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
    return 1
