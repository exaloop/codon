### Codon tokenizer; f-string modes follow CPython 3.12 Parser/tokenizer.c.

from ..bridge import ClassVar, Dict, Generator, List, Optional, Set, Tuple, static


class Tokens:
    ENDMARKER = 0
    NAME = 1
    NUMBER = 2
    STRING = 3
    NEWLINE = 4
    INDENT = 5
    DEDENT = 6
    LPAR = 7
    RPAR = 8
    LSQB = 9
    RSQB = 10
    COLON = 11
    COMMA = 12
    SEMI = 13
    PLUS = 14
    MINUS = 15
    STAR = 16
    SLASH = 17
    VBAR = 18
    AMPER = 19
    LESS = 20
    GREATER = 21
    EQUAL = 22
    DOT = 23
    PERCENT = 24
    LBRACE = 25
    RBRACE = 26
    EQEQUAL = 27
    NOTEQUAL = 28
    LESSEQUAL = 29
    GREATEREQUAL = 30
    TILDE = 31
    CIRCUMFLEX = 32
    LEFTSHIFT = 33
    RIGHTSHIFT = 34
    DOUBLESTAR = 35
    PLUSEQUAL = 36
    MINEQUAL = 37
    STAREQUAL = 38
    SLASHEQUAL = 39
    PERCENTEQUAL = 40
    AMPEREQUAL = 41
    VBAREQUAL = 42
    CIRCUMFLEXEQUAL = 43
    LEFTSHIFTEQUAL = 44
    RIGHTSHIFTEQUAL = 45
    DOUBLESTAREQUAL = 46
    DOUBLESLASH = 47
    DOUBLESLASHEQUAL = 48
    AT = 49
    ATEQUAL = 50
    RARROW = 51
    ELLIPSIS = 52
    COLONEQUAL = 53
    EXCLAMATION = 54
    OP = 55
    AWAIT = 56
    ASYNC = 57
    TYPE_IGNORE = 58
    TYPE_COMMENT = 59
    SOFT_KEYWORD = 60
    FSTRING_START = 61
    FSTRING_MIDDLE = 62
    FSTRING_END = 63
    COMMENT = 64
    NL = 65
    # These aren't used by the C tokenizer but are needed for tokenize.py
    ERRORTOKEN = 66
    ENCODING = 67
    N_TOKENS = 68
    # Codon-specific tokens
    STRING_PREFIX = 200
    NUMBER_SUFFIX = 201
    PIPE = 202
    PARALLEL_PIPE = 203
    # Special definitions for cooperation with parser
    NT_OFFSET = 256

    EXACT_TOKEN_TYPES = {
        "!": EXCLAMATION,
        "!=": NOTEQUAL,
        "%": PERCENT,
        "%=": PERCENTEQUAL,
        "&": AMPER,
        "&=": AMPEREQUAL,
        "(": LPAR,
        ")": RPAR,
        "*": STAR,
        "**": DOUBLESTAR,
        "**=": DOUBLESTAREQUAL,
        "*=": STAREQUAL,
        "+": PLUS,
        "+=": PLUSEQUAL,
        ",": COMMA,
        "-": MINUS,
        "-=": MINEQUAL,
        "->": RARROW,
        ".": DOT,
        "...": ELLIPSIS,
        "/": SLASH,
        "//": DOUBLESLASH,
        "//=": DOUBLESLASHEQUAL,
        "/=": SLASHEQUAL,
        ":": COLON,
        ":=": COLONEQUAL,
        ";": SEMI,
        "<": LESS,
        "<<": LEFTSHIFT,
        "<<=": LEFTSHIFTEQUAL,
        "<=": LESSEQUAL,
        "=": EQUAL,
        "==": EQEQUAL,
        ">": GREATER,
        ">=": GREATEREQUAL,
        ">>": RIGHTSHIFT,
        ">>=": RIGHTSHIFTEQUAL,
        "@": AT,
        "@=": ATEQUAL,
        "[": LSQB,
        "]": RSQB,
        "^": CIRCUMFLEX,
        "^=": CIRCUMFLEXEQUAL,
        "{": LBRACE,
        "|": VBAR,
        "|=": VBAREQUAL,
        "}": RBRACE,
        "~": TILDE,
        "|>": PIPE,
        "||>": PARALLEL_PIPE,
    }

    tok_name: ClassVar[Dict[int, str]] = {}
    tok_value: ClassVar[Dict[str, int]] = {}

    def ISTERMINAL(x):
        return x < Tokens.NT_OFFSET

    def ISNONTERMINAL(x):
        return x >= Tokens.NT_OFFSET

    def ISEOF(x):
        return x == Tokens.ENDMARKER

    def init():
        for name, val in static.vars(Tokens):
            if isinstance(val, int):
                Tokens.tok_value[name] = val
        for name, val in static.vars(Tokens):
            if isinstance(val, int):
                Tokens.tok_name[val] = name

    def get_token(token_name: str):
        if not Tokens.tok_value:
            Tokens.init()
        return Tokens.tok_value.get(token_name)

    def get_name(token: int):
        if not Tokens.tok_value:
            Tokens.init()
        return Tokens.tok_name[token]


