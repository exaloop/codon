# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast, cache
from . import infer, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_assert(self: TypeVisitor, node: ast.AssertStmt) -> ast.Node:
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
    base = self.ctx.get_base()
    is_test = self.ctx.in_function() and base and base.func and base.func.has(ast.Attr.Test)
    call = ast.CallExpr(
        ast.types.mangle(cls="__internal__", func="seq_assert_test" if is_test else "seq_assert"),
        items=[ast.StringExpr(node.info.file), ast.IntExpr(node.info.line), message],
    )
    statement = ast.IfStmt(
        ast.UnaryExpr("!", expr=node.expr),
        if_suite=ast.ExprStmt(call) if is_test else ast.ThrowStmt(call),
    )
    return self.visit(statement)


def typecheck_try(self: TypeVisitor, node: ast.TryStmt) -> ast.Node:
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
        node.suite = self.visit(node.suite)
        node.suite = ast.SuiteStmt.wrap(node.suite)

    catches = []
    python_catch_body = ast.SuiteStmt()
    python_catch = ast.WhileStmt(ast.BoolExpr(True), suite=python_catch_body)
    done = node.suite and node.suite.done
    for catch in node.items:
        value = None
        if catch.var:
            if not catch.has(ast.Attr.ExprDominated) and not catch.has(ast.Attr.ExprDominatedUsed):
                value = self.ctx.add(
                    utils.get_unmangled_name(self.ctx, catch.var),
                    utils.generate_canonical_name(self.ctx, catch.var),
                    utils.instantiate_unbound(self.ctx),
                    self.ctx.time,
                )
            elif catch.has(ast.Attr.ExprDominatedUsed):
                value = self.ctx.force_find(catch.var)
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
                value = self.ctx.force_find(catch.var)
            catch.var = value.canonical_name
        if catch.exc:
            catch.exc = self.visit(catch.exc)
        exception_class = (
            None if catch.exc is None else utils.extract_class_type(self.ctx, catch.exc)
        )
        if exception_class and exception_class.is_type("pyobj"):
            if not node.has(ast.Attr.TryPyVar):
                # Transform python.Error exceptions
                node.set(ast.Attr.TryPyVar, utils.get_temporary_var(self.ctx, "pyexc"))
            python_variable: str = node.attributes[ast.Attr.TryPyVar]
            if catch.var:
                catch.suite = ast.SuiteStmt(
                    ast.AssignStmt(
                        ast.IdExpr(catch.var),
                        rhs=ast.DotExpr(ast.IdExpr(python_variable), member="pytype"),
                    ),
                    catch.suite,
                )
            catch.suite = ast.SuiteStmt(
                ast.IfStmt(
                    ast.CallExpr(
                        ast.IdExpr("isinstance"),
                        items=[
                            ast.DotExpr(ast.IdExpr(python_variable), member="pytype"),
                            catch.exc,
                        ],
                    ),
                    if_suite=ast.SuiteStmt(catch.suite, ast.BreakStmt()),
                )
            )
            python_catch_body.add_stmt(catch.suite)
        elif exception_class and exception_class.is_type(ast.types.Stdlib.PyError):
            if not node.has(ast.Attr.TryPyVar):
                # Transform PyExc exceptions
                node.set(ast.Attr.TryPyVar, utils.get_temporary_var(self.ctx, "pyexc"))
            python_variable = str(node.attributes[ast.Attr.TryPyVar])
            if catch.var:
                catch.suite = ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(catch.var), rhs=ast.IdExpr(python_variable)),
                    catch.suite,
                )
            catch.suite = ast.SuiteStmt(catch.suite, ast.BreakStmt())
            python_catch_body.add_stmt(catch.suite)
        else:
            if catch.exc:
                # Handle all other exceptions
                catch.exc = self.visit(catch.exc, enforce_type=True)
                exception_type = utils.extract_class_type(self.ctx, catch.exc)
                if not any(
                    parent.is_type(ast.types.Stdlib.BaseException)
                    for parent in utils.get_mro(self.ctx, exception_type)
                ):
                    raise TypecheckError(
                        catch.exc, f"invalid exception type {exception_type.pretty_string()}"
                    )
                if value:
                    infer.unify(value.type, utils.extract_type(self.ctx, exception_type))

            with self.ctx.substitute("block_level", self.ctx.block_level + 1):
                catch.suite = self.visit(catch.suite)
                catch.suite = ast.SuiteStmt.wrap(catch.suite)
            done = done and (catch.exc is None or catch.exc.done)
            done = done and catch.suite and catch.suite.done
            catches.append(catch)

    if python_catch_body.items:
        # Process PyError catches
        python_variable: str = node.attributes[ast.Attr.TryPyVar]
        python_catch_body.add(ast.ThrowStmt())
        python_exception = self.visit(ast.IdExpr(ast.types.Stdlib.PyError), enforce_Type=True)
        catch = ast.TryStmt.Except(python_variable, exc=python_exception, suite=python_catch)
        self.ctx.add_var(
            python_variable,
            python_variable,
            utils.extract_type(self.ctx, python_exception),
            self.time,
        )
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            catch.suite = self.transform(catch.suite)
            catch.suite = ast.SuiteStmt.wrap(catch.suite)
        done = done and catch.exc.done and catch.suite and catch.suite.done
        catches.append(catch)

    node.items = catches
    if node.else_suite:
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            node.else_suite = self.transform(node.else_suite)
            node.else_suite = ast.SuiteStmt.wrap(node.else_suite)
        done = done and node.else_suite and node.else_suite.done
    if node.finally_suite:
        with self.ctx.substitute("block_level", self.ctx.block_level + 1):
            node.finally_suite = self.transform(node.finally_suite)
            node.finally_suite = ast.SuiteStmt.wrap(node.finally_suite)
        done = done and node.finally_suite and node.finally_suite.done
    if done:
        node.done = True
    return node


def typecheck_throw(self: TypeVisitor, node: ast.ThrowStmt) -> ast.Node:
    """
    Transform `raise` statements.
    @example
    `raise exc` -> ```raise BaseException.set_header(exc, "fn", "file", line, col)```
    """
    if node.expr is None:
        node.done = True
        return node

    node.expr = self.transform(node.expr)
    setter_name = ast.types.mangle(
        module="std.internal.types.error", cls="BaseException", func="_set_header"
    )
    match node.expr:
        case ast.CallExpr(ast.IdExpr(value)) if value == setter_name:
            # already wrapped
            pass
        case _:
            base = self.ctx.get_base()
            call = ast.CallExpr(
                ast.IdExpr(setter_name),
                items=[
                    node.expr,
                    ast.StringExpr("" if base is None else base.name),
                    ast.StringExpr(node.info.file),
                    ast.IntExpr(node.info.line),
                    ast.IntExpr(node.info.col),
                    node.from_expr or ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType)),
                ],
            )
            node.expr = self.visit(call)
    if node.expr.done:
        node.done = True
    return node


def transform_with(self: TypeVisitor, node: ast.WithStmt) -> ast.Node:
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

    is_async = node.is_async()
    content = []
    for var, item in reversed(zip(node.vars, node.items)):
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
    result = self.visit(ast.SuiteStmt(*content))
    return result
