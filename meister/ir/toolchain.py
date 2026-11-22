# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Locate LLVM and the Codon runtime independently of the native compiler."""

import ctypes.util
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD_NAMES = ("build", "build-rel", "build-linux", "build-rel-linux")
SUFFIX = ".dll" if sys.platform == "win32" else ".dylib" if sys.platform == "darwin" else ".so"
RUNTIME_NAME = f"libcodonrt{SUFFIX}"


def runtime_path(requested=None):
    if requested := requested or os.environ.get("MEISTER_RUNTIME"):
        path = Path(requested).expanduser().absolute()
        if path.is_dir():
            path /= RUNTIME_NAME
        if path.is_file():
            return path
        raise FileNotFoundError(f"Codon runtime not found at {path}")
    for directory in (
        *(ROOT / name for name in BUILD_NAMES),
        ROOT / "lib/codon",
        Path(sys.prefix) / "lib/codon",
    ):
        path = directory / RUNTIME_NAME
        if path.is_file():
            return path
    raise FileNotFoundError(f"Cannot find {RUNTIME_NAME}. Set MEISTER_RUNTIME or pass --runtime.")


def llvm_path(requested=None, *, build=None):
    if requested := requested or os.environ.get("MEISTER_LLVM_LIBRARY"):
        return Path(requested).expanduser().resolve(strict=True)
    filename = f"libLLVM{SUFFIX}" if sys.platform != "win32" else "LLVM-C.dll"
    directories = [Path(sys.prefix) / "lib"]
    for directory in (build, *(ROOT / name for name in BUILD_NAMES)):
        if directory is None:
            continue
        cache = directory / "CMakeCache.txt"
        if cache.is_file():
            for line in cache.read_text(encoding="utf-8").splitlines():
                if line.startswith("LLVM_DIR:"):
                    config = Path(line.partition("=")[2])
                    directories.extend((config.parent.parent, config.parent.parent.parent / "bin"))
                    break
    for directory in directories:
        if (path := directory / filename).is_file():
            return path
    if library := ctypes.util.find_library("LLVM-20"):
        return library
    raise FileNotFoundError("Cannot find shared LLVM 20. Set MEISTER_LLVM_LIBRARY or pass --library.")
