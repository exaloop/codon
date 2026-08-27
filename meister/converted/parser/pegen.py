import os
from dataclasses import dataclass

from ...bridge import (
    CODON,
    Any,
    Codon,
    Dict,
    Generator,
    List,
    Literal,
    Optional,
    Tuple,
    cast,
    inline,
)
from ..ast import nodes as ast
from . import tokenize

Mark = int

# Static flag for verbosity to avoid unnecessary branching in hotspots.
ENABLE_VERBOSE: Literal[bool] = False

EXPR_NAME_MAPPING = {
    "DotExpr": "attribute",
    "IndexExpr": "subscript",
    "StarExpr": "starred",
    "IdExpr": "name",
    "ListExpr": "list",
    "TupleExpr": "tuple",
    "LambdaExpr": "lambda",
    "CallExpr": "function call",
    "BinaryExpr": "expression",
    "ChainBinaryExpr": "comparison",
    "UnaryExpr": "expression",
    "GeneratorExpr": "generator expression",
    "YieldExpr": "yield expression",
    "AwaitExpr": "await expression",
    "DictExpr": "dict literal",
    "SetExpr": "set display",
    "IfExpr": "conditional expression",
    "AssignExpr": "named expression",
}


@dataclass
class Comprehension:
    """Parser-only carrier used until a GeneratorExpr loop nest is built."""

    target: object
    iter: object
    ifs: List[object]
    is_async: bool = False


def unescape(string):
    # C++ implementation: codon/parser/common.cpp:182
    result = ""
    i = 0
    while i < len(string):
        if string[i] == "\\" and i + 1 < len(string):
            if string[i + 1] == "a":
                result += "\a"
                i += 1
            elif string[i + 1] == "b":
                result += "\b"
                i += 1
            elif string[i + 1] == "f":
                result += "\f"
                i += 1
            elif string[i + 1] == "n":
                result += "\n"
                i += 1
            elif string[i + 1] == "r":
                result += "\r"
                i += 1
            elif string[i + 1] == "t":
                result += "\t"
                i += 1
            elif string[i + 1] == "v":
                result += "\v"
                i += 1
            elif string[i + 1] == '"':
                result += '"'
                i += 1
            elif string[i + 1] == "'":
                result += "'"
                i += 1
            elif string[i + 1] == "\\":
                result += "\\"
                i += 1
            elif string[i + 1] == "x":
                if i + 3 > len(string):
                    raise ValueError("invalid \\x code")
                digits = string[i + 2 : i + 4]
                pos = 0
                while pos < len(digits) and digits[pos] in "0123456789abcdefABCDEF":
                    pos += 1
                if pos == 0:
                    raise ValueError("invalid \\x code")
                code = int(digits[:pos], 16)
                result += chr(code & 0xFF)
                i += pos + 1
            elif "0" <= string[i + 1] <= "7":
                digits = string[i + 1 : i + 4]
                pos = 0
                while pos < len(digits) and "0" <= digits[pos] <= "7":
                    pos += 1
                code = int(digits[:pos], 8)
                result += chr(code & 0xFF)
                i += pos
            else:
                result += string[i]
        else:
            result += string[i]
        i += 1
    return result


def shorttok(tok: tokenize.TokenInfo) -> str:
    formatted = (
        f"{tok.start[0]}.{tok.start[1]}: {tokenize.Tokens.get_name(tok.type)}:{tok.string!r}"
    )
    return f"{formatted:<25.25}"


