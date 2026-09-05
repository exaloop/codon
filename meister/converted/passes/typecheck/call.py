# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast, cache
from . import classes, infer, ops, special, utils
from .ctx import TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_print(self: TypeVisitor, node: ast.PrintStmt) -> ast.Node:
    """
    Transform print statement.
    @example
    `print a, b` -> `print(a, b)`
    `print a, b,` -> `print(a, b, end=' ')`
    """

    args = [ast.CallExpr.Arg(e) for e in node.items]
    if not node.has_newline():
        args.append(ast.CallExpr.Arg(name="end", value=ast.StringExpr(" ")))
    result = ast.ExprStmt(ast.CallExpr(ast.IdExpr("print"), items=args))
    return self.visit(result)


def typecheck_star(self: TypeVisitor, node: ast.StarExpr) -> ast.Node:
    """Just ensure that this expression is not independent of CallExpr where it is handled."""
    raise TypecheckError(node, "unexpected star expression")


def typecheck_keywordstar(self: TypeVisitor, node: ast.KeywordStarExpr) -> ast.Node:
    """Just ensure that this expression is not independent of CallExpr where it is handled."""
    raise TypecheckError(node, "unexpected keyword-star expression")


def typecheck_ellipsis(self: TypeVisitor, node: ast.EllipsisExpr) -> ast.Node:
    """
    Typechecks an ellipsis. Ellipses are typically replaced during the typechecking; the
    only remaining ellipses are those that belong to PipeExprs.
    """

    if node.is_pipe() and infer.realize(self, node.type):
        node.done = True
    elif node.is_standalone():
        result = self.visit(ast.CallExpr(ast.IdExpr("ellipsis")))
        infer.unify(node.type, result.type)
        return result
    return node


