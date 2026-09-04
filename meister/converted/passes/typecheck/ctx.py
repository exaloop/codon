# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
from __future__ import annotations

from ....bridge import Dict, List, Set, contextmanager, dataclass
from ... import ast, cache


class TypecheckError(ast.NodeError):
    pass


@dataclass(init=False)
class Item:
    """
    Typecheck context identifier.
    Can be either a function, a class (type), or a variable.
    """

    # Unique identifier (canonical name)
    canonical_name: str = ""
    # Base name (e.g., foo.bar.baz)
    base_name: str = ""
    # Full module name
    module_name: str = ""
    # Type
    type: ast.types.Type | None = None
    # Information about number of nested conditionals (blocks).
    block_level: int = 0
    # Specifies at which time the name was added to the context.
    # Used to prevent using later definitions early (can happen in
    # advanced type checking iterations).
    time: int = 0
    # Set if an identifier is a class or a function generic
    generic: bool = False
    # Points to another type in case this type is not useful
    # [used within isinstance blocks].
    alternative: Item | None = None
    info: ast.Node.SrcInfo

    def __init__(
        self,
        canonical_name: str = "",
        base_name: str = "",
        module_name: str = "",
        type: ast.types.Type | None = None,
        block_level: int = 0,
        time: int = 0,
        generic: bool = False,
        alternative: Item | None = None,
        info: ast.Node.SrcInfo | None = None,
    ):
        self.canonical_name = canonical_name
        self.base_name = base_name
        self.module_name = module_name
        self.type = type
        self.block_level = block_level
        self.time = time
        self.generic = generic
        self.alternative = alternative
        self.info = info or ast.Node.SrcInfo()
        self.__post_init__()

    def is_var(self):
        return not self.generic and not self.is_func() and not self.is_type()

    def is_func(self):
        return self.type.get_func() is not None

    def is_type(self):
        return self.type.is_type(ast.types.Stdlib.Type)

    def is_global(self):
        """True if we are at the toplevel."""
        return self.block_level == 0 and not self.base_name

    def is_conditional(self):
        """
        True if an identifier is within a conditional block
        (i.e., a block that might not be executed during the runtime)
        True if we are within a conditional block.
        """
        return self.block_level > 0

    def get_static_kind(self) -> ast.types.Type.Behaviour:
        return self.type.get_static_kind()


@dataclass(init=False)
class Base:
    """
    Holds the information about current base.
    A base is defined as a function or a class block.
    """

    @dataclass
    class LoopData:
        break_var: str = ""
        # False if a loop has continue/break statement.
        # Used for flattening static loops.
        flat: bool = True

    @dataclass
    class DeduceData:
        # Set if the base is class base and if class is marked with @deduce.
        # Stores the list of class fields in the order of traversal.
        deduced_members: List[str] | None = None
        # Canonical name of `self` parameter that is used to deduce class fields
        # (e.g., self in self.foo).
        self_name: str = ""

    # Canonical name of a function or a class that owns this base.
    # Map of captured identifiers (i.e., identifiers not defined in a function).
    # Captured (canonical) identifiers are mapped to the new canonical names
    # (representing the canonical function argument names that are appended to the
    name: str = ""
    # Function type
    type: ast.types.Type | None = None
    # The return type of currently realized function
    return_type: ast.types.Type | None = None
    # Typechecking iteration
    iteration: int = 0
    # Only set for functions.
    func: ast.FunctionStmt | None = None
    suite: ast.Stmt | None = None
    # Index of the parent base
    parent: int = 0
    deduce: DeduceData
    # Map of identifiers that are to be fetched from Python.
    py_captures: Set[str] | None = None

    # A stack of nested loops enclosing the current statement used for transforming
    # "break" statement in loop-else constructs. Each loop is defined by a "break"
    # variable created while parsing a loop-else construct. If a loop has no else
    # block, the corresponding loop variable is empty.
    loops: List[LoopData]
    pending_defaults: Dict[int, Set[ast.types.Type]]

    def __init__(
        self,
        name: str = "",
        type: ast.types.Type | None = None,
        return_type: ast.types.Type | None = None,
        iteration: int = 0,
        func: ast.FunctionStmt | None = None,
        suite: ast.Stmt | None = None,
        parent: int = 0,
        deduce: DeduceData | None = None,
        py_captures: Set[str] | None = None,
        loops: List[LoopData] | None = None,
        pending_defaults: Dict[int, Set[ast.types.Type]] | None = None,
    ):
        self.name = name
        self.type = type
        self.return_type = return_type
        self.iteration = iteration
        self.func = func
        self.suite = suite
        self.parent = parent
        self.deduce = deduce or Base.DeduceData()
        self.py_captures = py_captures
        self.loops = [] if loops is None else loops
        self.pending_defaults = {} if pending_defaults is None else pending_defaults

    def get_loop(self) -> LoopData | None:
        return None if not self.loops else self.loops[-1]

    def is_type(self):
        return self.func is None


