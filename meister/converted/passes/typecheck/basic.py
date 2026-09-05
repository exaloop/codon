# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ... import ast
from . import infer, utils

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_none(self: TypeVisitor, node: ast.NoneExpr):
    """Set type to `Optional[?]`"""
    node |= utils.instantiate_type(
        self.ctx, utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Optional)
    )
    if infer.realize(node.type):
        # Realize the appropriate `Optional.__new__` for the translation stage
        infer.realize(
            utils.instantiate_type(
                self.ctx,
                utils.force_find(
                    self.ctx, ast.types.mangle(cls="Optional", func="__new__")
                ).get_type(),
                [utils.extract_class_type(self.ctx, node)],
            )
        )
        node.done = True
    return node


def typecheck_bool(self: TypeVisitor, node: ast.BoolExpr):
    """Set type to `bool`"""
    node |= utils.instantiate_static(self.ctx, node.value)
    node.done = True
    return node


def typecheck_int(self: TypeVisitor, node: ast.IntExpr):
    """
    Parse various integer representations depending on the integer suffix.
    @example
    `123u`   -> `UInt[64](123)`
    `123i56` -> `Int[56](123)`
    `123pf`  -> `int.__suffix_pf__(123)`
    """
    value, suffix = node.get_raw_data()
    holder: ast.Expr | None = None
    if not node.has_stored_value():
        holder = ast.StringExpr(value)
        suffix = suffix or "i64"
    else:
        holder = ast.IntExpr(node.get_value())

    width = None
    if len(suffix) > 1 and suffix[0] in ("u", "i") and suffix[1:].isdigit():
        try:
            width = int(suffix[1:])
            if width > 10000:
                width = None
        except ValueError:
            pass

    if not suffix and node.has_stored_value():
        # A normal integer (int64_t)
        node |= utils.instantiate_static(self.ctx, node.get_value())
        node.done = True
        return node
    elif suffix == "u":
        # Unsigned integer: call `UInt[64](value)`
        width = 64
        call = ast.CallExpr(
            ast.IndexExpr(
                ast.IdExpr(ast.types.Stdlib.UInt),
                index=ast.IntExpr(width),
            ),
            items=[holder],
        )
        return self.visit(call)
    elif width:
        # Fixed-width numbers (with `uNNN` and `iNNN` suffixes):
        # call `UInt[NNN](value)` or `Int[NNN](value)`
        type_name = ast.types.Stdlib.UInt if suffix[0] == "u" else ast.types.Stdlib.Int
        call = ast.CallExpr(
            ast.IndexExpr(ast.IdExpr(type_name), index=ast.IntExpr(width)),
            items=[holder],
        )
        return self.visit(call)
    else:
        # Custom suffix: call `int.__suffix_[suffix]__(value)`
        call = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(value="int"), member=f"__suffix_{suffix}__"),
            items=[holder],
        )
        return self.visit(call)


def typecheck_float(self: TypeVisitor, node: ast.FloatExpr):
    """
    Parse various float representations depending on the suffix.
    @example
      `123.4pf` -> `float.__suffix_pf__(123.4)`
    """

    value, suffix = node.get_raw_data()
    if not node.has_stored_value():
        holder = ast.StringExpr(value)
    else:
        holder = ast.FloatExpr(node.get_value())
    if not suffix and node.has_stored_value():
        # A normal float (double)
        node |= utils.get_stdlib_type(self.ctx, ast.types.Stdlib.Float)
        node.done = True
        return node
    elif not suffix:
        member = "__new__"
        call = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Float), member=member),
            items=[holder],
        )
        return self.visit(call)
    else:
        # Custom suffix: call `float.__suffix_[suffix]__(value)`
        member = f"__suffix_{suffix}__"
        call = ast.CallExpr(
            ast.DotExpr(ast.IdExpr(ast.types.Stdlib.Float), member=member),
            items=[holder],
        )
        return self.visit(call)


def typecheck_str(self: TypeVisitor, node: ast.StringExpr):
    """
    Set type to `str`. Concatinate strings in list and apply appropriate transformations
    (e.g., `str` wrap).
    """
    if node.is_simple:
        node |= utils.instantiate_static(self.ctx, node.get_value())
        node.done = True
        return node

    items = []
    for part in node.strings:
        if expr := part.expr:
            conv = ""
            match part.format.conversion:
                case "r":
                    conv = "repr"
                case "s":
                    conv = "str"
                case "a":
                    conv = "ascii"
                case _:
                    pass
            if conv:
                expr = ast.CallExpr(ast.IdExpr(conv), items=[expr])
            if part.format.spec:
                expr = ast.CallExpr(
                    ast.DotExpr(expr, member="__format__"),
                    items=[ast.StringExpr(part.format.spec)],
                )
            expr = ast.CallExpr(ast.IdExpr("str"), items=[expr])
            if part.value:
                expr = ast.CallExpr(
                    ast.DotExpr(ast.IdExpr(ast.types.Stdlib.String), member="cat"),
                    items=[ast.StringExpr(part.value), expr],
                )
            items.append(expr)
        elif part.prefix:
            # Custom prefix strings:
            # call `str.__prefsix_[prefix]__(str, [static length of str])`
            items.append(
                ast.CallExpr(
                    ast.DotExpr(
                        ast.IdExpr(ast.types.Stdlib.String),
                        member=f"__prefix_{part.prefix}__",
                    ),
                    items=[
                        ast.StringExpr(part.value),
                        ast.IntExpr(len(part.value)),
                    ],
                )
            )
        else:
            items.append(ast.StringExpr(part.value))
    if len(items) == 1:
        return self.visit(items[0])
    else:
        return self.visit(
            ast.CallExpr(
                ast.DotExpr(ast.IdExpr(ast.types.Stdlib.String), member="cat"),
                items=items,
            )
        )
