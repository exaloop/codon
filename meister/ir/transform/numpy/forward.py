# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from collections import Counter
from dataclasses import dataclass

from ... import AssignInstr, CallInstr, Func, Value, Var, VarValue, cast
from ...analyze.module.side_effect import SideEffectResult
from ...util.operator import Operator
from .expr import NumPyExpr


@dataclass(init=False, eq=False)
class GetVars(Operator):
    identifiers: set[int]

    def __init__(self, identifiers):
        super().__init__()
        self.identifiers = identifiers

    def pre_hook(self, node):
        self.identifiers.update(var.id for var in node.get_used_variables() if not cast(Func, var))


@dataclass(init=False, eq=False)
class OkToForwardPast(Operator):
    variables: set[int]
    parsed_values: dict[int, NumPyExpr]
    effects: SideEffectResult
    ok: bool

    def __init__(self, variables, parsed_values, effects):
        super().__init__()
        self.variables, self.parsed_values, self.effects = variables, parsed_values, effects
        self.ok = True

    def pre_hook(self, value):
        if not self.ok:
            return
        if isinstance(value, AssignInstr):
            self.ok = value.lhs.id not in self.variables
        elif isinstance(value, Value):
            if parsed := self.parsed_values.get(value.id):
                self.ok = all(
                    not self.effects.has_side_effect(leaf.val)
                    for leaf in parsed.walk() if leaf.is_leaf()
                )
                # The arithmetic tree already accounts for these operands.
                for child in value.get_used_values():
                    self.see(child)
            else:
                self.ok = not self.effects.has_side_effect(value)


@dataclass(init=False, eq=False)
class GetAllUses(Operator):
    var: Var
    uses: list[Value]

    def __init__(self, var):
        super().__init__()
        self.var, self.uses = var, []

    def pre_hook(self, node):
        if isinstance(node, Value) and self.var in node.get_used_variables():
            self.uses.append(node)


def can_forward_in_block(source, destination, variables, parsed_values, effects, block):
    ready = source is None
    for value in block:
        if not ready:
            ready = value is source
            continue
        if value is destination:
            return True
        checker = OkToForwardPast(variables, parsed_values, effects)
        checker.process(value)
        if not checker.ok:
            return False
    return ready and destination is None


def can_forward_expression(unit, target, parsed_values, cfg, effects):
    variables = set()
    for leaf in unit.expr.walk():
        if leaf.is_leaf():
            if effects.has_side_effect(leaf.val):
                return False
            GetVars(variables).process(leaf.val)

    source = unit.assign
    start, end = cfg.get_block(source), cfg.get_block(target)
    assert start and end, "forwarding endpoints are missing from the CFG"
    # A consumer cannot run again without reexecuting the producer.
    pending, visited = list(end.successors), set()
    while pending:
        block = pending.pop()
        if block is start:
            continue
        if block is end:
            return False
        if block not in visited:
            visited.add(block)
            pending.extend(block.successors)

    # Every path must reach the consumer without cycles, writes, or effects.
    # Otherwise forwarding could suppress an eager shape error.
    blocks, incoming = [start], {start: 0}
    for block in blocks:
        if not can_forward_in_block(
            source if block is start else None, target if block is end else None,
            variables, parsed_values, effects, block,
        ):
            return False
        if block is end:
            continue
        if not block.successors:
            return False
        for successor in block.successors:
            if successor not in incoming:
                blocks.append(successor)
                incoming[successor] = 0
            incoming[successor] += 1
    if end not in incoming:
        return False
    ready = [block for block, count in incoming.items() if count == 0]
    for block in ready:
        if block is not end:
            for successor in block.successors:
                incoming[successor] -= 1
                if incoming[successor] == 0:
                    ready.append(successor)
    return len(ready) == len(blocks)


def can_forward_variable(assign, destination, fn, reaching):
    var = assign.lhs
    definitions = reaching.get_reaching_definitions(var, destination)
    if len(definitions) != 1 or definitions[0].assignment.id != assign.id:
        return False
    uses = GetAllUses(var)
    uses.process(fn)
    # Even repeated references in one consumer prevent forwarding.
    return not any(
        definition.assignment.id == assign.id
        for use in uses.uses if use is not destination and use.id != assign.id
        for definition in reaching.get_reaching_definitions(var, use)
    )


@dataclass
class Forwarding:
    source: object
    leaf: NumPyExpr
    source_index: int


def build_forwarding_dag(fn, reaching, cfg, effects, units):
    parsed_values = {}
    for unit in units:
        # An expanded summary still occupies the original call site in the CFG.
        parsed_values.setdefault(unit.value.id, unit.expr)
        for expr in unit.expr.walk():
            if expr.val:
                parsed_values.setdefault(expr.val.id, expr)

    dag = {}
    for destination in units:
        edges = dag[destination] = []
        variables = [
            (ref.var, leaf) for leaf in destination.expr.walk() if leaf.is_leaf()
            and (ref := cast(VarValue, leaf.val))
        ]
        occurrences = Counter(leaf.val for _, leaf in variables)
        for var, leaf in variables:
            if occurrences[leaf.val] != 1:
                continue
            for index, source in enumerate(units):
                if (
                    source is not destination and source.assign and source.assign.lhs is var
                    and not source.expr.is_reduction()
                    and can_forward_variable(source.assign, leaf.val, fn, reaching)
                    and can_forward_expression(source, leaf.val, parsed_values, cfg, effects)
                ):
                    edges.append(Forwarding(source, leaf, index))
    return dag


