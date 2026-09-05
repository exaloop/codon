# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, Tuple, cast
from ... import ast
from . import classes, infer, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_unary(self: TypeVisitor, node: ast.UnaryExpr) -> ast.Expr:
    """
    Replace unary operators with the appropriate magic calls.
    Also evaluate static expressions. See @c evaluateStaticUnary for details.
    """

    node.expr = self.visit(node.expr)
    if isinstance(node.expr, ast.IntExpr) and node.op == "-":
        # Special case: make - INT(val) same as INT(-val) to simplify IR and everything
        return self.visit(ast.IntExpr(-node.expr.get_value()))

    static_type = None
    static_ops = {
        ast.types.Type.Behaviour.Int: {"-", "+", "!", "~"},
        ast.types.Type.Behaviour.String: {"!"},
        ast.types.Type.Behaviour.Bool: {"!"},
    }
    # Handle static expressions
    op_kind = node.expr.type.get_static_kind()
    if op_kind is not ast.types.Type.Behaviour.Runtime:
        if node.op in static_ops.get(op_kind, set()):
            if expr := evaluate_static_unary(self, node):
                static_type = expr.type.get_static()
            else:
                return node
    elif utils.is_unbound(node.expr.type):
        return node

    if node.op == "!":
        # `not expr` -> `expr.__bool__().__invert__()`
        result = ast.CallExpr(
            ast.DotExpr(
                ast.CallExpr(ast.DotExpr(node.expr, member="__bool__")), member="__invert__"
            )
        )
    else:
        magics = {"~": "invert", "+": "pos", "-": "neg"}
        assert not (node.op not in magics), f"invalid unary operator '{node.op}'"
        result = ast.CallExpr(ast.DotExpr(node.expr, member=f"__{magics[node.op]}__"))

    result = self.visit(result)
    if static_type:
        result.type = static_type
    return result


def typecheck_binary(self: TypeVisitor, node: ast.BinaryExpr) -> ast.Expr:
    """
    Replace binary operators with the appropriate magic calls.
    See @c transformBinarySimple , @c transformBinaryIs , @c transformBinaryMagic and
    @c transformBinaryInplaceMagic for details.
    Also evaluate static expressions. See @c evaluateStaticBinary for details.
    """

    expects_bool = node.expected_type and node.expected_type.is_type(ast.types.Stdlib.Bool)
    if expects_bool and node.op in {"&&", "||"}:
        node.lexpr.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
        node.rexpr.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)

    node.lexpr = self.visit(node.lexpr)

    # Static short-circuit
    left_kind = node.lexpr.type.get_static_kind()
    if left_kind is not ast.types.Type.Behaviour.Runtime and node.op in {"&&", "||"}:
        truth = False
        if b := node.lexpr.type.get_bool_static():
            truth = b.value
        elif s := node.lexpr.type.get_str_static():
            truth = bool(s.value)
        elif i := node.lexpr.type.get_int_static():
            truth = bool(i.value)
        else:
            node.type.get_unbound().static_kind = ast.types.Type.Behaviour.Bool
            return node

        ignore_right = (node.op == "&&" and not truth) or (node.op == "||" and truth)
        if ignore_right:
            short_result = ast.BoolExpr(truth) if expects_bool else node.lexpr
        else:
            short_result = ast.StmtExpr([ast.ExprStmt(node.lexpr)], expr=node.rexpr)
        return self.visit(short_result)

    node.rexpr = self.visit(node.rexpr)

    static_type = None
    # fmt: off
    static_ops = {
        ast.types.Type.Behaviour.Int: {
            "<", "<=", ">", ">=", "==", "!=", "&&", "||",
            "+", "-", "*", "//", "%", "&", "|", "^", ">>", "<<",
        },
        ast.types.Type.Behaviour.String: {"==", "!=", "+"},
        ast.types.Type.Behaviour.Bool: {"<", "<=", ">", ">=", "==", "!=", "&&", "||"},
    }
    # fmt: on
    left_kind = node.lexpr.type.get_static_kind()
    right_kind = node.rexpr.type.get_static_kind()
    if (
        left_kind is not ast.types.Type.Behaviour.Runtime
        and right_kind is not ast.types.Type.Behaviour.Runtime
    ):
        is_static = left_kind is right_kind and node.op in static_ops.get(left_kind, set())
        if (
            not is_static
            and {left_kind, right_kind}
            == {ast.types.Type.Behaviour.Int, ast.types.Type.Behaviour.Bool}
            and node.op in static_ops[ast.types.Type.Behaviour.Int]
        ):
            is_static = True
        if is_static:
            if expr := evaluate_static_binary(self, node):
                static_type = expr.type.get_static()
            else:
                return node

    result = None
    if (
        node.op == "|"
        and (utils.is_type_expr(node.lexpr) or isinstance(node.lexpr, ast.NoneExpr))
        and (utils.is_type_expr(node.rexpr) or isinstance(node.rexpr, ast.NoneExpr))
    ):
        # Case: unions
        union_expr = ast.InstantiateExpr(
            ast.IdExpr(ast.types.Stdlib.Union), items=[node.lexpr, node.rexpr]
        )
        transformed_union = self.visit(union_expr)
        result = transformed_union
    elif result := transform_binary_simple(self, node):
        # Case: simple binary expressions
        pass
    elif node.lexpr.type.get_unbound() or (node.op != "is" and node.rexpr.type.get_unbound()):
        # Case: types are unknown, so continue later
        return node
    elif node.op == "is":
        # Case: is operator
        result = transform_binary_is(self, node)
    elif result := transform_binary_inplace_magic(self, node, False):
        # Case: in-place magic methods
        pass
    elif result := transform_binary_magic(self, node):
        # Case: normal magic methods
        pass
    elif node.lexpr.type.is_type(ast.types.Stdlib.Optional):
        result = self.visit(
            ast.BinaryExpr(
                ast.CallExpr(ast.IdExpr(ast.types.Stdlib.OptionalUnwrap), items=[node.lexpr]),
                op=node.op,
                rexpr=node.rexpr,
                in_place=node.in_place,
            )
        )
    else:
        raise TypecheckError(
            node,
            f"unsupported operand type(s) for {node.op}: "
            f"'{node.lexpr.type.pretty_string()}' and "
            f"'{node.rexpr.type.pretty_string()}'",
        )
    if static_type:
        result.type = static_type
    return result


