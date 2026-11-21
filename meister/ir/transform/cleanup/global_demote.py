# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ... import BodiedFunc, Func, Var, cast
from ...util.operator import Operator
from ..pass_ import Pass


@dataclass(init=False, eq=False)
class GetUsedGlobals(Operator):
    variables: list[Var] = field(default_factory=list)

    def __init__(self):
        super().__init__()
        self.variables = []

    def pre_hook(self, value):
        self.variables.extend(
            var for var in value.get_used_variables()
            if not cast(Func, var) and var.global_ and not var.tls
        )


@dataclass(init=False, eq=False)
class GlobalDemotionPass(Pass):
    """C++: codon/cir/transform/cleanup/global_demote.cpp."""

    KEY = "core-cleanup-global-demote"

    num_demotions: int

    def __init__(self):
        super().__init__()
        self.num_demotions = 0

    def run(self, module):
        self.num_demotions = 0
        local_globals = {}
        for candidate in (module.main_func, *module):
            fn = cast(Func, candidate)
            if not fn:
                continue
            globals_ = GetUsedGlobals()
            globals_.process(fn)
            for var in globals_.variables:
                if local_globals.setdefault(var, fn) is not fn:
                    local_globals[var] = None
        for var, fn in local_globals.items():
            if fn is None or var.external or var.id in (module.argv_var.id, module.argc_var.id):
                continue
            assert var.global_
            var.global_ = False
            if body := cast(BodiedFunc, fn):
                body.push_back(var)
                self.num_demotions += 1
