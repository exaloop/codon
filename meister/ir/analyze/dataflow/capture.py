# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ... import AssignInstr, BodiedFunc, ExternalFunc, Func, InternalFunc, LLVMFunc, Value, Var, cast
from ...type import Type
from ...util.irtools import get_func, get_return_type, has_attribute
from ...util.operator import Operator
from ...util.side_effect import (
    DERIVES_ATTR, NO_CAPTURE_ATTR, NO_SIDE_EFFECT_ATTR, PURE_ATTR, SELF_CAPTURES_ATTR,
)
from ...util.visitor import ConstVisitor
from ..analysis import Analysis
from .cfg import AssignKind, CFBlock, CFGraph, SyntheticAssignInstr
from .dominator import DominatorResult
from .reaching import RDInspector, RDResult


def should_track(node):
    # Only types containing pointers can be captured.
    return node is not None and not (node if isinstance(node, Type) else node.type).is_atomic()


@dataclass
class CaptureInfo:
    arg_captures: list[int] = field(default_factory=list)
    return_captures: bool = False
    extern_captures: bool = False
    modified: bool = False

    def __bool__(self):
        return bool(self.arg_captures or self.return_captures or self.extern_captures)

    @staticmethod
    def unknown(fn, typ):
        if not should_track(typ):
            return CaptureInfo()
        return CaptureInfo(
            [index for index, arg in enumerate(fn.args) if should_track(arg)],
            should_track(get_return_type(fn)),
            extern_captures=True,
            modified=True,
        )


@dataclass(init=False, eq=False)
class OrderCache:
    """Conditional encounter order, including paths that return through a back edge."""

    cfg: CFGraph
    positions: dict[CFBlock, dict[int, int]]
    block_order: dict[tuple[CFBlock, CFBlock], bool]

    def __init__(self, cfg):
        self.cfg = cfg
        self.positions, self.block_order = {}, {}

    def happens_before(self, before, after):
        before_block, after_block = self.cfg.get_block(before), self.cfg.get_block(after)
        if before_block is None or after_block is None:
            return False
        if before_block is after_block:
            if before_block not in self.positions:
                positions = {}
                for index, value in enumerate(before_block):
                    positions.setdefault(value.id, index)
                self.positions[before_block] = positions
            positions = self.positions[before_block]
            earlier, later = positions.get(before.id), positions.get(after.id)
            assert earlier is not None or later is not None, "values are missing from the CFG block"
            if earlier is None or (later is not None and earlier > later):
                return False

        pair = before_block, after_block
        if pair not in self.block_order:
            # A nonempty path from the later block back to the first reverses the order.
            answer = True
            pending, seen = list(after_block.successors), set()
            while pending:
                block = pending.pop()
                if block is before_block:
                    answer = False
                    break
                if block not in seen:
                    seen.add(block)
                    pending.extend(block.successors)
            self.block_order[pair] = answer
        return self.block_order[pair]


@dataclass
class RDManager:
    rd: RDInspector
    cache: dict[tuple[int, int], set[int]] = field(default_factory=dict)

    def get_reaching_definitions(self, var, location):
        key = var.id, location.id
        if key not in self.cache:
            self.cache[key] = {
                definition.assignment.id
                for definition in self.rd.get_reaching_definitions(var, location)
            }
        return self.cache[key]

    def is_invalid(self, var):
        return self.rd.is_invalid(var)


@dataclass(init=False, eq=False)
class DerivedSet:
    func: Func
    root: Var | None
    args: list[int]
    derived_values: set[int]
    derived_vars: dict[int, dict[int, Value]]
    result: CaptureInfo

    def __init__(self, func, root=None):
        self.func, self.root = func, root
        self.args = [arg.id for arg in func.args] if root else []
        self.derived_values, self.derived_vars = set(), {}
        self.result = CaptureInfo()

    def set_extern_captured(self):
        self.result.extern_captures = True
        if should_track(get_return_type(self.func)):
            self.result.return_captures = True

    def is_var_derived(self, var, location, rd):
        causes = self.derived_vars.get(var.id)
        if causes is None:
            return False
        # Globals and variables whose addresses are taken can change at any point.
        if var.global_ or rd.is_invalid(var):
            return True
        reached = rd.get_reaching_definitions(var, location)
        return any(reached & rd.get_reaching_definitions(var, cause) for cause in causes.values())

    def set_var_derived(self, var, cause, should_arg_capture=True):
        if not should_track(var):
            return
        if var.global_:
            self.set_extern_captured()
        identifier = var.id
        if should_arg_capture and self.root and identifier != self.root.id:
            for index, argument in enumerate(self.args):
                if argument == identifier and index not in self.result.arg_captures:
                    self.result.arg_captures.append(index)
        self.derived_vars.setdefault(identifier, {}).setdefault(cause.id, cause)

    def set_derived(self, value):
        if should_track(value):
            self.derived_values.add(value.id)

    def size(self):
        return len(self.derived_values) + sum(map(len, self.derived_vars.values()))


