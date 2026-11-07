# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast, cache
from ...bridge import List, cast
from ...error import TypecheckError
from . import TypeContext, access, infer, utils

if TYPE_CHECKING:
    from . import TypeVisitor


def vtable_signature(ctx, fn):
    """Stable virtual-method signature, including the static receiver and generics."""
    assert fn.can_realize(), f"cannot realize {fn!r}"
    values = ",".join(arg.realized_name() for arg in fn)
    generics = ",".join(g.type.realized_name() for g in fn.func_generics if g.name)
    signature = values + ";" + generics if values and generics else values or generics
    return f"{fn[0].require_cls.name}.{utils.get_unmangled_name(ctx, fn.func_name)}({signature})"


def value_method_id(ctx, method):
    return ctx.cache.thunk_ids.setdefault("$any." + method, len(ctx.cache.thunk_ids) + 1)


def prepare_vtables(ctx):
    """Realize virtual and cooperative super targets before checking constructors."""
    from . import TypeVisitor, classes

    visitor = TypeVisitor(ctx)

    def typed_argument(typ):
        return ast.NoneExpr(type=typ, done=True)

    def subtype(source, destination):
        if source.unify(destination, None) >= 0:
            return True
        cls, base = source.cls, destination.cls
        return bool(
            cls
            and base
            and not cls.is_tuple
            and not base.is_tuple
            and any(parent.unify(base, None) >= 0 for parent in utils.get_mro(ctx, cls))
        )

    def adapt_target(signature, receiver, target):
        if not subtype(target.ret_type, signature.ret_type):
            raise TypecheckError(
                signature.ast,
                f"function {utils.get_unmangled_name(ctx, receiver.name)}."
                f"{utils.get_unmangled_name(ctx, target.func_name)} returns {target.ret_type}, expected {signature.ret_type}",
            )
        compatible = (
            len(signature) == len(target)
            and signature.ret_type.realized_name() == target.ret_type.realized_name()
            and all(
                signature[i].realized_name() == target[i].realized_name()
                for i in range(1, len(signature))
            )
        )
        if compatible:
            return target
        source_params = [
            p
            for p in signature.ast.items
            if p.is_value()
            and not utils.get_unmangled_name(ctx, p.get_name_with_stars()[1]).startswith("$")
        ]
        assert len(source_params) == len(signature), "dispatch signature argument count mismatch"
        params, args, placeholders = [], [], []
        for idx, (param, typ) in enumerate(zip(source_params, signature)):
            typ = typ.literal.runtime_type if typ.literal else typ
            name = f"argument{idx}"
            params.append(ast.Param(name, ast.IdExpr(typ.realized_name())))
            value = ast.IdExpr(name)
            stars, source_name = param.get_name_with_stars()
            if not idx:
                value = ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="type", func="_force_cast")),
                    [value, ast.IdExpr(target[0].realized_name())],
                )
            elif stars == 1:
                value = ast.StarExpr(value)
            elif stars == 2:
                value = ast.KeywordStarExpr(value)
            args.append(
                ast.CallExpr.Arg(
                    value,
                    name=utils.get_unmangled_name(ctx, source_name)
                    if param.kind is ast.Param.Kind.KeywordOnly
                    else "",
                )
            )
            placeholders.append(typed_argument(typ))
        name = utils.get_temporary_var(ctx, "dispatch.adapter")
        adapter = ast.FunctionStmt(
            name,
            ast.IdExpr(signature.ret_type.realized_name()),
            params,
            ast.ReturnStmt(expr=ast.CallExpr(ast.IdExpr(target.func_name), args)),
        )
        ctx.cache.scope(ast.SuiteStmt(adapter))
        adapter.set(ast.Attr.ExprTime, ctx.time)
        visitor.visit_stmt(adapter)
        call = visitor.visit_expr(ast.CallExpr(ast.IdExpr(name), placeholders))
        return call.expr.type.require_func

    def virtual_arguments(signature, receiver):
        args, idx = [], 0
        for param in signature.ast.items:
            stars, name = param.get_name_with_stars()
            if not param.is_value() or utils.get_unmangled_name(ctx, name).startswith("$"):
                continue
            typ = signature[idx]
            idx += 1
            typ = typ.literal.runtime_type if typ.literal else typ
            if stars == 1:
                args.extend(
                    ast.CallExpr.Arg(typed_argument(g.type)) for g in typ.require_cls.generics
                )
            elif stars == 2:
                names = ctx.cache.generated_tuple_names[typ.require_cls[0].require_int]
                args.extend(
                    ast.CallExpr.Arg(typed_argument(value), name=key)
                    for key, value in zip(names, typ.require_cls[1].require_cls)
                )
            else:
                args.append(
                    ast.CallExpr.Arg(
                        typed_argument(typ),
                        name=utils.get_unmangled_name(ctx, name)
                        if param.kind is ast.Param.Kind.KeywordOnly
                        else "",
                    )
                )
        args[0].value.type = receiver
        return args

    def super_arguments(signature, receiver):
        args = [ast.CallExpr.Arg(typed_argument(receiver))]
        args.extend(ast.CallExpr.Arg(typed_argument(typ)) for typ in signature[1].require_cls)
        names = ctx.cache.generated_tuple_names[signature[2].require_cls[0].require_int]
        args.extend(
            ast.CallExpr.Arg(typed_argument(value), name=key)
            for key, value in zip(names, signature[2].require_cls[1].require_cls)
        )
        return args

    def find_target(receiver, member, args, require_own):
        methods = utils.matching_methods(
            ctx, receiver, utils.find_method(ctx, receiver, member), args
        )
        return next(
            (
                method
                for method in methods
                if not require_own or method.ast.get(ast.Attr.ParentClass, str) == receiver.name
            ),
            None,
        )

    def realize_target(target, args):
        request = ast.CallExpr(ast.IdExpr(target.func_name), args)
        request.set(ast.Attr.Internal)
        call = visitor.visit_expr(request)
        assert infer.realize(ctx, call.type), "cannot realize dispatch return type"
        resolved = call.expr.type.require_func
        assert resolved.can_realize(), "bad dispatch realization"
        return resolved

    def descendants(base, identifier):
        for name in sorted(utils.get_class(ctx, base).descendants):
            for _, realization in sorted(utils.get_class(ctx, name).realizations.items()):
                if identifier not in realization.vtable:
                    yield realization

    added = True
    while added:
        added = False
        for _, cls in sorted(ctx.cache.classes.items()):
            for member in sorted(cls.virtuals):
                root = cls.methods.get(member)
                if not root:
                    continue
                for method_name in utils.get_overloads(ctx, root):
                    fn_data = utils.get_function(ctx, method_name)
                    for _, method_real in sorted(fn_data.realizations.items()):
                        method = method_real.type
                        if not utils.is_polymorphic(ctx, method):
                            continue
                        base = method[0].require_cls
                        key = vtable_signature(ctx, method)
                        identifier = ctx.cache.thunk_ids.setdefault(
                            key, len(ctx.cache.thunk_ids) + 1
                        )
                        for realization in list(descendants(base, identifier)):
                            receiver = realization.type
                            if receiver.realized_name() != base.realized_name() and not any(
                                parent.realized_name() == base.realized_name()
                                for parent in realization.bases
                            ):
                                continue
                            args = virtual_arguments(method, receiver)
                            target = find_target(
                                receiver,
                                utils.get_unmangled_name(ctx, method.func_name),
                                args,
                                False,
                            )
                            if not target:
                                raise TypecheckError(
                                    method.ast,
                                    f"no compatible virtual implementation of {key} for {receiver}",
                                )
                            if owner := target.ast.get(ast.Attr.ParentClass, str):
                                for parent in utils.get_mro(ctx, receiver):
                                    if parent.require_cls.name == owner:
                                        args[0].value.type = parent
                                        break
                            resolved = (
                                method
                                if target.func_name == method.func_name
                                else realize_target(target, args)
                            )
                            realization.vtable[identifier] = adapt_target(
                                method, receiver, resolved
                            )
                            added = True
        for identifier, signature in sorted(list(ctx.cache.super_calls.items())):
            lexical = signature[0].require_cls[0].require_cls
            member = utils.extract_func_generic(signature).require_str
            for realization in list(descendants(lexical, identifier)):
                mro = utils.get_mro(ctx, realization.type)
                anchor = next(
                    (
                        idx
                        for idx, base in enumerate(mro)
                        if base.realized_name() == lexical.realized_name()
                    ),
                    None,
                )
                if anchor is None:
                    continue
                for candidate in mro[anchor + 1 :]:
                    receiver = candidate.require_cls
                    args = super_arguments(signature, receiver)
                    target = find_target(receiver, member, args, True)
                    if target:
                        resolved = realize_target(target, args)
                        realization.vtable[identifier] = adapt_target(
                            ctx.cache.super_call_signatures[identifier], receiver, resolved
                        )
                        added = True
                        break
        added |= realize_value_methods(ctx, visitor)
    for _, fn in sorted(ctx.cache.functions.items()):
        for _, realization in sorted(fn.realizations.items()):
            if realization.check_initialization and realization.ast:
                classes.check_field_initialization(ctx, realization.type, realization.ast)


