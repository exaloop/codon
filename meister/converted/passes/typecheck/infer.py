# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast, cache, error
from . import utils
from .ctx import Base, Item, TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def unify(left: ast.types.Type, right: ast.types.Type):
    """
    Unify types a (passed by reference) and b.
    Destructive operation as it modifies both a and b. If types cannot be unified, raise
    an error.
    @param a Type (by reference)
    @param b Type
    @return a
    """
    assert left and right, "unifying a null type"
    if (left << right) is None:
        undo = ast.types.Type.UnifyContext()
        left.unify(right, undo)
        raise TypecheckError(
            left.info,
            f"'{left.pretty_string()}' does not match expected type '{right.pretty_string()}'",
        )
    return left


def ior(self: ast.Expr, other: ast.types.Type):
    if not self.type:
        self.type = other
    else:
        undo = ast.types.Type.UnifyContext()
        if self.type.unify(other, undo) < 0:
            raise TypecheckError(
                self,
                f"'{self.type.pretty_string()}' does not match expected type "
                f"'{other.pretty_string()}'",
            )
    return self.type


ast.Expr.__ior__ = ior


def infer_types(
    self: TypeVisitor, result: ast.Stmt | None, is_toplevel: bool = False
) -> ast.Stmt | None:
    """
    Infer all types within a Stmt *. Implements the LTS-DI typechecking.
    @param isToplevel set if typechecking the program toplevel.
    """
    from . import TypeVisitor

    if not result:
        return None

    base = self.ctx.get_base()
    base.iteration = 1
    while True:
        if base.iteration >= 1000:
            source = "toplevel" if not base.name else utils.get_unmangled_name(self.ctx, base.name)
            raise TypecheckError(
                result,
                f"cannot typecheck '{source}' in reasonable time",
                stack=utils.find_typecheck_errors(self.ctx, result),
            )
        # Keep iterating until:
        # (1) success: the statement is marked as done; or
        # (2) failure: no expression or statements were marked as done during an
        # iteration (i.e., changedNodes is zero)
        with (
            self.ctx.substitute("typecheck_level", self.ctx.typecheck_level + 1),
            self.ctx.substitute("changed_nodes", 0),
            self.ctx.substitute("return_early", False),
        ):
            visitor = TypeVisitor(ctx=self.ctx, preamble=self.preamble)
            result = visitor.visit(result)
            changed_nodes = self.ctx.changed_nodes
        if base.iteration == 1 and is_toplevel:
            # Realize all @force_realize functions
            # Copy keys to avoid modifications during the iteration (#768)
            for function_name in list(self.ctx.cache.functions.keys()):
                function = self.ctx.cache.functions[function_name]
                if (
                    function.type
                    and not function.realizations
                    and function.ast
                    and (
                        function.ast.has(ast.Attr.ForceRealize)
                        or function.ast.has(ast.Attr.Export)
                        or (function.ast.has(ast.Attr.C) and not function.ast.has(ast.Attr.CVarArg))
                    )
                ):
                    assert function.type.can_realize(), f"cannot realize {function_name}"
                    realize(utils.instantiate_type(self.ctx, function.type))
                    assert function.realizations, f"cannot realize {function_name}"
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
                        utils.extract_class_type(self.ctx, unbound.default_type)
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


