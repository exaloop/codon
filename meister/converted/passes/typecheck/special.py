# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast, cache, error
from ..scope import ScopingVisitor
from . import TypeContext, access, infer, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def prepare_vtables(ctx: TypeContext):
    """
    Generate ASTs for all internal functions that deal with vtable generation.
    Intended to be called once the typechecking is done.
    TODO: add JIT compatibility.
    """

    # def RTTIType._get_thunk_id(F, T):
    # return VID
    fn = utils.get_function(self.ctx, ast.types.mangle(cls="RTTIType", func="_get_thunk_id"))
    # Keep iterating as thunks can generate more thunks.
    old_ast = fn.ast
    processed, added = set(), True
    while added:
        added = False
        for realized_name, realization in list(fn.realizations.items()):
            if realized_name in processed:
                continue
            processed.add(realized_name)
            added = True
            fn.ast.suite = generate_get_thunk_id_ast(self, realization.type)
            realization.type.ast = fn.ast
            infer.realize_func(self, realization.type, True)
            fn.ast = old_ast

    fn = utils.get_function(self.ctx, ast.types.mangle(cls="RTTIType", func="_populate_vtables"))
    fn.ast.suite = generate_class_populate_vtables_ast(self)
    first_realization = next(iter(fn.realizations.values()))
    function_type = first_realization.type
    function_type.ast = fn.ast
    infer.realize_func(self, function_type, True)

    # def RTTIType._dist(B, D):
    # return Tuple[<types before B is reached in D>].__elemsize__
    fn = utils.get_function(self.ctx, ast.types.mangle(cls="RTTIType", func="_dist"))
    old_ast = fn.ast
    for realization in list(fn.realizations.values()):
        fn.ast.suite = generate_base_derived_dist_ast(self, realization.type)
        realization.type.ast = fn.ast
        infer.realize_func(self, realization.type, True)
    fn.ast = old_ast


def generate_class_populate_vtables_ast(self: TypeVisitor) -> ast.SuiteStmt:
    suite = ast.SuiteStmt()
    for cls_data in self.ctx.cache.classes.values():
        for realization in cls_data.realizations.values():
            # p[real.ID].__setitem__(f.ID, Function[<TYPE_F>](f).__raw__())
            if not realization.vtable:
                continue
            suite.add(
                ast.ExprStmt(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="TypeInfo", func="cache")),
                        items=[ast.IdExpr("vtable"), ast.IdExpr(realization.type.realized_name())],
                    )
                )
            )
            thunks = []
            for key in realization.vtable:
                thunk_id = self.ctx.cache.thunk_ids.get(key)
                assert thunk_id is not None, f"key {key} not found in thunkIds"
                thunks.append((key, thunk_id))
            thunks.sort(key=lambda item: item[1])
            for key, thunk_id in thunks:
                thunk = realization.vtable[key]
                identifiers = []
                for generic in thunk:
                    identifiers.append(ast.IdExpr(generic.type.realized_name()))
                ret_type = thunk.get_ret_type()
                fn_call = ast.CallExpr(
                    ast.InstantiateExpr(
                        ast.IdExpr(ast.types.Stdlib.Function),
                        items=[
                            ast.InstantiateExpr(ast.IdExpr(ast.types.Stdlib.Tuple), identifiers),
                            ast.IdExpr(ret_type.realized_name()),
                        ],
                    ),
                    items=[ast.IdExpr(thunk.realized_name())],
                )
                suite.add(
                    ast.ExprStmt(
                        ast.CallExpr(
                            ast.DotExpr(ast.IdExpr("vtable"), member="set_thunk"),
                            items=[
                                ast.IntExpr(realization.id),
                                ast.IntExpr(thunk_id),
                                ast.CallExpr(ast.DotExpr(fn_call, member="__raw__")),
                            ],
                        )
                    )
                )
    return suite


def generate_base_derived_dist_ast(self: TypeVisitor, function: ast.types.Function):
    # Dist from Base to Derived. Assumes Derived is indeed a derived class of base.
    # Rules:
    # - Base is within Derived.
    # - Use MRO order.
    base_type = utils.extract_func_generic(function, 0).get_class()
    derived_type = utils.extract_func_generic(function, 1).get_class()
    derived_bases = utils.get_base_classes(self.ctx, derived_type)
    fields = utils.get_class_fields(derived_type)
    derived_idx, field_idx = 0, 0
    while derived_idx < len(derived_bases):
        derived_base_type = derived_bases[derived_idx].get_class()
        if derived_base_type.realized_name() == base_type.realized_name():
            break
        while field_idx < len(fields) and fields[field_idx].base_class == derived_base_type.name:
            field_idx += 1
        derived_idx += 1
    assert derived_idx < len(derived_bases), (
        f"class {base_type.debug_string(2)} is not a base class of {derived_type.debug_string(2)}"
    )
    if field_idx == 0:
        return ast.SuiteStmt(ast.ReturnStmt(expr=ast.IntExpr(0)))
    return ast.SuiteStmt(
        ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="type", func="_get_class_offset")),
                items=[ast.IdExpr(derived_type.realized_name()), ast.IntExpr(field_idx)],
            )
        )
    )


def generate_thunk_ast(
    self: TypeVisitor, function: ast.types.Function, base: ast.types.Class, derived: ast.types.Class
) -> ast.FunctionStmt | None:
    derived_generic = utils.extract_type(
        self.ctx, self.ctx.force_find(derived.name).type
    ).get_class()
    derived_type = utils.instantiate(self.ctx, derived_generic, [base]).get_class()
    arg_types = [generic.type for generic in function]
    arg_types[0] = derived_type
    method = utils.best_method(
        self.ctx,
        derived_type,
        utils.get_unmangled_name(self.ctx, function.get_func_name()),
        arg_types,
    )
    # Print a nice error message
    if not method:
        arguments_nice = f"({', '.join(t.pretty_string() for t in arg_types)})"
        raise TypecheckError(
            self.ctx.node_stack[-1],
            f"'{derived_type.pretty_string()}' object has no method "
            f"'{utils.get_unmangled_name(self.ctx, function.get_func_name())}' "
            f"with arguments {arguments_nice}",
        )

    names = [arg.realized_name() for arg in arg_types]
    thunk_name = f"_thunk.{base.name}.{function.get_func_name()}.{'.'.join(names)}"
    if utils.get_function(self.ctx, ast.types.mangle(func=thunk_name, no_core=True)):
        return None

    # Thunk contents:
    # def _thunk.<BASE>.<FN>.<ARGS>(self, <ARGS...>):
    # return <FN>(RTTIType._cast(self, <DERIVED>), <ARGS...>)
    params = [ast.Param("self", type=ast.IdExpr(base.realized_name()))]
    for idx in range(1, len(arg_types)):
        params.append(
            ast.Param(
                utils.get_unmangled_name(self.ctx, function.ast.items[idx].name),
                type=ast.IdExpr(arg_types[idx].realized_name()),
            )
        )
    # For debugging
    call_args = [
        ast.CallExpr(
            ast.DotExpr(ast.IdExpr("RTTIType"), member="_cast"),
            items=[
                ast.IdExpr("self"),
                ast.IdExpr(derived.realized_name()),
            ],
        )
    ]
    for idx in range(1, len(arg_types)):
        call_args.append(
            ast.IdExpr(utils.get_unmangled_name(self.ctx, function.ast.items[idx].name))
        )
    debug_args = [
        ast.StringExpr(base.name),
        ast.StringExpr(function.get_func_name()),
        ast.StringExpr(".".join(names)),
        *call_args,
    ]
    thunk_ast = ast.FunctionStmt(
        thunk_name,
        items=params,
        suite=ast.SuiteStmt(
            ast.ExprStmt(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_thunk_debug")),
                    items=debug_args,
                )
            ),
            ast.ReturnStmt(expr=ast.CallExpr(ast.IdExpr(method.ast.name), items=call_args)),
        ),
    )
    thunk_ast.set(ast.Attr.Inline)
    return self.visit(thunk_ast)


