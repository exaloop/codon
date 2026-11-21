# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field
from enum import Enum, auto

from ... import BodiedFunc, CallInstr, ForFlow, Instr, SeriesFlow, Value, Var, VarValue, cast
from ...base import IdMixin
from ...dsl import CustomFlow, CustomInstr
from ...util.visitor import ConstVisitor
from ..analysis import Analysis


class AssignKind(Enum):
    UNKNOWN = auto()
    KNOWN = auto()
    NEXT_VALUE = auto()
    ADD = auto()


@dataclass(init=False, eq=False)
class SyntheticInstr(Instr):
    source: object

    def __init__(self, source):
        super().__init__()
        self.source = source


@dataclass(init=False, eq=False)
class SyntheticAssignInstr(SyntheticInstr):
    lhs: Var
    kind: AssignKind
    arg: Value | None
    diff: int

    def __init__(self, source, lhs, arg=None, kind=AssignKind.KNOWN, diff=0):
        super().__init__(source)
        self.lhs, self.kind, self.arg, self.diff = lhs, kind, arg, diff

    def _get_used_values(self):
        return [self.arg] if self.arg is not None else []

    def _get_used_variables(self):
        return [self.lhs]

    def _replace_used_value(self, identifier, new):
        if self.arg is not None and self.arg.id == identifier:
            self.arg = new
            return 1
        return 0

    def _replace_used_variable(self, identifier, new):
        if self.lhs.id == identifier:
            self.lhs = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class SyntheticPhiInstr(SyntheticInstr):
    preds: list[tuple["CFBlock", Value]]

    def __init__(self, source, preds):
        super().__init__(source)
        self.preds = list(preds)

    def _get_used_values(self):
        return [value for _, value in self.preds]

    def _replace_used_value(self, identifier, new):
        count = 0
        for index, (block, value) in enumerate(self.preds):
            if value.id == identifier:
                self.preds[index] = (block, new)
                count += 1
        return count


@dataclass(init=False, eq=False)
class CFBlock(IdMixin):
    graph: "CFGraph"
    name: str
    values: list[Value]
    successors: set["CFBlock"]
    predecessors: set["CFBlock"]

    def __init__(self, graph, name):
        super().__init__()
        self.graph, self.name = graph, name
        self.values, self.successors, self.predecessors = [], set(), set()

    def get_actual(self):
        return self

    def __iter__(self):
        return iter(self.values)

    def append(self, value):
        self.values.append(value)
        self.graph.value_locations[value.id] = self

    def add_successor(self, block):
        self.successors.add(block)
        block.predecessors.add(self)


@dataclass(init=False, eq=False)
class CFGraph:
    func: BodiedFunc
    blocks: list[CFBlock]
    current: CFBlock
    value_mapping: dict[int, Value]
    value_locations: dict[int, CFBlock]
    synthetic_vars: list[Var]

    def __init__(self, func):
        self.func = func
        self.blocks, self.value_mapping, self.value_locations = [], {}, {}
        self.synthetic_vars = []
        self.current = self.new_block("entry")

    def __iter__(self):
        return iter(self.blocks)

    @property
    def entry(self):
        return self.blocks[0]

    def new_block(self, name, make_current=False):
        block = CFBlock(self, name)
        self.blocks.append(block)
        if make_current:
            self.current = block
        return block

    def new_node(self, cls, *args):
        node = cls(*args)
        node.module = self.func.module
        if isinstance(node, Var):
            self.synthetic_vars.append(node)
        else:
            self.value_mapping[node.id] = node
        return node

    def get_block(self, value):
        value = self.value_mapping.get(value.id, value)
        return self.value_locations.get(value.id)

    def get_value(self, identifier):
        value = self.value_mapping.get(identifier)
        return self.func.module.get_value(identifier) if value is None else value

    def remap_value(self, original, replacement):
        self.value_mapping[original.id] = replacement


@dataclass
class CFResult:
    graphs: dict[int, CFGraph] = field(default_factory=dict)


@dataclass
class Loop:
    next_iteration: CFBlock
    end: CFBlock
    loop_id: int
    try_catch_index: int


