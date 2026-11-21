# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field
from enum import Enum, auto

from ... import (
    BodiedFunc, BoolConst, CallInstr, FloatConst, IfFlow, IntConst, ReturnInstr,
    SeriesFlow, StringConst, TernaryInstr, Value, Var, VarValue, cast,
)
from ...module import Module
from ...util.cloning import CloneVisitor
from ...util.irtools import (
    call, get_derived_func_name, get_func, get_return_type, make_tuple, make_var, series, tuple_get,
)
from .ops import Op
from .types import FUSION_MODULE, NumPyPrimitiveTypes, NumPyType, TypeCode as TC


_REDUCTIONS = {Op.SUM, Op.MEAN, Op.PROD, Op.ANY, Op.ALL, Op.AMIN, Op.AMAX}
_CLIP = {Op.CLIP, Op.CLIP_MIN, Op.CLIP_MAX}
_PRODUCERS = {Op.CAST, Op.ZEROS_LIKE, Op.ONES_LIKE}
_VECTOR_LOOPS = frozenset({
    "arccos", "arccosh", "arcsin", "arcsinh", "arctan", "arctanh", "arctan2",
    "cos", "exp", "exp2", "expm1", "log", "log10", "log1p",
    "log2", "sin", "sinh", "tanh", "hypot",
})
_ARRAY_FLOATS = {TC.ARR_F16, TC.ARR_F32, TC.ARR_F64}
_ARRAY_COMPLEX = {TC.ARR_C64, TC.ARR_C128}
_ARRAY_INTS = {TC.ARR_I8, TC.ARR_U8, TC.ARR_I16, TC.ARR_U16, TC.ARR_I32, TC.ARR_U32, TC.ARR_I64, TC.ARR_U64}
_PYTHON_SCALARS = {TC.BOOL, TC.I64, TC.F64, TC.C128}


class BroadcastInfo(Enum):
    YES = auto()
    NO = auto()
    MAYBE = auto()


