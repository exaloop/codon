"""Compile one source case in a subprocess, including native runtime execution."""

import argparse
from pathlib import Path

from .. import ast, parser
from ..cache import FrontendOptions
from ..error import ParserError
from ..ir import llvm
from ..ir.codegen import emit_module, prepare_compiler
from ..ir.transform import PassManager
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import typecheck_program
from .cases import TestCase, split_test_file
from .transforms import register_test_passes


def compile_cir(cache, defines, case, *, fixture_passes=False):
    file = str(case.path)
    cache.fs.set_module0(file)
    node = parser.parse(file=file, code=case.code, line_offset=case.line)
    checked = typecheck_program(
        cache, node, file=file, early_defines=defines, barebones=case.barebones
    )
    TranslateVisitor.apply(cache, checked)
    cache.module.info = ast.Node.SrcInfo(file)
    manager = PassManager(cache.options)
    if fixture_passes:
        register_test_passes(manager)
    manager.run(cache.module)
    return cache.module


def main(argv=None):
    arguments = argparse.ArgumentParser()
    arguments.add_argument("file", type=Path)
    arguments.add_argument("--library", required=True)
    arguments.add_argument("--runtime", type=Path, required=True)
    arguments.add_argument("--section", type=int)
    arguments.add_argument("--release", action="store_true")
    arguments.add_argument("--c-numerics", action="store_true")
    arguments.add_argument("--compile-only", action="store_true")
    arguments.add_argument("--fixture-passes", action="store_true")
    args = arguments.parse_args(argv)
    options = FrontendOptions(debug=not args.release, test=True, pynum=not args.c_numerics)
    cache, defines = prepare_compiler(args.runtime, options)
    case = TestCase(args.file) if args.section is None else next(
        case for case in split_test_file(args.file) if case.line == args.section
    )

    try:
        compile_cir(cache, defines, case, fixture_passes=args.fixture_passes)
    except ParserError as error:
        for group in error.errors:
            for message in group:
                print(message.message)
        return 1
    except (ast.NodeError, parser.pegen.CodonSyntaxError, parser.tokenize.TokenError) as error:
        print(error.args[0])
        return 1

    with emit_module(
        cache, llvm.Library(args.library, args.runtime), str(case.path), defines, standalone=True
    ) as module:
        if not args.compile_only:
            return module.run([str(case.path)], debug=options.debug)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
