# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import Callable, Dict, List, Set, Tuple, cast, contextmanager, dataclass
from ... import ast, cache
from ...error import ErrorMessage, ParserErrors, TypecheckError
from . import infer
from .classes import generate_tuple
from .ctx import TypeContext

if TYPE_CHECKING:
    from . import TypeVisitor


@dataclass
class PartialCallData:
    """Holds partial call information for a CallExpr."""

    # true if the call is partial
    is_partial: bool = False
    # set if calling a partial type itself
    var: str = ""
    # mask of known args
    known: str = ""
    args: ast.Expr | None = None
    # partial *args/**kwargs expressions
    kw_args: ast.Expr | None = None


def best_method(
    ctx: TypeContext, typ: ast.types.Class, member: str, args
) -> ast.types.Function | None:
    """
    Select the best method indicated of an object that matches the given arg
    types. See @c findMatchingMethods for details.
    """
    call_args = []
    for arg in args:
        if isinstance(arg, ast.types.Type):
            call_args.append(ast.CallExpr.Arg(value=ast.NoneExpr(type=arg)))
        elif isinstance(arg, ast.Expr):
            call_args.append(ast.CallExpr.Arg(value=arg))
        else:
            call_args.append(ast.CallExpr.Arg(name=arg[0], value=ast.NoneExpr(type=arg[1])))
    methods = find_method(ctx, typ, member, hide_shadowed=False)
    if matches := matching_methods(ctx, typ, methods, call_args):
        return matches[0]
    return None


def matching_methods(
    ctx: TypeContext,
    typ: ast.types.Class,
    methods: List[ast.types.Function],
    args: List[ast.CallExpr.Arg],
    partial: ast.types.Class | None = None,
) -> List[ast.types.Function]:
    """
    Select the best method among the provided methods given the list of args.
    See @c reorderNamedArgs for details.
    """

    # Pick the last method that accepts the given args
    results = []
    for method in methods:
        if not method:
            continue  # avoid overloads that have not been seen yet
        fn = instantiate(ctx, method, typ).func
        if fn and can_call(ctx, fn, args, partial) >= 0:
            results.append(method)
    return results


def can_call(
    ctx: TypeContext,
    function: ast.types.Function,
    args: List[ast.CallExpr.Arg],
    partial: ast.types.Class | None = None,
) -> int:
    """
    Check if a function can be called with the given args.
    See @c reorderNamedArgs for details.
    """
    partial_args = []
    known = []
    if partial and partial.partial:
        known = partial.partial_mask
        known_arg_types = partial[1].require_cls
        known_idx = 0
        for flag in known:
            if flag == ast.types.Class.Flag.Included:
                partial_args.append(known_arg_types[known_idx])
                known_idx += 1
            elif flag == ast.types.Class.Flag.Default:
                known_idx += 1

    reordered: List[Tuple[ast.types.Type | None, int]] = []
    non_inferrable = function.ast.get_non_inferrable_generics()

    try:
        score, (star_idx, kwstar_idx, slots, _) = reorder_named_args(ctx, function, args, known)
        generic_idx = 0
        partial_idx = 0
        for slot_idx, slot in enumerate(slots):
            param = function.ast.items[slot_idx]
            if param.is_generic():
                if not slot:
                    if param.name in non_inferrable and not param.default:
                        return -1  # is this "real" type?
                    reordered.append((None, 0))
                else:
                    expected_type = extract_func_generic(function, generic_idx)
                    arg = args[slot[0]].value
                    if expected_type.is_runtime and not is_type_expr(arg):
                        return -1
                    reordered.append((arg.type, slot[0]))
                generic_idx += 1
            elif slot_idx in {star_idx, kwstar_idx} or len(slot) != 1:
                # Partials
                if (
                    not slot
                    and partial
                    and partial.partial
                    and known[slot_idx] is ast.types.Class.Flag.Included
                ):
                    reordered.append((partial_args[partial_idx], 0))
                    partial_idx += 1
                else:
                    # Ignore *args, *kwargs and default args
                    reordered.append((None, 0))
            else:
                reordered.append((args[slot[0]].value.type, slot[0]))
    except TypecheckError:
        return -1

    value_idx = 0
    generic_idx = 0
    real_generic_idx = 0
    arg_idx = 0
    while score != -1 and arg_idx < len(reordered):
        param = function.ast.items[arg_idx]
        if param.is_value():
            expected_type = function[value_idx]
            value_idx += 1
        else:
            expected_type = extract_func_generic(function, generic_idx)
            generic_idx += 1
        arg_type, source_idx = reordered[arg_idx]
        if arg_type is None:
            arg_idx += 1
            continue
        if not param.is_value():
            real_generic_idx += 1
            if not expected_type.is_runtime:
                arg = args[source_idx].value.type
                # Check if this is a good generic!
                if arg and arg.require_cls.is_runtime:
                    score = -1
                    break
                arg_type = arg
            else:
                arg_idx += 1
                # TODO: check if these are real types or if traits are satisfied
                continue

        assert arg_type
        _, wrapped_type, _ = can_wrap_expr(ctx, arg_type, expected_type, function)
        candidate = wrapped_type or arg_type
        if candidate.unify(expected_type, None) < 0:
            score = -1
        arg_idx += 1
    if score >= 0:
        score += int(real_generic_idx == len(function.func_generics))
    return score


