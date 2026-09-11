# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast, cache
from ...error import TypecheckError
from . import utils

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_assert(self: TypeVisitor, node: ast.AssertStmt) -> ast.Stmt:
    """
    Transform asserts.
    @example
    `assert foo()` -> `if not foo(): raise __internal__.seq_assert([file], [line], "")`
    `assert foo(), msg` -> `if not foo(): raise __internal__.seq_assert([file], [line], str(msg))`
    Use `seq_assert_test` instead of `seq_assert` and do not raise anything during unit
    testing (i.e., when the enclosing function is marked with `@test`).
    """

    message = ast.StringExpr("")
    if node.message:
        message = ast.CallExpr(ast.IdExpr("str"), items=[node.message])
    base = self.ctx.base
    is_test = self.ctx.in_function and base and base.func and base.func.has(ast.Attr.Test)
    call = ast.CallExpr(
        ast.IdExpr(
            ast.types.mangle(
                cls="__internal__", func="seq_assert_test" if is_test else "seq_assert"
            )
        ),
        items=[ast.StringExpr(node.info.file), ast.IntExpr(node.info.line), message],
    )
    statement = ast.IfStmt(
        ast.UnaryExpr("!", expr=node.expr),
        if_suite=ast.ExprStmt(call) if is_test else ast.ThrowStmt(call),
    )
    return self.visit_stmt(statement)


def typecheck_try(self: TypeVisitor, node: ast.TryStmt) -> ast.Stmt:
    """
    Typecheck try-except statements. Handle Python exceptions separately.
    @example
    ```try: ...
    except python.Error as e: ...
    except PyExc as f: ...
    except ValueError as g: ...
    ``` -> ```
    try: ...
    except ValueError as g: ...                   # ValueError
    except PyExc as exc:
    while True:
    if isinstance(exc.pytype, python.Error):  # python.Error
    e = exc.pytype; ...; break
    f = exc; ...; break                       # PyExc
    raise```
    """

    with self.ctx.substitute("block_level", self.ctx.block_level + 1):
        node.suite = ast.SuiteStmt.wrap(self.visit_stmt(node.suite))

    catches = []
    python_catch_body = ast.SuiteStmt()
    python_catch = ast.WhileStmt(ast.BoolExpr(True), suite=python_catch_body)
    done = node.suite and node.suite.done
    for catch in node.items:
        value = None
        if catch.var:
            if not catch.has(ast.Attr.ExprDominated) and not catch.has(ast.Attr.ExprDominatedUsed):
                value = self.ctx.add_item(
                    utils.get_unmangled_name(self.ctx, catch.var),
                    self.ctx.generate_canonical_name(catch.var),
                    utils.instantiate_unbound(self.ctx),
                    self.ctx.time,
                )
            elif catch.has(ast.Attr.ExprDominatedUsed):
                value = self.ctx[catch.var]
                catch.attributes.pop(ast.Attr.ExprDominatedUsed, None)
                catch.set(ast.Attr.ExprDominated)
                catch.suite = ast.SuiteStmt(
                    ast.AssignStmt(
                        ast.IdExpr(
                            utils.get_unmangled_name(self.ctx, catch.var) + cache.VAR_USED_SUFFIX
                        ),
                        rhs=ast.BoolExpr(True),
                        update=ast.AssignStmt.Mode.Update,
                    ),
                    catch.suite,
                )
            else:
                value = self.ctx[catch.var]
            catch.var = value.canonical
        if catch.exc:
            catch.exc = self.visit_expr(catch.exc)
        exception_class = (
            None if catch.exc is None else utils.extract_class_type(self.ctx, catch.exc)
        )
        if exception_class and exception_class == "pyobj":
            if not node.has(ast.Attr.TryPyVar):
                # Transform python.Error exceptions
                node.set(ast.Attr.TryPyVar, utils.get_temporary_var(self.ctx, "pyexc"))
            py_var = node.get(ast.Attr.TryPyVar, "")
            if catch.var:
                catch.suite = ast.SuiteStmt(
                    ast.AssignStmt(
                        ast.IdExpr(catch.var), rhs=ast.DotExpr(ast.IdExpr(py_var), member="pytype")
                    ),
                    catch.suite,
                )
            catch.suite = ast.SuiteStmt(
                ast.IfStmt(
                    ast.CallExpr(
                        ast.IdExpr("isinstance"),
                        items=[ast.DotExpr(ast.IdExpr(py_var), member="pytype"), catch.exc],
                    ),
                    if_suite=ast.SuiteStmt(catch.suite, ast.BreakStmt()),
                )
            )
            python_catch_body.add(catch.suite)
        elif exception_class and exception_class == ast.types.Stdlib.PyError:
            if not node.has(ast.Attr.TryPyVar):
                # Transform PyExc exceptions
                node.set(ast.Attr.TryPyVar, utils.get_temporary_var(self.ctx, "pyexc"))
            py_var = str(node.attributes[ast.Attr.TryPyVar])
            if catch.var:
                catch.suite = ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(catch.var), rhs=ast.IdExpr(py_var)),
                    catch.suite,
                )
            catch.suite = ast.SuiteStmt(catch.suite, ast.BreakStmt())
            python_catch_body.add(catch.suite)
        else:
            if catch.exc:
                # Handle all other exceptions
                catch.exc = self.visit_expr(catch.exc, enforce_type=True)
                exception_type = utils.extract_class_type(self.ctx, catch.exc)
                if not any(
                    parent == ast.types.Stdlib.BaseException
                    for parent in utils.get_mro(self.ctx, exception_type)
                ):
                    raise TypecheckError(catch.exc, f"invalid exception type {exception_type}")
                if value:
                    value.type |= utils.extract_type(self.ctx, exception_type)

            with self.ctx.substitute("block_level", self.ctx.block_level + 1):
                catch.suite = ast.SuiteStmt.wrap(self.visit_stmt(catch.suite))
            done = done and (catch.exc is None or catch.exc.done)
            done = done and catch.suite and catch.suite.done
            catches.append(catch)

    if python_catch_body.items:
        # Process PyError catches
        py_var = node.get(ast.Attr.TryPyVar, "")
        python_catch_body.add(ast.ThrowStmt())
        python_exception = self.visit_expr(ast.IdExpr(ast.types.Stdlib.PyError), enforce_type=True)
        catch = ast.TryStmt.Except(python_exception, var=py_var, suite=python_catch)
        self.ctx.add_item(
            py_var, py_var, utils.extract_type(self.ctx, python_exception), self.ctx.time
        )
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            catch.suite = ast.SuiteStmt.wrap(self.visit_stmt(catch.suite))
        done = done and catch.exc.done and catch.suite and catch.suite.done
        catches.append(catch)

    node.items = catches
    if node.else_suite:
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            node.else_suite = ast.SuiteStmt.wrap(self.visit_stmt(node.else_suite))
        done = done and node.else_suite and node.else_suite.done
    if node.finally_suite:
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            node.finally_suite = ast.SuiteStmt.wrap(self.visit_stmt(node.finally_suite))
        done = done and node.finally_suite and node.finally_suite.done
    if done:
        node.done = True
    return node


