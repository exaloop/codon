# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import dataclass
from .value import Value


@dataclass(init=False, eq=False)
class Const(Value):
    """C++: codon/cir/const.h:12. Constants are immutable once created."""

    _type: object

    # C++: codon/cir/const.h:23
    def __init__(self, typ, name=""):
        """
        Constructs a constant.
        @param type the type
        @param name the name
        """
        super().__init__(name)
        self._type = typ

    # C++: codon/cir/const.h:27
    def _get_type(self):
        return self._type

    # C++: codon/cir/const.h:29
    def _get_used_types(self):
        return [self._type]

    # C++: codon/cir/const.cpp:12
    def _replace_used_type(self, name, new):
        if self._type.name == name:
            self._type = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class TemplatedConst(Const):
    """C++: codon/cir/const.h:34. Constant carrying an internal scalar value."""

    val: object

    # C++: codon/cir/const.h:45
    def __init__(self, value, typ, name=""):
        super().__init__(typ, name)
        self.val = value


@dataclass(init=False, eq=False)
class IntConst(TemplatedConst):
    """C++: const.h:61. Signed 64-bit integer constant."""


@dataclass(init=False, eq=False)
class FloatConst(TemplatedConst):
    """C++: const.h:62. Double precision constant."""


@dataclass(init=False, eq=False)
class BoolConst(TemplatedConst):
    """C++: const.h:63. Boolean constant."""


@dataclass(init=False, eq=False)
class StringConst(TemplatedConst):
    """C++: codon/cir/const.h:79. String constant."""


@dataclass(init=False, eq=False)
class BytesConst(TemplatedConst):
    """C++: codon/cir/const.h:86. Bytes constant (raw string storage, as in C++)."""
