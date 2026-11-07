# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy
from typing import TYPE_CHECKING

from ... import ast, cache, error
from ...bridge import List, cast, dataclass
from ...error import TypecheckError
from . import utils
from .ctx import Item, TypeContext

if TYPE_CHECKING:
    from . import TypeVisitor


@dataclass(init=False)
class InitializedFields:
    """Initialization facts for the tracked receiver on a modeled control-flow path."""

    def __init__(self, fields=None, aliases=None, reachable=True):
        self.fields = set() if fields is None else set(fields)
        self.aliases = set() if aliases is None else set(aliases)
        self.reachable = reachable

    def clone(self):
        return InitializedFields(self.fields, self.aliases, self.reachable)

    def intersect(self, other):
        if not other.reachable:
            return
        if not self.reachable:
            self.fields, self.aliases, self.reachable = set(other.fields), set(other.aliases), True
        else:
            self.fields.intersection_update(other.fields)
            self.aliases.intersection_update(other.aliases)


class ReboundLocalsVisitor(ast.NodeVisitor):
    def __init__(self):
        self.names = set()

    def visit_FunctionStmt(self, node):
        pass

    def visit_ClassStmt(self, node):
        pass

    def visit_AssignStmt(self, node):
        if isinstance(node.lhs, ast.IdExpr):
            self.names.add(node.lhs.value)
        self.generic_visit(node)

    def visit_ForStmt(self, node):
        if isinstance(node.var, ast.IdExpr):
            self.names.add(node.var.value)
        self.generic_visit(node)

    def visit_Except(self, node):
        self.names.add(node.var)
        self.generic_visit(node)


class FieldInitializationVisitor(ast.NodeVisitor):
    """Conservatively track field writes in a realized AST, following known helpers."""

    def __init__(self, ctx, receiver, active, summaries):
        self.ctx, self.receiver = ctx, receiver
        self.active, self.summaries = active, summaries
        self.state = InitializedFields()
        self.exits, self.exceptions = [], []

    def visit(self, node):
        if node and self.state.reachable:
            return super().visit(node)

    def record_exception(self):
        if self.state.reachable and self.exceptions:
            self.exceptions[-1].intersect(self.state)

    def forget_rebound_aliases(self, node):
        rebound = ReboundLocalsVisitor()
        rebound.visit(node)
        self.state.aliases.difference_update(rebound.names)

    def analyze_loop_body(self, body):
        incoming = self.state.clone()
        previous = len(self.exits)
        self.visit(body)
        pending = self.exits[previous:]
        del self.exits[previous:]
        self.state = incoming
        for kind, state in pending:
            if kind == "break":
                self.state.intersect(state)
            elif kind == "return":
                self.exits.append((kind, state))

    def is_instance(self, expr):
        match expr:
            case ast.IdExpr(value=value):
                return value in self.state.aliases
            case ast.DotExpr(expr=source, member="_obj") if source.type == "Super":
                return self.is_instance(source)
            case ast.DotExpr(expr=ast.DotExpr(expr=source, member="args"), member="item1") if (
                source.type == "Partial"
            ):
                return self.is_instance(source)
            case ast.CallExpr() if expr.items:
                fn = expr.expr.type.func if expr.expr.type else None
                if not fn:
                    return False
                if fn.func_name.startswith(
                    (
                        ast.types.mangle("std.internal.types.rtti", func="force_cast"),
                        ast.types.mangle(cls="type", func="_force_cast"),
                        ast.types.mangle(cls="Super", func="__getattr__"),
                    )
                ):
                    return self.is_instance(expr.items[0].value)
                if fn.func_name.startswith(ast.types.mangle(cls="Super", func="__new__")):
                    return any(self.is_instance(a.value) for a in expr.items)
        return False

    def analyze(self, fn, argument):
        if fn.async_ or fn.has(ast.Attr.IsGenerator):
            return InitializedFields()
        key = (id(fn), argument)
        if key in self.summaries:
            return self.summaries[key].clone()
        if key in self.active:
            return self.state.clone()
        self.active.add(key)
        self.state.aliases.add(fn.items[argument].name)
        self.visit(fn.suite)
        for kind, state in self.exits:
            if kind == "return":
                self.state.intersect(state)
        self.active.remove(key)
        self.summaries[key] = self.state.clone()
        return self.state

    def visit_FunctionStmt(self, node):
        pass

    def visit_ClassStmt(self, node):
        pass

    def visit_AssignStmt(self, node):
        self.visit(node.rhs)
        if isinstance(node.lhs, ast.IdExpr) and node.rhs:
            if self.is_instance(node.rhs):
                self.state.aliases.add(node.lhs.value)
            else:
                self.state.aliases.discard(node.lhs.value)

    def visit_AssignMemberStmt(self, node):
        self.visit(node.lhs)
        self.visit(node.rhs)
        if self.is_instance(node.lhs):
            self.state.fields.add(node.member)

    def visit_CallExpr(self, node):
        self.generic_visit(node)
        if not self.state.reachable:
            return
        self.record_exception()
        fn = node.expr.type.func if node.expr.type else None
        if not fn:
            return
        if node.has(ast.Attr.ExprVirtual):
            dispatch = node.get(ast.Attr.ExprVirtual, int)
            thunk = dispatch if dispatch >= 0 else 0
            if dispatch < 0 and utils.is_polymorphic(self.ctx, fn):
                from .special import vtable_signature

                thunk = self.ctx.cache.thunk_ids.get(vtable_signature(self.ctx, fn), 0)
                if not thunk:
                    return
            if thunk:
                if not node.items or not self.is_instance(node.items[0].value):
                    return
                fn = self.receiver.vtable.get(thunk)
                if not fn:
                    return
        cached = self.ctx.cache.functions.get(fn.func_name)
        realization = cached.realizations.get(fn.realized_name()) if cached else None
        if not realization or not realization.ast:
            return
        argument = 0
        for idx, param in enumerate(realization.ast.items):
            if not param.is_value():
                continue
            if argument >= len(node.items):
                break
            value = node.items[argument].value
            argument += 1
            if self.is_instance(value):
                helper = FieldInitializationVisitor(
                    self.ctx, self.receiver, self.active, self.summaries
                )
                initialized = helper.analyze(realization.ast, idx)
                self.state.fields.update(initialized.fields)
                self.state.reachable &= initialized.reachable

    def visit_IfStmt(self, node):
        self.visit(node.cond)
        incoming = self.state.clone()
        self.visit(node.if_suite)
        first = self.state.clone()
        self.state = incoming
        self.visit(node.else_suite)
        self.state.intersect(first)

    def visit_IfExpr(self, node):
        self.visit(node.cond)
        incoming = self.state.clone()
        self.visit(node.ifexpr)
        first = self.state.clone()
        self.state = incoming
        self.visit(node.elsexpr)
        self.state.intersect(first)

    def visit_WhileStmt(self, node):
        self.forget_rebound_aliases(node)
        self.visit(node.cond)
        self.analyze_loop_body(node.suite)

    def visit_ForStmt(self, node):
        self.visit(node.iter)
        self.forget_rebound_aliases(node)
        self.record_exception()
        self.analyze_loop_body(node.suite)

    def visit_BreakStmt(self, node):
        self.exits.append(("break", self.state.clone()))
        self.state.reachable = False

    def visit_ContinueStmt(self, node):
        self.exits.append(("continue", self.state.clone()))
        self.state.reachable = False

    def visit_ReturnStmt(self, node):
        self.visit(node.expr)
        if self.state.reachable:
            self.exits.append(("return", self.state.clone()))
        self.state.reachable = False

    def visit_ThrowStmt(self, node):
        self.visit(node.expr)
        self.record_exception()
        self.state.reachable = False

    def visit_TryStmt(self, node):
        previous = len(self.exits)
        self.exceptions.append(InitializedFields(reachable=False))
        self.visit(node.suite)
        thrown = self.exceptions[-1].clone()
        self.exceptions[-1] = InitializedFields(reachable=False)
        self.visit(node.else_suite)
        normal = self.state.clone()
        catches_all = False
        for handler in node.items:
            self.state = thrown.clone()
            self.state.aliases.discard(handler.var)
            self.visit(handler.suite)
            normal.intersect(self.state)
            if (
                not handler.exc
                or utils.extract_type(self.ctx, handler.exc.type) == ast.types.Stdlib.BaseException
            ):
                catches_all = True
        if not catches_all:
            self.exceptions[-1].intersect(thrown)
        unhandled = self.exceptions.pop()
        pending = self.exits[previous:]
        del self.exits[previous:]
        for kind, state in pending:
            self.state = state.clone()
            self.visit(node.finally_suite)
            if self.state.reachable:
                self.exits.append((kind, self.state.clone()))
        self.state = unhandled
        self.visit(node.finally_suite)
        self.record_exception()
        self.state = normal
        self.visit(node.finally_suite)