def typecheck_call(self: TypeVisitor, node: ast.CallExpr) -> ast.Node:
    """
    Typecheck a call expression. This is the most complex expression to typecheck.
    @example
    `fn(1, 2, x=3, y=4)` -> `func(a=1, x=3, args=(2,), kwargs=KwArgs(y=4), T=int)`
    `fn(arg1, ...)`      -> `(_v = Partial.N10(arg1); _v)`
    See @c transformCallArgs , @c getCalleeFn , @c callReorderArguments ,
    @c typecheckCallArgs , @c transformSpecialCall and @c wrapExpr for more details.
    """

    if self.ctx.simple_types:
        raise TypecheckError(node, "cannot use calls in type signatures")
    if isinstance(node.expr, ast.IdExpr) and node.expr.value == "tuple" and len(node.items) == 1:
        node.set(ast.Attr.TupleCall)

    validate_call(node)

    # Check if this call is partial call
    partial = utils.PartialCallData()
    if node.items:
        last = node.items[-1].value
        partial.is_partial = isinstance(last, ast.EllipsisExpr) and last.is_partial()

    # Do not allow realization here (function will be realized later);
    # used to prevent early realization of compile_error
    node.set(ast.Attr.ParentCallExpr)
    if partial.is_partial:
        node.expr.set(ast.Attr.ExprDoNotRealize)
    node.expr = self.visit(node.expr)
    node.erase(ast.Attr.ParentCallExpr)
    if utils.is_unbound(node.expr):
        return node  # delay

    callee_fn, new_expr = get_callee_fn(self, node, partial)

    # Transform `tuple(i for i in tup)` into a GeneratorExpr
    # that will be handled during the type checking.
    if callee_fn is None and node.has(ast.Attr.TupleCall):
        first = node.items[0].value
        if isinstance(first, ast.GeneratorExpr):
            if first.kind is not ast.GeneratorExpr.Kind.Generator or first.loop_count() != 1:
                raise TypecheckError(
                    node,
                    "tuple constructor does not accept nested or conditioned comprehensions",
                )
            first.kind = ast.GeneratorExpr.Kind.TupleGenerator
            return self.visit(first)
        return special.transform_tuple_fn(self, node)
    elif new_expr:
        return new_expr
    elif callee_fn is None:
        return node

    with utils.with_class_generics(self.ctx, callee_fn, transform_arguments, True, True):
        if not transform_call_args(self, node):
            return node

    # Early dispatch modifier
    if utils.is_dispatch(callee_fn):
        if callee_fn.get_func_name().startswith("Tuple.__new__"):
            classes.generate_tuple(self, len(node.items))
        matching = None
        head_expr = utils.get_head_expr(node.expr)
        if isinstance(head_expr, ast.IdExpr) and not partial.var:
            # Case: function overloads (IdExpr)
            # Make sure to ignore partial constructs (they are also StmtExpr(IdExpr, ...))
            methods = []
            key = head_expr.value
            if utils.is_dispatch(key):
                key = key.removesuffix(cache.FN_DISPATCH_SUFFIX)
            for overload in utils.get_overloads(self.ctx, key):
                if not utils.is_dispatch(overload):
                    function = utils.get_function(self.ctx, overload)
                    methods.append(function.type)
            methods.reverse()
            parent_class = (
                None if callee_fn.func_parent is None else callee_fn.func_parent.get_class()
            )
            partial_class = None if node.expr.type is None else node.expr.type.get_partial()
            matching = utils.find_matching_methods(
                self.ctx, parent_class, methods, node.items, partial_class
            )
        # partials have dangling ellipsis that messes up with the unbound check below
        do_dispatch = matching is None or not matching or partial.is_partial
        if not do_dispatch and matching is not None and len(matching) > 1:
            for arg in node.items:
                if utils.is_unbound(arg.value):
                    return node  # typecheck this later once we know the argument
        if not do_dispatch:
            parent_class = (
                None if callee_fn.func_parent is None else callee_fn.func_parent.get_class()
            )
            instantiated = utils.instantiate_type(self.ctx, matching[0], parent_class)
            callee_fn = instantiated.get_func()
            identifier = ast.IdExpr(callee_fn.get_func_name(), type=callee_fn)
            if isinstance(node.expr, ast.IdExpr):
                node.expr = identifier
            elif isinstance(node.expr, ast.StmtExpr):
                expr = node.expr
                while isinstance(expr.expr, ast.StmtExpr):
                    expr = expr.expr
                expr.expr = identifier
            else:
                node.expr = ast.StmtExpr(ast.ExprStmt(node.expr), expr=identifier)
            node.expr.type = callee_fn
        elif matching is not None and not matching:
            arg_names = [
                arg.value.get_class_type().name
                if arg.value.type.get_static() and arg.value.get_class_type()
                else arg.value.type.pretty_string()
                for arg in node.items
            ]
            name = utils.get_unmangled_name(self.ctx, callee_fn.get_func_name())
            parent_name = callee_fn.ast.get(ast.Attr.ParentClass, "")
            if parent_name:
                name = f"{utils.get_user_facing_name(self.ctx, parent_name)}.{name}"
            raise TypecheckError(
                node,
                f"no function '{name}' with arguments ({', '.join(arg_names)})",
            )

    # Handle named and default arguments
    reordered = call_reorder_arguments(self, callee_fn, node, partial)
    if reordered:
        return reordered

    # Handle special calls
    if not partial.is_partial:
        is_special, special = transform_special_call(self, node)
        if is_special:
            return special or node

    # Typecheck arguments with the function signature
    done = typecheck_call_args(self, callee_fn, node.items, partial)
    if not partial.is_partial and callee_fn.can_realize():
        # Previous unifications can qualify existing identifiers.
        # Transform again to get the full identifier
        node.expr = self.visit(node.expr)
    done = done and node.expr.done

    # Final call
    if partial.is_partial:
        # Case: partial call. `calleeFn(args...)` -> `Partial(args..., fn, mask)`
        new_args = []
        for arg in node.items:
            if not isinstance(arg.value, ast.EllipsisExpr):
                new_args.append(arg.value)
                new_args[-1].set(ast.Attr.ExprSequenceItem)
        new_args.append(partial.args)
        partial_call = generate_partial_call(
            self,
            partial.known,
            callee_fn.get_func(),
            ast.TupleExpr(new_args),
            partial.kw_args,
        )
        var_name = utils.get_temporary_var(self.ctx, "part")
        if partial.var:
            # Callee is already a partial call
            stmts = [s.clone() for s in node.expr.items]
            call = ast.StmtExpr(
                [*stmts, ast.AssignStmt(ast.IdExpr(var_name), rhs=partial_call)],
                expr=ast.IdExpr(var_name),
            )
        else:
            # New partial call: `(part = Partial(stored_args...); part)`
            call = ast.StmtExpr(
                [ast.AssignStmt(ast.IdExpr(var_name), rhs=partial_call)],
                expr=ast.IdExpr(var_name),
            )
        call.set(ast.Attr.ExprPartial)
        return self.visit(call)
    else:
        infer.unify(node.type, callee_fn.get_ret_type())
        if done:
            node.done = True
        return node


def validate_call(expr: ast.CallExpr):
    if expr.has(ast.Attr.Validated):
        return
    names_started = False
    found_ellipsis = False
    for arg in expr.items:
        if (
            not arg.name
            and names_started
            and not isinstance(arg.value, (ast.KeywordStarExpr, ast.EllipsisExpr))
        ):
            raise TypecheckError(expr, "positional argument follows keyword argument")
        if arg.name and isinstance(arg.value, (ast.StarExpr, ast.KeywordStarExpr)):
            raise TypecheckError(expr, "cannot use starred expression here")
        if isinstance(arg.value, ast.EllipsisExpr) and found_ellipsis:
            raise TypecheckError(expr, "multiple ellipsis expressions")
        found_ellipsis = found_ellipsis or isinstance(arg.value, ast.EllipsisExpr)
        names_started = names_started or bool(arg.name)
    expr.set(ast.Attr.Validated)


