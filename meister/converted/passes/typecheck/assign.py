# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy
from typing import TYPE_CHECKING

from ....bridge import List, cast
from ... import ast, cache
from ..scope import Bindings
from . import infer, ops, utils
from .ctx import Item, TypecheckError

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_assignexpr(self: TypeVisitor, node: ast.AssignExpr) -> ast.Node:
    """
    Transform walrus (assignment) expression.
    @example
    `(expr := var)` -> `var = expr; var`
    """
    assignment = ast.AssignStmt(node.var.clone(), rhs=node.expr)
    assignment.attributes = copy.deepcopy(node.attributes)
    transformed = self.visit(ast.StmtExpr(assignment, expr=node.var))
    return transformed


def typecheck_assign(self: TypeVisitor, node: ast.AssignStmt) -> ast.Node:
    """
    Transform assignments. Handle dominated assignments, forward declarations, static
    assignments and type/function aliases.
    See @c transformAssignment and @c unpackAssignments for more details.
    See @c wrapExpr for more examples.
    """

    if isinstance(node.lhs, (ast.TupleExpr, ast.ListExpr)):
        unpacked = unpack_assignment(self, node.lhs, node.rhs)
        return self.visit(unpacked)

    must_update = node.is_update() or node.is_atomic_update()
    must_update = must_update or node.lhs.has(ast.Attr.ExprDominated)
    must_update = must_update or node.lhs.has(ast.Attr.ExprDominatedUsed)
    if isinstance(node.rhs, ast.BinaryExpr) and node.rhs.in_place:
        # Update case: a += b
        assert node.type_expr is None, f"invalid AssignStmt {node.to_string()}"
        must_update = True

    result = transform_assignment(self, node, must_update)
    if node.lhs.has(ast.Attr.ExprDominatedUsed):
        # If this is dominated, set __used__ if needed
        node.lhs.erase(ast.Attr.ExprDominatedUsed)
        assert isinstance(node.lhs, ast.IdExpr), "dominated bad assignment"
        used_assignment = ast.AssignStmt(
            ast.IdExpr(
                f"{utils.get_unmangled_name(self.ctx, node.lhs.value)}{cache.VAR_USED_SUFFIX}"
            ),
            rhs=ast.BoolExpr(True),
            update=ast.AssignStmt.Mode.Update,
        )
        result = self.visit(ast.SuiteStmt(result, used_assignment))
    return result


def typecheck_del(self: TypeVisitor, node: ast.DelStmt) -> ast.Node:
    """
    Transform deletions.
    @example
    `del a`    -> `a = type(a)()` and remove `a` from the context
    `del a[x]` -> `a.__delitem__(x)`
    """

    match node.expr:
        case ast.IndexExpr(expr=expr, index=index):
            call = ast.CallExpr(ast.DotExpr(expr, member="__delitem__"), items=[index])
            return self.visit(ast.ExprStmt(call))
        case ast.IdExpr(value=name):
            # Assign `a` to `type(a)()` to mark it for deletion
            type_call = ast.CallExpr(
                ast.CallExpr(ast.IdExpr(ast.types.Stdlib.Type), items=[node.expr.clone()])
            )
            assignment = ast.AssignStmt(node.expr, rhs=type_call, update=ast.AssignStmt.Mode.Update)
            result = self.visit(assignment)

            # Allow deletion *only* if the binding is dominated
            value = self.ctx.find(name)
            if not value:
                raise TypecheckError(node.expr, f"name '{name}' is not defined")
            # TODO: check if variable can be deleted (e.g., can you delete a variable in
            # outside scope?!)
            self.ctx.remove(name)
            self.ctx.remove(utils.get_unmangled_name(self.ctx, name))
            return result
        case _:
            raise TypecheckError(node, "cannot delete given expression")