def wrap_expr(
    ctx: TypeContext,
    expr: ast.Expr,
    expected_type: ast.types.Type | None,
    callee: ast.types.Function | None = None,
    allow_unwrap: bool = True,
) -> Tuple[bool, ast.Expr]:
    """
    Wrap an expression to coerce it to the expected type if the type of the expression
    does not match it. Also unify types.
    @example
    expected `Generator`                -> `expr.__iter__()`
    expected `float`, got `int`         -> `float(expr)`
    expected `Optional[T]`, got `T`     -> `Optional(expr)`
    expected `T`, got `Optional[T]`     -> `unwrap(expr)`
    expected `Function`, got a function -> partialize function
    expected `T`, got `Union[T...]`     -> `Union._get(expr, T)`
    expected `Union[T...]`, got `T`     -> `Union._new(expr, Union[T...])`
    expected base class, got derived    -> downcast to base class
    @param allowUnwrap allow optional unwrapping.
    """

    from . import TypeVisitor

    assert expr.type
    can_wrap, _, wrapper = can_wrap_expr(
        ctx, expr.type, expected_type, callee, allow_unwrap, isinstance(expr, ast.EllipsisExpr)
    )
    # TODO: get rid of this line one day!
    if not expr.type.is_runtime and (not expected_type or expected_type.is_runtime):
        expr.type = get_underlying_static_type(ctx, expr.type)
    if can_wrap and wrapper:
        expr = TypeVisitor(ctx).visit_expr(wrapper(expr))
    return can_wrap, expr