def generate_get_thunk_id_ast(self: TypeVisitor, function: ast.types.Function):
    """
    Generate thunks in all derived classes for a given virtual function (must be fully
    realizable) and the corresponding base class.
    @return unique thunk ID.
    """

    fn_type = utils.extract_type(self.ctx, utils.extract_func_generic(function)).get_func()
    cls_type = utils.extract_type(self.ctx, utils.extract_func_generic(function, 1)).get_class()
    # Function signature for storing thunks.
    # Needs to append function generics to realized name.
    # TODO: refactor / remove (why is this needed)?
    ret_type = fn_type.get_ret_type()
    assert (
        cls_type.can_realize() and fn_type.can_realize() and ret_type and ret_type.can_realize()
    ), f"bad {function.debug_string(2)}"

    def signature(value: ast.types.Function):
        generics = [arg.type.realized_name() for arg in value]
        generics.append("|")
        generics += [arg.type.realized_name() for arg in value.func_generics if arg.name]
        return f"{utils.get_unmangled_name(self.ctx, value.get_func_name())}:{','.join(generics)}"

    # Set up the base class information
    base_class = cls_type.name
    fn_signature = signature(fn_type)
    key = (base_class, fn_signature)
    # Add or extract thunk ID
    base_realization = utils.get_class_realization(self.ctx, cls_type)
    assert key not in base_realization.vtable, f"thunk {base_class}.{fn_signature} already added"
    if key not in self.ctx.cache.thunk_ids:
        self.ctx.cache.thunk_ids[key] = 1 + len(self.ctx.cache.thunk_ids)
    virtual_id = self.ctx.cache.thunk_ids[key]
    base_realization.vtable[key] = fn_type

    # Iterate through all derived classes and instantiate the corresponding thunk
    for cls_name, cls_data in self.ctx.cache.classes.items():
        # First check if our class descends from our base class
        # (ignore generics for now; this is just a speed-up).
        # TODO: use hashmap
        in_mro = False
        for mro_type in cls_data.mro:
            if mro_type and mro_type.is_type(base_class):
                in_mro = True
                break
        if not in_mro or cls_name == base_class:
            continue
        for realization in list(cls_data.realizations.values()):
            # Now check if generics match!
            in_mro = False
            # now check realizations!
            for mro_type in realization.bases:
                if mro_type.realized_name() == cls_type.realized_name():
                    in_mro = True
                    break
            if not in_mro:
                continue
            thunk_ast = generate_thunk_ast(self, fn_type, cls_type, realization.type)
            if thunk_ast:
                thunk_fn = utils.get_function(self.ctx, thunk_ast.name)
                thunk_type = utils.instantiate(self.ctx, thunk_fn.type).get_func()
                thunk_type = infer.realize_func(self, thunk_type, True)
                assert thunk_type is not None, f"bad thunk {thunk_fn.type.debug_string(2)}"
                assert key not in realization.vtable, (
                    f"thunk {base_class}.{fn_signature} already added to "
                    f"{realization.type.realized_name()}"
                )
                realization.vtable[key] = thunk_type.get_func()
    return ast.SuiteStmt(ast.ReturnStmt(ast.IntExpr(virtual_id)))


def generate_function_call_internal_ast(self: TypeVisitor, function: ast.types.Function):
    # Special case: Function.__call_internal__
    # TODO: move to IR one day
    llvm_lines, llvm_args = [], []
    arg_tuple = function[1].get_class()
    assert arg_tuple.is_type(ast.types.Stdlib.Tuple), (
        f"bad function base: {function[1].debug_string(2)}"
    )
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


def generate_union_new_ast(self: TypeVisitor, function: ast.types.Function):
    parent = function.func_parent
    union = None if parent is None else parent.get_union()
    assert isinstance(union, ast.types.Union), f"expected union, got {parent}"
    return ast.SuiteStmt(
        ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_new")),
                items=[
                    ast.IdExpr(function.ast.items[0].name),
                    ast.IdExpr(union.realized_name()),
                ],
            )
        )
    )


def generate_union_tag_ast(self: TypeVisitor, function: ast.types.Function):
    # return Union._get_data(union, T0)
    tag = utils.get_int_literal(utils.extract_func_generic(function))
    union = function[0].get_union()
    union_types = union.get_realization_types()
    if tag < 0 or tag >= len(union_types):
        raise TypecheckError(self.ctx.node_stack[-1], "bad union tag")
    self_var = function.ast.items[0].name
    return ast.SuiteStmt(
        ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
                items=[ast.IdExpr(self_var), ast.IdExpr(union_types[tag].realized_name())],
            )
        )
    )


