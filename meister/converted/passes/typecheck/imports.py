# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from typing import TYPE_CHECKING

from ....bridge import List
from ... import ast, cache
from ...error import TypecheckError
from . import infer, utils
from .ctx import TypeContext

if TYPE_CHECKING:
    from . import TypeVisitor


def typecheck_import(self: TypeVisitor, node: ast.ImportStmt) -> ast.Stmt:
    """
    Import and parse a new module into its own context.
    Also handle special imports ( see @c transformSpecialImport ).
    To simulate Python's dynamic import logic and import stuff only once,
    each import statement is guarded as follows:
    if not _import_N_done:
    _import_N()
    _import_N_done = True
    See @c transformNewImport and below for more details.
    """

    assert not self.ctx.in_class
    # Transform special `from C` and `from python` imports.
    match node:
        case ast.ImportStmt(from_expr=ast.IdExpr(value="C"), what=ast.IdExpr(value=value)):
            if node.is_c_var():
                # C variable imports
                assert node.ret
                return transform_c_var_import(self, value, node.ret, node.as_)
            else:
                # C function imports
                return transform_c_import(self, value, node.args, node.ret, node.as_)
        case ast.ImportStmt(from_expr=ast.IdExpr(value="C"), what=ast.DotExpr() as what):
            # dylib C imports
            return transform_cdll_import(
                self, what.expr, what.member, node.args, node.ret, node.as_, not node.is_c_var()
            )
        case ast.ImportStmt(from_expr=ast.IdExpr(value="python")):
            return transform_python_import(self, node.what, node.args, node.ret, node.as_)
        case _:
            pass

    def code(node: ast.Expr):
        match node:
            case ast.IdExpr(value=value):
                return value
            case ast.DotExpr(expr=expr, member=member):
                return f"{code(expr)}.{member}"
            case _:
                raise TypecheckError(node, f"unexpected import expression {node}")

    # Fetch the import
    components = get_import_path(node.from_expr, node.dots)
    path = "/".join(components)
    # from "." case
    if node.dots == 1 and not path:
        assert isinstance(node.what, ast.IdExpr), f"not an identifier: {node.what}"
        return self.visit_stmt(ast.ImportStmt(node.what, dots=1))
    import_file = self.ctx.cache.get_import_file(path, self.ctx.filename)
    if not import_file:
        if node.dots == 0 and self.ctx.auto_python:
            assert node.from_expr
            python_name = code(node.from_expr)
            if node.what:
                python_name += "." + code(node.what)
            name_expr = self.ctx.cache.parse(expr=python_name)
            assert isinstance(name_expr, ast.Expr)
            python_import = ast.ImportStmt(
                name_expr,
                ast.IdExpr("python"),
                args=node.args,
                ret=node.ret,
                as_=node.as_,
            )
            return self.visit_stmt(python_import)
        display_name = "." * node.dots
        for component in components:
            if component == "..":
                continue
            if display_name and display_name[-1] != ".":
                display_name += f".{component}"
            else:
                display_name += component
        if (
            display_name
            and all(c == "." for c in display_name)
            and isinstance(node.what, ast.IdExpr)
        ):
            display_name = node.what.value
        raise TypecheckError(node, f"no module named '{display_name}'")

    # If the file has not been seen before, load it into cache
    handled = True
    result = None
    if import_file.path not in self.ctx.cache.imports:
        if not (result := transform_new_import(self, import_file)):
            # we need an import
            handled = False
    imported = utils.get_import_module(self.ctx, import_file.path)
    import_var = imported.import_var
    if not imported.loaded_at_toplevel:
        handled = False

    # Construct `if _import_done.__invert__(): (_import(); _import_done = True)`.
    # Do not do this during the standard library loading (we assume that standard library
    # imports are "clean" and do not need guards). Note that the importVar is empty if
    # the import has been loaded during the standard library loading.
    if not handled:
        result = ast.ExprStmt(
            ast.CallExpr(ast.IdExpr(ast.types.mangle(func=f"{import_var}_call", no_core=True)))
        )

    # Import requested identifiers from the import's scope to the current scope
    match node.what:
        case None:  # import foo
            name = path if not node.as_ else node.as_
            imported_item = self.ctx.force_find(import_var)
            self.ctx.add(name, imported_item)
        case ast.IdExpr("*"):  # from foo import *
            assert not node.as_
            # Just copy all symbols from import's context here.
            for name, value in imported.ctx:
                if not name.startswith("_") or (
                    self.ctx.is_stdlib_loading and name.startswith("__")
                ):
                    # Ignore all identifiers that start with `_` but not those that start with
                    # `__` while the standard library is being loaded
                    imported_item = value[0]
                    if imported_item.is_conditional() and "." not in name:
                        if replacement := imported.ctx.find(name):
                            imported_item = replacement
                    # Imports should ignore noShadow property
                    self.ctx.add(name, imported_item)
        case _:  # from foo import bar
            assert isinstance(node.what, ast.IdExpr), "not a valid import what expression"
            # Make sure that we are importing an existing global symbol
            if not (found := imported.ctx.get(node.what.value)):
                raise TypecheckError(
                    node.what,
                    f"cannot import name '{node.what.value}' from '{import_file.module}'",
                )
            if found.is_conditional():
                if replacement := imported.ctx.get(node.what.value):
                    found = replacement
            # Imports should ignore noShadow property
            self.ctx.add(node.what.value if not node.as_ else node.as_, found)
    result = self.visit_stmt(ast.SuiteStmt() if not result else result)  # erase the statement
    return result