class Tokenizer:
    """Caching wrapper for the tokenize module.

    This is pretty tied to Python's syntax.
    """

    _tokens: List[tokenize.TokenInfo]
    _tokengen: Generator[tokenize.TokenInfo]
    _index: int
    _verbose: bool
    _lines: Dict[int, str]
    _path: str

    def __init__(
        self,
        tokengen: Generator[tokenize.TokenInfo],
        path: str = "",
        verbose: bool = False,
    ):
        self._tokengen = tokengen
        self._tokens = []
        self._index = 0
        self._verbose = verbose
        self._lines = {}
        self._path = path
        if ENABLE_VERBOSE and verbose:
            self.report(False, False)

    def getnext(self) -> tokenize.TokenInfo:
        """Return the next token and updates the index."""
        if ENABLE_VERBOSE and self._verbose:
            cached = not self._index == len(self._tokens)
            tok = self.peek()
            self._index += 1
            self.report(cached, False)
        else:
            tok = self.peek()
            self._index += 1
        return tok

    def advance(self, tok) -> tokenize.TokenInfo:
        """Updates the index with a provided token (always peeked in advance)."""
        self._index += 1
        return tok

    def peek(self) -> tokenize.TokenInfo:
        """Return the next token *without* updating the index."""
        while self._index == len(self._tokens):
            tok = next(self._tokengen)
            if tok.type in (tokenize.Tokens.NL, tokenize.Tokens.COMMENT):
                # TODO(Codon AST): preserve `## codon: key=value` comments as
                # DirectiveStmt nodes. The copied tokenizer currently drops all
                # comments before grammar rules can distinguish directives.
                continue
            if tok.type == tokenize.Tokens.ERRORTOKEN and tok.string.isspace():
                continue
            if (
                tok.type == tokenize.Tokens.NEWLINE
                and self._tokens
                and self._tokens[-1].type == tokenize.Tokens.NEWLINE
            ):
                continue
            self._tokens.append(tok)
            if not self._path and tok.start[0] not in self._lines:
                self._lines[tok.start[0]] = tok.line
        return self._tokens[self._index]

    def diagnose(self) -> tokenize.TokenInfo:
        if not self._tokens:
            self.getnext()
        return self._tokens[-1]

    def get_last_non_whitespace_token(self) -> Optional[tokenize.TokenInfo]:
        for toki in range(self._index - 1, -1, -1):
            tok = self._tokens[toki]
            if tok.type != tokenize.Tokens.ENDMARKER and (
                tok.type < tokenize.Tokens.NEWLINE or tok.type > tokenize.Tokens.DEDENT
            ):
                return tok
        return self._tokens[-1] if self._tokens else None

    def get_lines(self, line_numbers: List[int]) -> List[str]:
        """Retrieve source lines corresponding to line numbers."""
        if self._lines:
            lines = self._lines
        else:
            n = len(line_numbers)
            lines = {}
            count = 0
            seen = 0
            with open(self._path) as f:
                for line in f:
                    count += 1
                    if count in line_numbers:
                        seen += 1
                        lines[count] = line
                        if seen == n:
                            break

        return [lines[n] for n in line_numbers]

    @inline
    def mark(self) -> Mark:
        return self._index

    @inline
    def reset(self, index: Mark):
        if ENABLE_VERBOSE and self._verbose:
            assert 0 <= index <= len(self._tokens), (index, len(self._tokens))
            old_index = self._index
            self._index = index
            self.report(True, index < old_index)
        else:
            self._index = index

    def report(self, cached: bool, back: bool):
        if back:
            fill = "-" * self._index + "-"
        elif cached:
            fill = "-" * self._index + ">"
        else:
            fill = "-" * self._index + "*"
        if self._index == 0:
            print(f"{fill} (Bof)")
        else:
            tok = self._tokens[self._index - 1]
            print(f"{fill} {shorttok(tok)} {tok.start}:{tok.line.strip()}")


def logger(method):
    """For non-memoized functions that we want to be logged.

    (In practice this is only non-leader left-recursive functions.)
    """

    def logger_wrapper(self, *args):
        if not ENABLE_VERBOSE and not self._verbose:
            return method(self, *args)

        method_name = method.__name__
        argsr = ",".join(repr(arg) for arg in args)
        fill = "  " * self._level
        print(f"{fill}{method_name}({argsr}) .... (looking at {self.showpeek()})")
        self._level += 1
        tree = method(self, *args)
        self._level -= 1
        print(f"{fill}... {method_name}({argsr}) --> {tree!s:.200}")
        return tree

    return logger_wrapper


def memoize(method):
    """Memoize a symbol method."""

    def memoize_wrapper(self, *args):
        verbose = self._verbose
        mark = self._mark()
        if CODON:
            fn = int(method.__raw__())
        else:
            fn = id(method)
        key = mark, fn
        R = Codon.return_type(method, self, *args)
        # Fast path: cache hit, and not verbose.
        hit = self._cache.get(key)
        if hit:
            if not ENABLE_VERBOSE and not verbose:
                tree, endmark = hit
                self._reset(endmark)
                return Codon.unwrap(tree, R)
        # Slow path: no cache hit, or verbose.
        method_name, argsr, fill = "", "", ""
        if ENABLE_VERBOSE and verbose:
            method_name = method.__name__
            argsr = ",".join(repr(arg) for arg in args)
            fill = "  " * self._level
        if not hit:
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}{method_name}({argsr}) ... (looking at {self.showpeek()})")
            self._level += 1
            tree = method(self, *args)
            self._level -= 1
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}... {method_name}({argsr}) -> {tree!s:.200}")
            endmark = self._mark()
            self._cache[key] = cast(Any, tree), endmark
            return tree
        else:
            tree, endmark = hit
            tree = Codon.unwrap(tree, R)
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}{method_name}({argsr}) -> {tree!s:.200}")
            self._reset(endmark)
            return tree

    return memoize_wrapper


