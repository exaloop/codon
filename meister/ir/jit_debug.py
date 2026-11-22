# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Format Codon's JITError backtrace using the loaded LLVM object addresses."""

from contextlib import contextmanager
from ctypes import CFUNCTYPE, POINTER, string_at
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import NamedTemporaryFile

from .llvm import N, P

JIT_ERROR_CALLBACK = CFUNCTYPE(None, P, N, POINTER(N), N)


def has_runtime_callback(library):
    return (
        sys.platform == "linux" and library.arch == "X86"
        and library.triple.startswith(b"x86_64-")
        and hasattr(library.load_runtime(), "seq_set_jit_error_callback")
    )


def symbolizer_path(library):
    path = Path(library.path)
    if path.is_absolute():
        symbolizer = path.parent.parent / "bin" / "llvm-symbolizer"
        if symbolizer.is_file():
            return str(symbolizer)
    return shutil.which("llvm-symbolizer")


def symbolize_frames(symbolizer, objects, pcs):
    # C++ DebugListener retains the original object and the loaded text start.
    # llvm-symbolizer uses the same LLVMSymbolizer implementation.
    frames = {}
    for obj in objects.values():
        if not obj.data:
            continue
        offsets = {
            pc: pc - start
            for pc in pcs
            for start, end in obj.text if start <= pc < end
        }
        if not offsets:
            continue
        with NamedTemporaryFile(suffix=".o") as file:
            file.write(obj.data)
            file.flush()
            result = subprocess.run(
                [symbolizer, "--no-inlines", "--output-style=JSON", "--obj", file.name],
                input="".join(f"{offset:#x}\n" for offset in offsets.values()),
                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, check=True,
            )
        for pc, line in zip(offsets, result.stdout.splitlines()):
            entries = json.loads(line).get("Symbol", ())
            if entries:
                frames[pc] = entries[0]
    return frames


def format_frame(pc, frame):
    function, file = frame.get("FunctionName", ""), frame.get("FileName", "")
    if function in ("", "??", "<invalid>") or file in ("", "??", "<invalid>"):
        return ""
    result = f"  [\033[33m{pc:#x}\033[0m] \033[32m{function}\033[0m at \033[36m{file}\033[0m"
    if frame.get("Line"):
        result += f":\033[33m{frame['Line']}\033[0m"
        if frame.get("Column"):
            result += f":\033[33m{frame['Column']}\033[0m"
    return result + "\n"


@contextmanager
def error_callback(library, manager, debug):
    if not has_runtime_callback(library):
        yield
        return
    symbolizer = symbolizer_path(library) if manager and debug else None

    @JIT_ERROR_CALLBACK
    def report(message, size, pcs, count):
        os.write(2, string_at(message, size))
        if debug:
            os.write(2, b"\n\033[1mBacktrace:\033[0m\n")
        if not symbolizer or not count:
            return
        addresses = pcs[:count]
        try:
            frames = symbolize_frames(symbolizer, manager.objects, addresses)
            for pc in addresses:
                if pc in frames:
                    os.write(2, format_frame(pc, frames[pc]).encode())
        except (OSError, ValueError, subprocess.CalledProcessError):
            # Preserve the original error when symbolization fails.
            return

    setter = library.runtime_function("seq_set_jit_error_callback", None, JIT_ERROR_CALLBACK)
    setter(report)
    try:
        yield
    finally:
        setter(JIT_ERROR_CALLBACK())
