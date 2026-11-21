# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast
from ...bridge import List, cast
from ...error import TypecheckError
from . import classes, infer, loops, utils

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_tuple(self: TypeVisitor, node: ast.TupleExpr) -> ast.Node:
    """
    Transform tuples.
        @example
        `(a1, ..., aN)` -> `Tuple.__new__(a1, ..., aN)`
    """

    result = self.visit_expr(
        ast.CallExpr(
            ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Tuple), member="__new__"),
            items=node.items,
        )
    )
    return result


def typecheck_list(self: TypeVisitor, node: ast.ListExpr) -> ast.Node:
    """
    Transform a list `[a1, ..., aN]` to the corresponding statement expression.
    See @c transformComprehension
    """

    node.type = utils.instantiate_unbound(self.ctx)
    result = transform_comprehension(
        self, ast.types.Stdlib.List, node.items, method="append", expected_type=node.expected_type
    )
    if result:
        result.set(ast.Attr.ExprList)
        return result
    return node


def typecheck_set(self: TypeVisitor, node: ast.SetExpr) -> ast.Node:
    """
    Transform a set `{a1, ..., aN}` to the corresponding statement expression.
    See @c transformComprehension
    """

    node.type = utils.instantiate_unbound(self.ctx)
    result = transform_comprehension(
        self, ast.types.Stdlib.Set, node.items, method="add", expected_type=node.expected_type
    )
    if result:
        result.set(ast.Attr.ExprSet)
        return result
    return node


def typecheck_dict(self: TypeVisitor, node: ast.DictExpr) -> ast.Node:
    """
    Transform a dictionary `{k1: v1, ..., kN: vN}` to a corresponding statement
    expression. See @c transformComprehension
    """

    node.type = utils.instantiate_unbound(self.ctx)
    if node.expected_type == ast.types.Stdlib.FixedDict:
        prepare_dict_literal(self, node, fixed=True)
        return transform_fixed_dict(self, node.items) or node
    result = transform_comprehension(
        self,
        ast.types.Stdlib.Dict,
        node.items,
        method="__setitem__",
        expected_type=node.expected_type,
    )
    if result:
        result.set(ast.Attr.ExprDict)
        return result
    return node


def prepare_dict_literal(self: TypeVisitor, expr: ast.Expr, fixed: bool = False) -> ast.Expr:
    """Preserve literal containers until their dictionary context is known.

    Visit other expressions now for source-order bindings (fixed_dict_nested_call_context).
    """
    if isinstance(expr, ast.TupleExpr):
        if fixed and isinstance(expr, ast.DictExpr):
            expr.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.FixedDict)
        for index, item in enumerate(expr.items):
            expr.items[index] = prepare_dict_literal(self, item, fixed)
        expr.type = utils.instantiate_unbound(self.ctx)
        return expr
    if isinstance(expr, (ast.StarExpr, ast.KeywordStarExpr)):
        expr.expr = prepare_dict_literal(self, expr.expr, fixed)
        return expr
    return self.visit_expr(expr)


def transform_fixed_dict(self: TypeVisitor, items: List[ast.Expr]) -> ast.Expr | None:
    names = []
    values = []
    statements = []
    positions = {}

    def append(name, value):
        if name not in positions:
            positions[name] = len(names)
            names.append(name)
            values.append(None)
        values[positions[name]] = value

    for item in items:
        if isinstance(item, ast.KeywordStarExpr):
            value = self.visit_expr(item.expr.clone())
            typ = value.cls
            if not typ:
                return None
            if typ != ast.types.Stdlib.FixedDict:
                raise TypecheckError(
                    item, "fixed dictionary literals can only expand fixed dictionaries"
                )
            if not typ[0].can_realize():
                return None
            fields = utils.get_record_fields(self.ctx, typ)
            temporary = utils.get_temporary_var(self.ctx, "fixed.spread")
            statements.append(ast.AssignStmt(ast.IdExpr(temporary), rhs=value))
            for field in fields:
                append(field.name, utils.get_record_field(ast.IdExpr(temporary), field))
            continue

        pair = item if isinstance(item, ast.TupleExpr) else item.orig
        if not isinstance(pair, ast.TupleExpr) or len(pair.items) != 2:
            raise TypecheckError(item, "fixed dictionary literals require string-keyed entries")
        key = self.visit_expr(pair.items[0].clone())
        if not key.type:
            return None
        literal = isinstance(key, ast.StringExpr) and key.is_simple
        constant = key.type.str
        if not literal and constant is None:
            raise TypecheckError(key, "fixed dictionary keys must be compile-time strings")
        name = constant if constant is not None else key.get_value()
        temporary = utils.get_temporary_var(self.ctx, "fixed.entry")
        statements.append(ast.AssignStmt(ast.IdExpr(temporary), rhs=item.clone()))
        append(name, ast.IndexExpr(ast.IdExpr(temporary), index=ast.IntExpr(1)))

    call = ast.CallExpr(
        ast.IdExpr(ast.types.Stdlib.FixedDict),
        [ast.TupleExpr(values), ast.IntExpr(classes.generate_kw_id(self, names))],
    )
    return self.visit_expr(ast.StmtExpr(statements, expr=call))


