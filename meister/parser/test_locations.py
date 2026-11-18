"""Source locations that are observable in generated runtime diagnostics."""

import unittest

from . import parse


class FunctionLocationTests(unittest.TestCase):
    def test_async_function_starts_at_def_token(self):
        for gap in (" ", "    ", "\t"):
            with self.subTest(gap=gap):
                node = parse(code=f"async{gap}def f():\n    pass\n").items[0]
                self.assertTrue(node.async_)
                self.assertEqual(node.info.line, 1)
                self.assertEqual(node.info.col, len("async" + gap) + 1)

    def test_decorators_preserve_function_start(self):
        node = parse(code="@decorator\nasync def f():\n    pass\n").items[0]
        self.assertEqual((node.info.line, node.info.col), (2, 7))

    def test_sync_function_start_is_unchanged(self):
        node = parse(code="def f():\n    pass\n").items[0]
        self.assertEqual((node.info.line, node.info.col), (1, 1))
