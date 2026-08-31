# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ....bridge import Callable, Dict, List, Set, Tuple, cast, contextmanager, dataclass
from ... import ast, cache, error
from . import TypecheckError, TypeVisitor, infer
from .classes import generate_tuple
from .ctx import TypeContext


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


def find_best_method(
    tc: TypeVisitor, typ: ast.types.Class, member: str, args
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
        elif isinstance(arg, Tuple[str, ast.types.Type]):
            call_args.append(ast.CallExpr.Arg(name=arg[0], value=ast.NoneExpr(type=arg[1])))
    methods = find_method(typ, member, hide_shadowed=False)
    matches = find_matching_methods(typ, methods, call_args)
    return matches[0] if matches else None


def can_call(
    tc: TypeVisitor,
    function: ast.types.Function,
    args: List[ast.CallExpr.Arg],
    partial: ast.types.Class | None = None,
) -> int:
    """
    Check if a function can be called with the given args.
    See @c reorderNamedArgs for details.
    """
    partial_args = []
    if partial and partial.get_partial():
        known = partial.get_partial_mask()
        known_arg_types = partial[1].get_class()
        known_idx = 0
        for flag in known:
            if flag == ast.types.Class.Flag.Included:
                partial_args.append(known_arg_types[known_idx])
                known_idx += 1
            elif flag == ast.types.Class.Flag.Default:
                known_idx += 1

    reordered: List[Tuple[ast.types.Type | None, int]] = []
    non_inferrable = function.ast.get_non_inferrable_generics()

    def on_done(
        star_idx: int,
        keyword_star_idx: int,
        slots: List[List[int]],
        is_partial: bool,
    ) -> int:
        generic_idx = 0
        partial_idx = 0
        for slot_idx, slot in enumerate(slots):
            param = function.ast.items[slot_idx]
            if param.is_generic():
                if not slot:
                    if param.name in non_inferrable and not param.default_value:
                        return -1  # is this "real" type?
                    reordered.append((None, 0))
                else:
                    expected_type = extract_func_generic(function, generic_idx)
                    arg = args[slot[0]].value
                    if (
                        expected_type.get_static_kind() is ast.types.Type.Behaviour.Runtime
                        and not is_type_expr(arg)
                    ):
                        return -1
                    reordered.append((arg.type, slot[0]))
                generic_idx += 1
            elif slot_idx in {star_idx, keyword_star_idx} or len(slot) != 1:
                # Partials
                if (
                    not slot
                    and partial
                    and partial.get_partial()
                    and known[slot_idx] == ast.types.Class.Flag.Included
                ):
                    reordered.append((partial_args[partial_idx], 0))
                    partial_idx += 1
                else:
                    # Ignore *args, *kwargs and default args
                    reordered.append((None, 0))
            else:
                reordered.append((args[slot[0]].value.type, slot[0]))
        return 0

    score = reorder_named_args(function, args, on_done, lambda *_: -1, known)
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
            if expected_type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
                arg = args[source_idx].value
                # Check if this is a good generic!
                if arg.type.get_static_kind() is ast.types.Type.Behaviour.Runtime:
                    score = -1
                    break
                arg_type = arg.type
            else:
                arg_idx += 1
                # TODO: check if these are real types or if traits are satisfied
                continue
        _, wrapped_type, _ = can_wrap_expr(arg_type, expected_type, function)
        candidate = wrapped_type or arg_type
        if candidate.unify(expected_type, None) < 0:
            score = -1
        arg_idx += 1
    if score >= 0:
        score += int(real_generic_idx == len(function.func_generics))
    return score


def find_matching_methods(
    tc: TypeVisitor,
    typ: ast.types.Class,
    methods: List[ast.types.Function | None],
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
        instantiated = instantiate_type(method, typ)
        if (
            isinstance(instantiated, ast.types.Function)
            and can_call(instantiated, args, partial) != -1
        ):
            results.append(method)
    return results


def wrap_expr(
    tc: TypeVisitor,
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
    can_wrap, _, wrapper = can_wrap_expr(
        expr.type,
        expected_type,
        callee,
        allow_unwrap,
        isinstance(expr, ast.EllipsisExpr),
    )
    # TODO: get rid of this line one day!
    if expr.type.get_static_kind() is not ast.types.Type.Behaviour.Runtime and (
        expected_type is None or expected_type.get_static_kind() is ast.types.Type.Behaviour.Runtime
    ):
        expr.type = get_underlying_static_type(expr.type)
    if can_wrap and wrapper:
        expr = tc.visit(wrapper(expr))
    return can_wrap, expr


def can_wrap_expr(
    tc: TypeVisitor,
    expr_type: ast.types.Type,
    expected_type: ast.types.Type | None,
    callee: ast.types.Function | None = None,
    allow_unwrap: bool = True,
    is_ellipsis: bool = False,
):
    expected_class = expected_type.get_class() if expected_type else None
    expr_class = expr_type.get_class()
    wrapped_type = None
    wrapper: Callable[[ast.Expr], ast.Expr] | None = None
    if (
        callee
        and isinstance(callee.ast, ast.FunctionStmt)
        and callee.ast.has_function_attribute(
            ast.types.mangle("std.internal.attributes", func="no_arg_wrap")
        )
    ):
        return True, expected_type, None  # do not wrap

    # Case: types are wrapped in TypeWrap when type is not expected
    if callee and expr_type.name == ast.types.Stdlib.Type:
        expr_type = extract_class_type(expr_type)
        if not expr_type:
            return False, None, None
        if not (expected_class and expected_class.name == ast.types.Stdlib.Type):
            wrapped_type = infer.instantiate_type(
                get_stdlib_type(ast.types.Stdlib.TypeWrap), [expr_type]
            )
            wrapper = lambda value: ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.TypeWrap),
                items=[value],
            )
        return True, wrapped_type, wrapper
    if (
        expected_type is None or expected_type.get_static_kind() is ast.types.Type.Behaviour.Runtime
    ) and expr_type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
        expr_type = get_underlying_static_type(expr_type)
        expr_class = expr_type.get_class()
        wrapped_type = expr_type

    hints = {ast.types.Stdlib.Generator, ast.types.Stdlib.Float, ast.types.Stdlib.Optional, "pyobj"}
    if expr_class is None and expected_class and expected_class.name in hints:
        return False, None, None  # arg type not yet known.

    # Use Capsule ONLY if the type signature explicitly asks for it!
    if (
        allow_unwrap
        and expected_class
        and expected_class.name == ast.types.Stdlib.Capsule
        and expr_class
        and expr_class.name != ast.types.Stdlib.Capsule
    ):
        wrapped_type = infer.instantiate_type(
            get_stdlib_type(ast.types.Stdlib.Capsule), [expr_class]
        )

        def wrap_capsule(value: ast.Expr) -> ast.Expr:
            match value:
                case ast.CallExpr(expr, items=[ast.CallExpr.Arg(value)]) if (
                    is_function_expr(expr, ast.types.mangle(cls="Capsule", func="_get")) and value
                ):
                    # Do not wrap already wrapped vars
                    return value.items[0].value
                case _:
                    return ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Capsule", func="_make")),
                        items=[value],
                    )

        wrapper = wrap_capsule
    elif (
        expected_class
        and expected_class.name != ast.types.Stdlib.Any
        and expr_class
        and expr_class.name == ast.types.Stdlib.Any
    ):
        wrapped_type = expected_class

        def unwrap_any(value: ast.Expr) -> ast.Expr:

            realized = infer.realize(expected_class)
            return ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.OptionalUnwrap),
                items=[
                    value,
                    ast.IdExpr(realized.realized_name()),
                ],
            )

        wrapper = unwrap_any
    elif (
        expected_class
        and expected_class.name == (ast.types.Stdlib.Any)
        and expr_class
        and expr_class.name != (ast.types.Stdlib.Any)
    ):
        wrapped_type = expected_class
        wrapper = lambda value: ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Any), items=[value])
    elif (
        expected_class
        and expected_class.name == ast.types.Stdlib.Generator
        and expr_class
        and expr_class.name != expected_class.name
        and not is_ellipsis
    ):
        if not find_method(expr_class, "__iter__"):
            # Do not wrap already wrapped vars
            return False, None, None
        # Note: do not do this in pipelines (TODO: why?)
        wrapped_type = instantiate_type(expected_class)
        wrapper = lambda value: ast.CallExpr(ast.DotExpr(value, member="__iter__"), items=[])
    elif (
        expected_class
        and expected_class.name == "float"
        and expr_class
        and expr_class.name == "int"
    ):
        wrapped_type = instantiate_type(expected_class)
        wrapper = lambda value: ast.CallExpr(ast.IdExpr("float"), items=[value])
    elif (
        callee is None
        and expected_class
        and expected_class.name == ast.types.Stdlib.Bool
        and expr_class
        and expr_class.name != ast.types.Stdlib.Bool
    ):
        # Do not do this in function calls---only use for if-else wrapping
        wrapped_type = instantiate_type(expected_class)
        wrapper = lambda value: ast.CallExpr(ast.DotExpr(value, member="__bool__"), items=[])
    elif (
        expected_class
        and expected_class.name == ast.types.Stdlib.Optional
        and expr_class
        and expr_class.name != ast.types.Stdlib.Optional
    ):
        expected_inner = expected_class[0]
        _, inner_type, inner_wrapper = can_wrap_expr(
            tc, expr_class, expected_inner, callee, allow_unwrap, is_ellipsis
        )
        wrapped_type = instantiate_type(
            get_stdlib_type(ast.types.Stdlib.Optional),
            [expr_class if inner_type is None else inner_type],
        )
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.Optional),
            items=[value if inner_wrapper is None else inner_wrapper(value)],
        )
    elif (
        allow_unwrap
        and expected_class
        and expr_class
        and expr_class.name == ast.types.Stdlib.Optional
        and expected_class.name != ast.types.Stdlib.Optional
    ):
        expression_inner = expr_class[0]
        _, inner_type, inner_wrapper = can_wrap_expr(
            tc, expression_inner, expected_class, callee, allow_unwrap, is_ellipsis
        )
        wrapped_type = instantiate_type(inner_type or expression_inner)
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.OptionalUnwrap),
            items=[inner_wrapper(value) if inner_wrapper else value],
        )
    elif (
        expected_class
        and expected_class.name == "pyobj"
        and expr_class
        and expr_class.name != "pyobj"
    ):
        if not find_method(expr_class, "__to_py__"):
            return False, None, None
        wrapped_type = instantiate_type(expected_class)
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr("pyobj"),
            items=[ast.CallExpr(ast.DotExpr(value, member="__to_py__"))],
        )
    elif (
        allow_unwrap
        and expected_class
        and expr_class
        and expr_class.is_type("pyobj")
        and not expected_class.is_type("pyobj")
    ):
        if not find_method(expected_class, "__from_py__"):
            return False, None, None
        wrapped_type = instantiate_type(expected_class)
        wrapper = lambda value: ast.CallExpr(
            ast.DotExpr(ast.IdExpr(expected_class.name, type=expected_type), member="__from_py__"),
            items=[ast.DotExpr(value, member="p")],
        )
    elif (
        expected_class
        and expected_class.is_type(ast.types.Stdlib.Callable)
        and expr_class
        and (
            expr_class.get_partial()
            or expr_type.get_func()
            or expr_class.is_type(ast.types.Stdlib.Function)
        )
    ):
        arg_types = []
        # Get list of args
        function_type: ast.types.Function | None = None

        if partial_class := expr_class.get_partial():
            instantiated_function = instantiate_type(partial_class.get_partial_func())
            function_type = instantiated_function.get_func()
            for idx, flag in enumerate(partial_class.get_partial_mask()):
                if flag != ast.types.Class.Flag.Included:
                    arg_type = function_type[idx]
                    arg_types.append(arg_type)
            return_type = function_type.get_ret_type()
        else:
            tuple_type = expr_class[0].get_class()
            for generic in tuple_type.generics:
                arg_types.append(generic.type)
            return_type = expr_class[1]

        expected_args = expected_class[0].get_class()
        if len(arg_types) != len(expected_args.generics):
            return False, None, None
        for idx, arg_type in enumerate(arg_types):
            expected_arg = expected_args[idx]
            if arg_type.unify(expected_arg) < 0:
                return False, None, None
        if return_type.unify(expected_class[1]) < 0:
            return False, None, None

        wrapped_type = expected_type

        def wrap_callable(value: ast.Expr) -> ast.Expr:
            value_class = value.type.get_class()
            expected_value_class = wrapped_type.get_class()

            value_arg_types = []
            if value_partial := value_class.get_partial():
                partial_fn = instantiate_type(value_partial.get_partial_func()).get_func()
                for idx, flag in enumerate(value_partial.get_partial_mask()):
                    if flag != ast.types.Class.Flag.Included:
                        value_arg_type = partial_fn[idx]
                        value_arg_types.append(value_arg_type)
                value_return_type = partial_fn.get_ret_type()
            else:
                value_tuple = value_class[0].get_class()
                for generic in value_tuple.generics:
                    value_arg_types.append(generic.type)
                value_return_type = value_class[1]

            callable_args = expected_value_class[0]
            for idx, arg_type in enumerate(value_arg_types):
                expected_arg = callable_args[idx]
                infer.unify(arg_type, expected_arg)
            infer.unify(value_return_type, expected_value_class[1])

            return_function: ast.Expr | None = None
            data_arg: ast.Expr | None = None
            data_type: ast.Expr | None = None
            if value_partial:
                realized_value = infer.realize(value_class, force=True)
                function_name = realized_value.realized_name()
                return_function = ast.IndexExpr(
                    ast.CallExpr(
                        ast.IndexExpr(
                            ast.IdExpr(ast.types.Stdlib.Ptr),
                            idx=ast.IdExpr(realized_value.realized_name()),
                        ),
                        items=[ast.IdExpr("data")],
                    ),
                    idx=ast.IntExpr(int_value=0),
                )
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            elif value.type.get_func():
                realized_value = infer.realize(value_class, force=True)
                function_name = realized_value.realized_name()
                return_function = ast.IdExpr(realized_value.get_func().realized_name())
                data_arg = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.CObj))
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            elif value_class.name == ast.types.Stdlib.Function:
                realized_value = infer.realize(value_class, force=True)
                function_name = realized_value.realized_name()
                return_function = ast.CallExpr(
                    ast.IdExpr(realized_value.realized_name()),
                    items=[ast.IdExpr("data")],
                )
                data_type = ast.IdExpr(ast.types.Stdlib.CObj)
            else:
                assert False, f"bad type: {value_class.debug_string(2)}"

            function_name = f".proxy.{function_name}"
            if not tc.ctx.find(function_name):
                proxy = ast.FunctionStmt(
                    function_name,
                    ret=None,
                    items=[
                        ast.Param("data", type=data_type),
                        ast.Param(
                            "args",
                            type=ast.IdExpr(callable_args.realized_name()),
                        ),
                    ],
                    suite=ast.SuiteStmt(
                        [
                            ast.ReturnStmt(
                                expr=ast.CallExpr(
                                    return_function,
                                    items=[ast.StarExpr(ast.IdExpr("args"))],
                                )
                            )
                        ]
                    ),
                )
                tc.visit(proxy)
            return ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.Callable),
                items=[ast.IdExpr(function_name), data_arg or value],
            )

        wrapper = wrap_callable
    elif (
        callee
        and expr_class
        and expr_type.get_func()
        and not (expected_class and expected_class.name == ast.types.Stdlib.Function)
    ):
        if expected_class:
            wrapped_type = instantiate_type(expected_class)
        # Create wrapper if needed
        function_name = expr_type.get_func().ast.name

        def wrap_raw_function(value: ast.Expr) -> ast.Expr:
            partial_call = ast.CallExpr(
                ast.IdExpr(function_name),
                items=[ast.EllipsisExpr(ast.EllipsisExpr.Kind.PARTIAL)],
            )
            if isinstance(value, ast.StmtExpr):
                return ast.StmtExpr(value.items, expr=partial_call)
            return partial_call

        wrapper = wrap_raw_function
    elif (
        expected_class
        and expected_class.name == ast.types.Stdlib.Function
        and expr_class
        and expr_class.get_partial()
        and expr_class.get_partial().is_partial_empty()
    ):
        wrapped_type = instantiate_type(expected_class)
        empty_function_name = expr_class.get_partial().get_partial_func().ast.name
        empty_function_type = instantiate_type(tc.ctx.force_find(empty_function_name).get_type())
        if wrapped_type.unify(empty_function_type) >= 0:
            wrapper = lambda value: ast.IdExpr(empty_function_name)
        else:
            wrapped_type = None
    elif (
        allow_unwrap
        and expr_class
        and expr_type.get_union()
        and expected_class
        and not expected_class.get_union()
    ):
        if not (realized_expected := infer.realize(expected_class)):
            return False, None, None
        realized_expression = infer.realize(expr_type)
        if not realized_expression or not realized_expression.get_union():
            return False, None, None
        union_types = realized_expression.get_union().get_realization_types()
        if any(item.unify(realized_expected) >= 0 for item in union_types):
            wrapped_type = realized_expected
            wrapper = lambda value: ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get")),
                items=[
                    value,
                    ast.IdExpr(realized_expected.realized_name()),
                ],
            )
    elif expr_class and expected_class and expected_class.get_union():
        if not (realized_expected := infer.realize(expected_class)):
            return False, None, None
        # Wrap raw Seq functions into Partial(...) call for easy realization.
        # Special case: Seq functions are embedded (via lambda!)
        if expected_class.unify(expr_class) == -1:
            wrapped_type = realized_expected
            wrapper = lambda value: ast.CallExpr(
                ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Union), member="_new"),
                items=[
                    value,
                    ast.IdExpr(realized_expected.realized_name()),
                ],
            )
    elif (
        expr_class
        and expr_class.name == ast.types.Stdlib.Type
        and expected_class
        and expected_class.name == ast.types.Stdlib.TypeWrap
    ):
        wrapped_type = instantiate_type(get_stdlib_type(ast.types.Stdlib.TypeWrap), [expr_class])
        wrapper = lambda value: ast.CallExpr(
            ast.IdExpr(ast.types.Stdlib.TypeWrap),
            items=[value],
        )
    elif expr_class and expected_class:
        source = expr_class
        destination = expected_class
        optional = source.is_type(ast.types.Stdlib.Optional) and destination.is_type(
            ast.types.Stdlib.Optional
        )
        if optional:
            source = source[0].get_class()
            destination = destination[0].get_class()
        if source and destination and source.name != destination.name:
            source_data = get_class(tc.ctx, source)
            # Cast derived classes to base classes
            for mro in source_data.mro[1:]:
                base = instantiate_type(mro, source)
                if base.unify(destination) >= 0:
                    infer.unify(base, destination)
                    wrapped_type = expected_class

                    def cast_base(value: ast.Expr, base_type: ast.types.Type = base) -> ast.Expr:
                        base_class = base_type.get_class()
                        type_expr = ast.IdExpr(
                            base_class.name, type=instantiate_type_var(base_class)
                        )
                        if optional:
                            type_expr = ast.InstantiateExpr(
                                ast.IdExpr(ast.types.Stdlib.Optional),
                                items=[type_expr],
                            )
                        return ast.CallExpr(
                            ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_cast")),
                            items=[
                                value,
                                type_expr,
                            ],
                        )

                    wrapper = cast_base
                    break
    return True, wrapped_type, wrapper


