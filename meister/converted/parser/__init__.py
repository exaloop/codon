from .. import ast
from . import parser, pegen, tokenize
from .tokenize import main as tokenize_main  # noqa: F401


def parse(file: str | None, code: str | None = None, verbose=False) -> ast.SuiteStmt:
    assert (file is not None) ^ (code is not None), "bad arguments"

    if file:
        f = open(file)
        gen = tokenize.generate_tokens(f)
    else:
        gen = tokenize.generate_tokens([l + "\n" for l in code.split("\n")])

    tokenizer = pegen.Tokenizer(gen, verbose=verbose)
    engine = parser.CodonParser(tokenizer, verbose=verbose)
    tree = engine.start()

    if file:
        f.close()
    if not tree:
        err = engine.make_syntax_error("fn")
        raise err
    return tree
