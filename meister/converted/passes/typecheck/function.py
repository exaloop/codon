# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy
from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast, cache
from ...error import TypecheckError
from ..scope import Bindings
from . import classes, infer, utils

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_lambda(self: TypeVisitor, node: ast.LambdaExpr) -> ast.Expr:
    """
    Unify the function return type with `Generator[?]`.
    The unbound type will be deduced from return/yield statements.
    """

    fn = ast.FunctionStmt(
        name=utils.get_temporary_var(self.ctx, "lambda"),
        items=[param.clone() for param in node.items],
        suite=ast.ReturnStmt(expr=node.expr),
    )
    # TODO: just copy BindingsAttribute from expr instead?
    self.ctx.cache.scope(ast.SuiteStmt(fn))
    fn.set(ast.Attr.ExprTime, self.ctx.time)  # to handle captures properly
    fn = self.visit_stmt(fn)
    assert isinstance(fn, ast.FunctionStmt)
    if bindings := node.get(ast.Attr.Bindings):
        fn.set(ast.Attr.Bindings, bindings.clone())
    self.ctx.prepend_stmts[-1].append(fn)
    return self.visit_expr(ast.IdExpr(fn.name))


def typecheck_yieldexpr(self: TypeVisitor, node: ast.YieldExpr) -> ast.Expr:
    """
    Unify the function return type with `Generator[?]`.
    The unbound type will be deduced from return/yield statements.
    """
    if not self.ctx.in_function:
        raise TypecheckError(node, "'yield' outside function")

    base = self.ctx.base
    assert node.type and base.return_type
    base.return_type |= utils.instantiate(
        self.ctx, utils.get_stdlib_type(self.ctx, "Generator"), [node.type]
    )
    if infer.realize(self.ctx, node.type):
        node.done = True
    return node


def typecheck_await(self: TypeVisitor, node: ast.AwaitExpr) -> ast.Expr:
    """Typecheck await statements."""
    if not self.ctx.in_function:
        raise TypecheckError(node, "'await' outside function")
    base = self.ctx.base
    assert base.func
    if not base.func.async_:
        raise TypecheckError(node, "'await' outside function")

    node.expr = self.visit_expr(node.expr)
    if not node.transformed and (expr_type := node.expr.cls):
        is_coroutine = (
            expr_type == ast.types.Stdlib.Coroutine
            or expr_type == ast.types.mangle("std.asyncio", cls="Future")
            or expr_type == ast.types.mangle("std.asyncio", cls="Task")
        )
        if not is_coroutine:
            if utils.find_method(self.ctx, expr_type, "__await__"):
                awaited = self.visit_expr(ast.CallExpr(ast.DotExpr(node.expr, member="__await__")))
                is_coroutine = (
                    awaited.type == ast.types.Stdlib.Coroutine
                    or awaited.type == ast.types.mangle("std.asyncio", cls="Future")
                    or awaited.type == ast.types.mangle("std.asyncio", cls="Task")
                    or awaited.type == ast.types.Stdlib.Generator
                )
                if not is_coroutine:
                    raise TypecheckError(node, "expected awaitable expression")
                node.expr = awaited
                node.transformed = True
            else:
                raise TypecheckError(node, "expected awaitable expression")

    assert node.expr.type and node.type
    if typ := node.expr.type.cls:
        node.type |= typ[0]
    if node.expr.done:
        node.done = True
    return node