def unpack_tuple_types(tc: TypeVisitor, expr: ast.Expr) -> List[Tuple[str, ast.types.Type]] | None:
    """
    Unpack a Tuple or KwTuple expression into (name, type) vector.
    Name is empty when handling Tuple; otherwise it matches names of KwTuple.
    """
    result = []
    match expr.orig_expr or expr:
        case ast.TupleExpr(items):
            for idx, arg in enumerate(items):
                transformed = tc.visit(arg)
                if not isinstance(transformed, ast.Expr) or transformed.get_class_type() is None:
                    return None
                items[idx] = transformed
                result.append(("", transformed.type))
            return result
        case ast.CallExpr():
            value = extract_class_type(expr.type)
            tuple_values = value[1].get_class()
            if (
                value.name != ast.types.Stdlib.NamedTuple
                or not tuple_values
                or not value[0].can_realize()
            ):
                return None
            tuple_id = get_int_literal(value)
            assert 0 <= tuple_id < len(tc.ctx.cache.generated_tuple_names)
            names = tc.ctx.cache.generated_tuple_names[tuple_id]
            for idx in range(len(tuple_values.generics)):
                if not (item_type := tuple_values[idx]):
                    return None
                result.append((names[idx], item_type))
            return result
        case _:
            return None


def extract_named_tuple(ctx: TypeContext, expr: ast.Expr) -> List[Tuple[str, ast.Expr]]:
    class_type = expr.get_class_type()
    arg_type = class_type[0]
    assert arg_type.can_realize(), f"bad named tuple: {expr.to_string()}"
    tuple_id = get_int_literal(class_type)
    assert 0 <= tuple_id < len(ctx.cache.generated_tuple_names)
    names = ctx.cache.generated_tuple_names[tuple_id]
    return [
        (
            name,
            ast.IndexExpr(
                ast.DotExpr(expr, member="args"),
                idx=ast.IntExpr(int_value=idx),
            ),
        )
        for idx, name in enumerate(names)
    ]