class UnionFind:
    def __init__(self, size):
        self.parents = list(range(size))
        self.ranks = [0] * size

    def find(self, index):
        if self.parents[index] != index:
            self.parents[index] = self.find(self.parents[index])
        return self.parents[index]

    def join(self, a, b):
        a, b = self.find(a), self.find(b)
        if a == b:
            return
        if self.ranks[a] < self.ranks[b]:
            a, b = b, a
        self.parents[b] = a
        if self.ranks[a] == self.ranks[b]:
            self.ranks[a] += 1


def components(dag, units):
    parents = UnionFind(len(units))
    for index, unit in enumerate(units):
        for edge in dag[unit]:
            parents.join(index, edge.source_index)
    groups = [{} for _ in units]
    for index, unit in enumerate(units):
        groups[parents.find(index)][unit] = dag[unit]
    return [group for group in groups if group]


def has_cycle(dag):
    active, visited = set(), set()

    def visit(unit):
        if unit in active:
            return True
        if unit in visited:
            return False
        active.add(unit)
        visited.add(unit)
        cyclic = any(visit(edge.source) for edge in dag[unit])
        active.remove(unit)
        return cyclic

    return any(visit(unit) for unit in dag)


def forwarding_root(dag):
    producers = {edge.source for edges in dag.values() for edge in edges}
    assert len(producers) == len(dag) - 1, "multiple roots in forwarding graph"
    return next(unit for unit in dag if unit not in producers)


def can_reuse_after_reads(source, leaf, consumer, units, cfg, reaching, effects):
    expr = consumer.expr
    if expr.is_reduction() or expr.depth() != 2 or leaf not in (expr.lhs, expr.rhs):
        return False
    assign, var = source.assign, source.assign.lhs
    definitions = reaching.get_reaching_definitions(var, leaf.val)
    if len(definitions) != 1 or definitions[0].assignment.id != assign.id:
        return False
    block = cfg.get_block(assign)
    positions = {}
    for value in block:
        positions.setdefault(value.id, len(positions))
    target = positions.get(leaf.val.id)
    if target is None:
        return False
    completed_reads = set()
    for reader in units:
        expr = reader.expr
        position = positions.get(reader.value.id)
        if (
            not expr.is_reduction() or not expr.lhs.is_leaf() or position is None
            or not positions[assign.id] < position < target
        ):
            continue
        operand = cast(VarValue, expr.lhs.val)
        invocation = cast(CallInstr, expr.val)
        if operand and operand.var is var and invocation and not any(
            effects.has_side_effect(arg) for arg in invocation.args[1:]
        ):
            completed_reads.add(operand.id)
    if not completed_reads:
        return False

    # Creating an alias also prevents reuse, even if it is not returned.
    uses = GetAllUses(var)
    uses.process(consumer.func)
    return not any(
        definition.assignment.id == assign.id and use.id not in completed_reads
        for use in uses.uses if use is not leaf.val and use.id != assign.id
        for definition in reaching.get_reaching_definitions(var, use)
    )


def get_forwarding_dags(fn, reaching, cfg, effects, units):
    from .fusion import has_owned_result

    dag = build_forwarding_dag(fn, reaching, cfg, effects, units)
    dags = [group for group in components(dag, units) if not has_cycle(group)]
    # Transfer ownership independently from forwarding the producer expression.
    for group in dags:
        root = forwarding_root(group)
        block = cfg.get_block(root.value)
        leaf_occurrences = Counter(
            ref.var for unit in group for leaf in unit.expr.walk() if leaf.is_leaf()
            and (ref := cast(VarValue, leaf.val))
        )
        for source in units:
            if (
                not block or not source.assign or source in group
                or leaf_occurrences[source.assign.lhs] != 1
                or not has_owned_result(source.expr) or source.assign.lhs.global_
                or cfg.get_block(source.assign) is not block
            ):
                continue
            available = False
            for value in block:
                if value is root.value:
                    break
                available |= value is source.assign
            if not available:
                continue
            for consumer in group:
                for leaf in consumer.expr.walk():
                    ref = cast(VarValue, leaf.val) if leaf.is_leaf() else None
                    if ref and ref.var is source.assign.lhs and (
                        (source.expr.is_reduction()
                         and can_forward_variable(source.assign, leaf.val, fn, reaching))
                        or (consumer is root
                            and can_reuse_after_reads(source, leaf, root, units, cfg, reaching, effects))
                    ):
                        leaf.owned_last_use = True
    return dags


def do_forwarding(dag, assignments_to_delete, substitutions):
    done = set()

    def forward(unit):
        if unit in done:
            return
        for edge in dag[unit]:
            forward(edge.source)
            # Forwarded leaves are known to have no effects.
            unit.leaves.extend(edge.source.leaves)
            substitutions.append((edge.leaf.val, edge.source.value))
            edge.leaf.replace(edge.source.expr)
            assignments_to_delete.append(edge.source.assign)
        done.add(unit)

    root = forwarding_root(dag)
    forward(root)
    return root
