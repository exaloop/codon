"""Converted Codon parser package."""

from ....bridge import Dict, List, dataclass
from ... import ast, cache
from . import (
    # access,
    # assign,
    basic,
    # call,
    # classes,
    collections,
    cond,
    ctx,
    error,
    # function,
    # imports,
    infer,
    loops,
    # ops,
    stmts,
    utils,
)


@dataclass(init=False)
class TypeVisitor(ast.NodeVisitor):
    """
    Visitor that infers expression types and performs type-guided transformations.

    Note: this stage *modifies* the provided AST. Clone it before simplification
    """

    ctx: ctx.TypeContext
    # Statements to prepend before the current statement.
    prepend_stmts: List[ast.Stmt]
    preamble: ast.SuiteStmt | None = None

    def __init__(
        self,
        ctx: ctx.TypeContext | None = None,
        prepend_stmts: List[ast.Stmt] | None = None,
        preamble: ast.SuiteStmt | None = None,
    ):
        self.ctx = ctx.TypeContext() if ctx is None else ctx
        self.prepend_stmts = [] if prepend_stmts is None else prepend_stmts
        self.preamble = preamble or ast.SuiteStmt()

    def visit(
        self,
        node: ast.Node | None,
        type_allowed: bool = True,
        enforce_type: bool = False,
        simple_types: bool = False,
    ):
        if node is None:
            return None

        if enforce_type:
            if isinstance(node, ast.NoneExpr):
                node = ast.IdExpr(ast.types.Stdlib.NoneType, info=node.info)
            with self.ctx.substitute("simple_types", simple_types):
                result = self.visit(node)
                if not result:
                    return node
                node = result
            if node.type.get_static_kind() is not ast.types.Type.Behaviour.Runtime:
                pass
            elif utils.is_type_expr(node):
                node.type = self.instantiate_type(node.type)
            elif node.type.get_unbound() and not node.type.get_unbound().generic_name:
                node.type = self.instantiate_type(node.type)
            elif node.type.get_unbound() and node.type.get_unbound().trait:
                node.type = self.instantiate_type(node.type)
            else:
                raise ctx.TypecheckError(node, "expected a type expression")
        elif isinstance(node, ast.Expr):
            if not isinstance(node.type, ast.types.Type):
                node.type = utils.instantiate_unbound(node.info)
            if not node.done:
                self.ctx.push_node(node)
                transformed = super().visit(node)
                self.ctx.pop_node()
                if transformed is not node:
                    for attr, value in node.attributes.items():
                        transformed.attributes.setdefault(attr, value)
                    transformed.orig_expr = node.orig_expr or node
                node = transformed
                if not isinstance(node.type, ast.types.Type):
                    node.type = utils.instantiate_unbound(node.info)
                if not ctx.allow_types and utils.is_type_expr(node):
                    raise ctx.TypecheckError(node, "unexpected type; expected a value")
                if node.done:
                    self.ctx.changed_nodes += 1
            if not node.has(ast.Attr.ExprDoNotRealize):
                if realized := infer.realize(node.type):
                    node |= realized
        elif isinstance(node, ast.Stmt):
            if node.done:
                return node
            prepend_start = len(self.prepend_stmts)

            self.ctx.push_node(node)
            with self.ctx.substitute("time", node.get(ast.Attr.ExprTime, 0)):
                transformed = super().visit()
            self.ctx.pop_node()
            node = transformed

            if prepended := self.prepend_stmts[prepend_start:]:
                del self.prepend_stmts[prepend_start:]
                if node:
                    prepended.append(node)
                node = ast.SuiteStmt(prepended, done=all(s.done for s in prepended))
            if node.done:
                self.ctx.changed_nodes += 1
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
        return basic.typecheck_string(self, node)

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
        return assign.typecheck_assign(self, node)

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
        return function.typecheck_yield(self, node)

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
        if file and file not in ctx.get_info().file:
            return
        base = self.ctx.get_base()
        iteration = 0 if base is None else base.iteration
        print(f"[{self.ctx.get_info()}] [{self.ctx.get_base_name()}${iteration}]: {prefix} {args}")

    def set_info(self, source: ast.Node.SrcInfo):
        self.ctx.get_last_node().info = source