def realize_value_methods(ctx, visitor):
    """Realize frontend value adapters; backend TypeInfo emission remains unported."""
    requests = []
    for method, count, fallback in [
        ("__hash__", 1, "_unsupported_hash"),
        ("__eq__", 2, "_unsupported_equal"),
        ("value_dict", 1, ""),
    ]:
        owner = "TypeInfo" if method == "value_dict" else "Any"
        fn = utils.get_function(ctx, ast.types.mangle(cls=owner, func=method))
        if fn and fn.realizations:
            requests.append(
                (
                    "_dict_value"
                    if method == "value_dict"
                    else "_hash_value"
                    if method == "__hash__"
                    else "_equal_values",
                    count,
                    fallback,
                )
            )
    if not requests:
        return False
    pending = []
    for name, generic in [
        (ast.types.mangle(cls="TypeInfo", func="_get"), True),
        (ast.types.mangle("std.internal.types.rtti", func="get_typeinfo"), False),
    ]:
        fn = utils.get_function(ctx, name)
        for _, realization in sorted(fn.realizations.items()) if fn else []:
            pending.append(
                utils.extract_func_generic(realization.type) if generic else realization.type[0]
            )
    for _, cls in sorted(ctx.cache.classes.items()):
        pending.extend(
            real.type for _, real in sorted(cls.realizations.items()) if real.value_accessed
        )
    seen, added = set(), False
    while pending:
        typ = pending.pop().require_cls
        key = typ.realized_name()
        if key in seen or typ == ast.types.Stdlib.Any:
            continue
        seen.add(key)
        realization = utils.get_class_realization(ctx, typ)
        pending.extend(g.type for g in typ.generics if g.type and g.type.cls)
        pending.extend(field for _, field in realization.fields)
        for name in sorted(utils.get_class(ctx, typ).descendants):
            for _, child in sorted(utils.get_class(ctx, name).realizations.items()):
                if any(base.realized_name() == key for base in child.bases):
                    pending.append(child.type)
        for member, count, fallback in requests:
            if member == "_dict_value":
                root = utils.get_stdlib_type(ctx, ast.types.Stdlib.Dict)
                if not any(base.name == root.name for base in [typ, *realization.bases]):
                    continue
            identifier = value_method_id(ctx, member)
            if identifier in realization.value_methods:
                continue
            pointer = utils.get_class_field_types(
                ctx, utils.get_stdlib_type(ctx, ast.types.Stdlib.Any)
            )[0]
            args = [ast.NoneExpr(type=pointer, done=True) for _ in range(count)]
            args.append(ast.IdExpr(key))
            contexts = {id(imp.ctx): imp.ctx for imp in ctx.cache.imports.values() if imp.ctx}
            with ctx.binding_checkpoint_group(contexts.values()) as checkpoints:
                incomplete = {
                    (name, key)
                    for name, fn in ctx.cache.functions.items()
                    for key, real in fn.realizations.items()
                    if not real.ir
                }
                try:
                    call = visitor.visit_expr(
                        ast.CallExpr(
                            ast.IdExpr(ast.types.mangle(cls="TypeInfo", func=member)), args
                        )
                    )
                except TypecheckError:
                    checkpoints.rollback()
                    for name, fn in ctx.cache.functions.items():
                        for key, real in list(fn.realizations.items()):
                            if not real.ir and (name, key) not in incomplete:
                                del fn.realizations[key]
                    if not fallback:
                        raise
                    call = visitor.visit_expr(
                        ast.CallExpr(
                            ast.IdExpr(ast.types.mangle(cls="TypeInfo", func=fallback)), args
                        )
                    )
            realization.value_methods[identifier] = call.expr.type.require_func
            added = True
    return added


def generate_function_call_internal_ast(function: ast.types.Function):
    # Special case: Function.__call_internal__
    # TODO: move to IR one day
    llvm_lines, llvm_args = [], []
    arg_tuple = function[1].require_cls
    assert arg_tuple == ast.types.Stdlib.Tuple, f"bad function base: {function[1]!r}"
    arg_count = len(arg_tuple.generics)
    _, arg_name = function.ast.items[1].get_name_with_stars()
    items = []
    for idx in range(arg_count):
        llvm_lines.append(f"%{idx} = extractvalue {{}} %args, {idx}")
        items.append(ast.ExprStmt(ast.IdExpr(arg_name)))
    items.append(ast.ExprStmt(ast.IdExpr("TR")))
    for idx in range(arg_count):
        items.append(ast.ExprStmt(ast.IndexExpr(ast.IdExpr(arg_name), index=ast.IntExpr(idx))))
        llvm_args.append(f"{{}} %{idx}")
    items.append(ast.ExprStmt(ast.IdExpr("TR")))
    llvm_lines.append(f"%{arg_count} = call {{}} %self({', '.join(llvm_args)})")
    llvm_lines.append(f"ret {{}} %{arg_count}")
    items.insert(0, ast.ExprStmt(ast.StringExpr("\n".join(llvm_lines))))
    return ast.SuiteStmt(*items)


def generate_union_new_ast(function: ast.types.Function):
    parent = function.func_parent
    union = None if parent is None else parent.union
    assert union, f"expected union, got {parent}"
    return ast.SuiteStmt(
        ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_new")),
                items=[ast.IdExpr(function.ast.items[0].name), ast.IdExpr(union.realized_name())],
            )
        )
    )


def generate_union_tag_ast(ctx: TypeContext, function: ast.types.Function):
    # return Union._get_data(union, T0)
    tag = utils.extract_func_generic(function).require_int
    union = function[0].require_union
    union_types = union.get_realization_types()
    if tag < 0 or tag >= len(union_types):
        raise TypecheckError(ctx.node_stack[-1], "bad union tag")
    self_var = function.ast.items[0].name
    return ast.SuiteStmt(
        ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
                items=[ast.IdExpr(self_var), ast.IdExpr(union_types[tag].realized_name())],
            )
        )
    )


def generate_union_iterator_ast(ctx, fn):
    """Acquire the active iterator eagerly and join its element types."""
    from . import TypeVisitor

    visitor = TypeVisitor(ctx)
    alternatives = fn[0].require_union.get_realization_types()
    elements, iterable = {}, [False] * len(alternatives)
    for tag, alternative in enumerate(alternatives):
        ctx.add_block()
        try:
            name = utils.get_temporary_var(ctx, "iterable")
            ctx.add_item(name, name, alternative)
            value = ast.IdExpr(name, type=alternative)
            if not access.access_attribute(visitor, value, "__iter__")[1]:
                continue
            iterator = visitor.visit_expr(ast.CallExpr(ast.DotExpr(ast.IdExpr(name), "__iter__")))
            if iterator.type != ast.types.Stdlib.Generator:
                raise TypecheckError(ctx.info, "expected iterable expression")
            element = iterator.type.require_cls[0]
            elements[element.realized_name()] = element
            iterable[tag] = True
        finally:
            ctx.pop_block()
    element_type = (
        ast.InstantiateExpr(
            ast.IdExpr(ast.types.Stdlib.Union), [ast.IdExpr(name) for name in sorted(elements)]
        )
        if len(elements) > 1
        else None
    )
    suite = ast.SuiteStmt()
    for tag, alternative in enumerate(alternatives):
        if not iterable[tag]:
            continue
        value = ast.CallExpr(
            ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
            [ast.IdExpr("union"), ast.IdExpr(alternative.realized_name())],
        )
        iterator = ast.CallExpr(ast.DotExpr(value, "__iter__"))
        if element_type:
            iterator = ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_iter_union")),
                [iterator, element_type.clone()],
            )
        condition = ast.BinaryExpr(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")), [ast.IdExpr("union")]
            ),
            "==",
            ast.IntExpr(tag),
        )
        suite.add(ast.IfStmt(condition, ast.ReturnStmt(expr=iterator)))
    suite.add(fn.ast.suite.clone())
    return suite


