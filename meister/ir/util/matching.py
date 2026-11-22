# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from .. import (
    AssignInstr, AwaitInstr, BodiedFunc, BreakInstr, CallInstr, Const, ContinueInstr,
    ExternalFunc, ExtractInstr, FlowInstr, ForFlow, Func, IfFlow, ImperativeForFlow,
    InsertInstr, InternalFunc, LLVMFunc, PipelineFlow, PointerValue, ReturnInstr,
    SeriesFlow, StackAllocInstr, TernaryInstr, ThrowInstr, Type, Value, Var, VarValue,
    WhileFlow, YieldInInstr, YieldInstr,
)
from ..dsl import CustomConst, CustomFlow, CustomInstr


class AnyValue(Value):
    """C++: codon/cir/util/matching.h. A wildcard for structural matching."""

    def _get_type(self):
        return self.module.get_int_type()


def match(first, second, check_names=False):
    """C++: codon/cir/util/matching.cpp. Compare CIR structure and node names."""
    active = set()

    def same_values(left, right):
        left, right = list(left), list(right)
        return len(left) == len(right) and all(compare(x, y) for x, y in zip(left, right))

    def compare(first, second):
        if first is None or second is None:
            return first is second
        first, second = first.get_actual(), second.get_actual()
        if isinstance(first, Type) or isinstance(second, Type):
            return isinstance(first, Type) and isinstance(second, Type) and first.is_type(second)
        if isinstance(first, AnyValue) or isinstance(second, AnyValue):
            return True
        if type(first) is not type(second) or (not check_names and first.name != second.name):
            return False
        pair = (first, second)
        if pair in active:
            return True
        active.add(pair)
        try:
            return compare_nodes(first, second)
        finally:
            active.remove(pair)

    def compare_nodes(first, second):
        match first:
            case BodiedFunc():
                return (
                    first.type.is_type(second.type)
                    and same_values(first.args, second.args)
                    and same_values(first.symbols, second.symbols)
                    and compare(first.body, second.body)
                    and first.jit == second.jit
                )
            case ExternalFunc():
                return (
                    first.unmangled_name == second.unmangled_name
                    and first.type.is_type(second.type)
                    and same_values(first.args, second.args)
                )
            case InternalFunc():
                return (
                    first.parent_type is second.parent_type
                    and first.intrinsic == second.intrinsic
                    and first.intrinsic_type is second.intrinsic_type
                    and first.type.is_type(second.type)
                    and same_values(first.args, second.args)
                )
            case LLVMFunc():
                return (
                    first.llvm_declares == second.llvm_declares
                    and first.llvm_body == second.llvm_body
                    and first.type.is_type(second.type)
                    and same_values(first.args, second.args)
                    and len(first.llvm_literals) == len(second.llvm_literals)
                    and all(
                        compare(x.value, y.value) if isinstance(x.value, Type)
                        else x.value == y.value
                        for x, y in zip(first.llvm_literals, second.llvm_literals)
                    )
                )
            case Func():
                return False
            case Var():
                return first.type.is_type(second.type)
            case VarValue():
                return first.var.type.is_type(second.var.type)
            case PointerValue():
                return first.var.type.is_type(second.var.type) and first.fields == second.fields
            case CustomConst() | CustomFlow() | CustomInstr():
                return first.match(second)
            case Const():
                return first.type.is_type(second.type) and first.val == second.val
            case SeriesFlow():
                return same_values(first, second)
            case IfFlow():
                return same_values(
                    (first.cond, first.true_branch, first.false_branch),
                    (second.cond, second.true_branch, second.false_branch),
                )
            case WhileFlow():
                return compare(first.cond, second.cond) and compare(first.body, second.body)
            case ForFlow():
                return first.async_ == second.async_ and same_values(
                    (first.iter, first.body, first.var), (second.iter, second.body, second.var)
                )
            case ImperativeForFlow():
                return first.step == second.step and same_values(
                    (first.var, first.body, first.start, first.end),
                    (second.var, second.body, second.start, second.end),
                )
            case PipelineFlow():
                return len(first.stages) == len(second.stages) and all(
                    x.generator == y.generator and x.parallel == y.parallel
                    and compare(x.callee, y.callee) and same_values(x.args, y.args)
                    for x, y in zip(first, second)
                )
            case AssignInstr() | InsertInstr():
                return (
                    (not isinstance(first, InsertInstr) or (
                        first.polymorphic == second.polymorphic and first.field == second.field
                    ))
                    and compare(first.lhs, second.lhs)
                    and compare(first.rhs, second.rhs)
                )
            case ExtractInstr():
                return (
                    first.polymorphic == second.polymorphic and first.field == second.field
                    and compare(first.val, second.val)
                )
            case CallInstr():
                return compare(first.callee, second.callee) and same_values(first, second)
            case StackAllocInstr():
                return first.count == second.count and first.type.is_type(second.type)
            case YieldInInstr():
                return first.type.is_type(second.type)
            case TernaryInstr() | FlowInstr():
                return same_values(first.get_used_values(), second.get_used_values())
            case BreakInstr() | ContinueInstr():
                return compare(first.loop, second.loop)
            case ReturnInstr() | YieldInstr():
                return compare(first.value, second.value)
            case AwaitInstr():
                return (
                    first.type.is_type(second.type) and first.generator == second.generator
                    and compare(first.value, second.value)
                )
            case ThrowInstr():
                return compare(first.value, second.value) and compare(first.fallback, second.fallback)
        return False

    return compare(first, second)