def transform_call_args(self: TypeVisitor, expr: ast.CallExpr):
    """
    Transform call arguments. Expand *args and **kwargs to the list of @c CallArg
    objects.
    @return false if expansion could not be completed; true otherwise
    """

    arg_index = 0
    while arg_index < len(expr.items):
        arg = expr.items[arg_index]
        if isinstance(arg.value, ast.StarExpr):
            # Case: *args expansion
            star = arg.value
            star.expr = self.visit(star.expr)
            tuple_type = star.expr.get_class_type()
            while tuple_type and tuple_type.is_type(ast.types.Stdlib.Optional):
                star.expr = self.visit(
                    ast.CallExpr(ast.IdExpr(ast.types.Stdlib.OptionalUnwrap), items=[star.expr])
                )
                tuple_type = star.expr.get_class_type()

            # Process later
            if not tuple_type:
                return False
            if not tuple_type.is_record():
                raise TypecheckError(
                    star,
                    f"argument after * must be a tuple, not '{tuple_type.pretty_string()}'",
                )
            fields = utils.get_class_fields(tuple_type)
            head = star.expr
            lead = None
            if utils.has_side_effect(head):
                var = utils.get_temporary_var(self.ctx, "star")
                lead = ast.AssignExpr(ast.IdExpr(var), expr=head)
                head = ast.IdExpr(var)
            inserted = []
            for field_idx, field in enumerate(fields):
                base = ((lead if lead and field_idx == 0 else head).clone(),)
                inserted.append(self.visit(ast.DotExpr(base, member=field.name)))
            expr.items[arg_index : arg_index + 1] = inserted
            arg_index += len(inserted)
        elif isinstance(arg.value, ast.KeywordStarExpr):
            # Case: **kwargs expansion
            kwstar = arg.value
            kwstar.expr = self.visit(kwstar.expr)
            named_type = kwstar.expr.get_class_type()
            while named_type and named_type.is_type(ast.types.Stdlib.Optional):
                kwstar.expr = self.visit(
                    ast.CallExpr(ast.IdExpr(ast.types.Stdlib.OptionalUnwrap), items=[kwstar.expr])
                )
                named_type = kwstar.expr.get_class_type()
            if not named_type:
                return False
            head = kwstar.expr
            lead = None
            if utils.has_side_effect(head):
                var = utils.get_temporary_var(self.ctx, "star")
                lead = ast.AssignExpr(ast.IdExpr(var), expr=head)
                head = ast.IdExpr(var)
            inserted = []
            if named_type.is_type(ast.types.Stdlib.NamedTuple):
                tuple_id = utils.get_int_literal(named_type)
                assert 0 <= tuple_id < len(self.ctx.cache.generated_tuple_names)
                names = self.ctx.cache.generated_tuple_names[tuple_id]
                for field_idx, name in enumerate(names):
                    base = (lead if lead and field_idx == 0 else head).clone()
                    field = self.visit(
                        ast.DotExpr(ast.DotExpr(base, member="args"), member=f"item{field_idx + 1}")
                    )
                    inserted.append(ast.CallExpr.Arg(name, value=field))
            elif named_type.is_record():
                fields = utils.get_class_fields(named_type)
                for field_idx, field in enumerate(fields):
                    base = (lead if lead and field_idx == 0 else head).clone()
                    field = self.visit(ast.DotExpr(base, member=field.name))
                    inserted.append(ast.CallExpr.Arg(field.name, value=field))
            else:
                raise TypecheckError(
                    kwstar,
                    f"argument after ** must be a named tuple, not '{named_type.pretty_string()}'",
                )
            expr.items[arg_index : arg_index + 1] = inserted
            arg_index += len(inserted)
        else:
            # Case: normal argument (no expansion)
            arg.value = self.visit(arg.value)
            arg_index += 1

    # Check if some argument names are reused after the expansion
    seen = set()
    for arg in expr.items:
        if arg.name:
            if arg.name in seen:
                raise TypecheckError(expr, f"keyword argument repeated: {arg.name}")
            seen.add(arg.name)
    return True


