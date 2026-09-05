# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast
from . import classes, infer, loops, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_tuple(self: TypeVisitor, node: ast.TupleExpr) -> ast.Node:
    """
    Transform tuples.
    @example
    `(a1, ..., aN)` -> `Tuple.__new__(a1, ..., aN)`
    """

    result = self.visit(
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
    result = transform_comprehension(self, ast.types.Stdlib.List, node.items, method="append")
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
    result = transform_comprehension(self, ast.types.Stdlib.Set, node.items, method="add")
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
    result = transform_comprehension(self, ast.types.Stdlib.Dict, node.items, method="__setitem__")
    if result:
        result.set(ast.Attr.ExprDict)
        return result
    return node


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
        match self.visit(final.iter.clone()):
            case ast.CallExpr(ast.IdExpr(value=name)) if not name.startswith("std.internal.static"):
                optimize = False
    var = ast.IdExpr(utils.get_temporary_var(self.ctx, "gen"))
    expr = node.final_expr()

    if node.kind is ast.GeneratorExpr.Kind.ListGenerator:
        # List comprehensions
        node.set_final_expr(ast.CallExpr(ast.DotExpr(var.clone(), member="append"), items=[expr]))
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
                ifexpr=ast.StmtExpr(opt.items, expr=var.clone()),
                elsexpr=ast.StmtExpr(plain.items, expr=var),
            )
        else:
            result = ast.StmtExpr(plain.items, expr=var)
        return self.visit(result)

    if node.kind is ast.GeneratorExpr.Kind.SetGenerator:
        # Set comprehensions
        head = ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Set)))
        node.set_final_expr(ast.CallExpr(ast.DotExpr(var.clone(), member="add"), items=[expr]))
        return self.visit(ast.StmtExpr([head, node.loops], expr=var))
    elif node.kind is ast.GeneratorExpr.Kind.DictGenerator:
        # Dictionary comprehensions
        head = ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Dict)))
        node.set_final_expr(
            ast.CallExpr(ast.DotExpr(var.clone(), member="__setitem__"), items=[ast.StarExpr(expr)])
        )
        return self.visit(ast.StmtExpr([head, node.loops], expr=var))
    elif node.kind is ast.GeneratorExpr.Kind.TupleGenerator:
        assert node.loop_count() == 1, "invalid tuple generator"
        generator_node = self.visit(final.iter)
        if not generator_node.type.can_realize():
            return node  # Wait until the iterator can be realized

        # `tuple = tuple_generator`
        tuple_name = utils.get_temporary_var(self.ctx, "tuple")
        block = ast.SuiteStmt(ast.AssignStmt(ast.IdExpr(tuple_name), rhs=generator_node))
        static_items = loops.populate_static_loop(
            self,
            final.var,
            final.suite,
            generator_node,
            expr,
        )

        return self.visit(ast.StmtExpr(block.items, expr=ast.TupleExpr(static_items)))
    else:
        node.loops = self.visit(node.loops)  # assume: internal data will be changed
        expr = node.final_expr()
        if not expr:
            # Case such as (0 for _ in static.range(2))
            # TODO: make this better.
            raise TypecheckError(
                node,
                "generator cannot be compiled. If using static tuple generator, use "
                "tuple(...) instead.",
            )
        infer.unify(
            node.type,
            utils.instantiate_type(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Generator),
                [expr.type],
            ),
        )
        if infer.realize(node.type):
            node.done = True
        return node


