from .. import ast
from . import parser, pegen, tokenize


def parse(
    file: str | None = None, code: str | None = None, verbose=False, rule: str = "start"
) -> ast.Node:
    assert (file is not None) ^ (code is not None), "bad arguments"

    def helper(gen):
        tokenizer = pegen.Tokenizer(gen, verbose=verbose)
        engine = parser.CodonParser(tokenizer, verbose=verbose)
        tree = engine.parse(rule)
        assert tree
        return tree

    with ast.Node.creation_context(ast.Node.SrcInfo(file or "")):
        if file:
            with open(file) as f:
                gen = tokenize.generate_tokens(f)
                return helper(gen)
        else:
            assert code
            gen = tokenize.generate_tokens([l + "\n" for l in code.split("\n")])
            return helper(gen)