def check_field_initialization(ctx, typ, fn):
    """Reject fields not proven initialized at a realized constructor's normal exits."""
    if (
        not typ.func_parent
        or utils.get_unmangled_name(ctx, typ.func_name) != "__init__"
        or not fn.items
    ):
        return
    parent = typ.func_parent.require_cls
    cls = utils.get_class(ctx, parent)
    if parent.is_tuple or not cls or not cls.ast or cls.ast.has(ast.Attr.Internal):
        return
    receiver = utils.get_class_realization(ctx, typ[0].cls or parent)
    initialized = FieldInitializationVisitor(ctx, receiver, set(), {}).analyze(fn, 0)
    if initialized.reachable:
        for field in utils.get_class_fields(ctx, parent):
            if field.name not in initialized.fields:
                raise TypecheckError(
                    fn,
                    f"field '{utils.get_unmangled_name(ctx, parent.name)}.{field.name}' may be uninitialized when __init__ returns",
                )


def typecheck_class(self: TypeVisitor, node: ast.ClassStmt) -> ast.Node:
    """Parse a class (type) declaration and add a (generic) type to the context."""

    name = node.name
    canonical = ""
    args_to_parse = node.items

    # item will be added later when the scope is different
    item = Item(
        "",
        "",
        module=self.ctx.module_name,
        typ=ast.types.Type(self.ctx.cache),  # dummy
        block_level=self.ctx.block_level,
        info=node.info,
    )
    timed_item = None
    class_type: ast.types.Class | None = None
    if not node.has(ast.Attr.Extend):
        canonical = self.ctx.generate_canonical_name(
            name,
            not node.has(ast.Attr.Internal),
            no_suffix=node.has(ast.Attr.Internal),
        )
        item.canonical = canonical
        if canonical == ast.types.Stdlib.Union:
            item.type = ast.types.Union(cache=self.ctx.cache)
        else:
            item.type = ast.types.Class(cache=self.ctx.cache, name=canonical)
        class_type = item.type.require_cls
        if node.is_record():
            class_type.is_tuple = True
        item.type.info = node.info
        if canonical != ast.types.Stdlib.Type:
            item.type = utils.instantiate_type_var(self.ctx, item.type)
        timed_item = copy.copy(item)
        # Reference types are added to the context here.
        # Tuple types are added after class contents are parsed to prevent
        # recursive record types (note: these are allowed for reference types)
        if not node.has(ast.Attr.Tuple):
            self.ctx.add(name, timed_item)
            self.ctx.add_always_visible(item)
    else:
        # Find the canonical name and AST of the class that is to be extended
        if not self.ctx.is_global or self.ctx.is_conditional:
            raise TypecheckError(node, "class extension must be a top-level statement")
        value = self.ctx.find_at(name, self.ctx.time)
        if not value or not value.is_type:
            raise TypecheckError(node, f"class name '{name}' is not defined")
        class_type = (
            value.type.require_cls
            if value.canonical == ast.types.Stdlib.Type
            else utils.extract_class_type(self.ctx, value.type)
        )
        cls_data = utils.get_class(self.ctx, class_type)
        assert cls_data and cls_data.ast
        if cls_data.ast.has(ast.Attr.NoExtend):
            if not self.ctx.is_stdlib_loading and not node.has(ast.Attr.AutoGenerated):
                raise TypecheckError(node, f"'{name}' cannot be extended")
        canonical = class_type.name
        args_to_parse = cls_data.ast.items

    cls_data = self.ctx.cache.classes.setdefault(canonical, cache.ClassData())
    class_stmts = []
    var_stmts = []
    fn_stmts = []
    add_later = []
    try:
        with self.ctx.within_base(canonical):
            current_base = self.ctx.base
            current_base.type = class_type

            # Parse and add class generics
            args = []
            if node.has(ast.Attr.Extend):
                for arg in args_to_parse:
                    if not arg.is_generic():
                        continue
                    generic_item = self.ctx[arg.name]
                    generic_item.type.require_link.kind = ast.types.Link.Kind.Unbound
                    self.ctx.add(
                        utils.get_unmangled_name(self.ctx, generic_item.canonical),
                        generic_item,
                    )
                    args.append(ast.Param(generic_item.canonical, status=arg.status))
            else:
                if node.has(ast.Attr.ClassDeduce):
                    if not auto_deduce_members(self, node, args_to_parse):
                        node.erase(ast.Attr.ClassDeduce)
                # Add all generics before parent classes, fields and methods
                for arg in args_to_parse:
                    if not arg.is_generic():
                        continue
                    var_name = self.ctx.generate_canonical_name(arg.name)
                    generic_type = utils.instantiate_unbound(self.ctx)
                    type_id = generic_type.id
                    generic_type.generic_name = var_name
                    default_expr = (
                        self.visit_expr(arg.default.clone(), enforce_type=True)
                        if arg.default
                        else None
                    )
                    if default_expr:
                        generic_type.default_type = utils.extract_type(self.ctx, default_expr)

                    if (
                        static_kind := ast.get_static_generic(arg.type)
                    ) is not ast.types.Type.Behaviour.Runtime:
                        generic_type._static_kind = static_kind
                        generic_item = self.ctx.add_item(arg.name, var_name, generic_type)
                        generic_item.generic = True
                    else:
                        if isinstance(arg.type, ast.IndexExpr):
                            # Parse TraitVar
                            arg.type = self.visit_expr(arg.type)
                            assert (
                                isinstance(arg.type, ast.InstantiateExpr)
                                and isinstance(arg.type.expr, ast.IdExpr)
                                and arg.type.expr.value == ast.types.Stdlib.TypeTrait
                                and arg.type.items
                            ), "invalid type trait"
                            arg.type = arg.type
                            trait_type = utils.extract_type(self.ctx, arg.type.items[0])
                            generic_type.trait = (
                                trait_type
                                if isinstance(trait_type, ast.types.TypeTrait)
                                else ast.types.TypeTrait(trait_type)
                            )
                        generic_item = self.ctx.add_item(arg.name, var_name, generic_type)
                        generic_item.generic = True
                    class_type.generics.append(
                        ast.types.Generic(
                            var_name,
                            generic_type.generalize(self.ctx.typecheck_level),
                            type_id,
                            generic_type.static_kind,
                        )
                    )
                    args.append(
                        ast.Param(var_name, type=arg.type, default=default_expr, status=arg.status)
                    )

            enum_init = prepare_enum(self, node, class_type, var_stmts)

            # Form class type node (e.g. `Foo`, or `Foo[T, U]` for generic classes)
            type_expr = None
            if not node.has(ast.Attr.Extend):
                type_expr = ast.IdExpr(canonical)
                for arg in args:
                    if arg.is_generic():
                        if not isinstance(type_expr, ast.InstantiateExpr):
                            type_expr = ast.InstantiateExpr(ast.IdExpr(canonical))
                        type_expr.items.append(
                            self.visit_expr(ast.IdExpr(arg.name), type_allowed=True)
                        )

            # A ClassStmt will be separated into class variable assignments, method-free
            # ClassStmts (that include nested classes) and method FunctionStmts
            transform_nested_classes(self, node, class_stmts, var_stmts, fn_stmts)

            # Collect class fields
            for arg in args_to_parse:
                if arg.is_value() and ast.ClassStmt.is_class_var(arg):
                    # Handle class variables. Transform them later to allow self-references
                    var_name = f"{canonical}.{arg.name}"
                    declaration = self.visit_stmt(ast.AssignStmt(ast.IdExpr(var_name)))
                    self.ctx.preamble.add(declaration)
                    cls_var = self.ctx[var_name]
                    cls_var.base = ""
                    cls_var.block_level = 0
                    utils.register_global(self.ctx, cls_var.canonical)
                    if arg.default:
                        default = self.visit_expr(arg.default)
                        annotated_type = (
                            None if not arg.type else cast(ast.IndexExpr, arg.type).index
                        )
                        var_stmts.append(
                            ast.AssignStmt(
                                ast.IdExpr(var_name),
                                rhs=default,
                                type_expr=annotated_type,
                                update=ast.AssignStmt.Mode.Update,
                            )
                        )
                    cls_data.class_vars[arg.name] = var_name
                    self.ctx.add(arg.name, cls_var)
                elif arg.is_value() and not node.has(ast.Attr.Extend):
                    var_name = arg.name
                    typ = (
                        self.visit_expr(
                            arg.type.clone(clean=True), enforce_type=True, simple_types=True
                        )
                        if arg.type
                        else None
                    )
                    default = (
                        self.visit_expr(arg.default.clone(), type_allowed=True)
                        if arg.default
                        else None
                    )
                    args.append(ast.Param(var_name, type=typ, default=default))
                    cls_data.fields.append(
                        cache.ClassData.Field(
                            name=var_name,
                            base_class=canonical,
                            type=ast.types.Type(self.ctx.cache),  # Use dummy type
                        )
                    )

            # Handle RTTI inheritance. Must come after other arguments are handled
            if not node.has(ast.Attr.Extend):
                parse_base_classes(self, node.base_classes, args, node, canonical, class_type)

            member_args = []
            arg_bases = {field.name: field.base_class for field in cls_data.fields}
            for arg in args:
                if arg.is_value() and arg_bases.get(arg.name, "") == canonical:
                    member_args.append(arg.clone())

            # Handle class members
            if not node.has(ast.Attr.Extend):
                # increase typecheck level to avoid unifying generics early
                with self.ctx.substitute("typecheck_level", self.ctx.typecheck_level + 1):
                    if canonical == ast.types.Stdlib.Tuple:
                        # Special tuple handling!
                        for field_idx in range(cache.MAX_TUPLE):
                            generic_name = f"T{field_idx + 1}"
                            generic_canonical_name = self.ctx.generate_canonical_name(generic_name)
                            tuple_generic = utils.instantiate_unbound(self.ctx)
                            tuple_generic.generic_name = generic_name
                            cls_data.fields.append(
                                cache.ClassData.Field(
                                    name=f"item{field_idx + 1}",
                                    type=tuple_generic.generalize(self.ctx.typecheck_level),
                                    type_expr=ast.IdExpr(generic_canonical_name),
                                )
                            )
                    else:
                        field_idx = 0
                        for arg in args:
                            if arg.is_value() and not ast.ClassStmt.is_class_var(arg):
                                cls_data.fields[field_idx].type_expr = arg.type.clone(True)
                                field_type = utils.extract_type(self.ctx, arg.type).generalize(
                                    self.ctx.typecheck_level - 1
                                )
                                field_type.info = arg.type.info
                                cls_data.fields[field_idx].type = field_type
                                field_idx += 1

            # Parse class members (arguments) and methods
            if not node.has(ast.Attr.Extend):
                # Now that we are done with arguments, add record type to the context
                if node.has(ast.Attr.Tuple):
                    assert timed_item
                    self.ctx.add(name, timed_item)
                    self.ctx.add_always_visible(item)
                node.set(
                    ast.Attr.Module,
                    cache.STDLIB_IMPORT
                    if self.ctx.module.status is cache.Import.File.Status.StdLibrary
                    else self.ctx.module.path,
                )
                # Create a cached AST
                cached_ast = ast.ClassStmt(
                    canonical,
                    items=[arg.clone() for arg in args],
                    suite=ast.SuiteStmt(),
                    base_classes=node.base_classes,
                )
                cached_ast.attributes = copy.deepcopy(node.attributes)
                cls_data.ast = cached_ast
                cls_data.module = self.ctx.module.path
                cls_data.jit_cell = self.ctx.cache.jit_cell

                # Codegen default magic methods
                custom_new = any(
                    isinstance(stmt, ast.FunctionStmt) and stmt.name == "__new__"
                    for stmt in get_class_methods(self, node.suite)
                )
                if magics := node.get(ast.Attr.ClassMagic, list[str]):
                    assert type_expr
                    for magic in magics:
                        if magic == "new" and custom_new and not node.is_record():
                            continue
                        magic_statement = codegen_magic(
                            self, magic, type_expr, member_args, node.has(ast.Attr.Tuple)
                        )
                        fn_stmts.append(self.visit_stmt(magic_statement))

            # Add class methods
            for stmt in get_class_methods(self, cast(ast.Stmt, node.suite)):
                if isinstance(stmt, ast.FunctionStmt):
                    for idx, decorator in enumerate(stmt.decorators):
                        match decorator:
                            case ast.DotExpr(ast.IdExpr(value=name), member="setter") if (
                                name == stmt.name and sum(p.is_value() for p in stmt.items) == 2
                            ):
                                # Handle @setter setters
                                stmt.name = f"{cache.FN_SETTER_SUFFIX}{stmt.name}"
                                stmt.decorators[idx] = None
                                break
                    # All tuple methods are marked with AutoGenerated for later convenience
                    # (e.g. heterogenous tuple processing).
                    if canonical == ast.types.Stdlib.Tuple:
                        stmt.set(ast.Attr.AutoGenerated)
                    fn_stmts.append(self.visit_stmt(stmt))

            # After popping context block, record types and nested classes will disappear.
            # Store their references and re-add them to the context after popping
            add_later = [self.ctx[stmt.name] for stmt in class_stmts]
            if node.has(ast.Attr.Tuple):
                add_later.append(self.ctx[name])

            # Mark functions as virtual
            not_virtual = {
                "__init__",
                "__new__",
                "__raw__",
                "__tuplesize__",
                "__repr_default__",
            }
            for name in sorted(cls_data.methods):
                # Current class
                for mro_idx in range(1, len(cls_data.mro)):
                    base_name = cls_data.mro[mro_idx].name
                    base_class = utils.get_class(self.ctx, base_name)
                    assert base_class
                    if name in base_class.methods and name not in not_virtual:
                        cls_data.virtuals.add(name)
                # Parent class
                for virtual_name in sorted(cls_data.virtuals):
                    for mro_idx in range(1, len(cls_data.mro)):
                        base_class = utils.get_class(self.ctx, cls_data.mro[mro_idx].name)
                        assert base_class
                        base_class.virtuals.add(virtual_name)

            # Generalize generics and remove them from the context
            for arg in args:
                if not arg.is_value():
                    generic_item = self.ctx[arg.name]
                    generic_type = generic_item.type
                    if arg.status is ast.Param.Status.Generic:
                        # Generalize generics. Hidden generics are linked to the class generics so
                        # ignore them
                        assert generic_type.require_link.kind is not ast.types.Link.Kind.Link
                        generic_type.require_link.kind = ast.types.Link.Kind.Generic
                    self.ctx.remove(arg.name)

            # if not canonical_name.startswith("Tuple"):
            #     self.log(f"[class] {canonical_name} -> {class_type} / {len(cls_data.fields)}")
            #     for class_field in cls_data.fields:
            #         self.log(f"       - member: {class_field.name}: {class_field.type}")
            #     for name, method_root in cls_data.methods.items():
            #         self.log(f"       - method: {name}: {method_root}")
            #     for mro_type in cls_data.mro:
            #         self.log(f"       - mro: {mro_type}")
    except error.ParserError:
        if not node.has(ast.Attr.Tuple):
            self.ctx.remove(name)
        self.ctx.cache.classes.pop(name, None)
        raise
    for item in add_later:
        self.ctx.add(utils.get_unmangled_name(self.ctx, item.canonical), item)

    # Extensions are not needed as the cache is already populated
    if not node.has(ast.Attr.Extend):
        cls_ast = cls_data.ast
        assert cls_ast is not None, f"not a class AST for {canonical}"
        cls_ast.done = True
        class_stmts.append(cls_ast)
        if canonical == ast.types.Stdlib.Int:
            from . import infer

            # Auto-realize int64
            realized = utils.instantiate(self.ctx, class_type)
            value = Item(
                "Int[64]", "", self.ctx.module_name, utils.instantiate_type_var(self.ctx, realized)
            )
            self.ctx.add_always_visible(value, True)
            self.ctx.cache.type_ctx.add("int", value)
            realized[0] |= ast.types.IntLiteral(64, cache=self.ctx.cache)
            self.ctx.cache.class_realization_cnt += 1
            cls_data.realizations["Int[64]"] = cache.ClassData.Realization(
                type=realized, id=self.ctx.cache.class_realization_cnt
            )
            infer.make_ir_type(self.ctx, realized)
    class_stmts.extend(fn_stmts)
    for assignment in var_stmts:
        class_stmts.append(self.visit_stmt(assignment))
    for stmt in enum_init:
        class_stmts.append(self.visit_stmt(stmt))
    return ast.SuiteStmt(*class_stmts)


