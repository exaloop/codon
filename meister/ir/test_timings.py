# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

import unittest
from contextlib import redirect_stderr
from io import StringIO
from unittest.mock import patch

from .codegen import StageTimings


class StageTimingsTest(unittest.TestCase):
    def test_nested_stages_are_exclusive(self):
        with patch("meister.ir.codegen.time.perf_counter", side_effect=iter(range(5))):
            timing = StageTimings()
            with timing.stage("typecheck"), timing.stage("parse"):
                pass
        self.assertEqual(timing.times, {"typecheck": 2, "parse": 1})
        self.assertEqual(timing.stack, [])
        self.assertGreater(timing.peaks["parse"], 0)

    def test_stage_is_closed_on_exception(self):
        timing = StageTimings()
        with self.assertRaises(ValueError), timing.stage("scope"):
            raise ValueError("failure")
        self.assertIn("scope", timing.times)
        self.assertEqual(timing.stack, [])

    def test_reports_before_next_stage_and_defers_import_totals(self):
        output = StringIO()
        with redirect_stderr(output):
            timing = StageTimings()
            with timing.stage("parse main"):
                pass
            self.assertIn("parse main", output.getvalue())
            with timing.stage("typecheck"):
                with timing.stage("scope"), timing.stage("parse"):
                    pass
                self.assertNotIn("  scope", output.getvalue())
            self.assertIn("  scope", output.getvalue())
            self.assertIn("  typecheck", output.getvalue())
            with timing.stage("llvm pass"):
                timing.transition("jit link")
                self.assertIn("  llvm pass", output.getvalue())
                timing.transition("run")
                self.assertIn("  jit link", output.getvalue())
            self.assertIn("  run", output.getvalue())
        self.assertEqual(timing.stack, [])


if __name__ == "__main__":
    unittest.main()
