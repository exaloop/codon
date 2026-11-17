# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ....bridge import dataclass
from ... import AssignInstr, BreakInstr, IfFlow, SeriesFlow, VarValue, WhileFlow, YieldInstr
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_return_type, make_var, series
from ..pass_ import OperatorPass


@dataclass(init=False, eq=False)
class AsyncForLowering(OperatorPass):
    """C++: codon/cir/transform/lowering/async_for.h:12. Lower an async loop through scheduler operations."""

    KEY = "core-async-for-lowering"

    # C++: codon/cir/transform/lowering/async_for.cpp:15
    def handle_ForFlow(self, value):
        if not value.async_:
            return
        module, coro = value.module, value.iter
        visitor = CloneVisitor(module)
        promise = module.get_or_realize_func("_promise", [coro.type], [], "std.asyncio")
        assert promise, "promise func not found"
        done = module.get_or_realize_func("_done", [coro.type], [], "std.asyncio")
        assert done, "done func not found"
        resume = module.get_or_realize_func("_resume", [coro.type], [], "std.asyncio")
        assert resume, "resume func not found"
        current = module.get_or_realize_func("_curr_task", [], [], "std.asyncio")
        assert current, "curr-task func not found"
        waiting = module.get_or_realize_func(
            "_is_waiting", [get_return_type(current)], [], "std.asyncio"
        )
        assert waiting, "is-waiting func not found"
        # task = curr_task()
        # if not coro.__done__():
        #   while True:
        #     coro.__resume__()
        #     if coro.__done__(): break
        #     if is_waiting(task): yield
        #     else: i = coro.__promise__(); <BODY>
        prefix = module.nr(SeriesFlow)
        task_var = make_var(call(current, []), prefix, self.get_parent_func())
        coro_var = make_var(visitor.clone(coro), prefix, self.get_parent_func())
        body = series(
            call(resume, [module.nr(VarValue, coro_var)]),
            module.nr(
                IfFlow, call(done, [module.nr(VarValue, coro_var)]), series(module.nr(BreakInstr))
            ),
            module.nr(
                IfFlow,
                call(waiting, [module.nr(VarValue, task_var)]),
                series(module.nr(YieldInstr)),
                series(
                    module.nr(
                        AssignInstr, value.var, call(promise, [module.nr(VarValue, coro_var)])
                    ),
                    visitor.clone(value.body),
                ),
            ),
        )
        prefix.push_back(
            module.nr(
                IfFlow,
                ~call(done, [module.nr(VarValue, coro_var)]),
                series(module.nr(WhileFlow, module.get_bool(True), body)),
            )
        )
        value.replace_all(prefix)