def get_import_path(from_expr: ast.Expr | None, dots: int = 0) -> List[str]:
    """
    Transform Dot(Dot(a, b), c...) into "{a, b, c, ...}".
    Useful for getting import paths.
    """

    # Path components
    components = []
    current = from_expr
    if current:
        while isinstance(current, ast.DotExpr):
            components.append(current.member)
            current = current.expr
        assert isinstance(current, ast.IdExpr), "invalid import statement"
        components.append(current.value)
    # Handle dots (i.e., `..` in `from ..m import x`)
    for _ in range(1, dots):
        components.append("..")
    components.reverse()
    return components


def transform_c_import(
    self: TypeVisitor,
    name: str,
    args: List[ast.Param],
    ret: ast.Expr | None,
    alt_name: str,
) -> ast.Stmt:
    """
    Transform a C function import.
    @example
    `from C import foo(int) -> float as f` ->
    ```@.c
    def foo(a1: int) -> float:
    pass
    f = foo # if altName is provided```
    No return type implies void return type. *args is treated as C VAR_ARGS.
    """

    function_args = []
    has_var_args = False
    for index, argument in enumerate(args):
        assert not argument.name, "unexpected argument name"
        assert not argument.default, "unexpected default argument"
        assert argument.type, "missing type"
        if isinstance(argument.type, ast.EllipsisExpr) and index + 1 == len(args):
            # C VAR_ARGS support
            has_var_args = True
            function_args.append(ast.Param("*args"))
        else:
            cloned_type = argument.type.clone()
            function_args.append(ast.Param(f"a{index}", type=cloned_type))
    # avoid canonicalName == name
    self.ctx.generate_canonical_name(name)
    if not ret:
        ret_type = ast.IdExpr(ast.types.Stdlib.NoneType)
    else:
        ret_clone = ret.clone()
        ret_type = ret_clone
    function = ast.FunctionStmt(name, ret=ret_type, items=function_args)
    function.set(ast.Attr.C)
    if has_var_args:
        function.set(ast.Attr.CVarArg)
    # Already in the preamble
    result = self.visit_stmt(function)
    if alt_name:
        self.ctx.add(alt_name, self.ctx[name])
        self.ctx.remove(name)
    return result


def transform_c_var_import(
    self: TypeVisitor,
    name: str,
    type_expr: ast.Expr,
    alt_name: str,
) -> ast.Stmt:
    """
    Transform a C variable import.
    @example
    `from C import foo: int as f` ->
    ```f: int = "foo"```
    """
    canonical = self.ctx.generate_canonical_name(name)
    type_expr = self.visit_expr(type_expr.clone(), enforce_type=True)
    linked_type = ast.types.Link(
        cache=self.ctx.cache,
        kind=ast.types.Link.Kind.Link,
        type=utils.extract_class_type(self.ctx, type_expr),
    )
    value = self.ctx.add_item(
        name if not alt_name else alt_name, canonical, linked_type, self.ctx.time
    )
    lhs = ast.IdExpr(canonical, type=value.type, done=True)
    lhs.set(ast.Attr.ExprExternVar)
    assignment = ast.AssignStmt(lhs, type_expr=type_expr, done=True)
    return assignment


