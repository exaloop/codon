# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ....ast.types import Stdlib
from ... import (
    AssignInstr, BodiedFunc, BoolConst, CallInstr, Const, FlowInstr,
    IfFlow, SeriesFlow, Value, Var, VarValue, cast,
)
from ...analyze.analysis import Analysis
from ...analyze.dataflow.cfg import CFBlock, CFGraph
from ...analyze.dataflow.reaching import RDInspector, RDResult
from ...analyze.module.side_effect import SideEffectResult
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func, make_var
from ...util.operator import Operator
from .expr import BroadcastInfo, CodegenContext, NumPyExpr, freeable
from .forward import do_forwarding, get_forwarding_dags
from .ops import Op
from .parse import is_named_function, parse
from .types import (
    NumPyPass, NumPyPrimitiveTypes, NumPyType, TypeCode, has_lifetime_tag, is_array_type,
    is_copy_index_type, is_owned_index, is_ufunc_type, set_lifetime_tag,
)


ALWAYS_FUSE_COST_THRESHOLD = 10
NEVER_FUSE_COST_THRESHOLD = 50


def is_basic_index_type(typ, types):
    if typ.is_type(types.i64) or typ.is_type(types.none):
        return True
    slice_ = typ.name.startswith(Stdlib.Slice + "[")
    if not slice_ and not typ.name.startswith("Tuple["):
        return False
    generics = typ.get_generics()
    return bool(generics) and all(
        generic.is_type() and (
            generic.get_type_value().is_type(types.i64) if slice_
            else is_basic_index_type(generic.get_type_value(), types)
        )
        for generic in generics
    )


_SCALAR_OPS = {
    "__add__", "__radd__", "__sub__", "__rsub__", "__mul__", "__rmul__",
    "__truediv__", "__rtruediv__", "__floordiv__", "__rfloordiv__", "__mod__", "__rmod__",
    "__pow__", "__rpow__", "__neg__", "__pos__",
}


def is_safe_fusion_leaf(value, types, effects):
    if has_lifetime_tag(value, "numpy.validation") or not effects.has_side_effect(value):
        return True
    call_ = cast(CallInstr, value)
    fn = get_func(call_.callee) if call_ else None
    if not fn:
        return False
    name, args = fn.unmangled_name, call_.args
    array_method = is_array_type(fn.parent_type)
    scalar = NumPyType.get(fn.parent_type, types) if fn.parent_type else NumPyType()
    complex_constructor = name == "__new__" and scalar.dtype in (TypeCode.C64, TypeCode.C128)
    safe = bool(scalar and not scalar.is_array() and (name in _SCALAR_OPS or complex_constructor))
    if safe:
        safe = all(
            (typ := NumPyType.get(arg.type, types)) and not typ.is_array() for arg in args
        )
    safe |= len(args) == 1 and fn.name.startswith("Int.__suffix_j__:") and args[0].type.is_type(types.i64)
    safe |= (
        array_method and name == "__getitem__" and len(args) == 2
        and NumPyType.get(args[0].type, types).is_array()
        and (is_basic_index_type(args[1].type, types) or is_copy_index_type(args[1].type, types))
    )
    if (
        name == "clip" and len(args) == 4
        and (array_method or is_named_function(fn, "std.numpy.routines", "clip"))
        and args[-1].type.is_type(types.none)
    ):
        safe = all(NumPyType.get(arg.type, types) or arg.type.is_type(types.none) for arg in args)
    return safe and all(is_safe_fusion_leaf(arg, types, effects) for arg in args)


def has_ufunc_argument_effects(expr, effects):
    # Evaluate arguments omitted from the arithmetic tree if they have effects.
    for node in expr.walk():
        if node.is_leaf():
            continue
        invocation = cast(CallInstr, node.val)
        fn = get_func(invocation.callee) if invocation else None
        if fn and is_ufunc_type(fn.parent_type):
            if effects.has_side_effect(invocation.front()) or effects.has_side_effect(invocation.args[-2]):
                return True
        if invocation and node.is_clip():
            if (
                effects.has_side_effect(invocation.back())
                or (node.op == Op.CLIP_MIN and effects.has_side_effect(invocation.args[2]))
                or (node.op == Op.CLIP_MAX and effects.has_side_effect(invocation.args[1]))
            ):
                return True
    return False