def is_enum(ctx, typ):
    return any(
        base == ast.types.mangle("std.enum", cls="Enum") for base in utils.get_class(ctx, typ).mro
    )


def prepare_enum(self, node, typ, values):
    """Lower Enum declarations to reference classes and singleton globals."""
    if node.has(ast.Attr.Extend):
        return []
    enumeration = False
    for base in node.base_classes:
        cls = utils.extract_class_type(
            self.ctx, self.visit_expr(base.clone(), enforce_type=True, simple_types=True)
        )
        if not cls or not is_enum(self.ctx, cls):
            continue
        for name, var in utils.get_class(self.ctx, cls).class_vars.items():
            if not name.startswith("__") and self.ctx[var].type == cls.name:
                raise TypecheckError(node, "cannot extend an enumeration with members")
        enumeration = True
    if not enumeration:
        return []
    if len(node.base_classes) != 1 or node.has(ast.Attr.Tuple):
        raise TypecheckError(node, "Enum requires a single enumeration base class")
    for stmt in get_class_methods(self, node.suite):
        if isinstance(stmt, ast.FunctionStmt) and stmt.name in {"__init__", "__new__"}:
            raise TypecheckError(stmt, "custom enumeration constructors are not supported")
    names, raw_values, members, init = [], [], [], []
    member_buffer = utils.get_temporary_var(self.ctx, "enum_members")
    auto_maximum = ""

    def self_type():
        return ast.IdExpr(typ.name)

    def helper(name):
        return ast.IdExpr(ast.types.mangle("std.enum", func=name))

    for param in node.items:
        if (
            not param.is_value()
            or not ast.ClassStmt.is_class_var(param)
            or not param.default
            or param.name.startswith("__")
        ):
            continue
        if param.name in {"_name_", "_value_", "name", "value"}:
            raise TypecheckError(param, f"reserved enumeration member name '{param.name}'")
        value = param.default.clone()
        if isinstance(value, ast.CallExpr):
            head = self.visit_expr(value.expr.clone())
            if utils.is_type_expr(head) and utils.extract_class_type(
                self.ctx, head
            ) == ast.types.mangle("std.enum", cls="auto"):
                if value.items:
                    raise TypecheckError(value, "auto() does not accept arguments")
                value = (
                    ast.IntExpr(1)
                    if not raw_values
                    else ast.CallExpr(
                        helper("_next_value"), [ast.IdExpr(auto_maximum), raw_values[0].clone()]
                    )
                )
        raw = utils.get_temporary_var(self.ctx, "enum_value")
        values.append(self.visit_stmt(ast.AssignStmt(ast.IdExpr(raw), value)))
        binding = self.ctx[raw]
        binding.base, binding.block_level = "", 0
        utils.register_global(self.ctx, binding.canonical)
        self.ctx.add(param.name, binding)
        raw_values.append(ast.IdExpr(binding.canonical))
        if not auto_maximum:
            auto_maximum = utils.get_temporary_var(self.ctx, "enum_maximum")
            values.append(
                self.visit_stmt(
                    ast.AssignStmt(
                        ast.IdExpr(auto_maximum),
                        ast.CallExpr(
                            helper("_auto_maximum"),
                            [raw_values[-1].clone(), ast.IntExpr(0), ast.BoolExpr(True)],
                        ),
                    )
                )
            )
            maximum = self.ctx[auto_maximum]
            maximum.base, maximum.block_level = "", 0
            utils.register_global(self.ctx, maximum.canonical)
            auto_maximum = maximum.canonical
        else:
            values.append(
                self.visit_stmt(
                    ast.AssignStmt(
                        ast.IdExpr(auto_maximum),
                        ast.CallExpr(
                            helper("_auto_maximum"),
                            [raw_values[-1].clone(), ast.IdExpr(auto_maximum), ast.BoolExpr(False)],
                        ),
                        update=ast.AssignStmt.Mode.Update,
                    )
                )
            )
        member = ast.IdExpr(f"{typ.name}.{param.name}")
        init.append(
            ast.AssignStmt(
                member.clone(),
                ast.CallExpr(
                    helper("_make_member"),
                    [
                        self_type(),
                        ast.StringExpr(param.name),
                        raw_values[-1].clone(),
                        ast.IdExpr(member_buffer),
                        ast.IntExpr(len(members)),
                    ],
                ),
                type_expr=self_type(),
                update=ast.AssignStmt.Mode.Update,
            )
        )
        names.append(param.name)
        members.append(member)
        param.type = ast.IndexExpr(ast.IdExpr("ClassVar"), self_type())
        param.default = None
    if raw_values:
        from . import infer

        value_type = self.ctx[raw_values[0].value].type
        for value in raw_values:
            actual = self.ctx[value.value].type
            if (
                utils.get_underlying_static_type(self.ctx, value_type).unify(
                    utils.get_underlying_static_type(self.ctx, actual), None
                )
                < 0
            ):
                raise TypecheckError(node, "enumeration values must have the same type")
        realized = infer.realize(self.ctx, utils.get_underlying_static_type(self.ctx, value_type))
        if not realized:
            raise TypecheckError(node, "cannot infer enumeration value type")
        node.items.append(ast.Param("_value_", ast.IdExpr(realized.realized_name())))
        index_type = ast.IndexExpr(
            ast.IdExpr(ast.types.mangle("std.enum", cls="_ValueIndex")),
            ast.TupleExpr([ast.IdExpr(realized.realized_name()), self_type()]),
        )
        node.items.append(
            ast.Param("__value_index__", ast.IndexExpr(ast.IdExpr("ClassVar"), index_type))
        )
        init.insert(
            0,
            ast.AssignStmt(
                ast.IdExpr(member_buffer),
                ast.CallExpr(
                    ast.IndexExpr(ast.IdExpr("Ptr"), self_type()), [ast.IntExpr(len(members))]
                ),
            ),
        )
    mapping_type = ast.IndexExpr(
        ast.IdExpr(ast.types.mangle("std.internal.types.collections.dict", cls="OrderedDict")),
        ast.TupleExpr([ast.IdExpr(ast.types.Stdlib.String), self_type()]),
    )
    node.items.append(
        ast.Param("__members__", ast.IndexExpr(ast.IdExpr("ClassVar"), mapping_type.clone()))
    )
    entries = [
        ast.TupleExpr([ast.StringExpr(name), member.clone()])
        for name, member in zip(names, members)
    ]
    mapping = (
        ast.CallExpr(
            mapping_type, [ast.CallExpr(ast.DotExpr(ast.ListExpr(items=entries), "__iter__"))]
        )
        if names
        else ast.CallExpr(mapping_type)
    )
    init.append(
        ast.AssignStmt(
            ast.IdExpr(f"{typ.name}.__members__"), mapping, update=ast.AssignStmt.Mode.Update
        )
    )
    if raw_values:
        init.append(
            ast.AssignStmt(
                ast.IdExpr(f"{typ.name}.__value_index__"),
                ast.CallExpr(helper("_make_value_index"), [self_type(), raw_values[0].clone()]),
                update=ast.AssignStmt.Mode.Update,
            )
        )
    node.erase(ast.Attr.ClassDeduce)
    if magic := node.get(ast.Attr.ClassMagic, List[str]):
        magic[:] = [name for name in magic if name not in {"init", "new"}]
    return init