def typecheck_chainbinary(self: TypeVisitor, node: ast.ChainBinaryExpr) -> ast.Node:
    """
    Transform chain binary expression.
    @example
    `a <= b <= c` -> `(a <= (chain := b)) and (chain <= c)`
    The assignment above ensures that all expressions are executed only once.
    """

    assert len(node.exprs) >= 2
    is_bool = isinstance(node.expected_type, ast.types.Type) and node.expected_type.is_type(
        ast.types.Stdlib.Bool
    )
    items = []
    prev = ""
    for index in range(1, len(node.exprs)):
        if prev:
            left = ast.IdExpr(prev)
        else:
            left = node.exprs[index - 1][1].clone()
        prev = self.ctx.generate_canonical_name("chain")
        if index + 1 == len(node.exprs):
            right = node.exprs[index][1].clone()
        else:
            assignment = ast.AssignStmt(ast.IdExpr(prev), rhs=node.exprs[index][1].clone())
            right = ast.StmtExpr([assignment], expr=ast.IdExpr(prev))
        items.append(
            ast.BinaryExpr(
                left,
                op=node.exprs[index][0],
                rexpr=right,
                expected_type=utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
                if is_bool
                else None,
            )
        )
    final = items[-1]
    for index in range(len(items) - 2, -1, -1):
        final = ast.BinaryExpr(
            items[index],
            op="&&",
            rexpr=final,
            expected_type=utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool)
            if is_bool
            else None,
        )
    return self.visit(final)


def find_ellipsis(self: TypeVisitor, expr: ast.Expr):
    """
    Helper function that locates the pipe ellipsis within a collection of (possibly
    nested) CallExprs.
    @return  List of CallExprs and their locations within the parent CallExpr
    needed to access the ellipsis.
    @example
    `foo(bar(1, baz(...)))` returns `[{0, baz}, {1, bar}, {0, foo}]`
    """

    if not isinstance(expr, ast.CallExpr):
        return
    for arg_idx, arg in enumerate(expr.items):
        arg_expr = arg.value
        if isinstance(arg_expr, ast.EllipsisExpr):
            if arg_expr.is_pipe():
                yield (arg_idx, expr)
        elif isinstance(arg_expr, ast.CallExpr):
            yield from find_ellipsis(self, arg_expr)


def typecheck_pipe(self: TypeVisitor, node: ast.PipeExpr) -> ast.Node:
    """
    Typecheck pipe expressions.
    Each stage call `foo(x)` without an ellipsis will be transformed to `foo(..., x)`.
    Stages that are not in the form of CallExpr will be transformed to it (e.g., `foo`
    -> `foo(...)`).
    Special care is taken of stages that can expand to multiple stages (e.g., `a |> foo`
    might become `a |> unwrap |> foo` to satisfy type constraints).
    """

    has_generator = False

    def iterable_type(typ: ast.types.Type) -> ast.types.Type:
        """Return T if t is of type `Generator[T]`; otherwise just `type(t)`"""
        nonlocal has_generator
        if typ.is_type("Generator"):
            has_generator = True
            return typ[0]
        return typ

    # List of output types
    # (e.g., for `a|>b|>c` it is `[type(a), type(a|>b), type(a|>b|>c)]`).
    # Note: the generator types are completely preserved (i.e., not extracted)
    node.in_types.clear()

    # Process the pipeline head
    node.items[0].expr = head = self.visit(node.items[0].expr)
    input_type = head.type  # input type to the next stage
    node.in_types.append(input_type)
    input_type = iterable_type(input_type)
    done = head.done
    pipe_idx = 1
    while pipe_idx < len(node.items):
        pipe_expr = node.items[pipe_idx].expr
        enclosing_stmtexpr = []  # enclosing list so that we can replace pipe_expr
        core = pipe_expr
        while isinstance(core, ast.StmtExpr):
            # handle StmtExpr (e.g., in partial calls)
            enclosing_stmtexpr.append(core)
            core = core.expr

        if isinstance(core, ast.CallExpr):
            # Case: a call. Find the position of the pipe ellipsis within it
            ellipsis_pos = -1
            for arg_idx, arg in enumerate(core.items):
                if isinstance(arg.value, ast.EllipsisExpr):
                    ellipsis_pos = arg_idx
                    break
            # No ellipses found? Prepend it as the first argument
            if ellipsis_pos == -1:
                core.items.insert(0, ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial))
                ellipsis_pos = 0
        else:
            # Case: not a call. Convert it to a call with a single ellipsis
            core = ast.CallExpr(core, items=[ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial)])
            if enclosing_stmtexpr:
                enclosing_stmtexpr[-1].expr = core
            else:
                node.items[pipe_idx].expr = core
            ellipsis_pos = 0
        # Set the ellipsis type
        ellipsis = core.items[ellipsis_pos].value
        ellipsis.mode = ast.EllipsisExpr.Kind.Pipe
        # Don't unify unbound inType yet (it might become a generator that needs to be extracted)
        if not ellipsis.type:
            ellipsis.type = utils.instantiate_unbound(self.ctx)
        if not input_type.get_unbound():
            infer.unify(ellipsis.type, input_type)

        # Transform the call. Because a transformation might wrap the ellipsis in layers,
        # make sure to extract these layers and move them to the pipeline.
        # Example: `foo(...)` that is transformed to `foo(unwrap(...))` will become
        # `unwrap(...) |> foo(...)`
        core = self.visit(core)
        # input type to the next stage
        if enclosing_stmtexpr:
            enclosing_stmtexpr[-1].expr = core
        layers = list(find_ellipsis(self, core))
        assert layers, "can't find the ellipsis"
        if len(layers) > 1:
            # Prepend layers
            insert_idx = pipe_idx
            for pos, prepend in layers:
                prepend.items[pos].value = ast.EllipsisExpr(ast.EllipsisExpr.Kind.Pipe)
                node.items.insert(insert_idx, ast.PipeExpr.Pipe("|>", expr=prepend))
                insert_idx += 1
                pipe_idx += 1
            # Rewind the loop (yes, the current expression will get transformed again)
            # TODO: avoid reevaluation
            del node.items[pipe_idx]
            pipe_idx = pipe_idx - len(layers) - 1
            pipe_idx += 1
            continue

        if core.type:
            infer.unify(node.items[pipe_idx].expr.type, core.type)
        node.items[pipe_idx].expr = core
        input_type = core.type
        if infer.realize(input_type) is None:
            done = False
        node.in_types.append(input_type)

        # Do not extract the generator in the last stage of a pipeline
        if pipe_idx + 1 < len(node.items):
            input_type = iterable_type(input_type)
        pipe_idx += 1
    infer.unify(
        node.type,
        utils.get_stdlib_type(self.ctx, ast.types.Stdlib.NoneType) if has_generator else input_type,
    )
    if done:
        node.done = True
    return node


