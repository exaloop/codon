"""Expose the native-only CIR, LLVM, DSL, and JIT assertions as individual cases."""

import re

import pytest

from .cases import ROOT
from .support import compiler_environment, native_binary, run_process


@pytest.mark.native
def test_native_assertions(native_case, pytestconfig):
    binary = native_binary(pytestconfig)
    result = run_process(
        [binary, f"--gtest_filter={native_case}"], cwd=ROOT,
        env=compiler_environment(binary.parent), capture_output=True, text=True,
        errors="backslashreplace", timeout=pytestconfig.getoption("--codon-timeout"),
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert re.search(r"\[ (?:      OK| SKIPPED) \]\s+" + re.escape(native_case), output), output
    if "[  SKIPPED ] " + native_case in output:
        pytest.skip(f"{native_case} requires a different target or configuration")