def parse_base_classes(
    self: TypeVisitor,
    bases: List[ast.Expr],
    args: List[ast.Param],
    attr: ast.Stmt,
    canonical_name: str,
    typ: ast.types.Class,
) -> List[ast.types.Class]:
    """
    Parse statically inherited classes.
    Returns a list of their ASTs. Also updates the class fields.
    @param args Class fields that are to be updated with base classes' fields.
    """

    asts = []
    mro = [[typ]]
    for idx, base_expr in enumerate(bases):
        # Get the base class and generic replacements (e.g., if there is Bar[T],
        # Bar in Foo(Bar[int]) will have `T = int`)
        original = base_expr.clone()
        base_expr = bases[idx] = self.visit_expr(base_expr, enforce_type=True, simple_types=True)
        if not base_expr or not base_expr.cls:
            raise TypecheckError(
                attr,
                f"cannot use '{utils.get_unmangled_name(self.ctx, str(original))}' as a base class",
            )
        base_type = utils.extract_class_type(self.ctx, base_expr)
        base_data = utils.get_class(self.ctx, base_type)
        assert base_data
        if not base_data.ast:
            raise TypecheckError(attr, "nested classes cannot inherit surrounding classes")

        # Sanity checks
        if attr.has(ast.Attr.Tuple):
            raise TypecheckError(attr, "tuple classes cannot inherit other classes")
        if base_data.ast.has(ast.Attr.Tuple):
            raise TypecheckError(attr, "reference classes cannot inherit tuple classes")
        if base_data.ast.has(ast.Attr.Internal):
            raise TypecheckError(attr, "internal classes cannot inherit other classes")

        # Mark parent classes as polymorphic as well.
        if not base_data.has_rtti():
            if self.ctx.cache.options.jit and base_data.jit_cell != self.ctx.cache.jit_cell:
                raise TypecheckError(
                    base_expr,
                    f"cannot inherit from a non-RTTI class defined in previous cell '{utils.get_unmangled_name(self.ctx, base_type.name)}' in JIT mode",
                )
            if base_type.name != ast.types.Stdlib.Object and len(base_data.mro) == 1:
                object_type = utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Object)
                base_data.mro.append(object_type)
                utils.get_class(self.ctx, object_type).descendants.add(base_type.name)
                for realization in base_data.realizations.values():
                    realization.bases.append(object_type)

        root_mro = [utils.instantiate(self.ctx, typ, base_type) for typ in base_data.mro]
        mro.append(root_mro)
        asts.append(base_type.require_cls)

        # Add hidden generics
        utils.add_class_generics(self.ctx, base_type)
        for generic in base_type.generics:
            generic.type = generic.type.generalize(self.ctx.typecheck_level)
            typ.hidden_generics.append(generic)
        for generic in base_type.hidden_generics:
            generic.type = generic.type.generalize(self.ctx.typecheck_level)
            typ.hidden_generics.append(generic)

    cls_data = utils.get_class(self.ctx, typ)
    assert cls_data
    if asts or typ.name == ast.types.Stdlib.Object:
        mro.append(asts)
    cls_data.mro = cache.Cache.merge_c3(mro)
    if not cls_data.mro:
        raise TypecheckError(attr, "inconsistent class hierarchy")
    asts = cls_data.mro  # use MRO order
    for mro_type in cls_data.mro:
        mro_class = utils.get_class(self.ctx, mro_type)
        assert mro_class
        mro_class.descendants.add(canonical_name)

    # Add normal fields
    for base in asts:
        if base.name == canonical_name:
            continue
        for var_name, canonical in sorted(utils.get_class(self.ctx, base).class_vars.items()):
            if canonical != f"{base.name}.{var_name}" or var_name in cls_data.class_vars:
                continue
            new_name = f"{canonical_name}.{var_name}"
            new_canonical = self.ctx.generate_canonical_name(new_name)
            cls_data.class_vars[var_name] = canonical
            self.ctx.add(new_name, self.ctx[canonical])
            self.ctx.add(new_canonical, self.ctx[canonical])
    for inherited_type in asts:
        if inherited_type.name == typ.name:
            continue
        inherited_data = utils.get_class(self.ctx, inherited_type)
        assert inherited_data
        with utils.with_class_generics(self.ctx, inherited_type):
            arg_idx = 0
            inherited_ast = inherited_data.ast
            assert inherited_ast
            for arg in inherited_ast:
                if arg.is_value() and not ast.ClassStmt.is_class_var(arg):
                    if inherited_data.fields[arg_idx].base_class == inherited_ast.name:
                        field_name = arg.name
                        for existing_arg in args:
                            if existing_arg.name == field_name:
                                raise TypecheckError(
                                    attr,
                                    f"field '{utils.get_unmangled_name(self.ctx, field_name)}' already defined",
                                )
                        args.append(
                            ast.Param(
                                field_name,
                                type=self.visit_expr(
                                    arg.type.clone(clean=True), enforce_type=True, simple_types=True
                                )
                                if arg.type
                                else None,
                                default=self.visit_expr(arg.default.clone(clean=True))
                                if arg.default
                                else None,
                            )
                        )
                        extracted = utils.extract_type(self.ctx, args[-1].type)
                        cls_data.fields.append(
                            cache.ClassData.Field(
                                field_name, extracted, inherited_data.fields[arg_idx].base_class
                            )
                        )
                    arg_idx += 1
    return asts


