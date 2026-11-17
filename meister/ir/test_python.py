# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Python plumbing checks only: no compiler input, IR pass runs or native comparisons."""

from __future__ import annotations

import ast
import dataclasses
import importlib
import inspect
import pkgutil
import typing
import unittest
from pathlib import Path
from types import ModuleType, SimpleNamespace

from .. import ast as codon_ast
from .. import ir
from ..cache import Cache
from ..passes import translate
from .transform import PassManager


def port_modules():
    for package in (ir, translate):
        yield package
        for item in pkgutil.walk_packages(package.__path__, package.__name__ + "."):
            yield importlib.import_module(item.name)


class PythonPortTests(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