def get_class_fields(cls: ast.types.Class) -> List[cache.ClassData.Field]:
    cache_class = get_class(cls.name)
    fields = [] if not cache_class else list(cache_class.fields)
    if cls.name == ast.types.Stdlib.Tuple:
        fields = fields[: len(cls.generics)]
    return fields


def get_class_field_types(tc: TypeVisitor, cls: ast.types.Class) -> List[ast.types.Type]:
    def collect() -> List[ast.types.Type]:
        result: List[ast.types.Type] = []
        for class_field in get_class_fields(cls):
            field_type = infer.instantiate_type(class_field.type, cls)
            if not field_type.can_realize() and class_field.type_expr is not None:
                cloned_type_expr = cast(ast.Expr, class_field.type_expr.clone(True))
                transformed = tc.visit(cloned_type_expr)
                extracted = extract_type(transformed)
                infer.unify(field_type, extracted)
            result.append(field_type)
        return result

    return with_class_generics(cls, collect)


def extract_type(ctx: TypeContext, value) -> ast.types.Type:
    match value:
        case ast.IdExpr(value) | ast.InstantiateExpr(expr=ast.IdExpr(value)) if (
            value == ast.types.Stdlib.Type
        ):
            return value.type
        case ast.Expr():
            return extract_type(value.type)
        case ast.types.Stdlib.Type:
            return ctx.force_find(value).get_type()
        case str():
            return extract_type(ctx.force_find(value).get_type())
        case ast.types.Type():
            result = value
            while result.get_class() and result.name == ast.types.Stdlib.Type:
                result = result[0]
            return result
        case _:
            raise TypecheckError("expected a type, expression, or canonical name")


