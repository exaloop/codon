# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..attribute import KeyValueAttribute
from ..base import cast, is_a
from ..const import IntConst
from ..flow import SeriesFlow
from ..func import BodiedFunc, Func, Intrinsic
from ..instr import AssignInstr, CallInstr, ExtractInstr, FlowInstr
from ..type import FuncType, IntType, RecordType
from ..var import Var, VarValue


# C++: codon/cir/util/irtools.cpp:20
def has_attribute(fn, attr):
    """C++: codon/cir/util/irtools.cpp:23. Test a function attribute."""
    attributes = fn.get_attribute(KeyValueAttribute)
    return attributes.has(attr) if attributes else False


def _attribute_name(name):
    from ...ast.types import mangle

    return mangle("std.internal.attributes", func=name)


# C++: codon/cir/util/irtools.cpp:27
def is_export(fn):
    return has_attribute(fn, _attribute_name("export"))


# C++: codon/cir/util/irtools.cpp:29
def set_export(fn, value):
    attrs = fn.get_attribute(KeyValueAttribute)
    if attrs is None:
        if not value:
            return
        attrs = KeyValueAttribute()
        fn.set_attribute(attrs)
    if value:
        attrs.attributes[_attribute_name("export")] = ""
    else:
        attrs.attributes.pop(_attribute_name("export"), None)


def is_gpu_kernel(fn):
    from ...ast.types import mangle

    return has_attribute(fn, mangle("std.internal.gpu", func="kernel"))


# C++: codon/cir/util/irtools.cpp:45
def is_inline(fn):
    return has_attribute(fn, _attribute_name("inline"))


# C++: codon/cir/util/irtools.cpp:47
def is_no_inline(fn):
    return has_attribute(fn, _attribute_name("noinline"))


# C++: codon/cir/util/irtools.cpp:49
def get_derived_func_name(name, parent, site=None):
    owner = parent.name
    result = f"{name}.context.{len(owner.encode())}:{owner}"
    if site:
        result += f".{site.info.line}:{site.info.col}"
    return result


# C++: codon/cir/util/irtools.cpp:60
def is_stdlib_func(fn, submodule):
    attr = fn.get_attribute(KeyValueAttribute)
    return attr.get(".module").startswith("std::" + submodule) if attr else False


# C++: codon/cir/util/irtools.cpp:68
def is_int(typ, length=64, sign=True):
    typ = cast(IntType, typ)
    return typ is not None and typ.len == length and typ.sign == sign


# C++: codon/cir/util/irtools.cpp:74
def call(fn, args):
    """C++: irtools.cpp:74. Construct a call through an unowned function value."""
    return fn.module.nr(CallInstr, fn.module.nr(VarValue, fn), args)


# C++: codon/cir/util/irtools.cpp:79
def virtual_call(callee, args, thunk_id, parent):
    assert parent and thunk_id and args, "virtual call requires a parent, method ID and receiver"
    module, args = callee.module, list(args)
    receiver = args[0]
    temporary = module.nr(Var, receiver.type, False, False, False, "rtti.self")
    parent.symbols.append(temporary)
    signature = module.unsafe_get_func_type(
        f"rtti.lookup[{receiver.type.name}].{callee.type.name}",
        callee.type,
        [receiver.type, module.get_int_type()],
    )
    lookup = module.get_intrinsic(Intrinsic.VIRTUAL_LOOKUP, receiver.type, signature)
    prefix = module.nr(SeriesFlow)
    prefix.push_back(callee)
    prefix.push_back(module.nr(AssignInstr, temporary, receiver))
    target = module.nr(
        FlowInstr, prefix, call(lookup, [module.nr(VarValue, temporary), module.get_int(thunk_id)])
    )
    args[0] = module.nr(VarValue, temporary)
    return module.n(CallInstr, callee, target, args)


# C++: codon/cir/util/irtools.cpp:103
def is_call_of(value, name, inputs=-1, output=None, method=False):
    value = cast(CallInstr, value)
    if not value:
        return False
    fn = get_func(value.callee)
    if not fn or fn.unmangled_name != name:
        return False
    if isinstance(inputs, int):
        if inputs >= 0 and len(value.args) != inputs:
            return False
    else:
        if len(value.args) != len(inputs):
            return False
        if any(
            typ is not None and not arg.type.is_type(typ) for typ, arg in zip(inputs, value.args)
        ):
            return False
    if output is not None and not value.type.is_type(output):
        return False
    if method:
        if fn.parent_type is None or not value.args:
            return False
        if isinstance(inputs, int):
            return value.args[0].type.is_type(fn.parent_type)
        return inputs[0] is None or fn.parent_type.is_type(inputs[0])
    return True


