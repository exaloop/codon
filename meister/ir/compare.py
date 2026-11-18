# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Compare typechecking, translation, and final debug IR; never intermediate passes."""

import argparse
import ctypes
import faulthandler
import gzip
import hashlib
import json
import multiprocessing
import os
import re
import signal
import sys
import tempfile
import time
import traceback
from pathlib import Path

from .. import ast, cache, parser
from ..error import ParserError, ScopeError
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from ..test import _NativeResult, discover_tests, find_library, prepare_python, use_build_stdlib
from .transform import PassManager
from .util.format import iter_dump

STAGES = ("typecheck", "translate", "ir")
_DumpWriter = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t)


def dump_typed(cc, node, indent):
    # Preserve the exact forest bytes without retaining every function's dump.
    yield ast.dump(node, include_attributes=True, indent=indent, types=True)
    yield "\n"
    for _, fn in sorted(cc.functions.items()):
        for _, real in sorted(fn.realizations.items()):
            if real.ast:
                yield ast.dump(real.ast, include_attributes=True, indent=indent, types=True)
                yield "\n"


class DumpFingerprint:
    """C++: capi.cpp. Ordered SHA-256 chunks avoid LLVM 20's long-message bug."""

    CHUNK_BYTES = 64 * 1024 * 1024

    def __init__(self):
        self.hash = hashlib.sha256()
        self.size, self.offset = 0, 0
        self.chunks = []

    def update(self, data):
        self.size += len(data)
        remaining = memoryview(data)
        while remaining:
            size = min(self.CHUNK_BYTES - self.offset, len(remaining))
            self.hash.update(remaining[:size])
            remaining = remaining[size:]
            self.offset += size
            if self.offset == self.CHUNK_BYTES:
                self.commit()

    def commit(self):
        self.chunks.append(self.hash.hexdigest())
        self.hash = hashlib.sha256()
        self.offset = 0

    def finish(self):
        if self.offset or not self.chunks:
            self.commit()
        return {"bytes": self.size, "sha256_chunks": self.chunks}


def signature(data):
    digest = DumpFingerprint()
    digest.update(data)
    return digest.finish()


def _native_stages(native, case, indent, ast_indent, summary, directory):
    outputs, failures = {}, []

    def write(stage, data, size):
        stage = stage.decode()
        if failures or not (directory / f"{stage}.python.gz").exists():
            return
        try:
            if stage not in outputs:
                outputs[stage] = gzip.open(  # noqa: SIM115 - callback streams close in finally
                    directory / f"{stage}.cpp.gz", "wb", compresslevel=1
                )
            for offset in range(0, size, 65536):
                outputs[stage].write(ctypes.string_at(data + offset, min(65536, size - offset)))
        except Exception:  # noqa: BLE001 - callback errors must not disappear inside ctypes
            failures.append(traceback.format_exc())

    writer = _DumpWriter(write) if not summary else None
    api = (
        native.library.codon_ir_dump_compare
        if summary
        else native.library.codon_ir_dump_compare_write
    )
    api.argtypes = [
        ctypes.c_char_p,
        ctypes.c_char_p,
        ctypes.c_int,
        ctypes.c_uint8,
        ctypes.c_uint8 if summary else _DumpWriter,
        ctypes.c_int,
        ctypes.c_int,
    ]
    api.restype = _NativeResult
    try:
        result = native._consume(
            api(
                None if case.code is None else case.code.encode(),
                str(case.path).encode(),
                case.line,
                case.barebones,
                True if summary else writer,
                indent,
                ast_indent,
            )
        )
    finally:
        for output in outputs.values():
            output.close()
    if failures:
        raise RuntimeError(failures[0])
    if result.error:
        raise RuntimeError(result.error)
    return json.loads(result.output)


def _native_worker(sender, native, case, indent, ast_indent, summary, directory):
    signal.signal(signal.SIGTERM, signal.SIG_DFL)
    try:
        sender.send((True, _native_stages(native, case, indent, ast_indent, summary, directory)))
    except Exception:  # noqa: BLE001 - report a native helper failure to the case worker
        sender.send((False, traceback.format_exc()))
    finally:
        sender.close()


def native_stages(native, case, indent, ast_indent, summary=True, directory=None):
    # Release the native compiler's large arenas/allocator caches with its process
    # before Python allocates its own AST/IR dumps. Only small digests cross here.
    context = multiprocessing.get_context("fork")
    receiver, sender = context.Pipe(duplex=False)
    process = context.Process(
        target=_native_worker, args=(sender, native, case, indent, ast_indent, summary, directory)
    )
    process.start()
    sender.close()
    try:
        try:
            ok, result = receiver.recv()
        except EOFError:
            process.join()
            raise RuntimeError(f"native comparison worker exited with {process.exitcode}") from None
        if not ok:
            raise RuntimeError(result)
        return result
    finally:
        receiver.close()
        process.join(timeout=1)
        if process.is_alive():
            process.terminate()
        process.join()


