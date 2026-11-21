# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from collections import defaultdict
from collections.abc import Callable
from dataclasses import dataclass

from ... import (
    AssignInstr, BodiedFunc, BoolConst, FloatConst, Flow, FlowInstr,
    Func, IntConst, PointerValue, ReturnInstr, SeriesFlow, Value, Var, VarValue, cast,
)
from ...analyze.module.side_effect import SideEffectResult
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func, get_return_type, is_no_inline
from ...util.operator import Operator
from .expr import CodegenContext
from .fusion import has_ufunc_argument_effects
from .types import NumPyPass, NumPyPrimitiveTypes, NumPyType, set_lifetime_tag


MAX_HELPER_DEPTH = 4
MAX_EXPRESSION_NODES = 16
MAX_TEMPLATE_NODES = 128
MAX_HELPER_STATEMENTS = 17
MAX_CALLER_EXPANSIONS = 16
MAX_CALLER_NODES = 256


class Count(Operator):
    size: int

    def __init__(self, node):
        super().__init__()
        self.size = 0
        self.process(node)

    def pre_hook(self, node):
        self.size += 1


class CheckLocals(Operator):
    local: bool

    def __init__(self, value):
        super().__init__()
        self.local = False
        self.process(value)

    def pre_hook(self, node):
        self.local |= any(not var.global_ and not isinstance(var, Func)
                          for var in node.get_used_variables())


class KeepInvariants(Operator):
    clone: CloneVisitor
    effects: SideEffectResult

    def __init__(self, clone, effects):
        super().__init__()
        self.clone, self.effects = clone, effects

    def pre_hook(self, node):
        # RD/SE describe the original program, not detached summary clones.
        # Preserve literals and pure, parameter-independent subtrees (notably
        # ufunc receivers and out=None). This shares IR identity, not evaluation.
        value = cast(Value, node)
        if value and not self.effects.has_side_effect(value) and not CheckLocals(value).local:
            self.clone.force_remap(value, value)


class BindReads(Operator):
    clone: CloneVisitor
    arguments: dict[Var, Value]

    def __init__(self, clone, arguments):
        super().__init__()
        self.clone, self.arguments = clone, arguments

    def handle_VarValue(self, read):
        if read.var in self.arguments:
            self.clone.force_remap(read, self.arguments[read.var])


class CheckTemplate(Operator):
    parameters: set[Var]
    templates: dict[int, InlineTemplate | None]
    size: int
    valid: bool

    def __init__(self, value, callee, templates):
        super().__init__()
        self.parameters, self.templates = set(callee.args), templates
        self.size, self.valid = 0, True
        self.process(value)

    def pre_hook(self, node):
        self.size += 1
        if isinstance(node, (Flow, FlowInstr, AssignInstr, PointerValue)):
            self.valid = False
        for var in node.get_used_variables():
            if not var.global_ and not isinstance(var, Func) and var not in self.parameters:
                self.valid = False

    def handle_CallInstr(self, call):
        callee = get_func(call.callee)
        # Only calls exposed by the NumPy parser participate in composition.
        # A helper hidden inside an opaque leaf must remain in its own function.
        if callee and callee.id in self.templates:
            self.valid = False


@dataclass
class InlineTemplate:
    value: Value
    size: int
    direct_bindings: bool
    locals: list[Var]
    # Compose the raw expression and its check order, not the emitted
    # validation FlowInstr. Nested wrappers hide arithmetic from the parser.
    raw: Value
    order: list[Value]
    raw_size: int
    nodes: int
    depth: int
    expansions: int
    reduction: bool


def get_statements(callee):
    body = cast(SeriesFlow, callee.body)
    if body is None or callee.generator or callee.async_ or (
        callee.name.startswith("std.numpy.") or is_no_inline(callee)
    ):
        return []

    statements = []

    def flatten(series):
        for statement in series:
            nested = cast(SeriesFlow, statement)
            if nested is None:
                statements.append(statement.get_actual())
            else:
                flatten(nested)

    flatten(body)
    if not statements or len(statements) > MAX_HELPER_STATEMENTS:
        return []
    returned = cast(ReturnInstr, statements[-1])
    if returned is None or returned.value is None:
        return []
    for statement in statements[:-1]:
        assignment = cast(AssignInstr, statement)
        if assignment is None or assignment.lhs.global_:
            return []
    return statements if Count(body).size <= MAX_TEMPLATE_NODES else []


def stable_read(value):
    read = cast(VarValue, value)
    return read is not None and not read.var.global_