def get_callee_fn(self: TypeVisitor, expr: ast.CallExpr, part):
    """
    Extract the @c FuncType that represents the function to be called by the callee.
    Also handle special callees: constructors and partial functions.
    @return a pair with the callee's @c FuncType and the replacement expression
    (when needed; otherwise nullptr).
    """

    callee = expr.expr.get_class_type()
    if callee is None:
        # Case: unknown callee, wait until it becomes known
        return None, None

    extracted = utils.extract_type(self.ctx, expr.expr)
    callee_fn = callee.get_func()
    if expr.has(ast.Attr.TupleCall) and (
        extracted.is_type(ast.types.Stdlib.Tuple)
        or (
            callee_fn
            and isinstance(callee_fn.ast, ast.FunctionStmt)
            and callee_fn.ast.name.startswith("std.internal.static.tuple.")
        )
    ):
        return None, None

    if utils.is_type_expr(expr.expr):
        class_type = expr.expr.get_class_type()
        if not (isinstance(expr.expr, ast.IdExpr) and expr.expr.value == ast.types.Stdlib.Type):
            class_type = class_type[0].get_class()
        if class_type is None:
            return None, None

        if class_type.is_record():
            if expr.has(ast.Attr.TupleCall):
                expr.erase(ast.Attr.TupleCall)
            # Case: tuple constructor. Transform to: `T.__new__(args)`
            replacement = ast.CallExpr(ast.DotExpr(expr.expr, member="__new__"), items=expr.items)
            return None, self.visit(replacement)

        # Case: reference type constructor. Transform to
        # `ctr = T.__new__(); v.__init__(args)`
        var = ast.IdExpr(utils.get_temporary_var(self.ctx, "ctr"))
        new_init = ast.AssignStmt(
            var.clone(), ast.CallExpr(ast.DotExpr(expr.expr, member="__new__"))
        )
        result = ast.StmtExpr([new_init], expr=var.clone())
        result.items.append(
            ast.ExprStmt(
                ast.CallExpr(ast.DotExpr(var.clone(), member="__init__"), items=expr.items)
            )
        )
        return None, self.visit(result)

    if partial_type := callee.get_partial():
        mask = partial_type.get_partial_mask()
        partial_function = partial_type.get_partial_func()
        generalized = partial_function.generalize(0)
        instantiated = utils.instantiate_type(self.ctx, generalized).get_func()
        if not partial_type.is_partial_empty() or any(
            flag != ast.types.Class.Flag.Missing for flag in mask
        ):
            # Case: calling partial object `p`. Transform roughly to
            # `part = callee; partial_fn(*part.args, args...)`
            part.var = utils.get_temporary_var(self.ctx, "partcall")
            expr.expr = self.visit(
                ast.StmtExpr(
                    [ast.AssignStmt(ast.IdExpr(part.var), rhs=expr.expr)],
                    expr=ast.IdExpr(instantiated.get_func_name(), type=instantiated),
                )
            )
            part.known = mask
        else:
            expr.expr = self.visit(ast.IdExpr(instantiated.get_func_name()))
        assert expr.expr.type.get_func() is not None, f"not a function: {expr.expr.type}"
        infer.unify(expr.expr.type, instantiated)

        # Unify partial generics with types known thus far
        known_argument_types = _extract_class_generic(partial_type, 1).get_class()
        generic_idx = 0
        known_idx = 0
        for param_idx, flag in enumerate(mask):
            if instantiated.ast.items[param_idx].is_generic():
                generic_idx += 1
            elif flag == ast.types.Class.Flag.Included:
                infer.unify(
                    instantiated.get_func()[param_idx - generic_idx],
                    known_argument_types[known_idx],
                )
                known_idx += 1
            elif flag == ast.types.Class.Flag.Default:
                known_idx += 1
        return instantiated, None

    if callee_fn is None:
        # Case: callee is not a function. Try __call__ method instead
        result = ast.CallExpr(ast.DotExpr(expr.expr, member="__call__"), items=expr.items)
        return None, self.visit(result)

    return callee_fn, None


