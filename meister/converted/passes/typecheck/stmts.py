# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ... import ast, cache
from . import TypecheckError, TypecheckVisitor


def typecheck_stmt(self: TypecheckVisitor, node: ast.StmtExpr) -> ast.Node:
    """Typecheck statement expressions."""

    done = True
    statements = []
    for stmt in node.items:
        transformed = self.visit(stmt)
        statements.append(transformed)
        done = done and transformed.done
    node.items = statements
    node.expr = self.visit(node.expr)
    node |= node.expr.type
    if done and node.expr.done:
        node.done = True
    return node


def typecheck_suite(self: TypecheckVisitor, node: ast.SuiteStmt) -> ast.Node:
    """Typecheck a list of statements."""

    output = []
    if bindings := node.get(ast.Attr.Bindings):
        prepended = []
        for name, binding in bindings.bindings.items():
            prepended.append(ast.AssignStmt(ast.IdExpr(name)))
            if binding.count > 0:
                prepended.append(
                    ast.AssignStmt(
                        ast.IdExpr(f"{name}{cache.VAR_USED_SUFFIX}"),
                        rhs=ast.BoolExpr(value=False),
                    )
                )
        node.erase(ast.Attr.Bindings)
        node.items[0:0] = prepended
    if local_renames := node.get(ast.Attr.LocalRenames):
        for original, renamed in local_renames.attributes.items():
            self.ctx.add(original, self.ctx.force_find(renamed))
    try:
        done = True
        for statement in node.items:
            if self.ctx.return_early:
                # If returnEarly is set (e.g., in the function) ignore the rest
                break
            transformed = self.visit(statement)
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
            for original in local_renames.attributes:
                self.ctx.remove(original)
    return node

def typecheck_expr(self, node: ast.ExprStmt) -> ast.Node:
    """Typecheck expression statements."""

    node.expr = self.visit(node.expr)
    node.done = node.expr.done
    return node

def typecheck_custom(self, node: ast.CustomStmt) -> ast.Node:
    if node.suite:
        block_callback = self.ctx.cache.custom_block_stmts.get(node.keyword)
        assert block_callback is not None, f"unknown keyword {node.keyword}"
        result = block_callback[1](self, node)
    else:
        expression_callback = self.ctx.cache.custom_expr_stmts.get(node.keyword)
        assert expression_callback is not None, f"unknown keyword {node.keyword}"
        result = expression_callback(self, node)
    return result

def typecheck_comment(self, node: ast.CommentStmt) -> ast.Node:
    node.done = True
    return node

def typecheck_directive(self, node: ast.DirectiveStmt) -> ast.Node:
    if node.key == "auto_python":
        self.ctx.auto_python = node.value == "1"
        self.log(f"directive '{node.key}' = {self.ctx.auto_python}")
    else:
        raise TypecheckError(node, f"unknown directive '{node.key}'")
    node.done = True
    return node