def typecheck_index(self: TypeVisitor, node: ast.IndexExpr) -> ast.Node:
    """
    Transform index expressions.
    @example
    `foo[T]`   -> Instantiate(foo, [T]) if `foo` is a type
    `tup[1]`   -> `tup.item1` if `tup` is tuple
    `foo[idx]` -> `foo.__getitem__(idx)`
    expr.itemN or a sub-tuple if index is static (see transformStaticTupleIndex()),
    """

    match node:
        case ast.IndexExpr(
            expr=ast.IdExpr(value="Literal" | "Static"),
            index=ast.IdExpr(value="int" | "str" | "bool"),
        ):
            # Special case: static types.
            infer.unify(
                node.type,
                utils.instantiate_unbound(self.ctx, static_kind=ast.get_static_generic(node)),
            )
            node.done = True
            return node
        case ast.IndexExpr(expr=ast.IdExpr(value="Literal" | "Static")):
            raise TypecheckError(node, "expected 'int', 'bool' or 'str'")
        case ast.IndexExpr(expr=ast.IdExpr("tuple")):
            node.expr.value = ast.types.Stdlib.Tuple

    node.expr = self.visit(node.expr)

    # IndexExpr[i1, ..., iN] is internally represented as
    # IndexExpr[TupleExpr[i1, ..., iN]] for N > 1
    items = list(node.index.items) if isinstance(node.index, ast.TupleExpr) else [node.index]
    is_tuple = isinstance(node.index, ast.TupleExpr)
    for idx, item in enumerate(items):
        if utils.is_type_expr(node.expr) and isinstance(item, ast.ListExpr):
            item = ast.InstantiateExpr(ast.IdExpr(ast.types.Stdlib.Tuple), items=list(item.items))
        items[idx] = self.visit(item)
    orig_index = node.index.clone()
    if utils.is_type_expr(node.expr):
        # Special case: `A[[A, B], C]` -> `A[Tuple[A, B], C]` (e.g., in `Function[...]`)
        return self.visit(ast.InstantiateExpr(node.expr, items=items))

    node.index = items[0] if not is_tuple and len(items) == 1 else ast.TupleExpr(items)
    expr_type = node.expr.get_class_type()
    if expr_type is None:
        return node  # Wait until the type becomes known

    # Case: static tuple access
    # Note: needs untransformed origIndex to parse statics nicely
    is_static_tuple, tuple_expression = transform_static_tuple_index(
        self, expr_type, node.expr, orig_index
    )
    if is_static_tuple:
        return tuple_expression or node

    # Case: normal __getitem__
    getitem_call = ast.CallExpr(ast.DotExpr(node.expr, member="__getitem__"), items=[node.index])
    return self.visit(getitem_call)


