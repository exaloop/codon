# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ... import ast, cache
from ...error import TypecheckError
from .. import scope
from . import TypeContext, classes, utils
from .ctx import Base, Item


def infer_types(
    ctx: TypeContext, result: ast.Stmt | None, is_toplevel: bool = False
) -> ast.Stmt | None:
    """
    Infer all types within a Stmt *. Implements the LTS-DI typechecking.
    @param isToplevel set if typechecking the program toplevel.
    """
    if not result:
        return None

    base = ctx.base
    base.iteration = 1
    while True:
        if base.iteration >= 1000:
            source = "toplevel" if not base.name else utils.get_unmangled_name(ctx, base.name)
            raise TypecheckError(
                result,
                f"cannot typecheck '{source}' in reasonable time",
                trace=utils.find_typecheck_errors(ctx, result),
            )
        # Keep iterating until:
        # (1) success: the statement is marked as done; or
        # (2) failure: no expression or statements were marked as done during an
        # iteration (i.e., changedNodes is zero)
        with (
            ctx.substitute("typecheck_level", ctx.typecheck_level + 1),
            ctx.substitute("changed_nodes", 0),
            ctx.substitute("return_early", False),
        ):
            result = ctx.cache.typecheck(result, ctx=ctx)
            changed_nodes = ctx.changed_nodes
        if base.iteration == 1 and is_toplevel:
            # Realize all @force_realize functions
            # Copy keys to avoid modifications during the iteration (#768)
            for fn_name in list(ctx.cache.functions.keys()):
                fn_data = ctx.cache.functions[fn_name]
                if (
                    fn_data.type
                    and not fn_data.realizations
                    and fn_data.ast
                    and (
                        fn_data.ast.has(ast.Attr.ForceRealize)
                        or fn_data.ast.has(ast.Attr.Export)
                        or (fn_data.ast.has(ast.Attr.C) and not fn_data.ast.has(ast.Attr.CVarArg))
                    )
                ):
                    assert fn_data.type.can_realize(), f"cannot realize {fn_name}"
                    realize(ctx, utils.instantiate(ctx, fn_data.type))
                    assert fn_data.realizations, f"cannot realize {fn_name}"
        if result.done:
            break
        if changed_nodes:
            base.iteration += 1
            continue
        # Special case: nothing was changed, however there are unbound types that have
        # default values (e.g., generics with default values). Unify those types with
        # their default values and then run another round to see if anything changed.
        another_round = False
        # Special case: return type might have default as well
        if base.return_type:
            base.pending_defaults.setdefault(0, set()).add(base.return_type)
        for default_level in sorted(base.pending_defaults):
            # First unify "explicit" generics (whose default type is explicit),
            # then "implicit" ones (whose default type is compiler generated,
            # e.g. compiler-generated variable placeholders with default NoneType)
            unbounds = base.pending_defaults[default_level]
            for unbound in list(unbounds):
                if isinstance(unbound, ast.types.Link) and unbound.default_type:
                    undo = ast.types.Type.UnifyContext()
                    # type[] or generic
                    target = (
                        utils.extract_class_type(ctx, unbound.default_type)
                        if isinstance(unbound.default_type, ast.types.Class)
                        else unbound.default_type
                    )
                    if unbound.unify(target, undo) >= 0:
                        another_round = True
            unbounds.clear()
            if another_round:
                break
        if another_round:
            base.iteration += 1
            continue
        # Nothing helps. Return nullptr.
        return None
    return result