def generate_union_binary_ast(ctx, fn):
    """Dispatch native binary operators for each active pair, joining distinct results."""
    from . import TypeVisitor, ops

    operation = utils.extract_func_generic(fn).str
    if operation is None:
        return None
    visitor = TypeVisitor(ctx)
    left, right = fn[0], fn[1]
    left_types = left.require_union.get_realization_types() if left.union else [left]
    right_types = right.require_union.get_realization_types() if right.union else [right]
    candidates, results = [], {}
    for left_tag, lt in enumerate(left_types):
        for right_tag, rt in enumerate(right_types):
            ctx.add_block()
            try:
                ln, rn = utils.get_temporary_var(ctx, "left"), utils.get_temporary_var(ctx, "right")
                ctx.add_item(ln, ln, lt)
                ctx.add_item(rn, rn, rt)
                compared = ops.transform_binary_magic(
                    visitor,
                    ast.BinaryExpr(
                        visitor.visit_expr(ast.IdExpr(ln)),
                        operation,
                        visitor.visit_expr(ast.IdExpr(rn)),
                    ),
                )
                if not compared:
                    continue
                result = compared.type
                assert result.can_realize(), f"cannot realize {result!r}"
                results[result.realized_name()] = result
            finally:
                ctx.pop_block()
            values, condition = [], None
            for name, original, selected, tag in [
                ("left", left, lt, left_tag),
                ("right", right, rt, right_tag),
            ]:
                value = ast.IdExpr(name)
                if original.union:
                    value = ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
                        [value, ast.IdExpr(selected.realized_name())],
                    )
                    matches = ast.BinaryExpr(
                        ast.CallExpr(
                            ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")),
                            [ast.IdExpr(name)],
                        ),
                        "==",
                        ast.IntExpr(tag),
                    )
                    condition = ast.BinaryExpr(condition, "&&", matches) if condition else matches
                values.append(value)
            candidates.append((condition, ast.BinaryExpr(values[0], operation, values[1])))
    result_type = (
        ast.InstantiateExpr(
            ast.IdExpr(ast.types.Stdlib.Union), [ast.IdExpr(name) for name in sorted(results)]
        )
        if len(results) > 1
        else None
    )
    suite = ast.SuiteStmt()
    for condition, value in candidates:
        if result_type:
            value = ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_new")), [value, result_type.clone()]
            )
        suite.add(ast.IfStmt(condition, ast.ReturnStmt(expr=value)))
    suite.add(fn.ast.suite.clone())
    return suite


def generate_union_dispatch_ast(ctx, fn):
    """Dispatch receiver and argument alternatives, preserving explicitly accepted unions."""
    from . import TypeVisitor

    attr = utils.extract_func_generic(fn).str
    if attr is None:
        return None
    visitor = TypeVisitor(ctx)
    union = fn[0].require_union
    suite = utils.get_function(ctx, fn.func_name).ast.suite.clone()
    is_call = fn[1] == ast.types.Stdlib.Tuple and fn[2] == ast.types.Stdlib.NamedTuple
    arguments = []
    if is_call:
        arguments.extend(("", typ) for typ in fn[1].require_cls)
        keywords = fn[2].require_cls
        names = ctx.cache.generated_tuple_names[keywords[0].require_int]
        arguments.extend(zip(names, keywords[1].require_cls))

    def argument_value(index, selected, args, kwargs):
        name, original = arguments[index]
        value = (
            ast.IndexExpr(ast.IdExpr(kwargs), ast.StringExpr(name))
            if name
            else ast.IndexExpr(ast.IdExpr(args), ast.IntExpr(index))
        )
        if selected is not None and selected is not original and original.union:
            value = ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
                [value, ast.IdExpr(selected.realized_name())],
            )
        return value

    candidates, results = [], {}
    for tag, typ in enumerate(union.get_realization_types()):
        condition = ast.BinaryExpr(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")), [ast.IdExpr("union")]
            ),
            "==",
            ast.IntExpr(tag),
        )
        receiver = ast.CallExpr(
            ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
            [ast.IdExpr("union"), ast.IdExpr(typ.realized_name())],
        )
        if is_call:
            signature_args = [
                ast.CallExpr.Arg(ast.NoneExpr(type=actual), name=name) for name, actual in arguments
            ]
            preserve = [True] * len(arguments)
            matched = False

            def inspect_signature(function, partial):
                nonlocal matched
                accepted = []
                if utils.can_call(ctx, function, signature_args, partial, accepted) < 0:
                    return
                matched = True
                preserve[:] = [old and new for old, new in zip(preserve, accepted)]

            if typ.partial:
                inspect_signature(
                    utils.instantiate(ctx, typ.partial_func.generalize(0)), typ.partial
                )
            else:
                for method in utils.find_method(ctx, typ.require_cls, "__call__"):
                    if method:
                        inspect_signature(utils.instantiate(ctx, method, typ.require_cls), None)
            if not matched:
                preserve[:] = [False] * len(arguments)
            selected = [None] * len(arguments)

            def dispatch(index, selected_condition):
                if index < len(arguments):
                    original = arguments[index][1]
                    if original.union and not preserve[index]:
                        for arg_tag, member in enumerate(
                            original.require_union.get_realization_types()
                        ):
                            selected[index] = member
                            matches = ast.BinaryExpr(
                                ast.CallExpr(
                                    ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")),
                                    [argument_value(index, None, "args", "kwargs")],
                                ),
                                "==",
                                ast.IntExpr(arg_tag),
                            )
                            dispatch(
                                index + 1, ast.BinaryExpr(selected_condition.clone(), "&&", matches)
                            )
                    else:
                        selected[index] = original
                        dispatch(index + 1, selected_condition)
                    return
                probe = [
                    ast.CallExpr.Arg(ast.NoneExpr(type=value), name=name)
                    for (name, _), value in zip(arguments, selected)
                ]
                if typ.partial:
                    function = utils.instantiate(ctx, typ.partial_func.generalize(0))
                    if utils.can_call(ctx, function, probe, typ.partial) < 0:
                        return
                elif not utils.matching_methods(
                    ctx, typ.require_cls, utils.find_method(ctx, typ.require_cls, "__call__"), probe
                ):
                    return
                ctx.add_block()
                try:
                    var, var_args, var_kwargs = [
                        utils.get_temporary_var(ctx, name) for name in ("union", "args", "kwargs")
                    ]
                    ctx.add_item(var, var, typ)
                    ctx.add_item(var_args, var_args, fn[1])
                    ctx.add_item(var_kwargs, var_kwargs, fn[2])
                    values = [
                        ast.CallExpr.Arg(
                            argument_value(i, value, var_args, var_kwargs), name=arguments[i][0]
                        )
                        for i, value in enumerate(selected)
                    ]
                    result = visitor.visit_expr(ast.CallExpr(ast.IdExpr(var), values)).type
                    assert result.can_realize(), f"cannot realize {result!r}"
                    results[result.realized_name()] = result
                finally:
                    ctx.pop_block()
                values = [
                    ast.CallExpr.Arg(
                        argument_value(i, value, "args", "kwargs"), name=arguments[i][0]
                    )
                    for i, value in enumerate(selected)
                ]
                candidates.append((selected_condition, ast.CallExpr(receiver.clone(), values)))

            dispatch(0, condition)
            continue
        ctx.add_block()
        try:
            name = utils.get_temporary_var(ctx, "union")
            ctx.add_item(name, name, typ)
            identifier = ast.IdExpr(name, type=typ)
            result = None
            if access.access_attribute(visitor, identifier, attr)[1]:
                result = visitor.visit_expr(ast.DotExpr(identifier, attr)).type
        finally:
            ctx.pop_block()
        if result:
            assert result.can_realize(), f"cannot realize {result!r}"
            results[result.realized_name()] = result
            candidates.append((condition, ast.DotExpr(receiver, attr)))
    wrapper = (
        ast.InstantiateExpr(
            ast.IdExpr(ast.types.Stdlib.Union), [ast.IdExpr(name) for name in sorted(results)]
        )
        if len(results) > 1
        else None
    )
    for condition, value in candidates:
        if wrapper:
            value = ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_new")), [value, wrapper]
            )
        suite.add(ast.IfStmt(condition, ast.ReturnStmt(expr=value)))
    suite.add(suite.items[0])
    suite.items[0] = None
    return suite


def generate_named_keys_ast(ctx: TypeContext, function: ast.types.Function):
    idx = utils.extract_func_generic(function).require_int
    if idx < 0 or idx >= len(ctx.cache.generated_tuple_names):
        raise TypecheckError(ctx.node_stack[-1], "bad namedkeys idx")
    values: List[ast.Expr] = [ast.StringExpr(name) for name in ctx.cache.generated_tuple_names[idx]]
    return ast.SuiteStmt(ast.ReturnStmt(expr=ast.TupleExpr(values)))


def generate_tuple_mul_ast(function: ast.types.Function) -> ast.SuiteStmt | None:
    count = max(0, utils.extract_func_generic(function).require_int)
    tuple_type = function[0].cls
    if not tuple_type or tuple_type != ast.types.Stdlib.Tuple:
        return None
    items: List[ast.Expr] = [
        ast.IndexExpr(ast.IdExpr(function.ast.items[0].name), index=ast.IntExpr(idx))
        for _ in range(count)
        for idx in range(len(tuple_type.generics))
    ]
    return ast.SuiteStmt(ast.ReturnStmt(expr=ast.TupleExpr(items)))