def can_wrap_expr(
    ctx: TypeContext,
    expr_type: ast.types.Type,
    expected_type: ast.types.Type | None,
    callee: ast.types.Function | None = None,
    allow_unwrap: bool = True,
    is_ellipsis: bool = False,
):
    from . import TypeVisitor

    expected_class = expected_type.require_cls if expected_type else None
    expr_class = expr_type.require_cls
    wrapped_type = None
    wrapper: Callable[[ast.Expr], ast.Expr] | None = None
    if (
        callee
        and callee.ast
        and callee.ast.has_function_attr(
            ast.types.mangle("std.internal.attributes", func="no_arg_wrap")
        )
    ):
        return True, expected_type, None  # do not wrap

    # Case: types are wrapped in TypeWrap when type is not expected
    if callee and expr_type == ast.types.Stdlib.Type:
        expr_type = extract_class_type(ctx, expr_type)
        if not expr_type:
            return False, None, None
        if not expected_class or expected_class != ast.types.Stdlib.Type:
            wrapped_type = instantiate(ctx, ast.types.Stdlib.TypeWrap, [expr_type])
            wrapper = lambda value: ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.TypeWrap),
                items=[value],
            )
        return True, wrapped_type, wrapper
    if (expected_type is None or expected_type.is_runtime) and not expr_type.is_runtime:
        expr_type = get_underlying_static_type(ctx, expr_type)
        expr_class = expr_type.require_cls
        wrapped_type = expr_type

    hints = {ast.types.Stdlib.Generator, ast.types.Stdlib.Float, ast.types.Stdlib.Optional, "pyobj"}
    if expr_class is None and expected_class and expected_class.name in hints:
        return False, None, None  # arg type not yet known.

    # Use Capsule ONLY if the type signature explicitly asks for it!
    if (
        allow_unwrap
        and expected_class
        and expected_class == ast.types.Stdlib.Capsule
        and expr_class
        and expr_class != ast.types.Stdlib.Capsule
    ):
        wrapped_type = instantiate(ctx, ast.types.Stdlib.Capsule, [expr_class])

        def wrap_capsule(value: ast.Expr) -> ast.Expr:
            match value:
                case ast.CallExpr(expr=expr, items=[item]) if (
                    is_function_expr(expr, ast.types.mangle(cls="Capsule", func="_get")) and item
                ):
                    # Do not wrap already wrapped vars
                    return item.value
                case _:
                    return ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Capsule", func="_make")), items=[value]
                    )

        wrapper = wrap_capsule
    elif (
        expected_class
        and expected_class != ast.types.Stdlib.Any
        and expr_class
        and expr_class == ast.types.Stdlib.Any
    ):
        wrapped_type = expected_class

        def unwrap_any(value: ast.Expr) -> ast.Expr:
            realized = infer.realize(ctx, expected_class)
            assert realized
            return ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.OptionalUnwrap),
                items=[value, ast.IdExpr(realized.realized_name())],
            )

        wrapper = unwrap_any
    elif (
        expected_class
        and expected_class == (ast.types.Stdlib.Any)
        and expr_class
        and expr_class != (ast.types.Stdlib.Any)
    ):
        wrapped_type = expected_class
        wrapper = lambda value: ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Any), items=[value])
    elif (
        expected_class
        and expected_class == ast.types.Stdlib.Generator
        and expr_class
        and expr_class != expected_class.name
        and not is_ellipsis
    ):
        if not find_method(ctx, expr_class, "__iter__"):
            # Do not wrap already wrapped vars
            return False, None, None
        # Note: do not do this in pipelines (TODO: why?)
        wrapped_type = instantiate(ctx, expected_class)
        wrapper = lambda value: ast.CallExpr(ast.DotExpr(value, member="__iter__"), items=[])
    elif expected_class and expected_class == "float" and expr_class and expr_class == "int":
        wrapped_type = instantiate(ctx, expected_class)
        wrapper = lambda value: ast.CallExpr(ast.IdExpr("float"), items=[value])
    elif (
        callee is None
        and expected_class
        and expected_class == ast.types.Stdlib.Bool
        and expr_class
        and expr_class != ast.types.Stdlib.Bool
    ):
        # Do not do this in function calls---only use for if-else wrapping
        wrapped_type = instantiate(ctx, expected_class)
        wrapper = lambda value: ast.CallExpr(ast.DotExpr(value, member="__bool__"), items=[])
    elif (
        expected_class
        and expected_class == ast.types.Stdlib.Optional
        and expr_class
        and expr_class != ast.types.Stdlib.Optional
    ):
        expected_inner = expected_class[0]
        _, inner_type, inner_wrapper = can_wrap_expr(
            ctx, expr_class, expected_inner, callee, allow_unwrap, is_ellipsis
        )
        wrapped_type = instantiate(ctx, ast.types.Stdlib.Optional, [inner_type or expr_class])
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.Optional),
            items=[value if inner_wrapper is None else inner_wrapper(value)],
        )
    elif (
        allow_unwrap
        and expected_class
        and expr_class
        and expr_class == ast.types.Stdlib.Optional
        and expected_class != ast.types.Stdlib.Optional
    ):
        expression_inner = expr_class[0]
        _, inner_type, inner_wrapper = can_wrap_expr(
            ctx, expression_inner, expected_class, callee, allow_unwrap, is_ellipsis
        )
        wrapped_type = instantiate(ctx, inner_type or expression_inner)
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.OptionalUnwrap),
            items=[inner_wrapper(value) if inner_wrapper else value],
        )
    elif expected_class and expected_class == "pyobj" and expr_class and expr_class != "pyobj":
        if not find_method(ctx, expr_class, "__to_py__"):
            return False, None, None
        wrapped_type = instantiate(ctx, expected_class)
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr("pyobj"),
            items=[ast.CallExpr(ast.DotExpr(value, member="__to_py__"))],
        )
    elif (
        allow_unwrap
        and expected_class
        and expr_class
        and expr_class == "pyobj"
        and expected_class != "pyobj"
    ):
        if not find_method(ctx, expected_class, "__from_py__"):
            return False, None, None
        wrapped_type = instantiate(ctx, expected_class)
        wrapper = lambda value: ast.CallExpr(
            ast.DotExpr(ast.IdExpr(expected_class.name, type=expected_type), member="__from_py__"),
            items=[ast.DotExpr(value, member="p")],
        )
    elif (
        expected_class
        and expected_class == ast.types.Stdlib.Callable
        and expr_class
        and (expr_class.partial or expr_type.func or expr_class == ast.types.Stdlib.Function)
    ):
        arg_types = []
        # Get list of args
        function_type: ast.types.Function | None = None

        if partial := expr_class.partial:
            function_type = instantiate(ctx, partial.partial_func)
            for idx, flag in enumerate(partial.partial_mask):
                if flag != ast.types.Class.Flag.Included:
                    arg_types.append(function_type[idx])
            ret_type = function_type.ret_type
        else:
            tuple_type = expr_class[0].require_cls
            for generic in tuple_type.generics:
                arg_types.append(generic.type)
            ret_type = expr_class[1]

        expected_args = expected_class[0].require_cls
        if len(arg_types) != len(expected_args.generics):
            return False, None, None
        for idx, arg_type in enumerate(arg_types):
            expected_arg = expected_args[idx]
            if arg_type.unify(expected_arg) < 0:
                return False, None, None
        if ret_type.unify(expected_class[1]) < 0:
            return False, None, None

        wrapped_type = expected_type

        def wrap_callable(value: ast.Expr) -> ast.Expr:
            assert value.type
            value_class = value.type.require_cls
            assert wrapped_type
            expected_class = wrapped_type.require_cls

            value_arg_types = []
            if value_partial := value_class.partial:
                partial_fn = instantiate(ctx, value_partial.partial_func)
                for idx, flag in enumerate(value_partial.partial_mask):
                    if flag != ast.types.Class.Flag.Included:
                        value_arg_types.append(partial_fn[idx])
                value_return_type = partial_fn.ret_type
            else:
                value_tuple = value_class[0].require_cls
                for generic in value_tuple.generics:
                    value_arg_types.append(generic.type)
                value_return_type = value_class[1]

            callable_args = expected_class[0].require_cls
            for idx, arg_type in enumerate(value_arg_types):
                arg_type |= callable_args[idx]
            value_return_type |= expected_class[1]

            ret_fn: ast.Expr | None = None
            data_arg: ast.Expr | None = None
            data_type: ast.Expr | None = None
            if value_partial:
                value_class = infer.realize(ctx, value_class, force=True)
                assert value_class
                fn_name = value_class.realized_name()
                ret_fn = ast.IndexExpr(
                    ast.CallExpr(
                        ast.IndexExpr(
                            ast.IdExpr(ast.types.Stdlib.Ptr),
                            index=ast.IdExpr(value_class.realized_name()),
                        ),
                        items=[ast.IdExpr("data")],
                    ),
                    index=ast.IntExpr(0),
                )
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            elif value.type.func:
                value_class = infer.realize(ctx, value_class, force=True)
                assert value_class
                fn_name = value_class.realized_name()
                ret_fn = ast.IdExpr(value_class.realized_name())
                data_arg = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.CObj))
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            elif value_class.name == ast.types.Stdlib.Function:
                value_class = infer.realize(ctx, value_class, force=True)
                assert value_class
                fn_name = value_class.realized_name()
                ret_fn = ast.CallExpr(
                    ast.IdExpr(value_class.realized_name()),
                    items=[ast.IdExpr("data")],
                )
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            else:
                assert False, f"bad type: {value_class!r}"

            fn_name = f".proxy.{fn_name}"
            if not ctx.get(fn_name):
                proxy = ast.FunctionStmt(
                    fn_name,
                    items=[
                        ast.Param("data", type=data_type),
                        ast.Param("args", type=ast.IdExpr(callable_args.realized_name())),
                    ],
                    suite=ast.ReturnStmt(
                        expr=ast.CallExpr(ret_fn, items=[ast.StarExpr(ast.IdExpr("args"))])
                    ),
                )
                TypeVisitor(ctx).visit_stmt(proxy)
            return ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.Callable),
                items=[ast.IdExpr(fn_name), data_arg or value],
            )

        wrapper = wrap_callable
    elif (
        callee
        and expr_class
        and expr_type.func
        and not (expected_class and expected_class.name == ast.types.Stdlib.Function)
    ):
        if expected_class:
            wrapped_type = instantiate(ctx, expected_class)
        # Create wrapper if needed
        function_name = expr_type.func.func_name

        def wrap_raw_function(value: ast.Expr) -> ast.Expr:
            partial_call = ast.CallExpr(
                ast.IdExpr(function_name), items=[ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial)]
            )
            if isinstance(value, ast.StmtExpr):
                return ast.StmtExpr(value.items, expr=partial_call)
            return partial_call

        wrapper = wrap_raw_function
    elif (
        expected_class
        and expected_class.name == ast.types.Stdlib.Function
        and expr_class
        and expr_class.partial
        and expr_class.is_partial_empty
    ):
        wrapped_type = instantiate(ctx, expected_class)
        empty_function_name = expr_class.partial_func.ast.name
        empty_function_type = instantiate(ctx, ctx[empty_function_name].type)
        if wrapped_type.unify(empty_function_type) >= 0:
            wrapper = lambda _: ast.IdExpr(empty_function_name)
        else:
            wrapped_type = None
    elif (
        allow_unwrap
        and expr_class
        and expr_class.union
        and expected_class
        and not expected_class.union
    ):
        if not (expected_class := infer.realize(ctx, expected_class)):
            return False, None, None
        expr_class = infer.realize(ctx, expr_class)
        if not expr_class or not expr_class.union:
            return False, None, None
        union_types = expr_class.union.get_realization_types()
        if any(item.unify(expected_class) >= 0 for item in union_types):
            wrapped_type = expected_class
            wrapper = lambda value: ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get")),
                items=[
                    value,
                    ast.IdExpr(expected_class.realized_name()),
                ],
            )
    elif expr_class and expected_class and expected_class.union:
        if not (expected_class := infer.realize(ctx, expected_class)):
            return False, None, None
        # Wrap raw Seq functions into Partial(...) call for easy realization.
        # Special case: Seq functions are embedded (via lambda!)
        if not (expected_class | expr_class):
            wrapped_type = expected_class
            wrapper = lambda value: ast.CallExpr(
                ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Union), member="_new"),
                items=[value, ast.IdExpr(expected_class.realized_name())],
            )
    elif (
        expr_class
        and expr_class == ast.types.Stdlib.Type
        and expected_class
        and expected_class == ast.types.Stdlib.TypeWrap
    ):
        wrapped_type = instantiate(ctx, ast.types.Stdlib.TypeWrap, [expr_class])
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.TypeWrap),
            items=[value],
        )
    elif expr_class and expected_class:
        source = expr_class
        destination = expected_class
        optional = source == ast.types.Stdlib.Optional and destination == ast.types.Stdlib.Optional
        if optional:
            source = source[0].require_cls
            destination = destination[0].require_cls
        if source and destination and source.name != destination.name:
            source_data = get_class(ctx, source)
            assert source_data
            # Cast derived classes to base classes
            for mro in source_data.mro[1:]:
                base = instantiate(ctx, mro, source)
                if base.unify(destination) >= 0:
                    base |= destination
                    wrapped_type = expected_class

                    def cast_base(value: ast.Expr, base_type: ast.types.Type = base) -> ast.Expr:
                        base_class = base_type.require_cls
                        type_expr = ast.IdExpr(
                            base_class.name, type=instantiate_type_var(ctx, base_class)
                        )
                        if optional:
                            type_expr = ast.InstantiateExpr(
                                ast.IdExpr(ast.types.Stdlib.Optional), items=[type_expr]
                            )
                        return ast.CallExpr(
                            ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_cast")),
                            items=[value, type_expr],
                        )

                    wrapper = cast_base
                    break
    return True, wrapped_type, wrapper


