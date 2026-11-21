# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from collections import Counter
from dataclasses import dataclass

from ... import BodiedFunc
from ...dsl import CustomFlow, CustomInstr
from ...util.irtools import get_func, has_attribute
from ...util.operator import Operator
from ...util.side_effect import (
    NON_PURE_ATTR, NO_CAPTURE_ATTR, NO_SIDE_EFFECT_ATTR, PURE_ATTR, SideEffectStatus,
)
from ...util.visitor import ConstVisitor
from ..analysis import Analysis
from ..dataflow.capture import CaptureResult, escapes


@dataclass(init=False, eq=False)
class VarUseAnalyzer(Operator):
    var_counts: Counter[int]
    assignment_counts: Counter[int]

    def __init__(self):
        super().__init__()
        self.var_counts, self.assignment_counts = Counter(), Counter()

    def pre_hook(self, node):
        self.var_counts.update(var.id for var in node.get_used_variables())

    def handle_AssignInstr(self, value):
        self.assignment_counts[value.lhs.id] += 1


def function_status_from_attributes(fn):
    for attr, status in (
        (PURE_ATTR, SideEffectStatus.PURE),
        (NO_SIDE_EFFECT_ATTR, SideEffectStatus.NO_SIDE_EFFECT),
        (NO_CAPTURE_ATTR, SideEffectStatus.NO_CAPTURE),
        (NON_PURE_ATTR, SideEffectStatus.UNKNOWN),
    ):
        if has_attribute(fn, attr):
            return status, True
    return SideEffectStatus.UNKNOWN, False