@dataclass(init=False, eq=False)
class ExtractVars(ConstVisitor):
    """Find the variables that participate in the value's capture dependencies."""

    context: "CaptureContext"
    variables: dict[int, Var]
    escapes: bool

    def __init__(self, context):
        self.context = context
        self.variables = {}
        self.escapes = False

    def visit_VarValue(self, value):
        if should_track(value.var):
            self.variables[value.var.id] = value.var

    visit_PointerValue = visit_VarValue

    def visit_CallInstr(self, value):
        fn = get_func(value.callee)
        if fn is None:
            for arg in value:
                if should_track(arg):
                    self.visit(arg)
            return
        summary = self.context.get(fn)
        for index, arg in enumerate(value):
            if should_track(arg):
                # Variadic C calls may supply more arguments than the signature.
                info = summary[index] if index < len(summary) else CaptureInfo.unknown(fn, arg.type)
                if info.return_captures:
                    self.visit(arg)

    def visit_YieldInInstr(self, value):
        # The value injected into a generator can be externally reachable.
        self.escapes = True

    def visit_TernaryInstr(self, value):
        self.visit(value.true_value)
        self.visit(value.false_value)

    def visit_ExtractInstr(self, value):
        self.visit(value.val)

    def visit_FlowInstr(self, value):
        self.visit(value.value)


def extract_vars(context, value):
    visitor = ExtractVars(context)
    visitor.visit(value)
    return list(visitor.variables.values()), visitor.escapes


@dataclass(init=False, eq=False)
class CaptureTracker(Operator):
    context: "CaptureContext"
    cfg: CFGraph
    rd: RDManager
    order: OrderCache
    dsets: list[DerivedSet]

    def __init__(self, context, fn, value=None):
        super().__init__()
        self.context = context
        self.cfg = context.reaching.cfg_result.graphs[fn.id]
        self.rd = RDManager(context.reaching.results[fn.id])
        self.order = OrderCache(self.cfg)
        self.dsets = []
        if value is not None:
            variables, escapes = extract_vars(context, value)
            derived = DerivedSet(fn)
            if escapes:
                derived.set_extern_captured()
            derived.set_derived(value)
            for var in variables:
                derived.set_var_derived(var, value)
            self.dsets.append(derived)
        else:
            assignments = {
                assignment.lhs.id: assignment
                for assignment in self.cfg.entry
                if isinstance(assignment, SyntheticAssignInstr) and should_track(assignment.lhs)
            }
            for arg in fn.args:
                if should_track(arg):
                    derived = DerivedSet(fn, arg)
                    derived.set_var_derived(arg, assignments[arg.id])
                    self.dsets.append(derived)

    def sets_of_value(self, value):
        if value is not None:
            for derived in self.dsets:
                if value.id in derived.derived_values:
                    yield derived

    def sets_of_var(self, var, location):
        return (derived for derived in self.dsets if derived.is_var_derived(var, location, self.rd))

    def backward_link(self, derived, cause, to_vars, from_vars, from_escapes):
        if from_escapes:
            derived.set_extern_captured()
        for to_var in to_vars:
            for to_cause in list(derived.derived_vars.get(to_var.id, {}).values()):
                if isinstance(to_cause, (AssignInstr, SyntheticAssignInstr)):
                    continue
                if self.order.happens_before(to_cause, cause):
                    continue
                if to_var.global_ or self.rd.is_invalid(to_var) or (
                    self.rd.get_reaching_definitions(to_var, cause)
                    & self.rd.get_reaching_definitions(to_var, to_cause)
                ):
                    for var in from_vars:
                        derived.set_var_derived(var, to_cause)

    def link(self, source, target, cause):
        from_vars, from_escapes = extract_vars(self.context, source)
        is_var = isinstance(target, Var)
        if is_var:
            to_vars, to_escapes = [target], False
        else:
            to_vars, to_escapes = extract_vars(self.context, target)

        for derived in self.sets_of_value(source):
            if to_escapes:
                derived.set_extern_captured()
            for var in to_vars:
                derived.set_var_derived(var, cause, should_arg_capture=not is_var)
        target_sets = (
            self.sets_of_var(target, cause) if is_var else self.sets_of_value(target)
        )
        for derived in target_sets:
            self.backward_link(derived, cause, to_vars, from_vars, from_escapes)

    def handle_VarValue(self, value):
        for derived in self.sets_of_var(value.var, value):
            derived.set_derived(value)

    handle_PointerValue = handle_VarValue

    def handle_AssignInstr(self, value):
        self.link(value.rhs, value.lhs, value)

    def handle_ExtractInstr(self, value):
        if should_track(value):
            for derived in self.sets_of_value(value.val):
                derived.set_derived(value)

    def handle_InsertInstr(self, value):
        self.link(value.rhs, value.lhs, value)
        for derived in self.sets_of_value(value.lhs):
            derived.result.modified = True

    def handle_CallInstr(self, value):
        fn = get_func(value.callee)
        if fn:
            summary = self.context.get(fn)
        else:
            tracked = [index for index, arg in enumerate(value) if should_track(arg)]
            unknown = CaptureInfo(tracked, should_track(value), extern_captures=True, modified=True)
            summary = [unknown if should_track(arg) else CaptureInfo() for arg in value]

        for index, arg in enumerate(value):
            info = summary[index] if index < len(summary) else CaptureInfo.unknown(fn, arg.type)
            for captured in info.arg_captures:
                self.link(arg, value.args[captured], value)
            for derived in self.sets_of_value(arg):
                if info.return_captures:
                    derived.set_derived(value)
                if info.extern_captures:
                    derived.set_extern_captured()
                if info.modified:
                    derived.result.modified = True

    def handle_ForFlow(self, value):
        if not should_track(value.var):
            return
        for derived in self.sets_of_value(value.iter):
            assignments = [
                synthetic for synthetic in self.cfg.value_mapping.values()
                if isinstance(synthetic, SyntheticAssignInstr)
                and synthetic.kind == AssignKind.NEXT_VALUE and synthetic.lhs.id == value.var.id
            ]
            assert len(assignments) <= 1, "multiple synthetic assignments for loop var"
            for assignment in assignments:
                derived.set_var_derived(value.var, assignment)

    def handle_TernaryInstr(self, value):
        for operand in (value.true_value, value.false_value):
            for derived in self.sets_of_value(operand):
                derived.set_derived(value)

    def handle_FlowInstr(self, value):
        for derived in self.sets_of_value(value.value):
            derived.set_derived(value)

    def handle_ReturnInstr(self, value):
        for derived in self.sets_of_value(value.value):
            derived.result.return_captures = True

    handle_YieldInstr = handle_ReturnInstr
    handle_AwaitInstr = handle_ReturnInstr

    def handle_ThrowInstr(self, value):
        for derived in self.sets_of_value(value.value):
            derived.set_extern_captured()

    def run_to_completion(self, fn):
        while True:
            previous = sum(derived.size() for derived in self.dsets)
            self.process(fn)
            self.reset()
            if sum(derived.size() for derived in self.dsets) == previous:
                return


