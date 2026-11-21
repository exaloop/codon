# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Python plumbing checks only: no compiler input, IR pass runs or native comparisons."""

from __future__ import annotations

import ast
import dataclasses
import hashlib
import importlib
import inspect
import pkgutil
import typing
import unittest
from contextlib import nullcontext
from pathlib import Path
from types import ModuleType, SimpleNamespace
from unittest.mock import patch

from .. import ast as codon_ast
from .. import ir
from ..cache import Cache, FrontendOptions
from ..passes import translate
from .transform import PassManager


def port_modules():
    for package in (ir, translate):
        yield package
        for item in pkgutil.walk_packages(package.__path__, package.__name__ + "."):
            yield importlib.import_module(item.name)


class PythonPortTests(unittest.TestCase):
    def test_outliner_sorts_captures_and_private_symbols_by_name(self):
        from .util.outlining import Outliner

        module = ir.Module(None)
        typ = module.get_int_type()
        fn = module.nr(ir.BodiedFunc, "parent")
        fn.realize(module.unsafe_get_func_type("parent", typ, []), [])
        variables = {
            name: module.nr(ir.Var, typ, False, False, False, name)
            for name in ("z", "a", "z_private", "a_private")
        }
        body = fn.body = module.nr(ir.SeriesFlow)
        body.series.extend(module.nr(ir.VarValue, var) for var in variables.values())
        outliner = Outliner(fn, body, 0, len(body.series), False, True)
        outliner.in_vars = {var.id for var in variables.values()}
        outliner.out_vars = {variables[name].id for name in ("z", "a")}
        signature = module.unsafe_get_func_type("outlined", typ, [typ, typ])
        with (
            patch.object(module, "get_func_type", return_value=signature),
            patch.object(module, "get_none_type", return_value=typ),
        ):
            result = outliner.outline()
        self.assertEqual([arg.name for arg in result.func.args], ["a", "z"])
        self.assertEqual([var.name for var in result.func.symbols], ["a_private", "z_private"])
        self.assertEqual([arg.var.name for arg in result.call.args], ["a", "z"])
        self.assertEqual(
            [value.var.name for value in result.func.body.series],
            ["z", "a", "z_private", "a_private"],
        )

    def test_single_overload_dispatch_uses_current_context_binding(self):
        from ..passes.typecheck.access import get_dispatch

        cc = Cache("codon")
        current = codon_ast.types.Function(codon_ast.FunctionStmt("f:0"), cache=cc)

        class Context:
            cache = SimpleNamespace(
                overloads={"f": ["f:0"]}, functions={"f:0": SimpleNamespace(type=object())}
            )

            def __getitem__(self, name):
                assert name == "f:0"
                return SimpleNamespace(type=current)

        self.assertIs(get_dispatch(SimpleNamespace(ctx=Context()), "f"), current)

    def test_isinstance_narrowing_reuses_transformed_target(self):
        from ..passes.typecheck import cond, utils

        cc = Cache("codon")
        raw = codon_ast.DotExpr(codon_ast.IdExpr("X"), "T")
        target = codon_ast.IdExpr("type[int]", done=True)
        fn = codon_ast.types.Function(
            codon_ast.FunctionStmt(codon_ast.types.mangle(cls="RTTIType", func="_isinstance")),
            cache=cc,
        )
        original = codon_ast.CallExpr(codon_ast.IdExpr("isinstance"), [codon_ast.IdExpr("x"), raw])
        normalized = codon_ast.CallExpr(
            codon_ast.IdExpr("isinstance", type=fn), [codon_ast.IdExpr("x")]
        )

        def visit_expr(node):
            original.items[1].value = target
            return normalized

        visitor = SimpleNamespace(
            ctx=SimpleNamespace(cache=cc, generate_canonical_name=lambda name: name + ".1"),
            visit_expr=visit_expr,
            visit_stmt=lambda node: node,
        )
        with (
            patch.object(utils, "get_stdlib_type", return_value=None),
            patch.object(utils, "get_temporary_var", return_value="cond"),
        ):
            result = cond.typecheck_if(
                visitor, codon_ast.IfStmt(original, if_suite=codon_ast.SuiteStmt())
            )
        getter = result.items[1].if_suite.items[0].rhs
        self.assertIs(getter.items[1].value, target)

    def test_call_arguments_bind_function_generics_without_instantiating(self):
        from ..passes.typecheck import call, utils

        ctx, fn = SimpleNamespace(simple_types=False), object()
        visitor = SimpleNamespace(ctx=ctx, visit_expr=lambda node: node)
        expr = codon_ast.CallExpr(codon_ast.IdExpr("f"))
        with (
            patch.object(utils, "is_unbound", return_value=False),
            patch.object(call, "get_callee_fn", return_value=(fn, None)),
            patch.object(call, "transform_call_args", return_value=False),
            patch.object(utils, "with_class_generics", return_value=nullcontext()) as generics,
        ):
            self.assertIs(call.typecheck_call(visitor, expr), expr)
        generics.assert_called_once_with(ctx, fn, func=True, only_mangled=True)

    def test_occurs_check_journals_the_nested_link_level(self):
        types = codon_ast.types
        cc = Cache("codon")
        outer = types.Link(cache=cc, id=256, level=1)
        nested = types.Link(cache=cc, id=257, level=4)
        container = types.Class("Box", [types.Generic("T", nested)], cache=cc)
        self.assertFalse(outer.occurs(container, None))
        self.assertEqual(nested.level, 4)
        undo = types.Type.UnifyContext()
        self.assertFalse(outer.occurs(container, undo))
        self.assertEqual(nested.level, 1)
        self.assertIs(undo.leveled[0][0], nested)
        undo.undo()
        self.assertEqual(nested.level, 4)

    def test_realization_uses_ast_module_for_generated_dispatchers(self):
        from ..passes.typecheck import infer, utils

        node = codon_ast.FunctionStmt("f:dispatch")
        node.set(codon_ast.Attr.Module, "owner.codon")
        fn = SimpleNamespace(ast=node, realized_name=lambda: "f")
        realized = object()
        data = SimpleNamespace(
            module="", realizations={"f": SimpleNamespace(ast=object(), type=realized)}
        )
        ctx = SimpleNamespace()
        with (
            patch.object(utils, "get_function", return_value=data),
            patch.object(utils, "get_import_module") as get_module,
        ):
            self.assertIs(infer.realize_func(ctx, fn), realized)
        get_module.assert_called_once_with(ctx, "owner.codon")

    def test_class_copy_owns_generic_slots_but_shares_types(self):
        types = codon_ast.types
        cc = Cache("codon")
        first, second = types.Class("First", cache=cc), types.Class("Second", cache=cc)
        original = types.Class(
            "Box", [types.Generic("T", first)], [types.Generic("Hidden", first)], cache=cc
        )
        copied = types.Class(base=original)
        self.assertIsNot(copied.generics[0], original.generics[0])
        self.assertIsNot(copied.hidden_generics[0], original.hidden_generics[0])
        self.assertIs(copied.generics[0].type, first)
        copied.generics[0].type = second
        copied.hidden_generics[0].type = second
        self.assertIs(original.generics[0].type, first)
        self.assertIs(original.hidden_generics[0].type, first)

    def test_pointer_argument_uses_head_of_statement_expression(self):
        from ..passes.typecheck import special, utils

        cc = Cache("codon")
        typ = codon_ast.types.Class("int", cache=cc)
        var = codon_ast.IdExpr("x", type=typ, done=True)
        wrapped = codon_ast.StmtExpr(codon_ast.SuiteStmt(), expr=var, type=typ, done=True)
        result_type = codon_ast.types.Link(cache=cc, id=256)
        expr = codon_ast.CallExpr(codon_ast.IdExpr("__ptr__"), [wrapped], type=result_type)
        pointer = codon_ast.types.Class("Ptr", [codon_ast.types.Generic("T", typ)], cache=cc)
        visitor = SimpleNamespace(
            visit_expr=lambda node: node,
            ctx=SimpleNamespace(
                find_at=lambda name, time: SimpleNamespace(is_var=lambda: True), time=0
            ),
        )
        with (
            patch.object(utils, "get_stdlib_type", return_value=pointer),
            patch.object(utils, "instantiate", return_value=pointer),
        ):
            self.assertIsNone(special.transform_ptr(visitor, expr))
        self.assertTrue(expr.done)
        self.assertIs(expr.type.cls, pointer)

    def test_parallel_decorator_transforms_arguments_and_callee_before_call(self):
        from ..passes.typecheck.loops import transform_for_decorator

        visits = []

        class Visitor:
            def visit_expr(self, node):
                visits.append(node)
                return node

        fn = codon_ast.IdExpr(codon_ast.types.mangle("std.openmp", func="for_par"))
        threads = codon_ast.IntExpr(2)
        decorator = codon_ast.CallExpr(fn, [codon_ast.CallExpr.Arg(threads, name="num_threads")])
        result = transform_for_decorator(Visitor(), decorator)
        self.assertEqual(visits[:2], [fn, threads])
        self.assertIsInstance(visits[2], codon_ast.IdExpr)
        self.assertEqual(visits[2].value, "for_par")
        self.assertIs(visits[3], result)
        self.assertIs(result.expr, visits[2])
        self.assertIs(result.items[0].value, threads)
        self.assertEqual(result.items[0].name, "num_threads")

    def test_index_deletion_preserves_deferred_statement_wrapper(self):
        from ..passes.typecheck import TypeVisitor
        from ..passes.typecheck.assign import typecheck_del
        from ..passes.typecheck.ctx import TypeContext

        class DoneExprVisitor(TypeVisitor):
            def visit_expr(self, node, *args, **kwargs):
                node.done = True
                return node

        visitor = DoneExprVisitor(TypeContext(Cache("codon")))
        stmt = codon_ast.DelStmt(
            expr=codon_ast.IndexExpr(codon_ast.IdExpr("values"), codon_ast.IntExpr(0))
        )
        result = typecheck_del(visitor, stmt)
        self.assertIsInstance(result, codon_ast.ExprStmt)
        self.assertTrue(result.expr.done)
        self.assertFalse(result.done)

    def test_comparison_discovers_nonprefixed_parser_files(self):
        from .compare import collect_cases

        directory = Path(__file__).resolve().parents[2] / "test/parser"
        cases = collect_cases(directory)
        self.assertEqual({case.path for case in cases}, set(directory.glob("*.codon")))
        self.assertIn(directory / "llvm.codon", {case.path for case in cases})

    def test_dump_fingerprint_chunks_ignore_emission_boundaries(self):
        from .compare import DumpFingerprint

        class SmallFingerprint(DumpFingerprint):
            CHUNK_BYTES = 7

        data = b"this dump is emitted in unrelated pieces"
        expected = {
            "bytes": len(data),
            "sha256_chunks": [
                hashlib.sha256(data[idx : idx + 7]).hexdigest() for idx in range(0, len(data), 7)
            ],
        }
        for width in (1, 5, 7, 11, len(data)):
            digest = SmallFingerprint()
            for idx in range(0, len(data), width):
                digest.update(data[idx : idx + width])
            self.assertEqual(digest.finish(), expected)
        self.assertEqual(
            SmallFingerprint().finish()["sha256_chunks"], [hashlib.sha256(b"").hexdigest()]
        )

    def test_typed_dump_chunks_preserve_forest_bytes(self):
        from .compare import dump_typed

        root = codon_ast.SuiteStmt([codon_ast.ExprStmt(codon_ast.IntExpr(1))])
        fn = codon_ast.FunctionStmt("f", suite=codon_ast.SuiteStmt([]))
        cc = SimpleNamespace(
            functions={"f": SimpleNamespace(realizations={"f": SimpleNamespace(ast=fn)})}
        )
        expected = "".join(
            codon_ast.dump(node, include_attributes=True, types=True, indent=0) + "\n"
            for node in (root, fn)
        )
        self.assertEqual("".join(dump_typed(cc, root, 0)), expected)

    def test_imports(self):
        for module in port_modules():
            with self.subTest(module=module.__name__):
                self.assertIsNotNone(module)

    def test_frontend_handle_types(self):
        # Existing typechecker annotations must reference the actual backbone,
        # not a second set of placeholder classes.
        self.assertIs(codon_ast.ir.Type, ir.Type)
        self.assertIs(codon_ast.ir.Function, ir.Func)

    def test_node_classes_are_identity_dataclasses(self):
        for name, cls in inspect.getmembers(ir, inspect.isclass):
            if not issubclass(cls, ir.Node):
                continue
            with self.subTest(cls=name):
                self.assertTrue(dataclasses.is_dataclass(cls))
                self.assertFalse(cls.__dataclass_params__.eq)
                self.assertIs(cls.__hash__, object.__hash__)

    def test_declared_python_state(self):
        # Catch misspelled/missing dataclass fields and unresolved forward
        # references without running translation or any transformation.
        for module in port_modules():
            if module is ir or module.__name__ == __name__:
                continue
            source = ast.parse(Path(module.__file__).read_text())
            for node in source.body:
                if not isinstance(node, ast.ClassDef):
                    continue
                cls = getattr(module, node.name)
                if not dataclasses.is_dataclass(cls):
                    continue
                declared = set(typing.get_type_hints(cls))
                for parent in cls.__mro__:
                    declared.update(
                        name
                        for name, member in vars(parent).items()
                        if isinstance(member, property)
                    )
                for method in node.body:
                    if not isinstance(method, ast.FunctionDef):
                        continue
                    for item in ast.walk(method):
                        if (
                            isinstance(item, ast.Attribute)
                            and isinstance(item.ctx, ast.Store)
                            and isinstance(item.value, ast.Name)
                            and item.value.id == "self"
                        ):
                            with self.subTest(cls=cls.__name__, member=item.attr):
                                self.assertIn(item.attr, declared)

    def test_cache_and_context_construction(self):
        cc = Cache("codon")
        self.assertIsInstance(cc.module, ir.Module)
        ctx = translate.TranslateContext(cc)
        self.assertIs(ctx.cache, cc)

    def test_python_api_call_signatures(self):
        # Inspect the port's Python calls without executing IR code. In
        # particular, C++ positional constructors can be keyword-only in the
        # existing Python AST, while Module.n/nr model C++ template factories.
        for module in port_modules():
            if module.__name__ == __name__:
                continue
            tree = ast.parse(Path(module.__file__).read_text())
            for node in ast.walk(tree):
                if not isinstance(node, ast.Call):
                    continue
                if any(isinstance(arg, ast.Starred) for arg in node.args) or any(
                    kw.arg is None for kw in node.keywords
                ):
                    continue
                target, args = None, node.args
                match node.func:
                    case ast.Name(id=name):
                        target = vars(module).get(name)
                    case ast.Attribute(value=ast.Name(id=name), attr=attr):
                        owner = vars(module).get(name)
                        if isinstance(owner, ModuleType):
                            with self.subTest(module=module.__name__, line=node.lineno):
                                self.assertTrue(hasattr(owner, attr), ast.unparse(node.func))
                            target = getattr(owner, attr)
                if (
                    isinstance(node.func, ast.Attribute)
                    and node.func.attr in {"n", "nr", "make"}
                    and node.args
                    and isinstance(node.args[0], ast.Attribute)
                    and isinstance(node.args[0].value, ast.Name)
                ):
                    cls = node.args[0]
                    owner = vars(module).get(cls.value.id)
                    if isinstance(owner, ModuleType):
                        target = getattr(owner, cls.attr)
                        args = node.args[1 if node.func.attr == "nr" else 2 :]
                if not callable(target):
                    continue
                try:
                    signature = inspect.signature(target)
                except (TypeError, ValueError):
                    continue
                with self.subTest(module=module.__name__, line=node.lineno):
                    signature.bind(
                        *[object() for _ in args],
                        **{kw.arg: object() for kw in node.keywords},
                    )

    def test_debug_pass_construction(self):
        options = SimpleNamespace(debug=True, test=False, pmempty=False, disabled=[])
        manager = PassManager(options)
        self.assertEqual(
            manager.execution_order,
            [
                "core-pipeline-lowering",
                "core-imperative-for-lowering",
                "core-missing-return-lowering",
                "core-throw-lowering",
                "core-async-for-lowering",
                "core-await-lowering",
                "core-parallel-openmp",
                "core-pythonic-formatting-opt",
            ],
        )
        for metadata in manager.passes.values():
            self.assertIs(metadata.pass_.manager, manager)
        # Deliberately do not call manager.run(). IR correctness is a separate step.

    def test_release_pass_construction(self):
        release = PassManager(FrontendOptions(debug=False))
        self.assertEqual(release.execution_order, [
            "core-pythonic-dict-arithmetic-opt",
            "core-pythonic-list-addition-opt",
            "core-pythonic-str-addition-opt",
            "core-pythonic-generator-argument-opt",
            "core-pythonic-enumerate-opt",
            "core-pythonic-io-cat-opt",
            "core-pythonic-formatting-opt",
            "core-pipeline-lowering",
            "core-imperative-for-lowering",
            "core-missing-return-lowering",
            "core-throw-lowering",
            "core-numpy-inline",
            "core-folding-pass-group",
            "core-numpy-fusion",
            "core-pythonic-generator-loop-fusion",
            "core-numpy-lifetime",
            "core-imperative-for-lowering:2",
            "core-numpy-bounds-check-elision",
            "core-async-for-lowering",
            "core-await-lowering",
            "core-parallel-openmp",
            "core-folding-pass-group:2",
            "core-devirtualize",
        ])
        jit = PassManager(FrontendOptions(debug=False, jit=True))
        self.assertEqual(
            jit.execution_order,
            [key for key in release.execution_order if key != "core-folding-pass-group:2"],
        )


if __name__ == "__main__":
    unittest.main()