def realize(self: TypeVisitor, typ: ast.types.Type, force: bool = False) -> ast.types.Type | None:
    """
    Realize a type and create IR type stub. If type is a function type, also realize the
    underlying function and generate IR function stub.
    @return realized type or nullptr if the type cannot be realized
    """
    if not typ or not typ.can_realize():
        return None

    try:
        if isinstance(typ, ast.types.Function):
            if realized := realize_func(self, typ, force):
                # Realize Function[..] type as well
                class_type = ast.types.Class(
                    name=realized.name,
                    generics=list(realized.generics),
                    hidden_generics=list(realized.hidden_generics),
                    is_tuple=realized.is_tuple,
                    _cached_name=realized._cached_name,
                    copy=realized,
                )
                realize_type(self, class_type)
                # Needed for return type unification
                unify(typ.get_ret_type(), realized[1])
                return realized
        else:
            return realize_type(self, typ)
    except TypecheckError as err:
        assert err.errors.errors
        backtrace = err.errors.errors[-1]
        if isinstance(typ, ast.types.Function):
            if typ.ast.has(ast.Attr.HiddenFromUser):
                backtrace.trace[-1].info = self.ctx.node_stack[-1].info
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
                        f"{'*' * stars}{utils.get_user_facing_name(self.ctx, name)}: "
                        f"{parameter_type.pretty_string()}"
                    )
                name = typ.ast.name
                name_arguments = ""
                if name.startswith("%_import_"):
                    for imported in self.ctx.cache.imports.values():
                        if (
                            ast.types.mangle(func=f"{imported.import_var}_call", no_core=True)
                            == name
                        ):
                            name = imported.name
                            break
                    name = f"<import {name}>"
                else:
                    name = utils.get_user_facing_name(self.ctx, typ.ast.name)
                    name_arguments = f"({', '.join(arguments)})"
                backtrace.add(
                    f"during the realization of {name}{name_arguments}",
                    self.ctx.node_stack[-1].info,
                )
        else:
            backtrace.add(
                f"during the realization of {typ.pretty_string()}",
                self.ctx.node_stack[-1].info,
            )
        raise
    return None


def realize_type(self: TypeVisitor, typ: ast.types.Class) -> ast.types.Type | None:
    """
    Realize a type and create IR type stub.
    @return realized type or nullptr if the type cannot be realized
    """
    if not typ or not typ.can_realize():
        return None

    # Check if the type fields are all initialized
    # (sometimes that's not the case: e.g., `class X: x: List[X]`)

    # generalize generics to ensure that they do not get unified later!
    if typ.is_type(ast.types.Stdlib.UnrealizedType):
        typ[0] = typ[0].generalize()
    if typ.is_type("__NTuple__"):
        count = max(0, utils.get_int_literal(typ))
        generated_generics = []
        generated = utils.instantiate_type(
            self.ctx, classes.generate_tuple(self, count * len(typ[1].generics))
        )
        generic_idx = 0
        for _ in range(count):
            for template_generic in typ[1].generics:
                generated_generic = generated.generics[generic_idx]
                unify(generated_generic.type, template_generic.type)
                generated_generics.append(generated_generic)
                generic_idx += 1
        typ.name = ast.types.Stdlib.Tuple
        typ.generics = generated_generics
        typ._cached_name = ""

    # Check if the type was already realized
    realized_name = typ.realized_name()
    class_data = utils.get_class(self.ctx, typ)
    existing = class_data.realizations.get(realized_name)
    if existing and existing.type:
        return existing.type
    if not class_data.ast:
        return None
    fields = utils.get_class_fields(typ)
    field_types = utils.get_class_field_types(self, typ)
    if len(field_types) != len(fields):
        # not yet done!
        return None
    realized = typ
    if isinstance(typ, ast.types.Literal):
        # do not cache static but its root type!
        if nonstatic := typ.get_non_static_type():
            realized = nonstatic

    # Realize generics
    if not typ.is_type(ast.types.Stdlib.UnrealizedType):
        for generic in realized.generics:
            if not generic.type or not realize(generic.type):
                return None
            if isinstance(generic.type, ast.types.Function):
                return_type = generic.type.get_ret_type()
                if return_type and not return_type.can_realize():
                    return None

    # Realizations should always be visible, so add them to the toplevel
    realized_name = typ.realized_name()
    generalized = realized.generalize()
    item = Item(realized_name, module_name=self.ctx.get_module(), type=generalized)
    if not generalized.is_type(ast.types.Stdlib.Type):
        item.type = utils.instantiate_type_var(self.ctx, realized)
    self.ctx.add_always_visible(item, True)

    realization = cache.ClassData.Realization(type=generalized)
    self.ctx.cache.class_realization_cnt += 1
    realization.id = self.ctx.cache.class_realization_cnt
    class_data.realizations[realized_name] = realization
    for parent in class_data.mro[1:]:
        # need to generalize it first because generics are
        # not yet generalized when parsing methods
        generalized_parent = parent.generalize()
        instantiated_parent = utils.instantiate_type(self.ctx, generalized_parent, realized)
        assert instantiated_parent.can_realize()
        realization.bases.append(instantiated_parent)

    # Create LLVM stub
    ir_type = make_ir_type(self, realized)
    # Realize fields
    ir_field_types = []
    names = []
    member_info = {}
    for idx, field_type in enumerate(field_types):
        if not realize(field_type):
            raise TypecheckError(
                field_type.info,
                f"type of attribute '{fields[idx].name}' of object "
                f"'{realized.pretty_string()}' cannot be inferred",
            )
        realization.fields.append((fields[idx].name, field_type))
        names.append(fields[idx].name)
        ir_field_types.append(make_ir_type(self, field_type))
        member_info[fields[idx].name] = field_type.info

    # IR attributes
    if names and isinstance(ir_type, ast.ir.types.RefType):
        ir_type.get_contents().realize(ir_field_types, names)
        ir_type.set(ast.ir.Attr.Member, member_info)
        ir_type.get_contents().set(ast.ir.Attr.Member, member_info)
    return generalized


