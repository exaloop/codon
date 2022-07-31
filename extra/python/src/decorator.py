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

from .codon_jit import JITWrapper, JITError

pod_conversions = {type(None): "NoneType",
                   int: "int",
                   float: "float",
                   bool: "bool",
                   str: "str",
                   complex: "complex",
                   slice: "slice"}

custom_conversions = {}

def _codon_type(arg):
    t = type(arg)

    if s := pod_conversions.get(t, ""):
        return s

    if issubclass(t, list):
        sub = "NoneType"
        x = next(iter(arg), None)
        if x is not None:
            sub = _codon_type(x)
        return f"List[{sub}]"

    if issubclass(t, set):
        sub = "NoneType"
        x = next(iter(arg), None)
        if x is not None:
            sub = _codon_type(x)
        return f"Set[{sub}]"

    if issubclass(t, dict):
        sub1 = "NoneType"
        sub2 = "NoneType"
        x = next(iter(arg.items()), None)
        if x is not None:
            sub1 = _codon_type(x[0])
            sub2 = _codon_type(x[1])
        return f"Dict[{sub1},{sub2}]"

    if issubclass(t, tuple):
        return f"Tuple[{','.join(_codon_type(a) for a in arg)}]"

    if s := custom_conversions.get(t, ""):
        return f"{s}[{','.join(_codon_type(getattr(arg, slot)) for slot in t.__slots__)}]"

    return "pyobj"

def _codon_types(args):
    return tuple(_codon_type(arg) for arg in args)

def _reset_jit():
    global _jit
    _jit = JITWrapper()
    init_code = ("from internal.python import "
                 "setup_decorator, PyTuple_GetItem, PyObject_GetAttrString\n"
                 "setup_decorator()\n")
    _jit.execute(init_code)
    return _jit

_jit = _reset_jit()

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

def convert(t):
    if not hasattr(t, "__slots__"):
        raise JITError(f"class '{str(t)}' does not have '__slots__' attribute")

    name = t.__name__
    slots = t.__slots__
    code = ("@tuple\n"
            "class " + name + "[" + ",".join(f"T{i}" for i in range(len(slots))) + "]:\n")
    for i, slot in enumerate(slots):
        code += f"    {slot}: T{i}\n"

    # PyObject_GetAttrString
    code += "    def __from_py__(p: cobj):\n"
    for i, slot in enumerate(slots):
        code += f"        a{i} = T{i}.__from_py__(PyObject_GetAttrString(p, '{slot}'.ptr))\n"
    code += f"        return {name}({', '.join(f'a{i}' for i in range(len(slots)))})\n"

    _jit.execute(code)
    custom_conversions[t] = name
    return t

def jit(obj):
    try:
        obj_name, obj_str = _parse_decorated(obj)
        _jit.execute(obj_str)
    except JITError as e:
        _reset_jit()
        raise

    def wrapped(*args, **kwargs):
        try:
            args = (*args, *kwargs.values())
            types = _codon_types(args)
            return _jit.run_wrapper(obj_name, types, args)
        except JITError as e:
            _reset_jit()
            raise

    return wrapped