def generate_special_ast(ctx: TypeContext, function: ast.types.Function) -> ast.SuiteStmt | None:
    """Generate ASTs for dynamically generated functions."""

    # Clone the generic AST that is to be realized
    ast_node = function.ast
    if (
        ast_node.has(ast.Attr.AutoGenerated)
        and ast_node.name.endswith(".__iter__:0")
        and utils.is_heterogenous(ctx, function[0].require_cls)
    ):
        # Special case: do not realize auto-generated heterogenous __iter__
        raise TypecheckError(ctx.node_stack[-1], "expected iterable expression")
    if (
        ast_node.has(ast.Attr.AutoGenerated)
        and ast_node.name.endswith(".__getitem__:0")
        and utils.is_heterogenous(ctx, function[0].require_cls)
    ):
        # Special case: do not realize auto-generated heterogenous __getitem__
        raise TypecheckError(ctx.node_stack[-1], "expected iterable expression")
    if ast_node.name.startswith("Function.__call_internal__"):
        return generate_function_call_internal_ast(function)
    if ast_node.name.startswith("Union.__new__"):
        return generate_union_new_ast(function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="_tag")):
        return generate_union_tag_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="_dispatch")):
        return generate_union_dispatch_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="__iter__")):
        return generate_union_iterator_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="_binary")):
        return generate_union_binary_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle("", "NamedTuple", "_namedkeys")):
        return generate_named_keys_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle("", "__magic__", "mul")):
        return generate_tuple_mul_ast(function)
    if ast_node.name.startswith(ast.types.mangle(cls="TypeInfo", func="_init_params")):
        return generate_type_info_init_ast(ctx, function)
    if ast_node.name.startswith(ast.types.mangle(cls="TypeInfo", func="_value_method_id")):
        return ast.SuiteStmt(
            ast.ReturnStmt(
                expr=ast.IntExpr(
                    value_method_id(ctx, utils.extract_func_generic(function).require_str)
                )
            )
        )
    if ast_node.name.startswith(ast.types.mangle("", "Super", "_dispatch")):
        return generate_super_dispatch_ast(ctx, function)
    return None


def transform_named_tuple(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Transform named tuples.
    @example
    `namedtuple("NT", ["a", ("b", int)])` -> ```@tuple
    class NT[T1]:
    a: T1
    b: int```
    """

    # Ensure that namedtuple call is valid
    assert expr.expr.type
    name = utils.extract_func_generic(expr.expr.type).require_str
    if len(expr.items) != 1:
        raise TypecheckError(expr, "namedtuple() takes 2 static arguments")
    # Construct the class statement
    generics, params = [], []
    original = expr.items[0].value.orig
    if not isinstance(original, ast.TupleExpr):
        raise TypecheckError(expr, "namedtuple() takes 2 static arguments")
    type_idx = 1
    for item in original.items:
        if isinstance(item, ast.StringExpr):
            generic_name = f"T{type_idx}"
            generics.append(
                ast.Param(
                    generic_name,
                    type=ast.IdExpr(ast.types.Stdlib.Type),
                    status=ast.Param.Status.Generic,
                )
            )
            params.append(ast.Param(item.get_value(), type=ast.IdExpr(generic_name)))
            type_idx += 1
            continue
        if (
            isinstance(item, ast.TupleExpr)
            and len(item.items) == 2
            and isinstance(item.items[0], ast.StringExpr)
        ):
            type_expr = self.visit_expr(item.items[1], enforce_type=True)
            params.append(ast.Param(item.items[0].get_value(), type=type_expr))
            continue
        raise TypecheckError(expr, "namedtuple() takes 2 static arguments")
    params.extend(generics)
    class_suite = ast.SuiteStmt(
        ast.ClassStmt(name, items=params, base_classes=[ast.IdExpr("tuple")])
    )
    self.ctx.cache.scope(class_suite)
    class_suite = self.visit_stmt(class_suite)
    self.ctx.prepend_stmts[-1].append(class_suite)
    return self.visit_expr(ast.IdExpr(name), enforce_type=True)


def transform_functools_partial(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Transform partial calls (Python syntax).
    @example
    `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
    """

    if not expr.items:
        raise TypecheckError(expr, "partial() takes 1 or more arguments")
    args = list(expr.items[1:])
    args.append(ast.CallExpr.Arg(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial)))
    return self.visit_expr(ast.CallExpr(expr.items[0].value, args))


def transform_super_f(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Typecheck superf method. This method provides the access to the previous matching
    overload.
    @example
    ```class cls:
    def foo(): print('foo 1')
    def foo():
    superf()  # access the previous foo
    print('foo 2')
    cls.foo()```
    prints "foo 1" followed by "foo 2"
    """

    base = self.ctx.base
    assert base.type and base.type.func
    fn_type = base.type.func
    # Find list of matching superf methods
    supers = []
    if not utils.is_dispatch_type(fn_type):
        if parent_name := fn_type.ast.attributes.get(ast.Attr.ParentClass):
            assert isinstance(parent_name, str)
            parent_cls_data = utils.get_class(self.ctx, parent_name)
            if parent_cls_data:
                if method_root := parent_cls_data.methods.get(
                    utils.get_unmangled_name(self.ctx, fn_type.func_name)
                ):
                    for overload in utils.get_overloads(self.ctx, method_root):
                        if utils.is_dispatch(overload):
                            continue
                        if overload == fn_type.func_name:
                            break
                        overload_fn = utils.get_function(self.ctx, overload)
                        if overload_fn and overload_fn.type:
                            supers.append(overload_fn.type)
                    supers.reverse()
    if not supers:
        raise TypecheckError(expr, "no superf methods found")
    assert len(expr.items) == 1 and isinstance(expr.items[0].value, ast.CallExpr), "bad superf call"
    inner_call = expr.items[0].value

    new_args = [ast.CallExpr.Arg(arg.value) for arg in inner_call.items]
    methods = utils.matching_methods(
        self.ctx,
        fn_type.func_parent.cls if fn_type.func_parent else None,
        supers,
        new_args,
    )
    if not methods:
        raise TypecheckError(expr, "no superf methods found")
    return self.visit_expr(ast.CallExpr(ast.IdExpr(methods[0].func_name), new_args))


def transform_super(self: TypeVisitor) -> ast.Expr:
    """
    Typecheck and transform super method. Replace it with the current self object cast
    to the first inherited type.
    TODO: only an empty super() is currently supported.
    """

    base = self.ctx.base
    if base is None or base.type is None:
        raise TypecheckError(self.ctx.node_stack[-1], "no super methods found")
    fn_type = base.type.func
    if not fn_type or not fn_type.ast or not fn_type.ast.has(ast.Attr.Method) or len(fn_type) == 0:
        raise TypecheckError(self.ctx.node_stack[-1], "no super methods found")
    self_type = fn_type[0].require_cls
    self_expr = ast.IdExpr(fn_type.ast.items[0].name, type=self_type)
    lexical = fn_type.func_parent.cls if fn_type.func_parent else self_type
    type_expr = ast.IdExpr(lexical.name, type=utils.instantiate_type_var(self.ctx, lexical))
    return self.visit_expr(
        ast.CallExpr(
            ast.IdExpr(ast.types.mangle(cls="Super", func="__new__")), items=[type_expr, self_expr]
        )
    )


def transform_ptr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
    the arg is a variable binding.
    """

    arg = expr.items[0].value = self.visit_expr(expr.items[0].value)
    head = arg
    while True:
        try:
            utils.extract_class_type(self.ctx, head)
        except (AssertionError, TypeError):
            return None
        if isinstance(head, ast.IdExpr):
            value = self.ctx.find_at(head.value, self.ctx.time)
            if value is None or not value.is_var():
                raise TypecheckError(
                    expr, "__ptr__() only takes identifiers or tuple fields as arguments"
                )
            break
        if isinstance(head, ast.DotExpr) and head.expr:
            head = head.expr
        else:
            raise TypecheckError(
                expr, "__ptr__() only takes identifiers or tuple fields as arguments"
            )

    assert expr.type and arg.type
    expr.type |= utils.instantiate(
        self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr), [arg.type]
    )
    if arg.done:
        expr.done = True
    return None