def generate_union_dispatch_ast(
    self: TypeVisitor, function: ast.types.Function
) -> ast.SuiteStmt | None:
    attr_type = utils.extract_func_generic(function).get_str_static()
    if not isinstance(attr_type, ast.types.StrLiteral):
        return None
    union = function[0].get_union()
    assert union, f"not an union: {function[0].debug_string(2)}"
    generic_fn = utils.get_function(self.ctx, function.get_func_name())
    suite = generic_fn.ast.suite.clone()
    positional_type = function[1]
    keyword_type = function[2]
    is_call = positional_type.is_type(ast.types.Stdlib.Tuple) and keyword_type.is_type(
        ast.types.Stdlib.NamedTuple
    )
    candidates = []
    ret_types = {}
    for tag, typ in enumerate(union.get_realization_types()):
        result_type = None
        if is_call:
            call_args = [ast.NoneExpr(type=generic.type) for generic in positional_type.generics]
            tuple_id = utils.get_int_literal(keyword_type)
            names = self.ctx.cache.generated_tuple_names[tuple_id]
            keyword_tuple_type = keyword_type[1].get_class()
            for idx, generic in enumerate(keyword_tuple_type.generics):
                call_args.append(ast.CallExpr.Arg(names[idx], value=ast.NoneExpr(generic.type)))
            callable_ok = False
            if partial := typ.get_partial():
                callable_ok = (
                    utils.can_call(self.ctx, partial.get_partial_func(), call_args, partial) >= 0
                )
            else:
                methods = utils.find_method(self.ctx, typ, "__call__", False)
                matches = utils.matching_methods(self.ctx, typ, methods, call_args)
                callable_ok = bool(matches)
            if callable_ok:
                self.ctx.add_block()
                try:
                    union_var = utils.get_temporary_var(self.ctx, "union")
                    self.ctx.add(union_var, union_var, typ, self.ctx.time)
                    args_var = utils.get_temporary_var(self.ctx, "args")
                    self.ctx.add(args_var, args_var, positional_type, self.ctx.time)
                    kwargs_var = utils.get_temporary_var(self.ctx, "kwargs")
                    self.ctx.add(kwargs_var, kwargs_var, keyword_type, self.ctx.time)
                    call_expr = ast.CallExpr(
                        ast.IdExpr(union_var),
                        items=[
                            ast.StarExpr(ast.IdExpr(args_var)),
                            ast.KeywordStarExpr(expr=ast.IdExpr(kwargs_var)),
                        ],
                    )
                    call_expr = self.visit(call_expr)
                    result_type = call_expr.type
                finally:
                    self.ctx.pop_block()
        else:
            # Do full expression typechecking.
            self.ctx.add_block()
            try:
                union_var = utils.get_temporary_var(self.ctx, "union")
                self.ctx.add_var(union_var, union_var, typ, self.ctx.time)
                receiver = ast.IdExpr(union_var, type=typ)
                _, found = access.access_attribute(self, receiver, attr_type.value)
                if found:
                    expr = self.visit(ast.DotExpr(receiver, member=attr_type.value))
                    result_type = expr.type
            finally:
                self.ctx.pop_block()
        if result_type:
            assert result_type.can_realize(), f"cannot realize {result_type.debug_string(2)}"
            ret_types[result_type.realized_name()] = result_type
            candidate = ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Union", func="_get_data")),
                items=[ast.IdExpr("union"), ast.IdExpr(typ.realized_name())],
            )
            if is_call:
                candidate = ast.CallExpr(
                    candidate,
                    items=[
                        ast.StarExpr(ast.IdExpr("args")),
                        ast.KeywordStarExpr(expr=ast.IdExpr("kwargs")),
                    ],
                )
            else:
                candidate = ast.DotExpr(candidate, member=attr_type.value)
            candidates.append((tag, candidate))
    if len(ret_types) > 1:
        union_args = [ast.IdExpr(name) for name in ret_types]
        wrapper = ast.InstantiateExpr(ast.IdExpr(ast.types.mangle("", "Union")), items=union_args)
        for idx, (candidate_tag, candidate) in enumerate(candidates):
            candidates[idx] = (
                candidate_tag,
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="Union", func="_new")),
                    items=[candidate, wrapper],
                ),
            )
    for candidate_tag, candidate in candidates:
        # # if static.function.can_call(T, *args, **kwargs):
        suite.add(
            ast.IfStmt(
                ast.BinaryExpr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")),
                        items=[ast.IdExpr("union")],
                    ),
                    op="==",
                    rexpr=ast.IntExpr(candidate_tag),
                ),
                if_suite=ast.SuiteStmt(ast.ReturnStmt(expr=candidate)),
            )
        )
    assert suite.items
    # Move the final check to the end
    suite.add(suite.items[0])
    del suite.items[0]
    return suite


def generate_named_keys_ast(self: TypeVisitor, function: ast.types.Function):
    idx = utils.get_int_literal(utils.extract_func_generic(function))
    if idx < 0 or idx >= len(self.ctx.cache.generated_tuple_names):
        raise TypecheckError(self.ctx.node_stack[-1], "bad namedkeys idx")
    values: List[ast.Expr] = [
        ast.StringExpr(name) for name in self.ctx.cache.generated_tuple_names[idx]
    ]
    return ast.SuiteStmt(ast.ReturnStmt(expr=ast.TupleExpr(values)))


def generate_tuple_mul_ast(self: TypeVisitor, function: ast.types.Function) -> ast.SuiteStmt | None:
    count = max(0, utils.get_int_literal(utils.extract_func_generic(function)))
    tuple_type = function[0].get_class()
    if not isinstance(tuple_type, ast.types.Class) or not tuple_type.is_type(
        ast.types.Stdlib.Tuple
    ):
        return None
    items: List[ast.Expr] = [
        ast.IndexExpr(ast.IdExpr(function.ast.items[0].name), idx=ast.IntExpr(idx))
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
        and utils.is_heterogenous(ctx, function[0])
    ):
        # Special case: do not realize auto-generated heterogenous __getitem__
        raise TypecheckError(ctx.node_stack[-1], "expected iterable expression")
    if ast_node.name.startswith("Function.__call_internal__"):
        return generate_function_call_internal_ast(self, function)
    if ast_node.name.startswith("Union.__new__"):
        return generate_union_new_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="_tag")):
        return generate_union_tag_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle(cls="Union", func="_dispatch")):
        return generate_union_dispatch_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle("", "NamedTuple", "_namedkeys")):
        return generate_named_keys_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle("", "__magic__", "mul")):
        return generate_tuple_mul_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle(cls="TypeInfo", func="_init_params")):
        return generate_type_info_init_ast(self, function)
    if ast_node.name.startswith(ast.types.mangle("", "Super", "_dispatch")):
        return generate_super_dispatch_ast(self, function)
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
    name = utils.get_str_literal(utils.extract_func_generic(expr.expr.type))
    if len(expr.items) != 1:
        raise TypecheckError(expr, "namedtuple() takes 2 static arguments")
    # Construct the class statement
    generics, params = [], []
    original = expr.items[0].value.orig_expr
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
            type_expr = self.visit(item.items[1], enforce_type=True)
            params.append(ast.Param(item.items[0].get_value(), type=type_expr))
            continue
        raise TypecheckError(expr, "namedtuple() takes 2 static arguments")
    params.extend(generics)
    class_suite = ast.SuiteStmt(
        ast.ClassStmt(name, items=params, base_classes=[ast.IdExpr("tuple")])
    )
    self.ctx.cache.scope(class_suite)
    class_suite = self.visit(class_suite)
    self.ctx.prepend_stmts[-1].append(class_suite)
    return self.visit(ast.IdExpr(name), enforce_type=True)


