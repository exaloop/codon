# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import dataclass
from .base import IdMixin, ReplaceableNode


@dataclass(init=False, eq=False)
class Value(ReplaceableNode, IdMixin):
    """C++: codon/cir/value.h:14, value.cpp. A typed CIR value."""

    # C++: codon/cir/value.h:20
    def __init__(self, name=""):
        """
        Constructs a value.
        @param the value's name
        """
        ReplaceableNode.__init__(self, name)
        IdMixin.__init__(self)

    # C++: codon/cir/value.h:24
    def reference_string(self):
        return f"{self.name}.{self.id}"

    # C++: codon/cir/value.h:59
    @property
    def type(self):
        """
        @return the value's type
        """
        return self.get_actual()._get_type()

    # C++: codon/cir/value.h:59
    def _get_type(self):
        """
        @return the value's type
        """
        raise NotImplementedError

    # C++: codon/cir/value.cpp:119
    def _unary_op(self, name):
        from .var import VarValue

        fn = self.module.get_or_realize_method(self.type, name, [self.type])
        return self.module.nr(VarValue, fn)(self) if fn else None

    # C++: codon/cir/value.cpp:131
    def _binary_op(self, name, other):
        from .var import VarValue

        fn = self.module.get_or_realize_method(self.type, name, [self.type, other.type])
        return self.module.nr(VarValue, fn)(self, other) if fn else None

    # Dereferenced C++ Value comparisons build IR. Python node equality must
    # retain pointer-identity semantics for maps, lists and replacement tracking.
    # C++: codon/cir/value.cpp:13
    def eq(self, other):
        return self._binary_op(self.module.EQ_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:17
    def ne(self, other):
        return self._binary_op(self.module.NE_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:21
    def __lt__(self, other):
        return self._binary_op(self.module.LT_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:25
    def __gt__(self, other):
        return self._binary_op(self.module.GT_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:29
    def __le__(self, other):
        return self._binary_op(self.module.LE_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:33
    def __ge__(self, other):
        return self._binary_op(self.module.GE_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:37
    def __pos__(self):
        return self._unary_op(self.module.POS_MAGIC_NAME)

    # C++: codon/cir/value.cpp:39
    def __neg__(self):
        return self._unary_op(self.module.NEG_MAGIC_NAME)

    # C++: codon/cir/value.cpp:41
    def __invert__(self):
        return self._unary_op(self.module.INVERT_MAGIC_NAME)

    # C++: codon/cir/value.cpp:37
    def __add__(self, other):
        return self._binary_op(self.module.ADD_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:39
    def __sub__(self, other):
        return self._binary_op(self.module.SUB_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:51
    def __mul__(self, other):
        return self._binary_op(self.module.MUL_MAGIC_NAME, other)

    # C++ API: codon/cir/value.h:14
    def __matmul__(self, other):
        return self._binary_op(self.module.MATMUL_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:59
    def __truediv__(self, other):
        return self._binary_op(self.module.TRUE_DIV_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:63
    def __floordiv__(self, other):
        return self._binary_op(self.module.FLOOR_DIV_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:67
    def __mod__(self, other):
        return self._binary_op(self.module.MOD_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:71
    def __pow__(self, other):
        return self._binary_op(self.module.POW_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:73
    def __lshift__(self, other):
        return self._binary_op(self.module.LSHIFT_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:77
    def __rshift__(self, other):
        return self._binary_op(self.module.RSHIFT_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:81
    def __and__(self, other):
        return self._binary_op(self.module.AND_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:85
    def __or__(self, other):
        return self._binary_op(self.module.OR_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:89
    def __xor__(self, other):
        return self._binary_op(self.module.XOR_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:93
    def logical_or(self, other):
        from .instr import TernaryInstr

        return self.module.nr(
            TernaryInstr, self.to_bool(), self.module.get_bool(True), other.to_bool()
        )

    # C++: codon/cir/value.cpp:98
    def logical_and(self, other):
        from .instr import TernaryInstr

        return self.module.nr(
            TernaryInstr, self.to_bool(), other.to_bool(), self.module.get_bool(False)
        )

    # C++: codon/cir/value.h:91
    def __call__(self, *args):
        from .instr import CallInstr

        return self.module.nr(CallInstr, self, list(args))

    # C++: codon/cir/value.cpp:103
    def __getitem__(self, other):
        return self._binary_op(self.module.GETITEM_MAGIC_NAME, other)

    # C++: codon/cir/value.cpp:107
    def to_int(self):
        return self._unary_op(self.module.INT_MAGIC_NAME)

    # C++: codon/cir/value.cpp:109
    def to_float(self):
        return self._unary_op(self.module.FLOAT_MAGIC_NAME)

    # C++: codon/cir/value.cpp:111
    def to_bool(self):
        return self._unary_op(self.module.BOOL_MAGIC_NAME)

    # C++: codon/cir/value.cpp:113
    def to_str(self):
        return self._unary_op(self.module.REPR_MAGIC_NAME)

    # C++: codon/cir/value.cpp:115
    def len(self):
        return self._unary_op(self.module.LEN_MAGIC_NAME)

    # C++: codon/cir/value.cpp:117
    def iter(self):
        return self._unary_op(self.module.ITER_MAGIC_NAME)