def typecheck_instantiate(self: TypeVisitor, node: ast.InstantiateExpr) -> ast.Node:
    """
    Transform an instantiation to canonical realized name.
    @example
    Instantiate(foo, [bar]) -> Id("foo[bar]")
    """

    node.expr = self.visit(node.expr, enforce_type=True)
    types_count = len(node.items)
    root_type = utils.extract_type(self.ctx, node.expr)
    if root_type.is_type(ast.types.Stdlib.Tuple):
        if node.items:
            first = node.items[0] = self.visit(node.items[0])
            if first.type and first.type.get_static_kind() is ast.types.Type.Behaviour.Int:
                tail = ast.InstantiateExpr(
                    ast.IdExpr(ast.types.Stdlib.Tuple), items=list(node.items[1:])
                )
                ntuple = ast.InstantiateExpr(ast.IdExpr("__NTuple__"), items=[first, tail])
                return self.visit(ntuple)
        typ = utils.instantiate_type(self.ctx, classes.generate_tuple(self, types_count))
    else:
        typ = utils.instantiate_type(self.ctx, root_type, info=node.expr.info)

    assert typ.get_class(), f"unknown type: {node.expr}"
    if not typ.get_union() and types_count != len(typ.generics):
        raise TypecheckError(
            node,
            f"{utils.get_user_facing_name(self.ctx, typ.name)} takes "
            f"{len(typ.generics)} generics ({types_count} given)",
        )

    result = None
    match node.expr, bool(typ.get_union()):
        case ast.IdExpr(value=ast.types.Stdlib.CallableTrait), _:
            # Case: CallableTrait[...] trait instantiation
            # CallableTrait error checking.
            trait_types = []
            for idx, param in enumerate(node.items):
                node.items[idx] = self.visit(param, enforce_type=True)
                param_type = utils.extract_type(self.ctx, nodes.items[idx])
                if param_type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
                    raise TypecheckError(node, "CallableTrait cannot take static types")
                trait_types.append(param_type)
            unbound = utils.instantiate_unbound(
                self.ctx, trait=ast.types.CallableTrait(cache=self.ctx.cache, args=trait_types)
            )
            infer.unify(node.type, utils.instantiate_type_var(self.ctx, unbound))
        case ast.IdExpr(value=ast.types.Stdlib.TypeTrait), _:
            assert node.items
            # Case: TypeTrait[...] trait instantiation
            node.items[0] = self.visit(node.items[0], enforce_type=True)
            unbound = utils.instantiate_unbound(
                self.ctx,
                trait=ast.types.TypeTrait(
                    cache=self.ctx.cache, type=utils.extract_type(self.ctx, node.items[0])
                ),
            )
            infer.unify(node.type, unbound)
        case _, True:  # union
            optional = False
            union_types = []
            solo_type_expression = None
            for idx, param in enumerate(node.items):
                node.items[idx] = self.visit(param, enforce_type=True)
                param_type = utils.extract_type(self.ctx, node.items[idx])
                optional = optional or param_type.is_type(ast.types.Stdlib.Optional)
                optional = optional or param_type.is_type(ast.types.Stdlib.NoneType)
                if param_type.get_union():
                    for generic in param_type[0].get_class().generics:
                        union_types.append(generic.type)
                        optional = optional or generic.type.is_type(ast.types.Stdlib.Optional)
                elif not param_type.is_type(ast.types.Stdlib.NoneType):
                    union_types.append(param_type)
                    solo_type_expression = node.items[idx]
            if optional:
                # A | B | ... | None -> Optional[A] | Optional[B] | ...
                for idx, union_type in enumerate(union_types):
                    if not union_type.is_type(ast.types.Stdlib.Optional):
                        union_types[idx] = utils.instantiate_type(
                            self.ctx,
                            utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional),
                            [union_type],
                        )
                if solo_type_expression:
                    solo_type_expression = self.visit(
                        ast.InstantiateExpr(
                            ast.IdExpr(ast.types.Stdlib.Optional), items=[solo_type_expression]
                        )
                    )
            union_types = {}
            for union_type in union_types:
                union_types.setdefault(union_type.realized_name(), union_type)
            if not union_types:
                # All nones: None | None ...
                result = self.visit(ast.IdExpr(ast.types.Stdlib.NoneType))
                infer.unify(node.type, none_identifier.type)
            elif len(union_types) == 1:
                # Union[T] = T. Note that we do not check for the same types here...
                assert solo_type_expression is not None, f"type not detected: {node.to_string()}"
                result = solo_type_expression
                infer.unify(node.type, result.type)
            else:
                tuple_type = utils.instantiate_type(
                    self.ctx,
                    classes.generate_tuple(self, len(union_types)),
                    [v for _, v in sorted(union_types.items())],
                )
                infer.unify(typ.get_union()[0], tuple_type)
                infer.unify(node.type, utils.instantiate_type_var(self.ctx, typ))
        case _:
            for idx, param in enumerate(node.items):
                node.items[idx] = self.visit(param, enforce_type=True)
                param_type = utils.instantiate_type(
                    self.ctx,
                    utils.extract_type(self.ctx, node.items[idx]),
                    info=node.items[idx].info,
                )
                if (
                    node.items[idx].type
                    and node.items[idx].type.get_static_kind()
                    is not generics[idx].type.get_static_kind()
                ):
                    # `None` -> `NoneType`
                    if isinstance(node.items[idx], ast.NoneExpr):
                        node.items[idx] = self.visit(node.items[idx], enforce_type=True)
                    if not utils.is_type_expr(node.items[idx]):
                        raise TypecheckError(node, "expected type expression")
                infer.unify(param_type, generics[idx].type)
            infer.unify(node.type, utils.instantiate_type_var(self.ctx, typ))

    realized = infer.realize(node.type)
    if realized and result is None:
        # If the type is realizable, use the realized name instead of instantiation
        # (e.g. use Id("Ptr[byte]") instead of Instantiate(Ptr, {byte}))
        result = ast.IdExpr(
            utils.extract_type(self.ctx, realized).realized_name(), type=realized, done=True
        )

    # Handle side effects
    if not self.ctx.simple_types:
        prepends = []
        for idx, param in enumerate(node.items):
            if utils.has_side_effect(param):
                name = utils.get_temporary_var(self.ctx, "call")
                assignment = ast.AssignStmt(
                    ast.IdExpr(name),
                    rhs=param,
                    type_expr=(None if not param.type else utils.get_param_type(param.type)),
                )
                front = self.visit(assignment)
                node.items[idx] = self.visit(ast.IdExpr(name), enforce_type=True)
                prepends.append(front)
        if prepends:
            result = self.visit(ast.StmtExpr(prepends, expr=result or None))
    return node if result is None else result


def typecheck_slice(self: TypeVisitor, node: ast.SliceExpr) -> ast.Node:
    """
    Transform a slice expression.
    @example
    `start::step` -> `Slice(start, Optional.__new__(), step)`
    """

    none_call = lambda: ast.CallExpr(ast.IdExpr(ast.types.mangle(cls="Optional", func="__new__")))
    start = none_call() if node.start is None else node.start
    stop = none_call() if node.stop is None else node.stop
    step = none_call() if node.step is None else node.step
    call = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Slice), items=[start, stop, step])
    return self.visit(call)