class TokenInfo:
    type: int
    string: str
    start: Tuple[int, int]
    end: Tuple[int, int]
    line: str

    def __init__(
        self,
        type: int,
        string: str,
        start: Tuple[int, int],
        end: Tuple[int, int],
        line: str,
    ):
        self.type = type
        self.string = string
        self.start = start
        self.end = end
        self.line = line

    def __repr__(self):
        annotated_type = f"{self.type} ({Tokens.get_name(self.type)})"
        return (
            f"TokenInfo(type={annotated_type}, string={self.string}, "
            f"start={self.start}, end={self.end}, line={self.line})"
        )

    @property
    def exact_type(self):
        if self.type == Tokens.OP and self.string in Tokens.EXACT_TOKEN_TYPES:
            return Tokens.EXACT_TOKEN_TYPES[self.string]
        else:
            return self.type


class TokenPatterns:
    specials: Dict[str, List[str]]
    single_quoted: Set[str]
    triple_quoted: Set[str]
    tabsize: int

    def match_string(quote, number):
        def match(s, start=0):
            i = start
            n = len(s)

            while i < n:
                if s[i] == "\\":
                    # \\.  → backslash + any char
                    if i + 1 >= n:
                        return 0
                    i += 2
                elif s[i] == quote:
                    # '(?!'') → single quote NOT followed by two quotes
                    if s.startswith(quote * number, i):
                        return i + number
                    i += 1
                else:
                    i += 1
            if s.endswith(quote * number) and i == n - number:
                return n
            return 0

        return match

    def _is_digit(c: str, base: int) -> bool:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)"""
        if "0" <= c <= "9":
            return ord(c) - ord("0") < base
        return base == 16 and ("a" <= c <= "f" or "A" <= c <= "F")

    def _scan_digits(line: str, pos: int, base: int) -> int:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)
        Scan DIGIT ("_"? DIGIT)*, or return -1 if DIGIT is absent."""
        n = len(line)
        if pos >= n or not TokenPatterns._is_digit(line[pos], base):
            return -1
        pos += 1
        while pos < n:
            if TokenPatterns._is_digit(line[pos], base):
                pos += 1
            elif line[pos] == "_" and pos + 1 < n and TokenPatterns._is_digit(line[pos + 1], base):
                pos += 2
            else:
                break
        return pos

    def _scan_exponent(line: str, pos: int) -> int:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)"""
        n = len(line)
        if pos >= n or line[pos] not in "eE":
            return -1
        pos += 1
        if pos < n and line[pos] in "+-":
            pos += 1
        return TokenPatterns._scan_digits(line, pos, 10)

    def _scan_number(line: str, pos: int) -> int:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)
        Scan the Number production shared by Token and PseudoToken."""
        n = len(line)
        if line[pos] == ".":
            end = TokenPatterns._scan_digits(line, pos + 1, 10)
            if end < 0:
                return -1
            exponent_end = TokenPatterns._scan_exponent(line, end)
            if exponent_end >= 0:
                end = exponent_end
            if end < n and line[end] in "jJ":
                end += 1
            return end

        decimal_end = TokenPatterns._scan_digits(line, pos, 10)
        if decimal_end < n and line[decimal_end] in "jJ":
            return decimal_end + 1

        point_end = -1
        if decimal_end < n and line[decimal_end] == ".":
            point_end = decimal_end + 1
            fraction_end = TokenPatterns._scan_digits(line, point_end, 10)
            if fraction_end >= 0:
                point_end = fraction_end
            exponent_end = TokenPatterns._scan_exponent(line, point_end)
            if exponent_end >= 0:
                point_end = exponent_end

        exponent_end = TokenPatterns._scan_exponent(line, decimal_end)
        float_end = point_end if point_end >= 0 else exponent_end
        if float_end >= 0:
            if float_end < n and line[float_end] in "jJ":
                float_end += 1
            return float_end

        if line[pos] == "0" and pos + 1 < n:
            prefix = line[pos + 1]
            base = 0
            if prefix in "xX":
                base = 16
            elif prefix in "oO":
                base = 8
            elif prefix in "bB":
                base = 2
            if base:
                digits_pos = pos + 2
                if (
                    digits_pos + 1 < n
                    and line[digits_pos] == "_"
                    and TokenPatterns._is_digit(line[digits_pos + 1], base)
                ):
                    digits_pos += 1
                prefixed_end = TokenPatterns._scan_digits(line, digits_pos, base)
                if prefixed_end >= 0:
                    return prefixed_end

        # Decnumber treats a leading zero specially: 012 tokenizes as 0, 12.
        if line[pos] == "0":
            end = pos + 1
            while end < n:
                if line[end] == "0":
                    end += 1
                elif end + 1 < n and line[end] == "_" and line[end + 1] == "0":
                    end += 2
                else:
                    break
            return end
        return decimal_end

    def _scan_special(self, line: str, pos: int) -> int:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)"""
        initial = line[pos]
        if initial in self.specials:
            for special in self.specials[initial]:
                if line.startswith(special, pos):
                    return pos + len(special)
        return -1

    def _scan_string(line: str, pos: int, allow_continuation: bool) -> int:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)
        Scan String or ContStr depending on allow_continuation."""
        n = len(line)
        quote = line[pos]
        pos += 1
        while pos < n:
            c = line[pos]
            if c == quote:
                return pos + 1
            if c == "\n":
                return -1
            if c == "\\":
                if allow_continuation:
                    if pos + 1 < n and line[pos + 1] == "\n":
                        return pos + 2
                    if pos + 2 < n and line[pos + 1 : pos + 3] == "\r\n":
                        return pos + 3
                elif pos + 1 < n and line[pos + 1] == "\n":
                    return -1
                elif pos + 2 < n and line[pos + 1 : pos + 3] == "\r\n":
                    return -1
                if pos + 1 >= n:
                    return -1
                pos += 2
            else:
                pos += 1
        return -1

    def _scan_plain_token(self, line: str, pos: int, allow_continuation: bool):
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)"""
        n = len(line)
        if pos >= n:
            return -1
        initial = line[pos]
        if TokenPatterns._is_digit(initial, 10) or (
            initial == "." and pos + 1 < n and TokenPatterns._is_digit(line[pos + 1], 10)
        ):
            return TokenPatterns._scan_number(line, pos)
        if initial == "\r" and pos + 1 < n and line[pos + 1] == "\n":
            return pos + 2
        if initial == "\n":
            return pos + 1
        special_end = self._scan_special(line, pos)
        if special_end >= 0:
            return special_end
        if initial == "'" or initial == '"':
            return TokenPatterns._scan_string(line, pos, allow_continuation)
        if initial == "_" or initial.isidentifier():
            end = pos + 1
            while end < n:
                char = line[end]
                if not (
                    "a" <= char <= "z"
                    or "A" <= char <= "Z"
                    or "0" <= char <= "9"
                    or char == "_"
                    or (ord(char) >= 128 and ("_" + char).isidentifier())
                ):
                    break
                end += 1
            return end
        return -1

    def token_span(self, line: str, pos: int = 0) -> Optional[Tuple[int, int]]:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)
        Hand-written equivalent of Token.match(line, pos).span()."""
        n = len(line)
        start = pos
        while pos < n and line[pos] in " \f\t":
            pos += 1
        while pos < n and line[pos] == "\\":
            continuation_end = -1
            if pos + 1 < n and line[pos + 1] == "\n":
                continuation_end = pos + 2
            elif pos + 2 < n and line[pos + 1 : pos + 3] == "\r\n":
                continuation_end = pos + 3
            if continuation_end < 0:
                break
            pos = continuation_end
            while pos < n and line[pos] in " \f\t":
                pos += 1
        if pos < n and line[pos] == "#":
            # Comment? is greedy in the old Token regex, but it backtracks until
            # the following PlainToken can match. Preserve that API behavior even
            # though Token is not used by _tokenize's hot loop.
            comment_start = pos
            comment_end = pos + 1
            while comment_end < n and line[comment_end] not in "\r\n":
                comment_end += 1
            while comment_end > comment_start:
                end = self._scan_plain_token(line, comment_end, False)
                if end >= 0:
                    return start, end
                comment_end -= 1
            return None
        end = self._scan_plain_token(line, pos, False)
        return (start, end) if end >= 0 else None

    def pseudo_token_span(self, line: str, pos: int = 0) -> Optional[Tuple[int, int]]:
        """(AI-generated specialized replacement of regex-based pseudotoken scanner.)
        Hand-written equivalent of PseudoToken.match(...).span(1)."""
        n = len(line)
        while pos < n and line[pos] in " \f\t":
            pos += 1
        start = pos
        if pos == n:
            return start, start

        initial = line[pos]
        if initial == "\\":
            if pos + 1 < n and line[pos + 1] == "\n":
                return start, pos + 2
            if pos + 2 < n and line[pos + 1 : pos + 3] == "\r\n":
                return start, pos + 3
        if initial == "#":
            end = pos + 1
            while end < n and line[end] not in "\r\n":
                end += 1
            return start, end
        if line.startswith("'''", pos) or line.startswith('"""', pos):
            return start, pos + 3
        end = self._scan_plain_token(line, pos, True)
        return (start, end) if end >= 0 else None

    def __init__(self):
        Tokens.init()
        self.specials = {}
        for special in sorted(Tokens.EXACT_TOKEN_TYPES, reverse=True):
            initial = special[0]
            if initial not in self.specials:
                self.specials[initial] = []
            self.specials[initial].append(special)
        self.tabsize = 8