@dataclass(eq=False)
class NumPyExpr:
    type: NumPyType
    val: Value | None
    op: Op = Op.NONE
    lhs: "NumPyExpr | None" = None
    rhs: "NumPyExpr | None" = None
    third: "NumPyExpr | None" = None
    owned_last_use: bool = False

    def replace(self, other):
        self.type, self.val, self.op = other.type, other.val, other.op
        self.lhs, self.rhs, self.third = other.lhs, other.rhs, other.third
        self.owned_last_use = other.owned_last_use
        other.type, other.val, other.op = NumPyType(), None, Op.NONE
        other.lhs = other.rhs = other.third = None
        other.owned_last_use = False

    def children(self):
        return (child for child in (self.lhs, self.rhs, self.third) if child is not None)

    def walk(self):
        # Preorder deliberately reads children after a possible replacement.
        yield self
        for child in self.children():
            yield from child.walk()

    def is_leaf(self):
        return self.lhs is self.rhs is self.third is None

    def is_clip(self):
        return self.op in _CLIP

    def is_reduction(self):
        return self.op in _REDUCTIONS

    def depth(self):
        return 1 + max((child.depth() for child in self.children()), default=0)

    def nodes(self):
        return 1 + sum(child.nodes() for child in self.children())

    def have_vectorized_loop(self):
        lhs, rhs = self.lhs, self.rhs
        if lhs and lhs.type.dtype in _ARRAY_COMPLEX and self.op == Op.ABS:
            return True
        dtypes = {TC.ARR_F32, TC.ARR_F64}
        if any(child and child.type.dtype not in dtypes for child in (lhs, rhs)):
            return False
        if lhs and rhs and lhs.type.dtype != rhs.type.dtype:
            return False
        return self.op.op_name in _VECTOR_LOOPS

    def cost(self):
        cost = self.op.cost
        if cost == -1:
            return -1
        if self.op == Op.ABS and self.lhs and self.lhs.type.dtype in _ARRAY_COMPLEX:
            cost = 50
        if self.have_vectorized_loop():
            # The runtime's vectorized loops are much faster than scalar evaluation.
            factor = 6 if self.lhs.type.dtype == TC.ARR_F32 else 3
            cost = max(cost * factor, 64)

        constants = [
            expr.val.get_actual() if expr and expr.is_leaf() else None
            for expr in (self.lhs, self.rhs)
        ]
        if any(isinstance(item, (IntConst, FloatConst)) for item in constants):
            if self.op in (Op.TRUE_DIV, Op.FLOOR_DIV, Op.MOD, Op.FMOD):
                cost = 1
            elif self.op == Op.POW and isinstance(constants[1], IntConst):
                cost = 1 if constants[1].val == 2 else 5
        for child in self.children():
            child_cost = child.cost()
            if child_cost == -1:
                return -1
            cost += child_cost
        return cost

    def roll_axis(self):
        axis = cast(IntConst, cast(CallInstr, self.val).back()).val
        return axis + self.type.ndim if axis < 0 else axis

    def get_broadcast_info(self):
        ndim = None
        var = None
        multiple_vars, other_arrays = False, 0
        for leaf in self.walk():
            if not leaf.is_leaf() or not leaf.type.is_array():
                continue
            if ndim is not None and ndim != leaf.type.ndim:
                return BroadcastInfo.YES
            ndim = leaf.type.ndim
            if ref := cast(VarValue, leaf.val):
                if var is not None and var is not ref.var:
                    multiple_vars = True
                var = ref.var
            else:
                other_arrays += 1
        if multiple_vars or other_arrays > 1 or (other_arrays == 1 and var is not None):
            return BroadcastInfo.MAYBE
        return BroadcastInfo.NO

    def codegen_broadcasts(self, context):
        target_shape, result = None, None
        for leaf in self.walk():
            if leaf.is_leaf() and leaf.type.is_array():
                variable = context.vars[leaf]
                shape = context.helper("_shape", [context.ref(variable)])
                if target_shape is None:
                    target_shape = shape
                else:
                    different = target_shape.ne(shape)
                    result = different if result is None else result | different
        return result if result is not None else context.module.get_bool(False)


    def codegen_layout(self, context):
        if self.is_leaf():
            return context.vars[self]
        if self in context.layouts:
            return context.layouts[self]
        module = context.module
        base = self.type.get_ir_base_type(context.types)
        if self.op in (Op.MATMUL, Op.TRANSPOSE, Op.WHERE):
            operands = [context.ref(child.codegen_layout(context)) for child in self.children()]
            name, generics = {
                Op.MATMUL: ("_matmul_layout", [base]),
                Op.TRANSPOSE: ("_transpose", []),
                Op.WHERE: ("_where_layout", [base]),
            }[self.op]
        else:
            operands = [
                context.ref(child.codegen_layout(context))
                for child in self.children() if child.type.is_array()
            ]
            if self.op == Op.ROLL:
                operands.append(module.get_bool(True))
                name, generics = "_producer_layout", [base, self.op.op_name, "K"]
            elif self.op in _PRODUCERS:
                original = cast(CallInstr, self.val)
                order = cast(StringConst, original.args[1]).val
                copy = cast(BoolConst, original.back()) if len(original.args) == 3 else None
                operands.append(module.get_bool(not copy or copy.val))
                name, generics = "_producer_layout", [base, self.op.op_name, order]
            else:
                operands = [make_tuple(operands)]
                name, generics = ("_clip_layout" if self.is_clip() else "_layout"), [base]
        result = make_var(context.helper(name, operands, generics), context.series, context.func)
        context.layouts[self] = result
        return result

    def codegen_scalar_expr(self, context, arguments, scalar_map, scalars):
        module = context.module
        if self.op == Op.ROLL:
            return self.lhs.codegen_scalar_expr(context, arguments, scalar_map, scalars)
        if self.is_leaf():
            if self.type.is_array():
                return context.ref(arguments[self])[module.get_int(0)]
            return tuple_get(context.ref(scalars), scalar_map[self])

        operands = [child.codegen_scalar_expr(context, arguments, scalar_map, scalars)
                    for child in self.children()]
        name = "_" + self.op.op_name
        if self.op == Op.WHERE or self.is_clip():
            return context.helper(name, operands, [self.type.get_ir_base_type(context.types)])
        return self.codegen_operation(context, operands)

    def codegen_operation(self, context, operands):
        generics = []
        if len(operands) == 2:
            common_type = decide_types(self, self.lhs.type, self.rhs.type, context.types)
            operands = [context.helper("_cast", [arg], [common_type]) for arg in operands]
        elif self.op in _PRODUCERS:
            generics = [self.type.get_ir_base_type(context.types)]
        return context.helper("_" + self.op.op_name, operands, generics)

    def codegen_fused_eval(self, context, destination=None):
        """Build the scalar callback and select the native stdlib loop helper."""
        module, series_, types = context.module, context.series, context.types
        reduction = self.is_reduction()
        element = self.lhs if reduction else self
        roll_inputs = {expr.lhs: expr for expr in element.walk() if expr.op == Op.ROLL}
        leaves = [(leaf, context.vars[leaf]) for leaf in element.walk() if leaf.is_leaf()]

        arrays, shifts, extra, extra_map = [], [], [], {}
        base = element.type.get_ir_base_type(types)
        names, signature = ([], []) if reduction else (["out"], [module.get_pointer_type(base)])
        for leaf, var in leaves:
            if leaf.type.is_array():
                names.append(f"in{len(arrays)}")
                signature.append(module.get_pointer_type(leaf.type.get_ir_base_type(types)))
                arrays.append(context.ref(var))
                if roll_inputs:
                    dimensions = [module.get_int(0)] * element.type.ndim
                    if roll := roll_inputs.get(leaf):
                        axis = roll.roll_axis() + element.type.ndim - roll.type.ndim
                        dimensions[axis] = context.ref(context.vars[roll.rhs])
                    shifts.append(make_tuple(dimensions))
            else:
                extra_map[leaf] = len(extra)
                extra.append(context.ref(var))

        extra_tuple = make_tuple(extra, module)
        names.append("extra")
        signature.append(extra_tuple.type)
        callback = new_scalar_callback(
            context, self.val, base if reduction else module.get_none_type(), names, signature
        )
        input_parameters = iter(callback.args[0 if reduction else 1:-1])
        arguments = {leaf: next(input_parameters) for leaf, _ in leaves if leaf.type.is_array()}
        scalar = element.codegen_scalar_expr(context, arguments, extra_map, callback.args[-1])
        if reduction:
            result = context.helper("_cast", [scalar], [base])
            callback.body = series(module.nr(ReturnInstr, result))
        else:
            output = context.ref(callback.args[0])
            callback.body = series(
                context.helper("_ptrset", [output, scalar], arg_types=[signature[0], base])
            )

        scalar_callback = context.ref(callback)
        if reduction:
            scalar_callback = context.helper("_reduction_function", [scalar_callback], [callback.name])
        loop_args = [make_tuple(arrays), scalar_callback, extra_tuple]
        loop_generics = [base]
        if reduction:
            if self.rhs:
                initial = context.ref(context.vars[self.rhs])
            elif self.op == Op.MEAN:
                initial = context.helper("_cast", [module.get_int(0)], [self.type.get_ir_base_type(types)])
            else:
                initial = make_tuple([], module)
            loop_args.append(initial)
            loop_generics = [base, self.type.get_ir_base_type(types), self.op.op_name]

        needs_layout = bool(roll_inputs) or (not destination and element in context.layouts)
        needs_layout |= any(
            expr.op == Op.WHERE or expr.is_clip() or (expr.type.ndim > 1 and expr.op in _PRODUCERS)
            for expr in element.walk()
        )
        loop_name = "_loop_reduce" if reduction else "_loop_alloc"
        axis = cast(CallInstr, self.val).args[1] if reduction else None
        axis_reduction = reduction and (self.type.is_array() or not axis.type.is_type(types.none))
        if needs_layout or axis_reduction:
            loop_args.append(context.ref(element.codegen_layout(context)))
            loop_name += "_layout"
        if roll_inputs:
            loop_args.append(make_tuple(shifts))
            loop_name = "_loop_alloc_roll"
        if axis_reduction:
            loop_args.append(CloneVisitor(module).clone(axis))
            loop_generics.append(self.type.ndim)
            loop_name = "_loop_reduce_axis"
        if destination:
            loop_args.append(context.ref(destination))
            loop_generics.clear()
            loop_name = "_loop_into"

        result = make_var(context.helper(loop_name, loop_args, loop_generics), series_, context.func)
        # Release only after the fused loop has consumed every operand.
        for leaf in self.walk():
            if leaf.is_leaf() and leaf.owned_last_use:
                free_args = [context.ref(context.vars[leaf])]
                name = "_free"
                if needs_layout and not reduction:
                    free_args.append(context.ref(result))
                    name = "_free_copy_source"
                series_.push_back(context.helper(name, free_args))
        return result

    def codegen_sequential_eval(self, context):
        if self.is_leaf():
            return context.vars[self]
        module, block = context.module, context.series
        left = self.lhs.codegen_sequential_eval(context)
        right = self.rhs.codegen_sequential_eval(context) if self.rhs else None
        can_release_left = freeable(self.lhs)
        can_release_right = freeable(self.rhs)
        left_reusable = can_release_left and self.lhs.type == self.type
        right_reusable = can_release_right and self.rhs.type == self.type

        def release(var):
            return context.helper("_free", [context.ref(var)])

        def bind(value):
            return make_var(value, block, context.func)

        if self.op == Op.ROLL:
            original = cast(CallInstr, self.val)
            result = bind(call(get_func(original.callee), [
                context.ref(left), context.ref(right), original.back(),
            ]))
            if can_release_left:
                block.push_back(release(left))
            return result

        if self.op == Op.WHERE or self.is_clip():
            third = self.third.codegen_sequential_eval(context) if self.third else None
            original = cast(CallInstr, self.val)
            args = list(original)
            args[0] = context.ref(left)
            args[2 if self.op == Op.CLIP_MAX else 1] = context.ref(right)
            if third:
                args[2] = context.ref(third)
            result = bind(call(get_func(original.callee), args))
            for var, eligible in ((left, can_release_left), (right, can_release_right),
                                  (third, freeable(self.third))):
                if eligible:
                    block.push_back(release(var))
            return result

        if self.type.ndim > 1 and self.op in _PRODUCERS:
            original = cast(CallInstr, self.val)
            args = list(original)
            args[0] = context.ref(left)
            result = bind(call(get_func(original.callee), args))
            if can_release_left:
                block.push_back(context.helper("_free_copy_source", [context.ref(left), context.ref(result)]))
            return result

        if self.op == Op.MATMUL and right:
            result = bind(context.helper("_matmul", [context.ref(left), context.ref(right)]))
            if can_release_left:
                block.push_back(release(left))
            if can_release_right:
                block.push_back(release(right))
            return result

        left_shape = context.helper("_shape", [context.ref(left)])
        like = left
        if right:
            right_shape = context.helper("_shape", [context.ref(right)])
            out_shape_value = context.helper("_broadcast", [left_shape, right_shape])
            if self.rhs.type.ndim > self.lhs.type.ndim:
                like = right
        else:
            out_shape_value = left_shape
        out_shape = bind(out_shape_value)
        base = self.type.get_ir_base_type(context.types)

        def new_array():
            return context.helper("_create", [context.ref(like), context.ref(out_shape)], [base])

        free_left = free_right = False
        left_cond = right_cond = None
        if right:
            if left_reusable and self.rhs.type.ndim == 0:
                result = left
            elif right_reusable and self.lhs.type.ndim == 0:
                result = right
            elif not left_reusable and not right_reusable:
                result = bind(new_array())
                free_left, free_right = can_release_left, can_release_right
            else:
                # Shape equality at runtime permits reuse of owned storage.
                if left_reusable:
                    shape = context.helper("_shape", [context.ref(left)])
                    left_cond = bind(shape.eq(context.ref(out_shape)))
                if right_reusable:
                    shape = context.helper("_shape", [context.ref(right)])
                    right_cond = bind(shape.eq(context.ref(out_shape)))
                allocation = new_array()
                if right_cond:
                    allocation = module.nr(TernaryInstr, context.ref(right_cond), context.ref(right), allocation)
                if left_cond:
                    allocation = module.nr(TernaryInstr, context.ref(left_cond), context.ref(left), allocation)
                result = bind(allocation)
                free_left = can_release_left and left_cond is None
                free_right = can_release_right and right_cond is None
        elif left_reusable:
            result = left
        else:
            result = bind(new_array())
            free_left = can_release_left

        if self.have_vectorized_loop():
            if right:
                name = "_apply_vectorized_loop_binary"
                args = [context.ref(left), context.ref(right), context.ref(result)]
            else:
                name = "_apply_vectorized_loop_unary"
                args = [context.ref(left), context.ref(result)]
            block.push_back(context.helper(name, args, [self.op.op_name]))
        else:
            self.codegen_basic_loop(context, result, left, right)

        assert not (free_left and left_cond) and not (free_right and right_cond)
        if left_cond and right_cond:
            block.push_back(module.nr(
                IfFlow, context.ref(left_cond), series(release(right)),
                series(release(left), module.nr(
                    IfFlow, context.ref(right_cond), module.nr(SeriesFlow), series(release(right))
                )),
            ))
        else:
            for var, static, cond in ((left, free_left, left_cond), (right, free_right, right_cond)):
                if static:
                    block.push_back(release(var))
                elif cond:
                    block.push_back(module.nr(
                        IfFlow, context.ref(cond), module.nr(SeriesFlow), series(release(var))
                    ))
        return result

    def codegen_basic_loop(self, context, result, left, right):
        module = context.module
        arrays = [context.ref(result)]
        extra = []
        base = self.type.get_ir_base_type(context.types)
        names = ["out"]
        signature = [module.get_pointer_type(base)]
        operand_slots = []
        for index, (element, var) in enumerate(((self.lhs, left), (self.rhs, right))):
            if not var:
                continue
            if element.type.is_array():
                if var is result:
                    operand_slots.append((True, 0))
                else:
                    operand_slots.append((True, len(arrays)))
                    arrays.append(context.ref(var))
                    names.append(f"in{index}")
                    signature.append(module.get_pointer_type(element.type.get_ir_base_type(context.types)))
            else:
                operand_slots.append((False, len(extra)))
                extra.append(context.ref(var))

        extra_tuple = make_tuple(extra, module)
        names.append("extra")
        signature.append(extra_tuple.type)
        scalar = new_scalar_callback(context, self.val, module.get_none_type(), names, signature)
        body = module.nr(SeriesFlow)
        operands = [
            context.ref(scalar.args[slot])[module.get_int(0)] if array
            else tuple_get(context.ref(scalar.args[-1]), slot)
            for array, slot in operand_slots
        ]
        element = self.codegen_operation(context, operands)
        body.push_back(context.helper("_ptrset", [context.ref(scalar.args[0]), element]))
        scalar.body = body
        context.series.push_back(
            context.helper("_loop_basic", [make_tuple(arrays), context.ref(scalar), extra_tuple])
        )