def transform_comprehension(
    self: TypeVisitor,
    type_name: str,
    items: List[ast.Expr],
    method: str,
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
    """

    def lowest_common_type(
        typ: ast.types.Class | None, item_type: ast.types.Class
    ) -> ast.types.Type | None:
        if not typ:
            return item_type
        elif typ.is_type("int") and item_type.is_type("float"):
            return item_type
        elif typ.name != ast.types.Stdlib.Optional and item_type.name == ast.types.Stdlib.Optional:
            return utils.instantiate_type(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional),
                [typ],
            )
        elif typ.name == ast.types.Stdlib.Optional and item_type.name != ast.types.Stdlib.Optional:
            return utils.instantiate_type(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional),
                [item_type],
            )
        elif not typ.is_type("pyobj") and item_type.is_type("pyobj"):
            return item_type
        elif typ.name != item_type.name:
            cls_data = utils.get_class(self.ctx, typ)
            item_cls_data = utils.get_class(self.ctx, item_type)
            if cls_data and item_cls_data:
                for collection_mro in cls_data.mro:
                    typ = utils.instantiate_type(
                        self.ctx, collection_mro, [g.type for g in typ.generics]
                    )
                    for item_mro in item_cls_data.mro:
                        candidate = utils.instantiate_type(
                            self.ctx, item_mro, [g.type for g in item_type.generics]
                        )
                        if typ.unify(candidate) >= 0:
                            return typ
        return None

    collection_type = utils.instantiate_unbound(self.ctx)
    done = True
    is_dict = type_name == ast.types.Stdlib.Dict
    for idx, item in enumerate(items):
        # Deduce the lowest common type of the collection--- in other words, the lowest
        # common ancestor of all types in the collection. For example, `type([1, 1.2]) ==
        # type([1.2, 1]) == float` because float is an "ancestor" of int.
        # TODO: use wrapExpr...

        item_type = None
        if not is_dict and isinstance(item, ast.StarExpr):
            item.expr = self.visit(ast.CallExpr(ast.DotExpr(item.expr, member="__iter__")))
            if item.expr and item.expr.type and item.expr.type.is_type("Generator"):
                item_type = item.expr.type[0]
        elif is_dict and isinstance(item, ast.KeywordStarExpr):
            item.expr = self.visit(ast.CallExpr(ast.DotExpr(item.expr, member="items")))
            if item.expr and item.expr.type and item.expr.type.is_type("Generator"):
                item_type = item.expr.type[0]
        else:
            items[idx] = item = self.visit(item)
            item_type = item.get_class_type()
        if not item_type:
            done = False
            continue

        if not collection_type:
            infer.unify(collection_type, item_type)
        elif not is_dict:
            if common := lowest_common_type(collection_type, item_type):
                collection_type = common
        else:
            tuple_type = infer.unify(
                item_type,
                utils.instantiate_type(self.ctx, classes.generate_tuple(self, 2)),
            )
            assert collection_type.is_record() and len(collection_type.generics) == 2
            assert len(tuple_type.generics) == 2

            new_types = []
            for dict_index in range(2):
                new_type = collection_type[dict_index]
                if not new_type:
                    infer.unify(new_type, tuple_type[dict_index])
                elif common := lowest_common_type(new_type, tuple_type[dict_index]):
                    new_type = common
                new_types.append(new_type)
            collection_type = utils.instantiate_type(
                self.ctx, classes.generate_tuple(self, len(new_types)), new_types
            )
    if not done:
        return None

    stmts = []
    var = ast.IdExpr(utils.get_temporary_var(self.ctx, "cont"))
    ctr_args: List[ast.CallExpr.Arg] = []
    if type_name == ast.types.Stdlib.List and items:
        ctr_args.append(ast.IntExpr(len(items)))
    ctr = ast.IdExpr(type_name)
    collection_template = utils.instantiate_type(
        self.ctx, utils.get_stdlib_type(self.ctx, type_name)
    )
    if is_dict and collection_type:
        assert collection_type.is_record()
        collection_template = utils.instantiate_type(
            self.ctx,
            utils.get_stdlib_type(self.ctx, type_name),
            [generic.type for generic in collection_type.generics],
        )
    elif not is_dict:
        collection_template = utils.instantiate_type(
            self.ctx,
            utils.get_stdlib_type(self.ctx, type_name),
            [collection_type],
        )
    ctr.type = utils.instantiate_type_var(self.ctx, collection_template)
    stmts.append(ast.AssignStmt(var.clone(), rhs=ast.CallExpr(ctr, items=ctr_args)))
    for item in items:
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
            # Same for **kwstars
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
    return self.visit(ast.StmtExpr(stmts, expr=var))
