# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Compare a single program after typechecking, translation and each debug pass."""

import argparse
import ctypes
import difflib
import json
import sys
import tempfile
from pathlib import Path

from .. import ast, cache
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from ..test import _NativeResult, find_library, use_build_stdlib
from .transform import PassManager
from .util.format import dump


def dump_typed(cc, node, indent):
    result = ast.dump(node, include_attributes=True, indent=indent, types=True) + "\n"
    for _, fn in sorted(cc.functions.items()):
        for _, real in sorted(fn.realizations.items()):
            if real.ast:
                result += (
                    ast.dump(real.ast, include_attributes=True, indent=indent, types=True) + "\n"
                )
    return result


def main(argv=None):
    arguments = argparse.ArgumentParser(description=__doc__)
    arguments.add_argument("file", type=Path)
    arguments.add_argument("--library")
    arguments.add_argument("--indent", type=int, default=2)
    arguments.add_argument(
        "--ast-indent",
        type=int,
        default=0,
        help="compact typechecked ASTs avoid very large repeated metadata indentation",
    )
    arguments.add_argument("--output", type=Path, help="retain stage dumps and bounded diffs here")
    args = arguments.parse_args(argv)
    root = Path(__file__).resolve().parents[2]
    filename = args.file.resolve()
    native, library = find_library(root, args.library)
    api = native.library.codon_ir_dump_stages
    api.argtypes = [ctypes.c_char_p, ctypes.c_uint8, ctypes.c_int, ctypes.c_uint8, ctypes.c_int]
    api.restype = _NativeResult
    sys.setrecursionlimit(max(sys.getrecursionlimit(), cache.MAX_REALIZATION_DEPTH * 100))
    with tempfile.TemporaryDirectory(prefix="meister-ir-stages-") as temporary:
        output = args.output or Path(temporary)
        output.mkdir(parents=True, exist_ok=True)
        with use_build_stdlib(root, library):
            native_result = native._consume(
                api(str(filename).encode(), True, args.indent, True, args.ast_indent)
            )
            if native_result.error:
                raise RuntimeError(native_result.error)
            native_stages = json.loads(native_result.output)
            cc = cache.Cache("codon")
            node = cc.parse(file=str(filename))
            node = typecheck_program(
                cc, node, file=str(filename), early_defines=native.get_early_defines()
            )
            failed = []

            def compare(stage, actual):
                expected = native_stages.pop(stage)
                (output / f"{stage}.cpp.txt").write_text(expected)
                (output / f"{stage}.python.txt").write_text(actual)
                if expected == actual:
                    (output / f"{stage}.diff").write_text("")
                    print(f"PASS {stage} ({len(actual.encode())} bytes)", flush=True)
                    return
                failed.append(stage)
                diff = list(
                    difflib.unified_diff(
                        expected.splitlines(),
                        actual.splitlines(),
                        fromfile="C++",
                        tofile="Python",
                        n=2,
                    )
                )
                (output / f"{stage}.diff").write_text("\n".join(diff) + "\n")
                print(f"FAIL {stage}: {len(diff)} diff lines", flush=True)
                print("\n".join(diff[:60]), flush=True)

            compare("typecheck", dump_typed(cc, node, args.ast_indent))
            TranslateVisitor.apply(cc, node)
            compare(
                "translate",
                dump(cc.module, include_attributes=True, indent=args.indent, types=True),
            )
            manager = PassManager(cc.options)
            for key in manager.execution_order:
                manager.run_pass(cc.module, key)
                compare(
                    key, dump(cc.module, include_attributes=True, indent=args.indent, types=True)
                )
            if native_stages:
                raise RuntimeError(f"native-only stages: {list(native_stages)}")
            if failed:
                print(
                    f"{len(failed)} stages differ; "
                    + (f"dumps: {output}" if args.output else "use --output DIR to retain dumps")
                )
                return 1
            print("All 10 stages match exactly.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