def typecheck_generator(self: TypeVisitor, node: ast.GeneratorExpr) -> ast.Node:
    """
    Transform a tuple generator expression.
    @example
    `tuple(expr for i in tuple_generator)` -> `Tuple.N.__new__(expr...)`
    """

    # List comprehension optimization:
    # Use `iter.__len__()` when creating list if there is a single for loop
    # without any if conditions in the comprehension
    optimize = node.kind is ast.GeneratorExpr.Kind.ListGenerator and node.loop_count() == 1
    final = node.final_suite()
    if optimize:
        # Turn off this optimization for static items
        match self.visit_expr(final.iter.clone()):
            case ast.CallExpr(expr=ast.IdExpr(value=name)) if name.startswith(
                "std.internal.static"
            ):
                optimize = False
    var = ast.IdExpr(utils.get_temporary_var(self.ctx, "gen"))
    expr = node.final_expr()
    assert expr

    def set_final_expr(node, expr):
        with ast.Node.creation_context(None):
            stmt = ast.ExprStmt(expr)
        node.iter_blocks(expr_fn=lambda _: stmt)

    if node.kind is ast.GeneratorExpr.Kind.ListGenerator:
        # List comprehensions
        set_final_expr(node, ast.CallExpr(ast.DotExpr(var.clone(), member="append"), items=[expr]))
        plain = ast.SuiteStmt(
            ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.List))),
            node.loops,
        )
        if optimize:
            opt_var = utils.get_temporary_var(self.ctx, "i")
            opt_for = node.loops.clone()
            opt_for.iter = ast.IdExpr(opt_var)
            opt = ast.SuiteStmt(
                ast.AssignStmt(ast.IdExpr(opt_var), rhs=final.iter.clone()),
                ast.AssignStmt(
                    var.clone(),
                    rhs=ast.CallExpr(
                        ast.IdExpr(ast.types.Stdlib.List),
                        items=[ast.CallExpr(ast.DotExpr(ast.IdExpr(opt_var), member="__len__"))],
                    ),
                ),
                opt_for,
            )
            result = ast.IfExpr(
                ast.CallExpr(
                    ast.IdExpr("hasattr"), items=[final.iter.clone(), ast.StringExpr("__len__")]
                ),
                ifexpr=ast.StmtExpr(opt, expr=var.clone()),
                elsexpr=ast.StmtExpr(plain, expr=var),
            )
        else:
            result = ast.StmtExpr(plain, expr=var)
        return self.visit_expr(result)

    if node.kind is ast.GeneratorExpr.Kind.SetGenerator:
        # Set comprehensions
        head = ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Set)))
        set_final_expr(node, ast.CallExpr(ast.DotExpr(var.clone(), member="add"), items=[expr]))
        return self.visit_expr(ast.StmtExpr(ast.SuiteStmt(head, node.loops), expr=var))
    elif node.kind is ast.GeneratorExpr.Kind.DictGenerator:
        # Dictionary comprehensions
        head = ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Dict)))
        set_final_expr(
            node,
            ast.CallExpr(
                ast.DotExpr(var.clone(), member="__setitem__"), items=[ast.StarExpr(expr=expr)]
            ),
        )
        return self.visit_expr(ast.StmtExpr(ast.SuiteStmt(head, node.loops), expr=var))
    elif node.kind is ast.GeneratorExpr.Kind.TupleGenerator:
        assert node.loop_count() == 1, "invalid tuple generator"
        generator_node = self.visit_expr(final.iter)
        assert generator_node and generator_node.type
        if not generator_node.type.can_realize():
            return node  # Wait until the iterator can be realized

        block = ast.SuiteStmt()
        # `tuple = tuple_generator`
        tuple_name = utils.get_temporary_var(self.ctx, "tuple")
        block.add(ast.AssignStmt(ast.IdExpr(tuple_name), rhs=generator_node))
        for_stmt = final.clone()
        ok, delay, preamble, static_items = loops.transform_static_loop_call(
            self,
            for_stmt.var,
            generator_node,
            lambda wrap: ast.StmtExpr(wrap.clone(), expr=expr.clone()),
            allow_non_heterogenous=True,
        )
        if not ok:
            raise TypecheckError(
                generator_node, f"iterable must be a tuple, not '{generator_node.type}'"
            )
        if delay:
            return node
        if preamble:
            block.add(preamble)
        return self.visit_expr(
            ast.StmtExpr(block, expr=ast.TupleExpr([cast(ast.Expr, item) for item in static_items]))
        )
    else:
        new_loops = self.visit_stmt(node.loops)  # assume: internal data will be changed
        assert isinstance(new_loops, ast.ForStmt)
        node.loops = new_loops
        expr = node.final_expr()
        if not expr:
            # Case such as (0 for _ in static.range(2))
            # TODO: make this better.
            raise TypecheckError(
                node,
                "generator cannot be compiled. If using static tuple generator, use "
                "tuple(...) instead.",
            )
        assert node.type
        node.type |= utils.instantiate(
            self.ctx,
            utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Generator),
            [expr.type],
        )
        if infer.realize(self.ctx, node.type):
            node.done = True
        return node


