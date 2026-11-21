# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass
from enum import IntEnum, auto

from ....ast.types import Stdlib, mangle
from ... import CallInstr, RecordType, Type, cast
from ...attribute import KeyValueAttribute
from ...util.irtools import get_func
from ..pass_ import OperatorPass


FUSION_MODULE = "std.numpy.fusion"


def is_array_type(typ):
    return bool(cast(RecordType, typ) and typ.name.startswith(Stdlib.NDArray + "["))


def is_ufunc_type(typ):
    return typ is not None and typ.name.startswith((
        mangle("std.numpy.ufunc", cls="UnaryUFunc") + "[",
        mangle("std.numpy.ufunc", cls="BinaryUFunc") + "[",
    ))


def array_generics(typ):
    base, ndim = typ.get_generics()
    assert base.is_type() and ndim.is_static(), "unrecognized ndarray generics"
    return base.get_type_value(), ndim.get_static_value()


class NumPyPass(OperatorPass):
    def should_run(self, module):
        return any(map(is_array_type, module.types))


@dataclass(init=False, eq=False)
class NumPyPrimitiveTypes:
    none: Type
    bool_: Type
    i8: Type
    u8: Type
    i16: Type
    u16: Type
    i32: Type
    u32: Type
    i64: Type
    u64: Type
    f16: Type
    f32: Type
    f64: Type
    c64: Type | None
    c128: Type | None

    def __init__(self, module):
        self.none, self.bool_ = module.get_none_type(), module.get_bool_type()
        self.i8, self.u8 = module.get_int_type(8, True), module.get_int_type(8, False)
        self.i16, self.u16 = module.get_int_type(16, True), module.get_int_type(16, False)
        self.i32, self.u32 = module.get_int_type(32, True), module.get_int_type(32, False)
        self.i64, self.u64 = module.get_int_type(64, True), module.get_int_type(64, False)
        self.f16, self.f32, self.f64 = (
            module.get_float16_type(), module.get_float32_type(), module.get_float_type()
        )
        self.c64, self.c128 = module.get_type(Stdlib.Complex64), module.get_type(Stdlib.Complex)


class TypeCode(IntEnum):
    NONE = -1
    BOOL = 0
    I8 = auto()
    U8 = auto()
    I16 = auto()
    U16 = auto()
    I32 = auto()
    U32 = auto()
    I64 = auto()
    U64 = auto()
    F16 = auto()
    F32 = auto()
    F64 = auto()
    C64 = auto()
    C128 = auto()
    SCALAR_END = auto()
    ARR_BOOL = auto()
    ARR_I8 = auto()
    ARR_U8 = auto()
    ARR_I16 = auto()
    ARR_U16 = auto()
    ARR_I32 = auto()
    ARR_U32 = auto()
    ARR_I64 = auto()
    ARR_U64 = auto()
    ARR_F16 = auto()
    ARR_F32 = auto()
    ARR_F64 = auto()
    ARR_C64 = auto()
    ARR_C128 = auto()


_BASE_ATTRS = (
    "bool_", "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64",
    "f16", "f32", "f64", "c64", "c128",
)


@dataclass(frozen=True)
class NumPyType:
    dtype: TypeCode = TypeCode.NONE
    ndim: int = 0

    @staticmethod
    def get(typ, types):
        base, ndim, offset = typ, 0, 0
        if is_array_type(typ):
            base, ndim = array_generics(typ)
            offset = TypeCode.ARR_BOOL
        for index, attr in enumerate(_BASE_ATTRS):
            candidate = getattr(types, attr)
            if candidate is not None and base.is_type(candidate):
                return NumPyType(TypeCode(offset + index), ndim)
        return NumPyType()

    def __bool__(self):
        return self.dtype != TypeCode.NONE

    def is_array(self):
        return self.dtype > TypeCode.SCALAR_END

    def get_ir_base_type(self, types):
        index = self.dtype - TypeCode.ARR_BOOL if self.is_array() else self.dtype
        assert 0 <= index < len(_BASE_ATTRS), "unexpected NumPy type code"
        return getattr(types, _BASE_ATTRS[index])


def is_copy_index_type(typ, types):
    index = NumPyType.get(typ, types)
    if index.is_array():
        return index.ndim > 0 and index.dtype in (TypeCode.ARR_BOOL, TypeCode.ARR_I64)
    if not typ.name.startswith("Tuple["):
        return False
    have_array = False
    for generic in typ.get_generics():
        if not generic.is_type():
            return False
        element = generic.get_type_value()
        if element.is_type(types.i64):
            continue
        if not is_array_type(element) or not is_copy_index_type(element, types):
            return False
        have_array = True
    return have_array


def is_owned_index(value, types):
    call_ = cast(CallInstr, value)
    fn = get_func(call_.callee) if call_ else None
    if not fn or not is_array_type(fn.parent_type) or fn.unmangled_name != "__getitem__":
        return False
    if len(call_.args) != 2 or not NumPyType.get(value.type, types).is_array():
        return False
    source = NumPyType.get(call_.front().type, types)
    return source.is_array() and source.ndim > 0 and is_copy_index_type(call_.back().type, types)


def has_lifetime_tag(value, tag):
    attrs = value.get_attribute(KeyValueAttribute)
    return attrs is not None and attrs.has(tag)


def set_lifetime_tag(value, tag):
    attrs = value.get_attribute(KeyValueAttribute)
    if attrs is None:
        attrs = KeyValueAttribute()
        value.set_attribute(attrs)
    attrs.attributes[tag] = ""