def call_reorder_arguments(
    self: TypeVisitor, callee_fn: ast.types.Function, expr: ast.CallExpr, part
) -> ast.Expr | None:
    """
    Reorder the call arguments to match the signature order. Ensure that every @c
    CallArg has a set name. Form *args/**kwargs tuples if needed, and use partial
    and default values where needed.
    @example
    `foo(1, 2, baz=3, baf=4)` -> `foo(a=1, baz=2, args=(3, ), kwargs=KwArgs(baf=4))`
    """

    if callee_fn.ast.has(ast.Attr.NoArgReorder):
        return None

    in_order = True
    ordered = []
    args = []  # stores ordered and processed arguments
    type_args = []  # stores type and static arguments (e.g., `T: type`)
    star_idx = -1  # for *args
    star_args = []
    kwstar_idx = -1  # for **kwargs
    kwstar_names = []
    kwstar_args = []
    new_mask = [ast.types.Class.Flag.Included] * len(callee_fn.ast.items)
    partial = False

    def get_partial_argument(partial_index: int) -> ast.Expr:
        """Extract pi-th partial argument from a partial object"""
        args_expr = self.visit(ast.DotExpr(ast.IdExpr(part.var), member="args"))
        # Manually call @c transformStaticTupleIndex to avoid spurious InstantiateExpr
        found, expr = ops.transform_static_tuple_index(
            self, args_expr.get_class_type(), args_expr, ast.IntExpr(partial_index)
        )
        assert found and expr is not None, f"partial indexing failed: {args_expr.type}"
        return expr

    def add_reordered(arg_idx: int):
        nonlocal in_order
        argument_expr = expr.items[arg_idx].value
        if utils.has_side_effect(argument_expr):
            if ordered and arg_idx < ordered[-1]:
                in_order = False
            ordered.append(arg_idx)
            return True
        return False

    # done_score = on_done(star_idx, keyword_star_idx, slots, partial)
    # return score + done_score if done_score != -1 else -1

    part.args = None
    # Reorder arguments if needed
    # Stores partial *args/**kwargs expression
    part.kw_args = None
    if expr.has(ast.Attr.ExprOrderedCall):
        args = expr.items
    else:
        _, (star_pos, kwstar_pos, slots, partial) = utils.reorder_named_args(
            self.ctx, callee_fn, expr.items, part.known
        )
        partial_idx, generic_idx = 0, 0
        for slot_idx, slot in enumerate(slots):
            # Get the argument name to be used later
            param = callee_fn.ast.items[slot_idx]
            _, raw_name = param.get_name_with_stars()
            real_name = utils.get_unmangled_name(self.ctx, raw_name)
            if param.is_generic():
                # Case: generic arguments. Populate typeArgs
                if real_name.startswith("$"):
                    if not slot:
                        if part.known and part.known[slot_idx] == ast.types.Class.Flag.Included:
                            type_args.append(
                                ast.IdExpr(
                                    real_name, type=callee_fn.func_generics[generic_idx].type
                                )
                            )
                        else:
                            type_args.append(self.visit(ast.IdExpr(real_name[1:])))
                    else:
                        type_args.append(expr.items[slot[0]].value)
                        if add_reordered(slot[0]):
                            # type arguments always need preprocessing
                            in_order = False
                    new_mask[slot_idx] = ast.types.Class.Flag.Included
                elif not slot:
                    type_args.append(None)
                    new_mask[slot_idx] = ast.types.Class.Flag.Missing
                else:
                    type_args.append(expr.items[slot[0]].value)
                    new_mask[slot_idx] = ast.types.Class.Flag.Included
                    if add_reordered(slot[0]):
                        # type arguments always need preprocessing
                        in_order = False
                generic_idx += 1
            elif slot_idx == star_pos and not (
                len(slot) == 1
                and expr.items[slot[0]].value
                and expr.items[slot[0]].value.has(ast.Attr.ExprStarArgument)
            ):
                # Case: *args. Build the tuple that holds them all
                if part.known:
                    star_args.append(ast.StarExpr(get_partial_argument(-1)))
                for source_idx in slot:
                    source = expr.items[source_idx].value
                    star_args.append(source)
                    add_reordered(source_idx)
                star_idx = len(args)
                args.append(ast.CallExpr.Arg(name=real_name))
                if partial:
                    new_mask[slot_idx] = ast.types.Class.Flag.Missing
            elif slot_idx == kwstar_pos and not (
                len(slot) == 1
                and expr.items[slot[0]].value
                and expr.items[slot[0]].value.has(ast.Attr.ExprKwStarArgument)
            ):
                # Case: **kwargs. Build the named tuple that holds them all
                new_names = {expr.items[source_index].name for source_index in slot}
                if part.known:
                    kwargs_expr = self.visit(ast.DotExpr(ast.IdExpr(part.var), member="kwargs"))
                    names, named_types = utils.extract_named_tuple(self.ctx, kwargs_expr)
                    for name, named_type in zip(names, named_types):
                        if name not in new_names:
                            new_names.add(name)
                            kwstar_names.append(name)
                            kwstar_args.append(self.visit(ast.NoneExpr(type=named_type)))
                # kwargs names can be overriden later
                for source_idx in slot:
                    source = expr.items[source_idx].value
                    kwstar_names.append(expr.items[source_idx].name)
                    kwstar_args.append(source)
                    add_reordered(source_idx)
                kwstar_idx = len(args)
                args.append(ast.CallExpr.Arg(name=real_name))
                if partial:
                    new_mask[slot_idx] = ast.types.Class.Flag.Missing
            elif not slot:
                # Case: no arguments provided.
                if part.known and part.known[slot_idx] == ast.types.Class.Flag.Included:
                    # Case 1: Argument captured by partial
                    args.append(ast.CallExpr.Arg(get_partial_argument(partial_idx), name=real_name))
                    partial_idx += 1
                elif real_name.startswith("$"):
                    # Case 3: Local name capture
                    added = False
                    if partial:
                        value = self.ctx.find(real_name[1:])
                        if (
                            value
                            and value.is_func()
                            and value.type.get_func()
                            and value.type.get_func().ast.name == callee_fn.ast.name
                        ):
                            # Special case: fn(fn=fn)
                            # Delay this one.
                            ellipsis = self.visit(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial))
                            args.append(ast.CallExpr.Arg(ellipsis, name=real_name))
                            new_mask[slot_idx] = ast.types.Class.Flag.Missing
                            added = True
                    if not added:
                        args.append(
                            ast.CallExpr.Arg(self.visit(ast.IdExpr(real_name[1:]), name=real_name))
                        )
                elif param.default:
                    default = param.default
                    # Case 4: default is present
                    if isinstance(default, ast.IdExpr):
                        # Case 4a: non-values (Ids / .default names)
                        if part.known and part.known[slot_idx] == ast.types.Class.Flag.Default:
                            # Case 4a/1: Default already captured by partial.
                            args.append(
                                ast.CallExpr.Arg(get_partial_argument(partial_idx), name=real_name)
                            )
                            partial_idx += 1
                        else:
                            # TODO: check if the value is toplevel to avoid capturing it
                            transformed = self.visit(ast.IdExpr(default.value))
                            assert transformed.type.get_link() is not None, "not a link type"
                            args.append(ast.CallExpr.Arg(transformed, name=real_name))
                        if partial:
                            new_mask[slot_idx] = ast.types.Class.Flag.Default
                    elif not partial:
                        # Case 4b: values / non-Id defaults (None, etc.)
                        if isinstance(default, ast.NoneExpr) and param.type is None:
                            transformed = ast.CallExpr(
                                ast.InstantiateExpr(
                                    ast.IdExpr(ast.types.Stdlib.Optional),
                                    type_expr=ast.IdExpr(ast.types.Stdlib.NoneType),
                                )
                            )
                        else:
                            transformed = default.clone()
                        args.append(ast.CallExpr.Arg(self.visit(transformed), name=real_name))
                    else:
                        ellipsis = self.visit(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial))
                        args.append(ast.CallExpr.Arg(ellipsis, name=real_name))
                        new_mask[slot_idx] = ast.types.Class.Flag.Missing
                elif partial:
                    # Case 5: this is partial call. Just add ... for missing arguments
                    ellipsis = self.visit(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial))
                    args.append(ast.CallExpr.Arg(ellipsis, name=real_name))
                    new_mask[slot_idx] = ast.types.Class.Flag.Missing
                else:
                    # Case: argument provided
                    assert False, "call transformation failed"
            else:
                assert len(slot) == 1
                source = expr.items[slot[0]].value
                args.append(ast.CallExpr.Arg(source, name=real_name))
                add_reordered(slot[0])

    # Do reordering
    if not in_order:
        prepends = []
        for source_idx in sorted(ordered):
            old = expr.items[source_idx].value
            name = utils.get_temporary_var(self.ctx, "call")
            front = self.visit(
                ast.AssignStmt(ast.IdExpr(name), rhs=old, type_expr=utils.get_param_type(old.type))
            )
            swap = self.visit(ast.IdExpr(name))
            expr.items[source_idx].value = swap
            for arg in args:
                if arg.value is old:
                    arg.value = swap
            type_args = [swap if item is old else item for item in type_args]
            star_args = [swap if item is old else item for item in star_args]
            kwstar_args = [swap if item is old else item for item in kwstar_args]
            prepends.append(front)
        return self.visit(ast.StmtExpr(prepends, expr=expr))

    # Handle *args
    if star_idx != -1:
        star_expr = ast.TupleExpr(star_args)
        star_expr.set(ast.Attr.ExprStarArgument)
        if not (isinstance(expr.expr, ast.IdExpr) and expr.expr.value == "hasattr"):
            transformed = self.visit(star_expr)
            star_expr = transformed
        if partial:
            part.args = star_expr
            args[star_idx].value = self.visit(ast.EllipsisExpr(ast.EllipsisExpr.PARTIAL))
        else:
            args[star_idx].value = star_expr

    # Handle **kwargs
    if kwstar_idx != -1:
        keyword_id = classes.generate_kw_id(self, kwstar_names)
        kwstar_expr = self.visit(
            ast.CallExpr(
                ast.IdExpr(ast.types.Stdlib.NamedTuple),
                items=[ast.TupleExpr(kwstar_args), ast.IntExpr(keyword_id)],
            )
        )
        kwstar_expr.set(ast.Attr.ExprKwStarArgument)
        if partial:
            part.kw_args = kwstar_expr
            args[kwstar_idx].value = self.visit(ast.EllipsisExpr(ast.EllipsisExpr.Kind.Partial))
        else:
            args[kwstar_idx].value = kwstar_expr

    # Populate partial data
    if part.args:
        part.args.set(ast.Attr.ExprSequenceItem)
    if part.kw_args:
        part.kw_args.set(ast.Attr.ExprSequenceItem)
    if part.is_partial:
        expr.items.pop()
        if part.args is None:
            # use ()
            part.args = self.visit(ast.TupleExpr())
        if part.kw_args is None:
            # use NamedTuple()
            part.kw_args = self.visit(ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NamedTuple)))

    # Unify function type generics with the provided generics
    assert (expr.has(ast.Attr.ExprOrderedCall) and not type_args) or (
        not expr.has(ast.Attr.ExprOrderedCall) and len(type_args) == len(callee_fn.func_generics)
    )
    if callee_fn.func_generics:
        non_inferrable = callee_fn.ast.get_non_inferrable_generics()
        for generic_idx, generic in enumerate(callee_fn.func_generics):
            if expr.has(ast.Attr.ExprOrderedCall):
                break
            type_arg = type_args[generic_idx]
            if type_arg:
                argument_type = utils.extract_type(self.ctx, type_arg)
                if (
                    generic.static_kind is not ast.types.Type.Behaviour.Runtime
                    and argument_type.get_static_kind() is ast.types.Type.Behaviour.Runtime
                ):
                    raise TypecheckError(expr, "expected static expression")
                infer.unify(argument_type, generic.type)
            elif (
                utils.is_unbound(generic.type)
                and callee_fn.ast.items[generic_idx].default_value is None
                and not partial
                and generic.name in non_inferrable
            ):
                raise TypecheckError(
                    expr,
                    f"'{utils.get_unmangled_name(self.ctx, generic.name)}' not provided",
                )

    expr.items = args
    expr.set(ast.Attr.ExprOrderedCall)
    part.known = new_mask
    return None


