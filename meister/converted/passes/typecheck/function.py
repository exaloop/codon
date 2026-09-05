# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast, cache, error
from ..scope import Bindings, ScopingVisitor
from . import classes, infer, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_lambda(self: TypeVisitor, node: ast.LambdaExpr) -> ast.Node:
    """
    Unify the function return type with `Generator[?]`.
    The unbound type will be deduced from return/yield statements.
    """

    function = ast.FunctionStmt(
        name=utils.get_temporary_var(self.ctx, "lambda"),
        items=[param.clone() for param in node.items],
        suite=ast.ReturnStmt(node.expr),
    )
    # TODO: just copy BindingsAttribute from expr instead?
    self.ctx.cache.scope(ast.SuiteStmt(function))
    function.set(ast.Attr.ExprTime, self.ctx.time)  # to handle captures properly
    function = self.visit(function)
    if bindings := node.get(ast.Attr.Bindings):
        function.set(ast.Attr.Bindings, bindings.clone())
    self.prepend_stmts.append(function)
    return self.visit(ast.IdExpr(function.name))


def typecheck_yieldexpr(self: TypeVisitor, node: ast.YieldExpr) -> ast.Node:
    """
    Unify the function return type with `Generator[?]`.
    The unbound type will be deduced from return/yield statements.
    """
    if not self.ctx.in_function():
        raise TypecheckError(node, "'yield' outside function")

    base = self.ctx.get_base()
    infer.unify(
        base.return_type,
        utils.instantiate_type(self.ctx, utils.get_stdlib_type(self.ctx, "Generator"), [node.type]),
    )
    if infer.realize(node.type):
        node.done = True
    return node


def typecheck_await(self: TypeVisitor, node: ast.AwaitExpr) -> ast.Node:
    """Typecheck await statements."""
    if not self.ctx.in_function():
        raise TypecheckError(node, "'await' outside function")
    base = self.ctx.get_base()
    if not base.func.async_:
        raise TypecheckError(node, "'await' outside function")

    node.expr = self.visit(node.expr)
    if not node.transformed and (expr_type := node.expr.type.get_class()):
        is_coroutine = (
            expr_type.is_type(ast.types.Stdlib.Coroutine)
            or expr_type.is_type(ast.types.mangle("std.asyncio", cls="Future"))
            or expr_type.is_type(ast.types.mangle("std.asyncio", cls="Task"))
        )
        if not is_coroutine:
            if utils.find_method(self.ctx, expr_type, "__await__"):
                awaited = self.visit(ast.CallExpr(ast.DotExpr(node.expr, member="__await__")))
                is_coroutine = (
                    awaited.type.is_type(ast.types.Stdlib.Coroutine)
                    or awaited.type.is_type(ast.types.mangle("std.asyncio", cls="Future"))
                    or awaited.type.is_type(ast.types.mangle("std.asyncio", cls="Task"))
                    or awaited.type.is_type(ast.types.Stdlib.Generator)
                )
                if not is_coroutine:
                    raise TypecheckError(node, "expected awaitable expression")
                node.expr = awaited
                node.transformed = True
            else:
                raise TypecheckError(node, "expected awaitable expression")
    if typ := node.expr.type.get_class():
        infer.unify(node.type, typ[0])
    if node.expr.done:
        node.done = True
    return node