@dataclass(init=False)
class TypeContext:
    """Context class that tracks identifiers during the typechecking."""

    # The absolute path of the current module.
    filename: str = ""
    # Maps a identifier to a stack of objects that share the same identifier.
    # Each object is represented by a nesting level and a pointer to that object.
    # Top of the stack is the current block; the bottom is the outer-most block.
    # Stack is represented as std::deque to allow iteration and access to the outer-most
    # block.
    map: Dict[str, List[Item]]
    # Stack of blocks and their corresponding identifiers.
    # Top of the stack is the current block.
    stack: List[List[str]]
    # Set of current context flags.
    flags: Set[str]
    # SrcInfo stack used for obtaining source information of the current expression.
    node_stack: List[ast.Node]

    # A pointer to the shared cache.
    cache: cache.Cache
    # Current base stack (the last enclosing base is the last base in the stack).
    bases: List[Base]
    # Current module. The default module is named `__main__`.
    module_name: cache.Import.File
    # Set if the standard library is currently being loaded.
    is_stdlib_loading: bool = False
    # The current type-checking level (for type instantiation and generalization).
    typecheck_level: int = 0
    changed_nodes: int = 0
    # Number of nested realizations. Used to prevent infinite instantiations.
    realization_depth: int = 0
    # Number of nested blocks (0 for toplevel)
    block_level: int = 0
    # True if an early return is found (anything afterwards won't be typechecked)
    return_early: bool = False
    # Stack of static loop control variables (used to emulate goto statements).
    static_loops: List[str]
    # Current statement time.
    time: int = 0
    # Type to be expected upon completed typechecking.
    expected_type: ast.types.Type | None = None
    auto_python: bool = False
    # True if no side-effects allowed
    simple_types: bool = False
    global_shadows: Dict[str, int]
    iteration_times: Dict[str, float]

    def __init__(
        self,
        cache: cache.Cache,
        filename: str = "",
        map: Dict[str, List[Item]] | None = None,
        stack: List[List[str]] | None = None,
        flags: Set[str] | None = None,
        node_stack: List[ast.Node] | None = None,
        bases: List[Base] | None = None,
        module_name: cache.Import.File | None = None,
        is_stdlib_loading: bool = False,
        typecheck_level: int = 0,
        changed_nodes: int = 0,
        realization_depth: int = 0,
        block_level: int = 0,
        return_early: bool = False,
        static_loops: List[str] | None = None,
        time: int = 0,
        expected_type: ast.types.Type | None = None,
        auto_python: bool = False,
        simple_types: bool = False,
        global_shadows: Dict[str, int] | None = None,
        iteration_times: Dict[str, float] | None = None,
    ):
        self.cache = cache
        self.filename = filename
        self.map = {} if map is None else map
        self.stack = [] if stack is None else stack
        self.flags = set() if flags is None else flags
        self.node_stack = [] if node_stack is None else node_stack
        self.bases = [Base()] if bases is None else bases
        self.module_name = module_name or cache.Import.File(cache.Import.File.Status.External)
        self.is_stdlib_loading = is_stdlib_loading
        self.typecheck_level = typecheck_level
        self.changed_nodes = changed_nodes
        self.realization_depth = realization_depth
        self.block_level = block_level
        self.return_early = return_early
        self.static_loops = [] if static_loops is None else static_loops
        self.time = time
        self.expected_type = expected_type
        self.auto_python = auto_python
        self.simple_types = simple_types
        self.global_shadows = {} if global_shadows is None else global_shadows
        self.iteration_times = {} if iteration_times is None else iteration_times

        self.push_node(ast.NoneExpr())

    def add(self, name: str, variable: Item):
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

    def find_all(self, name: str) -> List[Item] | None:
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

    def add_item(
        self,
        name: str,
        canonical_name: str,
        typ: ast.types.Type,
        time: int = 0,
        info: ast.Node.SrcInfo | None = None,
    ) -> Item:
        """Convenience method for adding an object to the context."""

        assert canonical_name, f"empty canonical name for '{name}'"
        item = Item(
            canonical_name,
            self.get_base_name(),
            self.get_module(),
            typ,
            self.block_level,
            info=info,
            time=time,
        )
        self.add(name, item)
        self.add_always_visible(item)
        return item

    def add_toplevel(self, name: str, item: Item):
        """Convenience method for adding an object to the context."""
        self.map.setdefault(name, []).insert(0, item)
        return item

    def add_always_visible(self, item: Item, pop: bool = False):
        """
        Add the item to the standard library module, thus ensuring its visibility from all
        modules.
        """
        self.add(item.canonical_name, item)
        if pop:
            self.stack[0].pop()  # do not remove it later!
        if self.cache.type_ctx and not self.cache.type_ctx.find(item.canonical_name):
            self.cache.type_ctx.add(item.canonical_name, item)
            if pop:
                self.cache.type_ctx.stack[0].pop()
            if item.canonical_name not in self.cache.reverse_identifier_lookup:
                self.cache.reverse_identifier_lookup[item.canonical_name] = item.canonical_name
        return item

    def find_at(self, name: str, time: int = 0, in_base: str | None = None) -> Item | None:
        # def find(self, name: str) -> Item | None:
        #     """Return a top-most object with a given identifier or nullptr if it does not exist."""
        #     values = self.map.get(name)
        #     return values[0] if values is not None else None

        values = self.map.get(name)
        is_mangled = "." in name
        base = in_base or self.get_base_name()
        if values:
            for item in values:
                if not is_mangled and not base.startswith(item.get_base_name()):
                    continue
                if (
                    is_mangled
                    or item.get_base_name() != base
                    or time == 0
                    or item.get_module() != self.get_module()
                ):
                    return item  # avoid middle realizations
                if item.get_time() <= time:
                    return item

        # Item is not found in the current module. Time to look in the standard library!
        # Note: the standard library items cannot be dominated.
        stdlib_ctx = self.cache.imports.get(cache.STDLIB_IMPORT).ctx
        if stdlib_ctx is not self and (values := stdlib_ctx.map.get(name, None)):
            return values[0]
        # Maybe we are looking for a canonical identifier?
        type_ctx = self.cache.type_ctx
        if type_ctx is not self:
            return type_ctx.find(type_ctx, name)
        return None

    def force_find(self, name: str) -> Item:
        """
        Get an item that exists in the context. If the item does not exist, assertion is
        raised.
        """
        item = self.find(name)
        assert item is not None, f"cannot find '{name}'"
        return item

    def get_base_name(self) -> str:
        return self.bases[-1].name

    def get_module(self) -> str:
        base = "std." if self.module_name.status is cache.Import.File.Status.StdLibrary else ""
        base += self.module_name.module
        base = base.removeprefix("__main__")
        return base

    def get_module_path(self) -> str:
        """Return the current module path."""
        return self.module_name.path

    def generate_canonical_name(
        self, name: str, include_base: bool = False, no_suffix: bool = False
    ) -> str:
        """Generate a unique identifier (name) for a given string."""
        new_name = name
        if "." in name:
            return name
        include_base = include_base and not name.startswith("%")
        if include_base:
            base = self.get_base_name()
            if not base:
                base = self.get_module()
            if base == "std.internal.core":
                no_suffix = True
                base = ""
            new_name = ("" if not base else f"{base}.") + new_name
        number = self.cache.identifier_count.get(new_name, 0)
        self.cache.identifier_count[new_name] = number + 1
        if not no_suffix:
            new_name = f"{new_name}.{number}"
        if name != new_name:
            self.cache.identifier_count[new_name] = self.cache.identifier_count.get(new_name, 0) + 1
        self.cache.reverse_identifier_lookup[new_name] = name
        return new_name

    def is_global(self):
        return len(self.bases) == 1

    def is_conditional(self):
        return self.block_level > 0

    def get_base(self) -> Base | None:
        """Get the current base."""
        return None if not self.bases else self.bases[-1]

    def in_function(self):
        """True if the current base is function."""
        return not self.is_global() and not self.bases[-1].is_type()

    def in_class(self):
        """True if the current base is class."""
        return not self.is_global() and self.bases[-1].is_type()

    def is_outer(self, value: Item):
        """True if an item is defined outside of the current base or a module."""
        return (
            self.get_base_name() != value.get_base_name() or self.get_module() != value.get_module()
        )

    def get_class_base(self) -> Base | None:
        """Get the enclosing class base (or nullptr if such does not exist)."""
        if len(self.bases) >= 2 and self.bases[-2].is_type():
            return self.bases[-2]
        return None

    def get_realization_depth(self):
        """Get the current realization depth (i.e., the number of nested realizations)."""
        return len(self.bases)

    def get_realization_stack_name(self) -> str:
        """Get the name of the current realization stack (e.g., `fn1:fn2:...`)."""
        if not self.bases:
            return ""
        names: List[str] = []
        for base in self.bases:
            if base.type is not None:
                names.append(base.type.realized_name())
        return ":".join(names)

    def dump(self, pad: int = 0):
        print(f"current module: {self.module_name.module} ({self.module_name.path})")
        base = self.get_base()
        print(f"current base:   {self.get_realization_stack_name()} / {base.name}")
        for name, items in sorted(self.map.items()):
            item = items[0]
            print(f"{' ' * (pad * 2)}{name:.<25}")
            print(
                "   ... kind:      "
                f"{int(item.is_type()) * 100 + int(item.is_func()) * 10 + int(item.is_var())}"
            )
            print(f"   ... canonical: {item.canonical_name}")
            print(f"   ... base:      {item.base_name}")
            print(f"   ... module:    {item.module_name}")
            print(
                "   ... type:      "
                + ("<null>" if item.type is None else item.type.debug_string(2))
            )
            print(f"   ... gnrc/sttc: {item.generic} / {int(item.get_static_kind().value)}")

    @contextmanager
    def within_base(self, name: str):
        self.bases.append(Base(name=name))
        self.add_block()
        yield
        self.pop_block()
        self.bases.pop()

    @contextmanager
    def substitute(self, name, value):
        old = getattr(self, name)
        setattr(self, name, value)
        yield
        setattr(self, name, old)