def unpack_assignment(self: TypeVisitor, lhs: ast.Expr, rhs: ast.Expr) -> ast.Stmt:
    """
    Unpack an assignment expression `lhs = rhs` into a list of simple assignment
    expressions (e.g., `a = b`, `a.x = b`, or `a[x] = b`).
    Handle Python unpacking rules.
    @example
    `(a, b) = c`     -> `a = c[0]; b = c[1]`
    `a, b = c`       -> `a = c[0]; b = c[1]`
    `[a, *x, b] = c` -> `a = c[0]; x = c[1:-1]; b = c[-1]`.
    Non-trivial right-hand expressions are first stored in a temporary variable.
    @example
    `a, b = c, d + foo()` -> `assign = (c, d + foo); a = assign[0]; b = assign[1]`.
    Each assignment is unpacked recursively to allow cases like `a, (b, c) = d`.
    """

    # Case: (a, b) = ... or [a, b] = ...
    left_side: List[ast.Expr] = []
    if isinstance(lhs, (ast.TupleExpr, ast.ListExpr)):
        left_side = list(lhs.items)
    else:
        return ast.AssignStmt(lhs, rhs=rhs)

    old_info = self.ctx.node_stack[-1].info
    self.set_info(rhs.info)
    try:
        # Prepare the right-side expression
        block = ast.SuiteStmt()
        if not isinstance(rhs, ast.IdExpr):
            # Store any non-trivial right-side expression into a variable
            var = utils.get_temporary_var(self.ctx, "assign")
            new_rhs = ast.IdExpr(var)
            block.items.append(ast.AssignStmt(new_rhs, rhs=rhs.clone()))
            rhs = new_rhs

        # Process assignments until the fist StarExpr (if any)
        star_idx = 0
        while star_idx < len(left_side):
            if isinstance(left_side[star_idx], ast.StarExpr):
                break
            # Transformation: `leftSide_st = rhs[st]` where `st` is static integer
            # Recursively process the assignment because of cases like `(a, (b, c)) = d)`
            block.items.append(
                unpack_assignment(
                    self,
                    left_side[star_idx],
                    ast.IndexExpr(rhs.clone(), index=ast.IntExpr(star_idx)),
                )
            )
            star_idx += 1
        # Process StarExpr (if any) and the assignments that follow it
        if star_idx < len(left_side) and isinstance(left_side[star_idx], ast.StarExpr):
            # StarExpr becomes SliceExpr (e.g., `b` in `(a, *b, c) = d` becomes
            # `list(d[1:-2])`)
            stop = (
                None
                if len(left_side) == star_idx + 1
                else ast.IntExpr(-len(left_side) + star_idx + 1)
            )
            right_side = ast.CallExpr(
                ast.IdExpr(
                    ast.types.mangle("std.internal.types.array", cls="List", func="as_list")
                ),
                items=[
                    ast.IndexExpr(
                        # this slice is either [st:] or [st:-lhs_len + st + 1]
                        rhs.clone(),
                        index=ast.SliceExpr(ast.IntExpr(star_idx), stop=stop),
                    )
                ],
            )
            star = left_side[star_idx]
            block.items.append(unpack_assignment(self, star.expr, right_side))
            star_idx += 1
            # Process remaining assignments. They will use negative indices (-1, -2 etc.)
            # because we do not know how big is StarExpr
            while star_idx < len(left_side):
                if isinstance(left_side[star_idx], ast.StarExpr):
                    raise TypecheckError(lhs, "multiple starred expressions in assignment")
                right_side = ast.IndexExpr(
                    rhs.clone(), index=ast.IntExpr(-(len(left_side) - star_idx))
                )
                block.items.append(unpack_assignment(self, left_side[star_idx], right_side))
                star_idx += 1
    finally:
        self.set_info(old_info)
    return block