def no_capture_info(fn, derives):
    return [CaptureInfo(return_captures=derives and should_track(arg)) for arg in fn.args]


@dataclass
class CaptureContext:
    reaching: RDResult
    dominating: DominatorResult
    known_results: dict[int, list[CaptureInfo]] | None = None
    results: dict[int, list[CaptureInfo]] = field(default_factory=dict)

    def get(self, fn):
        identifier = fn.id
        if self.known_results is not None and identifier in self.known_results:
            return self.known_results[identifier]
        if isinstance(fn, (ExternalFunc, LLVMFunc)):
            derives = has_attribute(fn, DERIVES_ATTR)
            if has_attribute(fn, SELF_CAPTURES_ATTR):
                result = no_capture_info(fn, derives)
                if result:
                    result[0].modified = True
                for index, arg in enumerate(fn.args[1:], 1):
                    if should_track(arg):
                        result[index].arg_captures.append(0)
                return result
            if any(has_attribute(fn, attr) for attr in (PURE_ATTR, NO_SIDE_EFFECT_ATTR, NO_CAPTURE_ATTR)):
                return no_capture_info(fn, derives)
            return [CaptureInfo.unknown(fn, arg.type) for arg in fn.args]
        if isinstance(fn, InternalFunc):
            # Aggregate fields and promise pointers retain their input dependencies.
            return no_capture_info(fn, fn.intrinsic_derives_from_arguments())

        assert isinstance(fn, BodiedFunc), "unknown function type"
        if identifier not in self.results:
            # Seed recursive calls conservatively until the body is analyzed.
            self.results[identifier] = [CaptureInfo.unknown(fn, arg.type) for arg in fn.args]
            tracker = CaptureTracker(self, fn)
            tracker.run_to_completion(fn)
            derived = iter(tracker.dsets)
            self.results[identifier] = [
                next(derived).result if should_track(arg) else CaptureInfo() for arg in fn.args
            ]
        return self.results[identifier]


@dataclass
class CaptureResult:
    rd_result: RDResult
    dom_result: DominatorResult
    results: dict[int, list[CaptureInfo]]


@dataclass(init=False, eq=False)
class CaptureAnalysis(Analysis):
    KEY = "core-analyses-capture"

    rd_analysis_key: str
    dom_analysis_key: str

    def __init__(self, rd_analysis_key, dom_analysis_key):
        super().__init__()
        self.rd_analysis_key, self.dom_analysis_key = rd_analysis_key, dom_analysis_key

    def run(self, module):
        reaching = self.get_analysis_result(self.rd_analysis_key)
        dominating = self.get_analysis_result(self.dom_analysis_key)
        context = CaptureContext(reaching, dominating)
        return CaptureResult(reaching, dominating, {
            fn.id: context.get(fn)
            for node in (module.main_func, *module)
            if (fn := cast(Func, node)) is not None
        })


def escapes(fn, value, captures):
    if not should_track(value):
        return CaptureInfo()
    context = CaptureContext(captures.rd_result, captures.dom_result, captures.results)
    tracker = CaptureTracker(context, fn, value)
    tracker.run_to_completion(fn)
    return tracker.dsets[0].result