# C++: codon/cir/util/irtools.cpp:155
def is_magic_method_call(value):
    value = cast(CallInstr, value)
    if not value:
        return False
    fn = get_func(value.callee)
    if (
        not fn
        or not fn.parent_type
        or not value.args
        or not value.args[0].type.is_type(fn.parent_type)
    ):
        return False
    name = fn.unmangled_name
    return len(name) >= 5 and name.startswith("__") and name.endswith("__")


# C++: codon/cir/util/irtools.cpp:174
def make_tuple(args, module=None):
    if module is None:
        assert args, "unknown module for empty tuple construction"
        module = args[0].module
    typ = module.get_tuple_type([arg.type for arg in args])
    constructor = module.get_or_realize_method(typ, "__new__", [arg.type for arg in args])
    assert constructor, "could not realize tuple new function"
    return module.nr(CallInstr, module.nr(VarValue, constructor), args)


_anonymous_global_counter = 1


# C++: codon/cir/util/irtools.cpp:190
def make_var(value, flow, parent, prepend=False):
    global _anonymous_global_counter

    global_ = parent is None
    var = value.module.nr(Var, value.type, global_)
    if global_:
        var.name = f".anon_global.{_anonymous_global_counter}"
        _anonymous_global_counter += 1
    assignment = value.module.nr(AssignInstr, var, value)
    if prepend:
        flow.series.insert(0, assignment)
    else:
        flow.series.append(assignment)
    if not global_:
        parent.symbols.append(var)
    return var


# C++: codon/cir/util/irtools.cpp:210
def alloc(typ, count):
    if isinstance(count, int):
        count = typ.module.get_int(count)
    return typ.module.get_pointer_type(typ)(count)


# C++: codon/cir/util/irtools.cpp:221
def get_var(value):
    value = cast(VarValue, value)
    return value.var if value and not is_a(Func, value.var) else None


# C++: codon/cir/util/irtools.cpp:106
def get_func(value):
    value = cast(VarValue, value)
    return cast(Func, value.var) if value else None


# C++: codon/cir/util/irtools.cpp:261
def ptr_load(pointer):
    value = pointer[pointer.module.get_int(0)]
    assert value, f"pointer getitem not found [{pointer.info}]"
    return value


# C++: codon/cir/util/irtools.cpp:268
def ptr_store(pointer, value):
    module = pointer.module
    fn = module.get_or_realize_method(
        pointer.type, module.SETITEM_MAGIC_NAME, [pointer.type, module.get_int_type(), value.type]
    )
    assert fn, f"pointer setitem not found [{pointer.info}]"
    return call(fn, [pointer, module.get_int(0), value])


# C++: codon/cir/util/irtools.cpp:277
def tuple_get(value, index):
    return value.module.nr(ExtractInstr, value, f"item{index + 1}")


# C++: codon/cir/util/irtools.cpp:282
def tuple_store(value, index, new):
    typ = cast(RecordType, value.type)
    assert typ, "argument is not a tuple"
    return make_tuple(
        [new if idx == index else tuple_get(value, idx) for idx in range(len(typ.fields))],
        value.module,
    )


# C++: codon/cir/util/irtools.cpp:293
def get_stdlib_func(value, name, submodule):
    fn = cast(BodiedFunc, get_func(value))
    return fn if fn and is_stdlib_func(fn, submodule) and fn.unmangled_name == name else None


# C++: codon/cir/util/irtools.cpp:317
def get_return_type(fn):
    return cast(FuncType, fn.type).return_type


# C++: codon/cir/util/irtools.cpp:321
def set_return_type(fn, typ):
    signature = cast(FuncType, fn.type)
    assert signature, "not a function type"
    fn.type = fn.module.get_func_type(typ, signature.arg_types)


# C++: codon/cir/util/irtools.h:141
def series(*values):
    """C++: irtools.h:138. Construct a series, or return null for no contents."""
    if not values:
        return None
    result = values[0].module.nr(SeriesFlow)
    result.series.extend(values)
    return result


# C++: codon/cir/util/irtools.h:157
def is_const(value, cls=IntConst):
    return cast(cls, value) is not None


# C++: codon/cir/util/irtools.h:177
def get_const(value, cls=IntConst):
    constant = cast(cls, value)
    assert constant is not None, "not a constant"
    return constant.val