def python_stages(case, prepared, defines, indent, ast_indent, expected, directory):
    cc, preamble = prepared
    stages = {}
    phase = "parse"

    def capture(stage, chunks):
        digest = DumpFingerprint()
        for text in chunks():
            data = text.encode("utf-8", errors="surrogatepass")
            digest.update(data)
            del data, text
        stages[stage] = digest.finish()
        (directory / "python-signatures.json").write_text(json.dumps(stages, indent=2))
        if stages[stage] != expected.get(stage):
            with gzip.open(directory / f"{stage}.python.gz", "wb", compresslevel=1) as output:
                for text in chunks():
                    output.write(text.encode("utf-8", errors="surrogatepass"))

    try:
        cc.fs.set_module0(str(case.path))
        node = parser.parse(file=str(case.path), code=case.code, line_offset=case.line)
        phase = "typecheck"
        node = typecheck_program(
            cc,
            node,
            file=str(case.path),
            early_defines=defines,
            barebones=case.barebones,
            _preamble=preamble,
        )
        capture(phase, lambda: dump_typed(cc, node, ast_indent))
        phase = "translate"
        TranslateVisitor.apply(cc, node)
        capture(
            phase, lambda: iter_dump(cc.module, include_attributes=True, indent=indent, types=True)
        )
        phase = "ir"
        PassManager(cc.options).run(cc.module)
        capture(
            phase, lambda: iter_dump(cc.module, include_attributes=True, indent=indent, types=True)
        )
    except Exception as error:  # noqa: BLE001 - report compiler/port failures per checkpoint
        kind = (
            "compiler"
            if isinstance(
                error,
                (
                    ParserError,
                    ScopeError,
                    ast.NodeError,
                    parser.pegen.CodonSyntaxError,
                    parser.tokenize.TokenError,
                ),
            )
            else "internal"
        )
        stages["_error"] = {
            "stage": phase,
            "message": getattr(error, "msg", str(error)),
            "kind": kind,
            "trace": traceback.format_exc(),
        }
    return stages


def first_difference(expected, actual):
    offset = 0
    with gzip.open(expected, "rb") as cpp, gzip.open(actual, "rb") as python:
        while True:
            left, right = cpp.read(65536), python.read(65536)
            if left != right:
                offset += next(
                    (idx for idx, (a, b) in enumerate(zip(left, right)) if a != b),
                    min(len(left), len(right)),
                )
                break
            if not left:
                break
            offset += len(left)
        low = max(0, offset - 400)
        cpp.seek(low)
        python.seek(low)
        return {
            "offset": offset,
            "cpp": cpp.read(offset + 700 - low).decode("utf-8", errors="backslashreplace"),
            "python": python.read(offset + 700 - low).decode("utf-8", errors="backslashreplace"),
        }


def compare_case(case, native, prepared, defines, indent, ast_indent, directory):
    directory.mkdir(parents=True, exist_ok=True)
    expected = native_stages(native, case, indent, ast_indent)
    (directory / "cpp-signatures.json").write_text(json.dumps(expected, indent=2))
    actual = python_stages(case, prepared, defines, indent, ast_indent, expected, directory)
    differences = [stage for stage in STAGES if expected.get(stage) != actual.get(stage)]
    cpp_error, py_error = expected.get("_error"), actual.get("_error")
    status = "match"
    if cpp_error or py_error:
        # Negative fixtures have no IR. Compare their compiler rejection, never
        # accept Python implementation errors or failures during translation/IR.
        if (
            cpp_error
            and py_error
            and cpp_error["kind"] == py_error["kind"] == "compiler"
            and cpp_error["stage"] in ("parse", "typecheck")
            and py_error["stage"] in ("parse", "typecheck")
            and not differences
        ):
            status = "rejected"
        else:
            status = "mismatch"
    elif differences:
        status = "mismatch"
    result = {
        "case": case.label,
        "status": status,
        "mismatches": differences,
        "cpp": expected,
        "python": actual,
        "directory": str(directory),
    }
    if status == "mismatch" and differences:
        full = native_stages(native, case, indent, ast_indent, False, directory)
        result["diffs"] = {}
        for stage in differences:
            if stage not in full:
                continue
            cpp = directory / f"{stage}.cpp.gz"
            path = directory / f"{stage}.python.gz"
            if cpp.exists() and path.exists():
                result["diffs"][stage] = first_difference(cpp, path)
    (directory / "result.json").write_text(json.dumps(result, indent=2))
    return result


def worker(sender, case, native, prepared, defines, indent, ast_indent, directory):
    def terminate(signum, frame):
        # Let native_stages' finally stop its helper on a timeout or cancellation.
        raise SystemExit(128 + signum)

    try:
        signal.signal(signal.SIGTERM, terminate)
        directory.mkdir(parents=True, exist_ok=True)
        with open(directory / "stderr.txt", "w") as error_output:
            os.dup2(error_output.fileno(), 2)
            faulthandler.enable(file=error_output)
            faulthandler.register(signal.SIGUSR1, file=error_output)
            sender.send(
                compare_case(case, native, prepared, defines, indent, ast_indent, directory)
            )
    except Exception:  # noqa: BLE001 - isolate and report a failed test worker
        sender.send(
            {
                "case": case.label,
                "status": "mismatch",
                "worker_error": traceback.format_exc(),
                "directory": str(directory),
            }
        )
    finally:
        sender.close()


