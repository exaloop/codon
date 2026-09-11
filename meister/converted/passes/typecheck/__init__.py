"""Converted Codon parser package."""

from ....bridge import Dict, List, cast, dataclass
from ... import ast
from ...cache import Cache
from ...error import TypecheckError
from . import (
    access,
    assign,
    basic,
    call,
    classes,
    collections,
    cond,
    error,
    function,
    imports,
    infer,
    loops,
    ops,
    stmts,
    utils,
)
from .ctx import TypeContext


@dataclass(init=False)
class TypeVisitor(ast.NodeVisitor):
    """
    Visitor that infers expression types and performs type-guided transformations.

    Note: this stage *modifies* the provided AST. Clone it before simplification
    """

    ctx: TypeContext

    def __init__(self, ctx: TypeContext):
        self.ctx = ctx

    def visit_stmt(self, node: ast.Stmt) -> ast.Stmt:
        if node.done:
            return node

        self.ctx.node_stack.append(node)
        self.ctx.prepend_stmts.append([])
        with self.ctx.substitute("time", node.get(ast.Attr.ExprTime, 0)):
            transformed = cast(ast.Stmt, self.visit(node))
        self.ctx.node_stack.pop()
        prepended = self.ctx.prepend_stmts.pop()
        node = transformed
        assert isinstance(node, ast.Stmt)
        if prepended:
            prepended.append(node)
            node = ast.SuiteStmt(*prepended, done=all(s.done for s in prepended))
        if node.done:
            self.ctx.changed_nodes += 1
        return node

    def visit_expr(
        self,
        node: ast.Expr,
        type_allowed: bool = True,
        enforce_type: bool = False,
        simple_types: bool = False,
    ) -> ast.Expr:
        if not node.type:
            node.type = utils.instantiate_unbound(self.ctx, node.info)
        if enforce_type:
            if isinstance(node, ast.NoneExpr):
                node = ast.IdExpr(ast.types.Stdlib.NoneType, info=node.info)
            with self.ctx.substitute("simple_types", simple_types):
                node = cast(ast.Expr, self.visit(node))
            assert isinstance(node, ast.Expr) and node.type
            if node.type.static_kind is not ast.types.Type.Behaviour.Runtime:
                pass
            elif utils.is_type_expr(node):
                node.type = utils.instantiate(self.ctx, node.type)
            elif (u := node.type.unbound) and (not u.generic_name or u.trait):
                node.type = utils.instantiate(self.ctx, node.type)
            else:
                raise TypecheckError(node, "expected a type expression")
        else:
            if not node.done:
                self.ctx.node_stack.append(node)
                transformed = self.visit(node)
                assert isinstance(transformed, ast.Expr) and transformed.type
                self.ctx.node_stack.pop()
                if transformed is not node:
                    for attr, value in node.attributes.items():
                        transformed.attributes.setdefault(attr, value)
                    transformed.orig = node.orig or node
                node = transformed
                node.type = node.type or utils.instantiate_unbound(self.ctx, node.info)
                if not type_allowed and utils.is_type_expr(node):
                    raise TypecheckError(node, "unexpected type; expected a value")
                if node.done:
                    self.ctx.changed_nodes += 1
            if not node.has(ast.Attr.ExprDoNotRealize):
                if realized := infer.realize(self.ctx, node.type):
                    node.type |= realized
        return node

    def visit_AssertStmt(self, node: ast.AssertStmt):
        return error.typecheck_assert(self, node)

    def visit_TryStmt(self, node: ast.TryStmt):
        return error.typecheck_try(self, node)

    def visit_ThrowStmt(self, node: ast.ThrowStmt):
        return error.typecheck_throw(self, node)

    def visit_WithStmt(self, node: ast.WithStmt):
        return error.typecheck_with(self, node)

    def visit_IdExpr(self, node: ast.IdExpr):
        return access.typecheck_id(self, node)

    def visit_DotExpr(self, node: ast.DotExpr):
        return access.typecheck_dot(self, node)

    def visit_ImportStmt(self, node: ast.ImportStmt):
        return imports.typecheck_import(self, node)

    def visit_RangeExpr(self, node: ast.RangeExpr):
        return cond.typecheck_range(self, node)

    def visit_IfExpr(self, node: ast.IfExpr):
        return cond.typecheck_ifexpr(self, node)

    def visit_IfStmt(self, node: ast.IfStmt):
        return cond.typecheck_if(self, node)

    def visit_MatchStmt(self, node: ast.MatchStmt):
        return cond.typecheck_match(self, node)

    def visit_BreakStmt(self, node: ast.BreakStmt):
        return loops.typecheck_break(self, node)

    def visit_ContinueStmt(self, node: ast.ContinueStmt):
        return loops.typecheck_continue(self, node)

    def visit_WhileStmt(self, node: ast.WhileStmt):
        return loops.typecheck_while(self, node)

    def visit_ForStmt(self, node: ast.ForStmt):
        return loops.typecheck_for(self, node)

    def visit_UnaryExpr(self, node: ast.UnaryExpr):
        return ops.typecheck_unary(self, node)

    def visit_BinaryExpr(self, node: ast.BinaryExpr):
        return ops.typecheck_binary(self, node)

    def visit_ChainBinaryExpr(self, node: ast.ChainBinaryExpr):
        return ops.typecheck_chainbinary(self, node)

    def visit_PipeExpr(self, node: ast.PipeExpr):
        return ops.typecheck_pipe(self, node)

    def visit_IndexExpr(self, node: ast.IndexExpr):
        return ops.typecheck_index(self, node)

    def visit_InstantiateExpr(self, node: ast.InstantiateExpr):
        return ops.typecheck_instantiate(self, node)

    def visit_SliceExpr(self, node: ast.SliceExpr):
        return ops.typecheck_slice(self, node)

    def visit_NoneExpr(self, node: ast.NoneExpr):
        return basic.typecheck_none(self, node)

    def visit_BoolExpr(self, node: ast.BoolExpr):
        return basic.typecheck_bool(self, node)

    def visit_IntExpr(self, node: ast.IntExpr):
        return basic.typecheck_int(self, node)

    def visit_FloatExpr(self, node: ast.FloatExpr):
        return basic.typecheck_float(self, node)

    def visit_StringExpr(self, node: ast.StringExpr):
        return basic.typecheck_str(self, node)

    def visit_TupleExpr(self, node: ast.TupleExpr):
        return collections.typecheck_tuple(self, node)

    def visit_ListExpr(self, node: ast.ListExpr):
        return collections.typecheck_list(self, node)

    def visit_SetExpr(self, node: ast.SetExpr):
        return collections.typecheck_set(self, node)

    def visit_DictExpr(self, node: ast.DictExpr):
        return collections.typecheck_dict(self, node)

    def visit_GeneratorExpr(self, node: ast.GeneratorExpr):
        return collections.typecheck_generator(self, node)

    def visit_StmtExpr(self, node: ast.StmtExpr):
        return stmts.typecheck_stmt(self, node)

    def visit_SuiteStmt(self, node: ast.SuiteStmt):
        return stmts.typecheck_suite(self, node)

    def visit_ExprStmt(self, node: ast.ExprStmt):
        return stmts.typecheck_expr(self, node)

    def visit_CustomStmt(self, node: ast.CustomStmt):
        return stmts.typecheck_custom(self, node)

    def visit_CommentStmt(self, node: ast.CommentStmt):
        return stmts.typecheck_comment(self, node)

    def visit_DirectiveStmt(self, node: ast.DirectiveStmt):
        return stmts.typecheck_directive(self, node)

    def visit_AssignExpr(self, node: ast.AssignExpr):
        return assign.typecheck_assignexpr(self, node)

    def visit_AssignStmt(self, node: ast.AssignStmt):
        return assign.typecheck_assign(self, node)

    def visit_DelStmt(self, node: ast.DelStmt):
        return assign.typecheck_del(self, node)

    def visit_AssignMemberStmt(self, node: ast.AssignMemberStmt):
        return assign.typecheck_assignmember(self, node)

    def visit_ClassStmt(self, node: ast.ClassStmt):
        return classes.typecheck_class(self, node)

    def visit_PrintStmt(self, node: ast.PrintStmt):
        return call.typecheck_print(self, node)

    def visit_StarExpr(self, node: ast.StarExpr):
        return call.typecheck_star(self, node)

    def visit_KeywordStarExpr(self, node: ast.KeywordStarExpr):
        return call.typecheck_keywordstar(self, node)

    def visit_EllipsisExpr(self, node: ast.EllipsisExpr):
        return call.typecheck_ellipsis(self, node)

    def visit_CallExpr(self, node: ast.CallExpr):
        return call.typecheck_call(self, node)

    def visit_LambdaExpr(self, node: ast.LambdaExpr):
        return function.typecheck_lambda(self, node)

    def visit_YieldExpr(self, node: ast.YieldExpr):
        return function.typecheck_yieldexpr(self, node)

    def visit_AwaitExpr(self, node: ast.AwaitExpr):
        return function.typecheck_await(self, node)

    def visit_ReturnStmt(self, node: ast.ReturnStmt):
        return function.typecheck_return(self, node)

    def visit_YieldStmt(self, node: ast.YieldStmt):
        return function.typecheck_yield(self, node)

    def visit_YieldFromStmt(self, node: ast.YieldFromStmt):
        return function.typecheck_yieldfrom(self, node)

    def visit_GlobalStmt(self, node: ast.GlobalStmt):
        return function.typecheck_global(self, node)

    def visit_FunctionStmt(self, node: ast.FunctionStmt):
        return function.typecheck_function(self, node)

    def log(self, prefix: str, file: str = "", *args: object):
        if file and file not in self.ctx.info.file:
            return
        iteration = 0 if self.ctx.base is None else self.ctx.base.iteration
        print(f"[{self.ctx.info}] [{self.ctx.base_name}${iteration}]: {prefix} {args}")


