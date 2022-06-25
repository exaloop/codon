import ctypes
import inspect
import importlib
import importlib.util
import sys
import os

from typing import List, Tuple

sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)

if "CODON_PATH" not in os.environ:
    os.environ["CODON_PATH"] = os.path.dirname(
        os.path.abspath(inspect.getfile(inspect.currentframe()))
    )
    os.environ["CODON_PATH"] += "/stdlib"


from .codon_jit import Jit, JitError


separator = "__"


# codon wrapper stubs


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

    module = PyImport_AddModule("__codon_interop__".c_str())


def _wrapper_stub_header():
    argt = PyObject_GetAttrString(module, "__codon_args__".c_str())


def _wrapper_stub_footer_ret():
    PyObject_SetAttrString(module, "__codon_ret__".c_str(), ret)


def _wrapper_stub_footer_void():
    pyobj.incref(Py_None)
    PyObject_SetAttrString(module, "__codon_ret__".c_str(), Py_None)


# helpers


def _reset_jit():
    global jit
    jit = Jit()
    lines = inspect.getsourcelines(_wrapper_stub_init)[0][1:]
    jit.execute("".join([l[4:] for l in lines]))

    return jit


def _init():
    spec = importlib.machinery.ModuleSpec("__codon_interop__", None)
    module = importlib.util.module_from_spec(spec)
    exec("__codon_args__ = ()\n__codon_ret__ = None", module.__dict__)
    sys.modules["__codon_interop__"] = module
    exec("import __codon_interop__")

    return _reset_jit(), module


jit, module = _init()


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


def _obj_name_full(obj) -> str:
    obj_name = _obj_name(obj)
    obj_name_stack = [obj_name]
    frame = inspect.currentframe()
    while frame.f_code.co_name != "codon":
        frame = frame.f_back
    frame = frame.f_back
    while frame:
        if frame.f_code.co_name == "<module>":
            obj_name_stack += [frame.f_globals["__name__"]]
            break
        else:
            obj_name_stack += [frame.f_code.co_name]
        frame = frame.f_back
    return obj_name, separator.join(reversed(obj_name_stack))


def _parse_decorated(obj):
    return _obj_name(obj), _obj_to_str(obj)


def _get_type_info(obj) -> Tuple[List[str], str]:
    sgn = inspect.signature(obj)
    par = [p.annotation for p in sgn.parameters.values()]
    ret = sgn.return_annotation
    return par, ret


def _type_str(typ) -> str:
    if typ in (int, float, str, bool):
        return typ.__name__
    obj_str = str(typ)
    return obj_str[7:] if obj_str.startswith("typing.") else obj_str


def _build_wrapper(obj, obj_name) -> str:
    arg_types, ret_type = _get_type_info(obj)
    arg_count = len(arg_types)
    wrap_name = f"{obj_name}{separator}wrapped"
    wrap = [f"def {wrap_name}():\n"]
    wrap += inspect.getsourcelines(_wrapper_stub_header)[0][1:]
    wrap += [
        f"    arg_{i} = {_type_str(arg_types[i])}.__from_py__(PyTuple_GetItem(argt, {i}))\n"
        for i in range(arg_count)
    ]
    args = ", ".join([f"arg_{i}" for i in range(arg_count)])
    if ret_type != inspect._empty:
        wrap += [f"    ret = {obj_name}({args}).__to_py__()\n"]
        wrap += inspect.getsourcelines(_wrapper_stub_footer_ret)[0][1:]
    else:
        wrap += [f"    {obj_name}({args})\n"]
        wrap += inspect.getsourcelines(_wrapper_stub_footer_void)[0][1:]
    return wrap_name, "".join(wrap)


# decorator


def codon(obj):
    try:
        obj_name, obj_str = _parse_decorated(obj)
        jit.execute(obj_str)

        wrap_name, wrap_str = _build_wrapper(obj, obj_name)
        jit.execute(wrap_str)
    except JitError as e:
        _reset_jit()
        raise

    def wrapped(*args, **kwargs):
        try:
            module.__codon_args__ = (*args, *kwargs.values())
            stdout = jit.execute(f"{wrap_name}()")
            print(stdout, end="")
            return module.__codon_ret__
        except JitError as e:
            _reset_jit()
            raise

    return wrapped