def realize_func(
    self: TypeVisitor, typ: ast.types.Function, force: bool = False
) -> ast.types.Type | None:
    fn_data = utils.get_function(self.ctx, typ)
    imported = utils.get_import_module(self.ctx, fn_data.module)
    existing = fn_data.realizations.get(typ.realized_name())
    if existing and not force:
        return existing.type

    old_ctx, self.ctx = self.ctx, imported.ctx
    try:
        if self.ctx.get_realization_depth() > cache.MAX_REALIZATION_DEPTH:
            raise TypecheckError(
                self.ctx.node_stack[-1],
                "maximum realization depth reached during the realization of "
                f"'{utils.get_user_facing_name(self.ctx, typ.ast.name)}'",
            )
        is_import = utils.is_import_fn(typ.ast.name)
        if is_import:
            return _realize_func_body(self, typ, force, fn_data, existing, True)
        self.ctx.add_block()
        try:
            with (
                self.ctx.substitute("typecheck_level", self.ctx.typecheck_level + 1),
                self.ctx.substitute(
                    "bases",
                    self.ctx.bases
                    + [Base(name=typ.ast.name, type=typ, return_type=typ.get_ret_type())],
                ),
            ):
                for idx in range(len(self.ctx.bases) - 2, -1, -1):
                    if self.ctx.get_base_name().startswith(self.ctx.bases[idx].name):
                        self.ctx.get_base().parent = idx
                        break
                return _realize_func_body(
                    self,
                    typ,
                    force,
                    fn_data,
                    existing,
                    False,
                )
        finally:
            self.ctx.pop_block()
    finally:
        self.ctx = old_ctx