def extract_class_type(ctx: TypeContext, value) -> ast.types.Class:
    cls = extract_type(ctx, value).get_class()
    assert cls, "bad class"
    return cls


def is_unbound(value: ast.types.Type | ast.Expr) -> bool:
    typ = value.type if isinstance(value, ast.Expr) else value
    return typ and typ.get_unbound()


def has_overloads(ctx: TypeContext, root: str) -> bool:
    overloads = ctx.cache.overloads.get(root)
    return overloads and len(overloads) > 1


def get_overloads(ctx: TypeContext, root: str):
    overloads = ctx.cache.overloads.get(root)
    assert overloads is not None, "bad root"
    return list(overloads)


def get_unmangled_name(ctx: TypeContext, name: str) -> str:
    if name in ctx.cache.reverse_identifier_lookup:
        return ctx.cache.rev(name)
    return name


def get_user_facing_name(ctx: TypeContext, name: str) -> str:
    result = get_unmangled_name(ctx, name)
    result = result.removeprefix("$")
    return result


def get_class(ctx: TypeContext, value: str | ast.types.Type) -> cache.Cache.ClassData | None:
    if isinstance(value, ast.types.Type):
        cls = value.get_class()
        assert cls, "bad class"
        name = cls.name
    else:
        name = value
    return ctx.cache.classes.get(name)


