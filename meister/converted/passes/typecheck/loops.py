# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import Callable, List, cast
from ... import ast, cache
from . import assign, infer, special, utils
from .ctx import Base, TypecheckError

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
    arguments = []
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
            arguments.append(
                ast.CallExpr.Arg(name="schedule", value=ast.StringExpr(match.group(1), info=info))
            )
            if match.group(2):
                arguments.append(
                    ast.CallExpr.Arg(
                        name="chunk_size", value=ast.IntExpr(int(match.group(2)), info=info)
                    )
                )
        elif text.startswith("num_threads"):
            arguments.append(
                ast.CallExpr.Arg(
                    name="num_threads", value=ast.IntExpr(int(match.group(3)), info=info)
                )
            )
        elif text.startswith("ordered"):
            arguments.append(ast.CallExpr.Arg(name="ordered", value=ast.BoolExpr(True, info=info)))
        elif text.startswith("collapse"):
            arguments.append(
                ast.CallExpr.Arg(name="collapse", value=ast.IntExpr(int(match.group(4)), info=info))
            )
        else:
            arguments.append(ast.CallExpr.Arg(name="gpu", value=ast.BoolExpr(True, info=info)))
        source = source[match.end() :].lstrip()
    return arguments


def typecheck_break(self: TypeVisitor, node: ast.BreakStmt) -> ast.Node:
    """
    Ensure that `break` is in a loop.
    Transform if a loop break variable is available
    (e.g., a break within loop-else block).
    @example
    `break` -> `no_break = False; break`
    """

    loop = self.ctx.get_base().get_loop()
    if not loop:
        raise TypecheckError(node, "'break' outside loop")
    loop.flat = False
    if loop.break_var:
        assignment = ast.AssignStmt(
            ast.IdExpr(loop.break_var),
            rhs=ast.BoolExpr(False),
            update=ast.AssignStmt.Mode.Update,
        )
        assignment = self.visit(assignment)
        return ast.SuiteStmt(assignment, ast.BreakStmt())

    node.done = True
    if self.ctx.static_loops[-1]:
        assignment = ast.AssignStmt(
            ast.IdExpr(self.ctx.static_loops[-1]),
            rhs=ast.BoolExpr(False),
            update=ast.AssignStmt.Mode.Update,
        )
        return self.visit(ast.SuiteStmt(assignment, node))
    return node


def typecheck_continue(self: TypeVisitor, node: ast.ContinueStmt) -> ast.Node:
    """Ensure that `continue` is in a loop"""

    loop = self.ctx.get_base().get_loop()
    if not loop:
        raise TypecheckError(node, "'continue' outside loop")
    loop.flat = False

    node.done = True
    if self.ctx.static_loops[-1]:
        return ast.BreakStmt(done=True)
    return node


def typecheck_while(self: TypeVisitor, node: ast.WhileStmt) -> ast.Node:
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
        assignment = self.visit(ast.AssignStmt(ast.IdExpr(break_var), rhs=ast.BoolExpr(True)))
        self.ctx.prepend_stmts[-1].append(assignment)

    base = self.ctx.get_base()
    base.loops.append(Base.LoopData(break_var=break_var))
    try:
        node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
        node.cond = self.visit(node.cond)
        _, node.cond = utils.wrap_expr(
            self, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
        )
        with (
            self.ctx.substitute("static_loops", self.ctx.static_loops + [node.goto_var or ""]),
            self.ctx.substitute("block_level", self.ctx.block_level + 1),
        ):
            node.suite = self.visit(node.suite)
            node.suite = ast.SuiteStmt.wrap(node.suite)
    finally:
        base.loops.pop()

    result: ast.Stmt = node
    # Complete while-else clause
    if node.else_suite and node.else_suite.first_in_block():
        suite, node.else_suite = node.else_suite, None
        result = self.visit(ast.SuiteStmt(node, ast.IfStmt(ast.IdExpr(break_var), if_suite=suite)))
    if node.cond.done and node.suite.done:
        node.done = True
    return result


