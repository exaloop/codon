# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from enum import IntEnum

from ...ast.types import mangle


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


# C++: codon/cir/util/side_effect.cpp. These are the frontend's mangled attributes.
NON_PURE_ATTR = mangle("std.internal.attributes", func="nonpure")
PURE_ATTR = mangle("", func="pure")
NO_SIDE_EFFECT_ATTR = mangle("std.internal.attributes", func="no_side_effect")
NO_CAPTURE_ATTR = mangle("std.internal.attributes", func="nocapture")
DERIVES_ATTR = mangle("", func="derives")
SELF_CAPTURES_ATTR = mangle("std.internal.attributes", func="self_captures")