OWNED_OPS = frozenset({
    Op.NEG, Op.INVERT, Op.ABS, Op.ADD, Op.SUB, Op.MUL, Op.MATMUL, Op.TRUE_DIV, Op.FLOOR_DIV,
    Op.MOD, Op.FMOD, Op.POW, Op.LSHIFT, Op.RSHIFT, Op.AND, Op.OR, Op.XOR, Op.LOGICAL_AND,
    Op.LOGICAL_OR, Op.LOGICAL_XOR, Op.EQ, Op.NE, Op.LT, Op.LE, Op.GT, Op.GE, Op.MIN, Op.MAX,
    Op.FMIN, Op.FMAX, Op.SIN, Op.COS, Op.TAN, Op.ARCSIN, Op.ARCCOS, Op.ARCTAN, Op.ARCTAN2,
    Op.HYPOT, Op.SINH, Op.COSH, Op.TANH, Op.ARCSINH, Op.ARCCOSH, Op.ARCTANH, Op.EXP, Op.EXP2,
    Op.LOG, Op.LOG2, Op.LOG10, Op.EXPM1, Op.LOG1P, Op.SQRT, Op.SQUARE, Op.CBRT, Op.LOGADDEXP,
    Op.LOGADDEXP2, Op.RECIPROCAL, Op.RINT, Op.FLOOR, Op.CEIL, Op.TRUNC, Op.ISNAN, Op.ISINF,
    Op.ISFINITE, Op.SIGN, Op.SIGNBIT, Op.COPYSIGN, Op.SPACING, Op.NEXTAFTER, Op.DEG2RAD,
    Op.RAD2DEG, Op.HEAVISIDE, Op.ROLL, Op.ZEROS_LIKE, Op.ONES_LIKE, Op.WHERE, Op.CLIP,
    Op.CLIP_MIN, Op.CLIP_MAX, Op.SUM, Op.MEAN, Op.PROD, Op.ANY, Op.ALL, Op.AMIN, Op.AMAX,
})


def has_owned_result(expr):
    if not expr.type.is_array() or expr.is_leaf():
        return False
    # Allocation provenance only, not last-use permission. Fail closed for views,
    # identity operations, and new opcodes until their allocation contract is known.
    if expr.op == Op.CAST:
        call_ = cast(CallInstr, expr.val)
        if not call_:
            return False
        copy = cast(BoolConst, call_.back())
        return len(call_.args) == 2 or bool(copy and copy.val)
    return expr.op in OWNED_OPS


def is_matmul_add(expr):
    if (
        expr.op not in (Op.ADD, Op.SUB) or not expr.lhs or not expr.rhs
        or expr.type.dtype not in (TypeCode.ARR_F32, TypeCode.ARR_F64)
    ):
        return False
    product, bias = (
        (expr.lhs, expr.rhs) if expr.lhs.op == Op.MATMUL else (expr.rhs, expr.lhs)
    )
    if product.op != Op.MATMUL or not bias.is_leaf() or not cast(VarValue, bias.val):
        return False

    def operand(input_):
        leaf = input_.lhs if input_.op == Op.TRANSPOSE else input_
        return leaf and leaf.is_leaf() and cast(VarValue, leaf.val) and input_.type.dtype == expr.type.dtype

    return bool(
        bias.type.is_array() and product.type.dtype == bias.type.dtype == expr.type.dtype
        and expr.type.ndim == product.type.ndim and bias.type.ndim <= product.type.ndim
        and product.lhs.type.ndim == 2 and product.rhs.type.ndim in (1, 2)
        and operand(product.lhs) and operand(product.rhs)
    )


