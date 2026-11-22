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
