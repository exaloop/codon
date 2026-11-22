# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Control-flow queries needed by the Codon LLVM passes."""

from dataclasses import dataclass
from functools import cached_property


def postorder(roots, successors, allowed):
    order = []
    visited = set()
    for root in roots:
        if root in visited:
            continue
        visited.add(root)
        stack = [(root, iter(successors[root]))]
        while stack:
            block, edges = stack[-1]
            if (edge := next(edges, None)) is None:
                order.append(block)
                stack.pop()
            elif edge in allowed and edge not in visited:
                visited.add(edge)
                stack.append((edge, iter(successors[edge])))
    return order


class DominatorTree:
    def __init__(self, entry, successors, predecessors):
        # Compute immediate dominators in reverse postorder.
        order = postorder((entry,), successors, successors)
        self.order = {block: i for i, block in enumerate(reversed(order))}
        self.parent = {entry: entry}
        changed = True
        while changed:
            changed = False
            for block in reversed(order[:-1]):
                parents = (p for p in predecessors[block] if p in self.parent)
                parent = next(parents)
                for other in parents:
                    parent = self.common_dominator(parent, other)
                if self.parent.get(block) != parent:
                    self.parent[block] = parent
                    changed = True

    def common_dominator(self, left, right):
        if left not in self.parent or right not in self.parent:
            return None
        while left != right:
            if self.order[left] > self.order[right]:
                left = self.parent[left]
            else:
                right = self.parent[right]
        return left

    def dominates(self, dominator, block):
        return dominator is not None and self.common_dominator(dominator, block) == dominator


@dataclass
class Loop:
    header: int
    blocks: set
    latches: list
    preheader: int | None
    exits: tuple
    dedicated_exits: bool


class FunctionCFG:
    def __init__(self, function):
        self.blocks = {b._as_parameter_: b for b in function.blocks()}
        self.successors = {
            key: tuple(s._as_parameter_ for s in block.successors())
            for key, block in self.blocks.items()
        }
        self.predecessors = invert(self.successors)
        self.dom = DominatorTree(next(iter(self.blocks)), self.successors, self.predecessors)
        self.loops = self.find_loops()

    def postdominators(self):
        # Join all exits at the virtual root. Blocks in regions without an exit
        # also reach it, so those regions cannot incorrectly justify a hoist.
        exits = [block for block, successors in self.successors.items() if not successors]
        reaches_exit = set(exits)
        work = exits.copy()
        while work:
            for parent in self.predecessors[work.pop()]:
                if parent not in reaches_exit:
                    reaches_exit.add(parent)
                    work.append(parent)
        root = 0
        successors = {**self.predecessors, root: [*exits, *(self.blocks.keys() - reaches_exit)]}
        return DominatorTree(root, successors, invert(successors))

    def find_loops(self):
        members = {}
        latches = {}
        for block, successors in self.successors.items():
            for header in dict.fromkeys(successors):
                if not self.dom.dominates(header, block):
                    continue
                latches.setdefault(header, []).append(block)
                loop = members.setdefault(header, {header})
                work = [block]
                while work:
                    member = work.pop()
                    if member not in loop and member in self.dom.parent:
                        loop.add(member)
                        work.extend(self.predecessors[member])
        result = []
        for header, blocks in members.items():
            outside = [p for p in self.predecessors[header] if p not in blocks]
            preheader = None
            if len(outside) == 1 and self.successors[outside[0]] == (header,):
                preheader = outside[0]
            exits = tuple(dict.fromkeys(
                successor for block in self.blocks if block in blocks
                for successor in self.successors[block] if successor not in blocks
            ))
            dedicated = all(all(pred in blocks for pred in self.predecessors[exit_]) for exit_ in exits)
            result.append(Loop(header, blocks, latches[header], preheader, exits, dedicated))
        # Inner loops appear first.
        return sorted(result, key=lambda loop: len(loop.blocks))

    def loop_for(self, block):
        return next((loop for loop in self.loops if block in loop.blocks), None)

    @cached_property
    def irreducible(self):
        # CycleInfo rejects an allocation in a cycle with multiple entries.
        # Strip the header of a reducible component to find nested cycles.
        result = set()
        entry = next(iter(self.blocks))
        work = [set(self.dom.parent)]
        while work:
            nodes = work.pop()
            order = postorder(nodes, self.successors, nodes)
            remaining = nodes.copy()
            for root in reversed(order):
                if root not in remaining:
                    continue
                component = set(postorder((root,), self.predecessors, remaining))
                remaining.difference_update(component)
                if len(component) == 1 and root not in self.successors[root]:
                    continue
                headers = [
                    block for block in component
                    if block == entry or any(
                        pred not in component and pred in self.dom.parent
                        for pred in self.predecessors[block]
                    )
                ]
                if len(headers) != 1:
                    result.update(component)
                elif len(component) > 1:
                    component.remove(headers[0])
                    work.append(component)
        return result


def invert(successors):
    predecessors = {block: [] for block in successors}
    for block, edges in successors.items():
        for edge in dict.fromkeys(edges):
            predecessors[edge].append(block)
    return predecessors