def auto_deduce_members(self: TypeVisitor, node: ast.ClassStmt, args: List[ast.Param]):
    """
    Find the first __init__ with self parameter and use it to deduce class members.
    Each deduced member will be treated as generic.
    @example
    ```@deduce
    class Foo:
    def __init__(self):
    self.x, self.y = 1, 2```
    will result in
    ```class Foo[T1, T2]:
    x: T1
    y: T2```
    @return the transformed init and the pointer to the original function.
    """

    members = set()
    for stmt in get_class_methods(self, cast(ast.Stmt, node.suite)):
        match stmt:
            case ast.FunctionStmt(name="__init__"):
                if deduced := stmt.get(ast.Attr.ClassDeduce, list[str]):
                    for member in deduced:
                        members.add(member)
    if members:
        if magics := node.get(ast.Attr.ClassMagic, list[str]):
            magics[:] = [m for m in magics if m != "init"]
        for member in sorted(members):
            generic_name = f"T_{member}"
            args.append(
                ast.Param(
                    generic_name,
                    type=ast.IdExpr(ast.types.Stdlib.Type),
                    default=ast.IdExpr(ast.types.Stdlib.NoneType),
                    status=ast.Param.Status.Generic,
                )
            )
            args.append(ast.Param(member, type=ast.IdExpr(generic_name)))
        return True
    return False


