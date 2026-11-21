# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ... import CallInstr, cast
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, make_tuple
from ..pass_ import OperatorPass


def inspect_addition(value, accepts, args):
    if not accepts(value):
        return False
    if inner := cast(CallInstr, value):
        fn = get_func(inner.callee)
        if (
            fn and fn.unmangled_name == "__add__" and len(inner.args) == 2
            and all(accepts(arg) for arg in inner)
        ):
            return all(inspect_addition(arg, accepts, args) for arg in inner)
    args.append(value)
    return True


class StrAdditionOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/str.cpp."""

    KEY = "core-pythonic-str-addition-opt"

    def handle_CallInstr(self, value):
        fn = get_func(value.callee)
        if not fn or fn.unmangled_name != "__add__":
            return
        module = value.module
        string = module.get_string_type()
        args = []
        if inspect_addition(value, lambda operand: operand.type.is_type(string), args) and len(args) > 2:
            clone = CloneVisitor(module)
            pieces = make_tuple([clone.clone(arg) for arg in args], module)
            replacement = module.get_or_realize_method(string, "cat", [pieces.type])
            assert replacement, f"could not find cat function [{value.info}]"
            value.replace_all(call(replacement, [pieces]))
