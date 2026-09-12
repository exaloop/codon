# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from meister.converted.passes import scope

from ... import ast, cache
from ...error import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_stmt(self: TypeVisitor, node: ast.StmtExpr) -> ast.Expr:
    """Typecheck statement expressions."""

    assert node.type

    done = True
    statements = []
    for stmt in node.items:
        transformed = self.visit_stmt(stmt)
        statements.append(transformed)
        done = done and transformed.done
    node.items = statements
    node.expr = self.visit_expr(node.expr)
    node.type |= node.expr.type
    if done and node.expr.done:
        node.done = True
    return node


def typecheck_suite(self: TypeVisitor, node: ast.SuiteStmt) -> ast.Stmt:
    """Typecheck a list of statements."""

    output = []
    if bindings := node.get(ast.Attr.Bindings, scope.Bindings):
        prepended = []
        for name, binding in bindings.bindings.items():
            prepended.append(ast.AssignStmt(ast.IdExpr(name)))
            if binding.count > 0:
                prepended.append(
                    ast.AssignStmt(
                        ast.IdExpr(f"{name}{cache.VAR_USED_SUFFIX}"), rhs=ast.BoolExpr(False)
                    )
                )
        node.erase(ast.Attr.Bindings)
        node.items[0:0] = prepended
    if local_renames := node.get(ast.Attr.LocalRenames, dict[str, str]):
        for original, renamed in local_renames.items():
            self.ctx.add(original, self.ctx[renamed])
    try:
        done = True
        for statement in node.items:
            if self.ctx.return_early:
                # If returnEarly is set (e.g., in the function) ignore the rest
                break
            transformed = self.visit_stmt(statement)
            if isinstance(transformed, ast.SuiteStmt):
                for nested in transformed.items:
                    done = done and nested.done
                    output.append(nested)
            elif isinstance(transformed, ast.Stmt):
                done = done and transformed.done
                output.append(transformed)
        node.items = output
        if done:
            node.done = True
    finally:
        if local_renames:
            for original in local_renames:
                self.ctx.remove(original)
    return node


def typecheck_expr(self: TypeVisitor, node: ast.ExprStmt) -> ast.Stmt:
    """Typecheck expression statements."""

    node.expr = self.visit_expr(node.expr)
    node.done = node.expr.done
    return node


def typecheck_custom(self: TypeVisitor, node: ast.CustomStmt) -> ast.Stmt:
    if node.suite:
        block_callback = self.ctx.cache.custom_block_stmts.get(node.keyword)
        assert block_callback is not None, f"unknown keyword {node.keyword}"
        result = block_callback[1](self.ctx, node)
    else:
        expression_callback = self.ctx.cache.custom_expr_stmts.get(node.keyword)
        assert expression_callback is not None, f"unknown keyword {node.keyword}"
        result = expression_callback(self.ctx, node.expr)
    return result


def typecheck_comment(_: TypeVisitor, node: ast.CommentStmt) -> ast.Stmt:
    node.done = True
    return node


def typecheck_directive(self: TypeVisitor, node: ast.DirectiveStmt) -> ast.Stmt:
    if node.key == "auto_python":
        self.ctx.auto_python = node.value == "1"
        self.log(f"directive '{node.key}' = {self.ctx.auto_python}")
    else:
        raise TypecheckError(node, f"unknown directive '{node.key}'")
    node.done = True
    return node