def get_function(ctx: TypeContext, value: str | ast.types.Type) -> cache.Cache.FunctionData | None:
    if isinstance(value, ast.types.Type):
        func = value.get_func()
        assert func, "bad function"
        name = func.get_func_name()
    else:
        name = value
    return ctx.cache.functions.get(name)


def get_class_realization(
    ctx: TypeContext, typ: ast.types.Type
) -> cache.Cache.ClassData.Realization:
    assert typ.can_realize(), "bad class"
    cache_class = get_class(ctx, typ)
    assert cache_class is not None, "bad class"
    cls_obj = typ.get_class()
    assert isinstance(cls_obj, ast.types.Class), "bad class"
    realization = cache_class.realizations.get(cls_obj.realized_name())
    assert realization is not None, f"bad class realization: {typ.debug_string(2)}"
    return realization


def get_root_name(ctx: TypeContext, typ: ast.types.Function) -> str:
    function = ctx.cache.functions.get(typ.get_func_name())
    assert function and function.root_name, "bad function"
    return function.root_name


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
    typ = value.get_func()
    return typ and typ.ast and typ.ast.name.endswith(cache.FN_DISPATCH_SUFFIX)


def is_dispatch_stmt(stmt: ast.FunctionStmt | None):
    return stmt and is_dispatch(stmt.name)