def memoize_left_rec(method):
    """Memoize a left-recursive symbol method."""

    def memoize_left_rec_wrapper(self):
        verbose = self._verbose
        mark = self._mark()
        if CODON:
            fn = int(method.__raw__())
        else:
            fn = id(method)
        key = mark, fn
        R = Codon.return_type(method, self)
        # Fast path: cache hit, and not verbose.
        hit = self._cache.get(key)
        if hit:
            if not ENABLE_VERBOSE and not verbose:
                tree, endmark = hit
                self._reset(endmark)
                return Codon.unwrap(tree, R)
        # Slow path: no cache hit, or verbose.

        method_name, fill = "", ""
        if ENABLE_VERBOSE and verbose:
            method_name = method.__name__
            fill = "  " * self._level if verbose else ""
        if not hit:
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}{method_name} ... (looking at {self.showpeek()})")
            self._level += 1

            # For left-recursive rules we manipulate the cache and
            # loop until the rule shows no progress, then pick the
            # previous result.  For an explanation why this works, see
            # https://github.com/PhilippeSigaud/Pegged/wiki/Left-Recursion
            # (But we use the memoization cache instead of a static
            # variable; perhaps this is similar to a paper by Warth et al.
            # (http://web.cs.ucla.edu/~todd/research/pub.php?id=pepm08).

            # Prime the cache with a failure.
            result: R = None
            self._cache[key] = cast(Any, result), mark
            lastresult: R = None
            lastmark = mark
            depth = 0
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}Recursive {method_name} at {mark} depth {depth}")

            while True:
                self._reset(mark)
                self.in_recursive_rule += 1
                try:
                    result = method(self)
                finally:
                    self.in_recursive_rule -= 1
                endmark = self._mark()
                depth += 1
                if ENABLE_VERBOSE and verbose:
                    print(
                        f"{fill}Recursive {method_name} at {mark} depth {depth}: "
                        f"{result!s:.200} to {endmark}"
                    )
                if not result:
                    if ENABLE_VERBOSE and verbose:
                        print(f"{fill}Fail with {lastresult!s:.200} to {lastmark}")
                    break
                if endmark <= lastmark:
                    if ENABLE_VERBOSE and verbose:
                        print(f"{fill}Bailing with {lastresult!s:.200} to {lastmark}")
                    break
                self._cache[key] = cast(Any, result), endmark
                lastresult, lastmark = result, endmark

            self._reset(lastmark)
            tree = lastresult

            self._level -= 1
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}{method_name}() -> {tree!s:.200} [cached]")
            if tree:
                endmark = self._mark()
            else:
                endmark = mark
                self._reset(endmark)
            self._cache[key] = cast(Any, tree), endmark
            return tree
        else:
            tree, endmark = hit
            tree = Codon.unwrap(tree, R)
            if ENABLE_VERBOSE and verbose:
                print(f"{fill}{method_name}() -> {tree!s:.200} [fresh]")
            if tree:
                self._reset(endmark)
            return tree

    return memoize_left_rec_wrapper


