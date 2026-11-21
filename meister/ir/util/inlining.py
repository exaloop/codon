# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from .. import AssignInstr, BodiedFunc, BreakInstr, FlowInstr, FuncType, SeriesFlow, Var, VarValue, WhileFlow, cast
from .cloning import CloneVisitor
from .irtools import get_func
from .operator import Operator


@dataclass(init=False, eq=False)
class ReturnVerifier(Operator):
    need_loop: bool

    def __init__(self):
        super().__init__()
        self.need_loop = False

    def handle_ReturnInstr(self, value):
        if self.need_loop:
            return
        previous = None
        for parent in self.node_stack:
            series = cast(SeriesFlow, parent)
            if not series or (previous and previous.back().id != series.id):
                self.need_loop = True
                return
            previous = series
        self.need_loop = previous is None or previous.back().id != value.id


@dataclass(init=False, eq=False)
class ReturnReplacer(Operator):
    implicit_loop: WhileFlow | None
    var: Var | None
    clone: CloneVisitor

    def __init__(self, implicit_loop, var, clone):
        super().__init__()
        self.implicit_loop, self.var, self.clone = implicit_loop, var, clone

    def handle_ReturnInstr(self, value):
        module = value.module
        replacement = module.n(SeriesFlow, value)
        if self.var:
            replacement.push_back(module.n(AssignInstr, value, self.var, self.clone.clone(value.value)))
        elif value.value:
            replacement.push_back(self.clone.clone(value.value))
        if self.implicit_loop:
            replacement.push_back(module.n(BreakInstr, value, self.implicit_loop))
        value.replace_all(replacement)


@dataclass
class InlineResult:
    result: object
    new_vars: list[Var]


def inline_function(fn, args, aggressive=False, info=None):
    """C++: codon/cir/util/inlining.cpp."""
    fn = cast(BodiedFunc, fn)
    signature = cast(FuncType, fn.type) if fn else None
    if not signature or len(args) != len(fn.args):
        return None
    module = fn.module
    if info is None:
        from ...ast.nodes import Node

        info = Node.SrcInfo()

    clone = CloneVisitor(module)
    flow = module.n(SeriesFlow, info, fn.name + "_inlined")
    variables = []
    for parameter, argument in zip(fn.args, args):
        var = clone.force_clone(parameter)
        variables.append(var)
        flow.push_back(module.n(AssignInstr, info, var, clone.clone(argument)))
    variables.extend(clone.force_clone(var) for var in fn)

    result = None
    if not signature.return_type.is_type(module.get_none_type()):
        result = module.n(Var, info, signature.return_type)
        variables.append(result)

    body = clone.clone(fn.body)
    verifier = ReturnVerifier()
    verifier.process(body)
    if verifier.need_loop and not aggressive:
        return None

    implicit = None
    if verifier.need_loop:
        wrapper = module.n(SeriesFlow, info)
        implicit = module.n(WhileFlow, info, module.get_bool(True), wrapper)
        wrapper.push_back(body)
        if result is None:
            wrapper.push_back(module.n(BreakInstr, info, implicit))

    ReturnReplacer(implicit, result, clone).process(body)
    flow.push_back(implicit or body)
    if result is not None:
        flow = module.n(FlowInstr, info, flow, module.n(VarValue, info, result))
    return InlineResult(flow, variables)


def inline_call(value, aggressive=False):
    return inline_function(get_func(value.callee), value.args, aggressive, value.info)