def unpack_tuple_types(
    visitor: TypeVisitor, expr: ast.Expr
) -> List[Tuple[str, ast.types.Type]] | None:
    """
    Unpack a Tuple or KwTuple expression into (name, type) vector.
    Name is empty when handling Tuple; otherwise it matches names of KwTuple.
    """
    result = []
    match expr.orig or expr:
        case ast.TupleExpr(items=items):
            for idx, arg in enumerate(items):
                arg = visitor.visit_expr(arg)
                if not arg.cls:
                    return None
                items[idx] = arg
                result.append(("", arg.type))
            return result
        case ast.CallExpr():
            value = extract_class_type(visitor.ctx, expr.type)
            tuple_values = value[1].require_cls
            if (
                value != ast.types.Stdlib.NamedTuple
                or not tuple_values
                or not value[0].can_realize()
            ):
                return None
            tuple_id = value[0].require_int
            assert 0 <= tuple_id < len(visitor.ctx.cache.generated_tuple_names)
            names = visitor.ctx.cache.generated_tuple_names[tuple_id]
            for idx in range(len(tuple_values.generics)):
                if not (item_type := tuple_values[idx]):
                    return None
                result.append((names[idx], item_type))
            return result
        case _:
            return None


def extract_named_tuple(ctx: TypeContext, expr: ast.Expr) -> List[Tuple[str, ast.Expr]]:
    assert expr.type
    tuple_id = expr.type.require_cls[0].require_int
    assert 0 <= tuple_id < len(ctx.cache.generated_tuple_names)
    names = ctx.cache.generated_tuple_names[tuple_id]
    return [
        (name, ast.IndexExpr(ast.DotExpr(expr, member="args"), index=ast.IntExpr(idx)))
        for idx, name in enumerate(names)
    ]


