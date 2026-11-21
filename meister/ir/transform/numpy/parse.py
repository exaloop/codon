# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ....ast.types import mangle
from ... import AssignInstr, BoolConst, CallInstr, Const, FlowInstr, IntConst, SeriesFlow, StringConst, VarValue, cast
from ...module import Module
from ...util.irtools import get_func
from .expr import NumPyExpr
from .ops import MAGICS, UFUNCS, Op
from .types import NumPyType, is_array_type, is_ufunc_type


_MAGICS = {getattr(Module, name): descriptor for name, descriptor in MAGICS.items()}
_REDUCTIONS = {
    "sum": Op.SUM, "prod": Op.PROD, "mean": Op.MEAN, "any": Op.ANY, "all": Op.ALL,
    "min": Op.AMIN, "max": Op.AMAX, "amin": Op.AMIN, "amax": Op.AMAX,
}
_NO_VALUE = mangle("std.numpy.util", cls="_NoValue")
_ORDERS = {"C", "F", "A", "K"}


def is_named_function(fn, module, name, suffix="["):
    return fn is not None and fn.name.startswith(mangle(module, func=name) + suffix)


def parse(value, leaves, types, allow_reduction=False, destination=None):
    """C++: codon/cir/transform/numpy/numpy.cpp. Parse with leaves in evaluation order."""
    typ = NumPyType.get(value.type, types)
    if not typ:
        return None
    invocation = cast(CallInstr, value)
    fn = get_func(invocation.callee) if invocation else None

    def node(op, *operands):
        children = []
        for operand in operands:
            child = parse(operand, leaves, types)
            if child is None:
                return None
            children.append(child)
        return NumPyExpr(typ, value, op, *children)

    if allow_reduction and fn and (op := _REDUCTIONS.get(fn.unmangled_name)):
        name = fn.unmangled_name
        logical = name in ("any", "all")
        mean = name == "mean"
        extrema = op in (Op.AMIN, Op.AMAX)
        expected_args = 4 if logical or mean else 6 if extrema else 5
        if len(invocation.args) == expected_args and (
            is_array_type(fn.parent_type) or is_named_function(fn, "std.numpy.reductions", name)
        ):
            if mean:
                generics = fn.type.get_generics()
                if not generics or not generics[0].is_type():
                    return None
                dtype = generics[0].get_type_value()
                if not dtype.is_type(types.none) and not dtype.is_type(typ.get_ir_base_type(types)):
                    return None
            args = invocation.args
            initial = None if logical or mean else args[4 if extrema else 3]
            have_initial = initial is not None and initial.type.name != _NO_VALUE
            if (
                args[2].type.is_type(types.none) and args[-1].type.name == _NO_VALUE
                and (not have_initial or cast(Const, initial) or cast(VarValue, initial))
            ):
                operand = parse(args[0], leaves, types)
                start = parse(initial, leaves, types) if have_initial else None
                if (
                    operand and operand.type.is_array() and operand.type.ndim > 0
                    and (not have_initial or (start and not start.type.is_array()))
                ):
                    return NumPyExpr(typ, value, op, operand, start)
                leaves.clear()

    # Scalars and zero-dimensional arrays are evaluated only once.
    if typ.ndim == 0:
        leaf = NumPyExpr(typ, value)
        leaves.append((leaf, value))
        return leaf

    if fn:
        args, name = invocation.args, fn.unmangled_name
        size = len(args)
        parent_array = is_array_type(fn.parent_type)
        if (
            size == 3 and is_array_type(args[0].type) and args[1].type.is_type(types.i64)
            and is_named_function(fn, "std.numpy.routines", "roll")
        ):
            axis = cast(IntConst, args[2])
            if axis and -typ.ndim <= axis.val < typ.ndim:
                if rolled := node(Op.ROLL, *args[:2]):
                    return rolled

        if (
            size == 4 and name == "clip"
            and (parent_array or is_named_function(fn, "std.numpy.routines", "clip"))
            and args[3].type.is_type(types.none)
        ):
            no_min, no_max = args[1].type.is_type(types.none), args[2].type.is_type(types.none)
            if no_min and no_max:
                return None
            if no_min:
                return node(Op.CLIP_MAX, args[0], args[2])
            if no_max:
                return node(Op.CLIP_MIN, *args[:2])
            return node(Op.CLIP, *args[:3])

        if size == 3 and is_named_function(fn, "std.numpy.routines", "where"):
            return node(Op.WHERE, *args)

        if (
            size == 2 and is_array_type(args[0].type) and name in ("zeros_like", "ones_like")
            and is_named_function(fn, "std.numpy.routines", name)
        ):
            order = cast(StringConst, args[1])
            if order and order.val in _ORDERS:
                if filled := node(Op.ZEROS_LIKE if name == "zeros_like" else Op.ONES_LIKE, args[0]):
                    return filled

        if parent_array and (
            (name == "astype" and size == 3 and cast(BoolConst, args[-1]))
            or (name == "copy" and size == 2)
        ):
            order = cast(StringConst, args[1])
            # A same-dtype no-copy cast can return the source storage.
            owns = name == "copy" or cast(BoolConst, args[-1]).val or not args[0].type.is_type(value.type)
            if owns and order and order.val in _ORDERS:
                if converted := node(Op.CAST, args[0]):
                    return converted

        if size == 3 and args[-1].type.is_type(types.none) and (
            is_named_function(fn, "std.numpy.linalg_sym", "matmul")
            or (typ.ndim == 2 and is_named_function(fn, "std.numpy.linalg_sym", "dot", "[]"))
        ):
            return node(Op.MATMUL, *args[:2])

        if size == 1 and is_named_function(fn, "std.internal.builtin", "abs"):
            return node(Op.ABS, args[0])
        if size == 1 and parent_array and name == "T":
            return node(Op.TRANSPOSE, args[0])

        if name == "__call__" and is_ufunc_type(fn.parent_type):
            generics = fn.parent_type.get_generics()
            assert generics and generics[0].is_static_str(), "unrecognized ufunc class generics"
            call_generics = fn.type.get_generics()
            assert call_generics and call_generics[0].is_type(), "unrecognized ufunc call generics"
            if call_generics[0].get_type_value().is_type(types.none):
                if descriptor := UFUNCS.get(generics[0].get_static_string_value()):
                    op, arity = descriptor
                    assert size == arity + 3, "unexpected ufunc signature"
                    output, where = args[-2], cast(BoolConst, args[-1])
                    # Receiver, one or two operands, then out and where.
                    has_out = not output.type.is_type(types.none)
                    if where and where.val and (not has_out or destination is not None):
                        result = node(op, *args[1:1 + arity])
                        if result and has_out:
                            destination.append(output)
                        return result

        if parent_array and (descriptor := _MAGICS.get(name)):
            op, arity, reflected = descriptor
            if size == arity:
                result = node(op, *args)
                if result and reflected:
                    result.lhs, result.rhs = result.rhs, result.lhs
                return result

    # The frontend can wrap reflected operators in assignments to preserve
    # argument evaluation order. Parse assignments in that order.
    flow = cast(FlowInstr, value)
    if flow:
        series = cast(SeriesFlow, flow.flow)
        call_ = cast(CallInstr, flow.value)
        fn = get_func(call_.callee) if call_ else None
        if series and fn and len(call_.args) == 2:
            descriptor = _MAGICS.get(fn.unmangled_name)
            if descriptor and descriptor[2]:
                op = descriptor[0]
                lhs, rhs = (cast(VarValue, arg) for arg in call_)
                first = lhs.var if lhs else None
                second = rhs.var if rhs else None
                assignments = [cast(AssignInstr, assignment) for assignment in series]
                if not assignments:
                    return parse(flow.value, leaves, types)
                if len(assignments) == 1 and (assignment := assignments[0]):
                    if assignment.lhs is first:
                        result = node(op, assignment.rhs, call_.back())
                        if result:
                            result.lhs, result.rhs = result.rhs, result.lhs
                        return result
                    if assignment.lhs is second:
                        return node(op, assignment.rhs, call_.front())
                if len(assignments) == 2 and all(assignments):
                    a, b = assignments
                    if a.lhs is first and b.lhs is second:
                        result = node(op, a.rhs, b.rhs)
                        if result:
                            result.lhs, result.rhs = result.rhs, result.lhs
                        return result
                    if a.lhs is second and b.lhs is first:
                        return node(op, a.rhs, b.rhs)

    leaf = NumPyExpr(typ, value)
    leaves.append((leaf, value))
    return leaf