def typecheck_return(self: TypeVisitor, node: ast.ReturnStmt) -> ast.Stmt:
    """
    Typecheck return statements. Empty return is transformed to `return NoneType()`.
    Also partialize functions if they are being returned.
    See @c wrapExpr for more details.
    """

    if node.has(ast.Attr.Internal):
        node.expr = self.visit_expr(
            ast.CallExpr(ast.IdExpr(ast.types.mangle(cls="NoneType", func="__new__")))
        )
        node.done = True
        return node
    if not self.ctx.in_function:
        raise TypecheckError(node, "'return' outside function")

    base = self.ctx.base
    assert base.func and base.return_type

    is_async = base.func.async_
    if node.expr is None and base.func.has(ast.Attr.IsGenerator):
        node.done = True
    else:
        if base.func.has(ast.Attr.IsGenerator):
            raise TypecheckError(node, "returning values from generators not yet supported")
        if node.expr is None:
            node.expr = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
        node.expr = self.visit_expr(node.expr)

        # Wrap expression to match the return type
        if not base.return_type.unbound:
            can_wrap, node.expr = utils.wrap_expr(self.ctx, node.expr, base.return_type)
            if not can_wrap:
                return node

        assert node.expr.type
        fn_type = node.expr.type.func
        ret_type = base.return_type.cls
        # Special case: partialize functions if we are returning them
        if fn_type and not (ret_type and base.return_type == ast.types.Stdlib.Function):
            partial = ast.CallExpr(
                ast.IdExpr(fn_type.ast.name),
                items=[ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial)],
            )
            node.expr = self.visit_expr(partial)

        assert node.expr.type
        if base.return_type.is_runtime and node.expr.type.literal:
            node.expr.type = node.expr.type.literal.runtime_type
        if is_async:
            base.return_type |= utils.instantiate(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Coroutine),
                [node.expr.type],
            )
        else:
            base.return_type |= node.expr.type

    if self.ctx.block_level == 0:
        # If we are not within conditional block, ignore later statements in this function.
        # Useful with static if statements.
        self.ctx.return_early = True

    if node.expr is None or node.expr.done:
        node.done = True
    return node


def typecheck_yield(self: TypeVisitor, node: ast.YieldStmt) -> ast.Stmt:
    """Typecheck yield statements. Empty yields assume `NoneType`."""
    if not self.ctx.in_function:
        raise TypecheckError(node, "'yield' outside function")

    base = self.ctx.base
    assert base.return_type and base.func
    expression = node.expr or ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
    node.expr = self.visit_expr(expression)
    assert node.expr.type
    base.return_type |= utils.instantiate(
        self.ctx,
        utils.get_stdlib_type(self.ctx, "AsyncGenerator" if base.func.async_ else "Generator"),
        [node.expr.type],
    )
    if node.expr.done:
        node.done = True
    return node


def typecheck_yieldfrom(self: TypeVisitor, node: ast.YieldFromStmt) -> ast.Stmt:
    """
    Transform `yield from` statements.
    @example
    `yield from a` -> `for var in a: yield var`
    """
    var = utils.get_temporary_var(self.ctx, "yield")
    result = ast.ForStmt(ast.IdExpr(var), iter=node.expr, suite=ast.YieldStmt(expr=ast.IdExpr(var)))
    return self.visit_stmt(result)


def typecheck_global(_: TypeVisitor, node: ast.GlobalStmt) -> ast.Node:
    """Handled in scoping stage."""
    return ast.SuiteStmt()


