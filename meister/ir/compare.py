# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Dump and fingerprint Meister's compilation stages for comparison."""

import hashlib

from .. import ast
from ..tests.cases import split_test_file


def dump_typed(cc, node, indent):
    # Preserve the exact forest bytes without retaining every function's dump.
    yield ast.dump(node, include_attributes=True, indent=indent, types=True)
    yield "\n"
    for _, fn in sorted(cc.functions.items()):
        for _, real in sorted(fn.realizations.items()):
            if real.ast:
                yield ast.dump(real.ast, include_attributes=True, indent=indent, types=True)
                yield "\n"


class DumpFingerprint:
    """C++: capi.cpp. Ordered SHA-256 chunks avoid LLVM 20's long-message bug."""

    CHUNK_BYTES = 64 * 1024 * 1024

    def __init__(self):
        self.hash = hashlib.sha256()
        self.size, self.offset = 0, 0
        self.chunks = []

    def update(self, data):
        self.size += len(data)
        remaining = memoryview(data)
        while remaining:
            size = min(self.CHUNK_BYTES - self.offset, len(remaining))
            self.hash.update(remaining[:size])
            remaining = remaining[size:]
            self.offset += size
            if self.offset == self.CHUNK_BYTES:
                self.commit()

    def commit(self):
        self.chunks.append(self.hash.hexdigest())
        self.hash = hashlib.sha256()
        self.offset = 0

    def finish(self):
        if self.offset or not self.chunks:
            self.commit()
        return {"bytes": self.size, "sha256_chunks": self.chunks}


def signature(data):
    digest = DumpFingerprint()
    digest.update(data)
    return digest.finish()


def collect_cases(path):
    """Collect a file or all immediate .codon files, regardless of naming prefix."""
    files = sorted(path.glob("*.codon")) if path.is_dir() else [path]
    return [case for filename in files for case in split_test_file(filename)]