def get_class_methods(self: TypeVisitor, node: ast.Stmt | None):
    """
    Return a list of all statements within a given class suite.
    Checks each suite recursively, and assumes that each statement is either
    a function, a class or a docstring.
    """
    match node:
        case None:
            return []
        case ast.SuiteStmt(items=items):
            for i in items:
                yield from get_class_methods(self, i)
        case ast.FunctionStmt() | ast.ClassStmt():
            yield node
        case ast.ExprStmt(expr=ast.StringExpr()):
            pass
        case _:
            raise TypecheckError(node, "unexpected expression in class definition")


def transform_nested_classes(
    self: TypeVisitor,
    node: ast.ClassStmt,
    cls_stmts: List[ast.Stmt],
    var_stmts: List[ast.Stmt],
    fn_stmts: List[ast.Stmt],
):
    """Extract nested classes and transform them before the main class."""

    # If class B is nested within A, it's name is always A.B, never B itself.
    # Ensure that parent class name is appended
    for stmt in get_class_methods(self, cast(ast.Stmt, node.suite)):
        if isinstance(stmt, ast.ClassStmt):
            orig_name = stmt.name
            parent_name = node.name
            stmt.name = f"{parent_name}.{orig_name}"
            transformed = self.visit_stmt(stmt)
            if isinstance(transformed, ast.SuiteStmt):
                name = ""
                for child in transformed.items:
                    if isinstance(child, ast.ClassStmt):
                        cls_stmts.append(child)
                        name = child.name
                    elif isinstance(child, ast.AssignStmt):
                        var_stmts.append(child)
                    else:
                        fn_stmts.append(child)
                self.ctx.add(orig_name, self.ctx[name])