class TokenError(Exception):
    args: Tuple[str, Tuple[int, int]]

    def __init__(self, *args):
        super().__init__(args[0])
        self.args = args


class IndentationError(Exception):
    args: Tuple[str, Tuple[str, int, int, str]]

    def __init__(self, *args):
        super().__init__(args[0])
        self.args = args


class StopTokenizing(Exception):
    def __init__(self, message: str = ""):
        super().__init__(message)


token_patterns = TokenPatterns()


def tokenize(readline):
    """
    Tokenize an iterable of physical source lines (strings, not bytes).

    The generator produces TokenInfo objects with these members: the token type; the
    token string; a 2-tuple (srow, scol) of ints specifying the row and
    column where the token begins in the source; a 2-tuple (erow, ecol) of
    ints specifying the row and column where the token ends in the source;
    and the line on which the token was found.  The line passed is the
    physical line.
    """
    yield from _tokenize(iter(readline), token_patterns)


generate_tokens = tokenize


class _FStringMode:
    # CPython's tokenizer_mode: literal text, regular expression code, or format text.
    TEXT = 0
    EXPR = 1
    FORMAT = 2

    def __init__(self, kind, quote, raw, start, depth=0):
        self.kind = kind
        self.quote = quote
        self.raw = raw
        self.start = start
        self.depth = depth
        self.brackets = []
        self.after_middle = False


