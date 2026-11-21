import sys
from pathlib import Path

from .bridge import *


def main(argv):
    mode = argv[0]
    root = str(Path(__file__).resolve().parent)

    from . import ast, cache, parser
    from .passes import typecheck

    if mode == "tokenize":
        from .parser.tokenize import main as tokenize_main

        tokenize_main(argv[1])
    elif mode == "scope":
        try:
            cache = cache.Cache(root)
            node = cache.parse(file=argv[1])
            node = cache.scope(node)
            print(ast.dump(node, indent=2, include_attributes=True))
        except parser.pegen.CodonSyntaxError as error:
            print(f"{error.location}: {error.msg}")
        except ast.NodeError as error:
            if error.info:
                print(f"{error.info}: {error}")
            else:
                print(error)
    elif mode == "typecheck":
        try:
            cache = cache.Cache(root)
            node = cache.parse(file=argv[1])
            node = cache.scope(node)
            node = typecheck.typecheck_program(cache, node, file=argv[1])

            print(ast.dump(node, indent=2, include_attributes=True, types=True))
            for fn_data in cache.functions.values():
                for r in fn_data.realizations.values():
                    if r.ast:
                        print(ast.dump(r.ast, indent=2, include_attributes=True, types=True))
        except parser.pegen.CodonSyntaxError as error:
            print(f"{error.location}: {error.msg}")
        except ast.NodeError as error:
            if error.info:
                print(f"{error.info}: {error}")
            else:
                print(error)
        finally:
            pass
    elif mode == "ir":
        from .ir.transform import PassManager
        from .ir.util.format import format_ir
        from .passes.translate import TranslateVisitor

        args_parser = argparse.ArgumentParser(description="Dump Codon IR without LLVM generation")
        args_parser.add_argument("-release", "--release", action="store_true", help="enable release optimizations")
        args_parser.add_argument("file")
        args_parser.add_argument("--indent", type=int, default=2)
        args_parser.add_argument("--no-attributes", action="store_true")
        args_parser.add_argument("--no-types", action="store_true")
        args_parser.add_argument("--no-field-names", action="store_true")
        args_parser.add_argument(
            "--no-passes", action="store_true", help="dump immediately after translation"
        )
        args = args_parser.parse_args(argv[1:])
        args.file = str(Path(args.file).resolve())
        # Like the native compiler's embedded stdlib, local builds include
        # generated tables that are not checked into the source stdlib.
        cc = cache.Cache(root)
        cc.options.debug = not args.release
        for build in ("build", "build-rel", "build-linux", "build-rel-linux"):
            generated = Path(root).parent / build / "stdlib"
            if generated.is_dir():
                cc.fs.add_search_path(str(generated))
                break
        sys.setrecursionlimit(max(sys.getrecursionlimit(), cache.MAX_REALIZATION_DEPTH * 100))
        node = cc.parse(file=args.file)
        node = typecheck.typecheck_program(
            cc, node, file=args.file, early_defines=cc.options.get_early_defines()
        )
        TranslateVisitor.apply(cc, node)
        if not args.no_passes:
            PassManager(cc.options).run(cc.module)
        sys.stdout.write(
            format_ir(
                cc.module,
                annotate_fields=not args.no_field_names,
                include_attributes=not args.no_attributes,
                indent=args.indent,
                types=not args.no_types,
            )
        )
    elif mode in {"llvm", "run"}:
        from .ir import codegen

        codegen.main(mode, argv[1:])
    elif mode == "test":
        from . import test

        test.main(argv[1:])
    else:
        print(f"Invalid mode: {mode}")


# from .parser import *
# def main(argv):
#     mode = argv[0]
#     if mode == "tokenize":
#         from .parser.tokenize import main

#         main(argv[1])
#     elif mode == "parse":
#         from .parser.pegen import simple_parser_main, Tokenizer
#         from .parser.tokenize import generate_tokens
#         from .parser.ast import dump, NodeVisitor
#         from .parser.parser import CodonParser
#         from .parser.scope import ScopeVisitor

#         file = """a = 5
# b = "woo"
# a and b or not c.d and a & b | c.x and True
# if b:
#     [a, 2]
# else:
#     foo(1, 2)

# @par(x=5)
# for i in a: print(i)

# @nocapture
# @llvm
# def __atomic_add__(d: Ptr[float32], b: float32) -> float32:
#     %5 = cmpxchg weak ptr %d, i32 %1, i32 %4 seq_cst monotonic, align 4
#     %6 = extractvalue { i32, i1 } %5, 1
#     br i1 %6, label %15, label %7
#     7:                                                ; preds = %0, %7
#     %8 = phi { i32, i1 } [ %13, %7 ], [ %5, %0 ]
#     ret float %16

# a |> b
# a ||> c or D |> e

# from C.foo import bar(int, str) -> float as baz
# from C.foo import ( bar(int, foo[x]) as baz, xa, xa() -> int as haha, fa() )

# WOOLY hai:
#     bar()
# WOOLY: lol()
# """
#         f = [l+"\n" for l in file.split("\n")]
#         tokenizer = Tokenizer(generate_tokens(f), verbose=False)
#         parser = CodonParser(tokenizer, verbose=False)
#         parser.SOFT_KEYWORDS.append("WOOLY")
#         tree = parser.start()
#         if not tree:
#             err = parser.make_syntax_error("fn")
#             print(err, err.location)
#         else:
#             print(dump(Codon.unwrap(tree), indent=2))

#         class Printer(NodeVisitor):
#             def visit_Name(self, node):
#                 print(f"const: {node.lineno:2}: {node.id}")
#                 return super().visit_Name(node)
#         Printer().visit(tree)

#         ScopeVisitor().visit(tree)

#         # simple_parser_main(CodonParser, argv[1:])
#         pass

if __name__ == "__main__":
    main(sys.argv[1:])