def typecheck_call_args(
    self: TypeVisitor,
    callee_fn: ast.types.Function,
    args: List[ast.CallExpr.Arg],
    partial,
) -> bool:
    """
    Unify the call arguments' types with the function declaration signatures.
    Also apply argument transformations to ensure the type compatibility and handle
    default generics.
    @example
    `foo(1, 2)` -> `foo(1, Optional(2), T=int)`
    """

    wrapping_done = True  # tracks whether all arguments are wrapped
    replacements = []  # list of replacement arguments
    with utils.with_class_generics(self.ctx, callee_fn, check_arguments, True):
        signature_idx = 0
        for _, param in enumerate(callee_fn.ast.items):
            if param.is_generic():
                continue
            arg = args[signature_idx]
            if param.name.startswith("*") and param.type:
                # Special case: `*args: type` and `**kwargs: type`
                if call_expr := cast(ast.CallExpr, arg.value):
                    type_expression = self.visit(param.type.clone())
                    expected_type = utils.extract_type(self.ctx, type_expression)
                    if param.name.startswith("**"):
                        call_expr = call_expr.items[0].value
                    for call_arg in call_expr.items:
                        can_wrap, call_arg.value = utils.wrap_expr(
                            self, call_arg.value, expected_type, callee_fn
                        )
                        if can_wrap:
                            infer.unify(call_arg.value.type, expected_type)
                        else:
                            wrapping_done = False
                    call_type = call_expr.get_class_type()
                    tuple_expr = self.visit(
                        ast.CallExpr(ast.IdExpr(call_type.name), items=call_expr.items)
                    )
                    if param.name.startswith("**"):
                        tuple_id = arg.value.type[0].get_int_static()
                        arg.value = self.visit(
                            ast.CallExpr(
                                ast.IdExpr(ast.types.mangle(cls="NamedTuple", func="__new__")),
                                items=[tuple_expr, ast.IntExpr(tuple_id.value)],
                            )
                        )
                    else:
                        arg.value = tuple_expr
                replacements.append(arg.value.type)
                # else this is empty and is a partial call; leave it for later
            elif (
                partial.is_partial
                and partial.known
                and partial.known[signature_idx] == ast.types.Class.Flag.Default
            ):
                # Defaults should not be unified (yet)!
                replacements.append(callee_fn[signature_idx])
            else:
                expected_type = callee_fn[signature_idx]
                can_wrap, arg.value = utils.wrap_expr(self, arg.value, expected_type, callee_fn)
                if can_wrap:
                    infer.unify(arg.value.type, expected_type)
                else:
                    wrapping_done = False
                replacements.append(
                    arg.value.type if expected_type.get_class() is None else expected_type
                )
            signature_idx += 1
        return True

    # Realize arguments
    done = True
    for arg in args:
        # Previous unifications can qualify existing identifiers.
        # Transform again to get the full identifier
        if infer.realize(arg.value.type):
            arg.value = self.visit(arg.value)
        done = done and arg.value.done

    # Handle default generics
    if not partial.is_partial:
        generic_idx = 0
        for param in callee_fn.ast.items:
            if not wrapping_done:
                break
            if param.is_generic():
                generic = utils.extract_func_generic(callee_fn, generic_idx)
                if param.default and utils.is_unbound(generic):
                    with utils.with_class_generics(self.ctx, callee_fn, True):
                        default = self.visit(param.default.clone())
                    infer.unify(generic, utils.extract_type(self.ctx, default))
                generic_idx += 1

    # Replace the arguments
    callee_type = callee_fn[0].get_class()
    for idx, replacement in enumerate(replacements):
        callee_type.generics[idx].type = replacement
    callee_type._rn = ""
    callee_fn._rn = ""  # TODO: TERRIBLE!
    return done


