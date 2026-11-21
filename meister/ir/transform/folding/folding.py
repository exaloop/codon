# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ..cleanup.canonical import CanonicalizationPass
from ..cleanup.dead_code import DeadCodeCleanupPass
from ..cleanup.global_demote import GlobalDemotionPass
from ..pass_ import PassGroup
from .const_fold import FoldingPass
from .const_prop import ConstPropPass


@dataclass(init=False, eq=False)
class FoldingPassGroup(PassGroup):
    """C++: codon/cir/transform/folding/folding.cpp."""

    KEY = "core-folding-pass-group"

    global_demotion: GlobalDemotionPass | None
    canonicalization: CanonicalizationPass
    folding: FoldingPass
    dead_code: DeadCodeCleanupPass

    def __init__(self, side_effects_key, reaching_def_key, global_vars_key, repeat, run_global_demotion):
        self.global_demotion = GlobalDemotionPass() if run_global_demotion else None
        self.canonicalization = CanonicalizationPass(side_effects_key)
        self.folding = FoldingPass()
        self.dead_code = DeadCodeCleanupPass(side_effects_key)
        passes = [self.global_demotion] if self.global_demotion else []
        passes.extend([
            ConstPropPass(reaching_def_key, global_vars_key),
            self.canonicalization,
            self.folding,
            self.dead_code,
        ])
        super().__init__(repeat, passes)

    def should_repeat(self, count):
        return super().should_repeat(count) and bool(
            (self.global_demotion and self.global_demotion.num_demotions)
            or self.canonicalization.num_replacements
            or self.folding.num_replacements
            or self.dead_code.num_replacements
        )
