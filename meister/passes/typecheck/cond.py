# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast
from ...error import TypecheckError
from . import infer, utils

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_range(_: TypeVisitor, node: ast.RangeExpr):
    """Only allowed in @c MatchStmt"""
    raise TypecheckError(node, "unexpected range expression")


def typecheck_ifexpr(self: TypeVisitor, node: ast.IfExpr) -> ast.Expr:
    """
    Typecheck if expressions. Evaluate static if blocks if possible.
    Also wrap conditional expressions to match each other. See @c wrapExpr for more
    details.
    """
    node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    node.cond = self.visit_expr(node.cond)
    assert node.cond.type and node.type

    # Static if evaluation
    if not node.cond.type.is_runtime:
        condition = False
        if node.cond.type.can_realize():
            match node.cond.type.literal:
                case ast.types.StrLiteral(value=v) if v:
                    condition = True
                case ast.types.IntLiteral(value=v) if v:
                    condition = True
                case ast.types.BoolLiteral(value=v) if v:
                    condition = True
                case _:
                    condition = False
            selected = node.ifexpr if condition else node.elsexpr
            if utils.has_side_effect(node.cond):
                selected = ast.StmtExpr([ast.ExprStmt(node.cond)], expr=selected)
            result = self.visit_expr(selected)

            node.type |= result.type
            return result
        elif unbound := node.type.unbound:
            # determine later!
            unbound._static_kind = ast.types.Type.Behaviour.Int
        return node

    node.ifexpr = self.visit_expr(node.ifexpr)
    node.elsexpr = self.visit_expr(node.elsexpr)
    _, node.cond = utils.wrap_expr(
        self.ctx, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    )
    for branch in (node.ifexpr, node.elsexpr):
        if branch.type and (literal := branch.type.literal):
            # Add wrappers and unify both sides
            branch.type = literal.runtime_type
    nullable = infer.join_nullable_types(self.ctx, node.ifexpr.type, node.elsexpr.type)
    _, node.elsexpr = utils.wrap_expr(
        self.ctx, node.elsexpr, nullable or node.ifexpr.type, allow_unwrap=False
    )
    _, node.ifexpr = utils.wrap_expr(
        self.ctx, node.ifexpr, nullable or node.elsexpr.type, allow_unwrap=False
    )

    assert node.elsexpr.type and node.ifexpr.type
    # Types not compatible! Check if an union can be made
    if (
        node.ifexpr.type.unify(node.elsexpr.type) < 0
        and node.expected_type == ast.types.Stdlib.Union
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
        node.ifexpr = self.visit_expr(ast.CallExpr(union_type, items=[node.ifexpr]))
        node.elsexpr = self.visit_expr(ast.CallExpr(union_type.clone(), items=[node.elsexpr]))
    node.type |= node.ifexpr.type
    node.type |= node.elsexpr.type
    if node.cond.done and node.ifexpr.done and node.elsexpr.done:
        node.done = True
    return node


def typecheck_if(self: TypeVisitor, node: ast.IfStmt) -> ast.Stmt:
    """
    Typecheck if statements. Evaluate static if blocks if possible.
    See @c wrapExpr for more details.
    """

    node.cond.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    node.cond = self.visit_expr(node.cond)

    match node.cond:
        case ast.CallExpr(
            expr=ast.IdExpr(type=ast.types.Function(ast=ast.FunctionStmt(name=fn_name))),
            items=[ast.CallExpr.Arg(value=ast.IdExpr(value=name)) as obj_arg, typ_arg],
        ) if fn_name.startswith(
            (
                ast.types.mangle(cls="RTTIType", func="_isinstance"),
                ast.types.mangle(cls="Any", func="_isinstance"),
            )
        ):
            # isinstance(a, T) ->
            # { if (c := isinstance(a, T)): i = getinstance(a, T)) } ; c
            original_name = name
            name = self.ctx.generate_canonical_name(utils.get_unmangled_name(self.ctx, name))
            condition_name = utils.get_temporary_var(self.ctx, "cond")
            if node.if_suite:
                renames = dict(node.if_suite.get(ast.Attr.LocalRenames, dict, {}))
                source_name = utils.get_unmangled_name(self.ctx, original_name)
                for alias, target in list(renames.items()):
                    if target in {source_name, original_name}:
                        renames[alias] = name
                renames[source_name] = name
                node.if_suite.set(ast.Attr.LocalRenames, renames)
            getter = fn_name.replace("_isinstance", "_getinstance")
            result = self.visit_stmt(
                ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(condition_name), rhs=node.cond),
                    ast.IfStmt(
                        ast.IdExpr(condition_name),
                        if_suite=ast.SuiteStmt(
                            ast.AssignStmt(
                                ast.IdExpr(name),
                                rhs=ast.CallExpr(ast.IdExpr(getter), items=[obj_arg, typ_arg]),
                            ),
                            *([] if not node.if_suite else node.if_suite.items),
                        ),
                        else_suite=node.else_suite,
                    ),
                )
            )
            return result

    # Static if evaluation
    assert node.cond.type
    if not node.cond.type.is_runtime:
        if not node.cond.type.can_realize():
            return node

        condition = False
        if node.cond.type.can_realize():
            match node.cond.type.literal:
                case ast.types.StrLiteral(value=v) if v:
                    condition = True
                case ast.types.IntLiteral(value=v) if v:
                    condition = True
                case ast.types.BoolLiteral(value=v) if v:
                    condition = True
                case _:
                    condition = False
            selected = node.if_suite if condition else node.else_suite
            if utils.has_side_effect(node.cond):
                selected = ast.SuiteStmt(ast.ExprStmt(node.cond), selected)
            # C++ transforms the selected null branch into a generated empty
            # suite after evaluating the condition.
            result = self.visit_stmt(selected or ast.SuiteStmt())
            return result

    _, node.cond = utils.wrap_expr(
        self.ctx, node.cond, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
    )
    with self.ctx.substitute("block_level", self.ctx.block_level + 1):
        node.if_suite = ast.SuiteStmt.wrap(self.visit_stmt(node.if_suite))
        node.else_suite = (
            ast.SuiteStmt.wrap(self.visit_stmt(node.else_suite)) if node.else_suite else None
        )
    if (
        node.cond.done
        and (not node.if_suite or node.if_suite.done)
        and (not node.else_suite or node.else_suite.done)
    ):
        node.done = True
    return node


