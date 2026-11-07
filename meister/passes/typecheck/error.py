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
        ast.DotExpr(
            ast.IdExpr("__internal__"),
            member="seq_assert_test" if is_test else "seq_assert",
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
            py_var = node.get(ast.Attr.TryPyVar, str, "")
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
            py_var = node.get(ast.Attr.TryPyVar, str, "")
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
        py_var = node.get(ast.Attr.TryPyVar, str, "")
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


def typecheck_throw(self, node):
    """
    Stage raise operands in source order and initialize exception headers once.

    Transform `raise` statements.
    @example
    `raise exc` -> ```raise BaseException.set_header(exc, "fn", "file", line, col)```
    """
    if not node.expr:
        node.done = True
        return node
    node.expr = self.visit_expr(node.expr)
    setter = ast.types.mangle("std.internal.types.error", cls="BaseException", func="_set_header")
    head = utils.get_head_expr(node.expr)
    initialized = (
        isinstance(head, ast.CallExpr)
        and isinstance(head.expr, ast.IdExpr)
        and head.expr.value == setter
    )
    # Cause staging can wrap the header in a StmtExpr. Recognize it on later passes
    # so neither exception construction nor header initialization is repeated.
    # Internal explicit rethrows also retain the original exception's metadata.
    if not node.has(ast.Attr.Internal) and not initialized:
        exception = node.expr
        cause = self.visit_expr(
            node.from_expr or ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
        )
        setup = []
        # `raise Error from cause()` evaluates cause() before constructing Error().
        # Save raw operands in source order before normalizing classes to instances;
        # in particular, a failing cause expression must not run Error's constructor.
        if utils.is_type_expr(exception) and node.from_expr:
            en, cn = (
                utils.get_temporary_var(self.ctx, "exception"),
                utils.get_temporary_var(self.ctx, "cause"),
            )
            setup.append(self.visit_stmt(ast.AssignStmt(ast.IdExpr(en), exception)))
            setup.append(self.visit_stmt(ast.AssignStmt(ast.IdExpr(cn), cause)))
            exception, cause = self.visit_expr(ast.IdExpr(en)), self.visit_expr(ast.IdExpr(cn))
        if utils.is_type_expr(exception):
            exception = self.visit_expr(ast.CallExpr(exception))
        if utils.is_type_expr(cause):
            cause = self.visit_expr(ast.CallExpr(cause))
        header = ast.CallExpr(
            ast.IdExpr(setter),
            [
                exception,
                ast.StringExpr(self.ctx.base_name),
                ast.StringExpr(node.info.file),
                ast.IntExpr(node.info.line),
                ast.IntExpr(node.info.col),
                cause,
            ],
        )
        node.expr = self.visit_expr(ast.StmtExpr(setup, header) if setup else header)
    if node.expr.done:
        node.done = True
    return node


def typecheck_with(self, node):
    """
    Save managers and exit callables separately from entry-result bindings.

    Transform with statements, saving the manager and exit callable independently
    of the entry-result binding.
    @example
    `with manager() as value: BODY` ->
    ```python
    tmp = manager()
    exit_method = tmp.__exit__
    entered = tmp.__enter__()
    if BaseException._context_exit_is_legacy_none(exit_method, False):
    try:
    value = entered
    BODY
    finally:
    exit_method()
    else:
    exceptional = False
    try:
    value = entered
    BODY
    except BaseException as error:
    exceptional = True
    if not BaseException._context_exit(exit_method, get_typeinfo(error),
    error):
    raise error  # Internal rethrow preserves the original metadata.
    finally:
    if not exceptional:
    BaseException._context_exit(exit_method, None, None)
    ```
    _context_exit_is_legacy_none selects a cleanup branch statically; both alternatives
    share BODY to avoid recursive duplication.

    _context_exit supplies a None traceback, prefers a callable three-argument exit, and
    otherwise calls the legacy zero-argument exit. Entry stays outside the try, while
    target assignment stays inside. The flag prevents a second exit call if exceptional
    cleanup suppresses or raises; finally handles normal completion and
    return/break/continue.

    For async with, save __aexit__, await __aenter__, pass True to the predicate, and
    await either the legacy exit or _context_aexit.

    Multiple managers nest this transformation so exits run in reverse entry order.
    Without an as target, entry is evaluated without binding its result and the target
    assignment is omitted.
    """
    assert node.items, "stmt.items is empty"
    is_async, content = node.async_, []
    helper = lambda name: ast.IdExpr(
        ast.types.mangle("std.internal.types.error", cls="BaseException", func=name)
    )
    for target, item in reversed(list(zip(node.vars, node.items))):
        var, exit_var, exceptional, exception, exit_result = [
            utils.get_temporary_var(self.ctx, name)
            for name in ("with", "exit", "with_exception", "exception", "exit_result")
        ]
        assignment = ast.AssignStmt(ast.IdExpr(var), item)
        save_exit = ast.AssignStmt(
            ast.IdExpr(exit_var),
            ast.DotExpr(ast.IdExpr(var), "__aexit__" if is_async else "__exit__"),
        )
        enter = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(var), "__aenter__" if is_async else "__enter__")
        )
        if is_async:
            enter = ast.AwaitExpr(enter)

        def call_exit(exc_type, exc_value):
            call = ast.CallExpr(
                helper("_context_aexit" if is_async else "_context_exit"),
                [ast.IdExpr(exit_var), exc_type, exc_value],
            )
            return ast.AwaitExpr(call) if is_async else call

        body = ast.SuiteStmt(*content) if content else node.suite.clone()
        entry = ast.ExprStmt(enter)
        if target:
            entered = utils.get_temporary_var(self.ctx, "entered")
            entry = ast.AssignStmt(ast.IdExpr(entered), enter)
            body = ast.SuiteStmt(
                ast.AssignStmt(
                    ast.IdExpr(target),
                    ast.IdExpr(entered),
                    update=ast.AssignStmt.Mode.Update
                    if item.has(ast.Attr.ExprDominated)
                    else ast.AssignStmt.Mode.Assign,
                ),
                body,
            )
        rethrow = ast.ThrowStmt(ast.IdExpr(exception))
        # An awaited exit can suspend outside the original native catch frame. Rethrow
        # the captured object rather than relying on the thread's active exception.
        rethrow.set(ast.Attr.Internal)
        handler = ast.TryStmt.Except(
            ast.IdExpr(ast.types.Stdlib.BaseException),
            var=exception,
            suite=ast.SuiteStmt(
                ast.AssignStmt(
                    ast.IdExpr(exceptional), ast.BoolExpr(True), update=ast.AssignStmt.Mode.Update
                ),
                ast.AssignStmt(
                    ast.IdExpr(exit_result),
                    call_exit(
                        ast.CallExpr(
                            ast.IdExpr(
                                ast.types.mangle("std.internal.types.rtti", func="get_typeinfo")
                            ),
                            [ast.IdExpr(exception)],
                        ),
                        ast.IdExpr(exception),
                    ),
                ),
                ast.IfStmt(ast.UnaryExpr("!", ast.IdExpr(exit_result)), rethrow),
            ),
        )
        none = lambda: ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
        full = ast.SuiteStmt(
            ast.AssignStmt(ast.IdExpr(exceptional), ast.BoolExpr(False)),
            ast.TryStmt(
                body,
                items=[handler],
                finally_suite=ast.SuiteStmt(
                    ast.IfStmt(
                        ast.UnaryExpr("!", ast.IdExpr(exceptional)),
                        ast.ExprStmt(call_exit(none(), none())),
                    )
                ),
            ),
        )
        legacy_exit = ast.CallExpr(ast.IdExpr(exit_var))
        if is_async:
            legacy_exit = ast.AwaitExpr(legacy_exit)
        legacy = ast.TryStmt(body, finally_suite=ast.SuiteStmt(ast.ExprStmt(legacy_exit)))
        predicate = ast.CallExpr(
            helper("_context_exit_is_legacy_none"), [ast.IdExpr(exit_var), ast.BoolExpr(is_async)]
        )
        content = [assignment, save_exit, entry, ast.IfStmt(predicate, legacy, full)]
    return self.visit_stmt(ast.SuiteStmt(*content))
