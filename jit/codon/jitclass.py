# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
from __future__ import annotations

import ast
import functools
import inspect
import re
import sys
import textwrap
import weakref
import astunparse

from typing import Any

from .decorator import (
    JITCallable,
    JITError,
    _jit,
    _reset_jit,
    debug_override,
)


def _jitclass_default_init(self):
    pass


class JITClassMeta:
    def __init__(self, py_cls, native_class_name):
        self.py_cls = py_cls
        self.class_name = py_cls.__name__
        self.native_class_name = native_class_name
        self.source_file = inspect.getsourcefile(py_cls) or "<internal>"
        self.init = None
        self.methods = {}
        self.fields = {}
        self.has_fields = False

    def bind_init(self, allow_default=False):
        init = self.py_cls.__dict__.get("__init__")
        if init is None:
            if not allow_default:
                raise JITError("jitclass requires an __init__ method")
            init = _jitclass_default_init
        self.init = init

    def bind_methods(self, method_names):
        self.methods = {name: self.py_cls.__dict__[name] for name in method_names}

    def bind_fields(self, fields):
        self.fields = dict(fields)


class JITClassASTTransformer(ast.NodeTransformer):
    def __init__(self, meta: JITClassMeta):
        self.meta = meta
        self.has_fields = False
        self.has_init = False
        self.fields = []
        self.method_names = []

    def visit_Module(self, node):
        class_nodes = [stmt for stmt in node.body if isinstance(stmt, ast.ClassDef)]
        if not class_nodes:
            raise JITError("jitclass expects a class definition")

        node.body = [self.visit(class_nodes[0])]
        return node

    def visit_ClassDef(self, node):
        if node.bases or node.keywords:
            raise JITError("jitclass does not support inheritance")

        node.decorator_list = []
        node.name = self.meta.native_class_name
        node.body = [self._visit_class_stmt(stmt) for stmt in node.body]
        node.body.extend(self._make_field_accessors())
        return node

    def _visit_class_stmt(self, stmt):
        if isinstance(stmt, ast.AnnAssign):
            if not isinstance(stmt.target, ast.Name):
                raise JITError("jitclass fields must be simple names")
            if stmt.value is not None:
                raise JITError("jitclass fields cannot have default values")
            self.fields.append((stmt.target.id, stmt.annotation))
            self.has_fields = True
            return stmt

        if isinstance(stmt, ast.FunctionDef):
            if stmt.name.startswith("_codon_jitclass_"):
                raise JITError(
                    "method name '{}' is reserved for jitclass".format(stmt.name)
                )
            if stmt.decorator_list:
                raise JITError("jitclass methods cannot have decorators")
            _validate_method_args(stmt.args, stmt.name)
            if stmt.name == "__init__":
                self.has_init = True
            else:
                self.method_names.append(stmt.name)
            return stmt

        if isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Constant):
            return stmt

        if isinstance(stmt, ast.Pass):
            return stmt

        raise JITError(
            "unsupported statement in jitclass '{}'".format(self.meta.class_name)
        )

    def _make_field_accessors(self):
        accessors = []
        for field_name, annotation in self.fields:
            accessors.append(self._make_field_getter(field_name, annotation))
            accessors.append(self._make_field_setter(field_name, annotation))
        return accessors

    def _make_field_getter(self, field_name, annotation):
        return ast.FunctionDef(
            name=_field_getter_name(field_name),
            args=ast.arguments(
                posonlyargs=[],
                args=[ast.arg(arg="self", annotation=None)],
                vararg=None,
                kwonlyargs=[],
                kw_defaults=[],
                kwarg=None,
                defaults=[],
            ),
            body=[
                ast.Return(
                    value=ast.Attribute(
                        value=ast.Name(id="self", ctx=ast.Load()),
                        attr=field_name,
                        ctx=ast.Load(),
                    )
                )
            ],
            decorator_list=[],
            returns=annotation,
            type_comment=None,
        )

    def _make_field_setter(self, field_name, annotation):
        return ast.FunctionDef(
            name=_field_setter_name(field_name),
            args=ast.arguments(
                posonlyargs=[],
                args=[
                    ast.arg(arg="self", annotation=None),
                    ast.arg(arg="value", annotation=annotation),
                ],
                vararg=None,
                kwonlyargs=[],
                kw_defaults=[],
                kwarg=None,
                defaults=[],
            ),
            body=[
                ast.Assign(
                    targets=[
                        ast.Attribute(
                            value=ast.Name(id="self", ctx=ast.Load()),
                            attr=field_name,
                            ctx=ast.Store(),
                        )
                    ],
                    value=ast.Name(id="value", ctx=ast.Load()),
                ),
                ast.Return(value=ast.Name(id="value", ctx=ast.Load())),
            ],
            decorator_list=[],
            returns=annotation,
            type_comment=None,
        )


