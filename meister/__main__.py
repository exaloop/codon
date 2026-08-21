import sys
from .bridge import *


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "tokenize":
        from .parser.tokenize import main

        main(sys.argv[2])
    elif mode == "parse":
        from .parser.pegen import simple_parser_main, Tokenizer
        from .parser.tokenize import generate_tokens
        from .parser.ast import dump, NodeVisitor
        from .parser.parser import CodonParser

# Directive: '##' _ 'codon:' _ NAME _ '=' _ (INT / NAME)

        file = """a = 5
b = "woo"
a and b or not c.d and a & b | c.x and True
if b:
    [a, 2]
else:
    foo(1, 2)

@par(x=5)
for i in a: print(i)

@nocapture
@llvm
def __atomic_add__(d: Ptr[float32], b: float32) -> float32:
    %5 = cmpxchg weak ptr %d, i32 %1, i32 %4 seq_cst monotonic, align 4
    %6 = extractvalue { i32, i1 } %5, 1
    br i1 %6, label %15, label %7
    7:                                                ; preds = %0, %7
    %8 = phi { i32, i1 } [ %13, %7 ], [ %5, %0 ]
    ret float %16

a |> b
a ||> c or D |> e

from C.foo import bar(int, str) -> float as baz
from C.foo import ( bar(int, foo[x]) as baz, xa, xa() -> int as haha, fa() )

WOOLY hai:
    bar()
WOOLY: lol()
"""
        f = [l+"\n" for l in file.split("\n")]
        tokenizer = Tokenizer(generate_tokens(f), verbose=False)
        parser = CodonParser(tokenizer, verbose=False)
        parser.SOFT_KEYWORDS.append("WOOLY")
        tree = parser.start()
        if not tree:
            err = parser.make_syntax_error("fn")
            print(err, err.location)
        else:
            print(dump(Codon.unwrap(tree), indent=2))

        class Printer(NodeVisitor):
            def visit_Name(self, node):
                print(f"const: {node.lineno:2}: {node.id}")
                return super().visit_Name(node)
        Printer().visit(tree)

        # simple_parser_main(CodonParser, sys.argv[2:])
        pass