def is_dispatch_type(typ: ast.types.Type):
    typ = typ.get_func()
    return isinstance(typ, ast.types.Function) and is_dispatch_stmt(typ.ast)


def is_heterogenous(ctx: TypeContext, typ: ast.types.Type):
    typ = typ.get_class()
    if not typ or not typ.is_record():
        return False
    fields = []
    if typ.name == ast.types.Stdlib.Tuple:
        fields = [g.type for g in typ.generics if g.type]
    else:
        fields = get_class_field_types(ctx)
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
        generic_type = generic.type
        if instantiate:
            link = generic_type.get_link()
            if link and link.kind is ast.types.Link.Kind.Generic:
                generic_type = ast.types.Link(kind=ast.types.Link.Kind.Unbound, src=link)
        assert (
            generic.static_kind is ast.types.Type.Behaviour.Runtime
            or generic_type.get_static_kind() is not ast.types.Type.Behaviour.Runtime
        )
        if generic.static_kind is ast.types.Type.Behaviour.Runtime and not generic_type.is_type(
            ast.types.Stdlib.Type
        ):
            generic_type = instantiate_type_var(ctx, generic_type)
        name = generic.name if only_mangled else get_unmangled_name(ctx, generic.name)
        value = ctx.add_type(name, generic.name, generic_type)
        added.add(name)
        if name != generic.name:
            added.add(generic.name)
        value.generic = True

    typ = typ.get_func()
    if function and typ:
        parent = typ.func_parent
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
        for generic in typ.func_generics:
            add_generic(generic)
    else:
        for generic in typ.hidden_generics:
            add_generic(generic)
        for generic in typ.generics:
            add_generic(generic)
    return added


def instantiate_type_var(ctx: TypeContext, typ: ast.types.Type) -> ast.types.Type:
    root = ctx.force_find(ast.types.Stdlib.Type).get_type()
    return instantiate_type(ctx, root, [typ])


def register_global(ctx: TypeContext, name: str):
    if name not in ctx.cache.globals:
        ctx.cache.globals[name] = None


def get_stdlib_type(ctx: TypeContext, type_name: str) -> ast.types.Class:
    module = get_import_module(ctx, cache.STDLIB_IMPORT)
    typ = module.ctx.force_find(type_name).get_type()
    if type_name == ast.types.Stdlib.Type:
        return typ.get_class()
    else:
        return extract_class_type(ctx, typ)


def extract_func_generic(typ: ast.types.Type, idx: int = 0) -> ast.types.Type:
    assert isinstance(typ, ast.types.Function)
    return typ.func_generics[idx].type


def get_class_method(ctx: TypeContext, typ: ast.types.Type, member: str) -> str:
    if class_data := get_class(ctx, typ):
        if method := class_data.methods.get(member):
            return method
    assert False, f"cannot find '{member}' in '{typ.pretty_string()}'"


def get_temporary_var(ctx: TypeContext, prefix: str) -> str:
    return ctx.cache.get_temporary_var(prefix)


def get_str_literal(typ: ast.types.Type, position: int = 0) -> str:
    direct = typ.get_str_static()
    if isinstance(direct, ast.types.StrLiteral):
        return direct.value
    static = typ[position].get_str_static()
    assert isinstance(static, ast.types.StrLiteral), "not a string literal"
    return static.value


def get_int_literal(typ: ast.types.Type, position: int = 0) -> int:
    direct = typ.get_int_static()
    if isinstance(direct, ast.types.IntLiteral):
        return direct.value
    static = typ[position].get_int_static()
    assert isinstance(static, ast.types.IntLiteral), "not a int literal"
    return static.value


