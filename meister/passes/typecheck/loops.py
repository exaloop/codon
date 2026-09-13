# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast, cache
from ...bridge import Callable, List, cast
from ...error import TypecheckError
from . import assign, special, utils
from .ctx import Base

if TYPE_CHECKING:
    from . import TypeVisitor


def _parse_open_mp(code: str, info: ast.Node.SrcInfo) -> List[ast.CallExpr.Arg]:
    """Parse the OpenMP clauses accepted by codon/parser/peg/openmp.peg.
    Codex-generated."""

    # TODO: use proper parser?
    import re

    source = code.strip()
    source = re.sub(r"^omp\b", "", source, count=1).lstrip()
    source = re.sub(r"^parallel\b", "", source, count=1).lstrip()
    args = []
    clause = re.compile(
        r"(?:schedule\s*\(\s*(static|dynamic|guided|auto|runtime)"
        r"(?:\s*,\s*([1-9][0-9]*))?\s*\)"
        r"|num_threads\s*\(\s*([1-9][0-9]*)\s*\)"
        r"|ordered\b|collapse\s*\(\s*([1-9][0-9]*)\s*\)|gpu\b)"
    )
    while source:
        match = clause.match(source)
        if not match:
            raise TypecheckError(info, "openmp: invalid syntax")
        text = match.group(0)
        if text.startswith("schedule"):
            args.append(
                ast.CallExpr.Arg(name="schedule", value=ast.StringExpr(match.group(1), info=info))
            )
            if match.group(2):
                args.append(
                    ast.CallExpr.Arg(
                        name="chunk_size", value=ast.IntExpr(int(match.group(2)), info=info)
                    )
                )
        elif text.startswith("num_threads"):
            args.append(
                ast.CallExpr.Arg(
                    name="num_threads", value=ast.IntExpr(int(match.group(3)), info=info)
                )
            )
        elif text.startswith("ordered"):
            args.append(ast.CallExpr.Arg(name="ordered", value=ast.BoolExpr(True, info=info)))
        elif text.startswith("collapse"):
            args.append(
                ast.CallExpr.Arg(name="collapse", value=ast.IntExpr(int(match.group(4)), info=info))
            )
        else:
            args.append(ast.CallExpr.Arg(name="gpu", value=ast.BoolExpr(True, info=info)))
        source = source[match.end() :].lstrip()
    return args


def typecheck_break(self: TypeVisitor, node: ast.BreakStmt) -> ast.Stmt:
    """
    Ensure that `break` is in a loop.
    Transform if a loop break variable is available
    (e.g., a break within loop-else block).
    @example
    `break` -> `no_break = False; break`
    """

    loop = self.ctx.base.loop
    if not loop:
        raise TypecheckError(node, "'break' outside loop")

    loop.flat = False
    if loop.break_var:
        assignment = ast.AssignStmt(
            ast.IdExpr(loop.break_var),
            rhs=ast.BoolExpr(False),
            update=ast.AssignStmt.Mode.Update,
        )
        assignment = self.visit_stmt(assignment)
        return ast.SuiteStmt(assignment, ast.BreakStmt())

    node.done = True
    if self.ctx.static_loops[-1]:
        assignment = ast.AssignStmt(
            ast.IdExpr(self.ctx.static_loops[-1]),
            rhs=ast.BoolExpr(False),
            update=ast.AssignStmt.Mode.Update,
        )
        return self.visit_stmt(ast.SuiteStmt(assignment, node))
    return node


def typecheck_continue(self: TypeVisitor, node: ast.ContinueStmt) -> ast.Stmt:
    """Ensure that `continue` is in a loop"""

    loop = self.ctx.base.loop
    if not loop:
        raise TypecheckError(node, "'continue' outside loop")
    loop.flat = False

    node.done = True
    if self.ctx.static_loops[-1]:
        return ast.BreakStmt(done=True)
    return node


