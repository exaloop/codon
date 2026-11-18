# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Meister's source-to-LLVM and run entry points."""

import argparse
import sys
from pathlib import Path

from ..ast.nodes import Node
from ..cache import MAX_REALIZATION_DEPTH, Cache
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from ..test import find_library
from . import llvm
from .llvisitor import LLVMVisitor
from .transform import PassManager


def main(mode, argv):
    parser = argparse.ArgumentParser(prog=f"python -m meister {mode}")
    parser.add_argument("--library", help="path to libcodonc from the Exaloop LLVM build")
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
    cc.fs.set_module0(file)
    node = cc.parse(file=file)
    # Use the native build's define order; it is observable in CIR/LLVM dumps.
    defines = native.get_early_defines() or cc.options.get_early_defines()
    node = typecheck_program(cc, node, file=file, early_defines=defines)
    TranslateVisitor.apply(cc, node)
    cc.module.info = Node.SrcInfo(file)
    PassManager(cc.options).run(cc.module)

    with llvm.Module(library_api, "codon", file) as module:
        version = ".".join(
            defines[f"__codon_version_{part}__"] for part in ("major", "minor", "micro")
        )
        LLVMVisitor(module, debug=cc.options.debug, version=version).process(cc.module)
        module.debug_builder.finalize()
        module.verify()
        if mode == "run":
            module.run([args.file, *args.args], debug=cc.options.debug)
        elif args.output:
            args.output.write_text(str(module), encoding="utf-8")
        else:
            sys.stdout.write(str(module))
