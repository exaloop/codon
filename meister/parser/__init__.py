from .. import ast
from . import parser, pegen, tokenize


def parse(
    file: str | None = None,
    code: str | None = None,
    verbose=False,
    rule: str = "start",
    line_offset=0,
) -> ast.Node:
    assert file is not None or code is not None, "bad arguments"

    def offset_tokens(tokens):
        for token in tokens:
            token.start = (token.start[0] + line_offset, token.start[1])
            token.end = (token.end[0] + line_offset, token.end[1])
            yield token

    def helper(gen):
        if line_offset:
            gen = offset_tokens(gen)
        tokenizer = pegen.Tokenizer(gen, verbose=verbose)
        engine = parser.CodonParser(tokenizer, verbose=verbose)
        tree = engine.parse(rule)
        assert tree
        return tree

    with ast.Node.creation_context(ast.Node.SrcInfo(file or "")):
        if code is None:
            with open(file) as f:
                gen = tokenize.generate_tokens(f)
                return helper(gen)
        else:
            assert code
            gen = tokenize.generate_tokens([l + "\n" for l in code.split("\n")])
            return helper(gen)