def typecheck_program(
    cache: cache.Cache,
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

    preamble = ast.SuiteStmt()
    assert cache.module is not None, "cache's module is not set"

    # Load standard library if it has not been loaded
    if cache.STDLIB_IMPORT not in cache.imports:
        load_std_library(cache, preamble, early_defines, barebones)

    # Set up the context and the cache
    type_ctx = ctx.TypeContext(filename=file, cache=cache)
    cache.imports.setdefault(file, cache.Import()).update(cache.MAIN_IMPORT, file, type_ctx)
    cache.imports[cache.MAIN_IMPORT] = cache.imports[file]
    type_ctx.set_filename(file)
    type_ctx.module_name = cache.Import.File(
        cache.Import.File.Status.External, file, cache.MODULE_MAIN
    )

    # Prepare the code
    visitor = TypeVisitor(ctx=type_ctx, preamble=preamble)
    statements: List[ast.Stmt] = []
    # Load compile-time defines (e.g., codon run -DFOO=1 ...)
    for name, value in (defines or {}).items():
        if value.startswith("str:"):
            defined_value = ast.StringExpr(value=value[4:])
            literal_name = "str"
        elif value.startswith("bool:"):
            defined_value = ast.BoolExpr(value=value == "bool:True")
            literal_name = "bool"
        else:
            defined_value = ast.IntExpr(value=value.removeprefix("int:"))
            literal_name = "int"
        statements.append(
            ast.AssignStmt(
                ast.IdExpr(name),
                rhs=defined_value,
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), index=ast.IdExpr(literal_name)),
            )
        )
    # Set up __name__
    statements.append(
        ast.AssignStmt(ast.IdExpr("__name__"), rhs=ast.StringExpr(value=cache.MODULE_MAIN))
    )
    statements.append(ast.AssignStmt(ast.IdExpr("__file__"), rhs=ast.StringExpr(value=file)))
    statements.append(node)
    suite = ast.SuiteStmt(statements)
    cache.scope(suite, type_ctx.global_shadows)
    inferred = visitor.infer_types(suite, True)
    if not inferred:
        raise ctx.TypecheckError(visitor.find_typecheck_errors(suite))
    result = ast.SuiteStmt([preamble, inferred])
    if isinstance(inferred, ast.SuiteStmt):
        visitor.prepare_vtables()
    if not type_ctx.cache.errors.empty():
        raise ctx.TypecheckError(type_ctx.cache.errors)
    return result


def load_std_library(
    cache: cache.Cache,
    preamble: ast.SuiteStmt,
    early_defines: Dict[str, str],
    barebones: bool,
):
    # Load the internal.__init__
    stdlib = ctx.TypeContext(filename=cache.STDLIB_IMPORT, cache=cache)
    stdlib_path = cache.get_import_file(cache.STDLIB_INTERNAL_MODULE, "", True)
    initial_file = "__init__.codon"
    if stdlib_path is None or not stdlib_path.path.endswith(initial_file):
        raise FileNotFoundError("standard library cannot be found")

    # Use __init_test__ for faster testing (e.g., #%% name,barebones)
    # TODO: get rid of it one day...
    if barebones:
        stdlib_path.path.removesuffix("__init__.codon")
        stdlib_path.path += "__init_test__.codon"
    stdlib.set_filename(stdlib_path.path)
    cache.imports.setdefault(stdlib_path.path, cache.Import()).update(
        cache.STDLIB_IMPORT, stdlib_path.path, stdlib
    )
    cache.imports[cache.STDLIB_IMPORT] = cache.imports[stdlib_path.path]

    # Load the standard library
    stdlib.is_stdlib_loading = True
    stdlib.module_name = cache.Import.File(
        cache.Import.File.Status.StdLibrary, stdlib_path.path, "__init__"
    )

    # 1. Core definitions
    cache.classes[cache.VAR_CLASS_TOPLEVEL] = cache.ClassData()
    core = cache.parse(code="from internal.core import *")
    cache.scope(core)
    visitor = TypeVisitor(ctx=stdlib, preamble=preamble)
    if core := visitor.infer_types(core, True):
        preamble.items.append(core)

    # 2. Load early compile-time defines (for standard library)
    for name, value in early_defines.items():
        if value.startswith("str:"):
            defined_value = ast.StringExpr(value=value[4:])
            literal_name = "str"
        elif value.startswith("bool:"):
            defined_value = ast.BoolExpr(value=value == "bool:True")
            literal_name = "bool"
        else:
            defined_value = ast.IntExpr(value=value.removeprefix("int:"))
            literal_name = "int"
        transformed = visitor.transform(
            ast.AssignStmt(
                ast.IdExpr(name),
                rhs=defined_value,
                type_expr=ast.IndexExpr(ast.IdExpr("Literal"), index=ast.IdExpr(literal_name)),
            )
        )
        if isinstance(transformed, ast.Stmt):
            preamble.items.append(transformed)

    # 3. Load stdlib
    stdlib = cache.parse(file=stdlib_path.path)
    cache.scope(stdlib)
    visitor = TypeVisitor(ctx=stdlib, preamble=preamble)
    if stdlib := visitor.infer_types(stdlib, True):
        preamble.items.append(stdlib)
    stdlib.is_stdlib_loading = False


def typecheck_node(
    context: ctx.TypeContext, node: ast.Stmt, file: str = "<internal>"
) -> ast.Stmt | None:
    with ctx.substitute("filename", file):
        preamble = ast.SuiteStmt()
        visitor = TypeVisitor(ctx=context, preamble=preamble)
        if inferred := visitor.infer_types(node, True):
            return ast.SuiteStmt([preamble, inferred])
        raise ctx.TypecheckError(visitor.find_typecheck_errors(node))
    return None