def transform_functools_partial(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Transform partial calls (Python syntax).
    @example
    `partial(foo, 1, a=2)` -> `foo(1, a=2, ...)`
    """

    if not expr.items:
        raise TypecheckError(expr, "partial() takes 1 or more arguments")
    arguments = list(expr.items[1:])
    arguments.append(ast.CallExpr.Arg(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial)))
    return self.visit(ast.CallExpr(expr.items[0].value, items=arguments))


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

    base = self.ctx.get_base()
    fn_type = base.type.get_func()
    # Find list of matching superf methods
    supers = []
    if not utils.is_dispatch_type(fn_type):
        if parent_name := fn_type.ast.attributes.get(ast.Attr.ParentClass):
            parent_cls_data = utils.get_class(self.ctx, parent_name)
            if parent_cls_data:
                if method_root := parent_cls_data.methods.get(
                    utils.get_unmangled_name(self.ctx, fn_type.get_func_name())
                ):
                    for overload in utils.get_overloads(self.ctx, method_root):
                        if utils.is_dispatch(overload):
                            continue
                        if overload == fn_type.get_func_name():
                            break
                        overload_fn = utils.get_function(self.ctx, overload)
                        if overload_fn and overload_fn.type:
                            supers.append(overload_fn.type)
                    supers.reverse()
    if not supers:
        raise TypecheckError(expr, "no superf methods found")
    assert len(expr.items) == 1 and isinstance(expr.items[0].value, ast.CallExpr), "bad superf call"
    inner_call = expr.items[0].value
    new_args = [arg.value for arg in inner_call.items]
    parent_type = None if fn_type.func_parent is None else fn_type.func_parent.get_class()
    methods = utils.matching_methods(self.ctx, parent_type, supers, new_args)
    if not methods:
        raise TypecheckError(expr, "no superf methods found")
    return self.visit(ast.CallExpr(ast.IdExpr(methods[0].get_func_name()), items=new_args))


def transform_super(self: TypeVisitor) -> ast.Expr:
    """
    Typecheck and transform super method. Replace it with the current self object cast
    to the first inherited type.
    TODO: only an empty super() is currently supported.
    """

    base = self.ctx.get_base()
    if base is None or base.type is None:
        raise TypecheckError(self.ctx.node_stack[-1], "no super methods found")
    fn_type = base.type.get_func()
    if not fn_type or not fn_type.ast or not fn_type.ast.has(ast.Attr.Method) or len(fn_type) == 0:
        raise TypecheckError(self.ctx.node_stack[-1], "no super methods found")
    self_type = fn_type[0].get_class()
    self_expr = ast.IdExpr(fn_type.ast.items[0].name, type=self_type)
    type_expr = ast.IdExpr(
        self_class_object.name, type=utils.instantiate_type_var(self.ctx, self_type)
    )
    return self.visit(
        ast.CallExpr(
            ast.IdExpr(ast.types.mangle(cls="Super", func="__new__")), items=[type_expr, self_expr]
        )
    )


def transform_ptr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Typecheck __ptr__ method. This method creates a pointer to an object. Ensure that
    the arg is a variable binding.
    """

    arg = expr.items[0].value = self.visit(expr.items[0].value)
    head = arg
    last = True
    while True:
        try:
            head_type = utils.extract_class_type(self.ctx, head)
        except (AssertionError, TypeError):
            return None
        if not last and not head_type.is_record():
            raise TypecheckError(
                expr, "__ptr__() only takes identifiers or tuple fields as arguments"
            )
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
        last = False
    infer.unify(
        expr.type,
        utils.instantiate(
            self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr), [arg.type]
        ),
    )
    if arg.done:
        expr.done = True
    return None


def transform_array(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Typecheck __array__ method. This method creates a stack-allocated array via alloca."""

    parent_type = expr.expr.type.get_func().get_parent_type()
    infer.unify(
        expr.type,
        utils.instantiate(
            self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Array), [parent_type[0]]
        ),
    )
    if infer.realize(self, expr.type):
        expr.done = True
    return None


def transform_is_instance(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Transform isinstance method to a static boolean expression.
    Special cases:
    `isinstance(obj, ByVal)` is True if `type(obj)` is a tuple type
    `isinstance(obj, ByRef)` is True if `type(obj)` is a reference type
    """

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool

    # again to realize it
    obj_arg = expr.items[0].value = self.visit(expr.items[0].value)
    obj_type = obj_arg.type.get_class()
    if not obj_type.can_realize():
        return None
    obj_arg = expr.items[0].value = self.visit(obj_arg)  # again to realize it

    obj_type = utils.extract_class_type(self.ctx, obj_type)
    type_expr = expr.items[1].value
    match type_expr:
        # Handle `isinstance(obj, (type1, type2, ...))`
        case ast.CallExpr(orig_expr=ast.TupleExpr(items=items)):
            result = self.visit(ast.BoolExpr(False))
            for item in items:
                result = self.visit(
                    ast.BinaryExpr(
                        result,
                        op="||",
                        rexpr=ast.CallExpr(ast.IdExpr("isinstance"), items=[obj_arg, item]),
                    )
                )
            return result

    target_type = utils.extract_type(self.ctx, type_expr)
    match type_expr:
        case ast.IdExpr(value="type"):
            return self.visit(ast.BoolExpr(utils.is_type_expr(obj_arg)))
        case ast.IdExpr(value="type[Tuple]"):
            return self.visit(ast.BoolExpr(obj_type.is_type(ast.types.Stdlib.Tuple)))
        case ast.IdExpr(value="type[ByVal]"):
            return self.visit(ast.BoolExpr(obj_type.is_record()))
        case ast.IdExpr(value="type[ByRef]"):
            return self.visit(ast.BoolExpr(not obj_type.is_record()))
        case _ if not target_type and (union := obj_type.get_union()):
            union_types = union.get_realization_types()
            matching_tag = -1
            for idx, union_type in enumerate(union_types):
                undo = ast.types.Type.UnifyContext()
                score = target_type.unify(union_type, undo)
                undo.undo()
                if score >= 0:
                    matching_tag = idx
                    break
            if matching_tag == -1:
                return self.visit(ast.BoolExpr(False))
            return self.visit(
                ast.BinaryExpr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Union", func="_get_tag")), [obj_arg]
                    ),
                    "==",
                    ast.IntExpr(matching_tag),
                )
            )
        case _ if type_expr.type.is_type("pyobj"):
            if obj_type.is_type("pyobj"):
                return self.visit(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle("std.internal.python", func="_isinstance")),
                        items=[obj_arg, type_expr],
                    )
                )
            else:
                return self.visit(ast.BoolExpr(False))

    type_expr = expr.items[1].value = self.visit(type_expr, enforce_type=True)
    target_type = utils.extract_type(self.ctx, type_expr)

    # Check type match
    if target_type.name == obj_type.name:
        undo = ast.types.Type.UnifyContext()
        score = obj_type.unify(target_type, undo)
        undo.undo()
        if score >= 0:
            return self.visit(ast.BoolExpr(True))

    instance_call = "_isinstance"
    if obj_type.is_type(ast.types.Stdlib.Any) and not utils.is_type_expr(obj_arg):
        return self.visit(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="Any", func=instance_call)),
                items=[obj_arg, type_expr],
            )
        )

    # Check RTTI super types
    target_cls_data = utils.get_class(self.ctx, target_type)
    value_cls_data = utils.get_class(self.ctx, obj_type)
    if (
        target_cls_data
        and value_cls_data
        and target_cls_data.has_rtti()
        and value_cls_data.has_rtti()
    ):
        for base in utils.get_class(self.ctx, obj_type).mro:
            typ = utils.instantiate(self.ctx, base, cls)
            mro_type = infer.realize(self, typ)
            undo = ast.types.Type.UnifyContext()
            score = mro_type.unify(target_type, undo)
            undo.undo()
            if score >= 0:
                return self.visit(ast.BoolExpr(True))

        # TODO: disallow all impossible cases that are not related to any MRO!
        return self.visit(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="RTTIType", func=instance_call)),
                items=[obj_arg, type_expr],
            )
        )

    return self.visit(ast.BoolExpr(False))