def typecheck_function(self: TypeVisitor, node: ast.FunctionStmt) -> ast.Node:
    """
    Parse a function stub and create a corresponding generic function type.
    Also realize built-ins and extern C functions.
    """
    from . import TypeVisitor

    if node.has(ast.Attr.Python):
        # Handle Python block
        return transform_python_definition(
            self, node.name, node.items, node.ret, cast(ast.Stmt, node.suite.first_in_block())
        )

    original_statement = node.clone(clean=True)

    # Parse attributes
    has_decorators = False
    decorators = []
    for idx in range(len(node.decorators) - 1, -1, -1):
        decorator = node.decorators[idx]
        decorators.append(decorator)
        if decorator is None:
            continue
        is_attr, attr_name, attr_realized_name = get_decorator(self, decorator)
        if attr_name:
            if attr_name == ast.types.mangle("std.internal.attributes", func="test"):
                node.set(ast.Attr.Test)
            elif attr_name == ast.types.mangle("std.internal.attributes", func="export"):
                node.set(ast.Attr.Export)
            elif attr_name == ast.types.mangle("std.internal.attributes", func="inline"):
                node.set(ast.Attr.Inline)
            elif attr_name == ast.types.mangle("std.internal.attributes", func="no_arg_reorder"):
                node.set(ast.Attr.NoArgReorder)
            elif attr_name == ast.types.mangle(func="overload"):
                node.set(ast.Attr.Overload)
            fn_attrs = node.get(ast.Attr.FunctionAttributes, {})
            if not fn_attrs:
                node.set(ast.Attr.FunctionAttributes, fn_attrs)
            fn_attrs[attr_name] = attr_realized_name
            attr_fn = utils.get_function(self.ctx, attr_name)
            if attr_fn and attr_fn.ast:
                if attr_fn.ast.has(ast.Attr.Export):
                    node.set(ast.Attr.Export)
                if attr_fn.ast.has(ast.Attr.Inline):
                    node.set(ast.Attr.Inline)
                if attr_fn.ast.has(ast.Attr.NoArgReorder):
                    node.set(ast.Attr.NoArgReorder)
                if attr_fn.ast.has(ast.Attr.ForceRealize):
                    node.set(ast.Attr.ForceRealize)
                if inherited_attrs := attr_fn.ast.get(ast.Attr.FunctionAttributes):
                    fn_attrs.update(inherited_attrs)
            if is_attr:
                decorators[idx] = None  # ignore it later on
        if not is_attr:
            has_decorators = True

    is_class_member = self.ctx.in_class
    if node.has(ast.Attr.ForceRealize) and (not self.ctx.is_global or is_class_member):
        raise TypecheckError(node, "builtin must be a top-level statement")

    # All overloads share the same canonical name except for the number at the
    # end (e.g., `foo.1:0`, `foo.1:1` etc.)
    root_name = ""
    current_base = self.ctx.base
    if is_class_member:
        # Case 1: method overload
        current_class = utils.get_class(self.ctx, current_base.name)
        assert current_class
        root_name = current_class.methods.get(node.name, "")
        # TODO: handle static inherits and auto-generated cases
        if not root_name and node.has(ast.Attr.Overload):
            utils.warning(
                f"function '{node.name}' marked with unnecessary @overload",
                self.ctx.node_stack[-1].info,
            )
    elif node.has(ast.Attr.Overload):
        # Case 2: function overload
        if (
            (existing := self.ctx.find_at(node.name, self.ctx.time))
            and existing.func
            and existing.module == self.ctx.module_name
            and existing.base == self.ctx.base_name
        ):
            root_name = existing.canonical
    if not root_name:
        root_name = self.ctx.generate_canonical_name(node.name, True, is_class_member)
    # Append overload number to the name
    canonical = root_name
    self.ctx.cache.overloads.setdefault(root_name, [])
    canonical += f":{len(utils.get_overloads(self.ctx, root_name))}"
    self.ctx.cache.reverse_identifier_lookup[canonical] = node.name

    if is_class_member:
        # Set the enclosing class name
        node.set(ast.Attr.ParentClass, current_base.name)
        # Add the method to the class' method list
        cls_data = utils.get_class(self.ctx, current_base.name)
        assert cls_data
        cls_data.methods[node.name] = root_name

    # Handle captures. Add additional argument to the function for every capture.
    # Make sure to account for **kwargs if present
    if bindings := node.get(ast.Attr.Bindings):
        assert isinstance(bindings, Bindings)
        insert_idx = len(node.items)
        if node.items and node.items[-1].name.startswith("**"):
            insert_idx -= 1
        for captured_name, capture_type in bindings.captures.items():
            capture_arg = f"${captured_name}"
            if (
                (captured_item := self.ctx.find_at(captured_name, self.ctx.time))
                and capture_type is not Bindings.Scope.Global
                and not captured_item.is_global()
            ):
                parent_class_generic = (
                    self.ctx.bases[-1].is_type and self.ctx.bases[-1].name == captured_item.base
                )
                if captured_item.generic and parent_class_generic:
                    node.set(ast.Attr.Method)
                if not captured_item.generic or (
                    not captured_item.type.is_runtime and not parent_class_generic
                ):
                    if not captured_item.func:
                        if captured_item.is_type:
                            node.items.insert(
                                insert_idx,
                                ast.Param(capture_arg, type=ast.IdExpr(ast.types.Stdlib.Type)),
                            )
                        elif not captured_item.type.is_runtime:
                            type_name = str(captured_item.type.static_kind)
                            node.items.insert(
                                insert_idx,
                                ast.Param(
                                    capture_arg,
                                    type=ast.IndexExpr(
                                        ast.IdExpr("Literal"), index=ast.IdExpr(type_name)
                                    ),
                                ),
                            )
                        elif capture_type is Bindings.Scope.Nonlocal:
                            node.items.insert(
                                insert_idx,
                                ast.Param(capture_arg, type=ast.IdExpr(ast.types.Stdlib.Capsule)),
                            )
                        else:
                            node.items.insert(insert_idx, ast.Param(capture_arg))
                        insert_idx += 1
                    else:
                        # Local function is captured. Just note its canonical name and add it to
                        # the context during realization.
                        bindings.local_renames[captured_name] = captured_item.canonical
                continue
            if (
                captured_name == node.name and has_decorators  # decorated recursive fns
            ) or captured_name in self.ctx.global_shadows:
                node.items.insert(insert_idx, ast.Param(capture_arg))
                insert_idx += 1

    arguments = []
    suite = None
    ret = None
    explicits = []
    base_type = None
    is_global = self.ctx.is_global

    with self.ctx.within_base(canonical):
        fn_base = self.ctx.base
        fn_base.func = node

        # Parse arguments and add them to the context
        for param in node.items:
            stars, var_name = param.get_name_with_stars()
            param_name = self.ctx.generate_canonical_name(var_name)

            # Mark as method if the first argument is self
            if is_class_member and node.has(ast.Attr.HasSelf) and param.name == "self":
                node.set(ast.Attr.Method)

            # Handle default values
            default = param.default
            match default, param.type:
                case ast.NoneExpr(), ast.IdExpr(
                    value=ast.types.Stdlib.Type | ast.types.Stdlib.TypeTrait
                ):
                    # Special case: `arg: type = None` -> `arg: type = NoneType
                    default = ast.IdExpr(ast.types.Stdlib.NoneType)
                case ast.NoneExpr(), _:
                    # Do nothing. NoneExpr will be handled later (we don't want it
                    # to be converted to Optional call yet.)
                    pass
                case (
                    (
                        ast.IntExpr()
                        | ast.BoolExpr()
                        | ast.FloatExpr()
                        | ast.IdExpr()
                        | ast.StringExpr()
                    ),
                    _,
                ):
                    default = self.visit(default)
                case _ if not param.is_value():
                    # Special case: generic defaults are evaluated as-is!
                    default = self.visit(default)
                case _:
                    default_name = f".default.{canonical}.{param.name}"

                    new_context = self.ctx.clone()
                    new_context.bases.pop()
                    if is_class_member:
                        del new_context.bases[1:]
                    default_visitor = TypeVisitor(ctx=new_context)
                    assignment = ast.AssignStmt(
                        ast.IdExpr(default_name),
                        rhs=default,
                        type_expr=None if param.is_value() else param.type,
                    )
                    if is_class_member:
                        # class variable; go to the global context
                        declaration = default_visitor.visit(
                            ast.AssignStmt(ast.IdExpr(default_name))
                        )
                        self.ctx.preamble.add(declaration)
                        utils.register_global(self.ctx, default_name)
                        assignment.set_update()
                    elif is_global:
                        utils.register_global(self.ctx, default_name)
                    self.ctx.prepend_stmts[-1].append(default_visitor.visit_stmt(assignment))
                    default_item = self.ctx[default_name]
                    # Default unbounds must be allowed to pass through
                    # to support cases such as `a = []`
                    for unbound in default_item.type.get_unbounds(False):
                        unbound.pass_through = True
                        node.set(ast.Attr.AllowPassThrough)
                    default = default_visitor.visit(ast.IdExpr(default_name))
            arguments.append(
                ast.Param(
                    f"{'*' * stars}{param_name}",
                    type=param.type,
                    default=default,
                    status=param.status,
                )
            )

            # Add generics to the context
            if not param.is_value():
                generic_type = utils.instantiate_unbound(self.ctx)
                type_id = generic_type.id
                generic_type.generic_name = var_name
                default = self.visit_expr(param.default.clone()) if param.default else None
                default_type = None
                if isinstance(default, ast.Expr):
                    default_type = utils.extract_type(self.ctx, default)
                static_kind = ast.get_static_generic(param.type)
                if static_kind is not ast.types.Type.Behaviour.Runtime:
                    generic_item = self.ctx.add_item(var_name, param_name, generic_type)
                    generic_item.generic = True
                    generic_type._static_kind = static_kind
                    if default_type:
                        generic_type.default_type = default_type
                else:
                    match param.type:
                        case ast.InstantiateExpr(
                            expr=ast.IdExpr(value=ast.types.Stdlib.TypeTrait), items=[type_expr, *_]
                        ):
                            # Parse TraitVar
                            trait_expr = self.visit_expr(
                                type_expr, enforce_type=True, simple_types=True
                            )
                            assert trait_expr.type
                            generic_type.trait = (
                                trait_expr.type.link.trait
                                if trait_expr.type.link
                                else ast.types.TypeTrait(trait_expr.type)
                            )
                    generic_item = self.ctx.add_item(var_name, param_name, generic_type)
                    generic_item.generic = True
                    if default_type:
                        generic_type.default_type = default_type
                generalized = generic_type.generalize(self.ctx.typecheck_level)
                var_name = var_name.removeprefix("$")
                explicits.append(
                    ast.types.Generic(param_name, generalized, type_id, generalized.static_kind)
                )

        # Prepare list of all generic types
        parent_class = None
        if is_class_member and node.has(ast.Attr.Method):
            # Get class generics (e.g., T for `class Cls[T]: def foo:`)
            parent_item = self.ctx[node.get(ast.Attr.ParentClass)]
            parent_class = utils.extract_class_type(self.ctx, parent_item.type)
        # Add function generics
        generic_types = []
        for explicit in explicits:
            generic_types.append(utils.extract_type(self.ctx, explicit.name))

        # Handle function arguments
        # Base type: `Function[[args,...], ret]`
        base_type = get_func_type_base(self, len(node.items) - len(explicits))
        with self.ctx.substitute("typecheck_level", self.ctx.typecheck_level + 1):
            # Parse arguments to the context. Needs to be done after adding generics
            # to support cases like `foo(a: T, T: type)`
            for arg in arguments:
                arg.type = self.visit_expr(arg.type, enforce_type=True, simple_types=True)

            # Unify base type generics with argument types. Add non-generic arguments to the
            # context. Delayed to prevent cases like `def foo(a, b=a)`
            arg_tuple = base_type[0].require_cls
            arg_idx = 0
            for param_idx, param in enumerate(node.items):
                if not param.is_value():
                    continue
                arg_generic = arg_tuple[arg_idx]
                if param.type is None:
                    if parent_class and param_idx == 0 and param.name == "self":
                        # Special case: self in methods
                        arg_generic |= parent_class
                        parent_cache = utils.get_class(self.ctx, parent_class)
                        if (
                            parent_cache
                            and parent_cache.ast
                            and parent_cache.ast.has(ast.Attr.ClassDeduce)
                            and node.has(ast.Attr.ClassDeduce)
                            and node.name == "__init__"
                        ):
                            for unbound in arg_generic.get_unbounds(True):
                                node.set(ast.Attr.AllowPassThrough)
                                unbound.pass_through = True
                    else:
                        generic_types.append(arg_generic)
                elif param.name.startswith("*"):
                    # Special case: `*args: type` and `**kwargs: type`. Do not add this type to the
                    # signature (as the real type is `Tuple[type, ...]`); it will be used during
                    # call typechecking
                    generic_types.append(arg_generic)
                else:
                    param_type_expr = self.visit_expr(
                        param.type, enforce_type=True, simple_types=True
                    )
                    arg_generic |= utils.extract_type(self.ctx, param_type_expr)
                arg_idx += 1

            # Parse the return type
            ret = (
                self.visit_expr(node.ret, enforce_type=True, simple_types=True)
                if node.ret
                else None
            )
            ret_type = base_type[1]
            if ret:
                # Fix for functions returning Literal types
                return_static_kind = ast.get_static_generic(ret)
                if return_static_kind is not ast.types.Type.Behaviour.Runtime:
                    base_type.generics[1].static_kind = return_static_kind
                ret_type |= utils.extract_type(self.ctx, ret)
            else:
                ret_type |= utils.instantiate_unbound(self.ctx)
                generic_types.append(ret_type)

        # Generalize generics and remove them from the context
        for generic_type in generic_types:
            for unbound in generic_type.get_unbounds(False):
                if unbound_link := unbound.get_unbound():
                    unbound_link.kind = ast.types.Link.Kind.Generic

        # Parse function body
        if not node.has(ast.Attr.Internal) and not node.has(ast.Attr.C):
            if node.has(ast.Attr.LLVM):
                code = node.suite.first_in_block()
                assert code
                suite = transform_llvm_definition(self, code)
            elif node.has(ast.Attr.C):
                pass
            else:
                suite = node.suite.clone()
    node.set(ast.Attr.Module, self.ctx.module.path)

    # Make function AST and cache it for later realization
    function_ast = ast.FunctionStmt(
        canonical, ret=ret, items=arguments, suite=suite, async_=node.async_, done=True
    )
    function_ast.attributes = copy.deepcopy(node.attributes)
    if "_thunk_dispatch" in canonical:
        node.set(ast.Attr.AllowPassThrough)
    fn_data = cache.FunctionData(
        module=self.ctx.module_path,
        root_name=root_name,
        ast=function_ast,
        orig_ast=original_statement,
        is_toplevel=not self.ctx.module_name and self.ctx.is_global,
    )
    self.ctx.cache.functions[canonical] = fn_data
    parent_class = None
    if parent_name := node.get(ast.Attr.ParentClass):
        parent_item = self.ctx[parent_name]
        parent_class = utils.extract_class_type(self.ctx, parent_item.type)

    # Construct the type
    fn_type = ast.types.Function(
        base=base_type, ast=function_ast, func_generics=explicits, info=self.ctx.node_stack[-1].info
    )
    if is_class_member and node.has(ast.Attr.Method):
        fn_type.func_parent = parent_class
    fn_type = fn_type.generalize(self.ctx.typecheck_level)
    fn_data.type = fn_type
    overloads = self.ctx.cache.overloads[root_name]
    if root_name == "Tuple.__new__":
        insert_idx = len(overloads)
        for idx, overload_name in enumerate(overloads):
            overload = utils.get_function(self.ctx, overload_name)
            if (
                overload
                and overload.type
                and len(fn_type.func_generics) < len(overload.type.func_generics)
            ):
                insert_idx = idx
                break
        overloads.insert(insert_idx, canonical)
    else:
        overloads.append(canonical)
    self.ctx.add_item(node.name, root_name, fn_type)
    self.ctx.add_item(canonical, canonical, fn_type)
    if node.has(ast.Attr.Overload) or is_class_member:
        self.ctx.remove(node.name)  # first overload will handle it!

    # Special method handling
    if is_class_member:
        assert parent_class
        method_root = utils.get_class_method(
            self.ctx, parent_class, utils.get_unmangled_name(self.ctx, canonical)
        )
        found = False
        for overload_name in utils.get_overloads(self.ctx, method_root):
            if overload_name == canonical:
                fn_data = utils.get_function(self.ctx, overload_name)
                assert fn_data
                fn_data.type = fn_type
                found = True
                break
        assert found, f"cannot find matching class method for {canonical}"
    else:
        # Hack so that we can later use same helpers for class overloads
        cls_data = utils.get_class(self.ctx, cache.VAR_CLASS_TOPLEVEL)
        assert cls_data
        cls_data.methods[node.name] = root_name

    # Ensure that functions with @C, @force_realize, and @export attributes can be realized
    if (
        node.has(ast.Attr.ForceRealize)
        or node.has(ast.Attr.Export)
        or (node.has(ast.Attr.C) and not node.has(ast.Attr.CVarArg))
    ) and not fn_type.can_realize():
        raise TypecheckError(node, "builtin, exported and external functions cannot be generic")

    # Expression to be used if function binding is modified by captures or decorators
    final = None
    for decorator in reversed(node.decorators):
        if decorator:
            # Replace each decorator with `decorator(finalExpr)` in the reverse order
            if final:
                final = ast.CallExpr(decorator, items=[final])
            else:
                e = ast.IdExpr(canonical)
                e.set(ast.Attr.ExprDoNotRealize)
                final = ast.CallExpr(decorator, items=[e])
    if final:
        assign_lhs = ast.IdExpr(node.name)
        assign = ast.AssignStmt(assign_lhs, rhs=final)
        if is_class_member:  # class method decorator
            new_context = self.ctx.clone()
            new_context.bases.pop()
            del new_context.bases[1:]  # go to global context
            visitor = TypeVisitor(ctx=new_context)
            decorated_name = self.ctx.generate_canonical_name(node.name)
            declaration = visitor.visit(ast.AssignStmt(ast.IdExpr(decorated_name)))
            self.ctx.preamble.add(declaration)
            utils.register_global(self.ctx, decorated_name)
            assign.set_update()
            assign_lhs.value = decorated_name
            call_args = []
            for arg in node.items:
                if arg.name.startswith("**"):
                    call_args.append(ast.KeywordStarExpr(expr=ast.IdExpr(arg.name)))
                elif arg.name.startswith("*"):
                    call_args.append(ast.StarExpr(ast.IdExpr(arg.name)))
                else:
                    call_args.append(ast.IdExpr(arg.name))
            wrapper_fn = ast.FunctionStmt(
                node.name,
                ret=None if node.ret is None else node.ret.clone(),
                items=[argument.clone() for argument in node.items],
                suite=ast.ReturnStmt(expr=ast.CallExpr(ast.IdExpr(decorated_name), call_args)),
                async_=node.async_,
            )
            wrapper_fn = self.visit(wrapper_fn)
            assign = self.visit(assign)
            return ast.SuiteStmt(function_ast, ast.SuiteStmt(assign, wrapper_fn))
        assign = self.visit(assign)
        return ast.SuiteStmt(function_ast, assign)
    return function_ast


