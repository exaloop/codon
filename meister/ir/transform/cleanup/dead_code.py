# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ... import BoolConst, IntConst, SeriesFlow, cast
from ...util.cloning import CloneVisitor
from ..pass_ import OperatorPass


@dataclass(init=False, eq=False)
class DeadCodeCleanupPass(OperatorPass):
    """C++: codon/cir/transform/cleanup/dead_code.cpp."""

    KEY = "core-cleanup-dce"

    side_effects_key: str
    num_replacements: int

    def __init__(self, side_effects_key):
        super().__init__()
        self.side_effects_key = side_effects_key
        self.num_replacements = 0

    def run(self, module):
        self.num_replacements = 0
        super().run(module)

    def replace(self, value, replacement):
        self.num_replacements += 1
        value.replace_all(replacement)

    def handle_SeriesFlow(self, value):
        effects = self.get_analysis_result(self.side_effects_key)
        remaining = [child for child in value if effects.has_side_effect(child)]
        self.num_replacements += len(value.series) - len(remaining)
        value.series[:] = remaining

    def handle_FlowInstr(self, value):
        flow = cast(SeriesFlow, value.flow)
        if flow and not flow.series:
            self.replace(value, value.value)

    def handle_IfFlow(self, value):
        cond = cast(BoolConst, value.cond)
        if cond:
            branch = value.true_branch if cond.val else value.false_branch
            replacement = (
                CloneVisitor(value.module).clone(branch) if branch else value.module.nr(SeriesFlow)
            )
            self.replace(value, replacement)

    def handle_WhileFlow(self, value):
        cond = cast(BoolConst, value.cond)
        if cond and not cond.val:
            self.replace(value, value.module.nr(SeriesFlow))

    def handle_ImperativeForFlow(self, value):
        start, end = cast(IntConst, value.start), cast(IntConst, value.end)
        if start and end and (
            (value.step < 0 and start.val <= end.val) or (value.step > 0 and start.val >= end.val)
        ):
            self.replace(value, value.module.nr(SeriesFlow))

    def handle_TernaryInstr(self, value):
        cond = cast(BoolConst, value.cond)
        if cond:
            branch = value.true_value if cond.val else value.false_value
            self.replace(value, CloneVisitor(value.module).clone(branch))