def typecheck_match(self: TypeVisitor, node: ast.MatchStmt) -> ast.Stmt:
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
    assignment = ast.AssignStmt(ast.IdExpr(var), rhs=node.expr.clone())
    assignment = self.visit_stmt(assignment)
    result = ast.SuiteStmt(assignment)
    for case in node.items:
        case_suite = ast.SuiteStmt(case.suite, ast.BreakStmt())
        if case.guard:
            case_suite = ast.IfStmt(case.guard, if_suite=ast.SuiteStmt(case_suite))
        result.add(typecheck_pattern(self, ast.IdExpr(var), case.pattern, case_suite))
    # Make sure to break even if there is no case _ to prevent infinite loop
    result.add(ast.BreakStmt())
    loop = ast.WhileStmt(ast.BoolExpr(True), suite=result)
    return self.visit_stmt(loop)


def typecheck_pattern(self, var, pattern, suite):
    """Lower match patterns, evaluating each field or index only once."""
    if not isinstance(var, ast.IdExpr):
        tmp = utils.get_temporary_var(self.ctx, "match")
        return ast.SuiteStmt(
            ast.AssignStmt(ast.IdExpr(tmp), var),
            typecheck_pattern(self, ast.IdExpr(tmp), pattern, suite),
        )

    def instance(value, typ):
        return ast.CallExpr(ast.IdExpr("isinstance"), [value.clone(), ast.IdExpr(typ)])

    match pattern:
        case ast.NoneExpr():
            return ast.IfStmt(ast.BinaryExpr(var, "is", pattern), suite)
        case ast.IntExpr() | ast.BoolExpr():
            typ = ast.types.Stdlib.Bool if isinstance(pattern, ast.BoolExpr) else "int"
            return ast.IfStmt(
                instance(var, typ), ast.IfStmt(ast.BinaryExpr(var, "==", pattern), suite)
            )
        case ast.RangeExpr(start=start, stop=stop):
            return ast.IfStmt(
                instance(var, "int"),
                ast.IfStmt(
                    ast.BinaryExpr(var, ">=", start),
                    ast.IfStmt(ast.BinaryExpr(var.clone(), "<=", stop), suite),
                ),
            )
        case ast.TupleExpr(items=items):
            is_tuple = type(pattern) is ast.TupleExpr
            wildcard, size = len(items), len(items)
            for idx, item in enumerate(items):
                if isinstance(item, ast.StarExpr) and not (
                    isinstance(item.expr, ast.IdExpr) and item.expr.value == "_"
                ):
                    raise TypecheckError(item, "only *_ is supported in sequence patterns")
                if isinstance(item, (ast.EllipsisExpr, ast.StarExpr)):
                    if wildcard != size:
                        raise TypecheckError(item, "multiple ellipses in a pattern")
                    wildcard = idx
            for idx in range(size - 1, wildcard, -1):
                suite = typecheck_pattern(
                    self, ast.IndexExpr(var.clone(), ast.IntExpr(idx - size)), items[idx], suite
                )
            for idx in range(wildcard - 1, -1, -1):
                suite = typecheck_pattern(
                    self, ast.IndexExpr(var.clone(), ast.IntExpr(idx)), items[idx], suite
                )
            length = ast.CallExpr(
                ast.IdExpr(
                    ast.types.mangle("std.internal.static", func="len") if is_tuple else "len"
                ),
                [var.clone()],
            )
            gap = wildcard != size
            return ast.IfStmt(
                instance(var, ast.types.Stdlib.Tuple if is_tuple else ast.types.Stdlib.List),
                ast.IfStmt(
                    ast.BinaryExpr(length, ">=" if gap else "==", ast.IntExpr(size - gap)), suite
                ),
            )
        case ast.BinaryExpr(op="|" | "||"):
            return ast.SuiteStmt(
                typecheck_pattern(self, var.clone(), pattern.lexpr, suite.clone()),
                typecheck_pattern(self, var, pattern.rexpr, suite),
            )
        case ast.IdExpr(value="_"):
            return suite
        case ast.IdExpr():
            return ast.SuiteStmt(ast.AssignStmt(pattern, var), suite)
        case ast.AssignExpr():
            assert isinstance(pattern.var, ast.IdExpr), (
                "only simple assignment expressions are supported"
            )
            return typecheck_pattern(
                self,
                var,
                pattern.expr,
                ast.SuiteStmt(ast.AssignStmt(pattern.var, var.clone()), suite),
            )
        case ast.CallExpr():
            head = self.visit_expr(pattern.expr.clone())
            if not utils.is_type_expr(head):
                raise TypecheckError(pattern, "called match pattern must be a class")
            if not pattern.items or any(arg.name for arg in pattern.items):
                for arg in reversed(pattern.items):
                    if not arg.name:
                        raise TypecheckError(arg.value, "expected a keyword class pattern")
                    attribute = utils.get_temporary_var(self.ctx, "match_attr")
                    suite = ast.IfStmt(
                        ast.CallExpr(
                            ast.IdExpr("hasattr"), [var.clone(), ast.StringExpr(arg.name)]
                        ),
                        ast.TryStmt(
                            ast.AssignStmt(
                                ast.IdExpr(attribute), ast.DotExpr(var.clone(), arg.name)
                            ),
                            items=[
                                ast.TryStmt.Except(
                                    ast.IdExpr(
                                        ast.types.mangle(
                                            "std.internal.types.error", cls="AttributeError"
                                        )
                                    ),
                                    suite=ast.SuiteStmt(),
                                )
                            ],
                            else_suite=typecheck_pattern(
                                self, ast.IdExpr(attribute), arg.value, suite
                            ),
                        ),
                    )
                temporary = utils.get_temporary_var(self.ctx, "match")
                optional_suite = ast.SuiteStmt.wrap(suite.clone())
                optional_suite.set(
                    ast.Attr.LocalRenames,
                    {utils.get_unmangled_name(self.ctx, var.value): temporary},
                )
                return ast.IfStmt(
                    instance(var, ast.types.Stdlib.Optional),
                    ast.IfStmt(
                        ast.BinaryExpr(var.clone(), "is not", ast.NoneExpr()),
                        ast.SuiteStmt(
                            ast.AssignStmt(
                                ast.IdExpr(temporary),
                                ast.CallExpr(ast.IdExpr("unwrap"), [var.clone()]),
                            ),
                            ast.IfStmt(
                                ast.CallExpr(
                                    ast.IdExpr("isinstance"), [ast.IdExpr(temporary), head.clone()]
                                ),
                                optional_suite,
                            ),
                        ),
                    ),
                    ast.IfStmt(ast.CallExpr(ast.IdExpr("isinstance"), [var, head]), suite),
                )

    pattern = self.visit_expr(pattern)
    if isinstance(pattern, ast.EllipsisExpr):
        pattern = ast.CallExpr(ast.IdExpr("ellipsis"))

    # Fallback (`__match__`) pattern
    has_match = ast.CallExpr(
        ast.IdExpr("hasattr"), items=[var.clone(), ast.StringExpr("__match__"), pattern.clone()]
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
