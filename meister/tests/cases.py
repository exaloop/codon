"""The file suites shared with GoogleTest, and their individual #%% cases."""

from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from ..test import TestCase, split_test_file


ROOT = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class Fixture:
    suite: str
    case: TestCase
    debug: bool
    pynum: bool
    run: bool

    @property
    def id(self):
        name = self.case.path.relative_to(ROOT / "test").as_posix()
        if self.case.code is not None:
            name += f":{self.case.line + 1}:{self.case.name}"
        mode = "debug" if self.debug else "release"
        numerics = "python" if self.pynum else "c"
        return f"{name}-{mode}-{numerics}"


def fixtures():
    manifest = ROOT / "test/fixtures.tsv"
    for line in manifest.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        suite, path = line.split("\t")
        path = ROOT / "test" / path
        if suite == "TypeTests" and path.stat().st_size == 0:
            continue
        cases = split_test_file(path) if suite == "TypeTests" else [TestCase(path)]
        for case in cases:
            if case.name == "__ignore__":
                continue
            for debug in (True, False):
                yield Fixture(
                    suite, case, debug,
                    pynum=suite not in {"TypeTests", "CNumericsTests"},
                    run=suite != "GpuTests",
                )


def expectations(case):
    source = case.code if case.code is not None else case.path.read_text(encoding="utf-8")
    lines = []
    rejected = False
    for line in source.split("\n"):
        for marker in ("# EXPECT: ", "#: ", "#! "):
            _, found, expected = line.partition(marker)
            if found:
                if expected:
                    lines.append(expected)
                    rejected |= marker == "#! "
                break
    return lines, rejected


@lru_cache
def meister_diagnostics():
    """Expected prefixes for named cases whose diagnostics differ from GoogleTest."""
    diagnostics = {}
    path = Path(__file__).with_name("diagnostics.tsv")
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            file, case, message = line.split("\t", 2)
            diagnostics.setdefault((ROOT / "test" / file, case), []).append(message)
    return diagnostics
