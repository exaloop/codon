# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast
from . import infer, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_range(self: TypeVisitor, node: ast.RangeExpr) -> ast.Node:
    """Only allowed in @c MatchStmt"""
    raise TypecheckError(node, "unexpected range expression")


def typecheck_ifexpr(self: TypeVisitor, node: ast.IfExpr) -> ast.Node:
    """
    Typecheck if expressions. Evaluate static if blocks if possible.
    Also wrap conditional expressions to match each other. See @c wrapExpr for more
    details.
    """
    node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    node.cond = self.visit(node.cond)
    assert node.cond.type

    # Static if evaluation
    if node.cond.type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
        condition = False
        if node.cond.type.can_realize():
            match node.cond.type:
                case ast.types.StrLiteral(value=v) if v:
                    condition = True
                case ast.types.IntLiteral(value=v) if v >= 0:
                    condition = True
                case ast.types.BoolLiteral(value=v) if v:
                    condition = True
                case _:
                    condition = False
            selected = node.ifexpr if condition else node.elsexpr
            if utils.has_side_effect(node.cond):
                selected = ast.StmtExpr([ast.ExprStmt(node.cond)], expr=selected)
            result = self.visit(selected)
            infer.unify(node.type, result.type)
            return result
        elif unbound := node.type.get_unbound():
            # determine later!
            unbound.static_kind = ast.types.Type.Behaviour.Int
        return node

    node.ifexpr = self.visit(node.ifexpr)
    node.elsexpr = self.visit(node.elsexpr)
    _, node.cond = utils.wrap_expr(
        self, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    )
    for branch in (node.ifexpr, node.elsexpr):
        if static := branch.type.get_static():
            # Add wrappers and unify both sides
            branch.type = static.get_non_static_type()
    _, node.elsexpr = utils.wrap_expr(self, node.elsexpr, node.ifexpr.type, allow_unwrap=False)
    _, node.ifexpr = utils.wrap_expr(self, node.ifexpr, node.elsexpr.type, allow_unwrap=False)
    # Types not compatible! Check if an union can be made
    if (
        node.ifexpr.type.unify(node.elsexpr.type) < 0
        and node.expected_type
        and node.expected_type.is_type(ast.types.Stdlib.Union)
    ):
        if not node.ifexpr.type.can_realize() or not node.elsexpr.type.can_realize():
            return node
        union_type = ast.InstantiateExpr(
            ast.IdExpr(ast.types.Stdlib.Union),
            items=[
                ast.IdExpr(node.ifexpr.type.realized_name()),
                ast.IdExpr(node.elsexpr.type.realized_name()),
            ],
        )
        node.ifexpr = self.visit(ast.CallExpr(union_type, items=[node.ifexpr]))
        node.elsexpr = self.visit(ast.CallExpr(union_type.clone(), items=[node.elsexpr]))
    infer.unify(node.type, node.ifexpr.type)
    infer.unify(node.type, node.elsexpr.type)
    if node.cond.done and node.ifexpr.done and node.elsexpr.done:
        node.done = True
    return node


def typecheck_if(self: TypeVisitor, node: ast.IfStmt) -> ast.Node:
    """
    Typecheck if statements. Evaluate static if blocks if possible.
    See @c wrapExpr for more details.
    """

    node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    node.cond = self.visit(node.cond)

    match node.cond:
        case ast.CallExpr(
            ast.IdExpr(type=ast.types.Function(ast=ast.FunctionStmt(name=fn_name))),
            args=[obj_arg, typ_arg],
        ) if fn_name.startswith(
            (
                ast.types.mangle(cls="RTTIType", func="_isinstance"),
                ast.types.mangle(cls="Any", func="_isinstance"),
            )
        ):
            # isinstance(a, T) ->
            # { if (c := isinstance(a, T)): i = getinstance(a, T)) } ; c
            name = self.ctx.generate_canonical_name(
                utils.get_unmangled_name(self.ctx, obj_arg.value)
            )
            condition_name = utils.get_temporary_var(self.ctx, "cond")
            if node.if_suite:
                node.if_suite.set(
                    ast.Attr.LocalRenames, {utils.get_unmangled_name(self.ctx, obj_arg.value): name}
                )
            getter = fn_name.replace("_isinstance", "_getinstance")
            result = self.visit(
                ast.SuiteStmt(
                    [
                        ast.AssignStmt(ast.IdExpr(condition_name), rhs=node.cond),
                        ast.IfStmt(
                            ast.IdExpr(condition_name),
                            if_suite=ast.SuiteStmt(
                                [
                                    ast.AssignStmt(
                                        ast.IdExpr(name),
                                        rhs=ast.CallExpr(
                                            ast.IdExpr(getter), items=[obj_arg.value, typ_arg.value]
                                        ),
                                    ),
                                    *([] if not node.if_suite else node.if_suite.items),
                                ]
                            ),
                            else_suite=node.else_suite,
                        ),
                    ]
                )
            )
            return result
    # Static if evaluation
    if node.cond.type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
        if not node.cond.type.can_realize():
            return node

        condition = False
        if node.cond.type.can_realize():
            match node.cond.type:
                case ast.types.StrLiteral(value=v) if v:
                    condition = True
                case ast.types.IntLiteral(value=v) if v >= 0:
                    condition = True
                case ast.types.BoolLiteral(value=v) if v:
                    condition = True
                case _:
                    condition = False
            selected = node.if_suite if condition else node.else_suite
            if utils.has_side_effect(node.cond):
                selected = ast.SuiteStmt([ast.ExprStmt(node.cond), selected])
            result = self.visit(selected)
            return result

    _, node.cond = utils.wrap_expr(
        self, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    )
    with self.ctx.substitute("block_level", self.ctx.block_level + 1):
        node.if_suite = self.visit(node.if_suite)
        node.else_suite = self.visit(node.else_suite) if node.else_suite else None
        node.if_suite = ast.SuiteStmt.wrap(node.if_suite)
        node.else_suite = ast.SuiteStmt.wrap(node.else_suite)
    if (
        node.cond.done
        and (not node.if_suite or node.if_suite.done)
        and (not node.else_suite or node.else_suite.done)
    ):
        node.done = True
    return node


