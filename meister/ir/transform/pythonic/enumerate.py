# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ....ast.types import mangle
from ... import (
    AssignInstr, BodiedFunc, CallInstr, ExtractInstr, FlowInstr, ForFlow, GeneratorType,
    ImperativeForFlow, ReturnInstr, SeriesFlow, Value, Var, VarValue, cast,
)
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, get_var, is_call_of, make_var
from ...util.operator import Operator
from ..pass_ import OperatorPass


@dataclass(init=False, eq=False)
class TupleUseChecker(Operator):
    loop: ForFlow
    index: Value
    element: Value
    valid: bool

    def __init__(self, loop, index, element):
        super().__init__()
        self.loop, self.index, self.element = loop, index, element
        self.valid = True

    def pre_hook(self, node):
        if node not in (self.loop, self.index, self.element):
            if any(var.id == self.loop.var.id for var in node.get_used_variables()):
                self.valid = False


def get_unpack(value, tuple_, field):
    assignment = cast(AssignInstr, value)
    extract = cast(ExtractInstr, assignment.rhs) if assignment else None
    var = get_var(extract.val) if extract else None
    return assignment if var and var.id == tuple_.id and extract.field == field else None


class EnumerateOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/enumerate.cpp."""

    KEY = "core-pythonic-enumerate-opt"

    def handle_ForFlow(self, loop):
        # A shared counter assumes serial, synchronous iteration and a local target.
        if loop.is_parallel() or loop.async_ or loop.var.global_:
            return
        iterator = cast(CallInstr, loop.iter)
        fn = get_func(iterator.callee) if iterator else None
        if (
            not fn or len(iterator.args) != 2
            or not fn.name.startswith(mangle("std.internal.builtin", func="enumerate"))
        ):
            return

        parent = cast(BodiedFunc, self.get_parent_func())
        body = cast(SeriesFlow, loop.body)
        if not parent or not body or len(body.series) < 2:
            return
        index = get_unpack(body.series[0], loop.var, "item1")
        element_assignment = get_unpack(body.series[1], loop.var, "item2")
        if not index or not element_assignment:
            return

        # Reads after the loop also keep the yielded tuple alive.
        uses = TupleUseChecker(
            loop, cast(ExtractInstr, index.rhs).val, cast(ExtractInstr, element_assignment.rhs).val
        )
        uses.process(parent.body)
        if not uses.valid:
            return

        module = loop.module
        iterable, start = iterator.args
        if not start.type.is_type(module.get_int_type()):
            return
        is_array = iterable.type.name.startswith(mangle("std.numpy.ndarray", cls="ndarray") + "[")
        len_fn = iter_fn = iter_expr = None
        if is_array:
            len_fn = module.get_or_realize_method(iterable.type, "__len__", [iterable.type])
            if not len_fn:
                return
        elif not cast(GeneratorType, iterable.type):
            # The frontend helper preserves virtual dispatch and default arguments.
            iter_fn = cast(BodiedFunc, module.get_or_realize_func(
                "iter", [iterable.type], [], "std.internal.builtin"
            ))
            iter_body = cast(SeriesFlow, iter_fn.body) if iter_fn else None
            if not iter_body or len(iter_body.series) != 1:
                return
            result = cast(ReturnInstr, iter_body.front())
            if not result or not result.value or not cast(GeneratorType, result.value.type):
                return
            iter_expr = result.value

        # Evaluate both inputs once, and advance before the user's body so
        # continue cannot skip the increment.
        setup = module.nr(SeriesFlow)
        source = make_var(iterable, setup, parent)
        counter = make_var(start, setup, parent)
        element = module.nr(Var, element_assignment.lhs.type)
        parent.push_back(element)
        index.rhs = module.nr(VarValue, counter)
        element_assignment.rhs = module.nr(VarValue, element)
        advance = module.nr(VarValue, counter) + module.get_int(1)
        body.insert(2, module.nr(AssignInstr, counter, advance))

        if is_array:
            # The zero-based offset preserves ndarray strides and row views.
            # Fetch before assigning the visible targets in case indexing throws.
            offset = module.nr(Var, module.get_int_type())
            parent.push_back(offset)
            length = call(len_fn, [module.nr(VarValue, source)])
            end = module.nr(FlowInstr, setup, length)
            item = module.nr(VarValue, source)[module.nr(VarValue, offset)]
            body.insert(0, module.nr(AssignInstr, element, item))
            loop.replace_all(
                module.n(ImperativeForFlow, loop, module.get_int(0), 1, end, body, offset)
            )
        else:
            iterator = module.nr(VarValue, source)
            if iter_expr:
                iterator = CloneVisitor(module).clone(iter_expr, parent, {iter_fn.args[0].id: source})
            # Preserve direct __iter__ calls for list lowering. With virtual calls,
            # setup must run before the vtable lookup as well as the receiver.
            iter_call = cast(CallInstr, iterator)
            if is_call_of(iterator, "__iter__", 1) and get_var(iter_call.front()) is source:
                iter_call.front().replace_all(
                    module.nr(FlowInstr, setup, module.nr(VarValue, source))
                )
            else:
                iterator = module.nr(FlowInstr, setup, iterator)
            loop.iter, loop.var = iterator, element