def realize[T](ctx: TypeContext, typ: T, force: bool = False) -> T | None:
    """
    Realize a type and create IR type stub. If type is a function type, also realize the
    underlying function and generate IR function stub.
    @return realized type or nullptr if the type cannot be realized
    """
    if not isinstance(typ, ast.types.Type):
        return None
    if not typ.can_realize():
        return None

    try:
        if isinstance(typ, ast.types.Function):
            if realized := realize_func(ctx, typ, force):
                # Realize Function[..] type as well
                class_type = ast.types.Class(
                    name=realized.name,
                    generics=list(realized.generics),
                    hidden_generics=list(realized.hidden_generics),
                    is_tuple=realized.is_tuple,
                    _cached_name=realized._cached_name,
                    cache=realized.cache,
                    info=realized.info,
                )
                realize_type(ctx, class_type)
                # Needed for return type unification
                ret_type = typ.ret_type
                ret_type |= realized[1]
                return realized  # type: ignore
        else:
            return realize_type(ctx, typ)  # type: ignore
    except TypecheckError as err:
        assert err.errors.errors
        backtrace = err.errors.errors[-1]
        if isinstance(typ, ast.types.Function):
            if typ.ast.has(ast.Attr.HiddenFromUser):
                backtrace.trace[-1].info = ctx.node_stack[-1].info
            else:
                arguments = []
                argument_idx = 0
                generic_idx = 0
                for parameter in typ.ast.items:
                    stars, name = parameter.get_name_with_stars()
                    if parameter.is_generic():
                        parameter_type = utils.extract_func_generic(typ, generic_idx)
                        generic_idx += 1
                    else:
                        parameter_type = typ[argument_idx]
                        argument_idx += 1
                    arguments.append(
                        f"{'*' * stars}{utils.get_user_facing_name(ctx, name)}: {parameter_type}"
                    )
                name = typ.ast.name
                name_arguments = ""
                if name.startswith("%_import_"):
                    for imported in ctx.cache.imports.values():
                        imported_var = ast.types.mangle(
                            func=f"{imported.import_var}_call", no_core=True
                        )
                        if imported_var == name:
                            name = imported.name
                            break
                    name = f"<import {name}>"
                else:
                    name = utils.get_user_facing_name(ctx, typ.ast.name)
                    name_arguments = f"({', '.join(arguments)})"
                backtrace.add(
                    f"during the realization of {name}{name_arguments}", ctx.node_stack[-1].info
                )
        else:
            backtrace.add(f"during the realization of {typ}", ctx.node_stack[-1].info)
        raise
    return None


def realize_type(ctx: TypeContext, typ: ast.types.Class) -> ast.types.Class | None:
    """
    Realize a type and create IR type stub.
    @return realized type or nullptr if the type cannot be realized
    """
    if not typ.can_realize():
        return None

    # Check if the type fields are all initialized
    # (sometimes that's not the case: e.g., `class X: x: List[X]`)

    # generalize generics to ensure that they do not get unified later!
    if typ == ast.types.Stdlib.UnrealizedType:
        typ.generics[0].type = typ[0].generalize(0)
    if typ == "__NTuple__":
        count = max(0, typ[0].require_int)
        generated_generics = []
        generated = utils.instantiate(
            ctx, classes.generate_tuple(ctx, count * len(typ[1].require_cls))
        )
        generic_idx = 0
        for _ in range(count):
            for item in typ[1].require_cls.generics:
                new_generic = generated.generics[generic_idx]
                new_generic.type |= item.type
                generated_generics.append(new_generic)
                generic_idx += 1
        typ.name = ast.types.Stdlib.Tuple
        typ.generics = generated_generics
        typ._cached_name = ""

    # Check if the type was already realized
    realized_name = typ.realized_name()
    cls_data = utils.get_class(ctx, typ)
    assert cls_data
    existing = cls_data.realizations.get(realized_name)
    if existing and existing.type:
        return existing.type
    if not cls_data.ast:
        return None
    fields = utils.get_class_fields(ctx, typ)
    field_types = utils.get_class_field_types(ctx, typ)
    if len(field_types) != len(fields):
        # not yet done!
        return None
    realized = typ
    if literal := typ.literal:
        # do not cache static but its root type!
        realized = literal.runtime_type

    # Realize generics
    if typ != ast.types.Stdlib.UnrealizedType:
        for generic in realized.generics:
            if not generic.type or not realize(ctx, generic.type):
                return None
            if isinstance(generic.type, ast.types.Function):
                return_type = generic.type.ret_type
                if return_type and not return_type.can_realize():
                    return None

    # Realizations should always be visible, so add them to the toplevel
    realized_name = typ.realized_name()
    generalized = realized.generalize(0)
    item = Item(realized_name, base="", module=ctx.module_name, typ=generalized)
    if generalized != ast.types.Stdlib.Type:
        item.type = utils.instantiate_type_var(ctx, realized)
    ctx.add_always_visible(item, True)

    realization = cache.ClassData.Realization(type=generalized)
    ctx.cache.class_realization_cnt += 1
    realization.id = ctx.cache.class_realization_cnt
    cls_data.realizations[realized_name] = realization
    for parent in cls_data.mro[1:]:
        # need to generalize it first because generics are
        # not yet generalized when parsing methods
        instantiated_parent = utils.instantiate(ctx, parent.generalize(0), realized)
        assert instantiated_parent.can_realize()
        realization.bases.append(instantiated_parent)

    # Create LLVM stub
    ir_type = make_ir_type(ctx, realized)
    # Realize fields
    ir_field_types = []
    names = []
    member_info = {}
    for idx, field_type in enumerate(field_types):
        if not realize(ctx, field_type):
            raise TypecheckError(
                field_type.info,
                f"type of attribute '{fields[idx].name}' of object '{realized}' cannot be inferred",
            )
        realization.fields.append((fields[idx].name, field_type))
        names.append(fields[idx].name)
        ir_field_types.append(make_ir_type(ctx, field_type.require_cls))
        member_info[fields[idx].name] = field_type.info

    # IR attributes
    if False:
        raise NotImplementedError
        if names and isinstance(ir_type, ast.ir.types.RefType):
            ir_type.get_contents().realize(ir_field_types, names)
            ir_type.set(ast.ir.Attr.Member, member_info)
            ir_type.get_contents().set(ast.ir.Attr.Member, member_info)
    return generalized