def prepare(callee, analysis, templates):
    statements = get_statements(callee)
    returned = statements[-1]
    module = callee.module
    depth = expansions = 1
    nested_nodes = 0
    estimated_size = Count(callee.body).size
    composable = True

    def resolve_helper(call):
        nonlocal depth, expansions, nested_nodes, estimated_size, composable
        nested = cast(BodiedFunc, get_func(call.callee))
        if nested is None or nested.id not in templates:
            return None
        summary = templates[nested.id]
        # Reductions are only parsed at expression roots. Count occurrences,
        # not cache entries, so branching helpers cannot grow exponentially.
        if (
            summary is None or not summary.direct_bindings or summary.reduction
            or len(call.args) != len(nested.args) or not all(map(stable_read, call.args))
            or summary.depth >= MAX_HELPER_DEPTH
            or expansions + summary.expansions > MAX_CALLER_EXPANSIONS
            or nested_nodes + summary.nodes > MAX_EXPRESSION_NODES
            or estimated_size + summary.raw_size > MAX_TEMPLATE_NODES
        ):
            composable = False
            return None

        depth = max(depth, summary.depth + 1)
        expansions += summary.expansions
        nested_nodes += summary.nodes
        estimated_size += summary.raw_size
        clone = CloneVisitor(module)
        KeepInvariants(clone, analysis.side_effects).process(summary.raw)
        BindReads(clone, dict(zip(nested.args, call.args))).process(summary.raw)
        return clone.clone(summary.raw), [clone.clone(step) for step in summary.order]

    extracted = analysis.extract(callee, resolve_helper)
    if not extracted or extracted.destinations or len(extracted.expressions) != 1 or not composable:
        return None
    expression = extracted.expressions[0]
    unit = expression.unit
    if unit.expr.nodes() > MAX_EXPRESSION_NODES or (
        expression.substitutions and has_ufunc_argument_effects(unit.expr, extracted.effects)
    ):
        return None
    if unit.value is not returned.value:
        read = cast(VarValue, returned.value)
        if read is None or not unit.assign or read.var is not unit.assign.lhs:
            return None
        definitions = extracted.reaching.get_reaching_definitions(read.var, read)
        if len(definitions) != 1 or definitions[0].assignment is not unit.assign:
            return None
    covered = {returned, *expression.assignments}
    if unit.assign:
        covered.add(unit.assign)
    if len(covered) != len(statements) or not all(statement in covered for statement in statements):
        return None

    clone = CloneVisitor(module)
    invariants = KeepInvariants(clone, analysis.side_effects)
    invariants.process(unit.value)
    for _, replacement in expression.substitutions:
        invariants.process(replacement)
    for original, replacement in expression.substitutions:
        clone.force_remap(original, clone.clone(replacement))
    value = clone.clone(unit.value)
    check = CheckTemplate(value, callee, templates)
    if not check.valid or check.size > MAX_TEMPLATE_NODES:
        return None

    raw, raw_size = value, check.size
    order = [clone.clone(element.val) for _, element in unit.validation_order]
    direct_bindings = all(
        stable_read(element.val) or cast((IntConst, FloatConst, BoolConst), element.val)
        for element in unit.expr.walk() if element.is_leaf()
    )
    original_order = [element.val for _, element in unit.validation_order if not element.is_leaf()]
    collapsed_order, first_leaf = [], None
    nodes = {}
    for element in unit.expr.walk():
        nodes.setdefault(element.val, element)
    substitutions = dict(expression.substitutions)
    visited = set()

    def collect_order(current):
        nonlocal first_leaf
        while current in substitutions:
            current = substitutions[current]
        if current in visited:
            return
        visited.add(current)
        for child in current.get_used_values():
            collect_order(child)
        element = nodes.get(current)
        if element is not None:
            if not element.is_leaf():
                collapsed_order.append(current)
            elif first_leaf is None:
                first_leaf = element

    collect_order(unit.value)
    locals_ = []
    size = raw_size
    if original_order != collapsed_order:
        if not direct_bindings or first_leaf is None:
            return None
        validation = module.nr(SeriesFlow)
        context = CodegenContext(module, validation, callee, extracted.types)
        unit.codegen_validation(context, False)
        checked_clone = CloneVisitor(module)
        for element, variable in context.vars.items():
            locals_.append(variable)
            checked_clone.force_remap(element.val, module.nr(VarValue, variable))
        locals_.extend(context.layouts.values())
        checked_leaf = module.nr(FlowInstr, validation, module.nr(VarValue, context.vars[first_leaf]))
        set_lifetime_tag(checked_leaf, "numpy.validation")
        checked_clone.force_remap(first_leaf.val, checked_leaf)
        for original, replacement in expression.substitutions:
            checked_clone.force_remap(original, checked_clone.clone(replacement))
        value = checked_clone.clone(unit.value)
        size = Count(value).size
        if size > MAX_TEMPLATE_NODES:
            return None
    return InlineTemplate(value, size, direct_bindings, locals_, raw, order, raw_size,
                          unit.expr.nodes(), depth, expansions, unit.expr.is_reduction())