def typecheck_match(self: TypeVisitor, node: ast.MatchStmt) -> ast.Node:
    """
    Simplify match statement by transforming it into a series of conditional statements.
    @example
    ```match e:
    case pattern1: ...
    case pattern2 if guard: ...
    ...``` ->
    ```_match = e
    while True:  # used to simulate goto statement with break
    [pattern1 transformation]: (...; break)
    [pattern2 transformation]: if guard: (...; break)
    ...
    break  # exit the loop no matter what```
    The first pattern that matches the given expression will be used; other patterns
    will not be used (i.e., there is no fall-through). See @c transformPattern for
    pattern transformations
    """

    var = utils.get_temporary_var(self.ctx, "match")
    assignment: ast.Stmt = ast.AssignStmt(ast.IdExpr(var), rhs=node.expr.clone())
    assignment = self.visit(assignment)
    result = ast.SuiteStmt([assignment])
    for case in node.items:
        case_suite = ast.SuiteStmt([case.suite, ast.BreakStmt()])
        if case.guard:
            case_suite = ast.IfStmt(case.guard, if_suite=ast.SuiteStmt([case_suite]))
        result.add(typecheck_pattern(self, ast.IdExpr(var), case.pattern, case_suite))
    # Make sure to break even if there is no case _ to prevent infinite loop
    result.add(ast.BreakStmt())
    loop = ast.WhileStmt(ast.BoolExpr(True), suite=result)
    return self.visit(loop)