def main(argv=None):
    arguments = argparse.ArgumentParser(description=__doc__)
    arguments.add_argument(
        "file", type=Path, help="Codon file or directory containing test_*.codon"
    )
    arguments.add_argument("--library")
    arguments.add_argument("--indent", type=int, default=0)
    arguments.add_argument("--ast-indent", type=int, default=0)
    arguments.add_argument("--output", type=Path)
    arguments.add_argument("--filter", help="run matching case labels (all cases by default)")
    arguments.add_argument("--jobs", type=int, default=1)
    arguments.add_argument("--timeout", type=float, default=180)
    arguments.add_argument("--stop-on-error", action="store_true")
    args = arguments.parse_args(argv)
    root = Path(__file__).resolve().parents[2]
    path = args.file.resolve()
    files = sorted(path.glob("test_*.codon")) if path.is_dir() else [path]
    cases = [case for filename in files for case in discover_tests(filename)]
    if args.filter:
        cases = [case for case in cases if args.filter in case.label]
    if not cases:
        arguments.error("no matching test cases")
    if args.jobs < 1:
        arguments.error("--jobs must be positive")
    native, library = find_library(root, args.library)
    output = (args.output or Path(tempfile.mkdtemp(prefix="meister-ir-compare-"))).resolve()
    output.mkdir(parents=True, exist_ok=True)
    sys.setrecursionlimit(max(sys.getrecursionlimit(), cache.MAX_REALIZATION_DEPTH * 100))
    context = multiprocessing.get_context("fork")
    results, running = [], []
    print(
        f"Comparing {len(cases)} cases at typecheck, translate, and final IR; output: {output}",
        flush=True,
    )
    with use_build_stdlib(root, library):
        defines = native.get_early_defines()
        prepared = {}
        for barebones in sorted({case.barebones for case in cases}):
            print(f"Preparing {'barebones' if barebones else 'full'} Python stdlib", flush=True)
            prepared[barebones] = prepare_python(2 if barebones else 1, defines)
        pending = iter(enumerate(cases))
        exhausted = False
        stop = False
        while running or not exhausted:
            while len(running) < args.jobs and not exhausted and not stop:
                try:
                    idx, case = next(pending)
                except StopIteration:
                    exhausted = True
                    break
                identifier = re.sub(
                    r"[^\w.-]+", "_", f"{case.path.stem}-{case.line + 1}-{case.name}"
                )
                directory = output / identifier
                receiver, sender = context.Pipe(duplex=False)
                process = context.Process(
                    target=worker,
                    args=(
                        sender,
                        case,
                        native,
                        prepared[case.barebones],
                        defines,
                        args.indent,
                        args.ast_indent,
                        directory,
                    ),
                )
                process.start()
                sender.close()
                running.append((idx, case, directory, process, receiver, time.monotonic()))
            for item in list(running):
                idx, case, directory, process, receiver, started = item
                result = None
                if receiver.poll():
                    try:
                        result = receiver.recv()
                    except EOFError:
                        pass
                if result is None and time.monotonic() - started > args.timeout:
                    process.terminate()
                    result = {
                        "case": case.label,
                        "status": "mismatch",
                        "worker_error": f"timeout after {args.timeout} seconds",
                        "directory": str(directory),
                    }
                if result is None and not process.is_alive():
                    result = {
                        "case": case.label,
                        "status": "mismatch",
                        "worker_error": f"worker exit {process.exitcode}",
                        "directory": str(directory),
                    }
                if result is None:
                    continue
                process.join()
                receiver.close()
                running.remove(item)
                results.append(result)
                elapsed = time.monotonic() - started
                print(
                    f"{result['status'].upper()} {idx + 1}/{len(cases)} {case.label} ({elapsed:.1f}s)",
                    flush=True,
                )
                if result["status"] == "mismatch":
                    print(
                        json.dumps(
                            {
                                key: result[key]
                                for key in ("mismatches", "diffs", "worker_error")
                                if key in result
                            },
                            indent=2,
                        ),
                        flush=True,
                    )
                    if "cpp" in result:
                        print("C++ error:", result["cpp"].get("_error"), flush=True)
                        print("Python error:", result["python"].get("_error"), flush=True)
                    stop |= args.stop_on_error
                (output / "summary.json").write_text(json.dumps(results, indent=2))
            if stop:
                exhausted = True
            if running:
                time.sleep(0.02)
    counts = {
        name: sum(result["status"] == name for result in results)
        for name in ("match", "rejected", "mismatch")
    }
    print(f"Completed {len(results)}/{len(cases)}: {counts}", flush=True)
    return int(bool(counts["mismatch"]))


if __name__ == "__main__":
    raise SystemExit(main())