def transform_static_len(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """
    Transform staticlen method to a static integer expression. This method supports only
    static strings and tuple types.
    """

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Int
    expr.items[0].value = self.visit(expr.items[0].value)
    typ = utils.extract_type(self.ctx, expr.items[0].value)
    if static := typ.get_str_static():
        # Case: staticlen on static strings
        return self.visit(ast.IntExpr(len(static.value)))
    if union := typ.get_union():
        if infer.realize(self, typ):
            return self.visit(ast.IntExpr(len(union.get_realization_types())))
        return None
    typ = typ.get_class()
    if not typ:
        return None
    if not typ.is_record():
        raise TypecheckError(expr, "expected tuple type")
    return self.visit(ast.IntExpr(len(utils.get_class_fields(typ))))


def transform_has_attr(
    self: TypeVisitor, expr: ast.CallExpr, allow_dynamic: bool = False
) -> ast.Expr | None:
    """
    Transform hasattr method to a static boolean expression.
    This method also supports additional arg types that are used to check
    for a matching overload (not available in Python).
    """

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ.is_type(ast.types.Stdlib.TypeWrap):
        typ = typ[0].get_class()
    if typ is None:
        return None
    attr = utils.get_str_literal(utils.extract_func_generic(expr.expr.type))
    arg_types = [("", typ)]
    positional = expr.items[1].value
    if isinstance(positional, ast.CallExpr):
        for arg in positional.items:
            arg.value = self.visit(arg.value)
            if arg.value.get_class_type() is None:
                return None
            arg_type = utils.extract_type(self.ctx, arg.value)
            if arg_type.is_type(ast.types.Stdlib.TypeWrap):
                arg_type = arg_type[0]
            arg_types.append(("", arg_type))
    for name, named_expr in utils.extract_named_tuple(self.ctx, expr.items[2].value):
        named_expr = self.visit(named_expr)
        named_type = utils.extract_type(self.ctx, named_expr)
        if named_type.is_type(ast.types.Stdlib.TypeWrap):
            named_type = named_type[0]
        arg_types.append((name, named_type))

    if (union := typ.get_union()) and allow_dynamic:
        condition = None
        for typ in union.get_realization_types():
            if not (typ := infer.realize(self, typ)):
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
        return self.visit(ast.BoolExpr(False) if condition is None else condition)

    if typ.is_type(ast.types.Stdlib.NamedTuple):
        if not typ.can_realize():
            return None
        tuple_id = utils.get_int_literal(typ)
        assert 0 <= tuple_id < len(self.ctx.cache.generated_tuple_names)
        return self.visit(ast.BoolExpr(attr in self.ctx.cache.generated_tuple_names[tuple_id]))

    exists = utils.find_method(self.ctx, typ, attr) or utils.find_member(self.ctx, typ, attr)
    if exists and len(arg_types) > 1:
        call_args = []
        for name, arg_type in arg_types:
            call_args.append(ast.CallExpr.Arg(ast.NoneExpr(type=arg_type), name=name))
        methods = utils.find_method(self.ctx, typ, attr, False)
        exists = bool(utils.matching_methods(self.ctx, typ, methods, call_args))

    cls_data = utils.get_class(self.ctx, typ)
    if not exists and allow_dynamic and cls_data and cls_data.has_rtti():
        return self.visit(
            ast.CallExpr(
                ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_hasattr")),
                items=[expr.items[0].value, ast.StringExpr(attr)],
            )
        )
    return self.visit(ast.BoolExpr(exists))


def transform_get_attr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform getattr method to a DotExpr."""

    attr = utils.get_str_literal(utils.extract_func_generic(expr.expr.type))
    attr_type = utils.extract_type(self.ctx, utils.extract_func_generic(expr.expr.type, 1))
    result, found = access.access_attribute(self, expr.items[0].value, attr)
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ.is_type(ast.types.Stdlib.TypeWrap):
        typ = typ[0]
    if not found:
        cls_data = utils.get_class(self.ctx, typ)
        if cls_data and cls_data.has_rtti():
            return self.visit(
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
            f"'{utils.extract_type(self.ctx, expr.items[0].value).pretty_string()}' "
            f"object has no attribute '{attr}'",
        )
    if result is None:
        result = self.visit(ast.DotExpr(expr.items[0].value, member=attr))
    if not attr_type.is_type(ast.types.Stdlib.NoneType):
        can_wrap, result = utils.wrap_expr(self, result, attr_type)
        if can_wrap:
            infer.unify(result.type, attr_type)
    return result


def transform_set_attr(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform setattr method to a AssignMemberStmt."""

    attr = utils.get_str_literal(utils.extract_func_generic(expr.expr.type))
    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    if typ.is_type(ast.types.Stdlib.TypeWrap):
        typ = typ[0]
    cls_data = utils.get_class(self.ctx, typ)
    if cls_data and cls_data.has_rtti():
        _, found = access.get_attr(self, expr.items[0].value, attr)
        if not found:
            return self.visit(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_setattr")),
                    items=[expr.items[0].value, ast.StringExpr(attr), expr.items[1].value],
                )
            )
    return self.visit(
        ast.StmtExpr(
            ast.AssignMemberStmt(expr.items[0].value, member=attr, rhs=expr.items[1].value),
            expr=ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType)),
        )
    )