class _LiteralLexer:
    """Streaming, non-recursive string lexer sharing Codon's ordinary token scanner.

    Reference: CPython 3.12 Parser/tokenizer.c, tok_get_normal_mode and
    tok_get_fstring_mode. No fragments are synthesized or tokenized again.
    """

    def __init__(self, readline, line, lnum, pos, patterns):
        self.readline = readline
        self.line = line
        self.lnum = lnum
        self.pos = pos
        self.patterns = patterns
        self.lines = [line]
        self.first_line = lnum
        self.modes = []
        self.pending_token = None
        self.number_end = (0, 0)
        self.fstring_count = 0

    def next_line(self):
        line = next(self.readline, "")
        if not line:
            return False
        self.line = line
        self.lines.append(line)
        self.lnum += 1
        self.pos = 0
        return True

    def token(self, kind, value, start, end):
        first = start[0] - self.first_line
        last = end[0] - self.first_line
        line = self.lines[first] if first == last else "".join(self.lines[first : last + 1])
        return TokenInfo(kind, value, start, end, line)

    def source(self, start, end):
        first = start[0] - self.first_line
        last = end[0] - self.first_line
        if first == last:
            return self.lines[first][start[1] : end[1]]
        return (
            self.lines[first][start[1] :]
            + "".join(self.lines[first + 1 : last])
            + self.lines[last][: end[1]]
        )

    def open_literal(self, prefix=None):
        char = self.line[self.pos]
        quote = char * 3 if self.line.startswith(char * 3, self.pos) else char
        if prefix and prefix.string.lower() in {"f", "fr", "rf"}:
            if self.fstring_count + 1 >= 150:
                raise TokenError("too many nested f-strings", prefix.start)
            self.pos += len(quote)
            self.modes.append(
                _FStringMode(_FStringMode.TEXT, quote, "r" in prefix.string.lower(), prefix.start)
            )
            self.fstring_count += 1
            return self.token(
                Tokens.FSTRING_START, prefix.string + quote, prefix.start, (self.lnum, self.pos)
            )
        string = self.scan_string(quote)
        if prefix:
            self.pending_token = string
            return prefix
        return string

    def scan_string(self, quote):
        start = (self.lnum, self.pos)
        self.pos += len(quote)
        while True:
            if self.pos >= len(self.line):
                if not self.next_line():
                    raise TokenError("EOF in multi-line string", start)
                continue
            if self.line.startswith(quote, self.pos):
                self.pos += len(quote)
                end = (self.lnum, self.pos)
                return self.token(Tokens.STRING, self.source(start, end), start, end)
            char = self.line[self.pos]
            if char in "\r\n" and len(quote) == 1:
                raise TokenError("unterminated string literal", start)
            if char == "\\":
                self.pos += 1
                if self.pos < len(self.line):
                    if self.line.startswith("\r\n", self.pos):
                        self.pos += 2
                    else:
                        self.pos += 1
            else:
                self.pos += 1

    def scan_text(self, mode):
        start = (self.lnum, self.pos)
        pieces = []
        piece_start = self.pos
        while True:
            if self.pos >= len(self.line):
                pieces.append(self.line[piece_start:])
                if not self.next_line():
                    raise TokenError("unterminated f-string literal", mode.start)
                piece_start = 0
            char = self.line[self.pos]
            boundary = (
                char in "{}"
                or self.line.startswith(mode.quote, self.pos)
                or (char in "\r\n" and len(mode.quote) == 1)
            )
            if boundary:
                value = "".join(pieces) + self.line[piece_start : self.pos]
                # Doubled braces are escapes only in literal mode, not format specs.
                if (
                    mode.kind == _FStringMode.TEXT
                    and char in "{}"
                    and self.line.startswith(char * 2, self.pos)
                ):
                    self.pos += 1
                    end = (self.lnum, self.pos)
                    self.pos += 1
                    mode.after_middle = False
                    return self.token(Tokens.FSTRING_MIDDLE, value + char, start, end)
                if value or (
                    mode.kind == _FStringMode.FORMAT
                    and (char != "{" or self.line.startswith("{{", self.pos))
                    and not mode.after_middle
                ):
                    mode.after_middle = True
                    return self.token(Tokens.FSTRING_MIDDLE, value, start, (self.lnum, self.pos))
                if char == "{":
                    if mode.depth >= 3:
                        raise TokenError("f-string: expressions nested too deeply", start)
                    mode.after_middle = False
                    self.pos += 1
                    self.modes.append(
                        _FStringMode(_FStringMode.EXPR, mode.quote, mode.raw, start, mode.depth + 1)
                    )
                    return self.token(Tokens.OP, "{", start, (self.lnum, self.pos))
                if char == "}":
                    if mode.kind == _FStringMode.TEXT:
                        raise TokenError("single '}' is not allowed in f-string", start)
                    self.pos += 1
                    self.modes.pop()
                    return self.token(Tokens.OP, "}", start, (self.lnum, self.pos))
                if char in "\r\n" and mode.kind == _FStringMode.FORMAT:
                    # CPython resumes regular lexing for a newline terminating a
                    # single-quoted format spec; the field's closing brace follows.
                    mode.kind = _FStringMode.EXPR
                    return self.scan_expr(mode)
                if mode.kind == _FStringMode.FORMAT:
                    raise TokenError("f-string: expecting '}'", start)
                if char in "\r\n":
                    raise TokenError("unterminated f-string literal", mode.start)
                self.pos += len(mode.quote)
                self.modes.pop()
                self.fstring_count -= 1
                return self.token(Tokens.FSTRING_END, mode.quote, start, (self.lnum, self.pos))
            if char == "\\":
                self.pos += 1
                if self.pos < len(self.line):
                    char = self.line[self.pos]
                    if char in "{}":
                        # A backslash never escapes a replacement-field brace.
                        continue
                    if not mode.raw and self.line.startswith("N{", self.pos):
                        # Braces belonging to a named Unicode escape are literal.
                        self.pos += 2
                        while self.pos < len(self.line) and self.line[self.pos] != "}":
                            self.pos += 1
                        if self.pos >= len(self.line):
                            raise TokenError("malformed Unicode escape", start)
                        self.pos += 1
                        mode.after_middle = False
                        return self.token(
                            Tokens.FSTRING_MIDDLE,
                            "".join(pieces) + self.line[piece_start : self.pos],
                            start,
                            (self.lnum, self.pos),
                        )
                    if self.line.startswith("\r\n", self.pos):
                        self.pos += 2
                    else:
                        self.pos += 1
            else:
                self.pos += 1

    def scan_expr(self, mode):
        while True:
            if self.pos >= len(self.line):
                if not self.next_line():
                    raise TokenError("f-string: expecting '}'", mode.start)
            while self.pos < len(self.line) and self.line[self.pos] in " \t\f":
                self.pos += 1
            if self.pos >= len(self.line):
                continue
            start = (self.lnum, self.pos)
            char = self.line[self.pos]
            if char == ":" and not mode.brackets:
                mode.kind = _FStringMode.FORMAT
                mode.after_middle = False
                self.pos += 1
                return self.token(Tokens.OP, ":", start, (self.lnum, self.pos))
            if char == "}" and not mode.brackets:
                self.pos += 1
                self.modes.pop()
                return self.token(Tokens.OP, "}", start, (self.lnum, self.pos))
            if char in "\"'":
                return self.open_literal()
            span = self.patterns.pseudo_token_span(self.line, self.pos)
            if not span:
                if char == "\\":
                    raise TokenError("unexpected character after line continuation", start)
                self.pos += 1
                return self.token(Tokens.ERRORTOKEN, char, start, (self.lnum, self.pos))
            _, self.pos = span
            end = (self.lnum, self.pos)
            value = self.line[start[1] : self.pos]
            if char in "\r\n":
                return self.token(Tokens.NL, value, start, end)
            if char == "#":
                return self.token(Tokens.COMMENT, value, start, end)
            if char == "\\":
                continue
            if "0" <= char <= "9" or (char == "." and value not in {".", "..."}):
                self.number_end = end
                return self.token(Tokens.NUMBER, value, start, end)
            if char.isidentifier():
                if self.pos < len(self.line) and self.line[self.pos] in "\"'":
                    return self.open_literal(self.token(Tokens.STRING_PREFIX, value, start, end))
                kind = Tokens.NAME
                if self.number_end == start and value not in {
                    "if",
                    "else",
                    "and",
                    "or",
                    "in",
                    "is",
                    "not",
                }:
                    kind = Tokens.NUMBER_SUFFIX
                return self.token(kind, value, start, end)
            if char in "([{":
                if len(mode.brackets) >= 200:
                    raise TokenError("too many nested parentheses", start)
                mode.brackets.append(char)
            elif char in ")]}":
                if not mode.brackets or "([{".index(mode.brackets[-1]) != ")]}".index(char):
                    raise TokenError("f-string: mismatched closing bracket", start)
                mode.brackets.pop()
            return self.token(Tokens.OP, value, start, end)

    def scan(self, prefix):
        yield self.open_literal(prefix)
        while self.pending_token or self.modes:
            if self.pending_token:
                token = self.pending_token
                self.pending_token = None
            else:
                mode = self.modes[-1]
                token = (
                    self.scan_expr(mode) if mode.kind == _FStringMode.EXPR else self.scan_text(mode)
                )
            yield token


