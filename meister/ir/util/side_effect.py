# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from enum import IntEnum


class SideEffectStatus(IntEnum):
    """C++: codon/cir/util/side_effect.h:12.

    Pure functions return the same output for the same inputs without effects.
    No-side-effect functions can return different outputs (e.g. time()).
    Capturing arguments is an effect. The implementation relies on the subset
    order pure < no_side_effect < no_capture < unknown.
    """

    PURE = 0
    NO_SIDE_EFFECT = 1
    NO_CAPTURE = 2
    UNKNOWN = 3