def typecheck_program(
    cache: Cache,
    node: ast.Stmt,
    file: str = "<internal>",
    defines: Dict[str, str] | None = None,
    early_defines: Dict[str, str] | None = None,
    barebones: bool = False,
) -> ast.Stmt:
    """
    Simplify an AST node. Load standard library if needed.
    @param cache     Pointer to the shared cache ( @c Cache )
    @param file      Filename to be used for error reporting
    @param barebones Use the bare-bones standard library for faster testing
    @param defines   User-defined static values (typically passed as `codon run -DX=Y`).
    Each value is passed as a string.
    Simplify an AST node. Assumes that the standard library is loaded.
    """

    from ...cache import MAIN_IMPORT, MODULE_MAIN, STDLIB_IMPORT, Import
    from . import infer, special, utils

    assert cache.module is not None, "cache's module is not set"

    preamble = ast.SuiteStmt()

    # Load standard library if it has not been loaded
    if STDLIB_IMPORT not in cache.imports:
        load_std_library(cache, preamble, early_defines or {}, barebones)

    # Set up the context and the cache
    type_ctx = TypeContext(filename=file, cache=cache)
    cache.imports[MAIN_IMPORT] = cache.imports.setdefault(file, Import(MAIN_IMPORT, file, type_ctx))
    type_ctx.filename = file
    type_ctx.module = Import.File(Import.File.Status.External, file, MODULE_MAIN)

    # Prepare the code
    stmts: List[ast.Stmt] = []
    # Load compile-time defines (e.g., codon run -DFOO=1 ...)
    for name, value in (defines or {}).items():
        if value.startswith("str:"):
            defined_value = ast.StringExpr(value[4:])
            literal_name = "str"
        elif value.startswith("bool:"):
            defined_value = ast.BoolExpr(value == "bool:True")
            literal_name = "bool"
        else:
            defined_value = ast.IntExpr(value.removeprefix("int:"))
            literal_name = "int"
        stmts.append(
            ast.AssignStmt(
                ast.IdExpr(name),
                rhs=defined_value,
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), index=ast.IdExpr(literal_name)),
            )
        )
    # Set up __name__
    stmts.append(ast.AssignStmt(ast.IdExpr("__name__"), rhs=ast.StringExpr(MODULE_MAIN)))
    stmts.append(ast.AssignStmt(ast.IdExpr("__file__"), rhs=ast.StringExpr(file)))
    stmts.append(node)
    suite = ast.SuiteStmt(*stmts)

    cache.scope(suite, type_ctx.global_shadows)
    with type_ctx.substitute("preamble", preamble):
        inferred = infer.infer_types(type_ctx, suite, True)
        if not inferred:
            raise TypecheckError(trace=utils.find_typecheck_errors(type_ctx, suite))
        result = ast.SuiteStmt(preamble, inferred)
        if isinstance(inferred, ast.SuiteStmt):
            special.prepare_vtables(type_ctx)
    return result