@dataclass(init=False, eq=False)
class CFVisitor(ConstVisitor):
    """C++: codon/cir/analyze/dataflow/cfg.cpp. Construct CFGs without changing CIR."""

    graph: CFGraph
    try_catch_stack: list[tuple[CFBlock, CFBlock | None]]
    seen: set[int]
    loop_stack: list[Loop]

    def __init__(self, graph):
        self.graph = graph
        self.try_catch_stack, self.seen, self.loop_stack = [], set(), []

    def process(self, value):
        if value is not None and value.id not in self.seen:
            self.seen.add(value.id)
            self.visit(value)

    def default_visit(self, value):
        assert not isinstance(value, (CustomFlow, CustomInstr)), "DSL CFG builders are unavailable"
        self.default_insert(value)

    def default_insert(self, value):
        graph = self.graph
        if self.try_catch_stack:
            original = graph.current
            block = graph.new_block("default", True)
            original.add_successor(block)
            block.add_successor(self.try_catch_stack[-1][0])
        graph.current.append(value)
        self.seen.add(value.id)

    def default_jump(self, target, try_catch_index=-1):
        current = len(self.try_catch_stack) - 1
        nearest_finally = next(
            (block for _, block in self.try_catch_stack[try_catch_index + 1:] if block),
            None,
        ) if current > try_catch_index else None
        if nearest_finally is not None:
            self.graph.current.add_successor(self.try_catch_stack[-1][0])
        if target is not None:
            (nearest_finally or self.graph.current).add_successor(target)

    def visit_BodiedFunc(self, fn):
        graph = self.graph
        for arg in fn.args:
            value = graph.new_node(VarValue, arg)
            graph.current.append(graph.new_node(SyntheticAssignInstr, fn, arg, value))
        self.process(fn.body)

    def visit_SeriesFlow(self, value):
        for child in value:
            self.process(child)

    def visit_IfFlow(self, value):
        graph = self.graph
        self.process(value.cond)
        original = graph.current
        end = graph.new_block("endIf")
        true_branch = graph.new_block("trueBranch", True)
        self.process(value.true_branch)
        graph.current.add_successor(end)
        false_branch = end
        if value.false_branch:
            false_branch = graph.new_block("falseBranch", True)
            self.process(value.false_branch)
            graph.current.add_successor(end)
        original.add_successor(true_branch)
        original.add_successor(false_branch)
        graph.current = end

    def enter_loop(self, start, end, value):
        self.loop_stack.append(Loop(start, end, value.id, len(self.try_catch_stack) - 1))

    def visit_WhileFlow(self, value):
        graph = self.graph
        original = graph.current
        end = graph.new_block("endWhile")
        begin = graph.new_block("whileBegin", True)
        original.add_successor(begin)
        self.process(value.cond)
        condition = graph.current
        condition.add_successor(end)
        self.enter_loop(begin, end, value)
        condition.add_successor(graph.new_block("whileBody", True))
        self.process(value.body)
        self.loop_stack.pop()
        graph.current.add_successor(begin)
        graph.current = end

    def begin_for(self, value):
        if value.is_parallel():
            for used in value.schedule.get_used_values():
                self.process(used)
        graph = self.graph
        original = graph.current
        end = graph.new_block("endFor")
        begin = graph.new_block("forBegin", True)
        original.add_successor(begin)
        return begin, end

    def visit_ForFlow(self, value):
        graph = self.graph
        _, end = self.begin_for(value)
        self.process(value.iter)
        check = graph.new_block("forCheck")
        graph.current.add_successor(check)
        check.add_successor(end)
        next_iteration = graph.new_block("forNext")
        check.add_successor(next_iteration)
        next_iteration.append(
            graph.new_node(SyntheticAssignInstr, value, value.var, value.iter, AssignKind.NEXT_VALUE)
        )
        self.enter_loop(check, end, value)
        next_iteration.add_successor(graph.new_block("forBody", True))
        self.process(value.body)
        graph.current.add_successor(check)
        self.loop_stack.pop()
        graph.current = end

    def visit_ImperativeForFlow(self, value):
        graph = self.graph
        begin, end = self.begin_for(value)
        begin.append(graph.new_node(SyntheticAssignInstr, value, value.var, value.start))
        self.process(value.start)
        self.process(value.end)
        check = graph.new_block("forCheck")
        graph.current.add_successor(check)
        check.add_successor(end)
        update = graph.new_block("forUpdate")
        update.append(
            graph.new_node(SyntheticAssignInstr, value, value.var, None, AssignKind.ADD, value.step)
        )
        update.add_successor(check)
        self.enter_loop(check, end, value)
        check.add_successor(graph.new_block("forBody", True))
        self.process(value.body)
        graph.current.add_successor(check)
        self.loop_stack.pop()
        graph.current = end

    def visit_TryCatchFlow(self, value):
        graph = self.graph
        route = graph.new_block("tcRoute")
        end = graph.new_block("tcEnd")
        else_block = graph.new_block("tcElse") if value.else_ else None
        finally_block = graph.new_block("tcFinally") if value.finally_ else None
        destination = finally_block or end

        self.try_catch_stack.append((route, finally_block))
        self.process(value.body)
        graph.current.add_successor(else_block or destination)
        for catch in value:
            block = graph.new_block("catch", True)
            if catch.var is not None:
                block.append(
                    graph.new_node(SyntheticAssignInstr, value, catch.var, None, AssignKind.UNKNOWN)
                )
            self.process(catch.handler)
            route.add_successor(block)
            graph.current.add_successor(destination)
        if else_block is not None:
            graph.current = else_block
            self.process(value.else_)
            graph.current.add_successor(destination)
        self.try_catch_stack.pop()
        if finally_block is not None:
            graph.current = finally_block
            self.process(value.finally_)
            graph.current.add_successor(end)
            route.add_successor(finally_block)
        if self.try_catch_stack:
            (finally_block or route).add_successor(self.try_catch_stack[-1][0])
        graph.current = end

    def visit_PipelineFlow(self, value):
        graph = self.graph
        stages = value.stages

        def lower(index, last):
            if index == len(stages):
                return last
            stage, previous = stages[index], stages[index - 1]
            var = graph.new_node(Var, previous.get_output_element_type()) if previous.generator else None
            previous_value = graph.new_node(VarValue, var) if var is not None else last
            call = graph.new_node(
                CallInstr, stage.callee,
                [previous_value if arg is None else arg for arg in stage.args],
            )
            result = lower(index + 1, call)
            if var is not None:
                body = graph.new_node(SeriesFlow)
                body.push_back(result)
                return graph.new_node(ForFlow, last, body, var)
            return result

        if stages:
            self.process(lower(1, stages[0].callee))

    def visit_AssignInstr(self, value):
        self.process(value.rhs)
        self.default_insert(value)

    def visit_ExtractInstr(self, value):
        self.process(value.val)
        self.default_insert(value)

    def visit_InsertInstr(self, value):
        self.process(value.lhs)
        self.process(value.rhs)
        self.default_insert(value)

    def visit_CallInstr(self, value):
        self.process(value.callee)
        for arg in value:
            self.process(arg)
        self.default_insert(value)

    def visit_TernaryInstr(self, value):
        graph = self.graph
        end = graph.new_block("ternaryDone")
        true_branch = graph.new_block("ternaryTrue")
        false_branch = graph.new_block("ternaryFalse")
        self.process(value.cond)
        graph.current.add_successor(true_branch)
        graph.current.add_successor(false_branch)
        graph.current = true_branch
        self.process(value.true_value)
        graph.current.add_successor(end)
        graph.current = false_branch
        self.process(value.false_value)
        graph.current.add_successor(end)
        phi = graph.new_node(
            SyntheticPhiInstr, value,
            [(true_branch, value.true_value), (false_branch, value.false_value)],
        )
        end.append(phi)
        graph.remap_value(value, phi)
        graph.current = end

    def find_loop(self, value):
        if value.loop is None:
            return self.loop_stack[-1]
        return next(loop for loop in self.loop_stack if loop.loop_id == value.loop.id)

    def visit_BreakInstr(self, value):
        loop = self.find_loop(value)
        self.default_jump(loop.end, loop.try_catch_index)
        self.default_insert(value)

    def visit_ContinueInstr(self, value):
        loop = self.find_loop(value)
        self.default_jump(loop.next_iteration, loop.try_catch_index)
        self.default_insert(value)

    def visit_ReturnInstr(self, value):
        self.process(value.value)
        self.default_jump(None)
        self.default_insert(value)

    def visit_YieldInstr(self, value):
        self.process(value.value)
        self.default_insert(value)

    visit_AwaitInstr = visit_YieldInstr
    visit_ThrowInstr = visit_YieldInstr

    def visit_FlowInstr(self, value):
        self.process(value.flow)
        self.process(value.value)
        self.default_insert(value)


def build_cf_graph(fn):
    graph = CFGraph(fn)
    CFVisitor(graph).process(fn)
    return graph


class CFAnalysis(Analysis):
    KEY = "core-analyses-cfg"

    def run(self, module):
        return CFResult({
            fn.id: build_cf_graph(fn)
            for value in (module.main_func, *module)
            if (fn := cast(BodiedFunc, value)) is not None
        })