class BaseParser:
    """Parsing base class."""

    _tokenizer: Tokenizer
    _verbose: bool
    _level: int
    _cache: Dict[Tuple[Mark, int], Tuple[Any, Mark]]
    in_recursive_rule: int
    call_invalid_rules: bool
    KEYWORDS: List[str]
    SOFT_KEYWORDS: List[str]

    def __init__(self, tokenizer: Tokenizer, verbose: bool = False):
        self._tokenizer = tokenizer
        self._verbose = verbose
        self._level = 0
        self._cache = {}
        # Integer tracking whether we are in a left recursive rule or not. Can be useful
        # for error reporting.
        self.in_recursive_rule = 0
        # Are we looking for syntax error ? When true enable matching on invalid rules
        self.call_invalid_rules = False

        self.KEYWORDS = []
        self.SOFT_KEYWORDS = []

    @inline
    def _mark(self):
        return self._tokenizer.mark()

    @inline
    def _reset(self, index: Mark):
        return self._tokenizer.reset(index)

    def start(self):
        """Expected grammar entry point.

        This is not strictly necessary but is assumed to exist in most utility
        functions consuming parser instances.

        """
        raise NotImplementedError()

    def showpeek(self) -> str:
        tok = self._tokenizer.peek()
        return f"{tok.start[0]}.{tok.start[1]}: {tokenize.Tokens.get_name(tok.type)}:{tok.string!r}"

    def any_but_newline(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type not in [
            tokenize.Tokens.NEWLINE,
            tokenize.Tokens.NL,
            tokenize.Tokens.INDENT,
            tokenize.Tokens.DEDENT,
            tokenize.Tokens.ENDMARKER,
        ]:
            return self._tokenizer.advance(tok)
        return None

    def name(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.NAME and tok.string not in self.KEYWORDS:
            return self._tokenizer.advance(tok)
        return None

    def number(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.NUMBER:
            return self._tokenizer.advance(tok)
        return None

    def number_suffix(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.NUMBER_SUFFIX:
            return self._tokenizer.advance(tok)
        return None

    def string(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.STRING:
            return self._tokenizer.advance(tok)
        return None

    def fstring_start(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.FSTRING_START:
            return self._tokenizer.advance(tok)
        return None

    def fstring_middle(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.FSTRING_MIDDLE:
            return self._tokenizer.advance(tok)
        return None

    def fstring_end(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.FSTRING_END:
            return self._tokenizer.advance(tok)
        return None

    def string_prefix(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.STRING_PREFIX:
            return self._tokenizer.advance(tok)
        return None

    def op(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.OP:
            return self._tokenizer.advance(tok)
        return None

    def type_comment(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.TYPE_COMMENT:
            return self._tokenizer.advance(tok)
        return None

    def soft_keyword(self) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == tokenize.Tokens.NAME and tok.string in self.SOFT_KEYWORDS:
            return self._tokenizer.advance(tok)
        return None

    def expect_literal(self, literal: str) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.string == literal:
            return self._tokenizer.advance(tok)
        return None

    def expect_type(self, typ: int) -> Optional[tokenize.TokenInfo]:
        tok = self._tokenizer.peek()
        if tok.type == typ:
            return self._tokenizer.advance(tok)
        return None

    def expect_forced(self, res, expectation: str) -> Optional[tokenize.TokenInfo]:
        if res is None:
            raise self.make_syntax_error(f"expected {expectation}")
        return res

    def positive_lookahead(self, func, *args):
        mark = self._mark()
        ok = func(*args)
        self._reset(mark)
        return ok

    def negative_lookahead(self, func, *args) -> bool:
        mark = self._mark()
        ok = func(*args)
        self._reset(mark)
        return not ok

    def make_syntax_error(self, message: str, filename: str = "<unknown>"):
        tok = self._tokenizer.diagnose()
        return CodonSyntaxError(message, (filename, tok.start[0], 1 + tok.start[1], tok.line))


def simple_parser_main(parser_class, argv):
    import sys
    import time

    from ...bridge import argparse

    argparser = argparse.ArgumentParser()
    argparser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Print timing stats; repeat for more debug output",
    )
    argparser.add_argument(
        "-q", "--quiet", action="store_true", help="Don't print the parsed program"
    )
    argparser.add_argument("-i", "--indent", type="int", help="Indentation level")
    argparser.add_argument("-r", "--run", action="store_true", help="Run the parsed program")
    argparser.add_argument("filename", help="Input file ('-' to use stdin)")

    args = argparser.parse_args(args=argv)
    verbose = int(args.verbose)
    verbose_tokenizer = verbose >= 3
    verbose_parser = verbose == 2 or verbose >= 4

    t0 = time.time()

    filename = str(args.filename)
    if filename == "" or filename == "-":
        filename = "<stdin>"
        file = sys.stdin
    else:
        file = open(filename)
    try:
        tokengen = tokenize.generate_tokens(file)
        tokenizer = Tokenizer(tokengen, verbose=verbose_tokenizer)
        parser = parser_class(tokenizer, verbose=verbose_parser)
        tree = parser.start()
        try:
            endpos = file.tell()
        except OSError:
            endpos = 0
    finally:
        if file is not sys.stdin:
            file.close()

    t1 = time.time()

    if not tree:
        err = parser.make_syntax_error(filename)
        print(err)
        sys.exit(1)

    if not bool(args.quiet):
        print(ast.dump(Codon.unwrap(tree), indent=int(args.indent)))
    # if bool(args.run):
    # exec(compile(tree, filename=filename, mode="exec"))

    if verbose:
        dt = t1 - t0
        diag = tokenizer.diagnose()
        nlines = diag.end[0]
        if diag.type == tokenize.Tokens.ENDMARKER:
            nlines -= 1
        print(f"Total time: {dt:.3f} sec; {nlines} lines", end="")
        if endpos:
            print(f" ({endpos} bytes)", end="")
        if dt:
            print(f"; {nlines / dt:.0f} lines/sec")
        else:
            print()
        print("Caches sizes:")
        print(f"  token array : {len(tokenizer._tokens):10}")
        print(f"        cache : {len(parser._cache):10}")
        ## print_memstats()


class CodonIndentationError(Exception):
    def __init__(self, message: str = ""):
        super().__init__(message)


class CodonSyntaxError(SyntaxError):
    location: Tuple[str, int, int, str, int, int]

    def __init__(self, message: str = "", location=("", 0, 0, "", 0, 0)):
        super().__init__(message)
        self.location = location


def parse_file(
    path: str,
    token_stream_factory=None,
    verbose: bool = False,
) -> ast.SuiteStmt:
    """Parse a file."""
    with open(path) as f:
        tok_stream = (
            token_stream_factory(f) if token_stream_factory else tokenize.generate_tokens(f)
        )
        tokenizer = Tokenizer(tok_stream, verbose=verbose, path=path)
        parser = Parser(
            tokenizer,
            verbose=verbose,
            filename=os.path.basename(path),
        )
        return parser.parse("file")


def parse_string(
    source: str,
    mode: str,
    token_stream_factory=None,
    verbose: bool = False,
):
    """Parse a string."""

    from ...bridge import io

    tok_stream = (
        token_stream_factory(io.StringIO(source))
        if token_stream_factory
        else tokenize.generate_tokens(io.StringIO(source))
    )
    tokenizer = Tokenizer(tok_stream, verbose=verbose)
    parser = Parser(tokenizer, verbose=verbose)
    return parser.parse(mode if mode == "eval" else "file")


class Target:
    FOR_TARGETS = 1
    STAR_TARGETS = 2
    DEL_TARGETS = 3


class Parser(BaseParser):
    #: Name of the source file, used in error reports
    filename: str

    def __init__(
        self,
        tokenizer: Tokenizer,
        verbose: bool = False,
        filename: str = "<unknown>",
    ):
        super().__init__(tokenizer, verbose=verbose)
        self.filename = filename

    def parse(self, rule: Literal[str], call_invalid_rules: bool = False) -> Optional[ast.Node]:
        self.call_invalid_rules = call_invalid_rules
        res = getattr(self, rule)()

        if res is None:
            # Grab the last token that was parsed in the first run to avoid
            # polluting a generic error reports with progress made by invalid rules.
            last_token = self._tokenizer.diagnose()

            if not call_invalid_rules:
                self.call_invalid_rules = True

                # Reset the parser cache to be able to restart parsing from the
                # beginning.
                self._reset(0)  # type: ignore
                self._cache.clear()

                res = getattr(self, rule)()

            self.raise_raw_syntax_error("invalid syntax", last_token.start, last_token.end)

        return res

    def raise_indentation_error(self, msg: str):
        """Raise an indentation error."""
        last_token = self._tokenizer.diagnose()
        args = (
            self.filename,
            last_token.start[0],
            last_token.start[1] + 1,
            last_token.line,
            last_token.end[0],
            last_token.end[1] + 1,
        )
        raise CodonIndentationError(msg, args)

    def get_expr_name(self, node) -> str:
        """Get a descriptive name for an expression."""
        # See https://github.com/python/cpython/blob/master/Parser/pegen.c#L161
        assert node is not None
        node_t = type(node)
        if node_t is ast.EllipsisExpr:
            return "ellipsis"
        elif node_t is ast.NoneExpr:
            return str(None)
        elif node_t is ast.BoolExpr:
            return str(node.value)
        elif node_t in (ast.IntExpr, ast.FloatExpr, ast.StringExpr):
            return "literal"

        try:
            return EXPR_NAME_MAPPING[node_t.__name__]
        except KeyError:
            raise ValueError(
                f"unexpected expression in assignment {type(node).__name__} "
                f"(line {node.info.line})."
            )

    def get_invalid_target(self, target: int, node: Optional[ast.Expr]) -> Optional[ast.Expr]:
        """Get the meaningful invalid target for different assignment type."""
        if node is None:
            return None

        # We only need to visit List and Tuple nodes recursively as those
        # are the only ones that can contain valid names in targets when
        # they are parsed as expressions. Any other kind of expression
        # that is a container (like Sets or Dicts) is directly invalid and
        # we do not need to visit it recursively.
        if isinstance(node, (ast.ListExpr, ast.TupleExpr)):
            for e in node.items:
                if (inv := self.get_invalid_target(target, e)) is not None:
                    return inv
        elif isinstance(node, ast.StarExpr):
            if target == Target.DEL_TARGETS:
                return node
            return self.get_invalid_target(target, node.expr)
        elif isinstance(node, (ast.BinaryExpr, ast.ChainBinaryExpr)):
            # This is needed, because the `a in b` in `for a in b` gets parsed
            # as a comparison, and so we need to search the left side of the comparison
            # for invalid targets.
            if target == Target.FOR_TARGETS:
                if isinstance(node, ast.BinaryExpr) and node.op == "in":
                    return self.get_invalid_target(target, node.lexpr)
                return None
            return node
        elif isinstance(node, (ast.IdExpr, ast.IndexExpr, ast.DotExpr)):
            return None
        else:
            return node

    def set_expr_context(self, node, context):
        """Codon targets use ordinary Expr nodes and have no load/store context."""
        return node

    def suite(self, statements=None, **locations):
        if isinstance(statements, ast.SuiteStmt):
            return statements
        suite = ast.SuiteStmt(
            items=[] if statements is None else statements,
            **locations,
        )
        suite.flatten()
        return suite

    def call_arg(self, name, value, **locations):
        return ast.CallExpr.Arg(name=name, value=value, **locations)

    def call(self, expr, positional=None, keywords=None, partial=False, **locations):
        items = [self.call_arg("", value, **locations) for value in (positional or [])]
        items.extend(keywords or [])
        if partial:
            items.append(
                self.call_arg(
                    "",
                    ast.EllipsisExpr(
                        mode=ast.EllipsisExpr.Kind.Partial,
                        **locations,
                    ),
                    **locations,
                )
            )
        return ast.CallExpr(
            expr=expr,
            items=items,
            partial=partial,
            **locations,
        )

    def binary_chain(self, first, pairs, **locations):
        expr = first
        for op, rhs in pairs:
            expr = ast.BinaryExpr(lexpr=expr, op=op, rexpr=rhs, **locations)
        return expr

    def make_number(self, value, suffix="", **locations):
        if any(marker in value for marker in (".", "e", "E")):
            return ast.FloatExpr(value=value, suffix=suffix, **locations)
        return ast.IntExpr(value=value, suffix=suffix, **locations)

    def dict_expr(self, pairs, **locations):
        items = []
        for key, value in pairs:
            if key is None:
                items.append(ast.KeywordStarExpr(expr=value, **locations))
            else:
                items.append(ast.TupleExpr(items=[key, value], **locations))
        return ast.DictExpr(items=items, **locations)

    def make_comparison(self, first, pairs, **locations):
        if len(pairs) == 1:
            op, rhs = pairs[0]
            return ast.BinaryExpr(lexpr=first, op=op, rexpr=rhs, **locations)
        return ast.ChainBinaryExpr(exprs=[("", first)] + pairs, **locations)

    def make_pipe(self, first, rest, **locations):
        return ast.PipeExpr(
            items=[ast.PipeExpr.Pipe("", first, **locations)]
            + [ast.PipeExpr.Pipe(op, expr, **locations) for op, expr in rest],
            **locations,
        )

    def assignments(self, targets, value, **locations):
        statements = []
        values = [t.clone() for t in targets] + [value]
        for index in range(len(targets) - 1, -1, -1):
            statements.append(
                ast.AssignStmt(
                    lhs=targets[index],
                    rhs=values[index + 1],
                    **locations,
                )
            )
        return self.suite(statements, **locations)

    def globals(self, names, non_local=False, **locations):
        return self.suite(
            [
                ast.GlobalStmt(
                    var=name.string,
                    non_local=non_local,
                    **locations,
                )
                for name in names
            ],
            **locations,
        )

    def deletes(self, targets, **locations):
        return self.suite(
            [ast.DelStmt(expr=target, **locations) for target in targets],
            **locations,
        )

    def dotted_expr(self, value, **locations):
        parts = value.split(".")
        expr = ast.IdExpr(value=parts[0], **locations)
        for part in parts[1:]:
            expr = ast.DotExpr(expr=expr, member=part, **locations)
        return expr

    def imports(self, aliases, **locations):
        return self.suite(
            [
                ast.ImportStmt(
                    from_expr=self.dotted_expr(name, **locations),
                    as_=as_name or "",
                    **locations,
                )
                for name, as_name, _, _, _ in aliases
            ],
            **locations,
        )

    def imports_from(self, module, aliases, dots, **locations):
        return self.suite(
            [
                ast.ImportStmt(
                    from_expr=self.dotted_expr(module, **locations) if module else None,
                    what=self.dotted_expr(name, **locations),
                    args=params,
                    ret=ret,
                    as_=as_name or "",
                    dots=dots,
                    is_function=is_function,
                    **locations,
                )
                for name, as_name, params, ret, is_function in aliases
            ],
            **locations,
        )

    def make_class(self, name, generics, arguments, suite, **locations):
        positional, keywords = arguments or ([], [])
        bases = list(positional)
        for arg in keywords:
            assert not arg.name, "class got invalid base"
            bases.append(arg.value)
        params = []
        body = []
        for statement in suite.items:
            if isinstance(statement, ast.AssignStmt) and isinstance(statement.lhs, ast.IdExpr):
                params.append(
                    ast.Param(
                        name=statement.lhs.value,
                        type=statement.type_expr,
                        default=statement.rhs,
                        **locations,
                    )
                )
            else:
                body.append(statement)
        params.extend(generics)
        return ast.ClassStmt(
            name=name,
            items=params,
            suite=self.suite(body, **locations),
            base_classes=bases,
            **locations,
        )

    def make_with_stmt(self, items, suite, async_=False, **locations):
        # C++ WithStmt stores simple IdExpr targets as names and uses an empty
        # string for every other target (codon/parser/ast/stmt.cpp:791).
        values = [item[0] for item in items]
        names = [item[1].value if isinstance(item[1], ast.IdExpr) else "" for item in items]
        return ast.WithStmt(
            items=values,
            vars=names,
            suite=self.suite(suite, **locations),
            async_=async_,
            **locations,
        )

    def generator(self, kind, expr, comprehensions, key=None, **locations):
        tail = ast.ExprStmt(
            expr=(ast.TupleExpr(items=[key, expr], **locations) if key is not None else expr),
            **locations,
        )
        current = tail
        for comp in reversed(comprehensions):
            nested = current
            for condition in reversed(comp.ifs):
                nested = ast.IfStmt(
                    cond=condition,
                    if_suite=nested,
                    **locations,
                )
            current = ast.ForStmt(
                var=comp.target,
                iter=comp.iter,
                suite=nested,
                async_=comp.is_async,
                **locations,
            )
        return ast.GeneratorExpr(kind=kind, loops=current, **locations)

    def ensure_real(self, number):
        # TODO
        return number

    def ensure_imaginary(self, number):
        # TODO
        return number

    def _concat_strings_in_constant(self, parts):
        return "".join(part.value for part in parts)

    def concatenate_strings(self, parts, **locations):
        """Concatenate adjacent Codon StringExpr nodes."""
        strings = []
        for part in parts:
            strings.extend(part.strings)
        return ast.StringExpr(strings=strings, **locations)

    def check_fstring_conversion(self, mark: tokenize.TokenInfo, name: tokenize.TokenInfo) -> str:
        if mark.end != name.start:
            self.raise_syntax_error_known_range(
                "f-string: conversion type must come right after the exclamation mark", mark, name
            )
        s = name.string
        if len(s) > 1 or s not in ("s", "r", "a"):
            self.raise_syntax_error_known_location(
                f"f-string: invalid conversion character '{s}': expected 's', 'r', or 'a'",
                name,
            )
        return name.string

    def fix_string(self, string, prefix="", **kwargs):
        source = string.string
        value = source
        if len(value) >= 6 and value[:3] == value[-3:]:
            value = value[3:-3]
        elif len(value) >= 2 and value[0] == value[-1]:
            value = value[1:-1]
        raw = prefix.lower() == "r"
        if raw:
            prefix = ""
        else:
            value = unescape(value)
        return ast.StringExpr(value=value, prefix=prefix, **kwargs)

    def generate_ast_for_string(self, tokens, **locations) -> Optional[ast.Expr]:
        """Generate AST nodes for strings."""

        if len(tokens) == 1:
            return tokens[0]
        else:
            return self.concatenate_strings(tokens, **locations)

    def extract_import_level(self, tokens: List[tokenize.TokenInfo]) -> int:
        """Extract the relative import level from the tokens preceding the module name.

        '.' count for one and '...' for 3.

        """
        level = 0
        for t in tokens:
            if t.string == ".":
                level += 1
            else:
                level += 3
        return level

    def set_decorators(self, target, decorators):
        """Set the decorators on a function or class definition."""
        if isinstance(target, ast.ForStmt):
            # The Codon ForStmt has one decorator slot.
            target.decorator = decorators[0] if decorators else None
        else:
            target.decorators = decorators
        target.validate()
        return target

    def get_comparison_ops(self, pairs):
        return [op for op, _ in pairs]

    def get_comparators(self, pairs):
        return [comp for _, comp in pairs]

    def make_arguments(
        self,
        pos_only=None,  #: Optional[List[Tuple[ast.Param, None]]],
        pos_only_with_default=None,  #: List[Tuple[ast.Param, Any]],
        param_no_default=None,  #: Optional[List[Tuple[ast.Param, None]]],
        param_default=None,  #: Optional[List[Tuple[ast.Param, Any]]],
        #: Optional[Tuple[Optional[ast.Param], List[Tuple[ast.Param, Any]], Optional[ast.Param]]]
        after_star=None,
        **locations,
    ) -> List[ast.Param]:
        """Build a function definition arguments."""
        params = []
        if pos_only is not None:
            params.extend(p for p, _ in pos_only)
        elif pos_only_with_default is not None:
            for param, default in pos_only_with_default:
                param.default = default
                params.append(param)
        if param_no_default is not None:
            params.extend(param_no_default)
        if param_default is not None:
            for param, default in param_default:
                param.default = default
                params.append(param)
        if after_star is not None:
            vararg, keyword_only, kwarg, generics = after_star
            if vararg is not None:
                vararg.name = "*" + vararg.name
                params.append(vararg)
            elif keyword_only:
                params.append(ast.Param(name="*", **locations))
            for param, default in keyword_only:
                param.default = default
                params.append(param)
            if kwarg is not None:
                kwarg.name = "**" + kwarg.name
                params.append(kwarg)
            for param, default in generics:
                param.default = default
                param.status = ast.Param.Status.Generic
                if param.type is None:
                    param.type = ast.IdExpr(value="type", **locations)
                params.append(param)
        return params

    def _build_syntax_error(
        self,
        message: str,
        start: Optional[Tuple[int, int]] = None,
        end: Optional[Tuple[int, int]] = None,
    ) -> CodonSyntaxError:
        line_from_token = start is None and end is None
        if start is None or end is None:
            tok = self._tokenizer.diagnose()
            start = start or tok.start
            end = end or tok.end

        if line_from_token:
            line = tok.line
        else:
            # End is used only to get the proper text
            line = "\\n".join(self._tokenizer.get_lines(list(range(start[0], end[0] + 1))))

        # tokenize.py index column offset from 0 while Cpython index column
        # offset at 1 when reporting SyntaxError, so we need to increment
        # the column offset when reporting the error.
        args = (self.filename, start[0], start[1] + 1, line, end[0], end[1] + 1)
        return CodonSyntaxError(message, args)

    def raise_raw_syntax_error(
        self,
        message: str,
        start: Optional[Tuple[int, int]] = None,
        end: Optional[Tuple[int, int]] = None,
    ):
        raise self._build_syntax_error(message, start, end)

    def make_syntax_error(self, message: str, filename: str = "<unknown>") -> CodonSyntaxError:
        return self._build_syntax_error(message)

    def expect_forced(self, res, expectation: str) -> Optional[tokenize.TokenInfo]:
        if res is None:
            last_token = self._tokenizer.diagnose()
            end = last_token.start
            end = last_token.end
            self.raise_raw_syntax_error(f"expected {expectation}", last_token.start, end)
        return res

    def raise_syntax_error(self, message: str):
        """Raise a syntax error."""
        tok = self._tokenizer.diagnose()
        raise self._build_syntax_error(message, tok.start, tok.end)

    def raise_syntax_error_known_location(self, message: str, node):
        """Raise a syntax error that occured at a given AST node."""
        if isinstance(Codon.unwrap(node), tokenize.TokenInfo):
            start = node.start
            end = node.end
        else:
            start = node.lineno, node.col_offset
            end = node.end_lineno, node.end_col_offset

        raise self._build_syntax_error(message, start, end)

    def raise_syntax_error_known_range(
        self,
        message: str,
        start_node,
        end_node,
    ):
        if isinstance(Codon.unwrap(start_node), tokenize.TokenInfo):
            start = start_node.start
        else:
            start = start_node.lineno, start_node.col_offset

        if isinstance(Codon.unwrap(end_node), tokenize.TokenInfo):
            end = end_node.end
        else:
            end = end_node.end_lineno, end_node.end_col_offset

        raise self._build_syntax_error(message, start, end)

    def raise_syntax_error_starting_from(self, message: str, start_node):
        if isinstance(Codon.unwrap(start_node), tokenize.TokenInfo):
            start = start_node.start
        else:
            start = start_node.lineno, start_node.col_offset

        last_token = self._tokenizer.diagnose()

        raise self._build_syntax_error(message, start, last_token.start)

    def raise_syntax_error_invalid_target(self, target: int, node: Optional[ast.Expr]):
        invalid_target = self.get_invalid_target(target, node)

        if invalid_target is None:
            return

        if target in (Target.STAR_TARGETS, Target.FOR_TARGETS):
            msg = f"cannot assign to {self.get_expr_name(invalid_target)}"
        else:
            msg = f"cannot delete {self.get_expr_name(invalid_target)}"

        self.raise_syntax_error_known_location(msg, invalid_target)

    def raise_syntax_error_on_next_token(self, message: str):
        next_token = self._tokenizer.peek()
        raise self._build_syntax_error(message, next_token.start, next_token.end)