def transform_assignment(
    self: TypeVisitor, stmt: ast.AssignStmt, must_exist: bool = False
) -> ast.Stmt:
    """
    Transform simple assignments.
    @example
    `a[x] = b`    -> `a.__setitem__(x, b)`
    `a.x = b`     -> @c AssignMemberStmt
    `a: type` = b -> @c AssignStmt
    `a = b`       -> @c AssignStmt or @c UpdateStmt (see below)
    """

    var: ast.IdExpr | None = None
    match stmt.lhs:
        case ast.IndexExpr(expr=expr, index=index):  # a[x] = b
            assert stmt.type_expr is None, "unexpected type annotation"
            if (
                isinstance(stmt.rhs, ast.BinaryExpr)
                and must_exist
                and stmt.rhs.in_place
                and not isinstance(stmt.rhs.rexpr, ast.IdExpr)
            ):
                # Case: a[x] += b (inplace operator)
                name = utils.get_temporary_var(self.ctx, "assign")
                result = ast.SuiteStmt(
                    ast.AssignStmt(ast.IdExpr(name), rhs=index),
                    ast.ExprStmt(
                        ast.CallExpr(
                            ast.DotExpr(expr, member="__setitem__"),
                            items=[
                                ast.IdExpr(name),
                                ast.BinaryExpr(
                                    ast.IndexExpr(expr.clone(), index=ast.IdExpr(name)),
                                    op=stmt.rhs.op,
                                    rexpr=stmt.rhs.rexpr,
                                    in_place=True,
                                ),
                            ],
                        )
                    ),
                )
            else:
                result = self.visit(
                    ast.ExprStmt(
                        ast.CallExpr(
                            ast.DotExpr(index.expr, member="__setitem__"),
                            items=[index.index, stmt.rhs],
                        )
                    )
                )
            return self.visit(result)
        case ast.DotExpr(expr=expr, member=member):  # a.x = b
            expr = self.visit(expr, type_allowed=True)
            rhs = self.visit(stmt.rhs)
            transformed = ast.AssignMemberStmt(
                expr, member=member, rhs=rhs, type_expr=stmt.type_expr
            )
            return self.visit(transformed)
        case ast.IdExpr():  # a (: T) = b
            var = stmt.lhs
            # Never do undef checks on assignments!
            var.set(ast.Attr.ExprNoUndefCheck)
            # continues below...
        case _:
            raise TypecheckError(stmt, "cannot assign to given expression")

    # Ensure that captured values are in a Capsule
    if self.ctx.in_function() and stmt.rhs and not must_exist:
        base = self.ctx.get_base()
        if bindings := base.func.attributes.get(ast.Attr.Bindings):
            if (binding := bindings.bindings.get(var.value)) and binding.is_nonlocal:
                stmt.type_expr = (
                    ast.IndexExpr(ast.IdExpr(ast.types.Stdlib.Capsule), index=stmt.type_expr)
                    if stmt.type_expr
                    else ast.IdExpr(ast.types.Stdlib.Capsule)
                )

    is_thread_local = False
    type_expr = self.visit(stmt.type_expr, enforce_type=True)
    if type_expr and utils.extract_type(self.ctx, type_expr).is_type(ast.types.Stdlib.ThreadLocal):
        is_thread_local = True
        if isinstance(type_expr, ast.IndexExpr):
            type_expr = self.visit(stmt.type_expr.index, enforce_type=True)
        else:
            type_expr = None

    # Make sure that existing values that cannot be shadowed are only updated
    # mustExist |= val && !ctx->isOuter(val);
    if must_exist:
        value = self.ctx.find(var.value, self.ctx.time)
        if not value:
            raise TypecheckError(
                var,
                f"local variable '{var.value}' referenced before assignment at {var.info}",
            )
        update = ast.AssignStmt(stmt.lhs, rhs=stmt.rhs, type_expr=type_expr)
        base = self.ctx.get_base()
        if not base.is_type() and base.func and base.func.has(ast.Attr.Atomic):
            update.set_atomic_update()
        else:
            update.set_update()
        return transform_update(self, update) or update  # delay on fail

    # Generate new canonical variable name for this assignment and add it to the context
    stmt.rhs = self.visit(stmt.rhs, type_allowed=True)
    stmt.type_expr = type_expr
    if var.value.endswith(cache.VAR_USED_SUFFIX):
        found = self.ctx.force_find(var.value.removesuffix(cache.VAR_USED_SUFFIX))
        canonical = f"{found.canonical_name}{cache.VAR_USED_SUFFIX}"
    else:
        canonical = self.ctx.generate_canonical_name(var.value)
    lhs = ast.IdExpr(canonical)
    lhs.attributes.update(var.attributes)
    assignment = ast.AssignStmt(lhs, rhs=stmt.rhs, type_expr=stmt.type_expr)
    lhs.type = var.type or utils.instantiate_unbound(self.ctx, lhs.info)
    if is_thread_local:
        assignment.set_thread_local()

    base = self.ctx.get_base()
    if (
        assignment.rhs is None
        and assignment.type_expr is None
        and self.ctx.find(ast.types.Stdlib.NoneType)
    ):
        # All declarations that are not handled are to be marked with NoneType later on
        # (useful for dangling declarations that are not initialized afterwards due to static check)
        link = lhs.type.get_link()
        link.default_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.NoneType)
        base.pending_defaults.setdefault(1, set()).add(lhs.type)
    if assignment.type_expr:
        annotated_type = utils.extract_type(self.ctx, assignment.type_expr)
        infer.unify(
            lhs.type,
            utils.instantiate_type(self.ctx, annotated_type, info=assignment.type_expr.info),
        )
    value = Item(
        canonical_name=canonical,
        base_name=self.ctx.get_base_name(),
        module_name=self.ctx.get_module(),
        type=lhs.type,
        block_level=self.ctx.block_level,
        time=self.ctx.time,
        info=self.ctx.node_stack[-1].info,
    )
    self.ctx.add(var.value, value)
    self.ctx.add_always_visible(value)

    if assignment.rhs:  # not a declaration
        # Check if we can wrap the expression (e.g., `a: float = 3` -> `a = float(3)`)
        can_wrap, assignment.rhs = utils.wrap_expr(self, assignment.rhs, lhs.type)
        if can_wrap:
            infer.unify(lhs.type, assignment.rhs.type)

        # Generalize non-variable types. That way we can support cases like:
        # `a = foo(x, ...); a(1); a('s')`
        if not value.is_var():
            value.type = value.type.generalize(self.ctx.typecheck_level - 1)
            # Fix capture_function_partial_proper_realize test
            lhs.type = assignment.rhs.type = value.type

    # Mark declarations or generalized type/functions as done
    if (assignment.rhs is None or assignment.rhs.done) and lhs.type.can_realize():
        realized = infer.realize(lhs.type)
        if realized:
            # overwrite types to remove dangling unbounds with some partials...
            lhs.type = realized
            if assignment.rhs:
                assignment.rhs.type = realized
            assignment.done = True
    elif assignment.rhs and not value.is_var() and not value.type.has_unbounds(False):
        assignment.done = True

    # Register all toplevel variables as global in JIT mode
    # or if they are in imported module (not toplevel)
    is_global = (
        self.ctx.cache.is_jit
        and value.is_global()
        and (not value.is_generic())
        or canonical == ast.types.Stdlib.Argv
        or (value.is_global() and value.module_name != "")
    )
    if is_global and value.is_var():
        utils.register_global(self.ctx, canonical)
        if self.ctx.cache.is_jit:
            imported_stdlib = utils.get_import_module(self.ctx, cache.STDLIB_IMPORT)
            imported_stdlib.ctx.add_toplevel(
                utils.get_unmangled_name(self.ctx, value.canonical_name), value
            )
    return assignment


