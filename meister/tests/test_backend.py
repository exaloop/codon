"""Regression coverage for the native-library boundary and build outputs."""

from pathlib import Path
import subprocess
import sys

import pytest

from ..cache import FrontendOptions
from ..ir import build
from ..ir.llvm import Library, Module


@pytest.mark.parametrize("debug", [True, False], ids=["debug", "release"])
def test_llvm_backend(llvm_library, codon_runtime, tmp_path, debug):
    library = Library(llvm_library, codon_runtime)
    options = FrontendOptions(debug=debug)

    def program():
        module = Module(library, "backend_test", "backend_test.codon")
        i32 = module.int_type(32)
        main = module.add_function("main", i32.function((i32, module.pointer_type())))
        module.builder.set_insert_point(main.append_block("entry"))
        module.builder.create_ret(i32.const_int(23))
        return module

    with program() as module:
        assert module.run(["backend_test"], debug=debug) == 23

    for kind in ("llvm", "bc", "asm", "obj", "exe", "lib"):
        if sys.platform == "win32" and kind in {"exe", "lib"}:
            continue
        path = build.output_path(str(tmp_path / "backend_test.codon"), kind)
        with program() as module:
            build.write(module, path, kind, options)
        assert path.stat().st_size
        if kind == "exe":
            assert subprocess.run([path], check=False).returncode == 23

    maps = Path("/proc/self/maps")
    if maps.is_file():
        loaded = maps.read_text(encoding="utf-8")
        assert str(Path(llvm_library).resolve()) in loaded or "libLLVM" in loaded
        assert "libcodonrt" in loaded
        assert "libcodonc" not in loaded


@pytest.mark.skipif(sys.platform != "linux", reason="LLVM GDB listener requires the Linux JIT layer")
def test_jit_registers_and_unregisters_gdb_object(llvm_library, codon_runtime):
    from ctypes import Structure, c_void_p

    from ..ir.llvm import U

    class Descriptor(Structure):
        _fields_ = [
            ("version", U), ("action", U), ("relevant", c_void_p), ("first", c_void_p),
        ]

    library = Library(llvm_library, codon_runtime)
    if library.arch != "X86" or not library.triple.startswith(b"x86_64-"):
        pytest.skip("writable section and GDB callbacks are verified on Linux x86-64")
    descriptor = Descriptor.in_dll(library._dll, "__jit_debug_descriptor")
    initial = descriptor.first

    def after_link(stage):
        if stage == 1:
            assert descriptor.first != initial
            assert descriptor.action == 1  # JIT_REGISTER_FN

    with Module(library, "debug_test", "debug_test.codon") as module:
        module.parse_and_link("define i32 @main(i32 %argc, ptr %argv) { ret i32 0 }")
        assert module.run(["debug_test"], debug=True, on_stage=after_link) == 0
    assert descriptor.first == initial
    assert descriptor.action == 2  # JIT_UNREGISTER_FN


@pytest.mark.compiler
def test_jit_backtrace_reports_codon_frames(llvm_library, codon_runtime, tmp_path, pytestconfig):
    import re
    import signal

    from ..ir.jit_debug import has_runtime_callback, symbolizer_path
    from .support import compiler_environment, run_process

    library = Library(llvm_library, codon_runtime)
    if not has_runtime_callback(library):
        pytest.skip("runtime has no C entry point for the JIT error callback")
    if not symbolizer_path(library):
        pytest.skip("the LLVM installation has no llvm-symbolizer")

    source = tmp_path / "jit_error.codon"
    source.write_text(
        'def fail():\n    raise ValueError("JIT backtrace")\n\n'
        'def outer():\n    fail()\n\nouter()\n', encoding="utf-8",
    )
    result = run_process(
        [sys.executable, "-m", "meister", "run", "-debug", "--library", str(llvm_library),
         "--runtime", str(codon_runtime), str(source)],
        cwd=tmp_path, env=compiler_environment(codon_runtime.parent), capture_output=True,
        timeout=pytestconfig.getoption("--codon-timeout"),
    )
    stderr = re.sub(r"\x1b\[[0-9;]*m", "", result.stderr.decode(errors="replace"))
    assert result.returncode == -signal.SIGABRT, stderr
    assert "ValueError: JIT backtrace" in stderr
    backtrace = stderr.split("Backtrace:\n")[1]
    frames = [re.sub(r"^  \[0x[0-9a-f]+\] ", "", line) for line in backtrace.splitlines()]
    assert frames == [
        f"fail.0:0.0 at {source}:2:5",
        f"outer.0:0.0 at {source}:5:5",
        f"main.0 at {source}:7:1",
    ]
