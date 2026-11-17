# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

import copy

from ....bridge import dataclass
from ... import (
    AssignInstr,
    CallInstr,
    ExtractInstr,
    ImperativeForFlow,
    IntConst,
    SeriesFlow,
    Var,
    VarValue,
    cast,
)
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func, make_var
from ..pass_ import OperatorPass


# C++: codon/cir/transform/lowering/imperative.cpp:17
def get_range_iter(iterator):
    """C++: imperative.cpp:18. Recognize range(...).__iter__()."""
    from ....ast.types import Stdlib

    module = iterator.module
    iter_call = cast(CallInstr, iterator)
    if not iter_call or len(iter_call.args) != 1:
        return None
    fn = get_func(iter_call.callee)
    if not fn or fn.unmangled_name != module.ITER_MAGIC_NAME:
        return None
    range_call = cast(CallInstr, iter_call.args[0])
    if not range_call:
        return None
    constructor = get_func(range_call.callee)
    if not constructor or constructor.unmangled_name != module.NEW_MAGIC_NAME:
        return None
    parent, typ = constructor.parent_type, module.get_or_realize_type(Stdlib.Range)
    if not parent or not typ or parent.name != typ.name:
        return None
    return range_call


# C++: codon/cir/transform/lowering/imperative.cpp:44
def get_list_iter(iterator):
    from ....ast.types import Stdlib

    call = cast(CallInstr, iterator)
    if not call or len(call.args) != 1:
        return None
    fn = get_func(call.callee)
    if not fn or fn.unmangled_name != iterator.module.ITER_MAGIC_NAME:
        return None
    value = call.args[0]
    return value if value.type.name.startswith(Stdlib.List + "[") else None


@dataclass(init=False, eq=False)
class ImperativeForFlowLowering(OperatorPass):
    """C++: codon/cir/transform/lowering/imperative.h:12. Range/list iterator to integer loop lowering."""

    KEY = "core-imperative-for-lowering"

    # C++: codon/cir/transform/lowering/imperative.cpp:63
    def handle_ForFlow(self, value):
        module = value.module
        schedule = copy.copy(value.schedule) if value.is_parallel() else None
        if range_call := get_range_iter(value.iter):
            visitor = CloneVisitor(module)
            args = range_call.args
            match len(args):
                case 1:
                    start, end, step = module.get_int(0), visitor.clone(args[0]), 1
                case 2:
                    start, end, step = visitor.clone(args[0]), visitor.clone(args[1]), 1
                case 3:
                    start, end = visitor.clone(args[0]), visitor.clone(args[1])
                    constant = cast(IntConst, args[2])
                    if not constant:
                        return
                    step = constant.val
                case _:
                    assert False, "unknown range constructor"
            if step == 0:
                return
            value.replace_all(
                module.n(
                    ImperativeForFlow, value, start, step, end, value.body, value.var, schedule
                )
            )
        elif list_ := get_list_iter(value.iter):
            # for a in list: body becomes:
            #   v = list; n = v._len; p = v._ptr
            #   imp_for i in range(0, n, 1): a = p[i]; body
            parent = self.get_parent_func()
            prefix = module.n(SeriesFlow, value)
            list_var = make_var(list_, prefix, parent)
            len_var = make_var(
                module.nr(ExtractInstr, module.nr(VarValue, list_var), "_len"), prefix, parent
            )
            ptr_var = make_var(
                module.nr(ExtractInstr, module.nr(VarValue, list_var), "_ptr"), prefix, parent
            )
            body = cast(SeriesFlow, value.body)
            assert body, f"loop body is not a series flow [{value.info}]"
            var = module.nr(Var, module.get_int_type())
            parent.symbols.append(var)
            replacement = module.n(
                ImperativeForFlow,
                value,
                module.get_int(0),
                1,
                module.nr(VarValue, len_var),
                body,
                var,
                schedule,
            )
            prefix.push_back(replacement)
            body.series.insert(
                0,
                module.nr(
                    AssignInstr, value.var, module.nr(VarValue, ptr_var)[module.nr(VarValue, var)]
                ),
            )
            value.replace_all(prefix)
