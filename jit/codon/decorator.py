# Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

from argparse import ArgumentError
import ctypes
import inspect
import sys
import os
import functools
import itertools
import ast
import textwrap
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
        obj_name = obj.__name__
    elif callable(obj) or isinstance(obj, str):
        is_str = isinstance(obj, str)
        lines = [i + '\n' for i in obj.split('\n')] if is_str else inspect.getsourcelines(obj)[0]
        if not is_str: lines = lines[1:]
        obj_str = textwrap.dedent(''.join(lines))

        pyvars = kwargs.get("pyvars", None)
        if pyvars:
            for i in pyvars:
                if not isinstance(i, str):
                    raise ValueError("pyvars only takes string literals")
            node = ast.fix_missing_locations(
                RewriteFunctionArgs(pyvars).visit(ast.parse(obj_str))
            )
            obj_str = astunparse.unparse(node)
        if is_str:
            try:
                obj_name = ast.parse(obj_str).body[0].name
            except:
                raise ValueError("cannot infer function name!")
        else:
            obj_name = obj.__name__
    else:
        raise TypeError("Function or class expected, got " + type(obj).__name__)
    return obj_name, obj_str.replace("_@par", "@par")


def _parse_decorated(obj, **kwargs):
    return  _obj_to_str(obj, **kwargs)


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


def _jit_register_fn(f, pyvars, debug):
    try:
        obj_name, obj_str = _parse_decorated(f, pyvars=pyvars)
        fn, fl = "<internal>", 1
        if hasattr(f, "__code__"):
            fn, fl = f.__code__.co_filename, f.__code__.co_firstlineno
        _jit.execute(obj_str, fn, fl, 1 if debug else 0)
        return obj_name
    except JITError:
        _reset_jit()
        raise

def _jit_callback_fn(obj_name, module, debug=None, sample_size=5, pyvars=None, *args, **kwargs):
    try:
        args = (*args, *kwargs.values())
        types = _codon_types(args, debug=debug, sample_size=sample_size)
        if debug:
            print("[python] {}({})".format(obj_name, list(types)), file=sys.stderr)
        return _jit.run_wrapper(
            obj_name, list(types), module, list(pyvars), args, 1 if debug else 0
        )
    except JITError:
        _reset_jit()
        raise

def _jit_str_fn(fstr, debug=None, sample_size=5, pyvars=None):
    obj_name = _jit_register_fn(fstr, pyvars, debug)
    def wrapped(*args, **kwargs):
        return _jit_callback_fn(obj_name, "__main__", debug, sample_size, pyvars, *args, **kwargs)
    return wrapped


def jit(fn=None, debug=None, sample_size=5, pyvars=None):
    if not pyvars:
        pyvars = []
    if not isinstance(pyvars, list):
        raise ArgumentError("pyvars must be a list")

    if fn and isinstance(fn, str):
        return _jit_str_fn(fn, debug, sample_size, pyvars)

    def _decorate(f):
        obj_name = _jit_register_fn(f, pyvars, debug)
        @functools.wraps(f)
        def wrapped(*args, **kwargs):
            return _jit_callback_fn(obj_name, f.__module__, debug, sample_size, pyvars, *args, **kwargs)
        return wrapped
    return _decorate(fn) if fn else _decorate


def execute(code, debug=False):
    try:
        _jit.execute(code, "<internal>", 0, int(debug))
    except JITError:
        _reset_jit()
        raise
