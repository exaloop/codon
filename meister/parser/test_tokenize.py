"""Lexer regressions; the stdlib tokenizer is used only as a test oracle."""

import ast as python_ast
import io
import token as python_token
import tokenize as python_tokenize
import unittest

from .. import parser
from ..ast import nodes as ast
from . import tokenize


class TokenizerTests(unittest.TestCase):
    def tokens(self, source):
        return list(tokenize.generate_tokens(io.StringIO(source)))

    def expression(self, source):
        return parser.parse(code=source + "\n").items[0].expr

    def test_cpython_fstring_tokens(self):
        sources = [
            'f""',
            'f"text"',
            'F"{value}"',
            'f"a{{b}}c"',
            'f"{{{{}}}}"',
            'f"{data["key"]}"',
            "f\"{'}'}\"",
            "f\"{'{'}\"",
            'f"{f"{value}"}"',
            "f\"{'\\n'.join(parts)}\"",
            'f"{(\n  1 + 2\n)}"',
            'f"""{value # } in a comment\n}"""',
            'f"""{value # : in a comment\n}"""',
            'f"""first\n{value}\nlast"""',
            'f"{value:#010x}"',
            'f"{value: 10}"',
            'f"{value:}"',
            'f"{value:{width}.{precision}f}"',
            'f"{value:{{}}}"',
            'f"{value:{width:{precision}}}"',
            'f"{value:=10}"',
            'f"{(value := 1)}"',
            'f"{ value = }"',
            'f"{value=!r:10}"',
            'rf"\\n{value}"',
            'Fr"\\n{value}"',
            'f"\\N{LATIN CAPITAL LETTER A}{value}"',
            'f"\\N{GREEK CAPITAL LETTER DELTA}3"',
            'f"\\N{LEFT CURLY BRACKET}1+1\\N{RIGHT CURLY BRACKET}"',
            'f"a\\\nb{value}"',
            'f"{value!s}"',
            'f"{value!a}"',
        ]
        for source in sources:
            with self.subTest(source=source):
                for ending in ("", "\n"):
                    text = source + ending
                    expected = [
                        (python_token.tok_name[t.type], t.string, t.start, t.end, t.line)
                        for t in python_tokenize.generate_tokens(io.StringIO(text).readline)
                    ]
                    actual = [
                        (tokenize.Tokens.get_name(t.type), t.string, t.start, t.end, t.line)
                        for t in self.tokens(text)
                    ]
                    self.assertEqual(actual, expected)

    def test_pep701_expressions_parse(self):
        for source in (
            'f"{data["key"]}"',
            'f"{f"{value}"}"',
            "f\"{'}'}\"",
            "f\"{'\\n'.join(parts)}\"",
            'f"{(\n  1 + 2\n)}"',
            'f"""{value # } : ignored\n}"""',
        ):
            with self.subTest(source=source):
                self.assertIsInstance(self.expression(source), ast.StringExpr)

    def test_codon_suffixes(self):
        for source, suffix in (("1u8", "u8"), ("1u", "u"), ("2.0f32", "f32")):
            for code in (source, 'f"{' + source + '}"'):
                with self.subTest(source=code):
                    tokens = self.tokens(code + "\n")
                    self.assertEqual(
                        [t.string for t in tokens if t.type == tokenize.Tokens.NUMBER_SUFFIX],
                        [suffix],
                    )
                    self.expression(code)

    def test_codon_string_prefixes(self):
        for code in ('dna"ACGT"', "f\"{dna'ACGT'}\""):
            with self.subTest(source=code):
                self.assertEqual(
                    [
                        t.string
                        for t in self.tokens(code + "\n")
                        if t.type == tokenize.Tokens.STRING_PREFIX
                    ],
                    ["dna"],
                )
                self.expression(code)

    def test_codon_operators_in_fields(self):
        for op in ("|>", "||>"):
            code = 'f"{value ' + op + ' fn}"'
            with self.subTest(op=op):
                self.assertIn(op, [t.string for t in self.tokens(code + "\n")])
                self.expression(code)

    def test_keywords_are_not_number_suffixes(self):
        for code in ("1if(flag)else(2)", 'f"{1if(flag)else(2)}"'):
            self.assertFalse(
                any(t.type == tokenize.Tokens.NUMBER_SUFFIX for t in self.tokens(code + "\n"))
            )
            self.expression(code)

    def test_unicode_identifier(self):
        self.assertEqual(self.expression("a\u0301").value, "a\u0301")
        self.assertEqual(self.expression('f"{a\u0301}"').strings[0].expr.value, "a\u0301")

    def test_raw_blocks_stay_opaque(self):
        for decorator in ("llvm", "python"):
            code = (
                "@" + decorator + "\n"
                "def external(x: int) -> int:\n"
                "    entry:\n"
                "\n"
                '    %v = f"{ ; deliberately not Codon syntax\n'
                "    ret i64 %v\n"
                "after = 1u8\n"
            )
            with self.subTest(decorator=decorator):
                tokens = self.tokens(code)
                self.assertFalse(any(t.type == tokenize.Tokens.FSTRING_START for t in tokens))
                self.assertIn(
                    '%v = f"{ ; deliberately not Codon syntax', [t.string for t in tokens]
                )
                self.assertEqual(
                    [t.string for t in tokens if t.type == tokenize.Tokens.NUMBER_SUFFIX], ["u8"]
                )
                tree = parser.parse(code=code)
                body = tree.items[0].suite.items[0].expr.value
                self.assertIn('entry:\n\n    %v = f"{', body)

    def test_directives(self):
        tokens = self.tokens("## codon: no_inline\nx = 1\n")
        self.assertEqual(tokens[0].type, tokenize.Tokens.TYPE_COMMENT)
        self.assertEqual(tokens[1].type, tokenize.Tokens.NEWLINE)

    def test_independent_streams(self):
        first = tokenize.generate_tokens(["1u8\n"])
        second = tokenize.generate_tokens(["identifier\n"])
        self.assertEqual(next(first).type, tokenize.Tokens.NUMBER)
        self.assertEqual(next(second).type, tokenize.Tokens.NAME)
        self.assertEqual(next(first).type, tokenize.Tokens.NUMBER_SUFFIX)

    def test_streaming_fstring(self):
        lines = iter(['f"""first\n', "{value}\n", 'last"""\n', "sentinel\n"])
        stream = tokenize.generate_tokens(lines)
        self.assertEqual(next(stream).type, tokenize.Tokens.FSTRING_START)
        self.assertEqual(next(lines), "{value}\n")

    def test_debug_expression_text(self):
        for code in (
            'f"{ value = }"',
            'f"{(\n value\n)=}"',
            'f"""{(\n value # comment\n)=}"""',
            'f"""{value= # comment\n}"""',
        ):
            with self.subTest(source=code):
                expected = python_ast.parse(code).body[0].value.values[0].value
                self.assertEqual(self.expression(code).strings[0].value, expected)

    def test_format_text_is_opaque(self):
        field = self.expression('f"{value: custom # opaque ! format}"').strings[0]
        self.assertEqual(field.format.spec, " custom # opaque ! format")

    def test_empty_format_and_debug(self):
        field = self.expression('f"{value=:}"').strings[0]
        self.assertEqual(field.format.conversion, "r")
        self.assertEqual(field.format.spec, "")

    def test_lexical_errors(self):
        for source in ('f"}"', 'f"{(]}"', 'f"{value', 'f"text\n', '"text\n', '"""text'):
            with self.subTest(source=source):
                with self.assertRaises(tokenize.TokenError):
                    self.tokens(source)

    def test_nesting_limits(self):
        source = "value"
        for _ in range(5):
            source = 'f"{' + source + '}"'
        self.tokens(source)
        self.expression(source)
        for _ in range(150):
            source = 'f"{' + source + '}"'
        with self.assertRaises(tokenize.TokenError):
            self.tokens(source)
        self.expression('f"{value:{width:{precision}}}"')
        with self.assertRaises(tokenize.TokenError):
            self.tokens('f"{value:{width:{precision:{extra}}}}"')


if __name__ == "__main__":
    unittest.main()
