# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

import math
import operator
from dataclasses import dataclass

from ... import BoolConst, CallInstr, FloatConst, FlowInstr, IntConst, SeriesFlow, cast
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func, is_call_of
from ..pass_ import OperatorPass


# C++: codon/cir/transform/folding/const_fold.cpp.
_COMPARISONS = {
    "__eq__": operator.eq,
    "__ne__": operator.ne,
    "__gt__": operator.gt,
    "__ge__": operator.ge,
    "__lt__": operator.lt,
    "__le__": operator.le,
}


def signed_int(value):
    return (value + (1 << 63)) % (1 << 64) - (1 << 63)


def c_div(left, right):
    quotient = abs(left) // abs(right)
    return -quotient if (left < 0) != (right < 0) else quotient


def float_div(left, right):
    if right != 0.0:
        return left / right
    if math.isnan(left):
        return left
    if left == 0.0:
        # Use the host's invalid-operation NaN, including its sign bit.
        return math.inf * 0.0
    sign = math.copysign(1.0, left) * math.copysign(1.0, right)
    return math.copysign(math.inf, sign)


def float_pow(left, right):
    try:
        return math.pow(left, right)
    except ValueError:
        if left != 0.0 or right >= 0.0:
            return math.inf * 0.0
        sign = -1.0 if math.copysign(1.0, left) < 0 and right % 2 == 1 else 1.0
        return math.copysign(math.inf, sign)
    except OverflowError:
        return -math.inf if left < 0 and right % 2 == 1 else math.inf


_INT_ARITHMETIC = {
    "__add__": operator.add,
    "__sub__": operator.sub,
    "__mul__": operator.mul,
    "__lshift__": lambda a, b: a << (b & 63),
    "__rshift__": lambda a, b: a >> (b & 63),
    "__pow__": lambda a, b: 0 if b < 0 else pow(a, b, 1 << 64),
    "__xor__": operator.xor,
    "__or__": operator.or_,
    "__and__": operator.and_,
}

_FLOAT_ARITHMETIC = {
    "__add__": operator.add,
    "__sub__": operator.sub,
    "__mul__": operator.mul,
    "__pow__": float_pow,
    "__truediv__": float_div,
    "cdiv": float_div,
}

_BOOL_ARITHMETIC = {
    "__xor__": operator.xor,
    "__or__": operator.or_,
    "__and__": operator.and_,
}


def constant(module, source, typ, value):
    if typ.is_type(module.get_int_type()):
        return module.n(IntConst, source, signed_int(value), typ)
    if typ.is_type(module.get_float_type()):
        return module.n(FloatConst, source, float(value), typ)
    if typ.is_type(module.get_bool_type()):
        return module.n(BoolConst, source, bool(value), typ)
    return None