def typecheck_return(self: TypeVisitor, node: ast.ReturnStmt) -> ast.Node:
    """
    Typecheck return statements. Empty return is transformed to `return NoneType()`.
    Also partialize functions if they are being returned.
    See @c wrapExpr for more details.
    """

    if node.has(ast.Attr.Internal):
        node.expr = self.visit(
            ast.CallExpr(ast.IdExpr(ast.types.mangle(cls="NoneType", func="__new__")))
        )
        node.done = True
        return node
    if not self.ctx.in_function():
        raise TypecheckError(node, "'return' outside function")

    base = self.ctx.get_base()
    is_async = base.func.async_
    if node.expr is None and base.func.has(ast.Attr.IsGenerator):
        node.done = True
    else:
        if base.func.has(ast.Attr.IsGenerator):
            raise TypecheckError(node, "returning values from generators not yet supported")
        if node.expr is None:
            node.expr = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
        node.expr = self.visit(node.expr)

        # Wrap expression to match the return type
        if base.return_type.get_unbound() is None:
            can_wrap, node.expr = utils.wrap_expr(self, node.expr, base.return_type)
            if not can_wrap:
                return node
        fn_type = node.expr.type.get_func()
        ret_type = base.return_type.get_class()
        # Special case: partialize functions if we are returning them
        if fn_type and not (ret_type and base.return_type.is_type(ast.types.Stdlib.Function)):
            partial = ast.CallExpr(
                ast.IdExpr(fn_type.ast.name),
                items=[ast.EllipsisExpr(ast.EllipsisExpr.Kind.PARTIAL)],
            )
            node.expr = self.visit(partial)
        if (
            base.return_type.get_static_kind() is ast.types.Type.Behaviour.Runtime
            and node.expr.type.get_static()
        ):
            node.expr.type = node.expr.type.get_static().get_non_static_type()
        if is_async:
            infer.unify(
                base.return_type,
                utils.instantiate_type(
                    self.ctx,
                    utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Coroutine),
                    [node.expr.type],
                ),
            )
        else:
            infer.unify(base.return_type, node.expr.type)

    if self.ctx.block_level == 0:
        # If we are not within conditional block, ignore later statements in this function.
        # Useful with static if statements.
        self.ctx.return_early = True

    if node.expr is None or node.expr.done:
        node.done = True
    return node


def typecheck_yield(self: TypeVisitor, node: ast.YieldStmt) -> ast.Node:
    """Typecheck yield statements. Empty yields assume `NoneType`."""
    if not self.ctx.in_function():
        raise TypecheckError(node, "'yield' outside function")

    base = self.ctx.get_base()
    expression = node.expr or ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NoneType))
    node.expr = self.visit(expression)
    infer.unify(
        base.return_type,
        utils.instantiate_type(
            self.ctx,
            utils.get_stdlib_type(self.ctx, "AsyncGenerator" if base.func.async_ else "Generator"),
            [node.expr.type],
        ),
    )
    if node.expr.done:
        node.done = True
    return node


def typecheck_yieldfrom(self: TypeVisitor, node: ast.YieldFromStmt) -> ast.Node:
    """
    Transform `yield from` statements.
    @example
    `yield from a` -> `for var in a: yield var`
    """
    var = utils.get_temporary_var(self.ctx, "yield")
    result = ast.ForStmt(ast.IdExpr(var), iter=node.expr, suite=ast.YieldStmt(ast.IdExpr(var)))
    return self.visit(result)