def typecheck_pattern(
    self: TypeVisitor, var: ast.Expr, pattern: ast.Expr, suite: ast.Stmt
) -> ast.Stmt:
    """
    Transform a match pattern into a series of if statements.
    @example
    `case True`          -> `if isinstance(var, bool): if var == True`
    `case 1`             -> `if isinstance(var, "int"): if var == 1`
    `case 1...3`         -> ```if isinstance(var, "int"):
    if var >= 1: if var <= 3```
    `case (1, pat)`      -> ```if isinstance(var, "Tuple"): if static.len(var) == 2:
    if match(var[0], 1): if match(var[1], pat)```
    `case [1, ..., pat]` -> ```if isinstance(var, "List"): if len(var) >= 2:
    if match(var[0], 1): if match(var[-1], pat)```
    `case 1 or pat`      -> `if match(var, 1): if match(var, pat)`
    (note: pattern suite is cloned for each `or`)
    `case (x := pat)`    -> `(x := var; if match(var, pat))`
    `case x`             -> `(x := var)`
    (only when `x` is not '_')
    `case expr`          -> `if hasattr(typeof(var), "__match__"): if
    var.__match__(foo())`
    (any expression that does not fit above patterns)
    """

    def isinstance_call(expression: ast.Expr, type_name: str) -> ast.Expr:
        """Convenience function to generate `isinstance(e, typ)` calls"""
        return ast.CallExpr(
            ast.IdExpr("isinstance"),
            items=[expression.clone(), ast.IdExpr(type_name)],
        )

    def ellipsis_index(items: List[ast.Expr]) -> int:
        """Convenience function to find the index of an ellipsis within a list pattern."""
        # TODO: replace with StarExpr
        result = len(items)
        for index, item in enumerate(items):
            if isinstance(item, ast.EllipsisExpr):
                if result != len(items):
                    raise TypecheckError(item, "multiple ellipses in a pattern")
                result = index
        return result

    match pattern:
        case ast.IntExpr():
            return ast.IfStmt(
                isinstance_call(var, ast.types.Stdlib.Int),
                if_suite=ast.IfStmt(ast.BinaryExpr(var, "==", pattern), suite),
            )
        case ast.BoolExpr():
            return ast.IfStmt(
                isinstance_call(var, ast.types.Stdlib.Bool),
                if_suite=ast.IfStmt(ast.BinaryExpr(var, "==", pattern), suite),
            )
        case ast.RangeExpr(start=start, stop=stop):
            upper = ast.IfStmt(ast.BinaryExpr(var.clone(), "<=", stop), suite)
            lower = ast.IfStmt(ast.BinaryExpr(var, ">=", start), upper)
            return ast.IfStmt(isinstance_call(var, "int"), lower)
        case ast.TupleExpr(items=items):
            nested = suite
            for idx in range(len(items) - 1, -1, -1):
                nested = typecheck_pattern(
                    self,
                    ast.IndexExpr(var.clone(), index=ast.IntExpr(int_value=idx)),
                    items[idx],
                    nested,
                )
            length = ast.CallExpr(
                ast.IdExpr(ast.types.mangle("std.internal.static", func="len")),
                items=[var],
            )
            return ast.IfStmt(
                isinstance_call(var, ast.types.Stdlib.Tuple),
                ast.IfStmt(
                    ast.BinaryExpr(length, "==", ast.IntExpr(int_value=len(items))), if_suite=nested
                ),
            )
        case ast.ListExpr(items=items):
            ellipsis = ellipsis_index(items)
            size = len(items)
            op = "=="
            if ellipsis != len(items):
                op, size = ">=", size - 1
            nested = suite
            for idx in range(len(items) - 1, ellipsis, -1):
                relative = idx - len(items)
                nested = typecheck_pattern(
                    self,
                    ast.IndexExpr(var.clone(), idx=ast.IntExpr(int_value=relative)),
                    items[idx],
                    nested,
                )
            for idx in range(ellipsis - 1, -1, -1):
                nested = typecheck_pattern(
                    self,
                    ast.IndexExpr(var.clone(), idx=ast.IntExpr(int_value=idx)),
                    items[idx],
                    nested,
                )
            return ast.IfStmt(
                isinstance_call(var, "List"),
                ast.IfStmt(
                    ast.BinaryExpr(
                        ast.CallExpr(ast.IdExpr("len"), items=[var]),
                        op,
                        ast.IntExpr(int_value=size),
                    ),
                    if_suite=nested,
                ),
            )
        case ast.BinaryExpr(op=op) if op in ("|", "||"):
            return ast.SuiteStmt(
                [
                    typecheck_pattern(self, var.clone(), pattern.lexpr, suite.clone()),
                    typecheck_pattern(self, var, pattern.rexpr, suite),
                ]
            )
        case ast.IdExpr(value="_"):
            return suite
        case ast.IdExpr():
            return ast.SuiteStmt([ast.AssignStmt(pattern, rhs=var), suite])
        case ast.AssignExpr(var=ast.IdExpr() as var, expr=expr):
            return ast.SuiteStmt(
                [ast.AssignStmt(var, rhs=var.clone()), typecheck_pattern(self, var, expr, suite)]
            )
        case ast.AssignExpr:
            assert False, "only simple assignment expressions are supported"

    pattern = self.visit(pattern)
    if isinstance(pattern, ast.EllipsisExpr):
        pattern = ast.CallExpr(ast.IdExpr("ellipsis"))

    # Fallback (`__match__`) pattern
    has_match = ast.CallExpr(
        ast.IdExpr("hasattr"),
        items=[var.clone(), ast.StringExpr(value="__match__"), pattern.clone()],
    )
    match_call = ast.CallExpr(
        ast.DotExpr(var.clone().clone(), "__match__"), items=[pattern.clone()]
    )
    same_type = ast.CallExpr(
        ast.IdExpr("isinstance"),
        items=[
            ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Type), items=[var.clone()]),
            ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Type), items=[pattern.clone()]),
        ],
    )
    return ast.IfStmt(
        has_match,
        if_suite=ast.IfStmt(match_call, suite),
        else_suite=ast.IfStmt(
            same_type, if_suite=ast.IfStmt(ast.BinaryExpr(var, "==", pattern), suite.clone())
        ),
    )