def evaluate_static_unary(self: TypeVisitor, node: ast.UnaryExpr) -> ast.Expr | None:
    """
    Evaluate a static unary expression and return the resulting static expression.
    If the expression cannot be evaluated yet, return nullptr.
    Supported operators: (strings) not (ints) not, -, +
    """

    op_type = node.expr.type
    # Case: static strings
    if op_type.get_static_kind() is ast.types.Type.Behaviour.String:
        if node.op == "!":
            if op_type.can_realize():
                return self.visit(ast.IntExpr(int(not utils.get_str_literal(op_type))))
            if unbound := node.type.get_unbound():
                # Cannot be evaluated yet: just set the type
                unbound.static_kind = ast.types.Type.Behaviour.Int
        return None
    # Case: static bools
    if op_type.get_static_kind() is ast.types.Type.Behaviour.Bool:
        if node.op == "!":
            if op_type.can_realize():
                return self.visit(ast.BoolExpr(value=not utils.get_bool_literal(op_type)))
            if unbound := node.type.get_unbound():
                # Cannot be evaluated yet: just set the type
                unbound.static_kind = ast.types.Type.Behaviour.Bool
        return None
    # Case: static integers
    if node.op in {"-", "+", "!", "~"}:
        if op_type.can_realize():
            value = utils.get_int_literal(op_type)
            if node.op == "-":
                value = -value
            elif node.op == "~":
                value = ~value
            elif node.op == "!":
                value = int(not bool(value))
            return self.visit(ast.BoolExpr(value) if node.op == "!" else ast.IntExpr(value))
        if unbound := node.type.get_unbound():
            # Cannot be evaluated yet: just set the type
            unbound.static_kind = (
                ast.types.Type.Behaviour.Bool if node.op == "!" else ast.types.Type.Behaviour.Int
            )
    return None


def div_mod(self: TypeVisitor, left: int, right: int):
    """Division and modulus implementations."""

    if right == 0:
        raise ZeroDivisionError("static integer division or modulo by zero")
    division = abs(left) // abs(right)
    if (left < 0) != (right < 0):
        division = -division
    modulus = left - division * right
    # Use Python implementation.
    if self.ctx.cache.python_compat and modulus and ((right ^ modulus) < 0):
        modulus += right
        division -= 1
    # Use C implementation.
    return division, modulus


def evaluate_static_binary(self: TypeVisitor, node: ast.BinaryExpr) -> ast.Expr | None:
    """
    Evaluate a static binary expression and return the resulting static expression.
    If the expression cannot be evaluated yet, return nullptr.
    Supported operators: (strings) +, ==, !=
    (ints) <, <=, >, >=, ==, !=, and, or, +, -, *, //, %, ^, |, &
    """

    left_type = node.lexpr.type
    right_type = node.rexpr.type
    # Case: static strings
    if right_type.get_static_kind() is ast.types.Type.Behaviour.String:
        if node.op == "+":
            # `"a" + "b"` -> `"ab"`
            if left_type.get_str_static() and right_type.get_str_static():
                value = utils.get_str_literal(left_type) + utils.get_str_literal(right_type)
                return self.visit(ast.StringExpr(value))
            if unbound := node.type.get_unbound():
                # Cannot be evaluated yet: just set the type
                unbound.static_kind = ast.types.Type.Behaviour.String
        else:
            # `"a" == "b"` -> `False` (also handles `!=`)
            if left_type.get_str_static() and right_type.get_str_static():
                equal = utils.get_str_literal(left_type) == utils.get_str_literal(right_type)
                transformed = self.visit(ast.BoolExpr(equal if node.op == "==" else not equal))
                return transformed
            if unbound := node.type.get_unbound():
                # Cannot be evaluated yet: just set the type
                unbound.static_kind = ast.types.Type.Behaviour.Bool
        return None

    # Case: static integers
    if left_type.get_static() and right_type.get_static():
        value = (
            utils.get_int_literal(left_type)
            if left_type.get_int_static()
            else int(utils.get_bool_literal(left_type))
        )
        right = (
            utils.get_int_literal(right_type)
            if right_type.get_int_static()
            else int(utils.get_bool_literal(right_type))
        )
        if node.op == "<":
            value = int(value < right)
        elif node.op == "<=":
            value = int(value <= right)
        elif node.op == ">":
            value = int(value > right)
        elif node.op == ">=":
            value = int(value >= right)
        elif node.op == "==":
            value = int(value == right)
        elif node.op == "!=":
            value = int(value != right)
        elif node.op == "&&":
            value = right if value else value
        elif node.op == "||":
            value = value if value else right
        elif node.op == "+":
            value += right
        elif node.op == "-":
            value -= right
        elif node.op == "*":
            value *= right
        elif node.op == "^":
            value ^= right
        elif node.op == "&":
            value &= right
        elif node.op == "|":
            value |= right
        elif node.op == ">>":
            value >>= right
        elif node.op == "<<":
            value <<= right
        elif node.op == "//":
            value = div_mod(self, value, right)[0]
        elif node.op == "%":
            value = div_mod(self, value, right)[1]
        else:
            assert False, f"unknown static operator {node.op}"
        comparisons = {"==", "!=", "<", "<=", ">", ">="}
        both_bools = left_type.get_bool_static() and right_type.get_bool_static()
        literal: ast.Expr = (
            ast.BoolExpr(value)
            if node.op in comparisons or (node.op in {"&&", "||"} and both_bools)
            else ast.IntExpr(value)
        )
        return self.visit(literal)

    if unbound := node.type.get_unbound():
        comparisons = {"==", "!=", "<", "<=", ">", ">="}
        both_bools = left_type.get_bool_static() and right_type.get_bool_static()
        unbound.static_kind = (
            ast.types.Type.Behaviour.Bool
            if node.op in comparisons or (node.op in {"&&", "||"} and both_bools)
            else ast.types.Type.Behaviour.Int
        )
    return None


