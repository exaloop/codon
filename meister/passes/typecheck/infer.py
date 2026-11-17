# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ... import ast, cache
from ...error import TypecheckError
from .. import scope
from . import TypeContext, classes, utils
from .ctx import Base, Item


def join_nullable_types(ctx, left, right):
    """
    Select a nullable target without committing unresolved payloads or coercing values.
    Ordinary wrapping/unification still checks payload compatibility at each use.
    """
    left = left.literal.runtime_type if left.literal else left
    right = right.literal.runtime_type if right.literal else right
    if right == ast.types.Stdlib.Optional:
        return right
    if left == ast.types.Stdlib.Optional:
        return left
    if left != ast.types.Stdlib.NoneType and right != ast.types.Stdlib.NoneType:
        return None
    member = right if left == ast.types.Stdlib.NoneType else left
    if member == ast.types.Stdlib.NoneType:
        return member
    return utils.instantiate(ctx, utils.get_stdlib_type(ctx, ast.types.Stdlib.Optional), [member])


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
        base = ctx.base  # TODO: why?
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
            # Speculative overload rollback can replace the context's Base objects.
            base = ctx.base
            # Join reached returns before wrapping them; early unification with NoneType
            # would prevent later value returns from establishing an optional.
            if base.infer_return_type and base.return_statements:
                first = inferred = None
                has_empty = has_unknown = False
                for stmt, bare in base.return_statements:
                    returned = stmt.expr.type
                    has_empty |= bare
                    if returned.unbound:
                        has_unknown = True
                    else:
                        first = first or returned
                        inferred = (
                            join_nullable_types(ctx, inferred, returned) or inferred
                            if inferred
                            else returned
                        )
                # Bare returns retain their existing checks (return_none_err_1/2).
                if inferred and (inferred != ast.types.Stdlib.NoneType or not has_unknown):
                    base.return_type |= first if has_empty else inferred
                    base.infer_return_type = False
                    base.return_statements.clear()
                    ctx.changed_nodes += 1
            changed_nodes = ctx.changed_nodes
        if ctx.base.iteration == 1 and is_toplevel:
            # Realize all @force_realize functions
            # Copy keys to avoid modifications during the iteration (#768)
            fns = sorted(ctx.cache.functions.keys())
            for fn_name in fns:
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
        base = ctx.base
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
        if fn := typ.func:
            if realized := realize_func(ctx, fn, force):
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
                ret_type = fn.ret_type
                ret_type |= realized.ret_type
                return realized  # type: ignore
        elif cls := typ.cls:
            return realize_type(ctx, cls)  # type: ignore
    except TypecheckError as err:
        assert err.errors.errors
        backtrace = err.errors.errors[-1]
        if fn := typ.func:
            if fn.ast.has(ast.Attr.HiddenFromUser):
                backtrace.trace[-1].info = ctx.node_stack[-1].info
            else:
                args = []
                arg_idx = 0
                generic_idx = 0
                for param in fn.ast.items:
                    stars, name = param.get_name_with_stars()
                    if param.is_generic():
                        param_type = utils.extract_func_generic(fn, generic_idx)
                        generic_idx += 1
                    else:
                        param_type = fn[arg_idx]
                        arg_idx += 1
                    args.append(
                        f"{'*' * stars}{utils.get_user_facing_name(ctx, name)}: {param_type}"
                    )
                name = fn.func_name
                name_args = ""
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
                    name = utils.get_user_facing_name(ctx, fn.ast.name)
                    name_args = f"({', '.join(args)})"
                backtrace.add(
                    f"during the realization of {name}{name_args}", ctx.node_stack[-1].info
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
    # C++ explicitly calls ClassType::realizedName() here. For a StaticType this
    # bypasses StaticType::realizedName() and uses the runtime class name.
    # TODO: make it nicer!
    realized_name = ast.types.Class.realized_name(typ)
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
            if fn := generic.type.func:
                return_type = fn.ret_type
                if return_type and not return_type.can_realize():
                    return None

    # Realizations should always be visible, so add them to the toplevel
    # TODO: also make it nicer (remove Class manual override)!
    realized_name = ast.types.Class.realized_name(typ)
    generalized = realized.generalize(0)
    generalized.freeze()
    item = Item(realized_name, base="", module=ctx.module_name, typ=generalized)
    if generalized != ast.types.Stdlib.Type or utils.is_metaclass_type(generalized):
        item.type = utils.instantiate_type_var(ctx, realized)
    # The zero-argument metaclass must not replace the type[T] compiler template.
    if not utils.is_metaclass_type(typ):
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
    # These below needed for IR
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

    # Set IR attributes
    from ... import ir

    if names and (reference := ir.cast(ir.RefType, ir_type)):
        reference.contents.realize(ir_field_types, names)
        reference.set_attribute(ir.MemberAttribute(member_info))
        reference.contents.set_attribute(ir.MemberAttribute(member_info))
    for field in fields:
        owner = ir_type
        for base in ir_type.runtime_info.mro:
            if base.ast_type.require_cls.name == field.base_class:
                owner = base
                break
        ir_type.runtime_info.field_owners.append(owner)
    return generalized


def realize_func(
    ctx: TypeContext, typ: ast.types.Function, force: bool = False
) -> ast.types.Function | None:
    fn_data = utils.get_function(ctx, typ)
    assert fn_data
    imported = utils.get_import_module(ctx, fn_data.module)
    existing = fn_data.realizations.get(typ.realized_name())
    if existing and not force:
        if not existing.ast and not existing.type.ret_type.can_realize():
            ctx.base.recursive_dependencies.add(existing.type)
        return existing.type

    old_ctx, ctx = ctx, imported.ctx
    ctx.prepare_checkpoint()
    with ctx.substitute("preamble", old_ctx.preamble):
        if len(ctx.bases) > cache.MAX_REALIZATION_DEPTH:
            raise TypecheckError(
                ctx.node_stack[-1],
                "maximum realization depth reached during the realization of "
                f"'{utils.get_user_facing_name(ctx, typ.ast.name)}'",
            )
        is_import = utils.is_import_fn(typ.ast.name)
        if is_import:
            with ast.Node.creation_context(ctx):
                return _realize_func_body(ctx, typ, force, fn_data, existing, True, old_ctx)
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
                with ast.Node.creation_context(ctx):
                    return _realize_func_body(ctx, typ, force, fn_data, existing, False, old_ctx)
        finally:
            ctx.pop_block()


def _realize_func_body(
    ctx: TypeContext,
    typ: ast.types.Function,
    force: bool,
    fn_data: cache.FunctionData,
    existing,
    is_import: bool,
    caller_ctx: TypeContext,
) -> ast.types.Function | None:
    from ..scope import Bindings
    from . import special

    # Types might change after realization, fix it.
    for arg in typ:
        if cls := arg.cls:
            realize_type(ctx, cls)

    # Clone the generic AST that is to be realized
    fn_ast = typ.ast.clone(clean=True)
    if special_suite := special.generate_special_ast(ctx, typ):
        fn_ast.suite = special_suite
    utils.add_class_generics(ctx, typ, True)
    base = ctx.base
    if base:
        base.func = fn_ast
        base.infer_return_type = (
            not fn_ast.ret
            and utils.is_unbound(typ.ret_type)
            and not fn_ast.async_
            and not fn_ast.has(ast.Attr.IsGenerator)
        )

    # Internal functions have no AST that can be realized
    has_ast = fn_ast.suite is not None and not fn_ast.has(ast.Attr.Internal)
    if bindings := fn_ast.attributes.get(ast.Attr.Bindings):
        assert isinstance(bindings, scope.Bindings)
        for captured, capture_type in sorted(bindings.captures.items()):
            if capture_type is Bindings.Scope.Global:
                captured_item = ctx.find_at(captured)
                if not captured_item:
                    raise TypecheckError(fn_ast, f"name '{captured}' is not defined")
                if not captured_item.is_global():
                    raise TypecheckError(fn_ast, f"no binding for global '{captured}' found")
        for name, canonical in sorted(bindings.local_renames.items()):
            ctx.add(name, ctx[canonical])

    arg_idx = 0
    generic_idx = 0
    for param in fn_ast.items if has_ast else []:
        _, variable_name = param.get_name_with_stars()
        unmangled = utils.get_unmangled_name(ctx, variable_name)
        if param.is_value():
            arg_type = typ[arg_idx]
            arg_idx += 1
            is_static = ast.get_static_generic(param.type) is not ast.types.Type.Behaviour.Runtime
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
                if generic_type.is_runtime and (
                    generic_type != ast.types.Stdlib.Type or utils.is_metaclass_type(generic_type)
                ):
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
        for captured in sorted(bindings.captures):
            captured_item = ctx.find_at(captured)
            realization.captures.append("" if not captured_item else captured_item.canonical)
    # Realizations should always be visible, so add them to the toplevel
    ctx.add_always_visible(Item(key, base="", module=ctx.module_name, typ=typ), True)

    if base:
        base.suite = fn_ast.suite
    if has_ast and base:
        # Prepare captured entry bindings once per realization, before inference retries.
        declared = fn_ast.suite.get(ast.Attr.Bindings, Bindings)
        prepend = []
        for param in fn_ast.items:
            if not param.is_value():
                continue
            name = utils.get_unmangled_name(ctx, param.get_name_with_stars()[1])
            binding = bindings.bindings.get(name) if bindings else None
            declaration = declared.bindings.get(name) if declared else None
            if declaration or (binding and binding.is_nonlocal):
                # Do not overwrite argument with a declaration if
                # argument is captured by some local function.
                prepend.append(ast.AssignStmt(ast.IdExpr(name), ast.IdExpr(name)))
                if declaration:
                    if declaration.count > 0:
                        prepend.append(
                            ast.AssignStmt(
                                ast.IdExpr(name + cache.VAR_USED_SUFFIX), ast.BoolExpr(False)
                            )
                        )
                    # The suite must not introduce another, uninitialized parameter binding.
                    del declared.bindings[name]
        # Entry bindings must be visible throughout the body, regardless of call time.
        for stmt in prepend:
            stmt.set(ast.Attr.ExprTime, fn_ast.suite.get(ast.Attr.ExprTime, int, 0))
            stmt.info = fn_ast.suite.info
        fn_ast.suite.items[0:0] = prepend
        with ctx.substitute("block_level", 0):
            inferred = infer_types(ctx, base.suite)
        base = ctx.base
        if not inferred:
            fn_data.realizations.pop(key, None)
            dependencies = base.recursive_dependencies
            base.recursive_dependencies = set()
            deferred = False
            for dependency in dependencies:
                if dependency.require_func.ret_type.can_realize():
                    continue
                for imported in ctx.cache.imports.values():
                    if imported.ctx:
                        for active in imported.ctx.bases:
                            if active.type is dependency:
                                deferred = True
                                caller_ctx.base.recursive_dependencies.add(dependency)
            # Lambda typecheck failures are "ignored" as they are treated as statements,
            # not functions.
            # TODO: generalize this further.
            if not fn_ast.name.startswith("%_lambda") and not deferred:
                # TODO: generalize this further.
                assert base.suite
                raise TypecheckError(trace=utils.find_typecheck_errors(ctx, base.suite))
            # inference must be delayed
            return None

        base.suite = inferred
        # Use NoneType as the return type when the return type is not specified and
        # function has no return statement
        return_type = typ.ret_type
        if not fn_ast.ret and return_type and utils.is_unbound(return_type):
            default_return = utils.get_stdlib_type(ctx, ast.types.Stdlib.NoneType)
            if fn_ast.async_:
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
    realized_params = []
    for param in fn_ast.items:
        _, variable_name = param.get_name_with_stars()
        realized_params.append(ast.Param(variable_name, status=param.status, kind=param.kind))
    realized_ast = ast.FunctionStmt(
        typ.realized_name(),
        items=realized_params,
        suite=None if not base else base.suite,
        async_=fn_ast.async_,
        info=fn_ast.info,
    )
    realized_ast.attributes = dict(fn_ast.attributes)
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


def make_ir_type(ctx: TypeContext, typ: ast.types.Class):
    """C++: infer.cpp:646. Make IR node for a realized type."""
    from ... import ir

    # Realize if not, and return cached value if it exists.
    name = ast.types.Class.realized_name(typ)
    cls = utils.get_class(ctx, typ)
    if name not in cls.realizations:
        typ = realize(ctx, typ).require_cls
        name = ast.types.Class.realized_name(typ)
        cls = utils.get_class(ctx, typ)
    real = cls.realizations[name]
    if real.ir:
        if cls.has_rtti():
            ir.cast(ir.RefType, real.ir).polymorphic = True
        return real.ir

    def force_find_ir_type(value):
        cls_type = value.cls
        assert cls_type, f"{value} not realized"
        key = ast.types.Class.realized_name(cls_type)
        data = utils.get_class(ctx, cls_type)
        assert key in data.realizations, f"{value} not realized"
        handle = data.realizations[key].ir
        assert handle, f"no LLVM type for {value}"
        return handle

    # Prepare generics and statics.
    types, statics = [], []
    if typ == ast.types.Stdlib.UnrealizedType:
        types.append(None)
    else:
        for generic in typ.generics:
            if literal := generic.type.literal:
                statics.append(literal)
            else:
                types.append(force_find_ir_type(generic.type))
    module = ctx.cache.module
    match typ.name:
        case ast.types.Stdlib.Bool:
            handle = module.get_bool_type()
        case ast.types.Stdlib.Float:
            handle = module.get_float_type()
        case "float32":
            handle = module.get_float32_type()
        case ast.types.Stdlib.Float16:
            handle = module.get_float16_type()
        case "bfloat16":
            handle = module.get_bfloat16_type()
        case "float128":
            handle = module.get_float128_type()
        case ast.types.Stdlib.String:
            handle = module.get_string_type()
        case "bytes":
            handle = module.get_bytes_type()
        case ast.types.Stdlib.Int | ast.types.Stdlib.UInt:
            handle = module.unsafe_get_int_type(
                statics[0].require_int, typ.name == ast.types.Stdlib.Int
            )
        case ast.types.Stdlib.Ptr:
            assert len(types) == 1, "bad generics/statics"
            handle = module.unsafe_get_pointer_type(types[0])
        case ast.types.Stdlib.Generator | "AsyncGenerator" | ast.types.Stdlib.Coroutine:
            assert len(types) == 1, "bad generics/statics"
            handle = module.unsafe_get_generator_type(types[0])
        case ast.types.Stdlib.Optional:
            assert len(types) == 1, "bad generics/statics"
            handle = module.unsafe_get_optional_type(types[0])
        case ast.types.Stdlib.NoneType:
            assert not types and not statics, "bad generics/statics"
            handle = ir.cast(ir.RecordType, module.unsafe_get_membered_type(name))
            handle.realize([], [])
        case ast.types.Stdlib.Union:
            assert types, "bad union"
            handle = module.unsafe_get_union_type(
                [force_find_ir_type(value) for value in typ.union.get_realization_types()]
            )
        case ast.types.Stdlib.Function:
            types = [force_find_ir_type(g.type) for g in typ[0].require_cls.generics]
            handle = module.unsafe_get_func_type(name, force_find_ir_type(typ[1]), types)
        case ast.types.Stdlib.Vec:
            assert len(types) == 1 and statics, "bad generics/statics"
            count = statics[0].require_int
            if count <= 0 or count > 0xFFFFFFFF:
                raise TypecheckError(
                    ctx.info, "SIMD vector width must be positive and fit in 32 bits"
                )
            element = types[0]
            if not isinstance(
                element.get_actual(),
                (
                    ir.IntType,
                    ir.FloatType,
                    ir.Float32Type,
                    ir.Float16Type,
                    ir.BFloat16Type,
                    ir.Float128Type,
                ),
            ):
                raise TypecheckError(
                    ctx.info, "SIMD element type must be an integer or floating-point type"
                )
            handle = module.unsafe_get_vector_type(count, element)
        case _:
            # Populate type arguments afterwards to avoid infinite loops with
            # recursive reference types (e.g. class X: x: Optional[X]).
            if typ.is_tuple:
                types, names, members = [], [], {}
                assert typ.name != "__NTuple__", "ntuple not inlined"
                fields = utils.get_class_field_types(ctx, typ)
                for idx, field in enumerate(fields):
                    if not realize(ctx, field):
                        raise TypecheckError(
                            ctx.info,
                            f"type of attribute '{cls.fields[idx].name}' of object '{typ}' cannot be inferred",
                        )
                    names.append(cls.fields[idx].name)
                    types.append(make_ir_type(ctx, field.require_cls))
                    members[cls.fields[idx].name] = field.info
                handle = ir.cast(ir.RecordType, module.unsafe_get_membered_type(name))
                handle.realize(types, names)
                handle.set_attribute(ir.MemberAttribute(members))
            else:
                handle = module.unsafe_get_membered_type(name, not typ.is_tuple)
                if cls.has_rtti():
                    ir.cast(ir.RefType, handle).polymorphic = True
    handle.info, handle.ast_type = typ.info, typ
    info = ir.RuntimeInfo()
    info.id, info.raw_name, info.nice_name, info.base_name = (
        real.id,
        typ.realized_name(),
        typ.to_string(0),
        typ.name,
    )
    info.mro.append(handle)
    for base in real.bases:
        base_type = realize(ctx, base).require_cls
        info.mro.append(make_ir_type(ctx, base_type))
    for base in info.mro:
        base_name = base.ast_type.require_cls.name
        if base_name == ast.types.Stdlib.BaseException:
            info.exception_base = base
        if base_name == ast.types.mangle("std.internal.types.error", cls="SystemExit"):
            info.system_exit_base = base
    if typ != ast.types.Stdlib.UnrealizedType:
        if typ == ast.types.Stdlib.Int:  # Avoid recursion for Int[N].
            info.parameters.append(handle)
        else:
            for generic in typ.generics:
                parameter = utils.get_underlying_static_type(ctx, generic.type)
                info.parameters.append(make_ir_type(ctx, parameter.require_cls))
    handle.runtime_info = info
    real.ir = handle
    return handle


def make_ir_function(ctx: TypeContext, real: cache.FunctionData.Realization):
    """C++: infer.cpp:820. Make IR node for a realized function."""
    from ... import ir

    module = ctx.cache.module
    name = ctx.cache.reverse_identifier_lookup[real.type.ast.name]
    # Create/store a function IR node and a realized AST for IR passes.
    if real.ast.has(ast.Attr.Internal):
        # __new__, Ptr.__new__, etc.
        fn = module.nr(ir.InternalFunc, real.type.realized_name())
    elif real.ast.has(ast.Attr.LLVM):
        fn = module.nr(ir.LLVMFunc, real.type.realized_name())
    elif real.ast.has(ast.Attr.C):
        attrs = real.ast.get(ast.Attr.FunctionAttributes, dict, {})
        if link := attrs.get(ast.types.mangle("std.internal.c_stubs", func="linkname")):
            name = utils.extract_func_generic(ctx.cache.find_function(link)).require_str
        fn = module.nr(ir.ExternalFunc, real.type.realized_name())
    else:
        fn = module.nr(ir.BodiedFunc, real.type.realized_name())
    fn.unmangled_name = name
    parent = real.type.func_parent
    if (owner := real.ast.get(ast.Attr.ParentClass, str)) and not real.ast.has(ast.Attr.Method):
        parent = ctx[owner].type  # Hack for non-generic methods.
    if parent and parent.is_instantiated() and parent.can_realize():
        parent = utils.extract_class_type(ctx, parent)
        realize(ctx, parent)
        fn.parent_type = make_ir_type(ctx, parent)
    fn.global_ = True
    # Mark this realization pending (realized but not translated).
    ctx.cache.pending_realizations.add((real.type.ast.name, real.type.realized_name()))
    assert len(real.ast.items) == len(real.type) + len(real.type.func_generics), (
        "type/AST argument mismatch"
    )
    names, types, idx = [], [], 0
    for arg in real.ast.items:
        if arg.is_value():
            if not real.type[idx].func:
                types.append(make_ir_type(ctx, real.type[idx].require_cls))
                names.append(ctx.cache.reverse_identifier_lookup[arg.name])
            idx += 1
    variadic = real.ast.has(ast.Attr.CVarArg)
    if variadic:
        types.pop()
        names.pop()
    signature = module.unsafe_get_func_type(
        real.type.realized_name(),
        make_ir_type(ctx, real.type.ret_type.require_cls),
        types,
        variadic,
    )
    signature.ast_type = real.type
    fn.realize(signature, names)
    # Identify builtins at the frontend boundary; CIR consumers use intrinsic IDs.
    if isinstance(fn, ir.InternalFunc):
        method = real.type.func_name
        if method == ast.types.mangle(cls="type", func="_ref_alloc"):
            fn.set_intrinsic(ir.Intrinsic.ALLOCATE, signature.return_type)
        elif method in {
            ast.types.mangle(cls="type", func=member)
            for member in ("_force_cast", "_ref_raw", "_force_value_cast")
        }:
            target = signature.return_type
            if method != ast.types.mangle(cls="type", func="_force_value_cast"):

                def pointer_like(typ):
                    if optional := ir.cast(ir.OptionalType, typ):
                        return ir.is_a(ir.RefType, optional.base)
                    return isinstance(
                        typ.get_actual(),
                        (ir.RefType, ir.PointerType, ir.FuncType, ir.GeneratorType),
                    )

                if len(types) != 1 or not pointer_like(types[0]) or not pointer_like(target):
                    raise TypecheckError(
                        real.ast, "pointer cast requires pointer-like source and destination types"
                    )
            fn.set_intrinsic(ir.Intrinsic.REPRESENTATION_CAST, target)
        elif method == ast.types.mangle(cls="type", func="_ref_size"):
            fn.set_intrinsic(
                ir.Intrinsic.ALLOCATION_SIZE,
                make_ir_type(ctx, utils.extract_func_generic(real.type).require_cls),
            )
        elif method == ast.types.mangle(cls="TypeInfo", func="_get"):
            fn.set_intrinsic(
                ir.Intrinsic.TYPEINFO,
                make_ir_type(ctx, utils.extract_func_generic(real.type).require_cls),
            )
        elif method == ast.types.mangle(cls="TypeInfo", func="_lookup"):
            fn.set_intrinsic(ir.Intrinsic.TYPEINFO_LOOKUP)
        elif method == ast.types.mangle(cls="TypeInfo", func="_lookup_method"):
            fn.set_intrinsic(ir.Intrinsic.TYPEINFO_METHOD_LOOKUP)
        elif (
            name == "__new__" and ir.is_a(ir.RecordType, fn.parent_type)
        ) or method == ast.types.mangle(cls="Partial", func="__new__"):
            fn.set_intrinsic(ir.Intrinsic.RECORD_NEW, signature.return_type)
        elif method == ast.types.mangle(cls="Generator", func="__promise__"):
            fn.set_intrinsic(ir.Intrinsic.GENERATOR_PROMISE, fn.parent_type)
    return fn
