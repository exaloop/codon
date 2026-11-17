# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ....bridge import dataclass
from ...base import cast
from ...const import IntConst


# C++: codon/cir/transform/parallel/schedule.cpp:16
def get_schedule_code(schedule="static", chunked=False, ordered=False, monotonic=False):
    """C++: schedule.cpp:17. Codes from LLVM OpenMP's enum sched_type (kmp.h)."""
    modifier = (1 << 29) if monotonic else (1 << 30)
    if schedule == "static":
        return (65 if ordered else 33) if chunked else (66 if ordered else 34)
    if schedule in ("dynamic", "guided", "runtime", "auto"):
        return (
            {"dynamic": 35, "guided": 36, "runtime": 37, "auto": 38}[schedule]
            + (32 if ordered else 0)
        ) | modifier
    return get_schedule_code()


# C++: codon/cir/transform/parallel/schedule.cpp:45
def null_if_neg(value):
    constant = cast(IntConst, value)
    return None if constant is not None and constant.val <= 0 else value


@dataclass(init=False, eq=False)
class OMPSched:
    """C++: codon/cir/transform/parallel/schedule.h:15, schedule.cpp:54."""

    code: int
    dynamic: bool
    threads: object | None
    chunk: object | None
    ordered: bool
    collapse: int
    gpu: bool

    # C++: codon/cir/transform/parallel/schedule.cpp:52
    def __init__(
        self, code=-1, dynamic=False, threads=None, chunk=None, ordered=False, collapse=0, gpu=False
    ):
        if isinstance(code, str):
            dynamic = code != "static" or ordered
            code = get_schedule_code(code, null_if_neg(chunk) is not None, ordered)
        self.code = get_schedule_code() if code < 0 else code
        self.dynamic, self.threads, self.chunk = dynamic, null_if_neg(threads), null_if_neg(chunk)
        self.ordered, self.collapse, self.gpu = ordered, collapse, gpu

    # C++: codon/cir/transform/parallel/schedule.cpp:66
    def get_used_values(self):
        return [value for value in (self.threads, self.chunk) if value is not None]

    # C++: codon/cir/transform/parallel/schedule.cpp:75
    def replace_used_value(self, identifier, new):
        count = 0
        for field in ("threads", "chunk"):
            value = getattr(self, field)
            if value is not None and value.id == identifier:
                setattr(self, field, new)
                count += 1
        return count
