# C++ comment: codon/parser/ctx.h:1
# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
from __future__ import annotations

from ..bridge import Dict, List, Set, contextmanager, dataclass
from . import ast


@dataclass(init=False)
class Context[T]:
    """
    A variable table (transformation context).
    Base class that holds a list of existing identifiers and their block hierarchy.
    """

    # The absolute path of the current module.
    filename: str = ""
    # Maps a identifier to a stack of objects that share the same identifier.
    # Each object is represented by a nesting level and a pointer to that object.
    # Top of the stack is the current block; the bottom is the outer-most block.
    # Stack is represented as std::deque to allow iteration and access to the outer-most
    # block.
    map: Dict[str, List[T]]
    # Stack of blocks and their corresponding identifiers.
    # Top of the stack is the current block.
    stack: List[List[str]]
    # Set of current context flags.
    flags: Set[str]
    # SrcInfo stack used for obtaining source information of the current expression.
    node_stack: List[ast.Node]

    def __init__(
        self,
        filename: str = "",
        map: Dict[str, List[T]] | None = None,
        stack: List[List[str]] | None = None,
        flags: Set[str] | None = None,
        node_stack: List[ast.Node] | None = None,
    ):
        self.filename = filename
        self.map = {} if map is None else map
        self.stack = [[]] if stack is None else stack
        self.flags = set() if flags is None else flags
        self.node_stack = [] if node_stack is None else node_stack

    def add(self, name: str, variable: T):
        """Add an object to the top of the stack."""
        assert name, "adding an empty identifier"
        self.map.setdefault(name, []).insert(0, variable)
        self.stack[0].append(name)

    def remove(self, name: str):
        """Remove the top-most object with a given identifier."""
        self.remove_from_map(name)
        for block in self.stack:
            if name in block:
                block.remove(name)
                return

    def find(self, name: str) -> T | None:
        """Return a top-most object with a given identifier or nullptr if it does not exist."""
        values = self.map.get(name)
        return values[0] if values is not None else None

    def find_all(self, name: str) -> List[T] | None:
        """Return all objects that share a common identifier or nullptr if it does not exist."""
        return self.map.get(name)

    def add_block(self):
        """Add a new block (i.e. adds a stack level)."""
        self.stack.insert(0, [])

    def pop_block(self):
        """Remove the top-most block and all variables it holds."""
        for name in self.stack[0]:
            self.remove_from_map(name)
        self.stack.pop(0)

    def get_block(self) -> List[str]:
        return self.stack[0].copy()

    def remove_from_top_stack(self, name: str):
        if name in self.stack[0]:
            self.stack[0].remove(name)

    def __iter__(self):
        yield from self.map.items()

    def remove_from_map(self, name: str):
        """Remove an identifier from the map only."""
        values = self.map.get(name)
        if values is None:
            return
        assert values, f"identifier {name} not found in the map"
        values.pop(0)
        if not values:
            del self.map[name]

    @contextmanager
    def substitute(self, name, value):
        old = getattr(self, name)
        setattr(self, name, value)
        yield
        setattr(self, name, old)
