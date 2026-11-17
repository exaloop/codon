# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ....bridge import dataclass
from ...util.irtools import call
from ..pass_ import OperatorPass


@dataclass(init=False, eq=False)
class ThrowLowering(OperatorPass):
    """C++: codon/cir/transform/lowering/throw.h:12. Resolve the no-active-exception fallback for bare throw."""

    KEY = "core-throw-lowering"

    # C++: codon/cir/transform/lowering/throw.cpp:14
    def handle_ThrowInstr(self, value):
        if value.value or value.fallback:
            return
        fallback = value.module.get_or_realize_func(
            "_raise_no_active_exception", [], [], "std.internal.types.error"
        )
        assert fallback, "empty-raise fallback not found"
        value.fallback = call(fallback, [])