def _tokenize(readline, token_patterns) -> Generator[TokenInfo]:
    str_prefix: Optional[TokenInfo] = None
    number_end = (0, 0)
    # Keywords can immediately follow numbers without being Codon suffixes.
    space_keywords = {"if", "else", "and", "or", "in", "is", "not"}

    lnum = parenlev = continued = 0
    raw_decorator = False
    raw_block_pending = False
    raw_def_indent = 0
    raw_block_indent = None
    numchars = "0123456789"
    indents = [0]

    last_line = b""
    line = b""
    while True:  # loop over lines in stream
        try:
            # We capture the value of the line variable here because
            # readline uses the empty string '' to signal end of input,
            # hence `line` itself will always be overwritten at the end
            # of this loop.
            last_line = line
            line = next(readline)
        except StopIteration:
            line = b""

        lnum += 1
        pos, max = 0, len(line)

        stripped = line.lstrip(" \t\f") if line else ""
        leading = len(line) - len(stripped) if line else 0
        if raw_block_pending and stripped and not stripped.startswith(("#", "\r", "\n")):
            if leading > raw_def_indent:
                raw_block_indent = leading
                indents.append(leading)
                yield TokenInfo(Tokens.INDENT, line[:leading], (lnum, 0), (lnum, leading), line)
            raw_block_pending = False
        if raw_block_indent is not None:
            if not line:
                raw_block_indent = None
                raw_decorator = False
            elif not stripped or stripped.startswith(("#", "\r", "\n")):
                yield TokenInfo(Tokens.NL, line, (lnum, 0), (lnum, len(line)), line)
                continue
            elif leading >= raw_block_indent:
                content = line[leading:].rstrip("\r\n")
                yield TokenInfo(
                    Tokens.NAME,
                    content,
                    (lnum, leading),
                    (lnum, leading + len(content)),
                    line,
                )
                yield TokenInfo(
                    Tokens.NEWLINE,
                    line[leading + len(content) :],
                    (lnum, leading + len(content)),
                    (lnum, len(line)),
                    line,
                )
                continue
            else:
                raw_block_indent = None
                raw_decorator = False

        if stripped.startswith(("@llvm", "@python")):
            raw_decorator = True
        elif raw_decorator and stripped.startswith("def "):
            raw_def_indent = leading
            raw_block_pending = True

        if parenlev == 0 and not continued:  # new statement
            if not line:
                break
            column = 0
            while pos < max:  # measure leading whitespace
                if line[pos] == " ":
                    column += 1
                elif line[pos] == "\t":
                    column = (column // token_patterns.tabsize + 1) * token_patterns.tabsize
                elif line[pos] == "\f":
                    column = 0
                else:
                    break
                pos += 1
            if pos == max:
                break

            if line[pos] in "#\r\n":  # skip comments or blank lines
                if line[pos] == "#":
                    comment_token = line[pos:].rstrip("\r\n")
                    yield TokenInfo(
                        Tokens.TYPE_COMMENT
                        if comment_token.startswith("## codon:")
                        else Tokens.COMMENT,
                        comment_token,
                        (lnum, pos),
                        (lnum, pos + len(comment_token)),
                        line,
                    )
                    pos += len(comment_token)

                yield TokenInfo(
                    Tokens.NEWLINE if line.lstrip().startswith("## codon:") else Tokens.NL,
                    line[pos:],
                    (lnum, pos),
                    (lnum, len(line)),
                    line,
                )
                continue

            if column > indents[-1]:  # count indents or dedents
                indents.append(column)
                yield TokenInfo(Tokens.INDENT, line[:pos], (lnum, 0), (lnum, pos), line)
            while column < indents[-1]:
                if column not in indents:
                    raise IndentationError(
                        "unindent does not match any outer indentation level",
                        ("<tokenize>", lnum, pos, line),
                    )
                indents.pop()

                yield TokenInfo(Tokens.DEDENT, "", (lnum, pos), (lnum, pos), line)

        else:  # continued statement
            if not line:
                raise TokenError("EOF in multi-line statement", (lnum, 0))
            continued = 0

        while pos < max:
            while pos < max and line[pos] in " \t\f":
                pos += 1
            if pos == max:
                break
            if line[pos] in "\"'":
                lexer = _LiteralLexer(readline, line, lnum, pos, token_patterns)
                yield from lexer.scan(str_prefix)
                str_prefix = None
                line, lnum, pos = lexer.line, lexer.lnum, lexer.pos
                max = len(line)
                continue
            pseudospan = token_patterns.pseudo_token_span(line, pos)
            if pseudospan:  # scan for tokens
                start, end = pseudospan
                spos, epos, pos = (lnum, start), (lnum, end), end
                if start == end:
                    continue
                token, initial = line[start:end], line[start]

                if (
                    initial in numchars  # ordinary number
                    or (initial == "." and token != "." and token != "...")
                ):
                    number_end = epos
                    yield TokenInfo(Tokens.NUMBER, token, spos, epos, line)
                elif initial in "\r\n":
                    if parenlev > 0:
                        yield TokenInfo(Tokens.NL, token, spos, epos, line)
                    else:
                        yield TokenInfo(Tokens.NEWLINE, token, spos, epos, line)

                elif initial == "#":
                    assert not token.endswith("\n")
                    yield TokenInfo(Tokens.COMMENT, token, spos, epos, line)

                elif initial.isidentifier():  # ordinary name
                    if end < len(line) and (line[end] == '"' or line[end] == "'"):
                        str_prefix = TokenInfo(Tokens.STRING_PREFIX, token, spos, epos, line)
                    elif (
                        number_end == spos
                        # Support stuff like 1if(condition)else(value) (not a suffix!)
                        and token not in space_keywords
                    ):
                        yield TokenInfo(Tokens.NUMBER_SUFFIX, token, spos, epos, line)
                    else:
                        yield TokenInfo(Tokens.NAME, token, spos, epos, line)
                elif initial == "\\":  # continued stmt
                    continued = 1
                else:
                    if initial in "([{":
                        parenlev += 1
                    elif initial in ")]}":
                        parenlev -= 1
                    yield TokenInfo(Tokens.OP, token, spos, epos, line)
            else:
                if line[pos] == "\\":
                    raise TokenError("unexpected character after line continuation", (lnum, pos))
                yield TokenInfo(Tokens.ERRORTOKEN, line[pos], (lnum, pos), (lnum, pos + 1), line)
                pos += 1

    # Add an implicit NEWLINE if the input doesn't end in one
    if last_line and last_line[-1] not in "\r\n" and not last_line.strip().startswith("#"):
        yield TokenInfo(
            Tokens.NEWLINE,
            "",
            (lnum - 1, len(last_line)),
            (lnum - 1, len(last_line) + 1),
            last_line,
        )
    for _ in indents[1:]:  # pop remaining indent levels
        yield TokenInfo(Tokens.DEDENT, "", (lnum, 0), (lnum, 0), "")
    yield TokenInfo(Tokens.ENDMARKER, "", (lnum, 0), (lnum, 0), "")


def main(filename):
    import sys

    def perror(message):
        sys.stderr.write(message)
        sys.stderr.write("\n")

    def error(message, filename=None, location=None):
        if location is not None:
            perror(f"{filename}:{location}: error: {message}")
        elif filename:
            perror(f"{filename}: error: {message}")
        else:
            perror(f"error: {message}")
        sys.exit(1)

    try:
        with open(filename, "r") as f:
            tokens = list(tokenize(f))
        for token in tokens:
            token_type = token.type
            token_range = f"{token.start[0]},{token.start[1]}-{token.end[0]},{token.end[1]}:"
            s = "'" + token.string.replace("\n", "\\n") + "'"
            print(f"{token_range:20}{Tokens.get_name(token_type):15}{s:15}")
    except IndentationError as err:
        line, column = err.args[1][1:3]
        error(err.args[0], filename, (line, column))
    except TokenError as err:
        line, column = err.args[1]
        error(err.args[0], filename, (line, column))
    except OSError as err:
        error(err)
    except Exception as err:
        perror(f"unexpected error: {err}")
        raise
