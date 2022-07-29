import ctypes
import inspect
import importlib
import importlib.util
import sys
import os

sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)

if "CODON_PATH" not in os.environ:
    os.environ["CODON_PATH"] = os.path.dirname(
        os.path.abspath(inspect.getfile(inspect.currentframe()))
    )
    os.environ["CODON_PATH"] += "/stdlib"

from .codon_jit import Jit, JitError

convertible = {type(None): "NoneType",
               int: "int",
               float: "float",
               bool: "bool",
               str: "str",
               list: "List",
               dict: "Dict",
               set: "Set",
               tuple: "Tuple"}

def _codon_type(arg):
    t = type(arg)
    s = convertible.get(t, "pyobj")

    if issubclass(t, list) or issubclass(t, set):
        sub = "NoneType"
        x = next(iter(arg), None)
        if x is not None:
            sub = _codon_type(x)
        s += f"[{sub}]"
    elif issubclass(t, dict):
        sub1 = "NoneType"
        sub2 = "NoneType"
        x = next(arg.items(), None)
        if x is not None:
            sub1 = _codon_type(x[0])
            sub2 = _codon_type(x[1])
        s += f"[{sub1},{sub2}]"
    elif issubclass(t, tuple):
        s += f"[{','.join(_codon_type(a) for a in arg)}]"

    return s

def _codon_types(args):
    return tuple(_codon_type(arg) for arg in args)

def _wrapper_stub_init():
    from internal.python import (
        pyobj,
        ensure_initialized,
        Py_None,
        PyImport_AddModule,
        PyObject_GetAttrString,
        PyObject_SetAttrString,
        PyTuple_GetItem,
    )

    ensure_initialized(True)

def _reset_jit():
    global jit
    jit = Jit()
    lines = inspect.getsourcelines(_wrapper_stub_init)[0][1:]
    jit.execute("".join([l[4:] for l in lines]))
    return jit

jit = _reset_jit()

def _obj_to_str(obj) -> str:
    if inspect.isclass(obj):
        lines = inspect.getsourcelines(obj)[0]
        extra_spaces = lines[0].find("class")
        obj_str = "".join(l[extra_spaces:] for l in lines)
    elif callable(obj):
        lines = inspect.getsourcelines(obj)[0]
        extra_spaces = lines[0].find("@")
        obj_str = "".join(l[extra_spaces:] for l in lines[1:])
    else:
        raise TypeError(f"Function or class expected, got {type(obj).__name__}.")
    return obj_str.replace("_@par", "@par")

def _obj_name(obj) -> str:
    if inspect.isclass(obj) or callable(obj):
        return obj.__name__
    else:
        raise TypeError(f"Function or class expected, got {type(obj).__name__}.")

def _parse_decorated(obj):
    return _obj_name(obj), _obj_to_str(obj)

def codon(obj):
    try:
        obj_name, obj_str = _parse_decorated(obj)
        jit.execute(obj_str)
    except JitError as e:
        _reset_jit()
        raise

    def wrapped(*args, **kwargs):
        try:
            args = (*args, *kwargs.values())
            types = _codon_types(args)
            return jit.run_wrapper(obj_name, types, args)
        except JitError as e:
            _reset_jit()
            raise

    return wrapped