def codegen_matmul_add(expr, context, destination=None):
    def operand(input_):
        if input_.is_leaf():
            return context.ref(context.vars[input_])
        return context.helper("_transpose", [context.ref(context.vars[input_.lhs])])

    product_left = expr.lhs.op == Op.MATMUL
    product, bias = (expr.lhs, expr.rhs) if product_left else (expr.rhs, expr.lhs)
    args = [operand(product.lhs), operand(product.rhs), operand(bias)]
    args.append(context.ref(destination) if destination else context.module.get_none_type()())
    # Static modes are product+bias, bias+product, product-bias, bias-product.
    mode = (2 if expr.op == Op.SUB else 0) + (0 if product_left else 1)
    result = make_var(context.helper("_matmul_add", args, [mode]), context.series, context.func)
    for leaf in expr.walk():
        if leaf.is_leaf() and leaf.owned_last_use:
            context.series.push_back(context.helper("_free", [context.ref(context.vars[leaf])]))
    return result


def optimize_helper(unit, expr, context):
    module, series = context.module, context.series

    def replace(node, var, owned):
        replacement = NumPyExpr(node.type, context.ref(var), owned_last_use=owned)
        node.replace(replacement)
        context.vars[node] = var

    # Handle non-elementwise operations before building scalar loops.
    for node in expr.walk():
        if not node.type.is_array():
            continue
        if is_matmul_add(node):
            replace(node, codegen_matmul_add(node, context), True)
        elif node.op == Op.TRANSPOSE:
            left = optimize_helper(unit, node.lhs, context)
            var = make_var(context.helper("_transpose", [context.ref(left)]), context.series, context.func)
            replace(node, var, node.lhs.owned_last_use)
        elif node.op == Op.MATMUL:
            left = optimize_helper(unit, node.lhs, context)
            right = optimize_helper(unit, node.rhs, context)
            var = make_var(
                context.helper("_matmul", [context.ref(left), context.ref(right)]), context.series, context.func
            )
            if freeable(node.lhs):
                series.push_back(context.helper("_free", [context.ref(left)]))
            if freeable(node.rhs):
                series.push_back(context.helper("_free", [context.ref(right)]))
            replace(node, var, True)

    # Collapse profitable subtrees. A runtime branch tests unknown broadcast shapes.
    changed = True
    while changed:
        changed = False
        for node in expr.walk():
            if node.depth() <= 2:
                continue
            cost = node.cost()
            broadcast = node.get_broadcast_info()
            if cost <= ALWAYS_FUSE_COST_THRESHOLD or (
                cost <= NEVER_FUSE_COST_THRESHOLD and broadcast == BroadcastInfo.NO
            ):
                result = node.codegen_fused_eval(context)
            elif cost <= NEVER_FUSE_COST_THRESHOLD and broadcast != BroadcastInfo.YES:
                broadcasts = node.codegen_broadcasts(context)
                sequential, fused = module.nr(SeriesFlow), module.nr(SeriesFlow)
                branch = module.nr(IfFlow, broadcasts, sequential, fused)
                context.series = sequential
                sequential_result = node.codegen_sequential_eval(context)
                context.series = fused
                fused_result = node.codegen_fused_eval(context)
                assert sequential_result.type.is_type(fused_result.type), "fusion changes type"
                result = module.nr(Var, sequential_result.type)
                unit.func.push_back(result)
                sequential.push_back(module.nr(AssignInstr, result, context.ref(sequential_result)))
                fused.push_back(module.nr(AssignInstr, result, context.ref(fused_result)))
                context.series = series
                series.push_back(branch)
            else:
                continue
            replace(node, result, True)
            changed = True
    return expr.codegen_sequential_eval(context)