def get_class_fields(ctx: TypeContext, cls: ast.types.Class) -> List[cache.ClassData.Field]:
    cache_class = get_class(ctx, cls.name)
    fields = [] if not cache_class else list(cache_class.fields)
    if cls.name == ast.types.Stdlib.Tuple:
        fields = fields[: len(cls.generics)]
    return fields


def get_class_field_types(ctx: TypeContext, cls: ast.types.Class) -> List[ast.types.Type]:
    result = []
    with with_class_generics(ctx, cls):
        for field in get_class_fields(ctx, cls):
            field_type = instantiate(ctx, field.type, cls)
            if not field_type.can_realize() and field.type_expr is not None:
                type_expr = ctx.cache.typecheck(field.type_expr.clone(clean=True), ctx=ctx)
                field_type |= extract_type(ctx, type_expr)
            result.append(field_type)
    return result


def extract_type(ctx: TypeContext, value) -> ast.types.Type:
    match value:
        case (
            ast.IdExpr(value=ast.types.Stdlib.Type)
            | ast.InstantiateExpr(expr=ast.IdExpr(value=ast.types.Stdlib.Type))
        ):
            assert value.type
            return value.type
        case ast.Expr():
            return extract_type(ctx, value.type)
        case ast.types.Stdlib.Type:
            return ctx[value].type
        case str():
            return extract_type(ctx, ctx[value].type)
        case ast.types.Type():
            result = value
            while result == ast.types.Stdlib.Type:
                result = result.require_cls[0]
            return result
        case _:
            raise TypecheckError("expected a type, expression, or canonical name")


def extract_class_type(ctx: TypeContext, value) -> ast.types.Class:
    cls = extract_type(ctx, value).cls
    assert cls, "bad class"
    return cls


def is_unbound(value: ast.types.Type | ast.Expr) -> bool:
    typ = value.type if isinstance(value, ast.Expr) else value
    return typ is not None and typ.unbound is not None


def has_overloads(ctx: TypeContext, root: str) -> bool:
    overloads = ctx.cache.overloads.get(root)
    return bool(overloads) and len(overloads) > 1


def get_overloads(ctx: TypeContext, root: str):
    return ctx.cache.overloads[root]


def get_unmangled_name(ctx: TypeContext, name: str) -> str:
    if name in ctx.cache.reverse_identifier_lookup:
        return ctx.cache.rev(name)
    return name


def get_user_facing_name(ctx: TypeContext, name: str) -> str:
    result = get_unmangled_name(ctx, name)
    result = result.removeprefix("$")
    return result


def get_class(ctx: TypeContext, value: str | ast.types.Type) -> cache.ClassData | None:
    if isinstance(value, ast.types.Type):
        name = value.require_cls.name
    else:
        name = value
    return ctx.cache.classes.get(name)


def get_function(ctx: TypeContext, value: str | ast.types.Type) -> cache.FunctionData | None:
    if isinstance(value, ast.types.Type):
        name = value.require_func.func_name
    else:
        name = value
    return ctx.cache.functions.get(name)


def get_class_realization(ctx: TypeContext, typ: ast.types.Type) -> cache.ClassData.Realization:
    assert typ.can_realize(), "bad class"
    cls_data = get_class(ctx, typ)
    assert cls_data, "bad class"
    cls_obj = typ.require_cls
    realization = cls_data.realizations.get(cls_obj.realized_name())
    assert realization, f"bad class realization: {typ!r}"
    return realization


def get_root_name(ctx: TypeContext, typ: ast.types.Function) -> str:
    fn = ctx.cache.functions.get(typ.func_name)
    assert fn and fn.root_name, "bad function"
    return fn.root_name


def is_type_expr(expr: ast.Expr | None):
    match expr:
        case ast.Expr(type=ast.types.Class(name=ast.types.Stdlib.Type)):
            return True
        case _:
            return False


def is_function_expr(expr: ast.Expr | None, fn_name: str = ""):
    match expr:
        case ast.Expr(type=ast.types.Function(ast=ast.FunctionStmt(name))):
            return name == fn_name if fn_name else True
        case ast.Expr(type=ast.types.Function()):
            return not fn_name
        case _:
            return False


def get_import_module(ctx: TypeContext, name: str) -> cache.Import:
    module = ctx.cache.imports.get(name)
    assert module, "bad import"
    return module


def is_dispatch(value: str | ast.FunctionStmt | ast.types.Type):
    if isinstance(value, str):
        return value.endswith(cache.FN_DISPATCH_SUFFIX)
    if isinstance(value, ast.FunctionStmt):
        return value.name.endswith(cache.FN_DISPATCH_SUFFIX)
    typ = value.func
    return typ and typ.ast and typ.ast.name.endswith(cache.FN_DISPATCH_SUFFIX)


def is_dispatch_stmt(stmt: ast.FunctionStmt | None):
    return stmt and is_dispatch(stmt.name)


def is_dispatch_type(typ: ast.types.Type):
    return typ.func and is_dispatch_stmt(typ.func.ast)


def is_heterogenous(ctx: TypeContext, typ: ast.types.Class):
    if not typ or not typ.is_tuple:
        return False
    fields = []
    if typ == ast.types.Stdlib.Tuple:
        fields = [g.type for g in typ.generics if g.type]
    else:
        fields = get_class_field_types(ctx, typ)
    if len(fields) > 1:
        first = fields[0].realized_name()
        for field in fields[1:]:
            if field.realized_name() != first:
                return True
    return False