def typecheck_while(self: TypeVisitor, node: ast.WhileStmt) -> ast.Stmt:
    """
    Transform a while loop.
    @example
    `while cond: ...`           ->  `while cond: ...`
    `while cond: ... else: ...` -> ```no_break = True
    while cond:
    ...
    if no_break: ...```
    """

    # Check for while-else clause
    break_var = ""
    if node.else_suite and node.else_suite.first_in_block():
        # no_break = True
        break_var = utils.get_temporary_var(self.ctx, "no_break")
        assignment = self.visit_stmt(ast.AssignStmt(ast.IdExpr(break_var), rhs=ast.BoolExpr(True)))
        self.ctx.prepend_stmts[-1].append(assignment)

    base = self.ctx.base
    base.loops.append(Base.LoopData(break_var=break_var))
    try:
        node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
        node.cond = self.visit_expr(node.cond)
        _, node.cond = utils.wrap_expr(
            self.ctx, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
        )
        with (
            self.ctx.substitute("static_loops", self.ctx.static_loops + [node.goto_var or ""]),
            self.ctx.substitute("block_level", self.ctx.block_level + 1),
        ):
            node.suite = ast.SuiteStmt.wrap(self.visit_stmt(node.suite))
    finally:
        base.loops.pop()

    result: ast.Stmt = node
    # Complete while-else clause
    if node.else_suite and node.else_suite.first_in_block():
        suite, node.else_suite = node.else_suite, None
        result = self.visit_stmt(
            ast.SuiteStmt(node, ast.IfStmt(ast.IdExpr(break_var), if_suite=suite))
        )
    if node.cond.done and node.suite.done:
        node.done = True
    return result