def transform_cdll_import(
    self: TypeVisitor,
    dylib: ast.Expr,
    name: str,
    args: List[ast.Param],
    ret: ast.Expr | None,
    alt_name: str,
    is_function: bool,
) -> ast.Stmt:
    """
    Transform a dynamic C import.
    @example
    `from C import lib.foo(int) -> float as f` ->
    `f = _dlsym(lib, "foo", Fn=Function[[int], float]); f`
    No return type implies void return type.
    """
    if is_function:
        argument_types = ast.ListExpr()
        if not ret:
            return_type = ast.IdExpr(ast.types.Stdlib.NoneType)
        else:
            return_type = ret.clone()
        for argument in args:
            assert not argument.name, "unexpected argument name"
            assert not argument.default, "unexpected default argument"
            assert argument.type, "missing type"
            argument_types.items.append(argument.type.clone())
        type_expr = ast.IndexExpr(
            ast.IdExpr(ast.types.Stdlib.Function),
            index=ast.TupleExpr([argument_types, return_type]),
        )
    else:
        assert ret
        type_expr = ret.clone()
    call = ast.CallExpr(
        ast.IdExpr("_dlsym"),
        items=[dylib.clone(), ast.StringExpr(name), ast.CallExpr.Arg(type_expr, name="Fn")],
    )
    assignment = ast.AssignStmt(ast.IdExpr(alt_name or name), rhs=call)
    return self.visit_stmt(assignment)


def transform_python_import(
    self: TypeVisitor,
    what: ast.Expr,
    args: List[ast.Param],
    ret: ast.Expr | None,
    alt_name: str,
) -> ast.Stmt:
    """
    Transform a Python module and function imports.
    @example
    `from python import module as f` -> `f = pyobj._import("module")`
    `from python import lib.foo(int) -> float as f` ->
    ```def f(a0: int) -> float:
    f = pyobj._import("lib")._getattr("foo")
    return float.__from_py__(f(a0))```
    If a return type is nullptr, the function just returns f (raw pyobj).
    """

    # Get a module name (e.g., os.path)
    components = get_import_path(what)
    assert components

    # Simple import: `from python import foo.bar` -> `bar = pyobj._import("foo.bar")`
    if not ret and not args:
        call = ast.CallExpr(
            ast.DotExpr(ast.IdExpr("pyobj"), member="_import"),
            items=[ast.StringExpr(".".join(components))],
        )
        assignment = ast.AssignStmt(ast.IdExpr(alt_name or components[-1]), rhs=call)
        return self.visit_stmt(assignment)

    # Python function import:
    # `from python import foo.bar(int) -> float` ->
    # ```def bar(a1: int) -> float:
    # f = pyobj._import("foo")._getattr("bar")
    # return float.__from_py__(f(a1))```

    # f = pyobj._import("foo")._getattr("bar")
    module_import = ast.CallExpr(
        ast.DotExpr(ast.IdExpr("pyobj"), member="_import"),
        items=[ast.StringExpr(".".join(components[:-1]))],
    )
    getattr_call = ast.CallExpr(
        ast.DotExpr(module_import, member="_getattr"), items=[ast.StringExpr(components[-1])]
    )
    local_fn = ast.AssignStmt(ast.IdExpr("f"), rhs=getattr_call)

    # Arguments: f(a1, ...)
    fn_params, call_args = [], []
    for index, argument in enumerate(args):
        fn_params.append(
            ast.Param(f"a{index}", type=argument.type.clone() if argument.type else None)
        )
        call_args.append(ast.IdExpr(f"a{index}"))
    # `return ret.__from_py__(f(a1, ...))`
    if ret and not isinstance(ret, ast.NoneExpr):
        return_type = ret.clone()
    else:
        return_type = ast.IdExpr(ast.types.Stdlib.NoneType)
    python_call = ast.CallExpr(ast.IdExpr("f"), items=call_args)
    ret_expr = ast.CallExpr(
        ast.DotExpr(return_type.clone(), member="__from_py__"),
        items=[ast.DotExpr(python_call, member="p")],
    )
    # Create a function
    fn = ast.FunctionStmt(
        alt_name or components[-1],
        ret=return_type,
        items=fn_params,
        suite=ast.SuiteStmt(local_fn, ast.ReturnStmt(expr=ret_expr)),
    )
    return self.visit_stmt(fn)


