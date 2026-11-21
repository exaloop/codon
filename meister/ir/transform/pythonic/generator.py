# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ....ast.types import mangle
from ... import (
    AssignInstr, BodiedFunc, BreakInstr, CallInstr, ExtractInstr, FlowInstr, ForFlow,
    FuncType, IfFlow, ImperativeForFlow, PointerValue, RecordType, ReturnInstr,
    SeriesFlow, TernaryInstr, TryCatchFlow, Value, Var, VarValue, WhileFlow, cast,
)
from ...util.cloning import CloneVisitor
from ...util.inlining import inline_call
from ...util.irtools import call, get_derived_func_name, get_func, get_var, is_no_inline, make_var, series
from ...util.operator import Operator
from ..pass_ import OperatorPass


@dataclass(init=False, eq=False)
class GeneratorSumTransformer(Operator):
    accumulator: Var
    valid: bool

    def __init__(self, accumulator):
        super().__init__()
        self.accumulator, self.valid = accumulator, True

    def handle_YieldInstr(self, value):
        module = value.module
        rhs = value.value
        if not rhs:
            self.valid = False
            return
        if rhs.type.is_type(module.get_bool_type()):
            rhs = module.nr(TernaryInstr, rhs, module.get_int(1), module.get_int(0))
        total = module.nr(VarValue, self.accumulator) + rhs
        if not total or not total.type.is_type(self.accumulator.type):
            self.valid = False
            return
        value.replace_all(module.nr(AssignInstr, self.accumulator, total))

    def handle_ReturnInstr(self, value):
        module = value.module
        ret = module.nr(ReturnInstr, module.nr(VarValue, self.accumulator))
        self.see(ret)
        value.replace_all(series(value.value, ret) if value.value else ret)

    def handle_YieldInInstr(self, value):
        self.valid = False


@dataclass(init=False, eq=False)
class GeneratorAnyAllTransformer(Operator):
    any_: bool
    valid: bool

    def __init__(self, any_):
        super().__init__()
        self.any_, self.valid = any_, True

    def handle_YieldInstr(self, value):
        module = value.module
        condition = module.get_bool_type()(value.value) if value.value else None
        if not condition:
            self.valid = False
            return
        if not self.any_:
            condition = module.nr(TernaryInstr, condition, module.get_bool(False), module.get_bool(True))
        ret = module.nr(ReturnInstr, module.get_bool(self.any_))
        self.see(ret)
        value.replace_all(module.nr(IfFlow, condition, series(ret)))

    def handle_ReturnInstr(self, value):
        if self.saw(value):
            return
        module = value.module
        ret = module.nr(ReturnInstr, module.get_bool(not self.any_))
        self.see(ret)
        value.replace_all(series(value.value, ret) if value.value else ret)

    def handle_YieldInInstr(self, value):
        self.valid = False


def clone_producer(generator, fn, signature, names):
    fn.realize(signature, names)
    remaps = {old.id: new for old, new in zip(generator.args, fn.args)}
    body = cast(SeriesFlow, CloneVisitor(fn.module).clone(generator.body, fn, remaps))
    fn.body = body
    return body


def generator_to_sum(generator, start_type, output_type, parent, site):
    if not generator or not generator.generator or not cast(FuncType, generator.type):
        return None
    module = generator.module
    signature = module.get_func_type(output_type, [*generator.type, start_type])
    name = get_derived_func_name(
        "__sum_wrapper." + generator.name + "." + signature.name, parent, site
    )
    fn = module.n(BodiedFunc, site, name)
    body = clone_producer(generator, fn, signature, [*(arg.name for arg in generator.args), "start"])

    init = module.nr(VarValue, fn.args[-1])
    if start_type.is_type(module.get_int_type()) and output_type.is_type(module.get_float_type()):
        init = module.get_float_type()(init)
    if not init or not init.type.is_type(output_type):
        module.remove(fn)
        return None

    accumulator = make_var(init, body, fn, prepend=True)
    transformer = GeneratorSumTransformer(accumulator)
    transformer.process(fn)
    body.push_back(module.nr(ReturnInstr, module.nr(VarValue, accumulator)))
    if not transformer.valid:
        module.remove(fn)
        return None
    return fn


def generator_to_any_all(generator, any_, parent, site):
    if not generator or not generator.generator:
        return None
    module = generator.module
    prefix = "__any_wrapper." if any_ else "__all_wrapper."
    fn = module.n(BodiedFunc, site, get_derived_func_name(prefix + generator.name, parent, site))
    signature = module.get_func_type(module.get_bool_type(), list(generator.type))
    body = clone_producer(generator, fn, signature, [arg.name for arg in generator.args])
    transformer = GeneratorAnyAllTransformer(any_)
    transformer.process(fn)
    body.push_back(module.nr(ReturnInstr, module.get_bool(not any_)))
    if not transformer.valid:
        module.remove(fn)
        return None
    return fn


