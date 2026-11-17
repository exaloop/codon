# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ....bridge import dataclass
from ... import (
    FlowInstr,
    ForFlow,
    GeneratorType,
    IfFlow,
    SeriesFlow,
    Var,
    VarValue,
    YieldInstr,
    cast,
)
from ...util.cloning import CloneVisitor
from ...util.irtools import call, make_var, series
from ..pass_ import OperatorPass


# C++: codon/cir/transform/lowering/await.cpp:17
def is_future(typ):
    return typ.name.startswith("std.asyncio.Future.")


# C++: codon/cir/transform/lowering/await.cpp:21
def is_task(typ):
    return typ.name.startswith("std.asyncio.Task.")


# C++: codon/cir/transform/lowering/await.cpp:25
def is_coroutine(typ):
    return cast(GeneratorType, typ)


@dataclass(init=False, eq=False)
class AwaitLowering(OperatorPass):
    """C++: codon/cir/transform/lowering/await.h:12. Lower futures/tasks and coroutine await expressions."""

    KEY = "core-await-lowering"

    # C++: codon/cir/transform/lowering/await.cpp:31
    def handle_AwaitInstr(self, value):
        module, awaited = value.module, value.value
        typ = awaited.type
        visitor = CloneVisitor(module)
        if is_future(typ) or is_task(typ):
            result = module.get_or_realize_method(typ, "result", [typ])
            assert result, "get-result method not found"
            wait = module.get_or_realize_func("_wait_on", [typ], [], "std.asyncio")
            assert wait, "wait-on function not found"
            checkpoint = module.get_or_realize_func("_cancel_checkpoint", [], [], "std.asyncio")
            assert checkpoint, "cancel-checkpoint function not found"
            # cancel_checkpoint()
            # if _wait_on(value, future): yield; cancel_checkpoint()
            # future.result()
            prefix = module.nr(SeriesFlow)
            future = make_var(visitor.clone(awaited), prefix, self.get_parent_func())
            prefix.push_back(call(checkpoint, []))
            prefix.push_back(
                module.nr(
                    IfFlow,
                    call(wait, [module.nr(VarValue, future)]),
                    series(module.nr(YieldInstr), call(checkpoint, [])),
                )
            )
            value.replace_all(
                module.nr(FlowInstr, prefix, call(result, [module.nr(VarValue, future)]))
            )
        elif generator := is_coroutine(typ):
            promise = module.get_or_realize_func("_promise", [typ], [], "std.asyncio")
            assert promise, "promise function not found"
            # for _ in coro: yield
            # coro.__promise__()
            prefix = module.nr(SeriesFlow)
            coro = make_var(visitor.clone(awaited), prefix, self.get_parent_func())
            var = module.nr(Var, generator.base, False)
            self.get_parent_func().symbols.append(var)
            if value.generator:
                requeue = module.get_or_realize_func("_requeue", [], [], "std.asyncio")
                assert requeue, "requeue function not found"
                body = series(call(requeue, []), module.nr(YieldInstr))
            else:
                body = series(module.nr(YieldInstr))
            prefix.push_back(module.nr(ForFlow, module.nr(VarValue, coro), body, var))
            value.replace_all(
                module.nr(FlowInstr, prefix, call(promise, [module.nr(VarValue, coro)]))
            )
        else:
            assert False, f"unexpected value type '{typ.name}' in await instruction"
