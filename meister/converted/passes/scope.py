from __future__ import annotations

import copy

from ...bridge import Dict, Enum, List, Set, contextmanager, dataclass
from .. import ast
from ..cache import Cache
from ..ctx import Context


class ScopeError(ast.NodeError):
    pass


@dataclass(init=False)
class Bindings(ast.Node.Attribute):
    class Scope(Enum):
        Read = 1
        Global = 2
        Nonlocal = 3

    @dataclass
    class Binding:
        name: str = ""
        count: int = 0
        is_nonlocal: bool = False

    captures: Dict[str, Scope]
    bindings: Dict[str, Binding]
    local_renames: Dict[str, str]

    def __init__(
        self,
        captures: Dict[str, Scope] | None = None,
        bindings: Dict[str, Binding] | None = None,
        local_renames: Dict[str, str] | None = None,
        **kwargs
    ):
        super().__init__(**kwargs)
        self.captures = {} if captures is None else captures
        self.bindings = {} if bindings is None else bindings
        self.local_renames = {} if local_renames is None else local_renames


@dataclass(init=False)
class ScopeContext(Context):
    @dataclass
    class Block:
        id: int
        suite: ast.Stmt | None = None
        # List of variables "seen" before their assignment within a loop.
        # Used to dominate variables that are updated within a loop.
        seen_names: Set[str] | None = None

    @dataclass(init=False)
    class Item:
        binding: ast.Node | None = None
        # Current hierarchy of conditional blocks.
        scope: List[int]
        ignore: bool = False
        # List of scopes where the identifier is accessible without __used__ check
        access_checked: List[List[int]]

        def __init__(
            self,
            binding: ast.Node | None = None,
            scope: List[int] | None = None,
            ignore: bool = False,
            access_checked: List[List[int]] | None = None,
        ):
            self.binding = binding
            self.scope = [] if scope is None else scope
            self.ignore = ignore
            self.access_checked = [] if access_checked is None else access_checked

    cache: Cache
    scope: List[ScopeContext.Block]
    map: Dict[str, List[ScopeContext.Item]]
    captures: Dict[str, Bindings.Scope]
    child_captures: Dict[str, Bindings.Scope]
    first_seen: Dict[str, ast.Node]
    class_deduce: tuple[str, Set[str]]
    assignment: ast.Node | None = None
    function_scope: ast.FunctionStmt | None = None
    in_class: bool = False
    renames: List[Dict[str, str]]
    temp_scope: bool = False
    # Time to track positions of assignments and references to them.
    time: int = 0

    def __init__(
        self,
        cache: Cache,
        scope: List[ScopeContext.Block],
        map: Dict[str, List[ScopeContext.Item]] | None = None,
        captures: Dict[str, Bindings.Scope] | None = None,
        child_captures: Dict[str, Bindings.Scope] | None = None,
        first_seen: Dict[str, ast.Node] | None = None,
        class_deduce: tuple[str, Set[str]] | None = None,
        assignment: ast.Node | None = None,
        function_scope: ast.FunctionStmt | None = None,
        in_class: bool = False,
        renames: List[Dict[str, str]] | None = None,
        temp_scope: bool = False,
        time: int = 0,
    ):
        self.cache = cache
        self.scope = scope
        self.map = {} if map is None else map
        self.captures = {} if captures is None else captures
        self.child_captures = {} if child_captures is None else child_captures
        self.first_seen = {} if first_seen is None else first_seen
        self.class_deduce = ("", set()) if class_deduce is None else class_deduce
        self.assignment = assignment
        self.function_scope = function_scope
        self.in_class = in_class
        self.renames = [{}] if renames is None else renames
        self.temp_scope = temp_scope
        self.time = time

    def get_scope(self) -> List[int]:
        return [block.id for block in self.scope]


def is_inside(inner, outer):
    return len(inner) >= len(outer) and inner[len(outer) - 1] == outer[-1]