def transform_array(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Typecheck __array__ method. This method creates a stack-allocated array via alloca."""

    assert expr.type and expr.expr.type
    parent_type = expr.expr.type.require_func.func_parent
    assert parent_type
    expr.type |= utils.instantiate(
        self.ctx,
        utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr),
        [parent_type.require_cls[0]],
    )
    if infer.realize(self.ctx, expr.type):
        expr.done = True
    return None


def transform_is_instance(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Transform isinstance method to a static boolean expression.
    Special cases:
    `isinstance(obj, ByVal)` is True if `type(obj)` is a tuple type
    `isinstance(obj, ByRef)` is True if `type(obj)` is a reference type
    """

    assert expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool

    # again to realize it
    obj_arg = expr.items[0].value = self.visit_expr(expr.items[0].value)
    assert obj_arg.type
    obj_type = obj_arg.type.require_cls
    if not obj_type.can_realize():
        return None
    obj_arg = expr.items[0].value = self.visit_expr(obj_arg)  # again to realize it

    obj_type = utils.extract_class_type(self.ctx, obj_type)
    type_expr = expr.items[1].value
    match type_expr:
        # Handle `isinstance(obj, (type1, type2, ...))`
        case ast.CallExpr(orig_expr=ast.TupleExpr(items=items)):
            result = self.visit_expr(ast.BoolExpr(False))
            for item in items:
                result = self.visit_expr(
                    ast.BinaryExpr(
                        result,
                        op="||",
                        rexpr=ast.CallExpr(ast.IdExpr("isinstance"), items=[obj_arg, item]),
                    )
                )
            return result

    target_type = utils.extract_type(self.ctx, type_expr)
    if utils.is_type_expr(type_expr) and target_type == ast.types.Stdlib.Type:
        return self.visit_expr(
            ast.BoolExpr(utils.is_type_expr(obj_arg) or obj_type == ast.types.Stdlib.TypeWrap)
        )
    match type_expr:
        case ast.IdExpr(value="type"):
            return self.visit_expr(ast.BoolExpr(utils.is_type_expr(obj_arg)))
        case ast.IdExpr(value="type[Tuple]"):
            return self.visit_expr(ast.BoolExpr(obj_type == ast.types.Stdlib.Tuple))
        case ast.IdExpr(value="type[ByVal]"):
            return self.visit_expr(ast.BoolExpr(obj_type.is_tuple))
        case ast.IdExpr(value="type[ByRef]"):
            return self.visit_expr(ast.BoolExpr(not obj_type.is_tuple))
        case _ if not target_type.union and (union := obj_type.union):
            union_types = union.get_realization_types()
            if not utils.is_type_expr(obj_arg) and any(
                member == ast.types.Stdlib.Optional for member in union_types
            ):
                return self.visit_expr(
                    ast.CallExpr(
                        ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Union), "_isinstance"),
                        [obj_arg, type_expr],
                    )
                )
            matching_tag = -1
            for idx, union_type in enumerate(union_types):
                undo = ast.types.Type.UnifyContext()
                score = target_type.unify(union_type, undo)
                undo.undo()
                if score >= 0:
                    matching_tag = idx
                    break
            if matching_tag == -1:
                return self.visit_expr(ast.BoolExpr(False))
            return self.visit_expr(
                ast.BinaryExpr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")), [obj_arg]
                    ),
                    "==",
                    ast.IntExpr(matching_tag),
                )
            )
        case _ if type_expr.type == "pyobj":
            if obj_type == "pyobj":
                return self.visit_expr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle("std.internal.python", func="_isinstance")),
                        items=[obj_arg, type_expr],
                    )
                )
            else:
                return self.visit_expr(ast.BoolExpr(False))

    type_expr = expr.items[1].value = self.visit_expr(type_expr, enforce_type=True)
    target_type = utils.extract_type(self.ctx, type_expr).require_cls

    # Check type match
    if target_type.name == obj_type.name:
        undo = ast.types.Type.UnifyContext()
        score = obj_type.unify(target_type, undo)
        undo.undo()
        if score >= 0:
            return self.visit_expr(ast.BoolExpr(True))

    instance_call = "_isinstance"
    if obj_type == ast.types.Stdlib.Any and not utils.is_type_expr(obj_arg):
        return self.visit_expr(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Any", func=instance_call)),
                items=[obj_arg, type_expr],
            )
        )

    # Check RTTI super types
    target_data = utils.get_class(self.ctx, target_type)
    obj_data = utils.get_class(self.ctx, obj_type)
    assert target_data and obj_data
    if target_data and obj_data and target_data.has_rtti() and obj_data.has_rtti():
        obj_data = utils.get_class(self.ctx, obj_type)
        assert obj_data
        for base in obj_data.mro:
            typ = utils.instantiate(self.ctx, base, obj_type)
            if mro_type := infer.realize(self.ctx, typ):
                if mro_type | target_type:
                    return self.visit_expr(ast.BoolExpr(True))

        # Check whether the target can be a subtype of the value's static type. Besides
        # rejecting unrelated classes, this specializes target generics through its base
        # type (e.g. Future[int] against Task[R] binds R to int).
        for target_base in utils.get_mro(self.ctx, target_type):
            undo = ast.types.Type.UnifyContext()
            if target_base.unify(obj_type, undo) >= 0:
                return self.visit_expr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="RTTIType", func=instance_call)),
                        items=[obj_arg, type_expr],
                    )
                )
            undo.undo()

    return self.visit_expr(ast.BoolExpr(False))


def transform_static_len(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Transform staticlen method to a static integer expression. This method supports only
    static strings and tuple types.
    """

    assert expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Int

    expr.items[0].value = self.visit_expr(expr.items[0].value)
    typ = utils.extract_type(self.ctx, expr.items[0].value)
    if (static := typ.str) is not None:
        # Case: staticlen on static strings
        return self.visit_expr(ast.IntExpr(len(static)))
    if union := typ.union:
        if infer.realize(self.ctx, typ):
            return self.visit_expr(ast.IntExpr(len(union.get_realization_types())))
        return None
    typ = typ.cls
    if not typ:
        return None
    if not typ.is_tuple:
        raise TypecheckError(expr, "expected tuple type")
    return self.visit_expr(ast.IntExpr(len(utils.get_class_fields(self.ctx, typ))))


def transform_has_attr(
    self: TypeVisitor, expr: ast.CallExpr, allow_dynamic: bool = False
) -> ast.Expr | None:
    """
    Transform hasattr method to a static boolean expression.
    This method also supports additional arg types that are used to check
    for a matching overload (not available in Python).
    """

    assert expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ == ast.types.Stdlib.TypeWrap:
        typ = typ[0].cls
    if typ is None:
        return None

    assert expr.expr.type
    attr = utils.extract_func_generic(expr.expr.type).require_str
    arg_types = [("", cast(ast.types.Type, typ))]
    positional = expr.items[1].value
    if isinstance(positional, ast.CallExpr):
        for arg in positional.items:
            arg.value = self.visit_expr(arg.value)
            if arg.value.cls is None:
                return None
            arg_type = utils.extract_type(self.ctx, arg.value)
            if arg_type == ast.types.Stdlib.TypeWrap:
                arg_type = arg_type.require_cls[0]
            arg_types.append(("", arg_type))
    for name, named_expr in utils.extract_named_tuple(self.ctx, expr.items[2].value):
        named_expr = self.visit_expr(named_expr)
        named_type = utils.extract_type(self.ctx, named_expr)
        if named_type == ast.types.Stdlib.TypeWrap:
            named_type = named_type.require_cls[0]
        arg_types.append((name, named_type))

    if (union := typ.union) and allow_dynamic:
        condition = None
        for typ in union.get_realization_types():
            if not (typ := infer.realize(self.ctx, typ)):
                return None
            type_expr = ast.IdExpr(typ.realized_name())
            branch = ast.BinaryExpr(
                ast.CallExpr(ast.IdExpr("isinstance"), items=[expr.items[0].value, type_expr]),
                op="&&",
                rexpr=ast.CallExpr(ast.IdExpr("hasattr"), items=[type_expr, ast.StringExpr(attr)]),
            )
            condition = (
                branch if condition is None else ast.BinaryExpr(condition, op="||", rexpr=branch)
            )
        return self.visit_expr(ast.BoolExpr(False) if condition is None else condition)

    if typ == ast.types.Stdlib.NamedTuple:
        if not typ.can_realize():
            return None
        tuple_id = typ[0].require_int
        assert 0 <= tuple_id < len(self.ctx.cache.generated_tuple_names)
        return self.visit_expr(ast.BoolExpr(attr in self.ctx.cache.generated_tuple_names[tuple_id]))

    exists = bool(
        utils.find_method(self.ctx, typ, attr)
        or utils.find_member(self.ctx, typ, attr)
        or attr in utils.get_class(self.ctx, typ).class_vars
    )
    if exists and len(arg_types) > 1:
        call_args = []
        for name, arg_type in arg_types:
            call_args.append(ast.CallExpr.Arg(ast.NoneExpr(type=arg_type), name=name))
        methods = utils.find_method(self.ctx, typ, attr)
        exists = bool(utils.matching_methods(self.ctx, typ, methods, call_args))

    cls_data = utils.get_class(self.ctx, typ)
    if not exists and allow_dynamic and cls_data and cls_data.has_rtti():
        return self.visit_expr(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_hasattr")),
                items=[expr.items[0].value, ast.StringExpr(attr)],
            )
        )
    return self.visit_expr(ast.BoolExpr(exists))


def transform_get_attr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform getattr method to a DotExpr."""

    assert expr.type and expr.expr.type
    attr = utils.extract_func_generic(expr.expr.type).require_str
    attr_type = utils.extract_type(self.ctx, utils.extract_func_generic(expr.expr.type, 1))
    result, found = access.access_attribute(self, expr.items[0].value, attr)
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ == ast.types.Stdlib.TypeWrap:
        typ = typ[0]
    if not found:
        cls_data = utils.get_class(self.ctx, typ)
        if cls_data and cls_data.has_rtti():
            return self.visit_expr(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_getattr")),
                    items=[
                        expr.items[0].value,
                        ast.StringExpr(attr),
                        ast.IdExpr(attr_type.realized_name()),
                    ],
                )
            )
        raise TypecheckError(
            expr,
            f"'{utils.extract_type(self.ctx, expr.items[0].value)}' "
            f"object has no attribute '{attr}'",
        )
    if result is None:
        result = self.visit_expr(ast.DotExpr(expr.items[0].value, member=attr))
    if attr_type != ast.types.Stdlib.NoneType:
        can_wrap, result = utils.wrap_expr(self.ctx, result, attr_type)
        if can_wrap:
            assert result.type
            result.type |= attr_type
    return result