def typecheck_global(self: TypeVisitor, node: ast.GlobalStmt) -> ast.Node:
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
    for idx in range(len(node.decorators) - 1, -1, -1):
        decorator = node.decorators[idx]
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
                node.decorators[idx] = None  # ignore it later on
        if not is_attr:
            has_decorators = True

    is_class_member = self.ctx.in_class()
    if node.has(ast.Attr.ForceRealize) and (not self.ctx.is_global() or is_class_member):
        raise TypecheckError(node, "builtin must be a top-level statement")

    # All overloads share the same canonical name except for the number at the
    # end (e.g., `foo.1:0`, `foo.1:1` etc.)
    root_name = ""
    current_base = self.ctx.get_base()
    if is_class_member:
        # Case 1: method overload
        current_class = utils.get_class(self.ctx, current_base.name)
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
            (existing := self.ctx.find(node.name, self.ctx.time))
            and existing.is_func()
            and existing.module_name == self.ctx.get_module()
            and existing.base_name == self.ctx.get_base_name()
        ):
            root_name = existing.canonical_name
    if not root_name:
        root_name = self.ctx.generate_canonical_name(node.name, True, is_class_member)
    # Append overload number to the name
    canonical_name = root_name
    self.ctx.cache.overloads.setdefault(root_name, [])
    canonical_name += f":{len(utils.get_overloads(self.ctx, root_name))}"
    self.ctx.cache.reverse_identifier_lookup[canonical_name] = node.name

    if is_class_member:
        # Set the enclosing class name
        node.set(ast.Attr.ParentClass, current_base.name)
        # Add the method to the class' method list
        cls_data = utils.get_class(self.ctx, current_base.name)
        cls_data.methods[node.name] = root_name

    # Handle captures. Add additional argument to the function for every capture.
    # Make sure to account for **kwargs if present
    if bindings := node.get(ast.Attr.Bindings):
        insert_idx = len(node.items)
        if node.items and node.items[-1].name.startswith("**"):
            insert_idx -= 1
        for captured_name, capture_type in bindings.captures.items():
            capture_arg = f"${captured_name}"
            if (
                (captured_item := self.ctx.find(captured_name, self.ctx.time))
                and capture_type is not Bindings.Scope.Global
                and not captured_item.is_global()
            ):
                parent_class_generic = (
                    self.ctx.bases[-1].is_type()
                    and self.ctx.bases[-1].name == captured_item.base_name
                )
                if captured_item.generic and parent_class_generic:
                    node.set(ast.Attr.Method)
                if not captured_item.generic or (
                    captured_item.get_static_kind() is not ast.types.Type.Behaviour.Runtime
                    and not parent_class_generic
                ):
                    if not captured_item.is_func():
                        if captured_item.is_type():
                            node.items.insert(
                                insert_idx,
                                ast.Param(capture_arg, type=ast.IdExpr(ast.types.Stdlib.Type)),
                            )
                        elif (
                            captured_item.get_static_kind() is not ast.types.Type.Behaviour.Runtime
                        ):
                            type_name = ast.types.Type.string_from_literal(
                                captured_item.get_static_kind()
                            )
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
                        bindings.local_renames[captured_name] = captured_item.canonical_name
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
    is_global = self.ctx.is_global()

    with self.ctx.within_base(canonical_name):
        fn_base = self.ctx.get_base()
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
                    default_name = f".default.{canonical_name}.{param.name}"
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
                        self.preamble.add(declaration)
                        utils.register_global(self.ctx, default_name)
                        assignment.set_update()
                    elif is_global:
                        utils.register_global(self.ctx, default_name)
                    self.prepend_stmts.append(default_visitor.visit(assignment))
                    default_item = self.ctx.force_find(default_name)
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
                transformed_generic_default = self.visit(
                    None if param.default_value is None else param.default_value.clone()
                )
                default_type = None
                if isinstance(transformed_generic_default, ast.Expr):
                    default_type = utils.extract_type(self.ctx, transformed_generic_default)
                static_kind = ast.get_static_generic(param.type)
                if static_kind is not ast.types.Type.Behaviour.Runtime:
                    generic_item = self.ctx.add(var_name, param_name, generic_type)
                    generic_item.generic = True
                    generic_type.static_kind = static_kind
                    if default_type:
                        generic_type.default_type = default_type
                else:
                    match param.type:
                        case ast.InstantiateExpr(
                            expr=ast.IdExpr(value=ast.types.Stdlib.TypeTrait), items=[type_expr, *_]
                        ):
                            # Parse TraitVar
                            trait_expr = self.visit(type_expr, enforce_type=True, simple_types=True)
                            trait_type = trait_expr.type.get_link()
                            generic_type.trait = (
                                trait_type.trait
                                if trait_type and trait_type.trait
                                else ast.types.TypeTrait(trait_expr.type)
                            )
                    generic_item = self.ctx.add(var_name, param_name, generic_type)
                    generic_item.generic = True
                    if default_type:
                        generic_type.default_type = default_type
                generalized = generic_type.generalize(self.ctx.typecheck_level)
                var_name = var_name.removeprefix("$")
                explicits.append(
                    ast.types.Generic(
                        param_name, generalized, type_id, generalized.get_static_kind()
                    )
                )

        # Prepare list of all generic types
        parent_class = None
        if is_class_member and node.has(ast.Attr.Method):
            # Get class generics (e.g., T for `class Cls[T]: def foo:`)
            parent_item = self.ctx.force_find(node.get(ast.Attr.ParentClass).value)
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
                arg.type = self.visit(arg.type, enforce_type=True, simple_types=True)

            # Unify base type generics with argument types. Add non-generic arguments to the
            # context. Delayed to prevent cases like `def foo(a, b=a)`
            arg_tuple = base_type[0].get_class()
            arg_idx = 0
            for param_idx, param in enumerate(node.items):
                if not param.is_value():
                    continue
                arg_generic = arg_tuple[arg_idx]
                if param.type is None:
                    if parent_class and param_idx == 0 and param.name == "self":
                        # Special case: self in methods
                        unified_self = infer.unify(arg_generic, parent_class)
                        parent_cache = utils.get_class(self.ctx, parent_class)
                        if (
                            parent_cache
                            and parent_cache.ast
                            and parent_cache.ast.has(ast.Attr.ClassDeduce)
                            and node.has(ast.Attr.ClassDeduce)
                            and node.name == "__init__"
                        ):
                            for unbound in unified_self.get_unbounds(True):
                                node.set(ast.Attr.AllowPassThrough)
                                link = unbound.get_link()
                                if link:
                                    link.pass_through = True
                    else:
                        generic_types.append(arg_generic)
                elif param.name.startswith("*"):
                    # Special case: `*args: type` and `**kwargs: type`. Do not add this type to the
                    # signature (as the real type is `Tuple[type, ...]`); it will be used during
                    # call typechecking
                    generic_types.append(arg_generic)
                else:
                    param_type_expr = self.visit(param.type, enforce_type=True, simple_types=True)
                    infer.unify(arg_generic, utils.extract_type(self.ctx, param_type_expr))
                arg_idx += 1

            # Parse the return type
            ret = self.visit(node.ret, enforce_type=True, simple_types=True)
            ret_type = base_type[1]
            if ret:
                # Fix for functions returning Literal types
                return_static_kind = ast.get_static_generic(ret)
                if return_static_kind is not ast.types.Type.Behaviour.Runtime:
                    base_type.generics[1].static_kind = return_static_kind
                infer.unify(ret_type, utils.extract_type(self.ctx, ret))
            else:
                generic_types.append(infer.unify(ret_type, utils.instantiate_unbound(self.ctx)))

        # Generalize generics and remove them from the context
        for generic_type in generic_types:
            for unbound in generic_type.get_unbounds(False):
                if unbound_link := unbound.get_unbound():
                    unbound_link.kind = ast.types.Link.Kind.Generic

        # Parse function body
        if not node.has(ast.Attr.Internal) and not node.has(ast.Attr.C):
            if node.has(ast.Attr.LLVM):
                suite = transform_llvm_definition(self, node.suite.first_in_block())
            elif node.has(ast.Attr.C):
                pass
            else:
                suite = node.suite.clone()
    node.set(ast.Attr.Module, self.ctx.module_name.path)

    # Make function AST and cache it for later realization
    function_ast = ast.FunctionStmt(
        canonical_name, ret=ret, items=arguments, suite=suite, async_=node.async_, done=True
    )
    function_ast.attributes = copy.deepcopy(node.attributes)
    if "_thunk_dispatch" in canonical_name:
        node.set(ast.Attr.AllowPassThrough)
    fn_data = cache.FunctionData(
        module=self.ctx.get_module_path(),
        root_name=root_name,
        ast=function_ast,
        orig_ast=original_statement,
        is_toplevel=not self.ctx.get_module() and self.ctx.is_global(),
    )
    self.ctx.cache.functions[canonical_name] = fn_data
    parent_class = None
    if parent_name := node.get(ast.Attr.ParentClass).value:
        parent_item = self.ctx.force_find(parent_name)
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
        overloads.insert(insert_idx, canonical_name)
    else:
        overloads.append(canonical_name)
    self.ctx.add(node.name, root_name, fn_type)
    self.ctx.add(canonical_name, canonical_name, fn_type)
    if node.has(ast.Attr.Overload) or is_class_member:
        self.ctx.remove(node.name)  # first overload will handle it!

    # Special method handling
    if is_class_member:
        method_root = utils.get_class_method(
            self.ctx, parent_class, utils.get_unmangled_name(self.ctx, canonical_name)
        )
        found = False
        for overload_name in utils.get_overloads(self.ctx, method_root):
            if overload_name == canonical_name:
                matching_function = utils.get_function(self.ctx, overload_name)
                matching_function.type = fn_type
                found = True
                break
        assert found, f"cannot find matching class method for {canonical_name}"
    else:
        # Hack so that we can later use same helpers for class overloads
        top_level_class = utils.get_class(self.ctx, cache.VAR_CLASS_TOPLEVEL)
        top_level_class.methods[node.name] = root_name

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
                e = ast.IdExpr(canonical_name)
                e.set(ast.Attr.ExprDoNotRealize)
                final = ast.CallExpr(decorator, items=[e])
    if final:
        assign = ast.AssignStmt(ast.IdExpr(node.name), rhs=final)
        if is_class_member:  # class method decorator
            new_context = self.ctx.clone()
            new_context.bases.pop()
            del new_context.bases[1:]  # go to global context
            visitor = TypeVisitor(ctx=new_context)
            decorated_name = self.ctx.generate_canonical_name(node.name)
            declaration = visitor.visit(ast.AssignStmt(ast.IdExpr(decorated_name)))
            self.preamble.add_stmt(declaration)
            utils.register_global(self.ctx, decorated_name)
            assign.set_update()
            assign.lhs.value = decorated_name
            call_args = []
            for arg in node.items:
                if arg.name.startswith("**"):
                    call_args.append(ast.KeywordStarExpr(ast.IdExpr(arg.name)))
                elif arg.name.startswith("*"):
                    call_args.append(ast.StarExpr(ast.IdExpr(arg.name)))
                else:
                    call_args.append(ast.IdExpr(arg.name))
            wrapper_fn = ast.FunctionStmt(
                node.name,
                ret=None if node.ret is None else node.ret.clone(),
                items=[argument.clone() for argument in node.items],
                suite=ast.ReturnStmt(ast.CallExpr(ast.IdExpr(decorated_name), items=call_args)),
                async_=node.async_,
            )
            wrapper_fn = self.visit(wrapper_fn)
            assign = self.visit(assign)
            return ast.SuiteStmt(function_ast, ast.SuiteStmt([assign, wrapper_fn]))
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
            ast.IdExpr("python"),
            what=ast.DotExpr(ast.IdExpr("__main__"), member=name),
            args=imported_args,
            ret=ret.clone() if ret else ast.IdExpr("pyobj"),
        ),
    )
    return self.visit(transformed)


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

            parsed = parse_string(expression_code, "eval")
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

    decorator = self.visit(expression.clone())
    decorator = utils.get_head_expr(decorator)

    match decorator:
        case ast.CallExpr(expr=ast.IdExpr() as expr):
            identifier = expr
        case ast.IdExpr() as expr:
            identifier = expr
        case _:
            identifier = None
    if identifier:
        item = self.ctx.find(identifier.value, self.ctx.time)
        if item and item.is_func():
            fn_name = item.type.get_func().ast.name
            fn = utils.get_function(self.ctx, fn_name)
            if fn is None:
                if (overloads := self.ctx.cache.overloads.get(fn_name)) and len(overloads) == 1:
                    fn = utils.get_function(self.ctx, overloads[0])

            # Special case: Id to Call
            if fn and fn.ast.has(ast.Attr.Attribute) and isinstance(decorator, ast.IdExpr):
                decorator = self.visit(ast.CallExpr(decorator))

            if fn:
                return (
                    fn.ast.has(ast.Attr.Attribute),
                    fn_name,
                    identifier.value if decorator.done else "",
                )
    return False, "", ""


def get_func_type_base(self: TypeVisitor, argument_count: int) -> ast.types.Class:
    """Generate and return `Function[Tuple[args...], ret]` type"""

    base = utils.instantiate_type(
        self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Function)
    ).get_class()
    infer.unify(
        base[0],
        utils.instantiate_type(self.ctx, classes.generate_tuple(self, argument_count, False)),
    )
    return base