def transform_new_import(self: TypeVisitor, file: cache.Import.File) -> ast.Stmt | None:
    """
    Import a new file into its own context and wrap its top-level statements into a
    function to support Python-like runtime import loading.
    @example
    ```_import_[I]_done = False
    def _import_[I]():
    global [imported global variables]...
    __name__ = [I]
    [imported top-level statements]```
    """

    # Use a clean context to parse a new file
    module_id = file.module.replace(".", "_")
    ctx = TypeContext(
        filename=file.path,
        cache=self.ctx.cache,
        is_stdlib_loading=self.ctx.is_stdlib_loading,
        module=file,
    )
    imported = self.ctx.cache.imports.setdefault(
        file.path, cache.Import(file.module, file.path, ctx)
    )
    current_module = self.ctx.cache.imports.get(self.ctx.module.path)
    parent_loaded = True if not current_module else current_module.loaded_at_toplevel
    imported.loaded_at_toplevel = parent_loaded and (
        self.ctx.is_stdlib_loading or (self.ctx.is_global and self.ctx.block_level == 0)
    )
    var = utils.get_temporary_var(self.ctx, f"import_{module_id}")
    imported.import_var = var

    # __name__ = [import name]
    initial = None
    if file.module != "internal.core":
        # str is not defined when loading internal.core; __name__ is not needed anyway
        initial = ast.SuiteStmt(
            ast.AssignStmt(ast.IdExpr("__name__"), ast.StringExpr(ctx.module.module)),
            ast.AssignStmt(ast.IdExpr("__file__"), ast.StringExpr(ctx.module.path)),
        )
        self.ctx.add_block()
        try:
            import_ctr = ast.CallExpr(
                ast.IdExpr("Import.__new__"),
                items=[ast.BoolExpr(False), ast.StringExpr(file.path), ast.StringExpr(file.module)],
            )
            import_assignment = ast.AssignStmt(
                ast.IdExpr(var), rhs=import_ctr, type_expr=ast.IdExpr("Import")
            )
            import_assignment = self.visit(import_assignment)
            self.ctx.preamble.add(import_assignment)
            value = self.ctx[var]
            value.block_level = 0
            value.base = ""
            value.module = cache.MODULE_MAIN
            value.time = 0
        finally:
            self.ctx.pop_block()
        stdlib_module = utils.get_import_module(self.ctx, cache.STDLIB_IMPORT)
        stdlib_module.ctx.add_toplevel(var, value)
        utils.register_global(self.ctx, value.canonical)

    parsed = self.ctx.cache.parse(file=file.path)
    assert isinstance(parsed, ast.Stmt)
    suite = ast.SuiteStmt(initial, parsed)
    self.ctx.cache.scope(suite, ctx.global_shadows, dominate_all=not self.ctx.is_stdlib_loading)

    # Add comment to the top of import for easier dump inspection
    suite = ast.SuiteStmt(ast.CommentStmt(f"import: {file.module} at {file.path}"), suite)
    if self.ctx.is_stdlib_loading:
        # When loading the standard library, imports are not wrapped.
        # We assume that the standard library has no recursive imports and that all
        # statements are executed before the user-provided code.
        with ctx.substitute("preamble", self.ctx.preamble):
            return ctx.cache.typecheck(suite, ctx)

    # Generate import identifier
    internal_return = ast.ReturnStmt()
    internal_return.set(ast.Attr.Internal)  # do not trigger toplevel ReturnStmt error
    stmts = ast.SuiteStmt(
        ast.IfStmt(ast.DotExpr(ast.IdExpr(var), member="loaded"), if_suite=internal_return),
        ast.ExprStmt(
            ast.CallExpr(
                ast.IdExpr("Import._set_loaded"),
                items=[ast.CallExpr(ast.IdExpr("__ptr__"), items=[ast.IdExpr(var)])],
            )
        ),
        suite,
    )
    # Wrap all imported top-level statements into a function.
    fn_name = f"{var}_call"
    fn = ast.FunctionStmt(fn_name, ret=ast.IdExpr(ast.types.Stdlib.NoneType), suite=stmts)
    with ctx.substitute("preamble", self.ctx.preamble):
        transformed = ctx.cache.typecheck(fn, ctx)
        infer.realize(ctx, ctx[fn_name].type)
        self.ctx.preamble.add(transformed)

    return None