def transform_python_definition(
    self: TypeVisitor,
    name: str,
    arguments: List[ast.Param],
    ret: ast.Expr | None,
    code_stmt: ast.Stmt,
) -> ast.Stmt:
    """
    Transform Python code blocks.
    @example
    ```@python
    def foo(x: int, y) -> int:
    [code]
    ``` -> ```
    pyobj._exec("def foo(x, y): [code]")
    from python import __main__.foo(int, _) -> int
    ```
    """

    assert isinstance(code_stmt, ast.ExprStmt) and isinstance(code_stmt.expr, ast.StringExpr), (
        "invalid Python definition"
    )

    code = code_stmt.expr.value
    python_args = [arg.name for arg in arguments]
    code = f"def {name}({', '.join(python_args)}):\n{code}\n"
    imported_args = [arg.clone() for arg in arguments]
    transformed = ast.SuiteStmt(
        ast.ExprStmt(
            ast.CallExpr(
                ast.DotExpr(ast.IdExpr("pyobj"), member="_exec"),
                items=[ast.StringExpr(code)],
            )
        ),
        ast.ImportStmt(
            ast.DotExpr(ast.IdExpr("__main__"), member=name),
            ast.IdExpr("python"),
            args=imported_args,
            ret=ret.clone() if ret else ast.IdExpr("pyobj"),
        ),
    )
    return self.visit_stmt(transformed)