def _realize_func_body(
    self: TypeVisitor,
    typ: ast.types.Function,
    force: bool,
    fn_data: cache.FunctionData,
    existing,
    is_import: bool,
) -> ast.types.Type | None:
    from ..scope import Bindings
    from . import special

    for generic in typ.generics:
        if generic.type:
            # Types might change after realization, fix it
            if isinstance(generic.type, ast.types.Class):
                realize_type(self, generic.type)

    # Clone the generic AST that is to be realized
    function_ast = typ.ast.clone(clean=True)
    if special_suite := special.generate_special_ast(self, typ):
        function_ast.suite = special_suite
    utils.add_class_generics(self.ctx, typ, True)
    base = self.ctx.get_base()
    if base:
        base.func = function_ast

    # Internal functions have no AST that can be realized
    has_ast = function_ast.suite is not None and not function_ast.has(ast.Attr.Internal)
    if bindings := function_ast.attributes.get(ast.Attr.Bindings):
        for captured, capture_type in bindings.captures.items():
            if capture_type is Bindings.Scope.Global:
                captured_item = self.ctx.find(captured)
                if not captured_item:
                    raise TypecheckError(function_ast, f"name '{captured}' is not defined")
                if not captured_item.is_global():
                    raise TypecheckError(function_ast, f"no binding for global '{captured}' found")
        for name, canonical in bindings.local_renames.items():
            value = self.ctx.find(canonical)
            self.ctx.add(name, value)

    argument_idx = 0
    generic_idx = 0
    for parameter in function_ast.items if has_ast else []:
        _, variable_name = parameter.get_name_with_stars()
        unmangled = utils.get_unmangled_name(self.ctx, variable_name)
        if parameter.is_value():
            argument_type = typ[argument_idx]
            argument_idx += 1
            is_static = bool(parameter.type) and bool(ast.get_static_generic(parameter.type))
            if not is_static and isinstance(argument_type, ast.types.Literal):
                argument_type = argument_type.get_non_static_type()
            unmangled = unmangled.removeprefix("$")
            if argument_type.is_type(ast.types.Stdlib.TypeWrap):
                self.ctx.add(
                    unmangled,
                    variable_name,
                    utils.instantiate_type_var(self.ctx, argument_type[0]),
                )
            else:
                linked = ast.types.Link(
                    cache=self.ctx.cache, kind=ast.types.Link.Kind.Link, type=argument_type
                )
                self.ctx.add(unmangled, variable_name, linked)
        else:
            if unmangled.startswith("$"):
                unmangled = unmangled.removeprefix("$")
                generic_type = typ.func_generics[generic_idx].type
                if (
                    generic_type.static_kind is ast.types.Type.Behaviour.Runtime
                    and not generic_type.is_type(ast.types.Stdlib.Type)
                ):
                    generic_type = utils.instantiate_type_var(self.ctx, generic_type)
                value = self.ctx.add(unmangled, variable_name, generic_type)
                value.generic_type = True
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
            captured_item = self.ctx.find(captured)
            realization.captures.append("" if not captured_item else captured_item.canonical_name)
    # Realizations should always be visible, so add them to the toplevel
    visibility_item = Item(key, module=self.ctx.get_module(), type=typ)
    self.ctx.add_always_visible(visibility_item, True)

    if base:
        base.suite = function_ast.suite
    if has_ast and base:
        with self.ctx.substitute("block_level", 0):
            inferred = infer_types(self, base.suite)
        if not inferred:
            fn_data.realizations.pop(key, None)
            errors = error.ParserErrors()
            if not function_ast.name.startswith("%_lambda"):
                # Lambda typecheck failures are "ignored" as they are treated as statements,
                # not functions.
                # TODO: generalize this further.
                errors = utils.find_typecheck_errors(self.ctx, base.suite)
            if errors.errors:
                raise TypecheckError(errors)
            # inference must be delayed
            return None

        base.suite = inferred
        # Use NoneType as the return type when the return type is not specified and
        # function has no return statement
        return_type = typ.get_ret_type()
        if not function_ast.ret and return_type and utils.is_unbound(return_type):
            default_return = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.NoneType)
            if function_ast.is_async():
                default_return = utils.instantiate_type(
                    self.ctx,
                    utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Coroutine),
                    [default_return],
                )
            unify(return_type, default_return)

    # Realize the return type
    return_type = typ.get_ret_type()
    realized_return = realize(return_type)
    # includeGenerics
    if typ.has_unbounds(False):
        fn_data.realizations.pop(key, None)
        return None
    assert realized_return, f"cannot realize return type '{return_type}'"
    realized_parameters = []
    for parameter in function_ast.items:
        _, variable_name = parameter.get_name_with_stars()
        realized_parameters.append(ast.Param(variable_name, status=parameter.status))
    realized_ast = ast.FunctionStmt(
        typ.realized_name(),
        items=realized_parameters,
        suite=None if not base else base.suite,
        async_=function_ast.is_async(),
        info=function_ast.info,
        attributes=dict(function_ast.attributes),
    )
    realization.ast = realized_ast
    generalized = typ.generalize(0)
    new_key = generalized.realized_name()
    pending_key = (generalized.get_func_name(), new_key)
    if pending_key not in self.ctx.cache.pending_realizations:
        fn_data.realizations[new_key] = realization
    elif new_key in fn_data.realizations:
        fn_data.realizations[key] = fn_data.realizations[new_key]
    if force and new_key in fn_data.realizations:
        fn_data.realizations[new_key].ast = realized_ast
    realization.type = generalized
    if not realization.ir:
        realization.ir = make_ir_function(self, realization)
    self.ctx.add_always_visible(Item(new_key, module=self.ctx.get_module(), type=generalized), True)
    return realization.type