def transform_binary_simple(self: TypeVisitor, expr: ast.BinaryExpr) -> ast.Expr | None:
    """
    Transform a simple binary expression.
    @example
    `a and b`    -> `b if a else False`
    `a or b`     -> `True if a else b`
    `a in b`     -> `a.__contains__(b)`
    `a not in b` -> `not (a in b)`
    `a is not b` -> `not (a is b)`
    """

    # Case: simple transformations
    if expr.op == "&&":
        if expr.expected_type and expr.expected_type.is_type(ast.types.Stdlib.Bool):
            result = ast.IfExpr(
                expr.lexpr,
                ast.CallExpr(ast.DotExpr(expr.rexpr, member="__bool__")),
                ast.BoolExpr(False),
            )
        else:
            name = utils.get_temporary_var(self.ctx, "cond")
            result = ast.IfExpr(
                ast.AssignExpr(ast.IdExpr(name), expr=expr.lexpr), expr.rexpr, ast.IdExpr(name)
            )
            result.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Union)
    elif expr.op == "||":
        if expr.expected_type and expr.expected_type.is_type(ast.types.Stdlib.Bool):
            result = ast.IfExpr(
                expr.lexpr,
                ast.BoolExpr(True),
                ast.CallExpr(ast.DotExpr(expr.rexpr, member="__bool__")),
            )
        else:
            name = utils.get_temporary_var(self.ctx, "cond")
            result = ast.IfExpr(
                ast.AssignExpr(ast.IdExpr(name), expr=expr.lexpr), ast.IdExpr(name), expr.rexpr
            )
            result.expected_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Union)
    elif expr.op == "not in":
        result = ast.CallExpr(
            ast.DotExpr(
                ast.CallExpr(ast.DotExpr(expr.rexpr, member="__contains__"), items=[expr.lexpr]),
                member="__invert__",
            )
        )
    elif expr.op == "in":
        result = ast.CallExpr(ast.DotExpr(expr.rexpr, member="__contains__"), items=[expr.lexpr])
    elif (
        expr.op == "is"
        and isinstance(expr.lexpr, ast.NoneExpr)
        and isinstance(expr.rexpr, ast.NoneExpr)
    ):
        result = ast.BoolExpr(True)
    elif expr.op == "is" and isinstance(expr.lexpr, ast.NoneExpr):
        result = ast.BinaryExpr(expr.rexpr, op="is", rexpr=expr.lexpr)
    elif expr.op == "is not":
        result = ast.UnaryExpr("!", expr=ast.BinaryExpr(expr.lexpr, op="is", rexpr=expr.rexpr))
    else:
        return None
    return self.visit(result)


def transform_binary_is(self: TypeVisitor, expr: ast.BinaryExpr) -> ast.Expr | None:
    """
    Transform a binary `is` expression by checking for type equality. Handle special `is
    None` cаses as well. See inside for details.
    """

    assert expr.op == "is", "not an is binary expression"
    has_side_left = utils.has_side_effect(expr.lexpr)
    has_side_right = utils.has_side_effect(expr.rexpr)

    def wrap_side(value: ast.Expr) -> ast.Expr:
        statements = []
        if has_side_left:
            statements.append(ast.ExprStmt(expr.lexpr))
        if has_side_right:
            statements.append(ast.ExprStmt(expr.rexpr))
        wrapped = ast.StmtExpr(statements, expr=value) if statements else value
        return self.visit(wrapped)

    # Case: `is None` expressions
    if isinstance(expr.rexpr, ast.NoneExpr):
        left_type = utils.extract_class_type(self.ctx, expr.lexpr)
        if left_type.is_type(ast.types.Stdlib.NoneType):
            return wrap_side(ast.BoolExpr(True))
        if not left_type.is_type(ast.types.Stdlib.Optional):
            # lhs is not optional: `return False`
            return wrap_side(ast.BoolExpr(False))
        final_type = left_type
        while final_type[0].is_type(ast.types.Stdlib.Optional):
            final_type = final_type[0].get_class()
        final_type = final_type[0]
        if final_type.get_class() is None:
            # Special case: Optional[Optional[... Optional[NoneType]]...] == NoneType
            infer.unify(
                expr.type,
                utils.instantiate_unbound(self.ctx, static_kind=ast.types.Type.Behaviour.Bool),
            )
            return None
        if final_type.is_type(ast.types.Stdlib.NoneType):
            return wrap_side(ast.BoolExpr(True))

        # lhs is optional: `return lhs.__has__().__invert__()`
        link = expr.type.get_unbound()
        if link and expr.type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
            link.static_kind = ast.types.Type.Behaviour.Runtime
        return self.visit(
            ast.CallExpr(
                ast.DotExpr(
                    ast.CallExpr(ast.DotExpr(expr.lexpr, member="__has__")), member="__invert__"
                )
            )
        )

    # Check the type equality (operand types and __raw__ pointers must match).
    if utils.is_type_expr(expr.lexpr) and utils.is_type_expr(expr.rexpr):
        left_type = utils.extract_type(self.ctx, expr.lexpr).get_class()
        right_type = utils.extract_type(self.ctx, expr.rexpr).get_class()
        if left_type is None or right_type is None:
            return None
        return wrap_side(
            ast.BoolExpr(
                left_type.name == right_type.name and left_type.unify(right_type, None) >= 0
            )
        )

    left_type = infer.realize(expr.lexpr.type)
    right_type = infer.realize(expr.rexpr.type)
    if left_type is None or right_type is None:
        # Types not known: return early
        infer.unify(expr.type, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Bool))
        return None

    if not left_type.is_record() and not right_type.is_record():
        # Both reference types: `return type._is(lhs, rhs)`
        result = ast.CallExpr(
            ast.IdExpr(ast.types.mangle(cls="type", func="_is")), items=[expr.lexpr, expr.rexpr]
        )
    elif left_type.is_type(ast.types.Stdlib.Optional):
        # lhs is optional: `return lhs.__is_optional__(rhs)`
        result = ast.CallExpr(ast.DotExpr(expr.lexpr, member="__is_optional__"), items=[expr.rexpr])
    elif right_type.is_type(ast.types.Stdlib.Optional):
        # rhs is optional: `return rhs.__is_optional__(lhs)`
        result = ast.CallExpr(ast.DotExpr(expr.rexpr, member="__is_optional__"), items=[expr.lexpr])
    elif left_type.realized_name() != right_type.realized_name():
        # tuple names do not match: `return False`
        return wrap_side(ast.BoolExpr(value=False))
    else:
        # Same tuple types: `return lhs == rhs`
        result = ast.BinaryExpr(expr.lexpr, op="==", rexpr=expr.rexpr)
    return self.visit(result)