@dataclass(eq=False)
class NumPyOptimizationUnit:
    value: Value
    func: BodiedFunc
    expr: NumPyExpr
    leaves: list[tuple[NumPyExpr, Value]]
    assign: AssignInstr | None
    validation_order: list[tuple[AssignInstr | None, NumPyExpr]] = field(default_factory=list)
    validation_flows: dict[AssignInstr, SeriesFlow] = field(default_factory=dict)

    def codegen_validation(self, context, at_definitions):
        series = context.series
        owns = has_owned_result(self.expr) and not self.expr.is_reduction()
        clone = CloneVisitor(context.module)
        for assignment, element in self.validation_order:
            context.series = series
            if at_definitions and assignment and assignment is not self.assign:
                if assignment not in self.validation_flows:
                    self.validation_flows[assignment] = context.module.nr(SeriesFlow)
                context.series = self.validation_flows[assignment]
            if element.is_leaf():
                var = make_var(clone.clone(element.val), context.series, self.func)
                context.vars.setdefault(element, var)
                if owns and not element.owned_last_use:
                    set_lifetime_tag(context.series.back(), "numpy.lifetime.input")
            elif element.type.is_array() and not element.is_reduction():
                element.codegen_layout(context)
        context.series = series

    def optimize(self, types, effects):
        expr = self.expr
        reduction = expr.is_reduction()
        owned_leaf = False
        for element in expr.walk():
            if element.is_leaf():
                if not expr.is_leaf() and is_owned_index(element.val, types):
                    element.owned_last_use = True
                owned_leaf |= element.owned_last_use
        if (
            (not expr.type.is_array() and not reduction)
            or (expr.depth() <= 2 and (reduction or not owned_leaf))
            or has_ufunc_argument_effects(expr, effects)
        ):
            return False

        rolls = [element for element in expr.walk() if element.op == Op.ROLL]
        if rolls and (
            any(not (roll.lhs.is_leaf() and roll.rhs.is_leaf()) for roll in rolls)
            or any(
                element.op in (Op.TRANSPOSE, Op.MATMUL) or (element.is_reduction() and element is not expr)
                for element in expr.walk()
            )
        ):
            return False
        if not all(is_safe_fusion_leaf(leaf, types, effects) for _, leaf in self.leaves):
            return False

        if reduction:
            invocation = cast(CallInstr, expr.val)
            if not invocation or any(effects.has_side_effect(arg) for arg in invocation.args[1:]):
                return False
            if expr.lhs.cost() > NEVER_FUSE_COST_THRESHOLD or any(
                element.op == Op.MATMUL or element.is_reduction()
                or (element.op == Op.TRANSPOSE and not element.lhs.is_leaf())
                for element in expr.lhs.walk()
            ):
                return False

        module = self.value.module
        context = CodegenContext(module, module.nr(SeriesFlow), self.func, types)
        owns_result = has_owned_result(expr) and not reduction
        self.codegen_validation(context, True)
        if reduction and rolls:
            element = expr.lhs
            var = element.codegen_fused_eval(context)
            element.replace(NumPyExpr(element.type, context.ref(var), owned_last_use=True))
            context.vars[element] = var
        if reduction:
            for element in expr.lhs.walk():
                if element.op == Op.TRANSPOSE:
                    optimize_helper(self, element, context)
            result = expr.codegen_fused_eval(context)
        else:
            result = optimize_helper(self, expr, context)
        replacement = module.nr(FlowInstr, context.series, context.ref(result))
        set_lifetime_tag(replacement, "numpy.lifetime.expression")
        if owns_result:
            set_lifetime_tag(replacement, "numpy.lifetime.owned")
        self.value.replace_all(replacement)
        return True


@dataclass(init=False, eq=False)
class CheckDestinationUses(Operator):
    output: Var
    definition: AssignInstr
    reaching: RDInspector
    cfg: CFGraph
    block: CFBlock
    allowed: set[int]
    preceding: set[int]
    safe: bool

    def __init__(self, output, definition, reaching, cfg, block, allowed, preceding):
        super().__init__()
        self.output, self.definition, self.reaching = output, definition, reaching
        self.cfg, self.block = cfg, block
        self.allowed, self.preceding = allowed, preceding
        self.safe = True

    def pre_hook(self, value):
        if (
            not self.safe or not isinstance(value, Value) or value is self.definition
            or value.id in self.allowed or self.output not in value.get_used_variables()
        ):
            return
        if any(
            definition.assignment.id == self.definition.id
            for definition in self.reaching.get_reaching_definitions(self.output, value)
        ) and (self.cfg.get_block(value) is not self.block or value.id in self.preceding):
            self.safe = False