class GeneratorArgumentOptimization(OperatorPass):
    """Specialize reducers that consume direct generator calls."""

    KEY = "core-pythonic-generator-argument-opt"

    def handle_CallInstr(self, value):
        fn = get_func(value.callee)
        parent = self.get_parent_func()
        if not fn or not value.args:
            return
        reducer = next(
            (name for name in ("sum", "any", "all")
             if fn.name.startswith(mangle("std.internal.builtin", func=name))),
            None,
        )
        if len(value.args) != (2 if reducer == "sum" else 1) or reducer is None:
            return
        producer = cast(CallInstr, value.front())
        generator = cast(BodiedFunc, get_func(producer.callee)) if producer else None
        if not generator:
            return
        args = list(producer)
        if reducer == "sum":
            start = value.back()
            replacement = generator_to_sum(generator, start.type, value.type, parent, value)
            args.append(start)
        else:
            if not value.type.is_type(value.module.get_bool_type()):
                return
            replacement = generator_to_any_all(generator, reducer == "any", parent, value)
        if replacement:
            value.replace_all(call(replacement, args))


@dataclass(init=False, eq=False)
class FusionVerifier(Operator):
    nodes: int
    yields: int
    valid: bool

    def __init__(self):
        super().__init__()
        self.nodes = self.yields = 0
        self.valid = True

    def pre_hook(self, node):
        self.nodes += 1
        self.valid &= self.nodes <= 256

    def handle_YieldInstr(self, value):
        self.valid &= bool(value.value) and not value.final
        self.yields += 1

    def handle_YieldInInstr(self, value):
        self.valid = False

    handle_AwaitInstr = handle_YieldInInstr
    handle_TryCatchFlow = handle_YieldInInstr
    handle_StackAllocInstr = handle_YieldInInstr

    def handle_PointerValue(self, value):
        self.valid &= value.var.global_

    def handle_ForFlow(self, value):
        self.valid &= not value.is_parallel() and not value.async_


@dataclass(init=False, eq=False)
class ConsumerVerifier(Operator):
    wrappers: set[int]
    valid: bool

    def __init__(self, wrappers):
        super().__init__()
        self.wrappers, self.valid = wrappers, True

    def handle_BreakInstr(self, value):
        self.valid &= value.loop is not None and value.loop.id in self.wrappers

    def handle_ContinueInstr(self, value):
        self.valid = False


@dataclass(init=False, eq=False)
class IteratorUseVerifier(Operator):
    iterator: Var
    consumer: ForFlow
    wrappers: set[int]
    assignment: AssignInstr | None
    reads: int
    writes: int
    position: int
    creation_position: int
    consumption_position: int
    address_taken: bool
    creation_loops: list[int]
    consumption_loops: list[int]

    def __init__(self, iterator, consumer, wrappers):
        super().__init__()
        self.iterator, self.consumer, self.wrappers = iterator, consumer, wrappers
        self.assignment = None
        self.reads = self.writes = self.position = 0
        self.creation_position = self.consumption_position = 0
        self.address_taken = False
        self.creation_loops, self.consumption_loops = [], []

    def enclosing_loops(self):
        result = []
        for position, node in enumerate(self.node_stack):
            node = node.get_actual()
            if (
                node is not self.consumer
                and isinstance(node, (ForFlow, WhileFlow, ImperativeForFlow, IfFlow, TryCatchFlow))
                and node.id not in self.wrappers
            ):
                result.append(node.id)
                # Branch identity keeps creation and consumption within one arm.
                if isinstance(node, (IfFlow, TryCatchFlow)) and position + 1 < len(self.node_stack):
                    if branch := cast(Value, self.node_stack[position + 1]):
                        result.append(branch.id)
        return result

    def pre_hook(self, value):
        self.position += 1
        if isinstance(value, Value) and not isinstance(value, (VarValue, PointerValue, AssignInstr)):
            self.address_taken |= any(var.id == self.iterator.id for var in value.get_used_variables())

    def handle_VarValue(self, value):
        if value.var.id == self.iterator.id:
            self.reads += 1
            self.consumption_position = self.position
            self.consumption_loops = self.enclosing_loops()

    def handle_PointerValue(self, value):
        self.address_taken |= value.var.id == self.iterator.id

    def handle_AssignInstr(self, value):
        if value.lhs.id == self.iterator.id:
            self.assignment = value
            self.writes += 1
            self.creation_position = self.position
            self.creation_loops = self.enclosing_loops()

    def valid(self):
        return (
            self.reads == self.writes == 1
            and not self.address_taken
            and self.creation_position < self.consumption_position
            and self.creation_loops == self.consumption_loops
        )