def transform_set_attr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Transform setattr method to a AssignMemberStmt."""

    assert expr.expr.type
    attr = utils.extract_func_generic(expr.expr.type).require_str
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ == (ast.types.Stdlib.TypeWrap):
        typ = typ[0]
    cls_data = utils.get_class(self.ctx, typ)
    if cls_data and cls_data.has_rtti():
        _, found = access.access_attribute(self, expr.items[0].value, attr)
        if not found:
            return self.visit_expr(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_setattr")),
                    items=[expr.items[0].value, ast.StringExpr(attr), expr.items[1].value],
                )
            )
    return self.visit_expr(
        ast.StmtExpr(
            ast.AssignMemberStmt(expr.items[0].value, member=attr, rhs=expr.items[1].value),
            expr=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType)),
        )
    )


def transform_compile_error(expr: ast.CallExpr) -> ast.Expr | None:
    """Raise a compiler error."""

    assert expr.expr.type
    message = utils.extract_func_generic(expr.expr.type).require_str
    raise TypecheckError(expr, message)


def transform_tuple_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Convert a class to a tuple."""

    for arg in expr.items:
        arg.value = self.visit_expr(arg.value)
    cls_type = utils.extract_class_type(self.ctx, expr.items[0].value)
    # tuple(ClassType) is a tuple type that corresponds to a class
    if utils.is_type_expr(expr.items[0].value):
        if infer.realize(self.ctx, cls_type) is None:
            return expr
        items = []
        field_types = utils.get_class_field_types(self.ctx, cls_type)
        cls_data = utils.get_class(self.ctx, cls_type)
        assert cls_data
        for idx, field_type in enumerate(field_types):
            realized_type = infer.realize(self.ctx, field_type)
            assert realized_type is not None, (
                f"cannot realize '{cls_data.fields[idx].name}' in {cls_type!r}"
            )
            items.append(ast.IdExpr(realized_type.realized_name()))
        return self.visit_expr(ast.InstantiateExpr(ast.IdExpr(ast.types.Stdlib.Tuple), items=items))
    items = []
    var = utils.get_temporary_var(self.ctx, "tup")
    for field in utils.get_class_fields(self.ctx, cls_type):
        items.append(ast.DotExpr(ast.IdExpr(var), member=field.name))
    return self.visit_expr(
        ast.StmtExpr(
            ast.AssignStmt(ast.IdExpr(var), rhs=expr.items[0].value), expr=ast.TupleExpr(items)
        )
    )


def transform_type_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform type function to a type IdExpr identifier."""

    expr.items[0].value = self.visit_expr(expr.items[0].value)
    assert expr.type and expr.items[0].value.type
    value = expr.items[0].value
    value_type = value.type
    if value_type == ast.types.Stdlib.Optional and value_type.has_unbounds(False):
        return None
    if utils.is_type_expr(value) or value_type == ast.types.Stdlib.TypeWrap:
        value_type = utils.get_metaclass_type(self.ctx)
    expr.type |= utils.instantiate_type_var(self.ctx, value_type)
    if infer.realize(self.ctx, expr.type) is None:
        return None
    return ast.IdExpr(expr.type.realized_name(), type=expr.type, done=True)


def transform_realized_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform static.realized function to a fully realized type identifier."""

    fn_type = utils.extract_type(self.ctx, expr.items[0].value.type)
    if static := fn_type.str:
        # First arg can just be a literal string of function canonical name
        value = self.ctx.get(static)
        if value and value.func:
            fn_type = utils.instantiate(self.ctx, value.type)
    else:
        if (
            fn_type.func is None
            and expr.items[0].value.type
            and (partial := expr.items[0].value.type.partial)
            and partial.is_partial_empty
        ):
            generalized = partial.partial_func.generalize(0)
            fn_type = utils.instantiate(self.ctx, generalized)

    fn_type = fn_type.func
    if not fn_type:
        raise TypecheckError(expr, "static.realized() only takes functions as a first arg")

    assert expr.items[1].value.type
    args_type = expr.items[1].value.type.cls
    if not args_type:
        return None
    assert args_type.name == ast.types.Stdlib.Tuple
    call_args = [
        ast.CallExpr.Arg(
            ast.NoneExpr(type=(arg.require_cls[0] if arg == ast.types.Stdlib.TypeWrap else arg))
        )
        for arg in args_type
    ]
    changes = ast.types.Type.UnifyContext()
    if utils.can_call(self.ctx, fn_type, call_args, undo=changes) < 0:
        raise TypecheckError(
            expr.items[0].value, "static.realized() only takes functions as a first arg"
        )
    if realized := infer.realize(self.ctx, fn_type):
        return ast.IdExpr(realized.realized_name(), type=realized, done=True)
    return None


def transform_static_print_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform __static_print__ function to a fully realized type identifier."""

    import sys

    for arg in cast(ast.CallExpr, expr.items[0].value):
        arg_type = (
            None
            if arg.value is None or not isinstance(arg.value.type, ast.types.Type)
            else arg.value.type
        )
        debug_value = "-" if arg_type is None else repr(arg_type)
        realized_name = "-" if arg_type is None else arg_type.realized_name()
        static_suffix = " [static]" if arg_type and arg_type.literal else ""
        print(
            f"[print] {self.ctx.info}: {debug_value} ({realized_name}){static_suffix}",
            file=sys.stderr,
        )
    return None


def transform_has_rtti_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform static.has_rtti to a static boolean that indicates RTTI status of a type."""

    assert expr.type and expr.expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool
    arg_type = utils.extract_func_generic(expr.expr.type).cls
    if not arg_type:
        return None

    cls_data = utils.get_class(self.ctx, arg_type)
    assert cls_data
    return self.visit_expr(ast.BoolExpr(cls_data.has_rtti()))