def typecheck_for(self: TypeVisitor, node: ast.ForStmt) -> ast.Stmt:
    """
    Typecheck for statements. Wrap the iterator expression with `__iter__` if needed.
    See @c transformHeterogenousTupleFor for iterating heterogenous tuples.
    """

    if node.decorator:
        node.decorator = transform_for_decorator(self, node.decorator)
    match node.decorator:
        case ast.CallExpr(expr=ast.IdExpr(type=typ)) if (
            typ and typ.func and typ.func.name == ast.types.mangle("std.openmp", func="for_par")
        ):
            if utils.extract_func_generic(typ, 3).bool:
                import_gpu = ast.ImportStmt(
                    ast.IdExpr("gpu"), args=[], as_=utils.get_temporary_var(self.ctx, "_")
                )
                self.ctx.prepend_stmts[-1].append(self.visit_stmt(import_gpu))

    break_var = ""
    # Needs in-advance transformation to prevent name clashes with the iterator variable
    # do not expand special calls here,
    node.iter.set(ast.Attr.ExprNoSpecial)
    # might be needed for static loops!
    node.iter = self.visit_expr(node.iter)
    # Check for for-else clause
    assignment: ast.Stmt | None = None
    if node.else_suite and node.else_suite.first_in_block():
        break_var = utils.get_temporary_var(self.ctx, "no_break")
        assignment = self.visit_stmt(ast.AssignStmt(ast.IdExpr(break_var), rhs=ast.BoolExpr(True)))

    # Extract the iterator type of the for
    if not (iterator_type := node.iter.cls):
        return node

    delay, static_loop = transform_static_for_loop(self, node)
    if delay:
        return node  # wait until the iterator is known
    elif static_loop:
        return static_loop
    if not isinstance(node.var, ast.IdExpr):
        temp = ast.IdExpr(utils.get_temporary_var(self.ctx, "for"))
        unpacked = assign.unpack_assignment(self, node.var, temp)
        node.var = temp
        node.suite = ast.SuiteStmt(unpacked, node.suite)

    # Replace for (i, j) in ... { ... } with for tmp in ...: { i, j = tmp ; ... }
    is_generator = iterator_type.name == ("AsyncGenerator" if node.async_ else "Generator")
    if not is_generator and not node.wrapped:
        node.iter = self.visit_expr(ast.CallExpr(ast.DotExpr(node.iter, member="__iter__")))
        iter_type = node.iter.cls
        node.wrapped = True
        if not iter_type:
            return node
        is_generator = iterator_type.name == ("AsyncGenerator" if node.async_ else "Generator")
    var = cast(ast.IdExpr, node.var)
    assert var, f"corrupt for variable: {node.var}"

    base = self.ctx.base
    base.loops.append(Base.LoopData(break_var=break_var))
    try:
        if not var.has(ast.Attr.ExprDominated) and not var.has(ast.Attr.ExprDominatedUsed):
            var.type = var.type or utils.instantiate_unbound(self.ctx)
            self.ctx.add_item(
                utils.get_unmangled_name(self.ctx, var.value),
                self.ctx.generate_canonical_name(var.value),
                var.type,
                self.ctx.time,
            )
        elif var.has(ast.Attr.ExprDominatedUsed):
            var.erase(ast.Attr.ExprDominatedUsed)
            var.set(ast.Attr.ExprDominated)
            node.suite = ast.SuiteStmt(
                ast.AssignStmt(
                    ast.IdExpr(f"{var.value}{cache.VAR_USED_SUFFIX}"),
                    rhs=ast.BoolExpr(True),
                    update=ast.AssignStmt.Mode.Update,
                ),
                node.suite,
            )
        node.var = self.visit_expr(var)

        # Case: iterating a non-generator. Wrap with `__iter__`
        if iterator_type and not is_generator:
            # Unify iterator var and the iterator type
            raise TypecheckError(node.iter, "expected iterable expression")
        if iterator_type:
            assert node.var.type
            node.var.type |= iterator_type[0]
        with (
            self.ctx.substitute("static_loops", self.ctx.static_loops + [""]),
            self.ctx.substitute("block_level", self.ctx.block_level + 1),
        ):
            node.suite = ast.SuiteStmt.wrap(self.visit_stmt(node.suite))
        if base.loop and base.loop.flat:
            node.flat = True
        result = node
    finally:
        base.loops.pop()

    # Complete for-else clause
    if node.else_suite and node.else_suite.first_in_block():
        suite, node.else_suite = node.else_suite, None
        result = self.visit_stmt(
            ast.SuiteStmt(assignment, node, ast.IfStmt(ast.IdExpr(break_var), if_suite=suite))
        )
    if node.iter.done and node.suite.done:
        node.done = True
    return result


def transform_for_decorator(self: TypeVisitor, decorator: ast.Expr) -> ast.Expr:
    """
    Transform and check for OpenMP decorator.
    @example
    `@par(num_threads=2, openmp="schedule(static)")` ->
    `for_par(num_threads=2, schedule="static")`
    """

    callee = decorator
    if isinstance(callee, ast.CallExpr):
        callee = callee.expr
    callee = self.visit_expr(callee)
    if not isinstance(callee, ast.IdExpr) or not callee.value.startswith(
        ast.types.mangle("std.openmp", func="for_par")
    ):
        raise TypecheckError(decorator, "invalid loop decorator")

    args = []
    omp_args = []
    if isinstance(decorator, ast.CallExpr):
        for arg in decorator.items:
            str_arg = arg.value if isinstance(arg.value, ast.StringExpr) else None
            if str_arg and (arg.name == "openmp" or not arg.name):
                omp_args = _parse_open_mp(str_arg.get_value(), str_arg.info)
            else:
                args.append(arg)
    args += omp_args
    return self.visit_expr(ast.CallExpr(ast.IdExpr("for_par"), args))


