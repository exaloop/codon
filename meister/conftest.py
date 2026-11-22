"""Fixtures shared by the native and Meister compiler integration tests."""

from pathlib import Path
import shutil
import sys

import pytest

from .ir.toolchain import llvm_path, runtime_path
from .tests.cases import ROOT
from .tests.support import build_directory, native_binary, native_cases


def pytest_addoption(parser):
    group = parser.getgroup("codon")
    group.addoption("--codon-build", type=Path, help="directory containing built Codon artifacts")
    group.addoption("--llvm-library", type=Path, help="shared LLVM library used by Meister")
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
def codon_runtime(pytestconfig):
    build = pytestconfig.getoption("--codon-build")
    try:
        return runtime_path(build)
    except FileNotFoundError as error:
        pytest.skip(str(error))


@pytest.fixture(scope="session")
def llvm_library(pytestconfig, codon_runtime):
    try:
        return llvm_path(pytestconfig.getoption("--llvm-library"), build=codon_runtime.parent)
    except FileNotFoundError as error:
        pytest.skip(str(error))


@pytest.fixture
def codon_workdir(tmp_path, codon_runtime):
    """Make fixture input visible while isolating all relative output paths."""
    (tmp_path / "build").mkdir()
    if sys.platform == "win32":
        # The file tests use test/stdlib/data for read-only fixture data.
        shutil.copytree(ROOT / "test/stdlib/data", tmp_path / "test/stdlib/data")
    else:
        (tmp_path / "test").symlink_to(ROOT / "test", target_is_directory=True)
        (tmp_path / "stdlib").symlink_to(ROOT / "stdlib", target_is_directory=True)
        for filename in ("libcodonrt.so", "libcodonrt.dylib"):
            target = codon_runtime.parent / filename
            if target.exists():
                (tmp_path / filename).symlink_to(target)
    return tmp_path