def get_bool_literal(typ: ast.types.Type, position: int = 0) -> bool:
    direct = typ.get_bool_static()
    if isinstance(direct, ast.types.BoolLiteral):
        return direct.value
    static = typ[position].get_bool_static()
    assert isinstance(static, ast.types.BoolLiteral), "not a bool literal"
    return static.value


def get_param_type(typ: ast.types.Type | None) -> ast.Expr | None:
    if typ is None:
        return None
    if typ.is_type(ast.types.Stdlib.Type):
        return ast.IdExpr(ast.types.Stdlib.Type)
    static_kind = typ.get_static_kind()
    if static_kind is not ast.types.Type.Behaviour.Runtime:
        return ast.IndexExpr(
            ast.IdExpr("Literal"),
            idx=ast.IdExpr(ast.types.Type.string_from_literal(static_kind)),
        )
    return None


def has_side_effect(expr: ast.Expr):
    # TODO: What if StringExpr has a nested value as a f-string?
    match expr:
        case ast.IdExpr:
            return False
        case ast.DotExpr(expr=ast.IdExpr):
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
    static_obj = typ.get_static()
    if isinstance(static_obj, ast.types.Literal):
        result = static_obj.get_non_static_type()
        return result
    static_kind = typ.get_static_kind()
    if static_kind is not ast.types.Type.Behaviour.Runtime:
        return get_stdlib_type(ctx, ast.types.Type.string_from_literal(static_kind))
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


def instantiate_type(
    ctx: TypeContext,
    root: ast.types.Type,
    generics: List[ast.types.Type] | ast.types.Class | None = None,
    info: ast.Node.SrcInfo | None = None,
) -> ast.types.Type:
    """
    Call `type->instantiate`.
    Prepare the generic instantiation table with the given a generic param.
    Example: when instantiating List[T].foo, generics=List[int].foo will ensure that
    T=int.
    """
    assert root is not None, "type is null"

    instantiate_ctx = ast.types.Type.InstantiateContext()
    if isinstance(generics, ast.types.Class):
        for generic in [*generics.hidden_generics, *generics.generics]:
            if generic.type is None:
                continue
            if not (
                isinstance(generic.type, ast.types.Link)
                and generic.type.kind is ast.types.Link.Kind.Generic
            ):
                instantiate_ctx.cache[generic.id] = generic.type
        if isinstance(root, ast.types.Function) and root.func_generics:
            self_generic = root.func_generics[0]
            if get_unmangled_name(ctx, self_generic.name) == "__SELF__":
                instantiate_ctx.cache[self_generic.id] = generics
    elif generics is not None:
        assert isinstance(root, ast.types.Class), "root class is null"
        if len(generics) != len(root.generics):
            raise TypeError(
                f"generic mismatch for "
                f"{get_user_facing_name(ctx, root.name)}: "
                f"expected {len(root.generics)}, got {len(generics)}"
            )
        dummy = ast.types.Class(cache=ctx.cache, name="")
        for idx, generic_type in enumerate(generics):
            generic = root.generics[idx]
            assert generic.type is not None, "generic is null"
            if (
                generic.static_kind is ast.types.Type.Behaviour.Runtime
                and generic_type.get_static()
            ):
                generic_type = generic_type.get_static().get_non_static_type()
            dummy.generics.append(
                ast.types.Generic(
                    type=generic_type,
                    id=generic.id,
                    static_kind=generic.static_kind,
                )
            )
        return instantiate_type(ctx, root, dummy, info)

    instantiate_ctx.next_unbound = ctx.cache.unbound_count
    instantiated = root.instantiate(ctx.typecheck_level, instantiate_ctx)
    ctx.cache.unbound_count = instantiate_ctx.next_unbound
    current_base = ctx.get_base()
    for value in instantiate_ctx.cache.values():
        if isinstance(value, ast.types.Link):
            value.info = info or ctx.info
            if value.default_type and current_base:
                current_base.pending_defaults.setdefault(0, set()).add(value)
    return instantiated


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
            function = get_function(ctx, exact_method)
            if is_dispatch(exact_method) or function is None or function.type is None:
                continue
            if hide_shadowed:
                signature = function.ast.get_signature()
                if signature not in signature_loci:
                    signature_loci.add(signature)
                    result.append(function.type)
            else:
                result.append(function.type)

    if typ and typ.is_type(ast.types.Stdlib.Tuple) and method == "__new__" and typ.generics:
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


def find_member(ctx: TypeContext, typ: ast.types.Class, member: str) -> cache.ClassData.Field | None:
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
            if parent_class.is_type(ast.types.Stdlib.Tuple) and idx >= len(typ.generics):
                break
            if class_field.name == member:
                return class_field
    return None


def get_base_classes(ctx: TypeContext, typ: ast.types.Class) -> List[ast.types.Type]:
    """Return list of instantiated base classes for a given type."""
    class_data = get_class(ctx, typ)
    bases: List[ast.types.Type] = []
    for base in class_data.mro:
        bases.append(instantiate_type(ctx, base, typ))
    return bases