def get_magic(op: str):
    """Return a binary magic opcode for the provided operator."""

    # Table of supported binary operations and the corresponding magic methods.
    magics = {
        "+": "add",
        "-": "sub",
        "*": "mul",
        "**": "pow",
        "/": "truediv",
        "//": "floordiv",
        "@": "matmul",
        "%": "mod",
        "<": "lt",
        "<=": "le",
        ">": "gt",
        ">=": "ge",
        "==": "eq",
        "!=": "ne",
        "<<": "lshift",
        ">>": "rshift",
        "&": "and",
        "|": "or",
        "^": "xor",
    }
    assert not (op not in magics), f"invalid binary operator '{op}'"
    right_magics = {
        "<": "gt",
        "<=": "ge",
        ">": "lt",
        ">=": "le",
        "==": "eq",
        "!=": "ne",
    }
    magic = magics[op]
    return magic, right_magics.get(op, f"r{magic}")


def transform_binary_inplace_magic(
    self: TypeVisitor, expr: ast.BinaryExpr, is_atomic: bool
) -> ast.Expr | None:
    """
    Transform an in-place binary expression.
    @example
    `a op= b` -> `a.__iopmagic__(b)`
    @param isAtomic if set, use atomic magics if available.
    """

    magic, _ = get_magic(expr.op)
    left_type = expr.lexpr.get_class_type()
    assert left_type is not None, "lhs type not known"

    method = None
    # Atomic operations: check if `lhs.__atomic_op__(Ptr[lhs], rhs)` exists
    if is_atomic:
        pointer = utils.instantiate_type(
            self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr), [left_type]
        )
        method = utils.find_best_method(
            self.ctx, left_type, f"__atomic_{magic}__", [pointer, expr.rexpr.type]
        )
        if method:
            expr.lexpr = ast.CallExpr(ast.IdExpr("__ptr__"), items=[expr.lexpr])

    # In-place operations: check if `lhs.__iop__(lhs, rhs)` exists
    if not method and expr.in_place:
        method = utils.find_best_method(
            self.ctx, left_type, f"__i{magic}__", [left_type, expr.rexpr.type]
        )
    if method:
        result = ast.CallExpr(ast.IdExpr(method.get_func_name()), items=[expr.lexpr, expr.rexpr])
        return self.visit(result)

    return None


def transform_binary_magic(self: TypeVisitor, expr: ast.BinaryExpr) -> ast.Expr | None:
    """
    Transform a magic binary expression.
    @example
    `a op b` -> `a.__opmagic__(b)`
    """

    magic, right_magic = get_magic(expr.op)
    left_type = expr.lexpr.type
    right_type = expr.rexpr.type
    if not left_type.is_type("pyobj") and right_type.is_type("pyobj"):
        # Special case: `obj op pyobj` -> `rhs.__rmagic__(lhs)` on lhs
        # Assumes that pyobj implements all left and right magics
        left_name = utils.get_temporary_var(self.ctx, "l")
        right_name = utils.get_temporary_var(self.ctx, "r")
        replacement = ast.StmtExpr(
            [
                ast.AssignStmt(ast.IdExpr(left_name), rhs=expr.lexpr),
                ast.AssignStmt(ast.IdExpr(right_name), rhs=expr.rexpr),
            ],
            expr=ast.CallExpr(
                ast.DotExpr(ast.IdExpr(right_name), member=f"__{right_magic}__"),
                items=[ast.IdExpr(left_name)],
            ),
        )
    elif left_type.get_union():
        # Special case: `union op obj` -> `union.__magic__(rhs)`
        replacement = ast.CallExpr(
            ast.DotExpr(expr.lexpr, member=f"__{magic}__"), items=[expr.rexpr]
        )
    else:
        left_type = left_type.get_class()
        right_type = right_type.get_class()
        # Normal operations: check if `lhs.__magic__(lhs, rhs)` exists
        if left_type and (
            method := utils.find_best_method(
                self.ctx, left_type, f"__{magic}__", [left_type, right_type]
            )
        ):
            # Normal case: `__magic__(lhs, rhs)`
            result = ast.CallExpr(
                ast.IdExpr(method.get_func_name()), items=[expr.lexpr, expr.rexpr]
            )
        elif right_type and (
            method := utils.find_best_method(
                self.ctx, right_type, f"__{right_magic}__", [right_type, left_type]
            )
        ):
            # Right-side magics: check if `rhs.__rmagic__(rhs, lhs)` exists
            left_name = utils.get_temporary_var(self.ctx, "l")
            right_name = utils.get_temporary_var(self.ctx, "r")
            result = ast.StmtExpr(
                [
                    ast.AssignStmt(ast.IdExpr(left_name), rhs=expr.lexpr),
                    ast.AssignStmt(ast.IdExpr(right_name), rhs=expr.rexpr),
                ],
                expr=ast.CallExpr(
                    ast.IdExpr(method.get_func_name()),
                    items=[ast.IdExpr(right_name), ast.IdExpr(left_name)],
                ),
            )
        else:
            return None
    return self.visit(result)