def make_ir_type(self: TypeVisitor, typ: ast.types.Class) -> ast.ir.Type:
    """Make IR node for a realized type."""

    # Realize if not, and return cached value if it exists
    realized_name = ast.types.Class.realized_name(typ)
    cls_data = utils.get_class(self.ctx, typ)
    if realized_name not in cls_data.realizations:
        typ = realize(typ)
        realized_name = typ.realized_name()
        cls_data = utils.get_class(self.ctx, typ)
    realization = cls_data.realizations[realized_name]
    if realization.ir:
        if cls_data.rtti:
            realization.ir.set_polymorphic()
        return realization.ir

    def force_find_ir_type(typ: ast.types.Type):
        assert isinstance(typ, ast.types.Class), f"{typ} not realized"
        cls_data = utils.get_class(self.ctx, typ)
        name = typ.realized_name()
        assert name in cls_data.realizations, f"{typ} not realized"
        handle = cls_data.realizations[name].ir
        assert handle, f"no LLVM type for {typ}"
        return handle

    # Prepare generics and statics
    type_arguments = []
    static_arguments = []
    if typ.is_type(ast.types.Stdlib.UnrealizedType):
        type_arguments.append(None)
    else:
        for generic in typ.generics:
            if static := generic.type.get_static():
                static_arguments.append(static)
            else:
                type_arguments.append(force_find_ir_type(generic.type))

    # Get the IR type
    module = self.ctx.cache.module
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

        field_types = utils.get_class_field_types(self, typ)
        for idx, field_type in enumerate(field_types):
            if not realize(field_type):
                raise TypecheckError(
                    field_type.info,
                    f"type of attribute '{cls_data.fields[idx].name}' of object "
                    f"'{typ.pretty_string()}' cannot be inferred",
                )
            names.append(cls_data.fields[idx].name)
            ir_field_types.append(make_ir_type(self, field_type))
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


def make_ir_function(self: TypeVisitor, realization: cache.FunctionData.Realization) -> object:
    """Make IR node for a realized function."""
    module = self.ctx.cache.module

    # Create and store a function IR node and a realized AST for IR passes
    if realization.ast.has(ast.Attr.Internal):
        # e.g., __new__, Ptr.__new__, etc.
        function = module.new_internal_func(realization.type.ast.name)
    elif realization.ast.has(ast.Attr.LLVM):
        function = module.new_llvm_func(realization.type.realized_name())
    elif realization.ast.has(ast.Attr.C):
        function = module.new_external_func(realization.type.realized_name())
    else:
        function = module.new_bodied_func(realization.type.realized_name())

    function.set_unmangled_name(utils.get_unmangled_name(self.ctx, realization.type.ast.name))
    parent = realization.type.func_parent
    parent_name = realization.ast.get(ast.Attr.ParentClass, "")
    if parent_name and not realization.ast.has(ast.Attr.Method):
        # Hack for non-generic methods
        parent_item = self.ctx.find(parent_name)
        parent = parent_item.type
    if parent and parent.is_instantiated() and parent.can_realize():
        parent_class = realize(utils.extract_class_type(self.ctx, parent))
        function.set_parent_type(make_ir_type(self, parent_class))
    function.set_global()

    # Mark this realization as pending (i.e., realized but not translated)
    self.ctx.cache.pending_realizations.add(
        (realization.type.ast.name, realization.type.realized_name())
    )
    assert len(realization.ast.items) == len(realization.type.generics) + len(
        realization.type.func_generics
    )
    names = []
    argument_types = []
    value_idx = 0
    for parameter in realization.ast.items:
        if parameter.is_value():
            argument_type = realization.type[value_idx]
            if not argument_type.get_func():
                argument_types.append(make_ir_type(self, argument_type))
                names.append(utils.get_unmangled_name(self.ctx, parameter.name))
            value_idx += 1
    is_c_vararg = realization.ast.has(ast.Attr.CVarArg)
    if is_c_vararg:
        argument_types.pop()
        names.pop()
    ir_type = module.unsafe_get_func_type(
        realization.type.realized_name(),
        make_ir_type(self, realization.type.get_ret_type()),
        argument_types,
        is_c_vararg,
    )
    ir_type.ast_type = realization.type
    function.realize(ir_type, names)
    return function