def realize_func(
    ctx: TypeContext, typ: ast.types.Function, force: bool = False
) -> ast.types.Function | None:
    fn_data = utils.get_function(ctx, typ)
    assert fn_data
    imported = utils.get_import_module(ctx, fn_data.module)
    existing = fn_data.realizations.get(typ.realized_name())
    if existing and not force:
        return existing.type

    old_ctx, ctx = ctx, imported.ctx
    try:
        if ctx.realization_depth > cache.MAX_REALIZATION_DEPTH:
            raise TypecheckError(
                ctx.node_stack[-1],
                "maximum realization depth reached during the realization of "
                f"'{utils.get_user_facing_name(ctx, typ.ast.name)}'",
            )
        is_import = utils.is_import_fn(typ.ast.name)
        if is_import:
            return _realize_func_body(ctx, typ, force, fn_data, existing, True)
        ctx.add_block()
        try:
            new_base = Base(name=typ.ast.name, type=typ, return_type=typ.ret_type)
            with (
                ctx.substitute("typecheck_level", ctx.typecheck_level + 1),
                ctx.substitute("bases", ctx.bases + [new_base]),
            ):
                for idx in range(len(ctx.bases) - 2, -1, -1):
                    if ctx.base_name.startswith(ctx.bases[idx].name):
                        ctx.base.parent = idx
                        break
                return _realize_func_body(ctx, typ, force, fn_data, existing, False)
        finally:
            ctx.pop_block()
    finally:
        ctx = old_ctx


