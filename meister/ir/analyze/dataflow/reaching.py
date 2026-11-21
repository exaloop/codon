# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from bisect import bisect_right
from collections import defaultdict, deque
from dataclasses import dataclass, field

from ... import AssignInstr, Instr, PointerValue, Value
from ..analysis import Analysis
from .cfg import AssignKind, CFBlock, CFGraph, CFResult, SyntheticAssignInstr


@dataclass(eq=False)
class ReachingDef:
    assignment: Instr | None
    assignee: Value | None = None

    def known(self):
        return self.assignee is not None

    @property
    def id(self):
        return (self.assignee if self.known() else self.assignment).id

    def __eq__(self, other):
        if not isinstance(other, ReachingDef):
            return NotImplemented
        return self.known() == other.known() and self.id == other.id

    def __hash__(self):
        return hash(self.id)


def get_generated(value):
    value = value.get_actual()
    if isinstance(value, AssignInstr):
        return ReachingDef(value, value.rhs)
    if isinstance(value, SyntheticAssignInstr):
        return ReachingDef(value, value.arg if value.kind == AssignKind.KNOWN else None)
    return ReachingDef(None)


@dataclass
class BlockData:
    incoming: int = 0
    positions: dict[int, int] = field(default_factory=dict)
    assignments: dict[int, list[tuple[int, Instr]]] = field(default_factory=dict)
    indexed: bool = False


@dataclass(init=False, eq=False)
class RDInspector:
    """C++: codon/cir/analyze/dataflow/reaching.cpp. Reaching definitions at CIR uses."""

    cfg: CFGraph
    invalid: set[int]
    sets: dict[CFBlock, BlockData]
    ordering: list[tuple[tuple[bool, int], ReachingDef]]
    var_masks: dict[int, int]

    def __init__(self, cfg):
        self.cfg = cfg
        self.invalid, self.sets = set(), {}
        self.ordering, self.var_masks = [], {}

    def analyze(self):
        lookup = {}
        for block in self.cfg:
            for value in block:
                value = value.get_actual()
                if isinstance(value, (AssignInstr, SyntheticAssignInstr)):
                    if value.id in lookup:
                        continue
                    bit = 1 << len(self.ordering)
                    lookup[value.id] = bit
                    definition = get_generated(value)
                    # Snapshot which definitions compare equal before a pass can
                    # replace their assignees. Keep the sets as bitsets until queried.
                    self.ordering.append(((definition.known(), definition.id), definition))
                    identifier = value.lhs.id
                    self.var_masks[identifier] = self.var_masks.get(identifier, 0) | bit
                elif isinstance(value, PointerValue):
                    # A pointer use may change the variable at any time.
                    self.invalid.add(value.var.id)

        # Build block gen and kill sets. Each variable's last assignment is generated.
        gen, kill = {}, {}
        for block in self.cfg:
            generated = 0
            killed = 0
            for value in block:
                value = value.get_actual()
                if isinstance(value, (AssignInstr, SyntheticAssignInstr)):
                    bit = lookup[value.id]
                    variable_mask = self.var_masks[value.lhs.id]
                    generated = (generated & ~variable_mask) | bit
                    killed |= variable_mask & ~bit
            gen[block], kill[block] = generated, killed
            self.sets[block] = BlockData()

        outgoing = gen.copy()
        worklist = deque(self.cfg)
        queued = set(worklist)
        while worklist:
            block = worklist.popleft()
            queued.remove(block)
            incoming = 0
            for predecessor in block.predecessors:
                incoming |= outgoing[predecessor]
            self.sets[block].incoming = incoming
            # OUT = GEN union (IN minus KILL).
            result = gen[block] | (incoming & ~kill[block])
            if outgoing[block] != result:
                outgoing[block] = result
                for successor in block.successors - queued:
                    queued.add(successor)
                    worklist.append(successor)

    def is_invalid(self, var):
        return var.id in self.invalid

    def get_reaching_definitions(self, var, location):
        if var.global_ or self.is_invalid(var):
            return []
        block = self.cfg.get_block(location)
        if block is None:
            return []
        data = self.sets[block]
        if not data.indexed:
            assignments = defaultdict(list)
            for index, value in enumerate(block):
                value = value.get_actual()
                data.positions.setdefault(value.id, index)
                if isinstance(value, (AssignInstr, SyntheticAssignInstr)):
                    assignments[value.lhs.id].append((index, value))
            data.assignments = dict(assignments)
            data.indexed = True
        limit = data.positions.get(location.id, len(block.values))
        assignments = data.assignments.get(var.id)
        if assignments:
            index = bisect_right(assignments, limit, key=lambda entry: entry[0])
            if index:
                return [get_generated(assignments[index - 1][1])]
        if block is self.cfg.entry:
            return []

        remaining = data.incoming & self.var_masks.get(var.id, 0)
        definitions = {}
        while remaining:
            bit = remaining & -remaining
            key, definition = self.ordering[bit.bit_length() - 1]
            definitions.setdefault(key, definition)
            remaining ^= bit
        return list(definitions.values())


@dataclass
class RDResult:
    cfg_result: CFResult
    results: dict[int, RDInspector] = field(default_factory=dict)


@dataclass(init=False, eq=False)
class RDAnalysis(Analysis):
    KEY = "core-analyses-rd"

    cf_analysis_key: str

    def __init__(self, cf_analysis_key):
        super().__init__()
        self.cf_analysis_key = cf_analysis_key

    def run(self, module):
        cfg_result = self.get_analysis_result(self.cf_analysis_key)
        result = RDResult(cfg_result)
        for identifier, graph in cfg_result.graphs.items():
            inspector = RDInspector(graph)
            inspector.analyze()
            result.results[identifier] = inspector
        return result
