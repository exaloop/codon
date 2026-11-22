"""Fixtures shared by the native and Meister compiler integration tests."""

from pathlib import Path
import shutil
import sys

import pytest

from .tests.cases import ROOT
from .tests.support import build_directory, native_binary, native_cases


def pytest_addoption(parser):
    group = parser.getgroup("codon")
    group.addoption("--codon-build", type=Path, help="directory containing built Codon artifacts")
    group.addoption("--codon-timeout", type=float, help="optional time limit in seconds per case")


@pytest.hookimpl(optionalhook=True)
def pytest_configure_node(node):
    # List GoogleTest once and use the same order on every xdist worker.
    node.workerinput["codon_native_cases"] = native_cases(native_binary(node.config))


def pytest_generate_tests(metafunc):
    if "native_case" in metafunc.fixturenames:
        config = metafunc.config
        if hasattr(config, "workerinput"):
            cases = config.workerinput["codon_native_cases"]
        else:
            cases = native_cases(native_binary(config))
        metafunc.parametrize("native_case", cases or [
            pytest.param(None, marks=pytest.mark.skip(reason="build the codon_test CMake target"))
        ])


@pytest.fixture(scope="session")
def codon_library(pytestconfig):
    build = build_directory(pytestconfig)
    for filename in ("libcodonc.so", "libcodonc.dylib", "libcodonc.dll", "codonc.dll"):
        library = build / filename
        if library.is_file():
            return library
    pytest.skip(f"build the codonc CMake target in {build}")


@pytest.fixture
def codon_workdir(tmp_path, codon_library):
    """Make fixture input visible while isolating all relative output paths."""
    (tmp_path / "build").mkdir()
    if sys.platform == "win32":
        # The file tests use test/stdlib/data for read-only fixture data.
        shutil.copytree(ROOT / "test/stdlib/data", tmp_path / "test/stdlib/data")
    else:
        (tmp_path / "test").symlink_to(ROOT / "test", target_is_directory=True)
        (tmp_path / "stdlib").symlink_to(ROOT / "stdlib", target_is_directory=True)
        for filename in ("libcodonrt.so", "libcodonrt.dylib"):
            target = codon_library.parent / filename
            if target.exists():
                (tmp_path / filename).symlink_to(target)
    return tmp_path