@dataclass
class CodegenContext:
    module: Module
    series: SeriesFlow
    func: BodiedFunc
    types: NumPyPrimitiveTypes
    vars: dict[NumPyExpr, Var] = field(default_factory=dict)
    layouts: dict[NumPyExpr, Var] = field(default_factory=dict)

    def helper(self, name, args, generics=(), arg_types=None):
        if arg_types is None:
            arg_types = [arg.type for arg in args]
        fn = self.module.get_or_realize_func(name, arg_types, list(generics), FUSION_MODULE)
        assert fn, f"NumPy helper {name} not found"
        return call(fn, args)

    def ref(self, var):
        return self.module.nr(VarValue, var)


def coerce_scalar_array(scalar, array, types):
    ad, sd = array.dtype, scalar.dtype
    if ad in (TC.ARR_F16, TC.ARR_F32) and sd == TC.C128:
        return types.c64
    if (
        (sd in (TC.BOOL, TC.I64) and ad in _ARRAY_INTS | _ARRAY_FLOATS | _ARRAY_COMPLEX)
        or (sd == TC.F64 and ad in _ARRAY_FLOATS | _ARRAY_COMPLEX)
        or (sd == TC.C128 and ad in _ARRAY_COMPLEX)
    ):
        return array.get_ir_base_type(types)
    return scalar.get_ir_base_type(types)


def decide_types(expr, lhs, rhs, types):
    if expr.op == Op.COPYSIGN:
        return expr.type.get_ir_base_type(types)
    if lhs.is_array() and rhs.dtype in _PYTHON_SCALARS:
        return coerce_scalar_array(rhs, lhs, types)
    if lhs.dtype in _PYTHON_SCALARS and rhs.is_array():
        return coerce_scalar_array(lhs, rhs, types)
    left, right = lhs.get_ir_base_type(types), rhs.get_ir_base_type(types)
    fn = left.module.get_or_realize_func("_coerce", [], [left, right], FUSION_MODULE)
    assert fn, "coerce function not found"
    return get_return_type(fn)


def new_scalar_callback(context, source, result, names, signature):
    module = context.module
    typ = module.get_func_type(result, signature)
    fn = module.n(
        BodiedFunc, source,
        get_derived_func_name("__numpy_fusion_scalar_fn", context.func, source),
    )
    fn.realize(typ, names)
    return fn


def freeable(expression):
    return bool(expression and expression.type.is_array() and (
        expression.owned_last_use or not expression.is_leaf()
    ))
