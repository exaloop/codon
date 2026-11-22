"""Execute each registered Codon input with the Meister pipeline in both modes."""

import sys

import pytest

from .cases import ROOT, expectations, fixtures, meister_diagnostics
from .support import compiler_environment, run_process


def output_lines(text):
    # GoogleTest reads through LF, allowing CRLF without stripping other whitespace.
    if not text:
        return []
    return [line.removesuffix("\r") for line in text.removesuffix("\n").split("\n")]


def fixture_parameters():
    for fixture in fixtures():
        uses_numpy = (
            fixture.suite == "NumPyTests"
            or fixture.case.path == ROOT / "test/python/pybridge.codon"
        )
        marks = [pytest.mark.numpy] if uses_numpy else []
        yield pytest.param(fixture, id=fixture.id, marks=marks)


@pytest.mark.compiler
@pytest.mark.parametrize("fixture", list(fixture_parameters()))
def test_codon_fixture(fixture, codon_library, codon_workdir, pytestconfig):
    case = fixture.case
    expected, rejected = expectations(case)
    if rejected:
        expected = meister_diagnostics().get((case.path, case.name), expected)
    command = [
        sys.executable, "-m", "meister.tests.runner", str(case.path), "--library", str(codon_library)
    ]
    if case.code is not None:
        command += ["--section", str(case.line)]
    if not fixture.debug:
        command.append("--release")
    if not fixture.pynum:
        command.append("--c-numerics")
    if not fixture.run:
        command.append("--compile-only")
    if fixture.suite == "OptTests" and case.path.stem in {"escapes", "inlining", "outlining"}:
        command.append("--fixture-passes")
    result = run_process(
        command, cwd=codon_workdir, env=compiler_environment(codon_library.parent),
        capture_output=True, timeout=pytestconfig.getoption("--codon-timeout"),
    )
    stdout = result.stdout.decode("utf-8", errors="surrogateescape")
    stderr = result.stderr.decode("utf-8", errors="backslashreplace")
    diagnostics = f"exit status {result.returncode}\nstdout:\n{stdout}\nstderr:\n{stderr}"
    assert result.returncode == int(rejected), diagnostics
    assert "TEST FAILED" not in stdout, diagnostics
    if expected:
        lines = output_lines(stdout)
        if rejected:
            assert len(lines) == len(expected), diagnostics
            assert all(line.startswith(prefix) for line, prefix in zip(lines, expected)), (
                f"expected diagnostic prefixes {expected!r}\n{diagnostics}"
            )
        else:
            assert lines == expected, diagnostics