class ReorderError(ast.NodeError):
    pass


def reorder_named_args(
    ctx: TypeContext,
    function: ast.types.Function,
    args: List[ast.CallExpr.Arg],
    known: str = "",
) -> int:
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
    for idx, param in enumerate(function.ast.items):
        if param.name.startswith("**"):
            keyword_star_idx = idx
            score -= 2
        elif param.name.startswith("*"):
            star_idx = idx
            score -= 2

    slots = [[] for _ in function.ast.items]
    extra: List[int] = []
    named_args: Dict[str, int] = {}
    extra_named_args: Dict[str, int] = {}
    slot_idx = 0
    assert not known or len(function.ast.items) == len(known), "bad 'known' string"
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
    score += 2 * (len(slots) - len(function.func_generics))
    for variadic_idx in [
        max(star_idx, keyword_star_idx),
        min(star_idx, keyword_star_idx),
    ]:
        if variadic_idx != -1 and slots[variadic_idx]:
            extra.insert(0, variadic_idx)
            slots[variadic_idx].clear()
    if named_args:
        slot_names: Dict[str, int] = {}
        for idx, param in enumerate(function.ast.items):
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
            f"{get_user_facing_name(function.ast.name)}() takes {len(function.ast)} arguments "
            f"({len(args) - int(partial)} given)",
        )
    if star_idx != -1:
        slots[star_idx] = extra
    if extra_named_args and keyword_star_idx == -1:
        invalid_name = min(extra_named_args)
        value = args[extra_named_args[invalid_name]].value
        raise ReorderError(
            value.info if value else ctx.info,
            f"'{extra_named_args.pop()[0]}' is an invalid keyword argument for {get_user_facing_name(function.ast.name)}()",
        )
    if keyword_star_idx != -1:
        for name in sorted(extra_named_args):
            slots[keyword_star_idx].append(extra_named_args[name])
    for idx, param in enumerate(function.ast.items):
        if not slots[idx] and idx not in (star_idx, keyword_star_idx):
            if param.is_value() and (
                param.default_value or (known and known[idx] == ast.types.Class.Flag.Included)
            ):
                score -= 2
            elif param.name.startswith("$"):
                score -= 2
            elif not partial and param.is_value():
                _, missing_name = param.get_name_with_stars()
                raise ReorderError(
                    ctx.info,
                    f"{get_unmangled_name(ctx, function.ast.name)}() missing 1 required positional argument: "
                    f"'{get_unmangled_name(ctx, missing_name)}'",
                )
    return score
    # done_score = on_done(star_idx, keyword_star_idx, slots, partial)
    # return score + done_score if done_score != -1 else -1


def is_canonical_name(name: str):
    return "." in name


def extract_function(typ: ast.types.Type) -> ast.types.Function | None:
    if isinstance(typ, ast.types.Function):
        return typ
    partial = typ.get_partial()
    if partial:
        result = typ.get_partial_func()
        return result if isinstance(result, ast.types.Function) else None
    return None


def find_typecheck_errors(ctx: TypeContext, node: ast.Stmt) -> error.ParserErrors:
    @dataclass
    class UnfinishedVisitor(ast.NodeVisitor):
        unfinished: List[ast.Node]

        def __init__(self):
            self.unfinished = []

        def visit(self, value):
            if value and not value.done:
                self.unfinished.append(value)
            else:
                super().visit(value)

    v = UnfinishedVisitor()
    v.visit(node)
    errors = []
    for unfinished_node in v.unfinished:
        content = ctx.cache.get_content(unfinished_node.info)
        message = "cannot typecheck " + (repr(content) if content else "expression")
        errors.append(error.ErrorMessage(message, unfinished_node.info))
    return error.ParserErrors.from_messages(errors)


@contextmanager
def with_class_generics(
    ctx: TypeContext,
    typ: ast.types.Class,
    function: Callable[[], object],
    func: bool = False,
    only_mangled: bool = False,
    instantiate: bool = False,
) -> object:
    # do not remove stuff that was added in the meantime, potentially by AssignExpr
    ctx.add_block()
    added = add_class_generics(ctx, typ, func, only_mangled, instantiate)
    yield
    add_later = [
        (name, ctx.force_find(name)) for name in ctx.get_block() if name not in added
    ]
    ctx.pop_block()
    for name, item in add_later:
        ctx.add(name, item)


def instantiate_static(ctx: TypeContext, value: object) -> ast.types.Literal:
    if isinstance(value, bool):
        return ast.types.BoolLiteral(cache=ctx.cache, value=value)
    if isinstance(value, int):
        return ast.types.IntLiteral(cache=ctx.cache, value=value)
    if isinstance(value, str):
        return ast.types.StrLiteral(cache=ctx.cache, value=value)
    raise TypeError("unsupported static value")


def warning(message: str, info: ast.Node.SrcInfo):
    print(f"{info.file}:{info.line}: warning: {message}")
