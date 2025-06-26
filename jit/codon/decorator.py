# Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

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
import numpy as np
from pathlib import Path

sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)

from .codon_jit import JITWrapper, JITError, codon_library

if "CODON_PATH" not in os.environ:
    codon_path = []
    codon_lib_path = codon_library()
    if codon_lib_path:
        codon_path.append(Path(codon_lib_path).parent / "stdlib")
    codon_path.append(
        Path(os.path.expanduser("~")) / ".codon" / "lib" / "codon" / "stdlib")
    for path in codon_path:
        if path.exists():
            os.environ["CODON_PATH"] = str(path.resolve())
            break
    else:
        raise RuntimeError("Cannot locate Codon. Please install Codon or set CODON_PATH.")

debug_override = int(os.environ.get("CODON_JIT_DEBUG", 0))

pod_conversions = {
    type(None): "pyobj",
    int: "int",
    float: "float",
    bool: "bool",
    str: "str",
    complex: "complex",
    slice: "slice",
    np.bool_: "bool",
    np.int8: "i8",
    np.uint8: "u8",
    np.int16: "i16",
    np.uint16: "u16",
    np.int32: "i32",
    np.uint32: "u32",
    np.int64: "int",
    np.uint64: "u64",
    np.float16: "float16",
    np.float32: "float32",
    np.float64: "float",
    np.complex64: "complex64",
    np.complex128: "complex",
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
        return "Dict[{},{}]".format(_common_type(arg.keys(), **kwargs),
                                    _common_type(arg.values(), **kwargs))
    if issubclass(t, tuple):
        return "Tuple[{}]".format(",".join(
            _codon_type(a, **kwargs) for a in arg))
    if issubclass(t, np.ndarray):
        if arg.dtype == np.bool_:
            dtype = "bool"
        elif arg.dtype == np.int8:
            dtype = "i8"
        elif arg.dtype == np.uint8:
            dtype = "u8"
        elif arg.dtype == np.int16:
            dtype = "i16"
        elif arg.dtype == np.uint16:
            dtype = "u16"
        elif arg.dtype == np.int32:
            dtype = "i32"
        elif arg.dtype == np.uint32:
            dtype = "u32"
        elif arg.dtype == np.int64:
            dtype = "int"
        elif arg.dtype == np.uint64:
            dtype = "u64"
        elif arg.dtype == np.float16:
            dtype = "float16"
        elif arg.dtype == np.float32:
            dtype = "float32"
        elif arg.dtype == np.float64:
            dtype = "float"
        elif arg.dtype == np.complex64:
            dtype = "complex64"
        elif arg.dtype == np.complex128:
            dtype = "complex"
        elif arg.dtype.name.startswith("datetime64["):
            units, step = np.datetime_data(arg.dtype)
            dtype = "np.datetime64['{}',{}]".format(units, step)
        elif arg.dtype.name.startswith("timedelta64["):
            units, step = np.datetime_data(arg.dtype)
            dtype = "np.timedelta64['{}',{}]".format(units, step)
        else:
            return "pyobj"
        return "np.ndarray[{},{}]".format(dtype, arg.ndim)

    s = custom_conversions.get(t, "")
    if s:
        j = ",".join(
            _codon_type(getattr(arg, slot), **kwargs) for slot in t.__slots__)
        return "{}[{}]".format(s, j)

    debug = kwargs.get("debug", 0)
    if debug > 0:
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
        "import numpy as np\n"
        "import numpy.pybridge\n"
    )
    if debug_override == 2:
        print(f"[jit_debug] execute:\n{init_code}", file=sys.stderr)
    _jit.execute(init_code, "", 0, int(debug_override > 0))
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
        lines = [i + '\n' for i in obj.split('\n')
                 ] if is_str else inspect.getsourcelines(obj)[0]
        if not is_str:
            lines = lines[1:]
        obj_str = textwrap.dedent(''.join(lines))

        pyvars = kwargs.get("pyvars", None)
        if pyvars:
            for i in pyvars:
                if not isinstance(i, str):
                    raise ValueError("pyvars only takes string literals")
            node = ast.fix_missing_locations(
                RewriteFunctionArgs(pyvars).visit(ast.parse(obj_str)))
            obj_str = astunparse.unparse(node)
        if is_str:
            try:
                obj_name = ast.parse(obj_str).body[0].name
            except:
                raise ValueError("cannot infer function name!")
        else:
            obj_name = obj.__name__
    else:
        raise TypeError("Function or class expected, got " +
                        type(obj).__name__)
    return obj_name, obj_str.replace("_@par", "@par")

