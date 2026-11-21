# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ...util.operator import Operator
from ..analysis import Analysis


@dataclass(init=False, eq=False)
class GlobalVarAnalyzer(Operator):
    assignments: dict[int, int]

    def __init__(self):
        super().__init__()
        self.assignments = {}

    def handle_PointerValue(self, value):
        if value.var.global_:
            self.assignments[value.var.id] = -1

    def handle_AssignInstr(self, value):
        if value.lhs.global_:
            identifier = value.lhs.id
            self.assignments[identifier] = -1 if identifier in self.assignments else value.rhs.id


@dataclass
class GlobalVarsResult:
    assignments: dict[int, int]


class GlobalVarsAnalyses(Analysis):
    KEY = "core-analyses-global-vars"

    def run(self, module):
        analysis = GlobalVarAnalyzer()
        analysis.process(module)
        return GlobalVarsResult(analysis.assignments)