@dataclass(init=False, eq=False)
class FoldingPass(OperatorPass):
    """Fold the native release pipeline's numeric operations and conversions."""

    KEY = "core-folding-const-fold"

    num_replacements: int

    def __init__(self):
        super().__init__(children_first=True)
        self.num_replacements = 0

    def run(self, module):
        self.num_replacements = 0
        super().run(module)

    def handle_CallInstr(self, value):
        fn = get_func(value.callee)
        if not fn or not is_call_of(value, fn.unmangled_name, len(value.args), method=True):
            return
        name = fn.unmangled_name
        if len(value.args) == 1:
            replacement = self.fold_unary(value, name)
        elif len(value.args) == 2:
            replacement = self.fold_binary(value, name)
        else:
            return
        if replacement is not None:
            self.num_replacements += 1
            value.replace_all(replacement)

    def fold_unary(self, value, name):
        module = value.module
        arg = value.front().get_actual()
        in_type, out_type = arg.type, value.type
        integer, floating, boolean = (
            module.get_int_type(), module.get_float_type(), module.get_bool_type()
        )
        if in_type.is_type(integer) and out_type.is_type(integer):
            if name == "__pos__":
                return arg
            if name in ("__neg__", "__invert__") and is_call_of(
                arg, name, [integer], integer, method=True
            ):
                return cast(CallInstr, arg).front()
        conversions = {"__int__": integer, "__float__": floating, "__bool__": boolean}
        if name in conversions and out_type.is_type(conversions[name]):
            if in_type.is_type(out_type):
                return CloneVisitor(module).clone(arg)
            if (
                isinstance(arg, (IntConst, FloatConst, BoolConst))
                and any(in_type.is_type(typ) for typ in (integer, floating, boolean))
            ):
                if isinstance(arg, FloatConst) and name == "__int__":
                    # The signed double-to-i64 conversion yields the minimum
                    # value when the input cannot be represented on the host.
                    number = arg.val
                    converted = int(number) if -(1 << 63) <= number < (1 << 63) else -(1 << 63)
                else:
                    converted = arg.val
                return constant(module, value, out_type, converted)
        if in_type.is_type(out_type):
            if name == "__neg__" and (
                (isinstance(arg, IntConst) and in_type.is_type(integer))
                or (isinstance(arg, FloatConst) and in_type.is_type(floating))
            ):
                return constant(module, value, out_type, -arg.val)
            if name == "__pos__" and isinstance(arg, FloatConst) and in_type.is_type(floating):
                return constant(module, value, out_type, +arg.val)
            if name == "__invert__":
                if isinstance(arg, IntConst) and in_type.is_type(integer):
                    return constant(module, value, out_type, ~arg.val)
                if isinstance(arg, BoolConst) and in_type.is_type(boolean):
                    return constant(module, value, out_type, not arg.val)
        return None

    def fold_binary(self, value, name):
        module = value.module
        left, right = (arg.get_actual() for arg in value)
        integer, floating, boolean = (
            module.get_int_type(), module.get_float_type(), module.get_bool_type()
        )
        pynum = self.get_options().pynum
        out = value.type
        integer_args = left.type.is_type(integer) and right.type.is_type(integer)

        # The floor-div-by-one rule precedes double-constant floor division.
        if integer_args and out.is_type(integer) and name == "__floordiv__":
            if isinstance(right, IntConst) and right.val == 1:
                return CloneVisitor(module).clone(left)

        both_int = integer_args and isinstance(left, IntConst) and isinstance(right, IntConst)
        both_float = (
            isinstance(left, FloatConst) and isinstance(right, FloatConst)
            and left.type.is_type(floating) and right.type.is_type(floating)
        )
        both_bool = (
            isinstance(left, BoolConst) and isinstance(right, BoolConst)
            and left.type.is_type(boolean) and right.type.is_type(boolean)
        )
        mixed_numeric = (
            isinstance(left, IntConst) and isinstance(right, FloatConst)
            or isinstance(left, FloatConst) and isinstance(right, IntConst)
        )
        if both_int and out.is_type(integer):
            if calculation := _INT_ARITHMETIC.get(name):
                return constant(module, value, out, calculation(left.val, right.val))
            if right.val and name in ("__floordiv__", "cdiv", "__mod__", "cmod"):
                a, b = left.val, right.val
                if a == -(1 << 63) and b == -1:
                    return None
                if pynum and name in ("__floordiv__", "__mod__"):
                    quotient, remainder = divmod(a, b)
                else:
                    quotient = c_div(a, b)
                    remainder = a - quotient * b
                return constant(
                    module, value, out,
                    quotient if name in ("__floordiv__", "cdiv") else remainder,
                )
        if both_float and out.is_type(floating):
            if not (pynum and name == "__truediv__" and right.val == 0.0):
                if calculation := _FLOAT_ARITHMETIC.get(name):
                    return constant(module, value, out, calculation(left.val, right.val))
        if (both_int or both_float or mixed_numeric) and out.is_type(boolean):
            if calculation := _COMPARISONS.get(name):
                a, b = (float(left.val), float(right.val)) if mixed_numeric else (left.val, right.val)
                return constant(module, value, boolean, calculation(a, b))
        if both_bool and out.is_type(boolean) and (calculation := _BOOL_ARITHMETIC.get(name)):
            return constant(module, value, boolean, calculation(left.val, right.val))
        if mixed_numeric and name in ("__add__", "__sub__", "__truediv__", "__mul__"):
            if not (pynum and name == "__truediv__" and right.val == 0):
                return constant(
                    module, value, floating, _FLOAT_ARITHMETIC[name](float(left.val), float(right.val))
                )
        if integer_args and out.is_type(integer):
            return self.simplify_integer_binary(name, left, right)
        return None

    @staticmethod
    def simplify_integer_binary(name, left, right):
        if name == "__mul__":
            for operand, other in ((left, right), (right, left)):
                if isinstance(operand, IntConst) and operand.val == 1:
                    return CloneVisitor(left.module).clone(other)
            for operand, other in ((left, right), (right, left)):
                if isinstance(operand, IntConst) and operand.val == 0:
                    # Evaluate the other operand even though the result is zero.
                    module = other.module
                    zero = module.n(IntConst, other, 0, module.get_int_type())
                    flow = module.n(SeriesFlow, other)
                    flow.push_back(other)
                    return module.n(FlowInstr, other, flow, zero)
        if name == "__add__" and isinstance(left, IntConst) and left.val == 0:
            return CloneVisitor(left.module).clone(right)
        if name in ("__add__", "__sub__", "cdiv") and isinstance(right, IntConst):
            if right.val == (1 if name == "cdiv" else 0):
                return CloneVisitor(left.module).clone(left)
        return None