def _parse_decorated(obj, **kwargs):
    return _obj_to_str(obj, **kwargs)

def convert(t):
    if not hasattr(t, "__slots__"):
        raise JITError("class '{}' does not have '__slots__' attribute".format(
            str(t)))

    name = t.__name__
    slots = t.__slots__
    code = ("@tuple\n"
            "class " + name + "[" +
            ",".join("T{}".format(i) for i in range(len(slots))) + "]:\n")
    for i, slot in enumerate(slots):
        code += "    {}: T{}\n".format(slot, i)

    # PyObject_GetAttrString
    code += "    def __from_py__(p: cobj):\n"
    for i, slot in enumerate(slots):
        code += "        a{} = T{}.__from_py__(PyObject_GetAttrString(p, '{}'.ptr))\n".format(
            i, i, slot)
    code += "        return {}({})\n".format(
        name, ", ".join("a{}".format(i) for i in range(len(slots))))

    if debug_override == 2:
        print(f"[jit_debug] execute:\n{code}", file=sys.stderr)
    _jit.execute(code, "", 0, int(debug_override > 0))
    custom_conversions[t] = name
    return t

def _jit_register_fn(f, pyvars, debug):
    try:
        obj_name, obj_str = _parse_decorated(f, pyvars=pyvars)
        fn, fl = "<internal>", 1
        if hasattr(f, "__code__"):
            fn, fl = f.__code__.co_filename, f.__code__.co_firstlineno
        if debug == 2:
            print(f"[jit_debug] execute:\n{obj_str}", file=sys.stderr)
        _jit.execute(obj_str, fn, fl, int(debug > 0))
        return obj_name
    except JITError:
        _reset_jit()
        raise

def _jit_callback_fn(fn,
                     obj_name,
                     module,
                     debug=0,
                     sample_size=5,
                     pyvars=None,
                     *args,
                     **kwargs):
    if fn is not None:
        sig = inspect.signature(fn)
        bound_args = sig.bind(*args, **kwargs)
        bound_args.apply_defaults()
        args = tuple(bound_args.arguments[param] for param in sig.parameters)
    else:
        args = (*args, *kwargs.values())

    try:
        types = _codon_types(args, debug=debug, sample_size=sample_size)
        if debug > 0:
            print("[python] {}({})".format(obj_name, list(types)), file=sys.stderr)
        return _jit.run_wrapper(
            obj_name, list(types), module, list(pyvars), args, int(debug > 0)
        )
    except JITError:
        _reset_jit()
        raise

def _jit_str_fn(fstr, debug=0, sample_size=5, pyvars=None):
    obj_name = _jit_register_fn(fstr, pyvars, debug)

    def wrapped(*args, **kwargs):
        return _jit_callback_fn(None, obj_name, "__main__", debug, sample_size,
                                pyvars, *args, **kwargs)

    return wrapped


def jit(fn=None, debug=0, sample_size=5, pyvars=None):
    if debug is None:
        debug = 0
    if not pyvars:
        pyvars = []

    if not isinstance(pyvars, list):
        raise ArgumentError("pyvars must be a list")

    if debug_override:
        debug = debug_override

    if fn and isinstance(fn, str):
        return _jit_str_fn(fn, debug, sample_size, pyvars)

    def _decorate(f):
        obj_name = _jit_register_fn(f, pyvars, debug)

        @functools.wraps(f)
        def wrapped(*args, **kwargs):
            return _jit_callback_fn(f, obj_name, f.__module__, debug,
                                    sample_size, pyvars, *args, **kwargs)

        return wrapped

    return _decorate(fn) if fn else _decorate


def execute(code, debug=0):
    if debug is None:
        debug = 0
    if debug_override:
        debug = debug_override
    try:
        if debug == 2:
            print(f"[jit_debug] execute:\n{code}", file=sys.stderr)
        _jit.execute(code, "<internal>", 0, int(debug))
    except JITError:
        _reset_jit()
        raise
