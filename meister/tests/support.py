"""Process and build helpers for compiler integration tests."""

from functools import lru_cache
import errno
import os
from pathlib import Path
import subprocess
import sys
import sysconfig
import time

from ..test import library_candidates
from .cases import ROOT


def build_directory(config):
    override = config.getoption("--codon-build")
    if override:
        return override.resolve()
    if library := os.environ.get("CODON_LIBRARY"):
        return Path(library).resolve().parent
    library = next((path for path in library_candidates(ROOT) if path.is_file()), None)
    return library.parent if library else ROOT / "build"


def native_binary(config):
    return build_directory(config) / ("codon_test.exe" if sys.platform == "win32" else "codon_test")


def compiler_environment(build):
    env = os.environ.copy()
    env.setdefault("CODON_PATH", str(ROOT / "stdlib"))
    env.setdefault("OPENBLAS_NUM_THREADS", "1")
    env.setdefault("GC_MARKERS", "1")
    libdir = sysconfig.get_config_var("LIBDIR")
    libname = sysconfig.get_config_var("LDLIBRARY")
    if sys.platform == "win32":
        dll = f"python{sys.version_info.major}{sys.version_info.minor}.dll"
        env.setdefault("CODON_PYTHON", str(Path(sys.base_prefix) / dll))
    elif libdir and libname:
        env.setdefault("CODON_PYTHON", str(Path(libdir) / libname))
    for key, paths in (
        ("PYTHONPATH", (str(ROOT), str(ROOT / "test/python"))),
        ("PATH" if sys.platform == "win32" else "LD_LIBRARY_PATH", (str(build),)),
    ):
        env[key] = os.pathsep.join((*paths, *filter(None, (env.get(key),))))
    for key in ("GTEST_TOTAL_SHARDS", "GTEST_SHARD_INDEX", "GTEST_FILTER", "GTEST_OUTPUT"):
        env.pop(key, None)
    return env


def run_process(command, *, timeout, **kwargs):
    """Wait for a process slot when the host process quota is temporarily full."""
    deadline = time.monotonic() + (60 if timeout is None else timeout)
    while True:
        try:
            remaining = None if timeout is None else max(0, deadline - time.monotonic())
            return subprocess.run(command, timeout=remaining, **kwargs)
        except OSError as error:
            if error.errno != errno.EAGAIN or time.monotonic() >= deadline:
                raise
            time.sleep(0.1)


@lru_cache
def native_cases(binary):
    if not binary.exists():
        return []
    listing = run_process(
        [binary, "--gtest_list_tests"], cwd=ROOT, env=compiler_environment(binary.parent),
        capture_output=True, text=True, check=True, timeout=60,
    ).stdout
    suite = ""
    cases = []
    for line in listing.splitlines():
        if not line.strip():
            continue
        name = line.split()[0]
        if line.startswith("  ") and suite.endswith(".") and "/SeqTest." not in suite:
            cases.append(suite + name)
        elif not line.startswith(" "):
            suite = name
    return cases
