# Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

from argparse import ArgumentError
import ctypes
import inspect
import sys
import os
import functools
import itertools
import ast
import shutil
import astunparse
from pathlib import Path

sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)

from .codon_jit import JITWrapper, JITError, codon_library

if "CODON_PATH" not in os.environ:
    codon_path = []
    codon_lib_path = codon_library()
    if codon_lib_path:
        codon_path.append(Path(codon_lib_path).parent / "stdlib")
    codon_path.append(
        Path(os.path.expanduser("~")) / ".codon" / "lib" / "codon" / "stdlib"
    )
    for path in codon_path:
        if path.exists():
            os.environ["CODON_PATH"] = str(path.resolve())
            break
    else:
        raise RuntimeError(
            "Cannot locate Codon. Please install Codon or set CODON_PATH."
        )

pod_conversions = {
    type(None): "pyobj",
    int: "int",
    float: "float",
    bool: "bool",
    str: "str",
    complex: "complex",
    slice: "slice",
}

custom_conversions = {}
_error_msgs = set()


def _common_type(t, debug, sample_size):
    sub, is_optional = None, False
    for i in itertools.islice(t, sample_size):
        if i is None:
            is_optional = True
        else:
            s = _codon_type(i, debug=debug, sample_size=sample_size)
            if sub and sub != s:
                return "pyobj"
            sub = s
    if is_optional and sub and sub != "pyobj":
        sub = "Optional[{}]".format(sub)
    return sub if sub else "pyobj"


def _codon_type(arg, **kwargs):
    t = type(arg)

    s = pod_conversions.get(t, "")
    if s:
        return s
    if issubclass(t, list):
        return "List[{}]".format(_common_type(arg, **kwargs))
    if issubclass(t, set):
        return "Set[{}]".format(_common_type(arg, **kwargs))
    if issubclass(t, dict):
        return "Dict[{},{}]".format(
            _common_type(arg.keys(), **kwargs), _common_type(arg.values(), **kwargs)
        )
    if issubclass(t, tuple):
        return "Tuple[{}]".format(",".join(_codon_type(a, **kwargs) for a in arg))
    s = custom_conversions.get(t, "")
    if s:
        j = ",".join(_codon_type(getattr(arg, slot), **kwargs) for slot in t.__slots__)
        return "{}[{}]".format(s, j)

    debug = kwargs.get("debug", None)
    if debug:
        msg = "cannot convert " + t.__name__
        if msg not in _error_msgs:
            print("[python]", msg, file=sys.stderr)
            _error_msgs.add(msg)
    return "pyobj"


def _codon_types(args, **kwargs):
    return tuple(_codon_type(arg, **kwargs) for arg in args)


def _reset_jit():
    global _jit
    _jit = JITWrapper()
    init_code = (
        "from internal.python import "
        "setup_decorator, PyTuple_GetItem, PyObject_GetAttrString\n"
        "setup_decorator()\n"
    )
    _jit.execute(init_code, "", 0, False)
    return _jit


_jit = _reset_jit()


class RewriteFunctionArgs(ast.NodeTransformer):
    def __init__(self, args):
        self.args = args

    def visit_FunctionDef(self, node):
        for a in self.args:
            node.args.args.append(ast.arg(arg=a, annotation=None))
        return node


def _obj_to_str(obj, **kwargs) -> str:
    if inspect.isclass(obj):
        lines = inspect.getsourcelines(obj)[0]
        extra_spaces = lines[0].find("class")
        obj_str = "".join(l[extra_spaces:] for l in lines)
    elif callable(obj):
        lines = inspect.getsourcelines(obj)[0]
        extra_spaces = lines[0].find("@")
        obj_str = "".join(l[extra_spaces:] for l in lines[1:])
        if kwargs.get("pyvars", None):
            node = ast.fix_missing_locations(
                RewriteFunctionArgs(kwargs["pyvars"]).visit(ast.parse(obj_str))
            )
            obj_str = astunparse.unparse(node)
    else:
        raise TypeError("Function or class expected, got " + type(obj).__name__)
    return obj_str.replace("_@par", "@par")


def _obj_name(obj) -> str:
    if inspect.isclass(obj) or callable(obj):
        return obj.__name__
    else:
        raise TypeError("Function or class expected, got " + type(obj).__name__)


def _parse_decorated(obj, **kwargs):
    return _obj_name(obj), _obj_to_str(obj, **kwargs)


def convert(t):
    if not hasattr(t, "__slots__"):
        raise JITError("class '{}' does not have '__slots__' attribute".format(str(t)))

    name = t.__name__
    slots = t.__slots__
    code = (
        "@tuple\n"
        "class "
        + name
        + "["
        + ",".join("T{}".format(i) for i in range(len(slots)))
        + "]:\n"
    )
    for i, slot in enumerate(slots):
        code += "    {}: T{}\n".format(slot, i)

    # PyObject_GetAttrString
    code += "    def __from_py__(p: cobj):\n"
    for i, slot in enumerate(slots):
        code += "        a{} = T{}.__from_py__(PyObject_GetAttrString(p, '{}'.ptr))\n".format(
            i, i, slot
        )
    code += "        return {}({})\n".format(
        name, ", ".join("a{}".format(i) for i in range(len(slots)))
    )

    _jit.execute(code, "", 0, False)
    custom_conversions[t] = name
    return t


def jit(fn=None, debug=None, sample_size=5, pyvars=None):
    if not pyvars:
        pyvars = []
    if not isinstance(pyvars, list):
        raise ArgumentError("pyvars must be a list")

    def _decorate(f):
        try:
            obj_name, obj_str = _parse_decorated(f, pyvars=pyvars)
            _jit.execute(
                obj_str,
                f.__code__.co_filename,
                f.__code__.co_firstlineno,
                1 if debug else 0,
            )
        except JITError:
            _reset_jit()
            raise

        @functools.wraps(f)
        def wrapped(*args, **kwargs):
            try:
                args = (*args, *kwargs.values())
                types = _codon_types(args, debug=debug, sample_size=sample_size)
                if debug:
                    print(
                        "[python] {}({})".format(f.__name__, list(types)),
                        file=sys.stderr,
                    )
                return _jit.run_wrapper(
                    obj_name, types, f.__module__, pyvars, args, 1 if debug else 0
                )
            except JITError:
                _reset_jit()
                raise

        return wrapped

    if fn:
        return _decorate(fn)
    return _decorate
