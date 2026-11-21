# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass
from functools import lru_cache

from ... import CallInstr, Const, FloatConst, FlowInstr, IntConst, SeriesFlow, VarValue, cast
from ...base import Node
from ...util.irtools import _attribute_name, call, get_func, has_attribute, is_call_of
from ...util.matching import match
from ...util.operator import Operator
from ..pass_ import OperatorPass


_MASK = (1 << 64) - 1
_HASH_MULTIPLIER = 0xC6A4A7935BD1E995


@lru_cache(maxsize=8192)
def string_hash(text):
    """The 64-bit libstdc++ string hash used by the native operand ranker."""
    data = text.encode("utf-8", "surrogateescape")
    h = (0xC70F6907 ^ (len(data) * _HASH_MULTIPLIER)) & _MASK
    stop = len(data) // 8 * 8
    for index in range(0, stop, 8):
        word = int.from_bytes(data[index:index + 8], "little")
        word = word * _HASH_MULTIPLIER & _MASK
        word = (word ^ (word >> 47)) * _HASH_MULTIPLIER & _MASK
        h = (h ^ word) * _HASH_MULTIPLIER & _MASK
    if stop < len(data):
        h = (h ^ int.from_bytes(data[stop:], "little")) * _HASH_MULTIPLIER & _MASK
    h = (h ^ (h >> 47)) * _HASH_MULTIPLIER & _MASK
    return h ^ (h >> 47)


@dataclass(init=False, eq=False)
class NodeRanker(Operator):
    # Rank constants last, deeper nodes first, then use the native hash order.
    root: Node | None
    max_depth: int
    hash: int

    def __init__(self):
        super().__init__()
        self.root, self.max_depth, self.hash = None, 0, 0

    def pre_hook(self, node):
        if self.root is None:
            self.root = node
        self.max_depth = max(self.max_depth, self.depth())
        for dependency in (*node.get_used_variables(), *node.get_used_types()):
            # boost hash_combine, with the same 64-bit wraparound as native CIR.
            h = self.hash
            self.hash ^= (
                string_hash(dependency.name) + 0x9E3779B9 + (h << 6) + (h >> 2)
            ) & _MASK

    def rank(self):
        return (1 if cast(Const, self.root) else -1, -self.max_depth, self.hash)


def get_rank(node):
    ranker = NodeRanker()
    ranker.process(node)
    return ranker.rank()


def operator_has(fn, name):
    return fn is not None and has_attribute(fn, _attribute_name(name))


def get_op(value):
    call_ = cast(CallInstr, value)
    return get_func(call_.callee) if call_ else None


def canon_op_chain(value):
    fn = get_func(value.callee)
    if not fn:
        return None
    op, typ = fn.unmangled_name, value.type
    if not is_call_of(value, op, [typ, typ], typ, method=True):
        return None

    operands = []
    if operator_has(fn, "associative"):
        def extract(operand):
            if is_call_of(operand, op, [typ, typ], typ, method=True):
                for arg in cast(CallInstr, operand):
                    extract(arg)
            else:
                operands.append(operand)

        extract(value)
    else:
        operands.extend(value)
    if operator_has(fn, "commutative"):
        operands.sort(key=lambda operand: (get_rank(operand), operand.id))

    left = value
    remaining = len(operands)
    while remaining > 1:
        call_ = cast(CallInstr, left)
        if (
            not call_ or len(call_.args) != 2 or get_func(call_.callee) is not fn
            or call_.name or call_.callee.name or call_.back() is not operands[remaining - 1]
        ):
            break
        left = call_.front()
        remaining -= 1
    if remaining == 1 and left is operands[0]:
        return None

    result = call(fn, operands[:2])
    for operand in operands[2:]:
        result = call(fn, [result, operand])
    return result if not match(value, result) else None


_INEQUALITY_SWAP = {
    "__eq__": "__eq__", "__ne__": "__ne__", "__lt__": "__gt__",
    "__le__": "__ge__", "__gt__": "__lt__", "__ge__": "__le__",
}


def canon_inequality(value):
    fn = get_func(value.callee)
    flip = _INEQUALITY_SWAP.get(fn.unmangled_name) if fn else None
    if len(value.args) != 2 or not flip:
        return None
    lhs, rhs = value.args
    if get_rank(lhs) <= get_rank(rhs):
        return None
    replacement = rhs._binary_op(flip, lhs)
    if replacement and replacement.type.is_type(value.type) and not match(value, replacement):
        return replacement
    return None


def add_mul(first, second, shared):
    total = first + second
    if not total:
        total = second + first
        if not total or not operator_has(get_op(total), "commutative"):
            return None
    result = total * shared
    if not result:
        result = shared * total
        if not operator_has(get_op(result), "commutative"):
            return None
    return result


def canon_add_mul(value):
    fn = get_func(value.callee)
    if not operator_has(fn, "commutative") or not is_call_of(value, "__add__", 2, method=True):
        return None

    def factors(operand):
        if is_call_of(operand, "__mul__", 2, method=True):
            return cast(CallInstr, operand).args
        return operand, value.module.get_int(1)

    left1, left2 = factors(value.front())
    right1, right2 = factors(value.back())
    for first, second, left, right in (
        (left1, right1, left2, right2),
        (left1, right2, left2, right1),
        (left2, right1, left1, right2),
        (left2, right2, left1, right1),
    ):
        first_var, second_var = cast(VarValue, first), cast(VarValue, second)
        if first_var and second_var and first_var.var.id == second_var.var.id:
            replacement = add_mul(left, right, first)
            if (
                replacement and operator_has(get_op(replacement), "distributive")
                and replacement.type.is_type(value.type) and not match(value, replacement)
            ):
                return replacement
            break
    return None


def canon_const_sub(value):
    if not is_call_of(value, "__sub__", 2, method=True):
        return None
    lhs, rhs = value.args
    if not lhs.type.is_type(rhs.type):
        return None
    module = value.module
    rhs = rhs.get_actual()
    if isinstance(rhs, IntConst) and rhs.val != -(1 << 63):
        replacement = lhs + module.get_int(-rhs.val)
    elif isinstance(rhs, FloatConst):
        replacement = lhs + module.get_float(-rhs.val)
    else:
        return None
    if replacement and replacement.type.is_type(value.type) and not match(value, replacement):
        return replacement
    return None


@dataclass(init=False, eq=False)
class CanonicalizationPass(OperatorPass):
    """C++: codon/cir/transform/cleanup/canonical.cpp."""

    KEY = "core-cleanup-canon"

    side_effects_key: str
    num_replacements: int

    def __init__(self, side_effects_key):
        super().__init__(children_first=True)
        self.side_effects_key = side_effects_key
        self.num_replacements = 0

    def run(self, module):
        self.num_replacements = 0
        super().run(module)

    def handle_CallInstr(self, value):
        effects = self.get_analysis_result(self.side_effects_key)
        if effects.has_side_effect(value):
            return
        result = value
        # Iteration of the native rule table in the libstdc++ build.
        for rule in (canon_const_sub, canon_add_mul, canon_inequality, canon_op_chain):
            replacement = rule(result)
            if replacement is not None:
                result = replacement
                self.num_replacements += 1
        if result is not value:
            value.replace_all(result)

    def handle_SeriesFlow(self, value):
        index = 0
        while index < len(value.series):
            item = value.series[index].get_actual()
            if isinstance(item, SeriesFlow):
                value.series[index:index + 1] = item.series
                index += len(item.series)
            elif isinstance(item, FlowInstr):
                value.series[index:index + 1] = [item.flow, item.value]
                # Revisit the inserted flow so its statements can also be flattened.
            else:
                index += 1