@dataclass(init=False, eq=False)
class FusionTransformer(Operator):
    consumer: ForFlow
    exit: WhileFlow

    def __init__(self, consumer, exit_):
        # Bottom-up so returns in the inserted consumer keep exiting the caller.
        super().__init__(children_first=True)
        self.consumer, self.exit = consumer, exit_

    def handle_YieldInstr(self, value):
        assignment = value.module.nr(AssignInstr, self.consumer.var, value.value)
        value.replace_all(series(assignment, self.consumer.body))

    def handle_ReturnInstr(self, value):
        module = value.module
        replacement = module.nr(SeriesFlow)
        # Discard the producer's return value while preserving its effects.
        if value.value:
            replacement.push_back(value.value)
        replacement.push_back(module.nr(BreakInstr, self.exit))
        value.replace_all(replacement)


@dataclass(init=False, eq=False)
class GeneratorLoopFusion(OperatorPass):
    """C++: codon/cir/transform/pythonic/generator.cpp."""

    KEY = "core-pythonic-generator-loop-fusion"

    wrappers: set[int]
    growth: dict[int, int]

    def __init__(self):
        super().__init__()
        self.wrappers, self.growth = set(), {}

    def run(self, module):
        self.wrappers.clear()
        self.growth.clear()
        super().run(module)

    def handle_CallInstr(self, value):
        parent = cast(BodiedFunc, self.get_parent_func())
        fn = cast(BodiedFunc, get_func(value.callee))
        if not parent or not fn or not fn.name.startswith("__sum_wrapper."):
            return
        verifier = FusionVerifier()
        verifier.process(fn.body)
        growth = self.growth.get(parent.id, 0) + verifier.nodes
        if not verifier.valid or growth > 1024:
            return
        result = inline_call(value, aggressive=True)
        if result is None:
            return
        parent.symbols.extend(result.new_vars)
        if expression := cast(FlowInstr, result.result):
            if body := cast(SeriesFlow, expression.flow):
                if wrapper := cast(WhileFlow, body.back()):
                    self.wrappers.add(wrapper.id)
        self.growth[parent.id] = growth
        value.replace_all(result.result)

    def unpack_iterator(self, loop, extract, parent):
        variable = get_var(extract.val)
        if not variable or variable.global_:
            return False
        uses = IteratorUseVerifier(variable, loop, self.wrappers)
        uses.process(parent.body)
        tuple_ = cast(CallInstr, uses.assignment.rhs) if uses.valid() else None
        constructor = get_func(tuple_.callee) if tuple_ else None
        record = cast(RecordType, constructor.parent_type) if constructor else None
        if not record or record.name != "Tuple" or constructor.unmangled_name != "__new__":
            return False
        index = cast(RecordType, extract.val.type).get_member_index(extract.field)
        if not 0 <= index < len(tuple_.args):
            return False
        # Evaluate every tuple operand at creation, including unused side effects.
        module = loop.module
        setup = module.nr(SeriesFlow)
        variables = [make_var(value, setup, parent) for value in tuple_]
        uses.assignment.replace_all(setup)
        loop.iter = module.nr(VarValue, variables[index])
        return True

    def handle_ForFlow(self, loop):
        parent = cast(BodiedFunc, self.get_parent_func())
        if loop.is_parallel() or loop.async_ or not parent:
            return
        if extract := cast(ExtractInstr, loop.iter):
            if self.unpack_iterator(loop, extract, parent):
                self.handle_ForFlow(loop)
            return

        invocation = cast(CallInstr, loop.iter)
        creation = None
        if iterator := get_var(loop.iter):
            if iterator.global_:
                return
            uses = IteratorUseVerifier(iterator, loop, self.wrappers)
            uses.process(parent.body)
            if uses.valid():
                creation = uses.assignment
                invocation = cast(CallInstr, creation.rhs)
        generator = cast(BodiedFunc, get_func(invocation.callee)) if invocation else None
        if (
            not generator or not generator.generator or not generator.body
            or generator.async_ or parent is generator or len(invocation.args) != len(generator.args)
            or is_no_inline(generator)
        ):
            return
        verifier = FusionVerifier()
        verifier.process(generator.body)
        consumer = ConsumerVerifier(self.wrappers)
        consumer.process(loop.body)
        growth = self.growth.get(parent.id, 0) + verifier.nodes
        if not verifier.valid or verifier.yields != 1 or not consumer.valid or growth > 1024:
            return
        self.growth[parent.id] = growth

        module = loop.module
        setup = module.nr(SeriesFlow)
        # A named handle evaluates producer arguments at creation, not at use.
        arguments = {arg.id: make_var(val, setup, parent) for arg, val in zip(generator.args, invocation)}
        if creation:
            creation.replace_all(setup)
            setup = module.nr(SeriesFlow)
        body = CloneVisitor(module).clone(generator.body, parent, arguments)

        wrapper = module.nr(SeriesFlow)
        exit_ = module.nr(WhileFlow, module.get_bool(True), wrapper)
        self.wrappers.add(exit_.id)
        FusionTransformer(loop, exit_).process(body)
        wrapper.push_back(body)
        wrapper.push_back(module.nr(BreakInstr, exit_))
        setup.push_back(exit_)
        loop.replace_all(setup)
