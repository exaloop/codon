import unittest

from run_tests import THREAD_LIMITS, filter_listing, phase_environment


class RunnerTests(unittest.TestCase):
    def test_partition_preserves_names_and_parameter_comments(self):
        listing = """Note: Google Test filter = *
Core/SeqTest.
  Run/core_bltin_codon_debug  # GetParam() = fixture
  Run/transform_omp_codon
  Run/stdlib_asyncio_test_codon
Typed/0.  # TypeParam = int
  Example
"""
        parallel = filter_listing(listing, False)
        serial = filter_listing(listing, True)
        self.assertIn("Run/core_bltin_codon_debug  # GetParam() = fixture", parallel)
        self.assertIn("Typed/0.  # TypeParam = int\n  Example", parallel)
        self.assertNotIn("transform_omp", parallel)
        self.assertNotIn("asyncio", parallel)
        self.assertNotIn("core_bltin", serial)
        self.assertNotIn("Typed/0", serial)
        self.assertIn("Run/transform_omp_codon", serial)
        self.assertIn("Run/stdlib_asyncio_test_codon", serial)
        self.assertEqual(filter_listing("", True), "\n")

    def test_thread_limits_do_not_leak_into_serial_phase(self):
        original = {"OMP_NUM_THREADS": "8", "OTHER": "preserved"}
        parallel = phase_environment(original, False)
        serial = phase_environment(original, True)
        for name in THREAD_LIMITS:
            self.assertEqual(parallel[name], "1")
        self.assertEqual(serial["OMP_NUM_THREADS"], "8")
        self.assertNotIn("OPENBLAS_NUM_THREADS", serial)
        self.assertEqual(serial["OTHER"], "preserved")
        self.assertEqual(original, {"OMP_NUM_THREADS": "8", "OTHER": "preserved"})


if __name__ == "__main__":
    unittest.main()
