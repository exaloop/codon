### File: token.py / tokenize.py (Python 3.11.11)

from ..bridge import *

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
    prev_token: Optional[TokenInfo]

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
            elif (
                line[pos] == "_"
                and pos + 1 < n
                and TokenPatterns._is_digit(line[pos + 1], base)
            ):
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
            initial == "."
            and pos + 1 < n
            and TokenPatterns._is_digit(line[pos + 1], 10)
        ):
            return TokenPatterns._scan_number(line, pos)
        if initial == "\r" and pos + 1 < n and line[pos + 1] == "\n":
            return pos + 2
        if initial == "\n":
            return pos + 1
        special_end = self._scan_special(line, pos)
        if special_end >= 0:
            return special_end
        if initial == "'" or initial == '\"':
            return TokenPatterns._scan_string(line, pos, allow_continuation)
        if initial == "_" or initial.isalnum():
            end = pos + 1
            while end < n and (line[end] == "_" or line[end].isalnum()):
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
        if line.startswith("'''", pos) or line.startswith('\"\"\"', pos):
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
        self.prev_token = None


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
    The tokenize() generator requires one argument, readline, which
    must be a generator object which provides the same interface as the
    readline method of built-in file objects. Each call to the function
    should return one line of input as bytes.

    The generator produces 5-tuples with these members: the token type; the
    token string; a 2-tuple (srow, scol) of ints specifying the row and
    column where the token begins in the source; a 2-tuple (erow, ecol) of
    ints specifying the row and column where the token ends in the source;
    and the line on which the token was found.  The line passed is the
    physical line.
    """
    for token in _tokenize(iter(readline), token_patterns):
        yield token
        token_patterns.prev_token = token


generate_tokens = tokenize


def _tokenize(readline, token_patterns) -> Generator[TokenInfo]:
    endprog = None
    strstart = (0, 0)
    str_prefix: Optional[TokenInfo] = None

    def get_string(token, spos, lnum, pos, line) -> Generator[TokenInfo]:
        nonlocal str_prefix

        prefix = ""
        if len(token) >= 6 and token[:3] == token[-3:]:
            prefix = token[:3]
        elif len(token) >= 2 and token[0] == token[-1]:
            prefix = token[0]

        if str_prefix and str_prefix.string in ["f", "fr", "rf"]:
            yield TokenInfo(
                Tokens.FSTRING_START,
                prefix,
                spos,
                (spos[0], spos[1] + len(prefix)),
                line,
            )

            last_brace, brace_cnt = len(prefix), 0
            last_pos = current_pos = spos[0], spos[1] + len(prefix)
            for i in range(len(prefix), len(token) - len(prefix)):
                if token[i] == "{":
                    if last_brace < i:
                        # from last_brace to i
                        yield TokenInfo(
                            Tokens.FSTRING_MIDDLE,
                            token[last_brace:i],
                            last_pos,
                            current_pos,
                            line,
                        )
                    if brace_cnt == 0:
                        last_brace = i + 1
                        last_pos = current_pos
                    brace_cnt += 1
                elif token[i] == "}":
                    brace_cnt -= 1
                    if brace_cnt == 0:
                        code = token[last_brace - 1 : i + 1].split("\n")
                        tokens = list(_tokenize(iter(code), token_patterns))
                        if tokens and tokens[-1].type == Tokens.ENDMARKER:
                            tokens.pop()
                        if tokens and tokens[-1].type == Tokens.NEWLINE:
                            tokens.pop()
                        for t in tokens:
                            t.start = (
                                last_pos[0] + t.start[0] - 1,
                                last_pos[1] + t.start[1],
                            )
                            t.end = (last_pos[0] + t.end[0] - 1, last_pos[1] + t.end[1])
                            yield t
                    last_brace = i + 1
                if token[i] == "\n":
                    current_pos = (current_pos[0] + 1, 0)
                else:
                    current_pos = (current_pos[0], current_pos[1] + 1)
            if brace_cnt > 0:
                yield TokenInfo(Tokens.ERRORTOKEN, line[pos], spos, (spos[0], spos[1] + len(prefix)), line)
            if brace_cnt < 0:
                yield TokenInfo(Tokens.ERRORTOKEN, line[pos], spos, (spos[0], spos[1] + len(prefix)), line)
            if last_brace < len(token) - len(prefix):
                yield TokenInfo(
                    Tokens.FSTRING_MIDDLE,
                    token[last_brace : len(token) - len(prefix)],
                    last_pos,
                    current_pos,
                    line,
                )

            yield TokenInfo(
                Tokens.FSTRING_END, prefix, (lnum, pos - len(prefix)), (lnum, pos), line
            )
            str_prefix = None
        else:
            if str_prefix:
                yield Codon.unwrap(str_prefix)
                str_prefix = None
            yield TokenInfo(Tokens.STRING, token, spos, (lnum, pos), line)

    lnum = parenlev = continued = 0
    numchars = "0123456789"
    contstr, needcont = "", 0
    contline = None
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

        if contstr:  # continued string
            if not line:
                raise TokenError("EOF in multi-line string", strstart)
            endmatch = endprog(line)
            if endmatch:
                pos = end = endmatch
                if str_prefix:
                    yield Codon.unwrap(str_prefix)
                    str_prefix = None
                yield TokenInfo(
                    Tokens.STRING,
                    contstr + line[:end],
                    strstart,
                    (lnum, end),
                    contline + line,
                )
                contstr, needcont = "", 0
                contline = None
            elif needcont and line[-2:] != "\\\n" and line[-3:] != "\\\r\n":
                yield TokenInfo(
                    Tokens.ERRORTOKEN,
                    contstr + line,
                    strstart,
                    (lnum, len(line)),
                    contline,
                )
                contstr = ""
                contline = None
                continue
            else:
                contstr = contstr + line
                contline = contline + line
                continue

        elif parenlev == 0 and not continued:  # new statement
            if not line:
                break
            column = 0
            while pos < max:  # measure leading whitespace
                if line[pos] == " ":
                    column += 1
                elif line[pos] == "\t":
                    column = (
                        column // token_patterns.tabsize + 1
                    ) * token_patterns.tabsize
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
                        Tokens.COMMENT,
                        comment_token,
                        (lnum, pos),
                        (lnum, pos + len(comment_token)),
                        line,
                    )
                    pos += len(comment_token)

                yield TokenInfo(
                    Tokens.NL, line[pos:], (lnum, pos), (lnum, len(line)), line
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
                    yield TokenInfo(Tokens.NUMBER, token, spos, epos, line)
                elif initial in "\r\n":
                    if parenlev > 0:
                        yield TokenInfo(Tokens.NL, token, spos, epos, line)
                    else:
                        yield TokenInfo(Tokens.NEWLINE, token, spos, epos, line)

                elif initial == "#":
                    assert not token.endswith("\n")
                    yield TokenInfo(Tokens.COMMENT, token, spos, epos, line)

                elif token == "'''" or token == '"""':
                    endprog = TokenPatterns.match_string(token[0], 3)
                    endmatch = endprog(line, pos)
                    if endmatch:  # all on one line
                        pos = endmatch
                        token = line[start:pos]
                        yield from get_string(token, spos, lnum, pos, line)
                    else:
                        strstart = (lnum, start)  # multiple lines
                        contstr = line[start:]
                        contline = line
                        break

                # Note that single quote checking must come after
                #  triple quote checking (above).
                elif initial == '"' or initial == "'":
                    if token[-1] == "\n":  # continued string
                        strstart = (lnum, start)
                        endprog = TokenPatterns.match_string(initial, 1)
                        contstr, needcont = line[start:], 1
                        contline = line
                        break
                    else:
                        yield from get_string(token, spos, lnum, pos, line)

                elif initial.isidentifier():  # ordinary name
                    if end < len(line) and (line[end] == '"' or line[end] == "'"):
                        str_prefix = TokenInfo(
                            Tokens.STRING_PREFIX, token, spos, epos, line
                        )
                    elif (
                        token_patterns.prev_token
                        and token_patterns.prev_token.end == (lnum, start)
                        and token_patterns.prev_token.type == Tokens.NUMBER
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
                yield TokenInfo(
                    Tokens.ERRORTOKEN, line[pos], (lnum, pos), (lnum, pos + 1), line
                )
                pos += 1

    # Add an implicit NEWLINE if the input doesn't end in one
    if (
        last_line
        and last_line[-1] not in "\r\n"
        and not last_line.strip().startswith("#")
    ):
        yield TokenInfo(
            Tokens.NEWLINE,
            "",
            (lnum - 1, len(last_line)),
            (lnum - 1, len(last_line) + 1),
            "",
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
            token_range = (
                f"{token.start[0]},{token.start[1]}-{token.end[0]},{token.end[1]}:"
            )
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