def transform_static_for_loop(self: TypeVisitor, stmt: ast.ForStmt):
    """
    Handle static for constructs.
    @example
    `for i in statictuple(1, x): <suite>` ->
    ```loop = True
    while loop:
    while loop:
    i: Literal[int] = 1; <suite>; break
    while loop:
    i = x; <suite>; break
    loop = False   # also set to False on break
    If a loop is flat, while wrappers are removed.
    A separate suite is generated for each static iteration.
    """

    loop_var = utils.get_temporary_var(self.ctx, "loop")
    suite = stmt.suite.clone(clean=True)

    def wrap(assignments: ast.Stmt) -> ast.Stmt:
        if not stmt.flat:
            break_stmt = ast.BreakStmt(done=True)  # set done to skip extra checks
            return ast.WhileStmt(
                ast.IdExpr(loop_var),
                suite=ast.SuiteStmt(assignments, suite.clone(), break_stmt),
                goto_var=loop_var,
            )
        else:
            return ast.SuiteStmt(assignments, stmt.suite.clone())

    ok, delay, preamble, items = transform_static_loop_call(self, stmt.var, stmt.iter, wrap)
    if not ok or delay:
        return ok, None
    block = ast.SuiteStmt(preamble, *items)
    if not stmt.flat:
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            block.add(
                ast.AssignStmt(
                    ast.IdExpr(loop_var), rhs=ast.BoolExpr(False), update=ast.AssignStmt.Mode.Update
                )
            )
            # var [: Static] := expr; suite...
            loop_result = self.visit_stmt(
                ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(loop_var), rhs=ast.BoolExpr(True)),
                    ast.WhileStmt(ast.IdExpr(loop_var), suite=block),
                )
            )
    else:
        # Close the loop
        loop_result = self.visit_stmt(block)
    return False, loop_result


def transform_static_loop_call(
    self: TypeVisitor,
    var: ast.Expr,
    iterator: ast.Expr,
    wrapper: Callable[[ast.Stmt], ast.Stmt],
    allow_non_heterogenous: bool = False,
) -> tuple[bool, bool, ast.Stmt | None, list[ast.Stmt]]:
    if not iterator.cls:
        return True, True, None, []

    vars = []
    match var:
        case ast.IdExpr(value=name):
            vars.append(var.value)
        case ast.ListExpr(items=items) | ast.TupleExpr(items=items):
            if not items:
                return False, False, None, []
            for item in items:
                if isinstance(item, ast.IdExpr):
                    vars.append(item.value)
                else:
                    return False, False, None, []

    preamble: ast.Stmt | None = None
    iterator = utils.get_head_expr(iterator)
    function = (
        iterator.expr
        if isinstance(iterator, ast.CallExpr) and isinstance(iterator.expr, ast.IdExpr)
        else None
    )
    block: List[ast.Stmt] = []
    name = "" if not function else function.value
    if function and name.startswith(ast.types.mangle("std.internal.static", func="tuple")):
        block = special.populate_static_tuple_loop(self, cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(
        ast.types.mangle("std.internal.static", func="range", overload=1)
    ):
        block = special.populate_simple_static_range_loop(cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="range")):
        block = special.populate_static_range_loop(cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(
        ast.types.mangle("std.internal.static", cls="function", func="overloads")
    ):
        block = special.populate_static_fn_overloads_loop(self, cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="enumerate")):
        block = special.populate_static_enumerate_loop(self, cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="vars")):
        block = special.populate_static_vars_loop(self, cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="methods")):
        block = special.populate_static_methods_loop(self, cast(ast.CallExpr, iterator), vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="vars_types")):
        block = special.populate_static_var_types_loop(self, cast(ast.CallExpr, iterator), vars)
    elif iterator.type and iterator.type == ast.types.Stdlib.Tuple:
        # Maybe heterogenous?
        if not iterator.type.can_realize():
            return True, True, None, []
        # wait until the tuple is fully realizable
        if (
            not utils.is_heterogenous(self.ctx, iterator.type.require_cls)
            and not allow_non_heterogenous
        ):
            return False, False, None, []
        assert iterator
        block = special.populate_static_heterogenous_tuple_loop(self, iterator, vars)
        preamble = block[-1]
        block.pop()
    else:
        return False, False, None, []
    return True, False, preamble, [wrapper(s) for s in block]