def _realize_func_body(
    ctx: TypeContext,
    typ: ast.types.Function,
    force: bool,
    fn_data: cache.FunctionData,
    existing,
    is_import: bool,
) -> ast.types.Function | None:
    from ..scope import Bindings
    from . import special

    for generic in typ.generics:
        if generic.type:
            # Types might change after realization, fix it
            if isinstance(generic.type, ast.types.Class):
                realize_type(ctx, generic.type)

    # Clone the generic AST that is to be realized
    function_ast = typ.ast.clone(clean=True)
    if special_suite := special.generate_special_ast(ctx, typ):
        function_ast.suite = special_suite
    utils.add_class_generics(ctx, typ, True)
    base = ctx.base
    if base:
        base.func = function_ast

    # Internal functions have no AST that can be realized
    has_ast = function_ast.suite is not None and not function_ast.has(ast.Attr.Internal)
    if bindings := function_ast.attributes.get(ast.Attr.Bindings):
        assert isinstance(bindings, scope.Bindings)
        for captured, capture_type in bindings.captures.items():
            if capture_type is Bindings.Scope.Global:
                captured_item = ctx.get(captured)
                if not captured_item:
                    raise TypecheckError(function_ast, f"name '{captured}' is not defined")
                if not captured_item.is_global():
                    raise TypecheckError(function_ast, f"no binding for global '{captured}' found")
        for name, canonical in bindings.local_renames.items():
            ctx.add(name, ctx[canonical])

    arg_idx = 0
    generic_idx = 0
    for param in function_ast.items if has_ast else []:
        _, variable_name = param.get_name_with_stars()
        unmangled = utils.get_unmangled_name(ctx, variable_name)
        if param.is_value():
            arg_type = typ[arg_idx]
            arg_idx += 1
            is_static = bool(param.type) and bool(ast.get_static_generic(param.type))
            if not is_static and (literal := arg_type.literal):
                arg_type = literal.runtime_type
            unmangled = unmangled.removeprefix("$")
            if arg_type == ast.types.Stdlib.TypeWrap:
                ctx.add_item(
                    unmangled,
                    variable_name,
                    utils.instantiate_type_var(ctx, arg_type.require_cls[0]),
                )
            else:
                linked = ast.types.Link(
                    cache=ctx.cache, kind=ast.types.Link.Kind.Link, type=arg_type
                )
                ctx.add_item(unmangled, variable_name, linked)
        else:
            if unmangled.startswith("$"):
                unmangled = unmangled.removeprefix("$")
                generic_type = typ.func_generics[generic_idx].type
                if generic_type.is_runtime and generic_type != ast.types.Stdlib.Type:
                    generic_type = utils.instantiate_type_var(ctx, generic_type)
                value = ctx.add_item(unmangled, variable_name, generic_type)
                value.generic = True
            generic_idx += 1

    # Populate realization table in advance to support recursive realizations
    # note: the key might change later
    key = typ.realized_name()
    old_ir = None if not existing else existing.ir

    # Get it if it was already made (force mode)
    realization = cache.FunctionData.Realization(type=typ, ir=old_ir)
    fn_data.realizations[key] = realization
    if bindings:
        for captured in bindings.captures:
            captured_item = ctx.get(captured)
            realization.captures.append("" if not captured_item else captured_item.canonical)
    # Realizations should always be visible, so add them to the toplevel
    ctx.add_always_visible(Item(key, base="", module=ctx.module_name, typ=typ), True)

    if base:
        base.suite = function_ast.suite
    if has_ast and base:
        with ctx.substitute("block_level", 0):
            inferred = infer_types(ctx, base.suite)
        if not inferred:
            fn_data.realizations.pop(key, None)
            if not function_ast.name.startswith("%_lambda"):
                # TODO: generalize this further.
                assert base.suite
                raise TypecheckError(trace=utils.find_typecheck_errors(ctx, base.suite))
            # inference must be delayed
            return None

        base.suite = inferred
        # Use NoneType as the return type when the return type is not specified and
        # function has no return statement
        return_type = typ.ret_type
        if not function_ast.ret and return_type and utils.is_unbound(return_type):
            default_return = utils.get_stdlib_type(ctx, ast.types.Stdlib.NoneType)
            if function_ast.async_:
                default_return = utils.instantiate(
                    ctx,
                    utils.get_stdlib_type(ctx, ast.types.Stdlib.Coroutine),
                    [default_return],
                )
            return_type |= default_return

    # Realize the return type
    return_type = typ.ret_type
    realized_return = realize(ctx, return_type)
    # includeGenerics
    if typ.has_unbounds(False):
        fn_data.realizations.pop(key, None)
        return None
    assert realized_return, f"cannot realize return type '{return_type}'"
    realized_parameters = []
    for param in function_ast.items:
        _, variable_name = param.get_name_with_stars()
        realized_parameters.append(ast.Param(variable_name, status=param.status))
    realized_ast = ast.FunctionStmt(
        typ.realized_name(),
        items=realized_parameters,
        suite=None if not base else base.suite,
        async_=function_ast.async_,
        info=function_ast.info,
        attributes=dict(function_ast.attributes),
    )
    realization.ast = realized_ast
    generalized = typ.generalize(0)
    new_key = generalized.realized_name()
    pending_key = (generalized.func_name, new_key)
    if pending_key not in ctx.cache.pending_realizations:
        fn_data.realizations[new_key] = realization
    elif new_key in fn_data.realizations:
        fn_data.realizations[key] = fn_data.realizations[new_key]
    if force and new_key in fn_data.realizations:
        fn_data.realizations[new_key].ast = realized_ast
    realization.type = generalized
    if not realization.ir:
        realization.ir = make_ir_function(ctx, realization)
    ctx.add_always_visible(Item(new_key, base="", module=ctx.module_name, typ=generalized), True)
    return realization.type