def transform_static_fn_can_call(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Transform internal.static calls"""

    assert expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool

    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    input_args = utils.unpack_tuple_types(self, expr.items[1].value)
    keyword_args = utils.unpack_tuple_types(self, expr.items[2].value)
    assert input_args is not None and keyword_args is not None, "bad call to fn_can_call"

    call_args = []
    for name, arg_type in [*input_args, *keyword_args]:
        call_args.append(ast.CallExpr.Arg(ast.NoneExpr(type=arg_type), name=name))
    if typ == ast.types.Stdlib.Function or typ.partial or typ == ast.types.Stdlib.TypeWrap:
        return self.visit_expr(ast.BoolExpr(utils.match_callable(self.ctx, typ, call_args) >= 0))

    print("cannot use fn_can_call on non-functions")
    return self.visit_expr(ast.BoolExpr(False))


def transform_static_fn_arg_has_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool

    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type:
        raise TypecheckError(expr, f"expected a function, got '{expr.items[0].value.type}'")
    idx = utils.extract_func_generic(expr.expr.type).require_int
    can_realize = 0 <= idx < len(fn_type) and fn_type[idx] and fn_type[idx].can_realize()
    return self.visit_expr(ast.BoolExpr(can_realize))


def transform_static_fn_arg_get_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type:
        raise TypecheckError(expr, f"expected a function, got '{expr.items[0].value.type}'")
    idx = utils.extract_func_generic(expr.expr.type).require_int
    arg_type = None if idx < 0 or idx >= len(fn_type) else fn_type[idx]
    if arg_type is None or not arg_type.can_realize():
        raise TypecheckError(expr, "arg does not have type")
    return self.visit_expr(ast.IdExpr(arg_type.realized_name()))


def transform_static_fn_args(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(expr, f"expected a function, got '{expr.items[0].value.type}'")
    values = []
    for arg in fn_type.ast.items:
        if arg.is_generic():
            continue
        _, name = arg.get_name_with_stars()
        values.append(ast.StringExpr(utils.get_unmangled_name(self.ctx, name)))
    return self.visit_expr(ast.TupleExpr(values))


def transform_static_fn_has_default(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool

    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(
            expr,
            f"expected a function, got '{expr.items[0].value.type}'",
        )
    idx = utils.extract_func_generic(expr.expr.type).require_int
    params = [p for p in fn_type.ast.items if not p.is_generic()]
    if idx < 0 or idx >= len(params):
        raise TypecheckError(expr, "arg out of bounds")

    return self.visit_expr(ast.BoolExpr(bool(params[idx].default)))


def transform_static_fn_get_default(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(
            expr,
            f"expected a function, got '{expr.items[0].value.type}'",
        )
    idx = utils.extract_func_generic(expr.expr.type).require_int
    params = [p for p in fn_type.ast.items if not p.is_generic()]
    if idx < 0 or idx >= len(params):
        raise TypecheckError(expr, "arg out of bounds")

    if default := params[idx].default:
        return self.visit_expr(default)
    return None


def transform_static_fn_wrap_call_args(self, expr):
    if not expr.items[0].value.cls:
        return None
    fn = utils.extract_function(utils.extract_type(self.ctx, expr.items[0].value.type))
    if not fn:
        raise TypecheckError(expr, f"expected a function, got '{expr.items[0].value.type}'")
    call = self.visit_expr(
        ast.CallExpr(
            expr.items[0].value,
            [ast.StarExpr(expr.items[1].value), ast.KeywordStarExpr(expr.items[2].value)],
        )
    )
    if not call.done:
        return None
    stmts = []
    while isinstance(call, ast.StmtExpr):
        stmts.extend(call.items)
        call = call.expr
    assert isinstance(call, ast.CallExpr), f"expected wrapped function call: {call}"
    stmts.append(ast.ExprStmt(call.expr))
    return self.visit_expr(ast.StmtExpr(stmts, ast.TupleExpr([arg.value for arg in call.items])))


def transform_static_vars(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    idx_arg_type = utils.extract_func_generic(expr.expr.type)
    if idx_arg_type.cls is None:
        return None

    with_idx = idx_arg_type.require_bool
    arg = self.visit_expr(expr.items[0].value)
    if not (arg_type := arg.cls):
        return None

    tuple_items = []
    for idx, field in enumerate(utils.get_class_fields(self.ctx, arg_type)):
        key = ast.StringExpr(field.name)
        value = ast.DotExpr(expr.items[0].value, member=field.name)
        if with_idx:
            tuple_items.append(ast.TupleExpr([ast.IntExpr(idx), key, value]))
        else:
            tuple_items.append(ast.TupleExpr([key, value]))
    return self.visit_expr(ast.TupleExpr(tuple_items))


def transform_static_children(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    assert expr.type and expr.expr.type and expr.items[0].value.type
    obj_type = utils.extract_func_generic(expr.expr.type).cls
    if not obj_type:
        return None

    tuple_items = []
    cls_data = utils.get_class(self.ctx, obj_type)
    assert cls_data
    for descendant_name in sorted(cls_data.descendants):
        if descendant_name == obj_type.name:
            continue
        child_data = utils.get_class(self.ctx, descendant_name)
        assert child_data
        for _, realization in sorted(child_data.realizations.items()):
            for base in realization.bases:
                if base.realized_name() == obj_type.realized_name():
                    tuple_items.append(ast.IdExpr(realization.type.realized_name()))
                    break
    return self.visit_expr(ast.TupleExpr(tuple_items))


def transform_static_tuple_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    assert expr.type and expr.expr.type
    fn_type = expr.expr.type.require_func
    tuple_type = utils.extract_func_generic(fn_type).cls
    if tuple_type is None or infer.realize(self.ctx, tuple_type) is None:
        return None
    assert tuple_type

    idx = utils.extract_func_generic(fn_type, 1).require_int
    fields = utils.get_class_fields(self.ctx, tuple_type)
    if idx < 0 or idx >= len(fields):
        raise TypecheckError(expr, "invalid idx")
    realized = infer.realize(self.ctx, utils.instantiate(self.ctx, fields[idx].type, [tuple_type]))
    assert realized
    return self.visit_expr(ast.IdExpr(realized.realized_name()))


def transform_static_format(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Transform staticlen method to a static integer expression. This method supports only
    static strings and tuple types.
    """

    assert expr.type and expr.expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.String

    fn_type = expr.expr.type.require_func
    format = utils.extract_func_generic(fn_type, 0).require_str
    arg = utils.extract_func_generic(fn_type, 1).require_str
    start = 0
    while True:
        start = format.find("%%", start)
        if start == -1:
            break
        format = format[:start] + "{}" + format[start + 2 :]
        start += 2
    format = format.replace("{}", arg)
    return self.visit_expr(ast.StringExpr(format))


def transform_static_int_to_str(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Transform int() method to a static string expression."""

    assert expr.type and expr.expr.type
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.String
    fn_type = expr.expr.type.require_func
    value = utils.extract_func_generic(fn_type).require_int
    return self.visit_expr(ast.StringExpr(f"{value}"))


def generate_type_info_init_ast(
    ctx: TypeContext, function: ast.types.Function
) -> ast.SuiteStmt | None:

    obj_type = utils.extract_func_generic(function).cls
    if not obj_type or not obj_type.can_realize():
        return None

    suite = ast.SuiteStmt()
    # Add extra initialization here!
    suite.add(
        ast.AssignStmt(
            ast.DotExpr(ast.IdExpr("self"), member="_base_name"), rhs=ast.StringExpr(obj_type.name)
        )
    )
    if obj_type != ast.types.Stdlib.UnrealizedType:
        for generic in obj_type.generics:
            param_type = generic.type
            if static := param_type.literal:
                param_type = static.runtime_type
            if param_type.func:
                param_type = infer.realize(ctx, param_type)
                assert param_type and param_type.cls
                param_type = param_type.cls
            suite.add(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="TypeInfo", func="cache")),
                    items=[
                        ast.CallExpr.Arg(name="vtable", value=ast.IdExpr("vt")),
                        ast.CallExpr.Arg(name="T", value=ast.IdExpr(param_type.realized_name())),
                    ],
                )
            )
            suite.add(
                ast.CallExpr(
                    ast.DotExpr(ast.DotExpr(ast.IdExpr("self"), member="_params"), member="append"),
                    items=[ast.IntExpr(utils.get_class_realization(ctx, param_type).id)],
                )
            )
    for field_idx, (field_name, field_type) in enumerate(
        utils.get_class_realization(ctx, obj_type).fields
    ):
        param_type = field_type
        if static := param_type.literal:
            param_type = static.runtime_type
        suite.add(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="TypeInfo", func="cache")),
                items=[
                    ast.CallExpr.Arg(name="vtable", value=ast.IdExpr("vt")),
                    ast.CallExpr.Arg(name="T", value=ast.IdExpr(param_type.realized_name())),
                ],
            )
        )
        suite.add(
            ast.CallExpr(
                ast.DotExpr(ast.DotExpr(ast.IdExpr("self"), member="_fields"), member="append"),
                items=[
                    ast.TupleExpr(
                        [
                            ast.StringExpr(field_name),
                            ast.IntExpr(utils.get_class_realization(ctx, param_type).id),
                            ast.CallExpr(
                                ast.IdExpr(ast.types.mangle(cls="type", func="_get_class_offset")),
                                items=[
                                    ast.IdExpr(obj_type.realized_name()),
                                    ast.IntExpr(field_idx),
                                ],
                            ),
                        ]
                    )
                ],
            )
        )
    return suite


def generate_super_dispatch_ast(ctx, fn):
    attr = utils.extract_func_generic(fn).str
    if attr is None:
        return None
    lexical = fn[0].require_cls[0].require_cls
    args = [ast.CallExpr.Arg(ast.NoneExpr())]
    args.extend(ast.CallExpr.Arg(ast.NoneExpr(type=typ)) for typ in fn[1].require_cls)
    keywords = fn[2].require_cls
    names = ctx.cache.generated_tuple_names[keywords[0].require_int]
    args.extend(
        ast.CallExpr.Arg(ast.NoneExpr(type=typ), name=name)
        for name, typ in zip(names, keywords[1].require_cls)
    )
    key = f"super.{lexical.realized_name()}.{attr}({fn[1].realized_name()},{fn[2].realized_name()})"
    identifier = ctx.cache.thunk_ids.setdefault(key, len(ctx.cache.thunk_ids) + 1)
    ctx.cache.super_calls[identifier] = fn
    for base in utils.get_mro(ctx, lexical)[1:]:
        infer.realize(ctx, base)
        args[0].value.type = base
        methods = utils.matching_methods(
            ctx, base.require_cls, utils.find_method(ctx, base.require_cls, attr), args
        )
        if not methods:
            continue
        call = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(base.realized_name()), attr),
            [
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="type", func="_force_cast")),
                    [ast.DotExpr(ast.IdExpr("self"), "_obj"), ast.IdExpr(base.realized_name())],
                ),
                ast.StarExpr(ast.IdExpr("args")),
                ast.KeywordStarExpr(ast.IdExpr("kwargs")),
            ],
        )
        call.set(ast.Attr.ExprVirtual, identifier)
        return ast.SuiteStmt(ast.ReturnStmt(expr=call))
    raise TypecheckError(ctx.info, f"super has no matching method '{attr}'")


def transform_static_platform(self, expr):
    import sys

    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.String
    platform = sys.platform
    if platform not in {"win32", "linux", "darwin"}:
        platform = "unix" if platform.startswith(("freebsd", "openbsd", "netbsd", "aix")) else ""
    if not platform:
        raise TypecheckError(expr, "unsupported platform")
    return self.visit_expr(ast.StringExpr(platform))


def transform_static_contains(self, expr):
    if unbound := expr.type.unbound:
        unbound._static_kind = ast.types.Type.Behaviour.Bool
    fn = expr.expr.type.require_func
    text = utils.extract_func_generic(fn, 0).require_str
    pattern = utils.extract_func_generic(fn, 1).require_str
    return self.visit_expr(ast.BoolExpr(pattern in text))


def populate_static_tuple_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")

    call = iterator.items[0].value
    assert isinstance(call, ast.CallExpr)
    block = []
    for arg in call:
        arg_expr = self.visit_expr(arg.value.clone(clean=True))
        assert arg_expr.type
        static = arg_expr.type.literal
        block.append(
            ast.AssignStmt(
                ast.IdExpr(vars[0]),
                rhs=arg_expr,
                type_expr=(
                    ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr(static.name))
                    if static
                    else None
                ),
            )
        )
    return block


def populate_simple_static_range_loop(iterator: ast.CallExpr, vars: List[str]) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")

    fn_expr = iterator.expr
    assert fn_expr.type
    end = utils.extract_func_generic(fn_expr.type).require_int
    if end > cache.MAX_STATIC_ITER:
        raise TypecheckError(
            iterator,
            f"static.range too large (expected 0..{cache.MAX_STATIC_ITER}, got instead {end})",
        )

    block = []
    for value in range(end):
        block.append(
            ast.AssignStmt(
                ast.IdExpr(vars[0]),
                rhs=ast.IntExpr(value),
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
            )
        )
    return block


def populate_static_range_loop(iterator: ast.CallExpr, vars: List[str]) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    fn_expr = iterator.expr
    assert fn_expr.type

    start = utils.extract_func_generic(fn_expr.type, 0).require_int
    end = utils.extract_func_generic(fn_expr.type, 1).require_int
    step = utils.extract_func_generic(fn_expr.type, 2).require_int
    iter_count = abs(start - end) // abs(step)
    if iter_count > cache.MAX_STATIC_ITER:
        raise TypecheckError(
            iterator,
            f"static.range too large (expected 0..{cache.MAX_STATIC_ITER}, "
            f"got instead {iter_count})",
        )

    value = start
    block = []
    while value < end if step > 0 else value > end:
        block.append(
            ast.AssignStmt(
                ast.IdExpr(vars[0]),
                ast.IntExpr(value),
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
            )
        )
        value += step
    return block


def populate_static_fn_overloads_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    fn_expr = iterator.expr
    assert fn_expr.type

    obj_type = utils.extract_func_generic(fn_expr.type, 0).require_cls
    name = utils.extract_func_generic(fn_expr.type, 1).require_str

    overloads = []
    if obj_type == (ast.types.Stdlib.NoneType):
        assert self.ctx.cache.type_ctx
        if value := self.ctx.cache.type_ctx.get(name):
            assert value.func
            overloads = utils.get_overloads(self.ctx, utils.get_root_name(self.ctx, value.func))
    else:
        cls_data = utils.get_class(self.ctx, obj_type)
        assert cls_data
        if method_root := cls_data.methods.get(name):
            overloads = utils.get_overloads(self.ctx, method_root)

    block = []
    for method in reversed(overloads):
        fn_data = utils.get_function(self.ctx, method)
        assert fn_data and fn_data.ast
        if utils.is_dispatch(method) or fn_data is None or fn_data.type is None:
            continue
        if utils.is_heterogenous(self.ctx, obj_type):
            if fn_data.ast.has(ast.Attr.AutoGenerated) and (
                fn_data.ast.name.endswith(".__iter__:0")
                or fn_data.ast.name.endswith(".__getitem__:0")
            ):
                # ignore __getitem__ and other heterogenuous methods
                continue
        value = ast.IdExpr(method)
        if obj_type != ast.types.Stdlib.NoneType and any(
            utils.get_unmangled_name(self.ctx, g.name) == "Self" for g in fn_data.type.func_generics
        ):
            value = ast.CallExpr(
                value,
                [
                    ast.CallExpr.Arg(
                        ast.IdExpr(utils.get_self_type(self.ctx, obj_type).realized_name()),
                        name="Self",
                    ),
                    ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial),
                ],
            )
        block.append(ast.AssignStmt(ast.IdExpr(vars[0]), rhs=value))
    return block


def populate_static_enumerate_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    fn_expr = iterator.expr
    assert fn_expr.type
    tuple_type = fn_expr.type.require_cls[0].cls

    block = []
    if tuple_type and tuple_type.is_tuple:
        fields = utils.get_class_fields(self.ctx, tuple_type)
        for idx in range(len(fields)):
            block.append(
                ast.SuiteStmt(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
                    ),
                    ast.AssignStmt(
                        ast.IdExpr(vars[1]),
                        rhs=ast.IndexExpr(iterator.items[0].value.clone(), ast.IntExpr(idx)),
                    ),
                )
            )
    else:
        raise TypecheckError(iterator, "static.enumerate needs a tuple")
    return block


def populate_static_vars_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    fn_expr = iterator.expr
    assert fn_expr.type

    with_idx = utils.extract_func_generic(fn_expr.type).require_bool
    if not with_idx and len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    if with_idx and len(vars) != 3:
        raise TypecheckError(iterator, "expected three items")

    obj_type = fn_expr.type.require_cls[0].require_cls
    idx = 0
    block = []
    if obj_type == ast.types.Stdlib.TypeWrap:
        obj_type = obj_type[0]
        cls_data = utils.get_class(self.ctx, utils.extract_class_type(self.ctx, obj_type))
        assert cls_data
        for field_name, field_var in sorted(cls_data.class_vars.items()):
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx)]),
                    rhs=ast.StringExpr(field_name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("str")),
                )
            )
            stmts.append(
                ast.AssignStmt(ast.IdExpr(vars[int(with_idx) + 1]), rhs=ast.IdExpr(field_var))
            )
            block.append(ast.SuiteStmt(*stmts))
            idx += 1
    else:
        for field in utils.get_class_fields(self.ctx, obj_type):
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx)]),
                    rhs=ast.StringExpr(field.name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("str")),
                )
            )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx) + 1]),
                    rhs=ast.DotExpr(
                        cast(ast.Expr, iterator.items[0].value.clone()), member=field.name
                    ),
                )
            )
            block.append(ast.SuiteStmt(*stmts))
            idx += 1
    return block


def populate_static_var_types_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    fn_expr = iterator.expr
    assert fn_expr.type

    realized = infer.realize(self.ctx, utils.extract_func_generic(fn_expr.type, 0))
    with_idx = utils.extract_func_generic(fn_expr.type, 1).require_bool
    if not with_idx and len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    if with_idx and len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    assert realized, (
        "vars_types expects a realizable type, got "
        f"'{utils.extract_func_generic(fn_expr.type, 0)}' instead"
    )

    block = []
    if union := realized.union:
        for idx, union_type in enumerate(union.get_realization_types()):
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(ast.IdExpr(vars[1]), rhs=ast.IdExpr(union_type.realized_name()))
            )
            block.append(ast.SuiteStmt(*stmts))
    else:
        realized = realized.require_cls
        for field_idx, field in enumerate(utils.get_class_fields(self.ctx, realized)):
            field_type = utils.instantiate(self.ctx, field.type, [realized])
            field_type = infer.realize(self.ctx, field_type)
            assert field_type is not None, f"cannot realize '{field.type!r}'"
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(field_idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx)]), rhs=ast.IdExpr(field_type.realized_name())
                )
            )
            block.append(ast.SuiteStmt(*stmts))
    return block


def populate_static_methods_loop(
    self: TypeVisitor, iterator: ast.CallExpr, vars: List[str]
) -> List[ast.Stmt]:
    fn_expr = iterator.expr
    assert fn_expr.type

    realized = infer.realize(self.ctx, utils.extract_func_generic(fn_expr.type, 0))
    assert realized, (
        "methods expects a realizable type, got "
        f"'{utils.extract_func_generic(fn_expr.type, 0)}' instead"
    )
    if realized == ast.types.Stdlib.TypeWrap:
        realized = realized.require_cls[0]
    realized = realized.require_cls
    cls_data = utils.get_class(self.ctx, realized)
    assert cls_data
    block = []
    for method_name in sorted(cls_data.methods):
        block.append(
            ast.SuiteStmt(
                ast.AssignStmt(
                    ast.IdExpr(vars[0]),
                    rhs=ast.StringExpr(method_name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr("str")),
                )
            )
        )
    return block


def populate_static_heterogenous_tuple_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    tuple_var = ""
    preamble = None
    if not isinstance(iterator, ast.IdExpr):
        tuple_var = utils.get_temporary_var(self.ctx, "tuple")
        preamble = ast.AssignStmt(ast.IdExpr(tuple_var), rhs=iterator)
    else:
        tuple_var = iterator.value

    block = []
    assert iterator.cls
    for idx in range(len(iterator.cls)):
        suite = ast.SuiteStmt()
        if len(vars) > 1:
            for var_idx, var in enumerate(vars):
                suite.add(
                    ast.AssignStmt(
                        ast.IdExpr(var),
                        rhs=ast.IndexExpr(
                            ast.IndexExpr(ast.IdExpr(tuple_var), index=ast.IntExpr(idx)),
                            index=ast.IntExpr(var_idx),
                        ),
                    )
                )
        else:
            suite.add(
                ast.AssignStmt(
                    ast.IdExpr(vars[0]),
                    rhs=ast.IndexExpr(ast.IdExpr(tuple_var), index=ast.IntExpr(idx)),
                )
            )
        block.append(suite)
    block.append(preamble)
    return block