def codegen_magic(
    self: TypeVisitor,
    magic: str,
    type_expr: ast.Expr,
    all_args: List[ast.Param],
    is_record: bool,
) -> ast.Stmt:
    """
    Generate a magic method `__op__` for each magic `op`
    described by @param typExpr and its arguments.
    Currently, generate:
    @li Constructors: __new__, __init__
    @li Utilities: __raw__, __hash__, __repr__, __tuplesize__, __add__, __mul__, __len__
    @li Iteration: __iter__, __getitem__, __len__, __contains__
    @li Comparisons: __eq__, __ne__, __lt__, __le__, __gt__, __ge__
    @li Pickling: __pickle__, __unpickle__
    @li Python: __to_py__, __from_py__
    @li GPU: __to_gpu__, __from_gpu__, __from_gpu_new__
    """

    ret = None
    fn_args = []
    stmts = []
    attrs = [ast.Attr.AutoGenerated]
    # Tuples: def __new__() -> T (internal)
    args = [arg.clone() for arg in all_args]

    M = lambda value: ast.DotExpr(ast.IdExpr("__magic__"), member=value)

    if magic == "new":
        ret = type_expr.clone()
        if is_record:
            # Tuples: def __new__() -> T (internal)
            for arg in args:
                fn_args.append(
                    ast.Param(
                        arg.name,
                        type=None if arg.type is None else arg.type.clone(),
                        default=None if arg.default is None else arg.default.clone(),
                    )
                )
            attrs.append(ast.Attr.Internal)
        else:
            # Classes: def __new__() -> T
            stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [type_expr.clone()])))
    elif magic == "init":
        # Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> None:
        # self.aI = aI ...
        ret = ast.IdExpr(ast.types.Stdlib.NoneType)
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        stmts.append(
            ast.IfStmt(
                ast.CallExpr(
                    ast.IdExpr(ast.types.mangle("std.internal.static", func="has_rtti")),
                    items=[type_expr.clone()],
                ),
                if_suite=ast.ExprStmt(
                    ast.CallExpr(
                        ast.DotExpr(ast.CallExpr(ast.IdExpr("super")), member="__init__"),
                        items=[
                            ast.StarExpr(ast.IdExpr("_args")),
                            ast.KeywordStarExpr(expr=ast.IdExpr("_kwargs")),
                        ],
                    )
                ),
            )
        )
        for arg in args:
            fn_args.append(
                ast.Param(
                    arg.name,
                    type=None if arg.type is None else arg.type.clone(True),
                    default=None if arg.default is None else arg.default.clone(),
                )
            )
            stmts.append(
                ast.AssignStmt(
                    ast.DotExpr(ast.IdExpr("self"), member=arg.name),
                    rhs=ast.IdExpr(arg.name),
                )
            )
        fn_args.append(ast.Param("*_args"))
        fn_args.append(ast.Param("**_kwargs"))
        attrs.append(ast.Attr.HasSelf)
    elif magic in {"raw", "dict"}:
        # Classes: def __raw__(self: T)
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self")])))
    elif magic == "tuplesize":
        # def __tuplesize__() -> int
        ret = ast.IdExpr("int")
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic))))
    elif magic == "getitem":
        # Tuples: def __getitem__(self: T, index: int)
        fn_args.extend(
            [ast.Param("self", type=type_expr.clone()), ast.Param("index", type=ast.IdExpr("int"))]
        )
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("index")]))
        )
    elif magic == "iter":
        # Tuples: def __iter__(self: T)
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        stmts.append(ast.YieldFromStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self")])))
    elif magic == "contains":
        # Tuples: def __contains__(self: T, what) -> bool
        fn_args.extend([ast.Param("self", type=type_expr.clone()), ast.Param("what")])
        ret = ast.IdExpr(ast.types.Stdlib.Bool)
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("what")]))
        )
    elif magic in {"eq", "ne", "lt", "le", "gt", "ge"}:
        # def __op__(self: T, obj: T) -> bool
        fn_args.extend(
            [ast.Param("self", type=type_expr.clone()), ast.Param("obj", type=type_expr.clone())]
        )
        ret = ast.IdExpr(ast.types.Stdlib.Bool)
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("obj")]))
        )
    elif magic in {"hash", "len"}:
        # def __hash__(self: T) -> int
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        ret = ast.IdExpr("int")
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self")])))
    elif magic == "pickle":
        # def __pickle__(self: T, dest: Ptr[u8])
        fn_args.extend(
            [
                ast.Param("self", type=type_expr.clone()),
                ast.Param(
                    "dest",
                    type=ast.IndexExpr(ast.IdExpr(ast.types.Stdlib.Ptr), index=ast.IdExpr("u8")),
                ),
            ]
        )
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("dest")]))
        )
    elif magic in {"unpickle", "from_py"}:
        # def __unpickle__(src: Ptr[u8]) -> T
        fn_args.append(
            ast.Param(
                "src",
                type=ast.IndexExpr(ast.IdExpr(ast.types.Stdlib.Ptr), index=ast.IdExpr("u8")),
            )
        )
        ret = type_expr.clone()
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("src"), type_expr.clone()]))
        )
    elif magic == "to_py":
        # def __to_py__(self: T) -> Ptr[u8]
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        ret = ast.IndexExpr(ast.IdExpr(ast.types.Stdlib.Ptr), index=ast.IdExpr("u8"))
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self")])))
    elif magic == "to_gpu":
        # def __to_gpu__(self: T, cache) -> T
        fn_args.extend([ast.Param("self", type=type_expr.clone()), ast.Param("cache")])
        ret = type_expr.clone()
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("cache")]))
        )
    elif magic == "from_gpu":
        # def __from_gpu__(self: T, other: T)
        fn_args.extend(
            [ast.Param("self", type=type_expr.clone()), ast.Param("other", type=type_expr.clone())]
        )
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("other")]))
        )
    elif magic == "from_gpu_new":
        # def __from_gpu_new__(other: T) -> T
        fn_args.append(ast.Param("other", type=type_expr.clone()))
        ret = type_expr.clone()
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("other")])))
    elif magic in {"repr", "repr_default"}:
        # def __repr__(self: T) -> str
        # def __repr_default__(self: T) -> str
        fn_args.append(ast.Param("self", type=type_expr.clone()))
        ret = ast.IdExpr(ast.types.Stdlib.String)
        stmts.append(ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self")])))
    elif magic == "add":
        # def __add__(self: TypeVisitor, obj)
        fn_args.extend([ast.Param("self", type=type_expr.clone()), ast.Param("obj")])
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("obj")]))
        )
    elif magic == "mul":
        # def __mul__(self: TypeVisitor, i: Literal[int])
        fn_args.extend(
            [
                ast.Param("self", type=type_expr.clone()),
                ast.Param("i", type=ast.IndexExpr(ast.IdExpr("Literal"), index=ast.IdExpr("int"))),
            ]
        )
        stmts.append(
            ast.ReturnStmt(expr=ast.CallExpr(M(magic), [ast.IdExpr("self"), ast.IdExpr("i")]))
        )
    else:
        assert False, f"invalid magic {magic}"

    with ast.Node.creation_context(None):
        fn = ast.FunctionStmt(f"__{magic}__", ret=ret, items=fn_args, suite=ast.SuiteStmt(*stmts))
    for attribute in attrs:
        fn.set(attribute)
    return fn