def make_ir_type(ctx: TypeContext, typ: ast.types.Class) -> ast.ir.Type:
    """Make IR node for a realized type."""

    # Realize if not, and return cached value if it exists
    realized_name = ast.types.Class.realized_name(typ)
    cls_data = utils.get_class(ctx, typ)
    assert cls_data
    if realized_name not in cls_data.realizations:
        realized = realize(ctx, typ)
        assert realized
        typ = realized
        realized_name = typ.realized_name()
        cls_data = utils.get_class(ctx, typ)
        assert cls_data
    realization = cls_data.realizations[realized_name]
    if realization.ir:
        if cls_data.rtti:
            if False:
                raise NotImplementedError
                realization.ir.set_polymorphic()
        return realization.ir

    def force_find_ir_type(typ: ast.types.Type):
        assert isinstance(typ, ast.types.Class), f"{typ} not realized"
        cls_data = utils.get_class(ctx, typ)
        assert cls_data
        name = typ.realized_name()
        assert name in cls_data.realizations, f"{typ} not realized"
        handle = cls_data.realizations[name].ir
        assert handle, f"no LLVM type for {typ}"
        return handle

    # Prepare generics and statics
    type_arguments = []
    static_arguments = []
    if typ == ast.types.Stdlib.UnrealizedType:
        type_arguments.append(None)
    else:
        for generic in typ.generics:
            if literal := generic.type.literal:
                static_arguments.append(literal)
            else:
                type_arguments.append(force_find_ir_type(generic.type))

    # Get the IR type
    handle = ast.ir.Type()
    if False:
        raise NotImplementedError
        module = ctx.cache.module
        if typ.name == ast.types.Stdlib.Bool:
            handle = module.get_bool_type()
        elif typ.name == "byte":
            handle = module.get_byte_type()
        elif typ.name == "int":
            handle = module.get_int_type()
        elif typ.name == ast.types.Stdlib.Float:
            handle = module.get_float_type()
        elif typ.name == "float32":
            handle = module.get_float32_type()
        elif typ.name == ast.types.Stdlib.Float16:
            handle = module.get_float16_type()
        elif typ.name == "bfloat16":
            handle = module.get_b_float16_type()
        elif typ.name == "float128":
            handle = module.get_float128_type()
        elif typ.name == ast.types.Stdlib.String:
            handle = module.get_string_type()
        elif typ.name in {ast.types.Stdlib.Int, ast.types.Stdlib.UInt}:
            assert static_arguments
            handle = module.new_int_n_type(
                utils.get_int_literal(static_arguments[0]), typ.name == ast.types.Stdlib.Int
            )
        elif typ.name == ast.types.Stdlib.Ptr:
            assert len(type_arguments) == 1
            handle = module.unsafe_get_pointer_type(type_arguments[0])
        elif (
            typ.name in {ast.types.Stdlib.Generator, "AsyncGenerator"}
            or typ.name == ast.types.Stdlib.Coroutine
        ):
            assert len(type_arguments) == 1
            handle = module.unsafe_get_generator_type(type_arguments[0])
        elif typ.name == ast.types.Stdlib.Optional:
            assert len(type_arguments) == 1
            handle = module.unsafe_get_optional_type(type_arguments[0])
        elif typ.name == ast.types.Stdlib.NoneType:
            assert not type_arguments and not static_arguments
            handle = module.unsafe_get_membered_type(realized_name)
            handle.realize()
        elif typ.name == ast.types.Stdlib.Union:
            assert type_arguments
            union = typ.get_union()
            union_types = [force_find_ir_type(value) for value in union.get_realization_types()]
            handle = module.unsafe_get_union_type(union_types)
        elif typ.name == ast.types.Stdlib.Function:
            type_arguments.clear()
            argument_tuple = typ[0]
            for generic in argument_tuple.generics:
                type_arguments.append(force_find_ir_type(generic.type))
            return_type_handle = force_find_ir_type(typ[1])
            handle = module.unsafe_get_func_type(realized_name, return_type_handle, type_arguments)
        elif typ.name == ast.types.Stdlib.Vec:
            assert len(type_arguments) == 1 and static_arguments
            handle = module.unsafe_get_vector_type(
                utils.get_int_literal(static_arguments[0]), type_arguments[0]
            )
        elif typ.is_record():
            # Type arguments will be populated afterwards to avoid infinite loop with recursive
            # reference types (e.g., `class X: x: Optional[X]`)

            ir_field_types = []
            names = []
            member_info = {}
            assert not typ.is_type("__NTuple__")

            field_types = utils.get_class_field_types(ctx, typ)
            for idx, field_type in enumerate(field_types):
                if not realize(field_type):
                    raise TypecheckError(
                        field_type.info,
                        f"type of attribute '{cls_data.fields[idx].name}' of object "
                        f"'{typ.pretty_string()}' cannot be inferred",
                    )
                names.append(cls_data.fields[idx].name)
                ir_field_types.append(make_ir_type(ctx, field_type))
                member_info[cls_data.fields[idx].name] = field_type.info
            handle = module.unsafe_get_membered_type(realized_name)
            handle.realize(ir_field_types, names)
            handle.set(module.make_member_attribute(member_info))
        else:
            handle = module.unsafe_get_membered_type(realized_name, not typ.is_record())
            if cls_data.rtti:
                handle.set_polymorphic()
        handle.info = typ.info
        handle.ast_type = typ
        realization.ir = handle
    return handle