def transform_update(self: TypeVisitor, stmt: ast.AssignStmt) -> ast.Stmt:
    """
    Transform binding updates. Special handling is done for atomic or in-place
    statements (e.g., `a += b`).
    See @c transformInplaceUpdate and @c wrapExpr for details.
    """

    stmt.lhs = self.visit(stmt.lhs)

    # Check inplace updates
    in_place, replacement = transform_inplace_update(self, stmt)
    if in_place:
        return replacement or stmt

    stmt.rhs = self.visit(stmt.rhs)
    stmt.type_expr = self.visit(stmt.type_expr, enforce_type=True)
    if stmt.type_expr:
        infer.unify(
            stmt.lhs.type,
            utils.instantiate_type(
                self.ctx, utils.extract_type(self.ctx, stmt.type_expr), info=stmt.type_expr.info
            ),
        )

    # Case: wrap expressions if needed (e.g. floats or optionals)
    can_wrap, stmt.rhs = utils.wrap_expr(self, stmt.rhs, stmt.lhs.type)
    if can_wrap:
        infer.unify(stmt.rhs.type, stmt.lhs.type)
    if stmt.rhs.done and infer.realize(stmt.lhs.type):
        stmt.done = True
    return stmt


def typecheck_assignmember(self: TypeVisitor, node: ast.AssignMemberStmt) -> ast.Node:
    """
    Typecheck instance member assignments (e.g., `a.b = c`) and handle optional
    instances. Disallow tuple updates.
    @example
    `opt.foo = bar` -> `unwrap(opt).foo = wrap(bar)`
    See @c wrapExpr for more examples.
    """

    node.lhs = self.visit(node.lhs)
    if (lhs_type := utils.extract_class_type(self.ctx, node.lhs)) is None:
        return node  # delay

    member = utils.find_member(self.ctx, lhs_type, node.member)
    # Case: property setters
    if member is None and (
        setters := utils.find_method(self.ctx, lhs_type, f"{cache.FN_SETTER_SUFFIX}{node.member}")
    ):
        setter_call = ast.CallExpr(
            ast.IdExpr(setters[0].get_func_name()), items=[node.lhs, node.rhs]
        )
        return self.visit(ast.ExprStmt(setter_call))

    # Case: class variables
    if member is None and (cls_data := utils.get_class(self.ctx, lhs_type)):
        if cls_var := cls_data.class_vars.get(node.member):
            rhs = self.visit(node.rhs)
            assignment = ast.AssignStmt(
                ast.IdExpr(cls_var), rhs=rhs, update=ast.AssignStmt.Mode.Update
            )
            return self.visit(assignment)

    # Unwrap optional and look up there
    if member is None and lhs_type.is_type(ast.types.Stdlib.Optional):
        unwrapped = ast.CallExpr(ast.IdExpr(ast.types.Stdlib.OptionalUnwrap), items=[node.lhs])
        assignment = ast.AssignMemberStmt(unwrapped, member=node.member, rhs=node.rhs)
        return self.visit(assignment)

    # Case: __setattr__ support. Ensure that only Literal[str] arguments are accepted.
    if member is None:
        static_name = utils.instantiate_unbound(self.ctx)
        static_name.static_kind = ast.types.Type.Behaviour.String
        value_type = utils.instantiate_unbound(self.ctx)
        setattr_method = utils.find_best_method(
            self.ctx, lhs_type, "__setattr__", [lhs_type, static_name, value_type]
        )
        if (
            setattr_method
            and setattr_method.func_generics
            and utils.extract_func_generic(setattr_method).get_static_kind()
            is ast.types.Type.Behaviour.String
        ):
            setattr_call = ast.CallExpr(
                ast.DotExpr(node.lhs, member="__setattr__"),
                items=[ast.StringExpr(node.member), node.rhs],
            )
            return self.visit(ast.ExprStmt(setattr_call))

    if member is None:
        raise TypecheckError(
            node,
            f"'{lhs_type.pretty_string()}' object has no attribute '{node.member}'",
        )

    if lhs_type.is_record():
        # prevent tuple member assignment
        raise TypecheckError(node, "cannot modify tuple attributes")

    node.rhs = self.visit(node.rhs)
    node.type_expr = self.visit(node.type_expr, enforce_type=True)
    if node.type_expr:
        infer.unify(
            node.rhs.type,
            utils.instantiate_type(
                self.ctx, utils.extract_type(self.ctx, node.type_expr), info=node.type_expr.info
            ),
        )
    field_type = utils.instantiate_type(self.ctx, member.type, lhs_type, node.lhs.info)
    if not field_type.can_realize() and member.type_expr:
        member_type = self.visit(member.type_expr.clone(clean=True))
        infer.unify(field_type, utils.extract_type(self.ctx, member_type))
    cache_class = utils.get_class(self.ctx, lhs_type)
    if member.base_class != lhs_type.name and cache_class and cache_class.has_rtti():
        base_type = None
        for candidate in utils.get_base_classes(self.ctx, lhs_type):
            if (
                candidate_class := candidate.get_class()
            ) and candidate_class.name == member.base_class:
                base_type = candidate
                break
        assert base_type is not None, f"cannot find base type of {lhs_type.debug_string(2)}"
        if not base_type.can_realize():
            return node  # delay!
        cast_call = ast.CallExpr(
            ast.IdExpr(ast.types.mangle("", "RTTIType", "_cast")),
            items=[node.lhs, ast.IdExpr(base_type.realized_name())],
        )
        base_assignment = ast.AssignMemberStmt(
            cast_call, member=node.member, rhs=node.rhs, type_expr=node.type_expr
        )
        return self.visit(base_assignment)

    can_wrap, node.rhs = utils.wrap_expr(self, node.rhs, field_type)
    if not can_wrap:
        return node
    infer.unify(node.rhs.type, field_type)
    if node.rhs.done:
        node.done = True
    return node