def transform_comprehension(
    self: TypeVisitor,
    type_name: str,
    items: List[ast.Expr],
    method: str,
    expected_type: ast.types.Type | None = None,
) -> ast.Expr | None:
    """
    Transform a collection of type `type` to a statement expression:
    `[a1, ..., aN]` -> `cont = [type](); (cont.[fn](a1); ...); cont`
    Any star-expression within the collection will be expanded:
    `[a, *b]` -> `cont.[fn](a); for i in b: cont.[fn](i)`.
    @example
    `[a, *b, c]`  -> ```cont = List(3)
    cont.append(a)
    for i in b: cont.append(i)
    cont.append(c)```
    `{a, *b, c}`  -> ```cont = Set()
    cont.add(a)
    for i in b: cont.add(i)
    cont.add(c)```
    `{a: 1, **d}` -> ```cont = Dict()
    cont.__setitem__((a, 1))
    for i in b.items(): cont.__setitem__((i[0], i[i]))```
    Union parameters in expectedType fix storage; other parameters use common-type
    inference. Dictionary keys and values are inferred independently.
    """

    def lowest_common_type(
        typ: ast.types.Class | None, item_type: ast.types.Class
    ) -> ast.types.Type | None:
        """
        Deduce the lowest common type of the collection--- in other words, the lowest
        common ancestor of all types in the collection. For example, `type([1, 1.2]) ==
        type([1.2, 1]) == float` because float is an "ancestor" of int.
        TODO: use wrapExpr...
        """
        if not typ:
            return item_type
        elif (typ == ast.types.Stdlib.NoneType) != (item_type == ast.types.Stdlib.NoneType):
            # NoneType introduces nullability without becoming the element type.
            member = item_type if typ == ast.types.Stdlib.NoneType else typ
            return (
                member
                if member == ast.types.Stdlib.Optional
                else utils.instantiate(self.ctx, ast.types.Stdlib.Optional, [member])
            )
        # Rule: int derives from float
        elif typ == "Int" and item_type == "float":
            return item_type
        # Rule: T derives from Optional[T]
        elif typ.name != ast.types.Stdlib.Optional and item_type.name == ast.types.Stdlib.Optional:
            return utils.instantiate(
                self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional), [typ]
            )
        elif typ.name == ast.types.Stdlib.Optional and item_type.name != ast.types.Stdlib.Optional:
            return utils.instantiate(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional),
                [item_type],
            )
        # Rule: anything derives from pyobj
        elif typ != "pyobj" and item_type == "pyobj":
            return item_type
        elif typ.name != item_type.name:
            cls_data = utils.get_class(self.ctx, typ)
            item_cls_data = utils.get_class(self.ctx, item_type)
            if cls_data and item_cls_data:
                # Rule: subclass derives from its base class
                for collection_mro in cls_data.mro:
                    common = utils.instantiate(self.ctx, collection_mro, typ)
                    for item_mro in item_cls_data.mro:
                        candidate = utils.instantiate(self.ctx, item_mro, item_type)
                        if common.unify(candidate) >= 0:
                            return common
        return None

    is_dict = type_name == ast.types.Stdlib.Dict
    root = utils.get_stdlib_type(self.ctx, type_name)
    if is_dict:
        # Dict is an alias: compare the resolved class, e.g. OrderedDict.
        root = utils.get_stdlib_type(self.ctx, root.name)
    element_types, declared_unions = [], []
    for idx in range(2 if is_dict else 1):
        member = (
            expected_type.require_cls[idx] if expected_type and expected_type == root.name else None
        )
        declared_unions.append(bool(member and member.union))
        element_types.append(member if declared_unions[-1] else utils.instantiate_unbound(self.ctx))
    done = True
    for idx, item in enumerate(items):
        item_type = None
        if not is_dict and isinstance(item, ast.StarExpr):
            item.expr = self.visit_expr(ast.CallExpr(ast.DotExpr(item.expr, member="__iter__")))
            if item.expr and item.expr.type and item.expr.type == "Generator":
                item_type = item.expr.type.require_cls[0]
        elif is_dict and isinstance(item, ast.KeywordStarExpr):
            item.expr = self.visit_expr(ast.CallExpr(ast.DotExpr(item.expr, member="items")))
            # Empty tuples contribute no key/value constraints.
            if item.expr.cls == ast.types.Stdlib.Tuple and not len(item.expr.cls):
                continue
            item.expr = self.visit_expr(ast.CallExpr(ast.DotExpr(item.expr, member="__iter__")))
            if item.expr and item.expr.type and item.expr.type == "Generator":
                item_type = item.expr.type.require_cls[0]
        else:
            items[idx] = item = self.visit_expr(item)
            item_type = item.cls
        if not item_type:
            done = False
            continue

        item_type = item_type.require_cls
        if is_dict:
            item_type |= utils.instantiate(self.ctx, classes.generate_tuple(self.ctx, 2))
        for dict_idx in range(len(element_types)):
            # A declared union fixes storage; only inferred slots participate in lctyp.
            # Keep dict keys/values independent (union_collection_common_type).
            if declared_unions[dict_idx]:
                continue
            member = item_type[dict_idx] if is_dict else item_type
            if not element_types[dict_idx].cls:
                element_types[dict_idx] |= member
            elif common := lowest_common_type(
                element_types[dict_idx].require_cls, member.require_cls
            ):
                element_types[dict_idx] = common
    if not done:
        return None

    collection_template = utils.instantiate(self.ctx, root, element_types)
    stmts = []
    var = ast.IdExpr(utils.get_temporary_var(self.ctx, "cont"))
    ctr_args = []
    # Optimization: pre-allocate the list with the exact number of elements
    if type_name == ast.types.Stdlib.List and items:
        ctr_args.append(ast.CallExpr.Arg(ast.IntExpr(len(items))))
    ctr = ast.IdExpr(root.name if is_dict else type_name)
    ctr.type = utils.instantiate_type_var(self.ctx, collection_template)
    stmts.append(ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ctr, items=ctr_args)))
    for item in items:
        # Unpack star-expression by iterating over it
        # `*star` -> `for i in star: cont.[fn](i)`
        if not is_dict and isinstance(item, ast.StarExpr):
            # `*star` -> `for i in star: cont.[fn](i)`
            loop_var = ast.IdExpr(utils.get_temporary_var(self.ctx, "i"))
            item.expr.set(ast.Attr.ExprStarSequenceItem)
            stmts.append(
                ast.ForStmt(
                    loop_var.clone(),
                    iter=item.expr,
                    suite=ast.ExprStmt(
                        ast.CallExpr(
                            ast.DotExpr(var.clone(), member=method), items=[loop_var.clone()]
                        )
                    ),
                )
            )
        elif is_dict and isinstance(item, ast.KeywordStarExpr):
            # Expand kwstar-expression by iterating over it: see the example above
            if item.expr.cls == ast.types.Stdlib.Tuple and not len(item.expr.cls):
                stmts.append(ast.ExprStmt(item.expr))
                continue
            loop_var = ast.IdExpr(utils.get_temporary_var(self.ctx, "it"))
            item.expr.set(ast.Attr.ExprStarSequenceItem)
            args = [
                ast.IndexExpr(loop_var.clone(), index=ast.IntExpr(0)),
                ast.IndexExpr(loop_var.clone(), index=ast.IntExpr(1)),
            ]
            stmts.append(
                ast.ForStmt(
                    loop_var.clone(),
                    iter=item.expr,
                    suite=ast.ExprStmt(
                        ast.CallExpr(ast.DotExpr(var.clone(), member=method), items=args)
                    ),
                )
            )
        else:
            item.set(ast.Attr.ExprSequenceItem)
            args = []
            if is_dict:
                head = item
                if utils.has_side_effect(head):
                    temporary = utils.get_temporary_var(self.ctx, "star")
                    lead = ast.AssignExpr(ast.IdExpr(temporary), expr=head)
                    head = ast.IdExpr(temporary)
                else:
                    lead = head.clone()
                lead.set(ast.Attr.ExprSequenceItem)
                head.set(ast.Attr.ExprSequenceItem)
                args = [
                    ast.IndexExpr(lead, index=ast.IntExpr(0)),
                    ast.IndexExpr(head, index=ast.IntExpr(1)),
                ]
            else:
                args = [item]
            stmts.append(
                ast.ExprStmt(ast.CallExpr(ast.DotExpr(var.clone(), member=method), items=args))
            )
    return self.visit_expr(ast.StmtExpr(stmts, expr=var))