class JITClassProxy:
    def __init__(self, meta: JITClassMeta, native_cls, handle, debug=0):
        self.meta = meta
        self.native_cls = native_cls
        self.handle = handle
        self.debug = debug
        self.closed = False

    def ensure_alive(self):
        if self.closed:
            raise JITError("jitclass object has been released")

    def close(self):
        if self.closed:
            return

        handle = self.handle
        self.closed = True
        self.handle = 0

        _jit.jitclass_release(
            self.meta.class_name,
            handle,
            int(self.debug > 0),
        )


def _close_jitclass_proxy(proxy, suppress=False):
    try:
        proxy.close()
    except Exception:
        if not suppress:
            raise


class JITNativeClass:
    def __init__(self, *args, **kwargs):
        proxy = type(self).__codon_constructor__(self, *args, **kwargs)
        object.__setattr__(self, "__codon_jitclass_proxy__", proxy)
        object.__setattr__(self, "__codon_handle__", proxy.handle)
        object.__setattr__(
            self,
            "__codon_finalizer__",
            weakref.finalize(self, _close_jitclass_proxy, proxy, True),
        )

    def close(self):
        proxy = getattr(self, "__codon_jitclass_proxy__", None)
        if proxy is not None:
            proxy.close()
            object.__setattr__(self, "__codon_handle__", proxy.handle)

        finalizer = getattr(self, "__codon_finalizer__", None)
        if finalizer is not None:
            finalizer.detach()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def __setattr__(self, name, value):
        if name.startswith("__codon_"):
            object.__setattr__(self, name, value)
            return

        field = getattr(type(self), name, None)
        if isinstance(field, JITField):
            field.__set__(self, value)
            return

        raise AttributeError(
            "jitclass '{}' has no field '{}'".format(type(self).__name__, name)
        )


class JITClassCtor(JITCallable):
    def __init__(self, meta: JITClassMeta, init: Any, debug=0, sample_size=5):
        super().__init__(init, init.__name__, init.__module__, debug, sample_size)
        self.meta = meta

    def __call__(self, obj, *args, **kwargs):
        def run():
            bound_args = self.bind_args((obj,) + args, kwargs, drop_self=True)
            arg_types = self.codon_types(bound_args)

            if self.debug == 2:
                print(
                    f"[python] {self.meta.class_name}.{self.py_func.__name__}({bound_args})",
                    file=sys.stderr,
                )
            handle = _jit.jitclass_new(
                self.meta.class_name,
                self.meta.native_class_name,
                list(arg_types),
                bound_args,
                int(self.debug > 0),
            )
            return JITClassProxy(self.meta, type(obj), handle, self.debug)

        return self.reset_on_jit_error(run)


class JITMethod(JITCallable):
    def __init__(self, meta: JITClassMeta, method: Any, debug=0, sample_size=5):
        super().__init__(method, method.__name__, method.__module__, debug, sample_size)
        self.meta = meta

    def __get__(self, obj, owner):
        if obj is None:
            return self

        @functools.wraps(self.py_func)
        def bound(*args, **kwargs):
            return self(obj, *args, **kwargs)

        return bound

    def __call__(self, obj, *args, **kwargs):
        def run():
            proxy = obj.__codon_jitclass_proxy__
            proxy.ensure_alive()
            bound_args = self.bind_args((obj,) + args, kwargs, drop_self=True)
            arg_types = self.codon_types(bound_args)

            if self.debug == 2:
                print(
                    f"[python] {self.meta.class_name}.{self.py_func.__name__}({bound_args})",
                    file=sys.stderr,
                )
            return _jit.jitclass_call(
                self.meta.class_name,
                proxy.handle,
                self.py_func.__name__,
                list(arg_types),
                bound_args,
                int(self.debug > 0),
            )

        return self.reset_on_jit_error(run)