_ALLOCATORS = {"empty", "zeros", "ones", "full", "empty_like", "zeros_like", "ones_like", "full_like"}
_DESTINATION_OPS = {Op.ADD, Op.SUB, Op.MUL, Op.NEG, Op.POS}


@dataclass
class DestinationExpression:
    unit: NumPyOptimizationUnit
    destination: Value
    assignment: bool

    def eligible(self, types, reaching, cfg, effects):
        expr, value = self.unit.expr, self.unit.value
        output = cast(VarValue, self.destination)
        if (
            not output or output.var.global_ or not expr.type.is_array()
            or (not self.assignment and expr.depth() <= 2)
            or has_ufunc_argument_effects(expr, effects)
        ):
            return False
        output_type = NumPyType.get(output.type, types)
        if not output_type.is_array() or output_type.dtype != expr.type.dtype:
            return False
        if self.assignment and (
            output_type.ndim == 0 or effects.has_side_effect(cast(CallInstr, value).args[1])
        ):
            return False
        base = output_type.get_ir_base_type(types)
        matmul = is_matmul_add(expr)
        if matmul:
            if output_type.ndim != expr.type.ndim:
                return False
            product = expr.lhs if expr.lhs.op == Op.MATMUL else expr.rhs
            # Exact pointwise self-reads are safe, but BLAS cannot overwrite its inputs.
            if any(
                ref.var is output.var
                for node in product.walk() if (ref := cast(VarValue, node.val))
            ):
                return False

        allowed = {output.id}
        for node in expr.walk():
            if (
                node.type.get_ir_base_type(types) is not base
                or (not matmul and node.type.ndim > output_type.ndim)
            ):
                return False
            if node.is_leaf():
                if not (cast(VarValue, node.val) or cast(Const, node.val)) or effects.has_side_effect(node.val):
                    return False
                allowed.add(node.val.id)
            elif node.op not in _DESTINATION_OPS and not (
                matmul and node.op in (Op.MATMUL, Op.TRANSPOSE)
            ):
                return False

        # Require a fresh allocation in this block and check for earlier aliasing uses.
        definitions = reaching.get_reaching_definitions(output.var, self.destination)
        if len(definitions) != 1:
            return False
        definition = cast(AssignInstr, definitions[0].assignment)
        allocation = cast(CallInstr, definition.rhs) if definition else None
        allocator = get_func(allocation.callee) if allocation else None
        if not allocator or allocator.unmangled_name not in _ALLOCATORS or not is_named_function(
            allocator, "std.numpy.routines", allocator.unmangled_name
        ):
            return False
        block = cfg.get_block(value)
        if not block or cfg.get_block(definition) is not block:
            return False
        preceding = set()
        for child in block:
            if child is value:
                break
            preceding.add(child.id)
        if definition.id not in preceding:
            return False
        uses = CheckDestinationUses(output.var, definition, reaching, cfg, block, allowed, preceding)
        uses.process(self.unit.func)
        return uses.safe

    def optimize(self, types):
        module, expr, fn = self.unit.value.module, self.unit.expr, self.unit.func
        series = module.nr(SeriesFlow)
        context = CodegenContext(module, series, fn, types)
        self.unit.codegen_validation(context, False)
        output = make_var(CloneVisitor(module).clone(self.destination), series, fn)
        result = (
            codegen_matmul_add(expr, context, output) if is_matmul_add(expr)
            else expr.codegen_fused_eval(context, output)
        )
        returned = module.get_none_type()() if self.assignment else context.ref(result)
        self.unit.value.replace_all(module.nr(FlowInstr, series, returned))


