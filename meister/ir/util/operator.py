# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ...bridge import List, Set, dataclass
from ..base import Node, cast
from ..flow import SeriesFlow
from ..func import BodiedFunc, Func
from ..module import Module
from ..value import Value
from .visitor import Visitor


@dataclass(init=False, eq=False)
class Operator(Visitor):
    """C++: codon/cir/util/operator.h:26. Visit all values in a module."""

    # IDs of previously visited nodes; node stack and series iterator stack.
    seen: Set[int]
    node_stack: List[Node]
    it_stack: List[object]
    children_first: bool

    # C++: codon/cir/util/operator.h:43
    def __init__(self, children_first=False):
        """
        Constructs an operator.
        @param childrenFirst true if children should be visited first
        """
        self.seen, self.node_stack, self.it_stack = set(), [], []
        self.children_first = children_first

    # C++: codon/cir/util/operator.h:51
    def pre_hook(self, node):
        """
        Applied to every node before handling it.

        This function is applied to all nodes before handling the node
        itself. It provides a way to write one function that gets
        applied to every visited node.
        @param node the node
        """

    # C++: codon/cir/util/operator.h:56
    def post_hook(self, node):
        """
        Applied to every node after handling it.

        This function is applied to all nodes after handling the node
        itself. It provides a way to write one function that gets
        applied to every visited node.
        @param node the node
        """

    # C++: codon/cir/util/operator.h:58
    def visit(self, node):
        node = node.get_actual()
        if isinstance(node, Module):
            self.node_stack.extend((node, node.main_func))
            self.process(node.main_func)
            self.node_stack.pop()
            # C++'s list iterators continue to visit globals appended by a pass.
            idx = 0
            while idx < len(node.vars):
                self.node_stack.append(node.vars[idx])
                self.process(node.vars[idx])
                self.node_stack.pop()
                idx += 1
            self.node_stack.pop()
            return
        if isinstance(node, BodiedFunc):
            if node.body:
                self.seen.add(node.body.id)
                self.process(node.body)
            return
        if not isinstance(node, Value):
            return self.default_visit(node)
        if self.children_first:
            self.process_children(node)
        self.pre_hook(node)
        handler = getattr(self, "handle_" + type(node).__name__, self.handle)
        handler(node)
        self.post_hook(node)
        if not self.children_first:
            self.process_children(node)

    # C++: codon/cir/util/operator.h:85
    def handle(self, node):
        pass

    # C++: codon/cir/util/operator.h:61
    def process(self, node):
        return node.accept(self)

    # C++: codon/cir/util/operator.h:128
    def get_parent(self, level=0, cls=Node):
        """
        Return the parent of the current node.
        @param level the number of levels up from the current node
        """
        return cast(cls, self.node_stack[-level - 1])

    # C++: codon/cir/util/operator.h:132
    def depth(self):
        """
        @return current depth in the tree
        """
        return len(self.node_stack)

    # C++: codon/cir/util/operator.h:136
    def find_last(self, cls):
        """
        @tparam Desired the desired type
        @return the last encountered example of the desired type
        """
        return next(
            (
                actual
                for node in reversed(self.node_stack)
                if (actual := cast(cls, node)) is not None
            ),
            None,
        )

    # C++: codon/cir/util/operator.h:144
    def get_parent_func(self):
        """
        @return the last encountered function
        """
        return self.find_last(Func)

    # C++: codon/cir/util/operator.h:153
    def saw(self, value):
        """
        @param v the value
        @return whether we have visited ("seen") the given value
        """
        return value.id in self.seen

    # C++: codon/cir/util/operator.h:156
    def see(self, value):
        """
        Avoid visiting the given value in the future.
        @param v the value
        """
        self.seen.add(value.id)

    # C++: codon/cir/util/operator.h:160
    def insert_before(self, value):
        """
        Insert before the current position in the last seen SeriesFlow.

        Inserts the new value before the current position in the last seen SeriesFlow.
        @param v the new value
        """
        flow, current = self.it_stack[-1]
        idx = flow.series.index(current)
        flow.series.insert(idx, value)
        return idx

    # C++: codon/cir/util/operator.h:165
    def insert_after(self, value):
        """
        Insert after the current series position, marking the new value seen.

        Inserts the new value after the current position in the last seen SeriesFlow.
        @param v the new value, which is marked seen
        """
        flow, current = self.it_stack[-1]
        self.see(value)
        idx = flow.series.index(current) + 1
        flow.series.insert(idx, value)
        return idx

    # C++: codon/cir/util/operator.h:174
    def reset(self):
        """
        Resets the operator.
        """
        self.seen.clear()
        self.node_stack.clear()
        self.it_stack.clear()

    # C++: codon/cir/util/operator.h:181
    def process_children(self, node):
        if isinstance(node, SeriesFlow):
            self.node_stack.append(node)
            idx = 0
            while idx < len(node.series):
                # Identity, rather than a list index, models stable std::list
                # iterators when insert_before/after mutate the series.
                current = node.series[idx]
                self.it_stack.append((node, current))
                self.process(current)
                self.it_stack.pop()
                idx = node.series.index(current) + 1
            self.node_stack.pop()
            return
        self.node_stack.append(node)
        # Calls visit arguments first, then callee; snapshot operands before
        # handlers can replace values or realize more functions.
        for child in list(node.get_used_values()):
            if child.id in self.seen:
                continue
            self.seen.add(child.id)
            self.process(child)
        self.node_stack.pop()