def typecheck_throw(self: TypeVisitor, node: ast.ThrowStmt) -> ast.Stmt:
    """
    Transform `raise` statements.
    @example
    `raise exc` -> ```raise BaseException.set_header(exc, "fn", "file", line, col)```
    """
    if node.expr is None:
        node.done = True
        return node

    node.expr = self.visit_expr(node.expr)
    setter_name = ast.types.mangle(
        module="std.internal.types.error", cls="BaseException", func="_set_header"
    )
    match node.expr:
        case ast.CallExpr(ast.IdExpr(value)) if value == setter_name:
            # already wrapped
            pass
        case _:
            call = ast.CallExpr(
                ast.IdExpr(setter_name),
                items=[
                    node.expr,
                    ast.StringExpr(self.ctx.base_name),
                    ast.StringExpr(node.info.file),
                    ast.IntExpr(node.info.line),
                    ast.IntExpr(node.info.col),
                    node.from_expr or ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType)),
                ],
            )
            node.expr = self.visit_expr(call)
    if node.expr.done:
        node.done = True
    return node


def typecheck_with(self: TypeVisitor, node: ast.WithStmt) -> ast.Stmt:
    """
    Transform with statements.
    @example
    `with foo(), bar() as a: ...` ->
    ```tmp = foo()
    tmp.__enter__()
    try:
    a = bar()
    a.__enter__()
    try:
    ...
    finally:
    a.__exit__()
    finally:
    tmp.__exit__()```
    """

    is_async = node.async_
    content = []
    for var, item in reversed(list(zip(node.vars, node.items))):
        var = var or utils.get_temporary_var(self.ctx, "with")
        assignment = ast.AssignStmt(
            ast.IdExpr(var),
            rhs=item,
            update=ast.AssignStmt.Mode.Update
            if item.has(ast.Attr.ExprDominated)
            else ast.AssignStmt.Mode.Assign,
        )
        enter = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(var), member="__aenter__" if is_async else "__enter__")
        )
        exit = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(var), member="__aexit__" if is_async else "__exit__")
        )
        if is_async:
            enter = ast.AwaitExpr(enter)
            exit = ast.AwaitExpr(exit)
        if content:
            body = ast.SuiteStmt(*content)
        else:
            body = node.suite.clone()
        try_statement = ast.TryStmt(body, finally_suite=ast.ExprStmt(exit))
        content = [assignment, ast.ExprStmt(enter), try_statement]
    return self.visit_stmt(ast.SuiteStmt(*content))