def transform_inplace_update(self: TypeVisitor, stmt: ast.AssignStmt):
    """
    Transform in-place and atomic updates.
    @example
    `a += b` -> `a.__iadd__(a, b)` if `__iadd__` exists
    Capsule operations:
    `a = b` -> a.val[0] = b
    `a += b` -> a.val[0] += b
    Atomic operations (when the needed magics are available):
    `a = b`         -> `type(a).__atomic_xchg__(__ptr__(a), b)`
    `a += b`        -> `type(a).__atomic_add__(__ptr__(a), b)`
    `a = min(a, b)` -> `type(a).__atomic_min__(__ptr__(a), b)` (same for `max`)
    @return a tuple indicating whether (1) the update statement can be replaced with an
    expression, and (2) the replacement expression.
    """

    match stmt.lhs, stmt.rhs:
        case ast.CallExpr(expr=expr), _ if utils.is_function_expr(
            expr, ast.types.mangle(cls="Capsule", func="_get")
        ):
            # Case: capsule operations
            transformed = ast.AssignStmt(
                ast.IndexExpr(
                    ast.CallExpr(
                        ast.IdExpr(ast.types.mangle(cls="Capsule", func="_ptr")),
                        items=[stmt.lhs.items[0].value],
                    ),
                    index=ast.IntExpr(0),
                ),
                rhs=stmt.rhs,
            )
            return True, self.visit(transformed)
        case _, ast.BinaryExpr(in_place=True) as binary if not stmt.is_atomic_update():
            # Case: in-place updates (e.g., `a += b`).
            # They are stored as `Update(a, Binary(a + b, inPlace=true))`
            binary.lexpr, binary.rexpr = self.visit(binary.lexpr), self.visit(binary.rexpr)
            if not isinstance(binary.type, ast.types.Type):
                binary.type = utils.instantiate_unbound(self.ctx)
            if binary.lexpr.get_class_type() and binary.rexpr.get_class_type():
                if replacement := ops.transform_binary_inplace_magic(
                    self, binary, stmt.is_atomic_update()
                ):
                    infer.unify(stmt.rhs.type, replacement.type)
                    transformed = self.visit(ast.ExprStmt(replacement))
                    return True, transformed
                return False, None
            else:
                infer.unify(
                    stmt.lhs.type, infer.unify(stmt.rhs.type, utils.instantiate_unbound(self.ctx))
                )
                return True, None
        case ast.IdExpr(value=name), ast.CallExpr(
            expr=ast.IdExpr(value="min" | "max" as fn_name),
            args=[ast.CallExpr.Arg(value=ast.IdExpr(value=arg_name)), other],
        ) if (
            stmt.is_atomic_update()
            and (item := self.ctx.find(arg_name))
            and item.canonical_name == name
        ):
            # Case: atomic min/max operations.
            # Note: check only `a = min(a, b)`; does NOT check `a = min(b, a)`

            # `type(a).__atomic_min__(__ptr__(a), b)`
            lhs_type = utils.extract_class_type(self.ctx, stmt.lhs)
            pointer = utils.instantiate_type(
                self.ctx,
                utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr),
                [lhs_type],
                stmt.lhs.info,
            )
            other.value = self.visit(other.value)
            if rhs_type := other.value.get_class_type():
                if method := utils.find_best_method(
                    self.ctx, lhs_type, f"__atomic_{fn_name}__", [pointer, rhs_type]
                ):
                    transformed = ast.ExprStmt(
                        ast.CallExpr(
                            ast.IdExpr(method.get_func_name()),
                            items=[ast.CallExpr(ast.IdExpr("__ptr__"), items=[stmt.lhs]), other],
                        )
                    )
                    return True, self.visit(transformed)
            return False, None
        case _ if stmt.is_atomic_update():
            # Case: atomic assignments
            lhs_type = utils.extract_class_type(self.ctx, stmt.lhs)
            stmt.rhs = self.visit(stmt.rhs)
            rhs_type = stmt.rhs.get_class_type()
            if lhs_type and rhs_type:
                pointer = utils.instantiate_type(
                    self.ctx,
                    utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Ptr),
                    [lhs_type],
                    stmt.lhs.info,
                )
                # `type(a).__atomic_xchg__(__ptr__(a), b)`
                if method := utils.find_best_method(
                    self.ctx, lhs_type, "__atomic_xchg__", [pointer, rhs_type]
                ):
                    transformed = ast.ExprStmt(
                        ast.CallExpr(
                            ast.IdExpr(method.get_func_name()),
                            items=[ast.CallExpr(ast.IdExpr("__ptr__"), items=[stmt.lhs]), stmt.rhs],
                        )
                    )
                    return True, self.visit(transformed)
    return False, None
