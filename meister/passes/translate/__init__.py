# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ... import ast, cache, ir
from ...bridge import Set, dataclass
from ..typecheck import special, utils
from .ctx import TranslateContext, TranslateItem


@dataclass(init=False, eq=False)
class IdVisitor(ast.NodeVisitor):
    """C++: codon/parser/visitors/translate/translate.cpp:254. Search an expression tree for identifiers."""

    ids: Set[str]

    # C++ API: codon/parser/visitors/translate/translate.cpp:254
    def __init__(self):
        self.ids = set()

    # C++: codon/parser/visitors/translate/translate.cpp:272
    def visit_IdExpr(self, expr):
        self.ids.add(expr.value)


@dataclass(init=False, eq=False)
class TranslateVisitor:
    """C++: codon/parser/visitors/translate/translate.h:15. AST to CIR."""

    ctx: TranslateContext
    result: ir.Value | None
    info: ast.Node.SrcInfo

    # C++: codon/parser/visitors/translate/translate.cpp:25
    def __init__(self, ctx):
        self.ctx, self.result = ctx, None
        self.info = ast.Node.SrcInfo()

    # C++: codon/parser/visitors/translate/translate.cpp:28
    @staticmethod
    def apply(cc, stmts):
        none = cc.classes["NoneType"].realizations["NoneType"].ir
        if cc.options.jit:
            name = f"_jit_{cc.jit_cell}"
            main = cc.module.nr(ir.BodiedFunc, name)
            main.info = ast.Node.SrcInfo("<jit>", 0, 0, 0)
            main.global_ = True
            main.realize(cc.module.unsafe_get_func_type(name, none, [], False), [])
            main.jit = True
        else:
            main = ir.cast(ir.BodiedFunc, cc.module.main_func)
            main.realize(
                cc.module.unsafe_get_func_type("<internal_func_type>", none, [], False), []
            )
            main.info = ast.Node.SrcInfo(str(cc.fs.get_module0()), 0, 0, 0)
        block = cc.module.nr(ir.SeriesFlow, "body")
        main.body = block
        if cc.codegen_ctx is None:
            cc.codegen_ctx = TranslateContext(cc)
        ctx = cc.codegen_ctx
        ctx.bases, ctx.series = [main], [block]
        cc.populate_python_module()
        ctx.main_func, ctx.main_user_start = main, None
        visitor = TranslateVisitor(ctx)
        visitor.initialize_globals()
        # Full typechecking returns [runtime preamble, user code]. JIT cells
        # contain [user code, pending functions...]; their runtime already runs.
        has_preamble = not cc.options.jit and isinstance(stmts, ast.SuiteStmt) and bool(stmts.items)
        if has_preamble:
            visitor.transform(stmts.items[0])
        before = len(block.series)
        if has_preamble:
            for stmt in stmts.items[1:]:
                visitor.transform(stmt)
        else:
            visitor.transform(stmts)
        if len(block.series) > before:
            ctx.main_user_start = block.series[before]
        for fn in list(cc.functions.values()):
            visitor.transform(fn.ast)
        return main

    # C++: codon/parser/visitors/translate/translate.cpp:83
    def translate_stmts(self, stmts):
        self.initialize_globals()
        TranslateVisitor(self.ctx.cache.codegen_ctx).transform(stmts)
        for fn in list(self.ctx.cache.functions.values()):
            TranslateVisitor(self.ctx.cache.codegen_ctx).transform(fn.ast)

    # C++: codon/parser/visitors/translate/translate.cpp:90
    def initialize_globals(self):
        for name, handle in sorted(self.ctx.cache.globals.items()):
            if handle is not None:
                continue
            typ = self.ctx.cache.type_ctx[name].type
            storage = None
            if typ:
                if not typ.is_instantiated() or typ == ast.types.Stdlib.Type or typ.func:
                    continue
                storage = self.get_type(typ)
            if name == ast.types.mangle(var="__argv__"):
                handle = self.ctx.module.argv_var
            elif name == ast.types.mangle(var="__argc__"):
                handle = self.ctx.module.argc_var
            else:
                handle = self.ctx.module.nr(ir.Var, storage, True, False, False, name)
            self.ctx.cache.globals[name] = handle
            self.ctx.add(TranslateItem.Kind.Var, name, handle)

    # C++: codon/parser/visitors/translate/translate.h:76
    def make(self, cls, source, *args):
        return self.ctx.module.n(cls, source, *args)

    # C++: codon/parser/visitors/translate/translate.cpp:113
    def transform(self, node):
        visitor = TranslateVisitor(self.ctx)
        visitor.info = node.info
        if isinstance(node, ast.Expr):
            return visitor.transform_expr(node)
        visitor.visit(node)
        if visitor.result is not None:
            self.ctx.current_series.push_back(visitor.result)
        return visitor.result

    # C++: codon/parser/visitors/translate/translate.cpp:197
    def visit(self, node):
        method = getattr(self, "visit_" + type(node).__name__, None)
        assert method is not None, f"invalid node {type(node).__name__}"
        method(node)

    # C++ API: codon/parser/visitors/translate/translate.h:15
    def transform_expr(self, expr):
        """C++: translate.cpp:120. Preserve literal/partial construction metadata."""
        sequence = any(
            expr.has(attr)
            for attr in (
                ast.Attr.ExprList,
                ast.Attr.ExprSet,
                ast.Attr.ExprDict,
                ast.Attr.ExprPartial,
            )
        )
        if sequence:
            self.ctx.seq_items.append([])
        partial = expr.type.require_cls if expr.has(ast.Attr.ExprPartial) else None
        self.visit(expr)
        result = self.result
        if expr.has(ast.Attr.ExprList) or expr.has(ast.Attr.ExprSet):
            elements = []
            for kind, value in self.ctx.seq_items[-1]:
                assert kind in (ast.Attr.ExprSequenceItem, ast.Attr.ExprStarSequenceItem), (
                    "invalid list/set element"
                )
                elements.append(ir.LiteralElement(value, kind == ast.Attr.ExprStarSequenceItem))
            result.set_attribute(
                ir.ListLiteralAttribute(elements)
                if expr.has(ast.Attr.ExprList)
                else ir.SetLiteralAttribute(elements)
            )
            self.ctx.seq_items.pop()
        if expr.has(ast.Attr.ExprDict):
            elements, idx = [], 0
            items = self.ctx.seq_items[-1]
            while idx < len(items):
                kind, value = items[idx]
                if kind == ast.Attr.ExprStarSequenceItem:
                    elements.append(ir.KeyValuePair(value, None))
                else:
                    assert (
                        kind == ast.Attr.ExprSequenceItem
                        and idx + 1 < len(items)
                        and items[idx + 1][0] == ast.Attr.ExprSequenceItem
                    ), "invalid dict element"
                    elements.append(ir.KeyValuePair(value, items[idx + 1][1]))
                    idx += 1
                idx += 1
            result.set_attribute(ir.DictLiteralAttribute(elements))
            self.ctx.seq_items.pop()
        if expr.has(ast.Attr.ExprPartial):
            assert partial, "invalid partial element"
            values, idx = [], 0
            fn = partial.partial_func
            for arg, flag in zip(fn.ast.items, partial.partial_mask):
                if flag == ast.types.Class.Flag.Included and arg.is_value():
                    assert (
                        idx < len(self.ctx.seq_items[-1])
                        and self.ctx.seq_items[-1][idx][0] == ast.Attr.ExprSequenceItem
                    ), "invalid partial element"
                    values.append(self.ctx.seq_items[-1][idx][1])
                    idx += 1
                elif arg.is_value():
                    values.append(None)
            result.set_attribute(ir.PartialFunctionAttribute(fn.ast.name, values))
            self.ctx.seq_items.pop()
        for attr in (ast.Attr.ExprSequenceItem, ast.Attr.ExprStarSequenceItem):
            if expr.has(attr):
                self.ctx.seq_items[-1].append((attr, result))
        return result

    # C++: codon/parser/visitors/translate/translate.cpp:197
    def visit_NoneExpr(self, expr):
        name = (
            expr.type.realized_name() + ":" + ast.types.mangle(cls="Optional", func="__new__")
            if expr.type == ast.types.Stdlib.Optional
            else ast.types.mangle(cls="NoneType", func="__new__")
        )
        item = self.ctx.force_find(name)
        self.result = self.make(ir.CallInstr, expr, self.make(ir.VarValue, expr, item.func), [])

    # C++: codon/parser/visitors/translate/translate.cpp:208
    def visit_BoolExpr(self, expr):
        self.result = self.make(ir.BoolConst, expr, expr.value, self.get_type(expr.type))

    # C++: codon/parser/visitors/translate/translate.cpp:212
    def visit_IntExpr(self, expr):
        self.result = self.make(ir.IntConst, expr, expr.int_value, self.get_type(expr.type))

    # C++: codon/parser/visitors/translate/translate.cpp:216
    def visit_FloatExpr(self, expr):
        self.result = self.make(ir.FloatConst, expr, expr.float_value, self.get_type(expr.type))

    # C++: codon/parser/visitors/translate/translate.cpp:220
    def visit_StringExpr(self, expr):
        cls = ir.BytesConst if expr.type == "bytes" else ir.StringConst
        self.result = self.make(cls, expr, expr.value, self.get_type(expr.type))

    # C++: codon/parser/visitors/translate/translate.cpp:227
    def visit_IdExpr(self, expr):
        item = self.ctx.force_find(expr.value)
        if item.var is not None or item.func is not None:
            self.result = self.make(
                ir.VarValue, expr, item.var if item.var is not None else item.func
            )
        else:
            # Just use NoneType, which is {} (same as type).
            constructor = self.ctx.force_find(ast.types.mangle(cls="NoneType", func="__new__"))
            self.result = self.make(
                ir.CallInstr, expr, self.make(ir.VarValue, expr, constructor.func), []
            )

    # C++: codon/parser/visitors/translate/translate.cpp:246
    def visit_IfExpr(self, expr):
        self.result = self.make(
            ir.TernaryInstr,
            expr,
            self.transform(expr.cond),
            self.transform(expr.ifexpr),
            self.transform(expr.elsexpr),
        )

    # C++: codon/parser/visitors/translate/translate.cpp:275
    def visit_GeneratorExpr(self, expr):
        name = self.ctx.cache.imports[cache.MAIN_IMPORT].ctx.generate_canonical_name("_generator")
        fn = self.ctx.module.nr(ir.BodiedFunc, name)
        fn.global_, fn.generator = True, True
        names, types, items = [], [], []
        visitor = IdVisitor()
        visitor.visit(expr)
        for name_ in visitor.ids:
            item = self.ctx.find(name_)
            if item and item.func is None and item.type is None and not item.var.global_:
                types.append(item.var.type)
                names.append(name_)
                items.append(self.make(ir.VarValue, expr, item.var))
        fn.realize(
            self.ctx.module.unsafe_get_func_type(
                name, self.ctx.force_find(expr.type.realized_name()).type, types, False
            ),
            names,
        )
        self.ctx.add_block()
        for name_ in names:
            self.ctx.add(TranslateItem.Kind.Var, name_, fn.get_arg_var(name_))
        body = self.make(ir.SeriesFlow, expr, "body")
        self.ctx.bases.append(fn)
        self.ctx.add_series(body)
        expr.iter_blocks(expr_fn=lambda value: ast.YieldStmt(expr=value))
        self.transform(expr.final_suite())
        self.ctx.pop_series()
        self.ctx.bases.pop()
        fn.body = body
        self.ctx.pop_block()
        self.result = self.make(ir.CallInstr, expr, self.make(ir.VarValue, expr, fn), items)

    # C++: codon/parser/visitors/translate/translate.cpp:315
    def visit_CallExpr(self, expr):
        from ...ir.util.irtools import virtual_call

        identifier = expr.expr.value if isinstance(expr.expr, ast.IdExpr) else ""
        if identifier == ast.types.mangle(func="__ptr__"):
            head = expr.items[0].value
            pre = ir.cast(ir.FlowInstr, self.transform(head))
            while isinstance(head, ast.StmtExpr):
                head = head.expr
            members = []
            while isinstance(head, ast.DotExpr):
                members.append(head.member)
                head = head.expr
            members.reverse()
            assert isinstance(head, ast.IdExpr), "expected IdExpr"
            item = self.ctx.find(head.value)
            assert item and item.var is not None, "not a variable"
            pointer = self.make(ir.PointerValue, expr, item.var, members)
            if pre:
                pre.value = pointer
                self.result = pre
            else:
                self.result = pointer
            return
        if identifier == ast.types.mangle(cls="__array__", func="__new__"):
            fn = expr.expr.type.require_func
            size = fn.func_generics[0].type.require_int
            typ = fn.func_parent.require_cls[0]
            pointer = self.ctx.module.unsafe_get_pointer_type(self.get_type(typ))
            pointer.ast_type = expr.type
            self.result = self.make(ir.StackAllocInstr, expr, pointer, size)
            return
        if identifier.startswith(ast.types.mangle(cls="Generator", func="_yield_in_no_suspend")):
            self.result = self.make(ir.YieldInInstr, expr, self.get_type(expr.type), False)
            return
        fn = expr.expr.type.func
        assert fn, "not calling function"
        callee = self.transform(expr.expr)
        variadic = fn.ast.has(ast.Attr.CVarArg)
        items = []
        for idx, arg in enumerate(expr.items):
            assert not isinstance(arg.value, ast.EllipsisExpr), "ellipsis not elided"
            if idx + 1 == len(expr.items) and variadic:
                assert isinstance(arg.value, ast.CallExpr), "expected *args tuple"
                items.extend(self.transform(item.value) for item in arg.value.items)
            else:
                items.append(self.transform(arg.value))
        thunk = 0
        identifier = expr.get(ast.Attr.ExprVirtual, int)
        if identifier is not None and identifier != -1:
            thunk = identifier
        elif identifier is not None and utils.is_polymorphic(self.ctx.cache.type_ctx, fn):
            signature = special.vtable_signature(self.ctx.cache.type_ctx, fn)
            assert signature in self.ctx.cache.thunk_ids, f"not a valid thunk: {signature}"
            thunk = self.ctx.cache.thunk_ids[signature]
        if thunk:
            self.result = virtual_call(callee, items, thunk, self.ctx.base)
            self.result.info = expr.info
        else:
            self.result = self.make(ir.CallInstr, expr, callee, items)

    # C++: codon/parser/visitors/translate/translate.cpp:399
    def visit_DotExpr(self, expr):
        if expr.member in ("__atomic__", "__elemsize__", "__contents_atomic__"):
            assert isinstance(expr.expr, ast.IdExpr), "expected IdExpr"
            typ = utils.extract_type(self.ctx.cache.type_ctx, expr.expr.type)
            handle = self.ctx.force_find(typ.realized_name()).type
            assert handle, "not a type"
            kind = (
                ir.Intrinsic.IS_ATOMIC
                if expr.member == "__atomic__"
                else ir.Intrinsic.IS_CONTENT_ATOMIC
                if expr.member == "__contents_atomic__"
                else ir.Intrinsic.SIZEOF
            )
            intrinsic = self.ctx.module.get_intrinsic(kind, handle)
            self.result = self.make(ir.CallInstr, expr, self.make(ir.VarValue, expr, intrinsic))
        else:
            cls = self.ctx.cache.classes.get(expr.expr.type.require_cls.name)
            self.result = self.make(
                ir.ExtractInstr,
                expr,
                self.transform(expr.expr),
                expr.member,
                "",
                bool(cls and cls.has_rtti()),
            )

    # C++: codon/parser/visitors/translate/translate.cpp:422
    def visit_YieldExpr(self, expr):
        self.result = self.make(ir.YieldInInstr, expr, self.get_type(expr.type))

    # C++: codon/parser/visitors/translate/translate.cpp:426
    def visit_PipeExpr(self, expr):
        from ...ir.util.cloning import CloneVisitor

        def is_generator(value):
            typ = value.type
            return ir.is_a(ir.GeneratorType, typ) or bool(
                (fn := ir.cast(ir.FuncType, typ)) and ir.is_a(ir.GeneratorType, fn.return_type)
            )

        first = self.transform(expr.items[0].expr)
        first_generator = is_generator(first)
        stages = [ir.Stage(first, [], first_generator, False)]
        simple = not first_generator
        for idx, pipe in enumerate(expr.items[1:], 1):
            call = pipe.expr
            assert isinstance(call, ast.CallExpr), "not a call"
            fn = self.transform(call.expr)
            if idx + 1 != len(expr.items):
                simple &= not is_generator(fn)
            args = [
                None if isinstance(arg.value, ast.EllipsisExpr) else self.transform(arg.value)
                for arg in call.items
            ]
            stages.append(ir.Stage(fn, args, is_generator(fn), False))
        if simple:
            # Transform a |> b |> c to c(b(a)).
            visitor = CloneVisitor(self.ctx.module)
            self.result = visitor.clone(stages[0].callee)
            for stage in stages[1:]:
                args = [
                    visitor.clone(arg) if arg is not None else self.result for arg in stage.args
                ]
                self.result = self.make(ir.CallInstr, expr, visitor.clone(stage.callee), args)
        else:
            for stage, pipe in zip(stages, expr.items):
                if pipe.op == "||>":
                    stage.parallel = True
            # This is a statement in IR.
            self.ctx.current_series.push_back(self.make(ir.PipelineFlow, expr, stages))

    # C++: codon/parser/visitors/translate/translate.cpp:478
    def visit_AwaitExpr(self, expr):
        self.result = self.make(
            ir.AwaitInstr,
            expr,
            self.transform(expr.expr),
            self.get_type(expr.type),
            expr.expr.type == ast.types.Stdlib.Generator,
        )

    # C++: codon/parser/visitors/translate/translate.cpp:484
    def visit_StmtExpr(self, expr):
        body = self.make(ir.SeriesFlow, expr, "body")
        self.ctx.add_series(body)
        for stmt in expr.items:
            self.transform(stmt)
        self.ctx.pop_series()
        self.result = self.make(ir.FlowInstr, expr, body, self.transform(expr.expr))

    # C++: codon/parser/visitors/translate/translate.cpp:508
    def visit_SuiteStmt(self, stmt):
        for item in stmt.items:
            self.transform(item)

    # C++: codon/parser/visitors/translate/translate.cpp:513
    def visit_BreakStmt(self, stmt):
        self.result = self.make(ir.BreakInstr, stmt)

    # C++: codon/parser/visitors/translate/translate.cpp:515
    def visit_ContinueStmt(self, stmt):
        self.result = self.make(ir.ContinueInstr, stmt)

    # C++: codon/parser/visitors/translate/translate.cpp:519
    def visit_ExprStmt(self, stmt):
        match stmt.expr:
            case ast.CallExpr(expr=ast.IdExpr(value=name)) as call if name == ast.types.mangle(
                cls="Generator", func="_yield_final"
            ):
                self.result = self.make(
                    ir.YieldInstr, stmt, self.transform(call.items[0].value), True
                )
                self.ctx.base.generator = True
            case _:
                self.result = self.transform(stmt.expr)

    # C++: codon/parser/visitors/translate/translate.cpp:531
    def visit_AssignStmt(self, stmt):
        argv, argc = ast.types.mangle(var="__argv__"), ast.types.mangle(var="__argc__")
        if isinstance(stmt.lhs, ast.IdExpr) and stmt.lhs.value in (argv, argc):
            return
        assert isinstance(stmt.lhs, ast.IdExpr), "expected IdExpr"
        name = stmt.lhs.value
        if stmt.is_update():
            # May not exist when static if/else combines types. TC would have
            # raised an error if the types did not match.
            if (item := self.ctx.find(name)) and (var := item.var) is not None:
                if var.type is None:
                    var.info = stmt.info
                    # Annotation without RHS establishes storage, not a value.
                    var.type = self.get_type((stmt.rhs or stmt.lhs).type)
                if stmt.rhs:
                    self.result = self.make(ir.AssignInstr, stmt, var, self.transform(stmt.rhs))
            return
        typ = stmt.lhs.type
        if not typ.is_instantiated() or typ == ast.types.Stdlib.Type or typ.func:
            if stmt.rhs and not isinstance(stmt.rhs, ast.IdExpr):
                self.result = self.transform(stmt.rhs)  # Side effect
            return  # Type/fn aliases, etc.
        if name in self.ctx.cache.globals:
            item = self.ctx.find(name)
            assert item and item.var is not None, f"cannot find global '{name}'"
            var = item.var
            var.info = stmt.info
            var.type = self.get_type((stmt.rhs or stmt.lhs).type)
        else:
            var = self.make(
                ir.Var, stmt, self.get_type((stmt.rhs or stmt.lhs).type), False, False, False, name
            )
            self.ctx.base.symbols.append(var)
            self.ctx.add(TranslateItem.Kind.Var, name, var)
        # Check if it is thread-local.
        if stmt.is_thread_local():
            var.tls, var.global_ = True, True
        # Check if it is a C variable.
        if stmt.lhs.has(ast.Attr.ExprExternVar):
            var.external, var.global_, var.name = True, True, self.ctx.cache.rev(name)
            return
        if stmt.rhs:
            self.result = self.make(ir.AssignInstr, stmt, var, self.transform(stmt.rhs))

    # C++: codon/parser/visitors/translate/translate.cpp:603
    def visit_AssignMemberStmt(self, stmt):
        cls = self.ctx.cache.classes.get(stmt.lhs.type.require_cls.name)
        self.result = self.make(
            ir.InsertInstr,
            stmt,
            self.transform(stmt.lhs),
            stmt.member,
            self.transform(stmt.rhs),
            "",
            bool(cls and cls.has_rtti()),
        )

    # C++: codon/parser/visitors/translate/translate.cpp:610
    def visit_ReturnStmt(self, stmt):
        self.result = self.make(
            ir.ReturnInstr, stmt, self.transform(stmt.expr) if stmt.expr else None
        )

    # C++: codon/parser/visitors/translate/translate.cpp:615
    def visit_YieldStmt(self, stmt):
        self.result = self.make(
            ir.YieldInstr, stmt, self.transform(stmt.expr) if stmt.expr else None
        )
        self.ctx.base.generator = True

    # C++: codon/parser/visitors/translate/translate.cpp:621
    def visit_WhileStmt(self, stmt):
        loop = self.make(
            ir.WhileFlow, stmt, self.transform(stmt.cond), self.make(ir.SeriesFlow, stmt, "body")
        )
        self.ctx.add_series(loop.body)
        self.transform(stmt.suite)
        self.ctx.pop_series()
        self.result = loop

    # C++: codon/parser/visitors/translate/translate.cpp:630
    def visit_ForStmt(self, stmt):
        from ...ir.transform.parallel.schedule import OMPSched

        schedule = None
        if stmt.decorator:
            call = stmt.decorator
            assert isinstance(call, ast.CallExpr), "for par is not a call"
            fn = call.expr.type.func
            assert fn and fn.ast.name == ast.types.mangle("std.openmp", func="for_par"), (
                "for par is not a function"
            )
            code, ordered = (
                fn.func_generics[0].type.require_str,
                fn.func_generics[1].type.require_bool,
            )
            threads, chunk = (
                self.transform(call.items[0].value),
                self.transform(call.items[1].value),
            )
            collapse, gpu = (
                fn.func_generics[2].type.require_int,
                fn.func_generics[3].type.require_bool,
            )
            schedule = OMPSched(
                code, threads=threads, chunk=chunk, ordered=ordered, collapse=collapse, gpu=gpu
            )
        assert isinstance(stmt.var, ast.IdExpr), "expected IdExpr"
        name = stmt.var.value
        item = self.ctx.find(name)
        var = (
            self.make(ir.Var, stmt, self.get_type(stmt.var.type), False, False, False, name)
            if not item or not stmt.var.has(ast.Attr.ExprDominated)
            else item.var
        )
        self.ctx.base.symbols.append(var)
        body = self.make(ir.SeriesFlow, stmt, "body")
        loop = self.make(ir.ForFlow, stmt, self.transform(stmt.iter), body, var)
        loop.async_, loop.schedule = stmt.async_, schedule
        self.ctx.add(TranslateItem.Kind.Var, name, var)
        self.ctx.add_series(body)
        self.transform(stmt.suite)
        self.ctx.pop_series()
        self.result = loop

    # C++: codon/parser/visitors/translate/translate.cpp:671
    def visit_IfStmt(self, stmt):
        cond = self.transform(stmt.cond)
        true = self.make(ir.SeriesFlow, stmt, "ifstmt_true")
        self.ctx.add_series(true)
        self.transform(stmt.if_suite)
        self.ctx.pop_series()
        false = None
        if stmt.else_suite:
            false = self.make(ir.SeriesFlow, stmt, "ifstmt_false")
            self.ctx.add_series(false)
            self.transform(stmt.else_suite)
            self.ctx.pop_series()
        self.result = self.make(ir.IfFlow, stmt, cond, true, false)

    # C++: codon/parser/visitors/translate/translate.cpp:688
    def visit_TryStmt(self, stmt):
        body = self.make(ir.SeriesFlow, stmt, "body")
        self.ctx.add_series(body)
        self.transform(stmt.suite)
        self.ctx.pop_series()
        final = self.make(ir.SeriesFlow, stmt, "finally")
        if stmt.finally_suite:
            self.ctx.add_series(final)
            self.transform(stmt.finally_suite)
            self.ctx.pop_series()
        else_ = None
        if stmt.else_suite:
            else_ = self.make(ir.SeriesFlow, stmt, "else")
            self.ctx.add_series(else_)
            self.transform(stmt.else_suite)
            self.ctx.pop_series()
        flow = self.make(ir.TryCatchFlow, stmt, body, final, else_)
        for catch in stmt.items:
            handler = self.make(ir.SeriesFlow, stmt, "catch")
            typ = (
                self.get_type(utils.extract_type(self.ctx.cache.type_ctx, catch.exc.type))
                if catch.exc
                else None
            )
            var = None
            if catch.var:
                item = self.ctx.find(catch.var)
                var = (
                    self.make(ir.Var, stmt, typ, False, False, False, catch.var)
                    if not item or not catch.has(ast.Attr.ExprDominated)
                    else item.var
                )
                self.ctx.add(TranslateItem.Kind.Var, catch.var, var)
                self.ctx.base.symbols.append(var)
            self.ctx.add_series(handler)
            self.transform(catch.suite)
            self.ctx.pop_series()
            flow.catches.append(ir.Catch(handler, typ, var))
        self.result = flow

    # C++: codon/parser/visitors/translate/translate.cpp:734
    def visit_ThrowStmt(self, stmt):
        self.result = self.make(
            ir.ThrowInstr, stmt, self.transform(stmt.expr) if stmt.expr else None
        )

    # C++: codon/parser/visitors/translate/translate.cpp:739
    def visit_FunctionStmt(self, stmt):
        # Process all realizations.
        self.transform_function_realizations(stmt.name, stmt.has(ast.Attr.LLVM))

    # C++: codon/parser/visitors/translate/translate.cpp:744
    def visit_ClassStmt(self, stmt):
        # All type handles are already generated. FunctionStmt handles methods.
        pass

    # C++: codon/parser/visitors/translate/translate.h:64
    def visit_CommentStmt(self, stmt):
        pass

    # C++: codon/parser/visitors/translate/translate.h:65
    def visit_DirectiveStmt(self, stmt):
        pass

    # C++: codon/parser/visitors/translate/translate.cpp:751
    def get_type(self, typ):
        assert typ and typ.cls, "not a class"
        name = ast.types.Class.realized_name(typ.require_cls)
        item = self.ctx.force_find(name)
        assert item.type is not None, f"type not IR-realized: {name}"
        return item.type

    # C++: codon/parser/visitors/translate/translate.cpp:760
    def transform_function_realizations(self, name, is_llvm):
        for key, real in list(self.ctx.cache.functions[name].realizations.items()):
            if (name, key) not in self.ctx.cache.pending_realizations:
                continue
            self.ctx.cache.pending_realizations.remove((name, key))
            real.ir.info = self.info
            assert real.ast is not None, f"AST not set for {key}"
            if is_llvm:
                self.transform_llvm_function(real.type, real.ast, real.ir)
            else:
                self.transform_function(real.type, real.ast, real.ir)

    # C++: codon/parser/visitors/translate/translate.cpp:778
    def transform_function(self, typ, node, fn):
        names, indices, idx = [], [], 0
        for pos, arg in enumerate(node.items):
            if arg.is_value():
                if not typ[idx].func:
                    names.append(self.ctx.cache.rev(arg.name))
                    indices.append(pos)
                idx += 1
        if node.has(ast.Attr.CVarArg):
            names.pop()
            indices.pop()
        # TODO: refactor IR attribute API.
        attrs = dict(node.get(ast.Attr.FunctionAttributes, dict, {}))
        attrs[".module"] = node.get(ast.Attr.Module, str, "")
        fn.set_attribute(ir.KeyValueAttribute(attrs))
        for name, idx in zip(names, indices):
            fn.get_arg_var(name).info = node.items[idx].info
        # fn.unmangled_name = self.ctx.cache.rev(typ.ast.name)
        if not node.has(ast.Attr.C) and not node.has(ast.Attr.Internal):
            self.ctx.add_block()
            for name, idx in zip(names, indices):
                self.ctx.add(TranslateItem.Kind.Var, node.items[idx].name, fn.get_arg_var(name))
            body = self.make(ir.SeriesFlow, node, "body")
            self.ctx.bases.append(ir.cast(ir.BodiedFunc, fn))
            self.ctx.add_series(body)
            self.transform(node.suite)
            self.ctx.pop_series()
            self.ctx.bases.pop()
            ir.cast(ir.BodiedFunc, fn).body = body
            self.ctx.pop_block()
        if node.async_:
            fn.async_ = True

    # C++ API: codon/parser/visitors/translate/translate.h:15
    def transform_llvm_function(self, typ, node, fn):
        names, indices = [], []
        for idx, arg in enumerate(node.items):
            if arg.is_value():
                names.append(self.ctx.cache.reverse_identifier_lookup[arg.name])
                indices.append(idx)
        fn = ir.cast(ir.LLVMFunc, fn)
        assert fn is not None, "not a function"
        attrs = dict(node.get(ast.Attr.FunctionAttributes, dict, {}))
        attrs[".module"] = node.get(ast.Attr.Module, str, "")
        fn.set_attribute(ir.KeyValueAttribute(attrs))
        for name, idx in zip(names, indices):
            fn.get_arg_var(name).info = node.items[idx].info
        first = node.suite.first_in_block()
        assert isinstance(first, ast.ExprStmt) and isinstance(first.expr, ast.StringExpr), (
            "LLVM function does not begin with a string"
        )
        literals = []
        for stmt in node.suite.items[1:]:
            value = stmt.expr.type
            if value.int is not None:
                literals.append(ir.Generic(value.require_int))
            elif value.str is not None:
                literals.append(ir.Generic(value.require_str))
            else:
                assert value is not None, "invalid LLVM type argument"
                literals.append(
                    ir.Generic(self.get_type(utils.extract_type(self.ctx.cache.type_ctx, value)))
                )
        is_declare, declares, lines = True, "", []
        for line in first.expr.value.splitlines():
            stripped = line.strip()
            # Extract declares and constants.
            if is_declare and not stripped.startswith("declare ") and not stripped.startswith("@"):
                if "private constant" not in stripped:
                    is_declare = False
                    if stripped and not stripped.endswith(":"):
                        lines.append("entry:")
            if is_declare:
                declares += stripped + "\n"
            else:
                lines.append(line)
        fn.llvm_body, fn.llvm_declares, fn.llvm_literals = "\n".join(lines), declares, literals