def typecheck_for(self: TypeVisitor, node: ast.ForStmt) -> ast.Node:
    """
    Typecheck for statements. Wrap the iterator expression with `__iter__` if needed.
    See @c transformHeterogenousTupleFor for iterating heterogenous tuples.
    """

    if node.decorator:
        node.decorator = transform_for_decorator(self, node.decorator)
    match node.decorator:
        case ast.CallExpr(expr=ast.IdExpr(type=typ)) if (
            typ := typ.get_func()
        ) and typ.get_func_name() == ast.types.mangle("std.openmp", func="for_par"):
            static_bool = utils.extract_func_generic(typ, 3).get_bool_static()
            if static_bool and bool(static_bool.value):
                import_gpu = ast.ImportStmt(
                    ast.IdExpr("gpu"), args=[], as_=utils.get_temporary_var(self.ctx, "_")
                )
                self.ctx.prepend_stmts[-1].append(self.visit(import_gpu))

    break_var = ""
    # Needs in-advance transformation to prevent name clashes with the iterator variable
    # do not expand special calls here,
    node.iter.set(ast.Attr.ExprNoSpecial)
    # might be needed for static loops!
    node.iter = self.visit(node.iter)
    # Check for for-else clause
    assignment: ast.Stmt | None = None
    if node.else_suite and node.else_suite.first_in_block():
        break_var = utils.get_temporary_var(self.ctx, "no_break")
        assignment = self.visit(ast.AssignStmt(ast.IdExpr(break_var), rhs=ast.BoolExpr(True)))

    # Extract the iterator type of the for
    if not (iterator_type := node.iter.get_class_type()):
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
        node.iter = self.visit(ast.CallExpr(ast.DotExpr(node.iter, member="__iter__")))
        iter_type = node.iter.get_class_type()
        node.wrapped = True
        if not iter_type:
            return node
        is_generator = iterator_type.name == ("AsyncGenerator" if node.async_ else "Generator")
    var = cast(ast.IdExpr, node.var)
    assert var, f"corrupt for variable: {node.var}"

    base = self.ctx.get_base()
    base.loops.append(Base.LoopData(break_var=break_var))
    try:
        if not var.has(ast.Attr.ExprDominated) and not var.has(ast.Attr.ExprDominatedUsed):
            var.type = var.type or utils.instantiate_unbound(self.ctx)
            self.ctx.add(
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
        node.var = self.visit(var)

        # Case: iterating a non-generator. Wrap with `__iter__`
        if iterator_type and not is_generator:
            # Unify iterator var and the iterator type
            raise TypecheckError(node.iter, "expected iterable expression")
        if iterator_type:
            infer.unify(node.var.type, iterator_type[0])
        with (
            self.ctx.substitute("static_loops", self.ctx.static_loops + [""]),
            self.ctx.substitute("block_level", self.ctx.block_level + 1),
        ):
            node.suite = self.visit(node.suite)
            node.suite = ast.SuiteStmt.wrap(node.suite)
        if base.get_loop() and base.get_loop().flat:
            node.flat = True
        result = node
    finally:
        base.loops.pop()

    # Complete for-else clause
    if node.else_suite and node.else_suite.first_in_block():
        suite, node.else_suite = node.else_suite, None
        result = self.visit(
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
    callee = self.visit(callee)
    if not isinstance(callee, ast.IdExpr) or not callee.value.startswith(
        ast.types.mangle("std.openmp", func="for_par")
    ):
        raise TypecheckError(decorator, "invalid loop decorator")

    arguments = []
    omp_arguments = []
    if isinstance(decorator, ast.CallExpr):
        for arg in decorator.items:
            str_arg = arg.value if isinstance(arg.value, ast.StringExpr) else None
            if str_arg and (arg.name == "openmp" or not arg.name):
                omp_arguments = _parse_open_mp(str_arg.get_value(), str_arg.info)
            else:
                arguments.append(arg)
    arguments += omp_arguments
    result = self.visit(ast.CallExpr(ast.IdExpr("for_par"), items=arguments))
    return result


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

    def wrap(assignments: ast.Stmt) -> ast.Node:
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
            loop_result = self.visit(
                ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(loop_var), rhs=ast.BoolExpr(True)),
                    ast.WhileStmt(ast.IdExpr(loop_var), suite=block),
                )
            )
    else:
        # Close the loop
        loop_result = self.visit(block)
    return False, loop_result


def populate_static_loop(
    self: TypeVisitor,
    var: ast.Expr | None,
    iterator: ast.Expr,
    final: ast.Expr,
) -> List[ast.Node]:
    results = []
    for idx in range(len(iterator.type.generics)):
        assignment = ast.AssignStmt(
            var.clone(), rhs=ast.IndexExpr(iterator.clone(), idx=ast.IntExpr(idx))
        )
        results.append(ast.StmtExpr([assignment], expr=final.clone))
    return results


def transform_static_loop_call(
    self: TypeVisitor,
    var: ast.Expr,
    iterator: ast.Expr,
    wrapper: Callable[[ast.Stmt], ast.Node],
    allow_non_heterogenous: bool = False,
):
    if not iterator.get_class_type():
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
        block = special.populate_static_tuple_loop(self, iterator, vars)
    elif function and name.startswith(
        ast.types.mangle("std.internal.static", func="range", overload=1)
    ):
        block = special.populate_simple_static_range_loop(self, iterator, vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="range")):
        block = special.populate_static_range_loop(self, iterator, vars)
    elif function and name.startswith(
        ast.types.mangle("std.internal.static", cls="function", func="overloads")
    ):
        block = special.populate_static_fn_overloads_loop(self, iterator, vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="enumerate")):
        block = special.populate_static_enumerate_loop(self, iterator, vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="vars")):
        block = special.populate_static_vars_loop(self, iterator, vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="methods")):
        block = special.populate_static_methods_loop(self, iterator, vars)
    elif function and name.startswith(ast.types.mangle("std.internal.static", func="vars_types")):
        block = special.populate_static_var_types_loop(self, iterator, vars)
    elif isinstance(iterator.type, ast.types.Type) and iterator.type.is_type(
        ast.types.Stdlib.Tuple
    ):
        # Maybe heterogenous?
        if not iterator.type.can_realize():
            return True, True, None, []
        # wait until the tuple is fully realizable
        if not utils.is_heterogenous(self.ctx, iterator.type) and not allow_non_heterogenous:
            return False, False, None, []
        block = special.populate_static_heterogenous_tuple_loop(self, iterator, vars)
        preamble = block[-1]
        block.pop()
    else:
        return False, False, None, []
    return True, False, preamble, [wrapper(s) for s in block]