class JITField(JITCallable):
    def __init__(self, meta: JITClassMeta, field_name: str, debug=0, sample_size=5):
        super().__init__(None, field_name, meta.py_cls.__module__, debug, sample_size)
        self.meta = meta
        self.field_name = field_name

    def __get__(self, obj, owner):
        if obj is None:
            return self

        def run():
            proxy = obj.__codon_jitclass_proxy__
            proxy.ensure_alive()
            if self.debug == 2:
                print(
                    f"[python] {self.meta.class_name}.{self.field_name}",
                    file=sys.stderr,
                )
            return _jit.jitclass_call(
                self.meta.class_name,
                proxy.handle,
                _field_getter_name(self.field_name),
                [],
                (),
                int(self.debug > 0),
            )

        return self.reset_on_jit_error(run)

    def __set__(self, obj, value):
        def run():
            proxy = obj.__codon_jitclass_proxy__
            proxy.ensure_alive()
            bound_args = (value,)
            arg_types = self.codon_types(bound_args)
            if self.debug == 2:
                print(
                    f"[python] {self.meta.class_name}.{self.field_name} = {value!r}",
                    file=sys.stderr,
                )
            _jit.jitclass_call(
                self.meta.class_name,
                proxy.handle,
                _field_setter_name(self.field_name),
                list(arg_types),
                bound_args,
                int(self.debug > 0),
            )

        return self.reset_on_jit_error(run)


class JITClassCreator:
    def __init__(self, py_cls, debug=0, sample_size=5):
        if not inspect.isclass(py_cls):
            raise TypeError("jitclass expects a class, got " + type(py_cls).__name__)

        self.meta = JITClassMeta(py_cls, _make_native_class_name(py_cls))
        self.debug = (
            debug_override if debug_override else (0 if debug is None else debug)
        )
        self.sample_size = sample_size
        self.compile()

    def create(self):
        meta = self.meta
        namespace = {
            "__module__": meta.py_cls.__module__,
            "__qualname__": meta.py_cls.__qualname__,
            "__doc__": meta.py_cls.__doc__,
            "__codon_jitclass_meta__": meta,
            "__codon_jitclass_name__": meta.class_name,
            "__codon_constructor__": JITClassCtor(
                meta, meta.init, self.debug, self.sample_size
            ),
        }

        for method_name, method in meta.methods.items():
            namespace[method_name] = JITMethod(
                meta, method, self.debug, self.sample_size
            )

        for field_name in meta.fields:
            namespace[field_name] = JITField(
                meta, field_name, self.debug, self.sample_size
            )

        return type(meta.py_cls.__name__, (JITNativeClass,), namespace)

    def compile(self):
        class_code = self._parse()

        if self.debug == 2:
            print(f"[jit_debug] execute:\n{class_code}", file=sys.stderr)
        try:
            _jit.execute(
                class_code, self.meta.source_file, 1, int(self.debug > 0)
            )
        except JITError:
            _reset_jit()
            raise

    def _parse(self):
        meta = self.meta
        src = textwrap.dedent(inspect.getsource(meta.py_cls))
        mod = ast.parse(src)
        transformer = JITClassASTTransformer(meta)
        mod = transformer.visit(mod)

        meta.has_fields = transformer.has_fields
        meta.bind_init(allow_default=not transformer.has_init and not meta.has_fields)
        meta.bind_methods(transformer.method_names)
        meta.bind_fields(transformer.fields)

        if transformer.has_init and not meta.has_fields:
            raise JITError("jitclass requires explicit field annotations")

        mod = ast.fix_missing_locations(mod)
        return astunparse.unparse(mod).replace("_@par", "@par")


def jitclass(cls=None, debug=0):
    def _decorate(t):
        try:
            return JITClassCreator(t, debug).create()
        except JITError:
            _reset_jit()
            raise

    return _decorate(cls) if cls else _decorate


def _validate_method_args(args, method_name):
    if args.vararg or args.kwarg:
        raise JITError("jitclass does not support *args or **kwargs")
    if args.kwonlyargs:
        raise JITError("jitclass does not support keyword-only arguments")
    if not args.args or args.args[0].arg != "self":
        raise JITError("jitclass method '{}' must take self".format(method_name))


def _field_getter_name(field_name):
    return "_codon_jitclass_get_{}".format(field_name)


def _field_setter_name(field_name):
    return "_codon_jitclass_set_{}".format(field_name)


def _make_native_class_name(py_cls):
    qualname = getattr(py_cls, "__qualname__", py_cls.__name__)
    name = re.sub(r"\W", "_", qualname.replace(".", "_"))
    return f"__codon_jitclass_{name}"