def load_std_library(
    cache: Cache,
    preamble: ast.SuiteStmt,
    early_defines: Dict[str, str],
    barebones: bool,
):
    from ...cache import (
        STDLIB_IMPORT,
        STDLIB_INTERNAL_MODULE,
        VAR_CLASS_TOPLEVEL,
        ClassData,
        Import,
    )

    # Load the internal.__init__
    stdlib = TypeContext(filename=STDLIB_IMPORT, cache=cache)
    stdlib_path = cache.get_import_file(STDLIB_INTERNAL_MODULE, "", True)
    initial_file = "__init__.codon"
    if stdlib_path is None or not stdlib_path.path.endswith(initial_file):
        raise FileNotFoundError("standard library cannot be found")

    # Use __init_test__ for faster testing (e.g., #%% name,barebones)
    # TODO: get rid of it one day...
    if barebones:
        stdlib_path.path.removesuffix("__init__.codon")
        stdlib_path.path += "__init_test__.codon"
    stdlib.filename = stdlib_path.path
    cache.imports[STDLIB_IMPORT] = cache.imports.setdefault(
        stdlib_path.path, Import(STDLIB_IMPORT, stdlib_path.path, stdlib)
    )

    # Load the standard library
    stdlib.is_stdlib_loading = True
    stdlib.module = Import.File(Import.File.Status.StdLibrary, stdlib_path.path, "__init__")
    stdlib.preamble = preamble

    # 1. Core definitions
    cache.classes[VAR_CLASS_TOPLEVEL] = ClassData()
    core = cache.parse(code="from internal.core import *")
    assert isinstance(core, ast.Stmt)
    cache.scope(core)
    if core := infer.infer_types(stdlib, core, True):
        preamble.items.append(core)

    # 2. Load early compile-time defines (for standard library)
    for name, value in early_defines.items():
        if value.startswith("str:"):
            value = ast.StringExpr(value[4:])
            type_name = "str"
        elif value.startswith("bool:"):
            value = ast.BoolExpr(value == "bool:True")
            type_name = "bool"
        else:
            value = ast.IntExpr(value.removeprefix("int:"))
            type_name = "int"
        assign = cache.typecheck(
            ast.AssignStmt(
                ast.IdExpr(name),
                rhs=value,
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), ast.IdExpr(type_name)),
            ),
            ctx=stdlib,
        )
        preamble.items.append(cast(ast.Stmt, assign))

    # 3. Load stdlib
    node = cache.parse(file=stdlib_path.path)
    assert isinstance(node, ast.Stmt)
    cache.scope(node)
    if node := infer.infer_types(stdlib, node, True):
        preamble.items.append(node)
    stdlib.is_stdlib_loading = False


def typecheck_node(ctx: TypeContext, node: ast.Stmt, file: str = "<internal>") -> ast.Stmt | None:
    with ctx.substitute("filename", file):
        preamble = ast.SuiteStmt()
        if inferred := infer.infer_types(ctx, node, is_toplevel=True):
            return ast.SuiteStmt(preamble, inferred)
        raise TypecheckError(trace=utils.find_typecheck_errors(ctx, node))
    return None