def add_class_generics(
    ctx: TypeContext,
    typ: ast.types.Class,
    function: bool = False,
    only_mangled: bool = False,
    instantiate: bool = False,
) -> Set[str]:
    added: Set[str] = set()

    def add_generic(generic: ast.types.Generic):
        typ = generic.type
        if instantiate:
            if isinstance(typ, ast.types.Link) and typ.kind is ast.types.Link.Kind.Generic:
                typ = ast.types.Link(kind=ast.types.Link.Kind.Unbound, src=typ)
        assert generic.static_kind is ast.types.Type.Behaviour.Runtime or not typ.is_runtime
        if generic.static_kind is ast.types.Type.Behaviour.Runtime and not typ.is_runtime:
            typ = instantiate_type_var(ctx, typ)
        name = generic.name if only_mangled else get_unmangled_name(ctx, generic.name)
        value = ctx.add_item(name, generic.name, typ)
        added.add(name)
        if name != generic.name:
            added.add(generic.name)
        value.generic = True

    if function and typ.func:
        parent = typ.func.func_parent
        while parent is not None:
            if isinstance(parent, ast.types.Function):
                # Add parent function generics
                for generic in parent.func_generics:
                    add_generic(generic)
                parent = parent.func_parent
            elif isinstance(parent, ast.types.Class):
                # Add parent class generics
                for generic in parent.hidden_generics:
                    add_generic(generic)
                for generic in parent.generics:
                    add_generic(generic)
                break
            else:
                assert False, f"not a class: {parent}"
        for generic in typ.func.func_generics:
            add_generic(generic)
    else:
        for generic in typ.hidden_generics:
            add_generic(generic)
        for generic in typ.generics:
            add_generic(generic)
    return added


def instantiate_type_var(ctx: TypeContext, typ: ast.types.Type) -> ast.types.Type:
    return instantiate(ctx, ctx[ast.types.Stdlib.Type].type, [typ])


def register_global(ctx: TypeContext, name: str):
    if name not in ctx.cache.globals:
        ctx.cache.globals[name] = None


def get_stdlib_type(ctx: TypeContext, type_name: str) -> ast.types.Class:
    module = get_import_module(ctx, cache.STDLIB_IMPORT)
    typ = module.ctx[type_name].type
    if type_name == ast.types.Stdlib.Type:
        return typ.require_cls
    else:
        return extract_class_type(ctx, typ)


def extract_func_generic(typ: ast.types.Type, idx: int = 0) -> ast.types.Type:
    assert isinstance(typ, ast.types.Function)
    return typ.func_generics[idx].type


def get_class_method(ctx: TypeContext, typ: ast.types.Type, member: str) -> str:
    if class_data := get_class(ctx, typ):
        if method := class_data.methods.get(member):
            return method
    assert False, f"cannot find '{member}' in {typ}"


def get_temporary_var(ctx: TypeContext, prefix: str) -> str:
    return ctx.cache.get_temporary_var(prefix)


def get_param_type(typ: ast.types.Type | None) -> ast.Expr | None:
    if typ is None:
        return None
    if typ == ast.types.Stdlib.Type:
        return ast.IdExpr(ast.types.Stdlib.Type)
    if not typ.is_runtime:
        return ast.IndexExpr(
            ast.IdExpr("Literal"),
            ast.IdExpr(str(ast.types.Type.Behaviour(typ.static_kind))),
        )
    return None


def has_side_effect(expr: ast.Expr):
    # TODO: What if StringExpr has a nested value as a f-string?
    match expr:
        case ast.IdExpr:
            return False
        case ast.DotExpr(expr=ast.IdExpr()):
            return False
        case ast.NoneExpr | ast.BoolExpr | ast.IntExpr | ast.FloatExpr | ast.StringExpr:
            return False
        case ast.EllipsisExpr | ast.YieldExpr:
            return False
        case ast.InstantiateExpr:
            return False
        case _:
            return True


def get_head_expr(expr: ast.Expr) -> ast.Expr:
    result = expr
    while isinstance(result, ast.StmtExpr):
        result = result.expr
    return result


def is_import_fn(name: str):
    return name.startswith("%_import_")


def get_underlying_static_type(ctx: TypeContext, typ: ast.types.Type) -> ast.types.Type:
    if literal := typ.literal:
        return literal.runtime_type
    if not typ.is_runtime:
        return get_stdlib_type(ctx, str(ast.types.Type.Behaviour(typ.static_kind)))
    return typ


def instantiate_unbound(
    ctx: TypeContext, info: ast.Node.SrcInfo | None = None, level: int | None = None
) -> ast.types.Link:
    """Create an unbound type with the provided typechecking level."""
    identifier = ctx.cache.unbound_count
    ctx.cache.unbound_count += 1
    return ast.types.Link(
        cache=ctx.cache,
        src_info=info or ctx.info,
        kind=ast.types.Link.Kind.Unbound,
        id=identifier,
        level=level or ctx.typecheck_level,
    )