def transform_special_call(self: TypeVisitor, expr: ast.CallExpr):
    """
    Transform and typecheck the following special call expressions:
    `superf(fn)`
    `super()`
    `__ptr__(var)`
    `__array__[int](sz)`
    `isinstance(obj, type)`
    `static.len(tup)`
    `hasattr(obj, "attr")`
    `getattr(obj, "attr")`
    `type(obj)`
    `compile_err("msg")`
    See below for more details.
    """

    if expr.has(ast.Attr.ExprNoSpecial):
        return False, None

    identifier = expr.expr if isinstance(expr.expr, ast.IdExpr) else None
    if not identifier:
        return False, None

    name = identifier.value
    if name == ast.types.mangle(func="superf"):
        return True, special.transform_super_f(self, expr)
    if name == ast.types.mangle(func="super") and not expr.items:
        return True, special.transform_super(self)
    if name == ast.types.mangle(func="__ptr__"):
        return True, special.transform_ptr(self, expr)
    if name == ast.types.mangle(cls="__array__", func="__new__"):
        return True, special.transform_array(self, expr)
    if name == ast.types.mangle(func="isinstance"):
        return True, special.transform_is_instance(self, expr)
    if name == ast.types.mangle("std.internal.static", func="len"):
        return True, special.transform_static_len(self, expr)
    if name == ast.types.mangle(func="hasattr"):
        return True, special.transform_has_attr(self, expr)
    if name == ast.types.mangle(func="hasattr_dynamic"):
        return True, special.transform_has_attr(self, expr, True)
    if name == ast.types.mangle(func="_getattr"):
        return True, special.transform_get_attr(self, expr)
    if name == ast.types.mangle(func="setattr"):
        return True, special.transform_set_attr(self, expr)
    if name == ast.types.mangle(cls="type", func="__new__"):
        return True, special.transform_type_fn(self, expr)
    if name == ast.types.mangle(func="compile_error"):
        return True, special.transform_compile_error(self, expr)
    if name == ast.types.mangle("std.internal.static", func="print"):
        return False, special.transform_static_print_fn(self, expr)
    if name == ast.types.mangle("std.collections", func="namedtuple"):
        return True, special.transform_named_tuple(self, expr)
    if name == ast.types.mangle("std.functools", func="partial"):
        return True, special.transform_functools_partial(self, expr)
    if name == ast.types.mangle("std.internal.static", func="has_rtti"):
        return True, special.transform_has_rtti_fn(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="realized"):
        return True, special.transform_realized_fn(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="can_call"):
        return True, special.transform_static_fn_can_call(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="has_type"):
        return True, special.transform_static_fn_arg_has_type(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="get_type"):
        return True, special.transform_static_fn_arg_get_type(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="args"):
        return True, special.transform_static_fn_args(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="has_default"):
        return True, special.transform_static_fn_has_default(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="get_default"):
        return True, special.transform_static_fn_get_default(self, expr)
    if name == ast.types.mangle("std.internal.static", cls="function", func="wrap_args"):
        return True, special.transform_static_fn_wrap_call_args(self, expr)
    if name == ast.types.mangle("std.internal.static", func="vars"):
        return True, special.transform_static_vars(self, expr)
    if name == ast.types.mangle("std.internal.static", func="children"):
        return True, special.transform_static_children(self, expr)
    if name == ast.types.mangle("std.internal.static", func="tuple_type"):
        return True, special.transform_static_tuple_type(self, expr)
    if name == ast.types.mangle("std.internal.static", func="format"):
        return True, special.transform_static_format(self, expr)
    if name == ast.types.mangle("std.internal.static", func="int_to_string"):
        return True, special.transform_static_int_to_str(self, expr)
    return False, None