@dataclass(init=False)
class ScopingVisitor(ast.NodeVisitor):
    ctx: ScopeContext

    def __init__(self, ctx: ScopeContext):
        self.ctx = ctx

    @contextmanager
    def conditional(self, suite=None, block_id: int = -1):
        # Holds the information about current scope.
        # A scope is defined as a stack of conditional blocks
        # (i.e., blocks that might not get executed during the runtime).
        # Used mainly to support Python's variable scoping rules.

        if block_id == -1:
            block_id = self.ctx.cache.block_count
            self.ctx.cache.block_count += 1
        self.ctx.scope.append(ScopeContext.Block(block_id, suite))
        yield
        assert self.ctx.scope and (self.ctx.scope[-1].id == 0 or len(self.ctx.scope) > 1), (
            "empty scope"
        )
        self.ctx.scope.pop()

    def visit(self, node):
        if node:
            if isinstance(node, ast.Stmt):
                self.ctx.time += 1
                node.set(ast.Attr.ExprTime, self.ctx.time)
            return super().visit(node)

    def visit_IdExpr(self, node: ast.IdExpr):
        if self.ctx.assignment and self.ctx.temp_scope:
            self.ctx.renames[-1][node.value] = self.ctx.cache.get_temporary_var(node.value)
        for renames in reversed(self.ctx.renames):
            if node.value in renames:
                node.value = renames[node.value]
                break
        if self.ctx.assignment:
            self.bind_name(node.value, node, source=self.ctx.assignment)
            # Disallow __used__ checks for bindings
            node.set(ast.Attr.ExprNoUndefCheck)
        else:
            self.read_name(node.value, node)

    def visit_DotExpr(self, node: ast.DotExpr):
        # Disable assignment in all cases (to handle a.x, y = b)
        match node.expr:
            case ast.IdExpr(value=value) if value == self.ctx.class_deduce[0]:
                self.ctx.class_deduce[1].add(node.member)
        with self.ctx.substitute("assignment", None):
            self.generic_visit(node)

    def visit_IndexExpr(self, node: ast.IndexExpr):
        # Disable assignment in all cases (to handle a[x], y = b)
        with self.ctx.substitute("assignment", None):
            self.generic_visit(node)

    def visit_GeneratorExpr(self, node: ast.GeneratorExpr):
        with self.ctx.substitute("temp_scope", True):
            self.ctx.renames.append({})
            try:
                self.visit(node.final_expr())
            finally:
                self.ctx.renames.pop()

    def visit_IfExpr(self, node: ast.IfExpr):
        self.visit(node.cond)
        with self.conditional():
            self.visit(node.ifexpr)
        with self.conditional():
            self.visit(node.elsexpr)

    def visit_BinaryExpr(self, node: ast.BinaryExpr):
        self.visit(node.lexpr)
        if node.op in ("&&", "||"):
            with self.conditional():
                self.visit(node.rexpr)
        else:
            self.visit(node.rexpr)

    def visit_AssignExpr(self, node: ast.AssignExpr):
        assert isinstance(node.var, ast.IdExpr), "only simple assignment expressions are supported"
        with self.ctx.substitute("temp_scope", False):
            self.visit(node.expr)
            with self.ctx.substitute("assignment", node):
                self.visit(node.var)

    def visit_LambdaExpr(self, node: ast.LambdaExpr):
        inner = ScopeContext(
            self.ctx.cache,
            scope=[ScopeContext.Block(0)],
            function_scope=ast.FunctionStmt(name="lambda"),
            renames=copy.deepcopy(self.ctx.renames),
        )
        visitor = ScopingVisitor(ctx=inner)
        for arg in node.items:
            visitor.bind_name(arg.name.lstrip("*"), arg, source=node)
            if arg.default:
                self.visit(arg.default)
        inner.scope.pop()

        suite = ast.SuiteStmt()
        inner.scope.append(ScopeContext.Block(0, suite))
        visitor.visit(node.expr)
        visitor.process_child_captures()
        inner.scope.pop()

        attr = Bindings(captures=copy.deepcopy(inner.captures))
        for name, values in inner.map.items():
            attr.bindings[name] = Bindings.Binding(name, len(values))
        self.ctx.child_captures.update(inner.captures)
        node.set(ast.Attr.Bindings, attr)

    def visit_AssignStmt(self, node: ast.AssignStmt):
        self.visit(node.rhs)
        self.visit(node.type_expr)
        with self.ctx.substitute("assignment", node):
            self.visit(node.lhs)

    def visit_IfStmt(self, node: ast.IfStmt):
        self.visit(node.cond)
        with self.conditional(node.if_suite):
            self.visit(node.if_suite)
        with self.conditional(node.else_suite):
            self.visit(node.else_suite)

    def visit_MatchStmt(self, node: ast.MatchStmt):
        self.visit(node.expr)
        for item in node.items:
            self.visit(item.pattern)
            self.visit(item.guard)
            with self.conditional(item.suite):
                self.visit(item.suite)

    def visit_WhileStmt(self, node: ast.WhileStmt):
        seen = set()
        with self.conditional(node.suite):
            self.ctx.scope[-1].seen_names = set()
            self.visit(node.cond)
            seen.update(self.ctx.scope[-1].seen_names)
        for var in seen:
            self.dominate(var)
        with self.conditional(node.suite):
            self.ctx.scope[-1].seen_names = set()
            self.visit(node.suite)
            seen.update(self.ctx.scope[-1].seen_names)
        for var in seen:
            self.dominate(var)
        with self.conditional(node.else_suite):
            self.visit(node.else_suite)

    def visit_ForStmt(self, node: ast.ForStmt):
        self.visit(node.iter)
        self.visit(node.decorator)
        for argument in node.omp_args:
            self.visit(argument.value)
        seen, seen_def = set(), set()
        with self.conditional(node.suite):
            seen_def = self.ctx.scope[-1].seen_names = set()
            with self.ctx.substitute("assignment", node):
                self.visit(node.var)
            seen = self.ctx.scope[-1].seen_names = set()
            self.visit(node.suite)
        for var in seen - seen_def:
            self.dominate(var)
        with self.conditional(node.else_suite):
            self.visit(node.else_suite)

    def visit_GlobalStmt(self, node: ast.GlobalStmt):
        # No shadowing od global/nonlocal allowed
        if self.ctx.function_scope is None:
            raise ScopeError(node, f"'{node.var}' outside a function")
        if node.var in self.ctx.map or node.var in self.ctx.captures:
            raise ScopeError(node, f"name '{node.var}' is assigned to before global declaration")
        self.bind_name(node.var, node)
        # Disallow shadowing od global/nonlocal names
        self.dominate(node.var, allow_shadow=False)
        self.ctx.captures[node.var] = (
            Bindings.Scope.Nonlocal if node.non_local else Bindings.Scope.Global
        )

    def visit_ImportStmt(self, node: ast.ImportStmt):
        match node:
            case ast.ImportStmt(what=ast.IdExpr(value="*")) if self.ctx.function_scope:
                raise ScopeError(node, "import * only allowed at module level")
            # dylib C imports
            case ast.ImportStmt(from_expr=ast.IdExpr(value="C"), what=ast.DotExpr(expr=what)):
                self.visit(what)
            case ast.ImportStmt(as_="", what=ast.IdExpr(value="*")):
                pass
            case ast.ImportStmt(as_=""):
                with self.ctx.substitute("assignment", node):
                    self.visit(node.what or node.from_expr)
            case _:
                self.bind_name(node.as_, node)
        for argument in node.args:
            self.visit(argument.type)
            self.visit(argument.default)
        self.visit(node.ret)

    def visit_TryStmt(self, node: ast.TryStmt):
        with self.conditional(node.suite):
            self.visit(node.suite)
        for catch in node.items:
            self.visit(catch.exc)
            with self.conditional(catch.suite):
                if catch.var:
                    new_name = self.ctx.cache.get_temporary_var(catch.var)
                    self.ctx.renames.append({catch.var: new_name})
                    catch.var = new_name
                    self.bind_name(catch.var, catch)
                self.visit(catch.suite)
                if catch.var:
                    self.ctx.renames.pop()
        with self.conditional(node.else_suite):
            self.visit(node.else_suite)
        self.visit(node.finally_suite)

    def visit_YieldStmt(self, node: ast.YieldStmt):
        if self.ctx.function_scope:
            self.ctx.function_scope.set(ast.Attr.IsGenerator)
        self.visit(node.expr)

    def visit_YieldExpr(self, _: ast.YieldExpr):
        if self.ctx.function_scope:
            self.ctx.function_scope.set(ast.Attr.IsGenerator)

    def visit_WithStmt(self, node: ast.WithStmt):
        with self.conditional(node.suite):
            for index, item in enumerate(node.items):
                self.visit(item)
                if node.vars[index]:
                    self.bind_name(node.vars[index], node)
            self.visit(node.suite)

    def visit_ClassStmt(self, node: ast.ClassStmt):
        if node.has(ast.Attr.Extend):
            self.read_name(node.name, node)
        else:
            self.bind_name(node.name, node)

        visitor = ScopingVisitor(
            ctx=ScopeContext(
                self.ctx.cache,
                scope=[ScopeContext.Block(0)],
                in_class=True,
                renames=copy.deepcopy(self.ctx.renames),
            )
        )
        for argument in node.items:
            visitor.visit(argument.type)
            visitor.visit(argument.default)
        visitor.visit(node.suite)
        for base_class in node.base_classes:
            self.visit(base_class)

    def visit_FunctionStmt(self, node: ast.FunctionStmt):
        if not any(isinstance(d, ast.IdExpr) and d.value == "overload" for d in node.decorators):
            self.bind_name(node.name, node)
        inner = ScopeContext(
            self.ctx.cache,
            scope=[ScopeContext.Block(0)],
            function_scope=node,
            renames=copy.deepcopy(self.ctx.renames),
        )
        if self.ctx.in_class and node.items:
            inner.class_deduce = (node.items[0].name, set())
        visitor = ScopingVisitor(ctx=inner)
        visitor.bind_name(node.name, node)
        for arg in node.items:
            visitor.bind_name(arg.name.lstrip("*"), node, source=arg)
            if arg.default:
                self.visit(arg.default)
        inner.scope.pop()

        inner.scope.append(ScopeContext.Block(0, node.suite))
        visitor.visit(node.suite)
        visitor.process_child_captures()
        inner.scope.pop()

        attr = Bindings(captures=dict(inner.captures))
        self.ctx.child_captures.update(inner.captures)
        if len(inner.map.get(node.name, [])) == 1 and node.name in inner.first_seen:
            attr.captures[node.name] = Bindings.Scope.Read
        for name, values in inner.map.items():
            attr.bindings[name] = Bindings.Binding(name, len(values))
            capture = inner.child_captures.get(name, inner.captures.get(name))
            if capture is Bindings.Scope.Nonlocal:
                attr.bindings[name].is_nonlocal = True
                self.ctx.child_captures[name] = Bindings.Scope.Nonlocal
        node.set(ast.Attr.Bindings, attr)
        if deduced := inner.class_deduce[1]:
            node.set(ast.Attr.ClassDeduce, list(deduced))

    def bind_name(self, name: str, node: ast.Node, source: ast.Node | None = None):
        """
        node: current node (typically IdExpr)
        source: originating node (e.g., AssignStmt); if None, same as node
        """
        if self.ctx.in_class:
            return False
        source = source or node
        capture = self.ctx.captures.get(name)
        if capture is Bindings.Scope.Read:
            raise ScopeError(
                self.ctx.first_seen[name],
                f"local variable '{name}' referenced before assignment at {source.info}",
            )
        if capture and node:
            self.update_name(name, node, has_used_var=False)
        elif capture is None:
            child = self.ctx.child_captures.get(name)
            items = self.ctx.map.setdefault(name, [])
            if child and child is not Bindings.Scope.Global and self.ctx.function_scope:
                new_scope = [self.ctx.scope[0].id]
                suite = self.ctx.scope[0].suite
                assert suite, "invalid suite"
                attr = suite.setdefault(ast.Attr.Bindings, Bindings())
                attr.bindings[name] = Bindings.Binding(
                    name, is_nonlocal=child is Bindings.Scope.Nonlocal
                )
                items.append(ScopeContext.Item(None, new_scope))
            items.insert(0, ScopeContext.Item(node, self.ctx.get_scope()))
        if value := self.dominate(name):
            self.fix_conditional(name, value)

    def read_name(self, name: str, source: ast.Node):
        if name not in self.ctx.first_seen:
            self.ctx.first_seen[name] = source
        if name not in self.ctx.map:
            self.ctx.captures[name] = Bindings.Scope.Read
        if value := self.dominate(name):
            self.fix_conditional(name, value)

    def update_name(self, name: str, node: ast.Node | None, has_used_var: bool):
        if node:
            if attr := node.get(ast.Attr.Bindings):
                attr.bindings.pop(name, None)
            node.attributes.pop(ast.Attr.ExprDominatedUsed, None)
            node.set(ast.Attr.ExprDominatedUsed if has_used_var else ast.Attr.ExprDominated)
        if isinstance(node, (ast.FunctionStmt, ast.ClassStmt)):
            raise ScopeError(node, f"cannot bind '{name}' to a class or a function")

    def fix_conditional(self, name: str, item: ScopeContext.Item):
        """
        Track loop variables to dominate them later.
        Example:
          x = 1
          while True:
            if x > 10: break
            x = x + 1
        Here, x must be dominated after the loop to ensure that it gets updated.
        """

        scope = self.ctx.get_scope()
        for index in range(len(self.ctx.scope) - 1, -1, -1):
            seen = self.ctx.scope[index].seen_names
            if seen:
                if is_inside(item.scope, scope):
                    break
                seen.add(name)
            scope.pop()
        # Variable binding check for variables that are defined within conditional blocks
        if item.access_checked:
            scope = self.ctx.get_scope()
            checked = any(is_inside(scope, a) for a in reversed(item.access_checked))
            if not checked:
                if item.binding is None or not item.binding.has(ast.Attr.Bindings):
                    # If the expression is not conditional, we can just do the check once
                    item.access_checked.append(self.ctx.get_scope())

    # Get an item from the context. Perform domination analysis for accessing items
    # defined in the conditional blocks (i.e., Python scoping).
    def dominate(self, name: str, allow_shadow: bool = True) -> ScopeContext.Item | None:
        def longest_common_prefix(a, b):
            common = min(len(a), len(b))
            while common > 0 and a[common - 1] != b[common - 1].id:
                common -= 1
            return common

        values = self.ctx.map.get(name, [])
        if not values:
            return None
        last_good_index = 0
        while last_good_index < len(values) and values[last_good_index].ignore:
            last_good_index += 1
        common_scope = len(self.ctx.scope)
        # Iterate through all bindings with the given name and find the closest binding
        # that dominates the current scope.
        for index, item in enumerate(values):
            if item.ignore:
                continue
            if is_inside(self.ctx.get_scope(), item.scope):
                common_scope = len(item.scope)
                last_good_index = index
                break
            assert item.scope[0] == 0 and self.ctx.scope[0].id == 0, "bad scoping"
            # Find the longest block prefix between the binding and the current common scope.
            common_scope = longest_common_prefix(item.scope, self.ctx.scope[:common_scope])
            last_good_index = index
        assert last_good_index < len(values), f"corrupted scoping for {name!r}"
        if not allow_shadow:
            common_scope = longest_common_prefix(values[-1].scope, self.ctx.scope[:common_scope])
        last_good = values[last_good_index]
        has_used_var = False
        if len(last_good.scope) != common_scope:
            scope = self.ctx.get_scope()
            new_scope = scope[:common_scope]
            for scope_index in range(common_scope - 1, -1, -1):
                if suite := self.ctx.scope[scope_index].suite:
                    attr = suite.attributes.setdefault(ast.Attr.Bindings, Bindings())
                    attr.bindings[name] = Bindings.Binding(name, count=1)
                    new_item = ScopeContext.Item(suite, new_scope, access_checked=[last_good.scope])
                    last_good_index += 1
                    values.insert(last_good_index, new_item)
                    last_good = new_item
                    has_used_var = True
                    break
        elif (
            last_good.binding
            and (attr := last_good.binding.get(ast.Attr.Bindings))
            and name in attr.bindings
        ):
            has_used_var = attr.bindings[name].count > 0
        for index, item in enumerate(values):
            if index == last_good_index:
                break
            self.update_name(name, item.binding, has_used_var)
            # The current scope is potentially reachable by multiple bindings that are
            # not dominated by a common binding. Create such binding in the scope that
            # dominates (covers) all of them.
            item.scope = list(last_good.scope)
            item.ignore = True
        if (
            not has_used_var
            and last_good.binding
            and (attr := last_good.binding.get(ast.Attr.Bindings))
        ):
            # Make sure to prepend a binding declaration: `var` and `var__used__ = False`
            # to the dominating scope.
            # Remove all bindings after the dominant binding.
            attr.bindings[name] = Bindings.Binding(name)
        return last_good

    def process_child_captures(self):
        for name, capture in self.ctx.child_captures.items():
            if (
                (values := self.ctx.map.get(name))
                and values[-1].binding
                and isinstance(values[-1].binding, ast.ClassStmt)
            ):
                continue
            if not self.dominate(name):
                self.ctx.captures[name] = capture