def instantiate[T: ast.types.Type](
    ctx: TypeContext,
    root: T | str,
    generics: List[ast.types.Type] | ast.types.Class | None = None,
    info: ast.Node.SrcInfo | None = None,
) -> T:
    """
    Call `type->instantiate`.
    Prepare the generic instantiation table with the given a generic param.
    Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
    T=int.
    """

    typ = get_stdlib_type(ctx, root) if isinstance(root, str) else root.require_cls

    instantiate_ctx = ast.types.Type.InstantiateContext(ctx)

    cls_type = None
    if isinstance(generics, List):
        if len(generics) != len(typ.generics):
            raise TypeError(
                f"generic mismatch for "
                f"{get_user_facing_name(ctx, typ.name)}: "
                f"expected {len(typ.generics)}, got {len(generics)}"
            )
        cls_type = ast.types.Class(cache=ctx.cache, name="")
        for idx, generic_type in enumerate(generics):
            generic = typ.generics[idx]
            assert generic.type is not None, "generic is null"
            if generic.is_runtime and generic.type.literal:
                generic_type = generic.type.literal.runtime_type
            cls_type.generics.append(
                ast.types.Generic(generic.name, generic_type, generic.id, generic.static_kind)
            )
    elif isinstance(generics, ast.types.Class):
        cls_type = generics

    if cls_type:
        for generic in [*cls_type.hidden_generics, *cls_type.generics]:
            if generic.type is None:
                continue
            if not ((link := generic.type.link) and link.kind is ast.types.Link.Kind.Generic):
                instantiate_ctx.cache[generic.id] = generic.type
        if (fn := typ.func) and fn.func_generics:
            self_generic = fn.func_generics[0]
            if get_unmangled_name(ctx, self_generic.name) == "__SELF__":
                instantiate_ctx.cache[self_generic.id] = cls_type

    instantiated = typ.instantiate(ctx.typecheck_level, instantiate_ctx)
    for value in instantiate_ctx.cache.values():
        if isinstance(value, ast.types.Link):
            value.info = info or ctx.info
            if value.default_type and ctx.base:
                ctx.base.pending_defaults.setdefault(0, set()).add(value)
    return cast(T, instantiated)


def find_method(
    ctx: TypeContext, typ: ast.types.Class | None, method: str, hide_shadowed: bool = True
) -> List[ast.types.Function]:
    """Returns the list of generic methods that correspond to typeName.method."""
    result: List[ast.types.Function] = []
    signature_loci: Set[str] = set()

    def populate(class_data: cache.ClassData):
        root = class_data.methods.get(method)
        if root is None:
            return
        methods = get_overloads(ctx, root)
        for exact_method in reversed(methods):
            fn_data = get_function(ctx, exact_method)
            if is_dispatch(exact_method) or fn_data is None or fn_data.type is None:
                continue
            if hide_shadowed:
                assert fn_data.ast
                signature = fn_data.ast.get_signature()
                if signature not in signature_loci:
                    signature_loci.add(signature)
                    result.append(fn_data.type)
            else:
                result.append(fn_data.type)

    if typ and typ == ast.types.Stdlib.Tuple and method == "__new__" and typ.generics:
        generate_tuple(ctx, len(typ.generics))
        tuple_class = get_class(ctx, ast.types.Stdlib.Tuple)
        if tuple_class:
            populate(tuple_class)
        for function_type in result:
            if len(function_type.generics) == len(typ.generics):
                return [function_type]
        return []

    if cache_class := None if typ is None else get_class(ctx, typ):
        for parent in cache_class.mro:
            parent_name = ast.types.Stdlib.Tuple if parent.name == "__NTuple__" else parent.name
            if method_class := get_class(ctx, parent_name):
                populate(method_class)
    return result


def find_member(
    ctx: TypeContext, typ: ast.types.Class, member: str
) -> cache.ClassData.Field | None:
    """
    Returns the generic type of typeName.member, if it exists (nullptr otherwise).
    Special cases: __elemsize__ and __atomic__.
    """

    class_data = get_class(ctx, typ)
    if class_data is None:
        return None
    for parent_class in class_data.mro:
        method_class = get_class(ctx, parent_class)
        if method_class is None:
            continue
        for idx, class_field in enumerate(method_class.fields):
            if parent_class == ast.types.Stdlib.Tuple and idx >= len(typ.generics):
                break
            if class_field.name == member:
                return class_field
    return None


def get_base_classes(ctx: TypeContext, typ: ast.types.Class) -> List[ast.types.Type]:
    """Return list of instantiated base classes for a given type."""
    class_data = get_class(ctx, typ)
    assert class_data
    bases: List[ast.types.Type] = []
    for base in class_data.mro:
        bases.append(instantiate(ctx, base, typ))
    return bases


class ReorderError(ast.NodeError):
    pass