def transform_static_tuple_index(
    self: TypeVisitor,
    tuple_type: ast.types.Class,
    expr: ast.Expr,
    index: ast.Expr,
) -> Tuple[bool, ast.Expr | None]:
    """
    Given a tuple type and the expression `expr[index]`, check if an `index` is static
    (integer or slice). If so, statically extract the specified tuple item or a
    sub-tuple (if the index is a slice).
    Works only on normal tuples and partial functions.
    """

    is_static_str = expr.type.get_static_kind() is ast.types.Type.Behaviour.String
    if is_static_str and not expr.type.can_realize():
        return True, None
    if not is_static_str:
        if not tuple_type.is_record():
            return False, None
        if not tuple_type.is_type(ast.types.Stdlib.Tuple):
            if tuple_type.is_type(ast.types.Stdlib.Optional):
                if new_tuple := tuple_type[0].get_class():
                    unwrapped = self.visit(
                        ast.CallExpr(ast.IdExpr(ast.types.Stdlib.OptionalUnwrap), items=[expr])
                    )
                    return transform_static_tuple_index(self, new_tuple, unwrapped, index)
                return True, None
            return False, None

    def get_integer(value: ast.Expr | None, default: int):
        """Extract the static integer value from expression"""
        if value is None:
            return True, default
        result = self.visit(value.clone())
        if static := result.type.get_int_static():
            return True, static.value
        return False, default

    str_value = utils.get_str_literal(expr.type) if is_static_str else ""
    class_fields = [] if is_static_str else utils.get_class_fields(tuple_type)
    size = len(str_value) if is_static_str else len(class_fields)
    start, stop, step = 0, size, 1
    multiple = False
    int_ok, int_val = get_integer(index, start)
    if int_ok:
        # Case: `tuple[int]`
        start = translate_index(self, int_val, stop)
    elif isinstance(index, ast.SliceExpr):
        # Case: `tuple[int:int:int]`
        start_ok, start = get_integer(index.start, start)
        stop_ok, stop = get_integer(index.stop, stop)
        step_ok, step = get_integer(index.step, step)
        if not start_ok or not stop_ok or not step_ok:
            return False, None

        # Adjust slice indices (Python slicing rules)
        if index.step and index.start is None:
            start = 0 if step > 0 else size - 1
        if index.step and index.stop is None:
            stop = size if step > 0 else -(size + 1)
        start_ref = [start]
        stop_ref = [stop]
        slice_adjust_indices(self, size, start_ref, stop_ref, step)
        start, stop = start_ref[0], stop_ref[0]
        multiple = True
    else:
        return False, None

    # Static string slicing
    if is_static_str:
        value = (
            str_value[start]
            if not multiple
            else "".join(str_value[i] for i in range(start, stop, step))
        )
        return True, self.visit(ast.StringExpr(value))
    if not multiple:
        return True, self.visit(ast.DotExpr(expr, member=class_fields[start].name))

    # Tuple slicing: generate a sub-tuple
    name = ast.IdExpr(utils.get_temporary_var(self.ctx, "tup"))
    assignment = ast.AssignStmt(name, rhs=expr)
    tuple_items = []
    for idx in range(start, stop, step):
        if idx < 0 or idx >= size:
            raise TypecheckError(
                index,
                f"tuple index out of range (expected 0..{size - 1}, got instead {idx})",
            )
        tuple_items.append(ast.DotExpr(cast(ast.Expr, name.clone()), member=class_fields[idx].name))
    classes.generate_tuple(self, len(tuple_items))
    result = ast.StmtExpr(
        [assignment],
        expr=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Tuple), items=tuple_items),
    )
    return True, self.visit(result)


def translate_index(self: TypeVisitor, index: int, length: int, clamp: bool = False):
    """
    Follow Python indexing rules for static tuple indices.
    Taken from https://github.com/python/cpython/blob/main/Objects/sliceobject.c.
    """

    if index < 0:
        index += length
    if clamp:
        index = max(index, 0)
        index = min(index, length)
    elif index < 0 or index >= length:
        raise TypecheckError(
            self.ctx.node_stack[-1],
            f"tuple index out of range (expected 0..{length - 1}, got instead {index})",
        )
    return index


def slice_adjust_indices(
    self: TypeVisitor, length: int, start: List[int], stop: List[int], step: int
) -> int:
    """
    Follow Python slice indexing rules for static tuple indices.
    Taken from https://github.com/python/cpython/blob/main/Objects/sliceobject.c.
    Quote (sliceobject.c:269): "this is harder to get right than you might think"
    """

    if step == 0:
        raise TypecheckError(self.ctx.node_stack[-1], "slice step cannot be zero")
    if start[0] < 0:
        start[0] += length
        if start[0] < 0:
            start[0] = -1 if step < 0 else 0
    elif start[0] >= length:
        start[0] = length - 1 if step < 0 else length

    if stop[0] < 0:
        stop[0] += length
        if stop[0] < 0:
            stop[0] = -1 if step < 0 else 0
    elif stop[0] >= length:
        stop[0] = length - 1 if step < 0 else length

    if step < 0:
        if stop[0] < start[0]:
            return (start[0] - stop[0] - 1) // (-step) + 1
    elif start[0] < stop[0]:
        return (stop[0] - start[0] - 1) // step + 1
    return 0