def generate_kw_id(self: TypeVisitor, names: List[str] | None = None) -> int:
    if names:
        key = ";".join(names)
        # Each set of names generates different tuple (i.e., `KwArgs[foo, bar]` is not the
        # same as `KwArgs[bar, baz]`). Cache the names and use an integer for each name
        # combination.
        if key not in self.ctx.cache.generated_kw_tuples:
            self.ctx.cache.generated_tuple_names.append(names)
            self.ctx.cache.generated_kw_tuples[key] = len(self.ctx.cache.generated_kw_tuples) + 1
        return self.ctx.cache.generated_kw_tuples[key]
    return 0


def generate_tuple(ctx: TypeContext, count: int, generate_new: bool = True) -> ast.types.Class:
    if count > cache.MAX_TUPLE:
        raise TypecheckError(ctx.node_stack[-1], f"tuple too large ({count})")
    key = f"{ast.types.Stdlib.Tuple}.{count}"
    stdlib = utils.get_import_module(ctx, cache.STDLIB_IMPORT)
    value = stdlib.ctx.get(key)
    if not value:
        tuple_type = ast.types.Class(cache=ctx.cache, name=ast.types.Stdlib.Tuple)
        tuple_type.is_tuple = True
        cls_data = utils.get_class(ctx, tuple_type)
        assert cls_data is not None and count <= len(cls_data.fields), "tuple too large"
        for idx in range(count):
            field = cls_data.fields[idx]
            assert isinstance(field.type_expr, ast.IdExpr)
            tuple_type.generics.append(
                ast.types.Generic(
                    field.type_expr.value,
                    field.type,
                    field.type.require_link.id,
                    ast.types.Type.Behaviour.Runtime,
                )
            )
        value = stdlib.ctx.add_item(key, key, tuple_type)

    result = value.type.require_cls
    if generate_new and count not in ctx.cache.generated_tuples:
        ctx.cache.generated_tuples.add(count)
        new_args = []
        type_args = []
        for idx in range(count):
            new_args.append(ast.Param(f"item{idx + 1}", type=ast.IdExpr(f"T{idx + 1}")))
            type_args.append(ast.IdExpr(f"T{idx + 1}"))
        for idx in range(count):
            new_args.append(ast.Param(f"T{idx + 1}", type=ast.IdExpr(ast.types.Stdlib.Type)))
        fn = ast.FunctionStmt(
            "__new__",
            ret=ast.IndexExpr(ast.IdExpr(ast.types.Stdlib.Tuple), index=ast.TupleExpr(type_args)),
            items=new_args,
        )
        fn.set(ast.Attr.Internal)

        extension = ast.ClassStmt(ast.types.Stdlib.Tuple, suite=fn)
        extension.set(ast.Attr.Extend)
        extension.set(ast.Attr.AutoGenerated)
        extension = ast.SuiteStmt(extension)
        ctx.cache.scope(extension)

        stdlib.ctx.prepare_checkpoint()
        with (
            stdlib.ctx.substitute("bases", [stdlib.ctx.bases[0]]),
            stdlib.ctx.substitute("block_level", 0),
        ):
            from . import typecheck_node

            extension = typecheck_node(stdlib.ctx, extension, sweep_force_realize=False)
            ctx.preamble.add(extension)
    return result