def transform_llvm_definition(self: TypeVisitor, code_stmt: ast.Stmt) -> ast.Stmt:
    """
    Transform LLVM functions.
    @example
    ```@llvm
    def foo(x: int) -> float:
    [code]
    ``` -> ```
    def foo(x: int) -> float:
    StringExpr("[code]")
    SuiteStmt(referenced_types)
    ```
    As LLVM code can reference types and static expressions in `{=expr}` blocks,
    all block expression will be stored in the `referenced_types` suite.
    "[code]" is transformed accordingly: each `{=expr}` block will
    be replaced with `{}` so that @c fmt::format can fill the gaps.
    Note that any brace (`{` or `}`) that is not part of a block is
    escaped (e.g. `{` -> `{{` and `}` -> `}}`) so that @c fmt::format can process them.
    """

    assert isinstance(code_stmt, ast.ExprStmt) and isinstance(code_stmt.expr, ast.StringExpr), (
        "invalid LLVM definition"
    )

    def escape_braces(value, start, count):
        return value[start : start + count].replace("{", "{{").replace("}", "}}")

    code = code_stmt.expr.value
    # Remove docstring (if any)
    start = 0
    while start < len(code) and code[start].isspace():
        start += 1
    if code[start:].startswith('"""'):
        start += 3
        found = False
        while start < len(code) - 2:
            if code[start : start + 3] == '"""':
                found = True
                start += 3
                break
            start += 1
        if found:
            code = code[start:]

    items = []
    final_code = ""

    # Parse LLVM code and look for expression blocks that start with `{=`
    brace_count, brace_start = 0, 0
    index = 0

    code = list(code)  # so that we can modify it
    while index < len(code):
        if index < len(code) - 1 and code[index] == "\\" and code[index + 1] == "\n":
            code[index] = " "
            code[index + 1] = " "
        if index < len(code) - 1 and code[index] == "{" and code[index + 1] == "=":
            if brace_start <= index:
                final_code += escape_braces("".join(code), brace_start, index - brace_start) + "{"
            if brace_count == 0:
                brace_start = index + 2
                brace_count += 1
            else:
                raise TypecheckError(code_stmt, "invalid LLVM code")
        elif brace_count and code[index] == "}":
            brace_count -= 1
            expression_code = "".join(code[brace_start:index])
            offset = self.ctx.node_stack[-1].info
            offset.col += index

            parsed = self.ctx.cache.parse(expr=expression_code)
            assert isinstance(parsed, ast.Expr)
            parsed.info = offset
            items.append(ast.ExprStmt(parsed))
            brace_start = index + 1
            final_code += "}"
        index += 1
    if brace_count:
        raise TypecheckError(code_stmt, "invalid LLVM code")
    if brace_start != len(code):
        final_code += escape_braces("".join(code), brace_start, len(code) - brace_start)
    return ast.SuiteStmt(ast.ExprStmt(ast.StringExpr(final_code)), *items)