def transform_compile_error(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Raise a compiler error."""

    message = utils.get_str_literal(utils.extract_func_generic(expr.expr.type))
    raise TypecheckError(expr, message)


def transform_tuple_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Convert a class to a tuple."""

    for arg in expr.items:
        arg.value = self.visit(arg.value)
    cls_type = utils.extract_class_type(self.ctx, expr.items[0].value)
    # tuple(ClassType) is a tuple type that corresponds to a class
    if utils.is_type_expr(expr.items[0].value):
        if infer.realize(self, cls_type) is None:
            return expr
        items = []
        field_types = utils.get_class_field_types(self, cls_type)
        cls_data = utils.get_class(self.ctx, cls_type)
        for idx, field_type in enumerate(field_types):
            realized_type = infer.realize(self, field_type)
            assert realized_type is not None, (
                f"cannot realize '{cls_data.fields[idx].name}' in {cls_type.debug_string(2)}"
            )
            items.append(ast.IdExpr(realized_type.realized_name()))
            items.append(ast.IdExpr(realized_type.realized_name()))
        return self.visit(ast.InstantiateExpr(ast.IdExpr(ast.types.Stdlib.Tuple), items=items))
    items = []
    var = utils.get_temporary_var(self.ctx, "tup")
    for field in utils.get_class_fields(cls_type):
        items.append(ast.DotExpr(ast.IdExpr(var), member=field.name))
    return self.visit(
        ast.StmtExpr(
            ast.AssignStmt(ast.IdExpr(var), rhs=expr.items[0].value), expr=ast.TupleExpr(items)
        )
    )


def transform_type_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform type function to a type IdExpr identifier."""

    expr.items[0].value = self.visit(expr.items[0].value)
    infer.unify(expr.type, utils.instantiate_type_var(self.ctx, expr.items[0].value.type))
    if infer.realize(self, expr.type) is None:
        return None
    return ast.IdExpr(expr.type.realized_name(), type=expr.type, done=True)


def transform_realized_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform static.realized function to a fully realized type identifier."""

    fn_type = utils.extract_type(self.ctx, expr.items[0].value.type)
    if static := fn_type.get_str_static():
        # First arg can just be a literal string of function canonical name
        value = self.ctx.find(static.value)
        if value and value.is_func():
            fn_type = utils.instantiate(self.ctx, value.type)
    else:
        if (
            fn_type.get_func() is None
            and (partial := expr.items[0].value.type.get_partial())
            and partial.is_partial_empty()
        ):
            generalized = partial.get_partial_func().generalize(0)
            fn_type = utils.instantiate(self.ctx, generalized)

    if not fn_type.get_func():
        raise TypecheckError(expr, "static.realized() only takes functions as a first arg")

    args_type = expr.items[1].value.type.get_class()
    if not isinstance(args_type, ast.types.Class):
        return None
    assert args_type.name == ast.types.Stdlib.Tuple
    for idx in range(min(len(args_type), len(fn_type))):
        arg_type = args_type[idx]
        fn_arg_type = fn_type[idx]
        if arg_type.is_type(ast.types.Stdlib.TypeWrap):
            arg_type = arg_type[0]
        infer.unify(fn_arg_type, arg_type)
    if realized := infer.realize(self, fn_type):
        return ast.IdExpr(realized.realized_name(), type=realized, done=True)
    return None


def transform_static_print_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform __static_print__ function to a fully realized type identifier."""

    import sys

    call = expr.items[0].value
    for arg in call.items:
        arg_type = (
            None
            if arg.value is None or not isinstance(arg.value.type, ast.types.Type)
            else arg.value.type
        )
        debug_value = "-" if arg_type is None else arg_type.debug_string(2)
        realized_name = "-" if arg_type is None else arg_type.realized_name()
        static_suffix = " [static]" if arg_type and arg_type.get_static() else ""
        print(
            f"[print] {self.ctx.get_src_info()}: {debug_value} ({realized_name}){static_suffix}",
            file=sys.stderr,
        )
    return None


def transform_has_rtti_fn(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    """Transform static.has_rtti to a static boolean that indicates RTTI status of a type."""

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool
    arg_type = utils.extract_func_generic(expr.expr.type).get_class()
    if not isinstance(arg_type, ast.types.Class):
        return None
    return self.visit(ast.BoolExpr(utils.get_class(self.ctx, arg_type).has_rtti()))


def transform_static_fn_can_call(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Transform internal.static calls"""

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool

    typ = utils.extract_class_type(self.ctx, expr.items[0].value)
    input_args = utils.unpack_tuple_types(self, expr.items[1].value)
    keyword_args = utils.unpack_tuple_types(self, expr.items[2].value)
    assert input_args is not None and keyword_args is not None, "bad call to fn_can_call"

    call_args = []
    for name, arg_type in [*input_args, *keyword_args]:
        call_args.append(ast.CallExpr.Arg(ast.NoneExpr(type=arg_type), name=name))
    if typ.get_func():
        return self.visit(ast.BoolExpr(utils.can_call(self.ctx, typ, call_args) >= 0))
    if partial := typ.get_partial():
        return self.visit(
            ast.BoolExpr(
                utils.can_call(self.ctx, partial.get_partial_func(), call_args, partial) >= 0
            )
        )

    print("cannot use fn_can_call on non-functions")
    return self.visit(ast.BoolExpr(False))


def transform_static_fn_arg_has_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool

    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type:
        raise TypecheckError(
            expr, f"expected a function, got '{expr.items[0].value.type.pretty_string()}'"
        )
    idx = utils.extract_func_generic(expr.expr.type).get_int_static()
    assert idx, "expected a static integer"
    idx = idx.value
    can_realize = 0 <= idx < len(fn_type) and fn_type[idx] and fn_type[idx].can_realize()
    return self.visit(ast.BoolExpr(can_realize))


def transform_static_fn_arg_get_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type:
        raise TypecheckError(
            expr, f"expected a function, got '{expr.items[0].value.type.pretty_string()}'"
        )
    idx = utils.extract_func_generic(expr.expr.type).get_int_static()
    assert idx, "expected a static integer"
    idx = idx.value
    arg_type = None if idx < 0 or idx >= len(fn_type) else fn_type[idx]
    if arg_type is None or not arg_type.can_realize():
        raise TypecheckError(expr, "arg does not have type")
    return self.visit(ast.IdExpr(arg_type.realized_name()))


def transform_static_fn_args(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(
            expr, f"expected a function, got '{expr.items[0].value.type.pretty_string()}'"
        )
    values = []
    for arg in fn_type.ast.items:
        _, name = arg.get_name_with_stars()
        values.append(ast.StringExpr(utils.get_unmangled_name(self.ctx, name)))
    return self.visit(ast.TupleExpr(values))


def transform_static_fn_has_default(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.Bool

    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(
            expr,
            f"expected a function, got '{expr.items[0].value.type.pretty_string()}'",
        )
    idx = utils.extract_func_generic(expr.expr.type).get_int_static()
    assert idx, "expected a static integer"
    idx = idx.value
    if idx < 0 or idx >= len(fn_type.ast.items):
        raise TypecheckError(expr, "arg out of bounds")
    return self.visit(ast.BoolExpr(fn_type.ast.items[idx].default_value))


def transform_static_fn_get_default(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type or not isinstance(fn_type.ast, ast.FunctionStmt):
        raise TypecheckError(
            expr,
            f"expected a function, got '{expr.items[0].value.type.pretty_string()}'",
        )
    idx = utils.extract_func_generic(expr.expr.type).get_int_static()
    assert idx, "expected a static integer"
    idx = idx.value
    if idx < 0 or idx >= len(fn_type.ast.items):
        raise TypecheckError(expr, "arg out of bounds")
    return self.visit(fn_type.ast.items[idx].default_value)


def transform_static_fn_wrap_call_args(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    if not expr.items[0].value.get_class_type():
        return None
    fn_type = utils.extract_function(expr.items[0].value.type)
    if not fn_type:
        raise TypecheckError(
            expr, f"expected a function, got '{expr.items[0].value.type.pretty_string()}'"
        )
    call_args = []
    if len(expr.items) > 1 and expr.items[1].value:
        orig_args = expr.items[1].value.orig_expr
        if isinstance(orig_args, ast.TupleExpr):
            call_args.extend(orig_args.items)
        if isinstance(orig_args, ast.CallExpr):
            kw_data = utils.get_class(self.ctx, utils.extract_class_type(self.ctx, expr))
            assert kw_data is not None, (
                f"cannot find {utils.extract_class_type(self.ctx, expr).name}"
            )
            for idx, arg in enumerate(orig_args.items):
                call_args.append(ast.CallExpr.Arg(arg.value, name=kw_data.fields[idx].name))

    call = self.visit(ast.CallExpr(ast.IdExpr(fn_type.get_func_name()), items=call_args))
    if not call.done:
        return None
    tuple_args = []
    for arg in call.items:
        tuple_args.append(arg.value)
    return self.visit(ast.TupleExpr(tuple_args))


def transform_static_vars(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    idx_arg_type = utils.extract_func_generic(expr.expr.type)
    if idx_arg_type.get_class() is None:
        return None

    with_idx = utils.get_bool_literal(idx_arg_type)
    arg = self.visit(expr.items[0].value)
    if not (arg_type := arg.get_class_type()):
        return None

    tuple_items = []
    for idx, field in enumerate(utils.get_class_fields(arg_type)):
        key = ast.StringExpr(field.name)
        value = ast.DotExpr(expr.items[0].value, member=field.name)
        if with_idx:
            tuple_items.append(ast.TupleExpr([ast.IntExpr(idx), key, value]))
        else:
            tuple_items.append(ast.TupleExpr([key, value]))
    return self.visit(ast.TupleExpr(tuple_items))


def transform_static_children(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    obj_type = utils.extract_func_generic(expr.expr.type).get_class()
    if not isinstance(obj_type, ast.types.Class):
        return None

    tuple_items = []
    cls_data = utils.get_class(self.ctx, obj_type)
    for descendant_name in cls_data.descendants:
        if descendant_name == obj_type.name:
            continue
        descendant = utils.get_class(self.ctx, descendant_name)
        for realization in descendant.realizations.values():
            for base in realization.bases:
                if base.realized_name() == obj_type.realized_name():
                    tuple_items.append(ast.IdExpr(realization.type.realized_name()))
                    break
    return self.visit(ast.TupleExpr(tuple_items))


def transform_static_tuple_type(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr | None:
    fn_type = expr.expr.type.get_func()
    tuple_type = utils.extract_func_generic(fn_type).get_class()
    if tuple_type is None or infer.realize(self, tuple_type) is None:
        return None

    idx = utils.get_int_literal(utils.extract_func_generic(fn_type, 1))
    fields = utils.get_class_fields(tuple_type)
    if idx < 0 or idx >= len(fields):
        raise TypecheckError(expr, "invalid idx")
    realized = infer.realize(self, utils.instantiate(self.ctx, fields[idx].type, [tuple_type]))
    return self.visit(ast.IdExpr(realized.realized_name()))


def transform_static_format(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """
    Transform staticlen method to a static integer expression. This method supports only
    static strings and tuple types.
    """

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.String

    fn_type = expr.expr.type.get_func()
    format = utils.get_str_literal(utils.extract_func_generic(fn_type, 0))
    arg = utils.get_str_literal(utils.extract_func_generic(fn_type, 1))
    start = 0
    while True:
        start = format.find("%%", start)
        if start == -1:
            break
        format = format[:start] + "{}" + format[start + 2 :]
        start += 2
    format = format.replace("{}", arg)
    return self.visit(ast.StringExpr(format))


def transform_static_int_to_str(self: TypeVisitor, expr: ast.CallExpr) -> ast.Expr:
    """Transform int() method to a static string expression."""

    if unbound := expr.type.get_unbound():
        unbound.static_kind = ast.types.Type.Behaviour.String
    fn_type = expr.expr.type.get_func()
    value = utils.get_int_literal(utils.extract_func_generic(fn_type))
    return self.visit(ast.StringExpr(f"{value}"))


def generate_type_info_init_ast(
    self: TypeVisitor, function: ast.types.Function
) -> ast.SuiteStmt | None:

    obj_type = utils.extract_func_generic(function).get_class()
    if not obj_type or not obj_type.can_realize():
        return None

    suite = ast.SuiteStmt()
    # Add extra initialization here!
    suite.add(
        ast.AssignStmt(
            ast.DotExpr(ast.IdExpr("self"), member="_base_name"), rhs=ast.StringExpr(obj_type.name)
        )
    )
    if not obj_type.is_type(ast.types.Stdlib.UnrealizedType):
        for generic in obj_type.generics:
            param_type = generic.type
            if static := param_type.get_static():
                param_type = static.get_non_static_type()
            if param_type.get_func():
                param_type = infer.realize(self, param_type).get_class()
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
                    items=[ast.IntExpr(utils.get_class_realization(self.ctx, param_type).id)],
                )
            )
    for field_idx, (field_name, field_type) in enumerate(
        utils.get_class_realization(self.ctx, obj_type).fields
    ):
        param_type = field_type
        if static := param_type.get_static():
            param_type = static.get_non_static_type()
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
                            ast.IntExpr(utils.get_class_realization(self.ctx, param_type).id),
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


def generate_super_dispatch_ast(
    self: TypeVisitor, function: ast.types.Function
) -> ast.SuiteStmt | None:
    attr_type = utils.extract_func_generic(function).get_str_static()
    if not attr_type:
        return None

    obj_type = function[0][0].get_class()
    suite = utils.get_function(self.ctx, function.get_func_name()).ast.suite.clone()

    star_type = function[1]
    kwstar_type = function[2]
    assert star_type.is_type(ast.types.Stdlib.Tuple)
    assert kwstar_type.is_type(ast.types.Stdlib.NamedTuple)
    call_args = [ast.NoneExpr()]
    for generic in star_type.generics:
        call_args.append(ast.NoneExpr(type=generic.type))
    tuple_id = utils.get_int_literal(kwstar_type)
    names = self.ctx.cache.generated_tuple_names[tuple_id]
    kwstar_type = kwstar_type[1].get_class()
    for idx, generic in enumerate(kwstar_type.generics):
        call_args.append(ast.CallExpr.Arg(ast.NoneExpr(type=generic.type), name=names[idx]))

    next_mro = {}
    cls_data = utils.get_class(self.ctx, obj_type)
    for descendant_name in cls_data.descendants:
        descendant = utils.get_class(self.ctx, descendant_name)
        idx = 0
        while idx < len(descendant.mro) - 1:
            if descendant.mro[idx].name == obj_type.name:
                break
            idx += 1
        idx += 1
        if idx < len(descendant.mro):
            next_type = utils.instantiate(self.ctx, descendant.mro[idx], [obj_type])
            if not next_type.can_realize():
                continue
            infer.realize(self, next_type)
            methods = utils.find_method(self.ctx, next_type, attr_type.value, False)
            call_args[0].value.type = next_type
            for method in utils.matching_methods(self.ctx, next_type, methods, call_args):
                parent = method.ast.attributes.get(ast.Attr.ParentClass)
                if parent == next_type.name:
                    next_mro[descendant.mro[idx].name] = next_type
                    break
    for next_type in next_mro.values():
        ret_stmt = ast.ReturnStmt(
            expr=ast.CallExpr(
                ast.DotExpr(ast.IdExpr(next_type.name), member=attr_type.value),
                items=[
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="RTTIType", func="_cast")),
                        items=[
                            ast.DotExpr(ast.IdExpr("self"), member="_obj"),
                            ast.IdExpr(next_type.realized_name()),
                        ],
                    ),
                    ast.StarExpr(ast.IdExpr("args")),
                    ast.KeywordStarExpr(expr=ast.IdExpr("kwargs")),
                ],
            )
        )
        if len(next_mro) == 1:
            suite.add(ret_stmt)
        else:
            next_id = utils.get_class_realization(self.ctx, next_type).id
            suite.add(
                ast.IfStmt(
                    ast.BinaryExpr(ast.IdExpr("base"), "==", ast.IntExpr(next_id)),
                    if_suite=ret_stmt,
                )
            )
    return suite


def populate_static_tuple_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")

    call = iterator.items[0].value
    block = []
    for arg in call.items:
        arg_expr = self.visit(arg.value.clone(clean=True))
        static = arg_expr.type.get_static()
        block.append(
            ast.AssignStmt(
                ast.IdExpr(vars[0]),
                rhs=arg_expr,
                type_expr=(
                    ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr(static.name))
                    if static
                    else None
                ),
            )
        )
    return block


def populate_simple_static_range_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")

    fn_expr = iterator.expr
    end = utils.get_int_literal(utils.extract_func_generic(fn_expr.type))
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
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
            )
        )
    return block


def populate_static_range_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    fn_expr = iterator.expr

    start = utils.get_int_literal(utils.extract_func_generic(fn_expr.type, 0))
    end = utils.get_int_literal(utils.extract_func_generic(fn_expr.type, 1))
    step = utils.get_int_literal(utils.extract_func_generic(fn_expr.type, 2))
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
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
            )
        )
        value += step
    return block