def get_full_slice_destination(call_, callee):
    if (
        not callee or not is_array_type(callee.parent_type)
        or callee.unmangled_name != "__setitem__" or len(call_.args) != 3
    ):
        return None
    index = call_.args[1]
    constructor = cast(CallInstr, index)
    fn = get_func(constructor.callee) if constructor else None
    if (
        not index.type.name.startswith(Stdlib.Slice + "[")
        or not fn or not fn.name.startswith(Stdlib.Slice + ".__new__:")
        or len(constructor.args) != 3
    ):
        return None
    module = call_.module
    optional_int = module.get_optional_type(module.get_int_type())
    for bound in constructor:
        empty = cast(CallInstr, bound)
        empty_fn = get_func(empty.callee) if empty else None
        if (
            not empty_fn or not bound.type.is_type(optional_int)
            or not empty_fn.name.startswith("Optional[Int[64]]:Optional.__new__:")
            or empty.args
        ):
            return None
    return call_.front()


@dataclass(init=False, eq=False)
class ExtractArrayExpressions(Operator):
    func: BodiedFunc
    types: NumPyPrimitiveTypes
    reaching: RDInspector
    cfg: CFGraph
    effects: SideEffectResult
    exprs: list[NumPyOptimizationUnit]
    destinations: list[DestinationExpression]
    extracted: set[int]
    resolve_helper: object | None
    helper_substitutions: list[tuple[Value, Value]]
    helper_order: dict[Value, list[Value]]

    def __init__(self, func, reaching, cfg, effects, resolve_helper):
        super().__init__()
        self.func, self.types = func, NumPyPrimitiveTypes(func.module)
        self.reaching, self.cfg, self.effects = reaching, cfg, effects
        self.exprs, self.destinations, self.extracted = [], [], set()
        self.resolve_helper = resolve_helper
        self.helper_substitutions, self.helper_order = [], {}

    def extract(self, value, assign=None):
        if value.id in self.extracted:
            return
        leaves, output = [], []
        call_ = cast(CallInstr, value)
        fn = get_func(call_.callee) if call_ else None
        destination = get_full_slice_destination(call_, fn)
        assignment = destination is not None
        if assignment:
            expr = parse(call_.back(), leaves, self.types)
        else:
            expr = parse(value, leaves, self.types, allow_reduction=True, destination=output)
            destination = output[0] if output else None
        # Claim store-like expressions only after their destination is proven safe.
        if destination and expr and not expr.is_leaf():
            unit = NumPyOptimizationUnit(value, self.func, expr, leaves, None)
            candidate = DestinationExpression(unit, destination, assignment)
            if candidate.eligible(self.types, self.reaching, self.cfg, self.effects):
                self.extracted.update(element.val.id for element in expr.walk())
                self.extracted.add(value.id)
                self.destinations.append(candidate)
                return
            leaves = []
            expr = parse(value, leaves, self.types, allow_reduction=True)
        if expr is None:
            return
        if self.resolve_helper:
            for element in expr.walk():
                helper = cast(CallInstr, element.val) if element.is_leaf() else None
                if helper is None:
                    continue
                result = self.resolve_helper(helper)
                if result is None:
                    continue
                replacement, order = result
                nested = parse(replacement, [], self.types, allow_reduction=True)
                if nested and not nested.is_leaf():
                    element.replace(nested)
                    self.helper_substitutions.append((helper, replacement))
                    self.helper_order.setdefault(helper, order)
                    self.extracted.add(helper.id)
            leaves = [(leaf, leaf.val) for leaf in expr.walk() if leaf.is_leaf()]
        num_array_nodes = 0
        for node in expr.walk():
            num_array_nodes += node.type.is_array()
            self.extracted.add(node.val.id)
        if num_array_nodes and expr.depth() > 1:
            self.exprs.append(NumPyOptimizationUnit(value, self.func, expr, leaves, assign))

    def pre_hook(self, node):
        if isinstance(node, AssignInstr):
            self.extract(node.rhs, None if node.lhs.global_ else node)
        elif isinstance(node, Value):
            self.extract(node)