def get_mro(self: TypeVisitor, class_type: ast.types.Class | None) -> List[ast.types.Type]:
    """
    Get the list that describes the inheritance hierarchy of a given type.
    The first type in the list is the most recently inherited type.
    """

    result = []
    if not class_type:
        return result
    cls_data = utils.get_class(self.ctx, class_type)
    for uninstantiated in cls_data.mro:
        instantiated = utils.instantiate_type(self.ctx, uninstantiated, class_type)
        # ensure that parent types are realized
        infer.realize(instantiated)
        result.append(instantiated)
    return result


def generate_partial_call(
    self: TypeVisitor,
    mask: str,
    function_type: ast.types.Function,
    args: ast.Expr | None = None,
    kwargs: ast.Expr | None = None,
) -> ast.Expr:
    """
    Return a partial type call `Partial(args, kwargs, fn, mask)` for a given function
    and a mask.
    @param mask a 0-1 vector whose size matches the number of function arguments.
    1 indicates that the argument has been provided and is cached within
    the partial object.
    """

    if args is None:
        args = ast.TupleExpr([ast.TupleExpr()])
    if kwargs is None:
        kwargs = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.NamedTuple))
    return ast.CallExpr(
        ast.IdExpr("Partial"),
        items=[
            ast.CallExpr.Arg(args, name="args"),
            ast.CallExpr.Arg(kwargs, name="kwargs"),
            ast.CallExpr.Arg(ast.StringExpr(mask), name="M"),
            ast.CallExpr.Arg(
                name="F",
                value=ast.IdExpr(
                    function_type.get_func_name(),
                    type=utils.instantiate_type(
                        self.ctx,
                        utils.get_stdlib_type(self.ctx, ast.types.Stdlib.UnrealizedType),
                        [function_type.get_func()],
                    ),
                    done=True,
                ),
            ),
        ],
    )