def populate_static_fn_overloads_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    if len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    fn_expr = iterator.expr

    obj_type = utils.extract_func_generic(fn_expr.type, 0).get_class()
    name = utils.extract_func_generic(fn_expr.type, 1).get_str_static()
    assert name, "bad static string"
    name = name.value

    overloads = []
    if obj_type.is_type(ast.types.Stdlib.NoneType):
        if value := self.ctx.cache.type_ctx.find(name):
            overloads = utils.get_overloads(self.ctx, utils.get_root_name(self.ctx, value.type))
    else:
        cls_data = utils.get_class(self.ctx, obj_type)
        if method_root := cls_data.methods.get(name):
            overloads = utils.get_overloads(self.ctx, method_root)

    block = []
    for method in reversed(overloads):
        fn_data = utils.get_function(self.ctx, method)
        if utils.is_dispatch(method) or fn_data is None or fn_data.type is None:
            continue
        if utils.is_heterogenous(self.ctx, obj_type):
            if fn_data.ast.has(ast.Attr.AutoGenerated) and (
                fn_data.ast.name.endswith(".__iter__:0")
                or fn_data.ast.name.endswith(".__getitem__:0")
            ):
                # ignore __getitem__ and other heterogenuous methods
                continue
        block.append(ast.AssignStmt(ast.IdExpr(vars[0]), rhs=ast.IdExpr(method)))
    return block


