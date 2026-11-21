# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ....ast.types import Stdlib
from ... import AssignInstr, BodiedFunc, CallInstr, FlowInstr, SeriesFlow, Value, Var, VarValue, cast
from ...attribute import ListLiteralAttribute
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, make_var
from ..pass_ import OperatorPass
from .str import inspect_addition


def is_list(value):
    return value.type.name.startswith(Stdlib.List + "[")


@dataclass
class ElementHandler:
    # Evaluate each expression once, in source order. Then count the total
    # length and append each piece to the preallocated result.
    elements: list[Value]
    vars: list[Var] = field(default_factory=list, init=False)

    def setup(self, block, parent):
        self.vars.extend(make_var(value, block, parent) for value in self.elements)

    def refs(self, module):
        return [module.nr(VarValue, var) for var in self.vars]

    @staticmethod
    def get(value, typ):
        if not value.type.is_type(typ):
            return None
        if access := cast(CallInstr, value):
            fn = get_func(access.callee)
            if (
                fn and fn.unmangled_name == "__getitem__" and len(access.args) == 2
                and is_list(access.front())
                and access.back().type.name == Stdlib.Slice + "[Int[64],Int[64],Int[64]]"
            ):
                return SliceHandler(list(access))
        literal = value.get_attribute(ListLiteralAttribute)
        if literal and not any(element.star for element in literal.elements):
            return LiteralHandler([element.value for element in literal.elements])
        return DefaultHandler([value])


class DefaultHandler(ElementHandler):
    def length(self, module):
        typ = self.elements[0].type
        fn = module.get_or_realize_method(typ, "_list_add_opt_default_len", [typ])
        assert fn, "could not find default list length helper"
        return call(fn, self.refs(module))

    def append(self, result):
        module, typ = result.module, result.type
        fn = module.get_or_realize_method(typ, "_list_add_opt_default_append", [typ, typ])
        assert fn, "could not find default list append helper"
        return call(fn, [result, *self.refs(module)])


class SliceHandler(ElementHandler):
    def length(self, module):
        element, slice_ = self.elements
        fn = module.get_or_realize_method(
            element.type, "_list_add_opt_slice_len", [element.type, slice_.type]
        )
        assert fn, "could not find slice list length helper"
        return call(fn, self.refs(module))

    def append(self, result):
        module, typ = result.module, result.type
        fn = module.get_or_realize_method(
            typ, "_list_add_opt_slice_append", [typ, typ, self.elements[1].type]
        )
        assert fn, "could not find slice list append helper"
        return call(fn, [result, *self.refs(module)])


class LiteralHandler(ElementHandler):
    def length(self, module):
        return module.get_int(len(self.elements))

    def append(self, result):
        module, typ = result.module, result.type
        block = module.nr(SeriesFlow)
        if self.vars:
            fn = module.get_or_realize_method(
                typ, "_list_add_opt_literal_append", [typ, self.elements[0].type]
            )
            assert fn, "could not find literal list append helper"
            for ref in self.refs(module):
                block.push_back(call(fn, [result, ref]))
        return block


class ListAdditionOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/list.cpp."""

    KEY = "core-pythonic-list-addition-opt"

    def handle_CallInstr(self, value):
        fn = get_func(value.callee)
        if not fn or fn.unmangled_name != "__add__":
            return
        parent = cast(BodiedFunc, self.get_parent_func())
        args = []
        if not parent or not inspect_addition(value, is_list, args) or len(args) <= 1:
            return
        module, typ = parent.module, args[0].type
        clone = CloneVisitor(module)
        handlers = [ElementHandler.get(clone.clone(arg), typ) for arg in args]
        if any(handler is None for handler in handlers):
            return

        block = module.nr(SeriesFlow)
        length = make_var(module.get_int(0), block, parent)
        for handler in handlers:
            handler.setup(block, parent)
        for handler in handlers:
            total = module.nr(VarValue, length) + handler.length(module)
            block.push_back(module.nr(AssignInstr, length, total))

        fn = module.get_or_realize_method(typ, "_list_add_opt_opt_new", [module.get_int_type()])
        assert fn, "could not find list new helper"
        result = make_var(call(fn, [module.nr(VarValue, length)]), block, parent)
        for handler in handlers:
            block.push_back(handler.append(module.nr(VarValue, result)))
        value.replace_all(module.nr(FlowInstr, block, module.nr(VarValue, result)))