def get_decorator(self: TypeVisitor, expression: ast.Expr):
    """
    Fetch a decorator canonical name. The first pair member indicates if a decorator is
    actually an attribute (a function with `@__attribute__`).
    """

    decorator = self.visit_expr(expression.clone())
    decorator = utils.get_head_expr(decorator)

    match decorator:
        case ast.CallExpr(expr=ast.IdExpr() as expr):
            identifier = expr
        case ast.IdExpr() as expr:
            identifier = expr
        case _:
            identifier = None
    if identifier:
        item = self.ctx.find_at(identifier.value, self.ctx.time)
        if item and item.func:
            fn_name = item.func.func_name
            fn_data = utils.get_function(self.ctx, fn_name)
            if not fn_data:
                if (overloads := self.ctx.cache.overloads.get(fn_name)) and len(overloads) == 1:
                    fn_data = utils.get_function(self.ctx, overloads[0])

            # Special case: Id to Call
            if (
                fn_data
                and fn_data.ast
                and fn_data.ast.has(ast.Attr.Attribute)
                and isinstance(decorator, ast.IdExpr)
            ):
                decorator = self.visit_expr(ast.CallExpr(decorator))

            if fn_data and fn_data.ast:
                return (
                    fn_data.ast.has(ast.Attr.Attribute),
                    fn_name,
                    identifier.value if decorator.done else "",
                )
    return False, "", ""


def get_func_type_base(self: TypeVisitor, argument_count: int) -> ast.types.Class:
    """Generate and return `Function[Tuple[args...], ret]` type"""

    base = utils.instantiate(
        self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Function)
    ).require_cls
    base.generics[0].type |= utils.instantiate(
        self.ctx, classes.generate_tuple(self.ctx, argument_count, False)
    )
    return base