def make_ir_function(
    ctx: TypeContext, realization: cache.FunctionData.Realization
) -> ast.ir.Function:
    """Make IR node for a realized function."""
    module = ctx.cache.module

    # Create and store a function IR node and a realized AST for IR passes
    assert realization.ast
    function = ast.ir.Function()
    if False:
        raise NotImplementedError
        if realization.ast.has(ast.Attr.Internal):
            # e.g., __new__, Ptr.__new__, etc.
            function = module.new_internal_func(realization.type.ast.name)
        elif realization.ast.has(ast.Attr.LLVM):
            function = module.new_llvm_func(realization.type.realized_name())
        elif realization.ast.has(ast.Attr.C):
            function = module.new_external_func(realization.type.realized_name())
        else:
            function = module.new_bodied_func(realization.type.realized_name())

        function.set_unmangled_name(utils.get_unmangled_name(ctx, realization.type.ast.name))

    parent = realization.type.func_parent
    parent_name = realization.ast.get(ast.Attr.ParentClass, "")
    if parent_name and not realization.ast.has(ast.Attr.Method):
        # Hack for non-generic methods
        parent = ctx[parent_name].type
    if False:
        raise NotImplementedError
        if parent and parent.is_instantiated() and parent.can_realize():
            parent_class = realize(ctx, utils.extract_class_type(ctx, parent))
            function.set_parent_type(make_ir_type(ctx, parent_class))
        function.set_global()

    # Mark this realization as pending (i.e., realized but not translated)
    ctx.cache.pending_realizations.add(
        (realization.type.ast.name, realization.type.realized_name())
    )
    assert len(realization.ast.items) == len(realization.type.generics) + len(
        realization.type.func_generics
    )
    names = []
    arg_types = []
    value_idx = 0
    for parameter in realization.ast.items:
        if parameter.is_value():
            arg_type = realization.type[value_idx]
            if not arg_type.func:
                arg_types.append(make_ir_type(ctx, arg_type.require_cls))
                names.append(utils.get_unmangled_name(ctx, parameter.name))
            value_idx += 1
    is_c_vararg = realization.ast.has(ast.Attr.CVarArg)
    if is_c_vararg:
        arg_types.pop()
        names.pop()
    if False:
        raise NotImplementedError
        ir_type = module.unsafe_get_func_type(
            realization.type.realized_name(),
            make_ir_type(ctx, realization.type.get_ret_type()),
            arg_types,
            is_c_vararg,
        )
        ir_type.ast_type = realization.type
        function.realize(ir_type, names)
    return function