def reorder_named_args(
    ctx: TypeContext,
    fn: ast.types.Function,
    args: List[ast.CallExpr.Arg],
    known: List[ast.types.Class.Flag],
):
    """
    Reorders a given vector or named args (consisting of names and the
    corresponding types) according to the signature of a given function.
    Returns the reordered vector and an associated reordering score (missing
    default args' score is half of the present args).
    Score is -1 if the given args cannot be reordered.
    @param known Bitmask that indicated if an arg is already provided
    (partial function) or not.

    See https://docs.python.org/3.6/reference/expressions.html#calls for details.
    Final score:
    - +1 for each matched arg
    -  0 for *args/**kwargs/default args
    - -1 for failed match
    0. Find *args and **kwargs
    True if there is a trailing ellipsis (full partial: fn(all_args, ...))
    1. Assign positional args to slots
    Each slot contains a list of arg's indices
    keep the map---we need it sorted!
    2. Assign named args to slots
    3. Fill in *args, if present
    4. Fill in **kwargs, if present
    5. Fill in the default args
    """

    score = 0
    partial = bool(
        args
        and (not args[-1].name)
        and isinstance(args[-1].value, ast.EllipsisExpr)
        and args[-1].value.is_partial()
    )

    star_idx = -1
    keyword_star_idx = -1
    for idx, param in enumerate(fn.ast.items):
        if param.name.startswith("**"):
            keyword_star_idx = idx
            score -= 2
        elif param.name.startswith("*"):
            star_idx = idx
            score -= 2

    slots = [[] for _ in fn.ast.items]
    extra: List[int] = []
    named_args: Dict[str, int] = {}
    extra_named_args: Dict[str, int] = {}
    slot_idx = 0
    assert not known or len(fn.ast.items) == len(known), "bad 'known' string"
    for arg_idx, arg in enumerate(args[: -int(partial)]):
        if not arg.name:
            while (
                known and slot_idx < len(slots) and known[slot_idx] == ast.types.Class.Flag.Included
            ):
                slot_idx += 1
            if slot_idx < len(slots) and (star_idx == -1 or slot_idx < star_idx):
                slots[slot_idx] = [arg_idx]
                slot_idx += 1
            else:
                extra.append(arg_idx)
        else:
            named_args[arg.name] = arg_idx
    score += 2 * (len(slots) - len(fn.func_generics))
    for variadic_idx in [
        max(star_idx, keyword_star_idx),
        min(star_idx, keyword_star_idx),
    ]:
        if variadic_idx != -1 and slots[variadic_idx]:
            extra.insert(0, variadic_idx)
            slots[variadic_idx].clear()
    if named_args:
        slot_names: Dict[str, int] = {}
        for idx, param in enumerate(fn.ast.items):
            if not known or known[idx] != ast.types.Class.Flag.Included:
                _, name = param.get_name_with_stars()
                slot_names[get_unmangled_name(ctx, name)] = idx
        for name, arg_idx in sorted(named_args.items()):
            if name not in slot_names:
                extra_named_args[name] = arg_idx
            elif not slots[slot_names[name]]:
                slots[slot_names[name]].append(arg_idx)
            else:
                value = args[arg_idx].value
                raise ReorderError(
                    value.info if value else ctx.info, f"keyword argument '{name}' repeated"
                )
    if extra and star_idx == -1:
        raise ReorderError(
            ctx.info,
            f"{get_user_facing_name(ctx, fn.func_name)}() takes {len(fn.ast)} arguments "
            f"({len(args) - int(partial)} given)",
        )
    if star_idx != -1:
        slots[star_idx] = extra
    if extra_named_args and keyword_star_idx == -1:
        invalid_name = min(extra_named_args)
        value = args[extra_named_args[invalid_name]].value
        raise ReorderError(
            value.info if value else ctx.info,
            f"'{next(iter(extra_named_args))}' is an invalid keyword argument for {get_user_facing_name(ctx, fn.func_name)}()",
        )
    if keyword_star_idx != -1:
        for name in sorted(extra_named_args):
            slots[keyword_star_idx].append(extra_named_args[name])
    for idx, param in enumerate(fn.ast.items):
        if not slots[idx] and idx not in (star_idx, keyword_star_idx):
            if param.is_value() and (
                param.default or (known and known[idx] == ast.types.Class.Flag.Included)
            ):
                score -= 2
            elif param.name.startswith("$"):
                score -= 2
            elif not partial and param.is_value():
                _, missing_name = param.get_name_with_stars()
                raise ReorderError(
                    ctx.info,
                    f"{get_unmangled_name(ctx, fn.func_name)}() missing 1 required positional argument: "
                    f"'{get_unmangled_name(ctx, missing_name)}'",
                )
    return score, (star_idx, keyword_star_idx, slots, partial)
    # done_score = on_done(star_idx, keyword_star_idx, slots, partial)
    # return score + done_score if done_score != -1 else -1


def is_canonical_name(name: str):
    return "." in name


def extract_function(typ: ast.types.Type) -> ast.types.Function | None:
    if isinstance(typ, ast.types.Function):
        return typ
    if partial := typ.partial:
        return partial.partial_func
    return None


def find_typecheck_errors(ctx: TypeContext, node: ast.Stmt) -> ParserErrors:
    @dataclass
    class UnfinishedVisitor(ast.NodeVisitor):
        unfinished: List[ast.Node]

        def __init__(self):
            self.unfinished = []

        def visit(self, node):
            if node and not node.done:
                self.unfinished.append(node)
            else:
                super().visit(node)

    v = UnfinishedVisitor()
    v.visit(node)
    errors = []
    for unfinished_node in v.unfinished:
        content = ctx.cache.get_content(unfinished_node.info)
        message = "cannot typecheck " + (repr(content) if content else "expression")
        errors.append(ErrorMessage(message, unfinished_node.info))
    return ParserErrors(errors)


@contextmanager
def with_class_generics(
    ctx: TypeContext,
    typ: ast.types.Class,
    func: bool = False,
    only_mangled: bool = False,
    instantiate: bool = False,
):
    # do not remove stuff that was added in the meantime, potentially by AssignExpr
    ctx.add_block()
    added = add_class_generics(ctx, typ, func, only_mangled, instantiate)
    yield
    add_later = [(name, ctx[name]) for name in ctx.get_block() if name not in added]
    ctx.pop_block()
    for name, item in add_later:
        ctx.add(name, item)


def instantiate_static(ctx: TypeContext, value) -> ast.types.Literal:
    if isinstance(value, bool):
        return ast.types.BoolLiteral(cache=ctx.cache, value=value)
    if isinstance(value, int):
        return ast.types.IntLiteral(cache=ctx.cache, value=value)
    if isinstance(value, str):
        return ast.types.StrLiteral(cache=ctx.cache, value=value)
    raise TypeError("unsupported static value")


def warning(message: str, info: ast.Node.SrcInfo):
    print(f"{info.file}:{info.line}: warning: {message}")


def get_mro(ctx: TypeContext, class_type: ast.types.Class | None) -> List[ast.types.Type]:
    """
    Get the list that describes the inheritance hierarchy of a given type.
    The first type in the list is the most recently inherited type.
    """

    result = []
    if not class_type:
        return result
    cls_data = get_class(ctx, class_type)
    assert cls_data
    for uninstantiated in cls_data.mro:
        instantiated = instantiate(ctx, uninstantiated, class_type)
        # ensure that parent types are realized
        infer.realize(ctx, instantiated)
        result.append(instantiated)
    return result