def get_validation_order(unit, helper_order=None):
    nodes = {element.val for element in unit.expr.walk()}
    order, visited = [], set()

    def visit(value):
        if value in visited:
            return
        visited.add(value)
        for child in value.get_used_values():
            visit(child)
        # Actual arguments precede the summary's own ordered shape checks.
        if helper_order and value in helper_order:
            for step in helper_order[value]:
                if step in nodes and step not in visited:
                    visited.add(step)
                    order.append((step, unit.assign))
        if value in nodes:
            order.append((value, unit.assign))

    visit(unit.value)
    return order


def set_validation_order(unit, order):
    # Multiple reads of one actual value still require separate leaf entries.
    nodes = {}
    for element in unit.expr.walk():
        nodes.setdefault(element.val, []).append(element)
    for node, assignment in order:
        unit.validation_order.extend((assignment, element) for element in nodes.get(node, ()))


@dataclass
class Expression:
    unit: NumPyOptimizationUnit
    assignments: list[AssignInstr]
    substitutions: list[tuple[Value, Value]]


@dataclass(init=False, eq=False)
class NumPyFunctionExpressions(ExtractArrayExpressions):
    expressions: list[Expression]

    def __init__(self, func, reaching, cfg, effects, resolve_helper=None):
        super().__init__(func, reaching, cfg, effects, resolve_helper)
        self.expressions = []
        self.process(func)
        for destination in self.destinations:
            set_validation_order(destination.unit, get_validation_order(destination.unit))
        order = []
        for unit in self.exprs:
            order.extend(get_validation_order(unit, self.helper_order))
        for dag in get_forwarding_dags(func, reaching, cfg, effects, self.exprs):
            assignments, substitutions = [], list(self.helper_substitutions)
            unit = do_forwarding(dag, assignments, substitutions)
            set_validation_order(unit, order)
            self.expressions.append(Expression(unit, assignments, substitutions))


@dataclass
class NumPyExpressionResult:
    reaching: RDResult
    side_effects: SideEffectResult
    functions: dict[int, NumPyFunctionExpressions | None] = field(default_factory=dict)

    def get(self, func):
        if func.id not in self.functions:
            self.functions[func.id] = self.extract(func)
        return self.functions[func.id]

    def extract(self, func, resolve_helper=None):
        definitions = self.reaching.results.get(func.id)
        if definitions is None:
            return None
        cfg = self.reaching.cfg_result.graphs[func.id]
        return NumPyFunctionExpressions(func, definitions, cfg, self.side_effects, resolve_helper)

    def take(self, func):
        return self.get(func) and self.functions.pop(func.id)


@dataclass(init=False, eq=False)
class NumPyExpressionAnalysis(Analysis):
    KEY = "core-numpy-expressions"

    reaching_def_key: str
    side_effects_key: str

    def __init__(self, reaching_def_key, side_effects_key):
        super().__init__()
        self.reaching_def_key, self.side_effects_key = reaching_def_key, side_effects_key

    def run(self, module):
        return NumPyExpressionResult(
            self.get_analysis_result(self.reaching_def_key),
            self.get_analysis_result(self.side_effects_key),
        )


@dataclass(init=False, eq=False)
class NumPyFusionPass(NumPyPass):
    KEY = "core-numpy-fusion"

    expressions_key: str
    side_effects_key: str

    def __init__(self, expressions_key, side_effects_key):
        super().__init__()
        self.expressions_key, self.side_effects_key = expressions_key, side_effects_key

    def visit(self, node):
        func = cast(BodiedFunc, node)
        if not func:
            return super().visit(node)
        expressions = self.get_analysis_result(self.expressions_key).take(func)
        if not expressions:
            return
        effects = self.get_analysis_result(self.side_effects_key)
        for destination in expressions.destinations:
            destination.optimize(expressions.types)
        for expression in expressions.expressions:
            unit = expression.unit
            if unit.optimize(expressions.types, effects):
                # Only remove producers after a replacement has been emitted.
                for assignment in expression.assignments:
                    replacement = unit.validation_flows.get(assignment)
                    assignment.replace_all(replacement or func.module.nr(SeriesFlow))