@dataclass(init=False, eq=False)
class SideEffectAnalyzer(ConstVisitor):
    """C++: codon/cir/analyze/module/side_effect.cpp. Classify expressions and calls."""

    uses: VarUseAnalyzer
    captures: CaptureResult
    global_assignment_has_side_effects: bool
    result: dict[int, SideEffectStatus]
    escape_purity: dict[tuple[int, int], bool]
    functions: list[BodiedFunc]
    func_status: SideEffectStatus

    def __init__(self, uses, captures, global_assignment_has_side_effects):
        self.uses, self.captures = uses, captures
        self.global_assignment_has_side_effects = global_assignment_has_side_effects
        self.result, self.escape_purity, self.functions = {}, {}, []
        self.func_status = SideEffectStatus.PURE

    def set(self, value, expr, func=SideEffectStatus.PURE):
        self.result[value.id] = expr
        self.func_status = max(self.func_status, func)

    def process(self, value):
        if value is None:
            return SideEffectStatus.PURE
        identifier = value.id
        if identifier not in self.result:
            self.visit(value)
        return self.result[identifier]

    def assignment_status(self, var):
        if var is None:
            return SideEffectStatus.PURE, SideEffectStatus.PURE
        used = self.uses.var_counts[var.id] != self.uses.assignment_counts[var.id]
        status = SideEffectStatus.UNKNOWN if var.global_ else SideEffectStatus.NO_CAPTURE
        # Later passes can introduce globals eliminated earlier, so the folding
        # groups differ in whether an unused global assignment has side effects.
        if self.global_assignment_has_side_effects or var.external:
            local, external = used or var.global_, var.global_
        else:
            local, external = used, used and var.global_
        return (
            status if local else SideEffectStatus.PURE,
            status if external else SideEffectStatus.PURE,
        )

    def handle_assignment(self, value, var, base):
        local, external = self.assignment_status(var)
        self.set(value, max(local, base), external)

    def visit_Module(self, module):
        self.process(module.main_func)
        for value in module:
            self.process(value)

    def visit_BodiedFunc(self, fn):
        status, force = function_status_from_attributes(fn)
        self.set(fn, status, status)  # Seed recursion before examining the body.
        previous = self.func_status
        self.func_status = SideEffectStatus.PURE
        self.functions.append(fn)
        self.process(fn.body)
        self.functions.pop()
        self.set(fn, status if force else self.func_status)
        self.func_status = previous

    def visit_ExternalFunc(self, fn):
        self.set(fn, function_status_from_attributes(fn)[0])

    visit_LLVMFunc = visit_ExternalFunc

    def visit_InternalFunc(self, fn):
        self.set(fn, fn.get_intrinsic_side_effect_status())

    def visit_Var(self, value):
        self.set(value, SideEffectStatus.PURE)

    visit_VarValue = visit_Var
    visit_PointerValue = visit_Var
    visit_IntConst = visit_Var
    visit_FloatConst = visit_Var
    visit_BoolConst = visit_Var
    visit_StringConst = visit_Var
    visit_BytesConst = visit_Var
    visit_CustomConst = visit_Var
    visit_StackAllocInstr = visit_Var

    def visit_SeriesFlow(self, value):
        self.set(value, max(map(self.process, value), default=SideEffectStatus.PURE))

    def visit_IfFlow(self, value):
        self.set(value, max(
            self.process(value.cond),
            self.process(value.true_branch),
            self.process(value.false_branch),
        ))

    def visit_WhileFlow(self, value):
        self.set(value, max(self.process(value.cond), self.process(value.body)))

    def handle_loop(self, value, status):
        if value.schedule:
            for operand in value.schedule.get_used_values():
                status = max(status, self.process(operand))
        self.handle_assignment(value, value.var, status)

    def visit_ForFlow(self, value):
        self.handle_loop(value, max(self.process(value.iter), self.process(value.body)))

    def visit_ImperativeForFlow(self, value):
        self.handle_loop(value, max(
            self.process(value.start), self.process(value.end), self.process(value.body)
        ))

    def visit_TryCatchFlow(self, value):
        status = max(self.process(value.body), self.process(value.else_), self.process(value.finally_))
        call_status = SideEffectStatus.PURE
        for catch in value:
            local, external = self.assignment_status(catch.var)
            status = max(status, local, self.process(catch.handler))
            call_status = max(call_status, external)
        self.set(value, status, call_status)

    def visit_PipelineFlow(self, value):
        status = call_status = SideEffectStatus.PURE
        for stage in value:
            fn = get_func(stage.callee)
            if fn:
                stage_status = self.process(fn)
                call_status = max(call_status, stage_status)
                status = max(status, stage_status)
            else:
                self.process(stage.callee)
                status = call_status = SideEffectStatus.UNKNOWN
            for arg in stage.args:
                status = max(status, self.process(arg))
        self.set(value, status, call_status)

    def visit_AssignInstr(self, value):
        self.handle_assignment(value, value.lhs, self.process(value.rhs))

    def visit_ExtractInstr(self, value):
        self.set(value, self.process(value.val))

    def visit_InsertInstr(self, value):
        self.process(value.lhs)
        self.process(value.rhs)
        fn = self.functions[-1]
        pure = all(
            not (info.extern_captures or info.modified or info.arg_captures)
            for info in self.captures.results[fn.id]
        )
        if pure:
            key = fn.id, value.lhs.id
            if key not in self.escape_purity:
                info = escapes(fn, value.lhs, self.captures)
                self.escape_purity[key] = not (info.extern_captures or info.arg_captures)
            pure = self.escape_purity[key]
        self.set(value, SideEffectStatus.UNKNOWN, SideEffectStatus.PURE if pure else SideEffectStatus.UNKNOWN)

    def visit_CallInstr(self, value):
        status = self.process(value.callee)
        for arg in value:
            status = max(status, self.process(arg))
        fn = get_func(value.callee)
        call_status = self.process(fn) if fn else SideEffectStatus.UNKNOWN
        self.set(value, max(status, call_status), call_status)

    def visit_YieldInInstr(self, value):
        self.set(value, SideEffectStatus.NO_CAPTURE)

    visit_BreakInstr = visit_YieldInInstr
    visit_ContinueInstr = visit_YieldInInstr

    def visit_TernaryInstr(self, value):
        self.set(value, max(
            self.process(value.cond), self.process(value.true_value), self.process(value.false_value)
        ))

    def visit_ReturnInstr(self, value):
        self.set(value, max(SideEffectStatus.NO_CAPTURE, self.process(value.value)))

    visit_YieldInstr = visit_ReturnInstr
    visit_AwaitInstr = visit_ReturnInstr

    def visit_ThrowInstr(self, value):
        self.process(value.value)
        self.set(value, SideEffectStatus.UNKNOWN, SideEffectStatus.NO_CAPTURE)

    def visit_FlowInstr(self, value):
        self.set(value, max(self.process(value.flow), self.process(value.value)))

    def default_visit(self, value):
        if isinstance(value, (CustomFlow, CustomInstr)):
            self.set(value, value.get_side_effect_status(True), value.get_side_effect_status(False))


@dataclass
class SideEffectResult:
    result: dict[int, SideEffectStatus]

    def has_side_effect(self, value):
        return self.result.get(value.id) != SideEffectStatus.PURE


@dataclass(init=False, eq=False)
class SideEffectAnalysis(Analysis):
    KEY = "core-analyses-side-effect"

    cap_analysis_key: str
    global_assignment_has_side_effects: bool

    def __init__(self, cap_analysis_key, global_assignment_has_side_effects):
        super().__init__()
        self.cap_analysis_key = cap_analysis_key
        self.global_assignment_has_side_effects = global_assignment_has_side_effects

    def run(self, module):
        uses = VarUseAnalyzer()
        uses.process(module)
        analyzer = SideEffectAnalyzer(
            uses, self.get_analysis_result(self.cap_analysis_key),
            self.global_assignment_has_side_effects,
        )
        analyzer.visit(module)
        return SideEffectResult(analyzer.result)
