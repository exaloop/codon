import ctypes
import inspect
import importlib
import importlib.util
import re
import sys
import warnings

sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)

from codon_jit import Jit


separator = "__"  # use interpunct once unicode is supported


class CodonError(Exception):
    pass


# codon wrapper stubs


def _wrapper_stub_init():
    from internal.python import (
        pyobj,
        ensure_initialized,
        PyImport_AddModule,
        PyObject_GetAttrString,
        PyObject_SetAttrString,
        PyTuple_GetItem,
    )

    ensure_initialized(True)

    module = PyImport_AddModule("__codon_interop__".c_str())


def _wrapper_stub_header():
    argt = PyObject_GetAttrString(module, "__codon_args__".c_str())


def _wrapper_stub_footer():
    PyObject_SetAttrString(module, "__codon_ret__".c_str(), ret.p)


# helpers


def _reset_jit():
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


def _get_type_info(obj) -> tuple[list[str], str]:
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
    arg_types, _ = _get_type_info(obj)
    arg_count = len(arg_types)
    wrap_name = f"{obj_name}{separator}wrapped"
    wrap = [f"def {wrap_name}():\n"]
    wrap += inspect.getsourcelines(_wrapper_stub_header)[0][1:]
    wrap += [
        f"    arg_{i} = {_type_str(arg_types[i])}.__from_py__(pyobj(PyTuple_GetItem(argt, {i})))\n"
        for i in range(arg_count)
    ]
    args = ", ".join([f"arg_{i}" for i in range(arg_count)])
    wrap += [f"    ret = {obj_name}({args}).__to_py__()\n"]
    wrap += inspect.getsourcelines(_wrapper_stub_footer)[0][1:]
    return wrap_name, "".join(wrap)


# decorator


def codon(obj):
    global jit
    try:
        obj_name, obj_str = _parse_decorated(obj)
        jit.execute(obj_str)

        wrap_name, wrap_str = _build_wrapper(obj, obj_name)
        jit.execute(wrap_str)
    except RuntimeError as e:
        jit = _reset_jit()
        raise CodonError() from e

    with warnings.catch_warnings():
        warnings.simplefilter("ignore")

        def wrapped(*args, **kwargs):
            global jit
            try:
                module.__codon_args__ = (*args, *kwargs.values())
                jit.execute(f"{wrap_name}()")
                return module.__codon_ret__
            except RuntimeError as e:
                jit = _reset_jit()
                raise CodonError() from e


        return wrapped
