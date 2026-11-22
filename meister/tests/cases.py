"""The file suites shared with GoogleTest, and their individual #%% cases."""

from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class TestCase:
    def __init__(self, path, name="", code=None, line=0, barebones=False):
        self.path = path
        self.name = name
        self.code = code
        self.line = line
        self.barebones = barebones

    @property
    def label(self):
        if self.code is None:
            return str(self.path)
        name = self.name or f"line-{self.line + 1}"
        return f"{self.path}:{self.line + 1}:{name}"


def split_test_file(path):
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    if not any(line.startswith("#%%") for line in lines):
        return [TestCase(path)]

    cases = []
    code = []
    name = ""
    start_line = 0
    saw_marker = False
    barebones = False

    for line_number, line in enumerate(lines):
        if line.startswith("#%%"):
            if saw_marker:
                cases.append(TestCase(path, name, "".join(code), start_line, barebones))
            elif not saw_marker and code:
                cases.append(TestCase(path, "", "".join(code), start_line))

            metadata = line[4:].split(",")
            name = metadata[0] if metadata else ""
            # C++: test/main.cpp:405 selects the stdlib from this marker.
            barebones = len(metadata) > 1 and metadata[1] == "barebones"
            code = [line + "\n"]
            start_line = line_number
            saw_marker = True
        else:
            code.append(line + "\n")

    if code:
        cases.append(TestCase(path, name, "".join(code), start_line, barebones))
    return cases


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
