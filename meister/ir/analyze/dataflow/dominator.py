# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ... import Value
from ..analysis import Analysis
from .cfg import CFBlock, CFGraph, CFResult


@dataclass(init=False, eq=False)
class DominatorInspector:
    """C++: codon/cir/analyze/dataflow/dominator.cpp."""

    cfg: CFGraph
    block_bits: dict[CFBlock, int]
    sets: dict[CFBlock, int]
    positions: dict[CFBlock, dict[Value, int]]

    def __init__(self, cfg):
        self.cfg = cfg
        self.block_bits, self.sets, self.positions = {}, {}, {}

    def analyze(self):
        self.block_bits = {block: 1 << index for index, block in enumerate(self.cfg)}
        universal = (1 << len(self.cfg.blocks)) - 1
        self.sets = dict.fromkeys(self.cfg, universal)
        self.sets[self.cfg.entry] = self.block_bits[self.cfg.entry]
        changed = True
        while changed:
            changed = False
            for block in self.cfg:
                dominators = universal if block.predecessors else 0
                for predecessor in block.predecessors:
                    dominators &= self.sets[predecessor]
                dominators |= self.block_bits[block]
                if dominators != self.sets[block]:
                    self.sets[block] = dominators
                    changed = True

    def is_dominated(self, value, dominator):
        block = self.cfg.get_block(value)
        dom_block = self.cfg.get_block(dominator)
        if block is dom_block:
            if block not in self.positions:
                positions = {}
                for index, item in enumerate(block):
                    positions.setdefault(item, index)
                self.positions[block] = positions
            positions = self.positions[block]
            # Missing entries (for example, mapped ternaries) are at the end.
            end = len(block.values)
            return positions.get(dominator, end) <= positions.get(value, end)
        return bool(self.sets[block] & self.block_bits[dom_block])


@dataclass
class DominatorResult:
    cfg_result: CFResult
    results: dict[int, DominatorInspector] = field(default_factory=dict)


@dataclass(init=False, eq=False)
class DominatorAnalysis(Analysis):
    KEY = "core-analyses-dominator"

    cf_analysis_key: str

    def __init__(self, cf_analysis_key):
        super().__init__()
        self.cf_analysis_key = cf_analysis_key

    def run(self, module):
        cfg_result = self.get_analysis_result(self.cf_analysis_key)
        result = DominatorResult(cfg_result)
        for identifier, graph in cfg_result.graphs.items():
            inspector = DominatorInspector(graph)
            inspector.analyze()
            result.results[identifier] = inspector
        return result