class Prepare(Operator):
    prepare: Callable[[BodiedFunc], None]

    def __init__(self, prepare):
        super().__init__()
        self.prepare = prepare

    def handle_CallInstr(self, call):
        callee = cast(BodiedFunc, get_func(call.callee))
        if callee:
            self.prepare(callee)


class Inliner(Operator):
    parent: BodiedFunc
    templates: dict[int, InlineTemplate | None]
    remaining: int
    budget: int

    def __init__(self, parent, templates):
        super().__init__()
        self.parent, self.templates = parent, templates
        self.remaining, self.budget = MAX_CALLER_EXPANSIONS, MAX_CALLER_NODES

    def handle_CallInstr(self, call):
        if not self.remaining:
            return
        callee = cast(BodiedFunc, get_func(call.callee))
        if not callee or callee is self.parent:
            return
        prepared = self.templates.get(callee.id)
        if not prepared or prepared.expansions > self.remaining or len(call.args) != len(callee.args):
            return
        module = call.module
        # Substitute only stable local reads directly. Otherwise bind every
        # actual argument once, including unused ones, before the helper body.
        direct_bindings = prepared.direct_bindings and all(map(stable_read, call.args))
        # The emitted wrapper is part of the caller's budget too. Actual values
        # themselves are retained and still run once in source order.
        cost = prepared.size + (0 if direct_bindings else len(call.args) + 2)
        if cost > self.budget:
            return
        clone = CloneVisitor(module)
        for local in prepared.locals:
            self.parent.symbols.append(clone.force_clone(local))
        bindings = module.nr(SeriesFlow)
        for argument, value in zip(callee.args, call.args):
            if direct_bindings:
                clone.force_remap(argument, cast(VarValue, value).var)
            else:
                variable = clone.force_clone(argument)
                self.parent.symbols.append(variable)
                bindings.push_back(module.nr(AssignInstr, variable, value))
        replacement = clone.clone(prepared.value)
        if not direct_bindings:
            replacement = module.nr(FlowInstr, bindings, replacement)
        self.remaining -= prepared.expansions
        self.budget -= cost
        call.replace_all(replacement)


class InlineFunctions(Operator):
    templates: dict[int, InlineTemplate | None]

    def __init__(self, templates):
        super().__init__()
        self.templates = templates

    def visit(self, node):
        fn = cast(BodiedFunc, node)
        if fn:
            Inliner(fn, self.templates).process(fn)
        else:
            super().visit(node)


class NumPyInlinePass(NumPyPass):
    """C++: codon/cir/transform/numpy/numpy.cpp:2222."""

    KEY = "core-numpy-inline"

    expressions_key: str

    def __init__(self, expressions_key):
        super().__init__()
        self.expressions_key = expressions_key

    def run(self, module):
        analysis = self.get_analysis_result(self.expressions_key)
        types = NumPyPrimitiveTypes(module)
        candidates, seen, candidate_ids = [], set(), set()

        def collect(callee):
            if callee.id in seen:
                return
            seen.add(callee.id)
            array = NumPyType.get(get_return_type(callee), types).is_array()
            for argument in callee.args:
                array |= NumPyType.get(argument.type, types).is_array()
            if array and get_statements(callee):
                candidates.append(callee)
                candidate_ids.add(callee.id)

        Prepare(collect).process(module)
        # Kahn's ordering is iterative even for arbitrarily deep input call graphs.
        # Recursive components (and callers depending on them) never become ready.
        # Finish every immutable summary before rewriting any function body, so a
        # caller cannot bypass depth limits by seeing an already-inlined callee.
        pending, callers, ready = {}, defaultdict(list), []
        for callee in candidates:
            dependencies = set()

            def collect_dependency(dependency):
                if dependency.id in candidate_ids:
                    dependencies.add(dependency.id)

            Prepare(collect_dependency).process(callee.body)
            pending[callee.id] = len(dependencies)
            for dependency in dependencies:
                callers[dependency].append(callee)
            if not dependencies:
                ready.append(callee)
        templates = {}
        for callee in ready:
            templates[callee.id] = prepare(callee, analysis, templates)
            for caller in callers[callee.id]:
                pending[caller.id] -= 1
                if pending[caller.id] == 0:
                    ready.append(caller)

        InlineFunctions(templates).process(module)