def populate_static_enumerate_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:

    if len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    fn_expr = iterator.expr
    tuple_type = fn_expr.type[0].get_class()

    block = []
    if isinstance(tuple_type, ast.types.Class) and tuple_type.is_record():
        fields = utils.get_class_fields(tuple_type)
        for idx in range(len(fields)):
            block.append(
                ast.SuiteStmt(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
                    ),
                    ast.AssignStmt(
                        ast.IdExpr(vars[1]),
                        rhs=ast.IndexExpr(iterator.items[0].value.clone(), idx=ast.IntExpr(idx)),
                    ),
                )
            )
    else:
        raise TypecheckError(iterator, "static.enumerate needs a tuple")
    return block


def populate_static_vars_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:

    fn_expr = iterator.expr
    with_idx = utils.get_bool_literal(utils.extract_func_generic(fn_expr.type))
    if not with_idx and len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    if with_idx and len(vars) != 3:
        raise TypecheckError(iterator, "expected three items")

    obj_type = fn_expr.type[0].get_class()
    idx = 0
    block = []
    if obj_type.is_type(ast.types.Stdlib.TypeWrap):
        obj_type = obj_type[0]
        cls_data = utils.get_class(self.ctx, utils.extract_class_type(self.ctx, obj_type))
        for field_name, field_var in cls_data.class_vars.items():
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx)]),
                    rhs=ast.StringExpr(field_name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("str")),
                )
            )
            stmts.append(
                ast.AssignStmt(ast.IdExpr(vars[int(with_idx) + 1]), rhs=ast.IdExpr(field_var))
            )
            block.append(ast.SuiteStmt(*stmts))
            idx += 1
    else:
        for field in utils.get_class_fields(obj_type):
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(
                    ast.IdExpr(vars[int(with_idx)]),
                    rhs=ast.StringExpr(field.name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("str")),
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
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    fn_expr = iterator.expr
    realized = infer.realize(self, utils.extract_func_generic(fn_expr.type, 0))
    with_idx = utils.get_bool_literal(utils.extract_func_generic(fn_expr.type, 1))
    if not with_idx and len(vars) != 1:
        raise TypecheckError(iterator, "expected one item")
    if with_idx and len(vars) != 2:
        raise TypecheckError(iterator, "expected two items")
    assert realized, (
        "vars_types expects a realizable type, got "
        f"'{utils.extract_func_generic(fn_expr.type, 0)}' instead"
    )

    block = []
    if union := realized.get_union():
        for idx, union_type in enumerate(union.get_realization_types()):
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
                    )
                )
            stmts.append(
                ast.AssignStmt(ast.IdExpr(vars[1]), rhs=ast.IdExpr(union_type.realized_name()))
            )
            block.append(ast.SuiteStmt(*stmts))
    else:
        realized = realized.get_class()
        for field_idx, field in enumerate(utils.get_class_fields(realized)):
            field_type = utils.instantiate(self.ctx, field.type, [realized])
            field_type = infer.realize(self, field_type)
            assert field_type is not None, f"cannot realize '{field.type.debug_string(2)}'"
            stmts = []
            if with_idx:
                stmts.append(
                    ast.AssignStmt(
                        ast.IdExpr(vars[0]),
                        rhs=ast.IntExpr(field_idx),
                        type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("int")),
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
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt]:
    fn_type = iterator.expr
    realized = infer.realize(self, utils.extract_func_generic(fn_type.type, 0))
    assert realized, (
        "methods expects a realizable type, got "
        f"'{utils.extract_func_generic(fn_type.type, 0)}' instead"
    )
    if realized.is_type(ast.types.Stdlib.TypeWrap):
        realized = realized[0]
    realized = realized.get_class()
    cls_data = utils.get_class(self.ctx, realized)
    block = []
    for method_name in cls_data.methods:
        block.append(
            ast.SuiteStmt(
                ast.AssignStmt(
                    ast.IdExpr(vars[0]),
                    rhs=ast.StringExpr(method_name),
                    type_expr=ast.IndexExpr(ast.IdExpr("Literal"), idx=ast.IdExpr("str")),
                )
            )
        )
    return block


def populate_static_heterogenous_tuple_loop(
    self: TypeVisitor, iterator: ast.Expr, vars: List[str]
) -> List[ast.Stmt | None]:
    tuple_var = ""
    preamble = None
    if not isinstance(iterator, ast.IdExpr):
        tuple_var = utils.get_temporary_var(self.ctx, "tuple")
        preamble = ast.AssignStmt(ast.IdExpr(tuple_var), rhs=iterator)
    else:
        tuple_var = iterator.value

    block = []
    for idx in range(len(iterator.get_class_type().generics)):
        suite = ast.SuiteStmt()
        if len(vars) > 1:
            for var_idx, var in enumerate(vars):
                suite.add(
                    ast.AssignStmt(
                        ast.IdExpr(var),
                        rhs=ast.IndexExpr(
                            ast.IndexExpr(ast.IdExpr(tuple_var), idx=ast.IntExpr(idx)),
                            idx=ast.IntExpr(var_idx),
                        ),
                    )
                )
        else:
            suite.add(
                ast.AssignStmt(
                    ast.IdExpr(vars[0]),
                    rhs=ast.IndexExpr(ast.IdExpr(tuple_var), idx=ast.IntExpr(idx)),
                )
            )
        block.append(suite)
    block.append(preamble)
    return block
