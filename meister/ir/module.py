# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import Dict, List, dataclass
from .base import Node, cast
from .const import BoolConst, BytesConst, FloatConst, IntConst, StringConst
from .func import BodiedFunc, InternalFunc, Intrinsic
from .type import (
    BFloat16Type,
    BoolType,
    Float16Type,
    Float32Type,
    Float128Type,
    FloatType,
    FuncType,
    GeneratorType,
    Generic,
    IntType,
    OptionalType,
    PointerType,
    PrimitiveType,
    RecordType,
    RefType,
    Type,
    UnionType,
    VectorType,
)
from .value import Value
from .var import Var


# C++: codon/cir/module.cpp:16
def translate_generics(cache, generics):
    """C++: codon/cir/module.cpp:17. Translate IR generics back to AST types."""
    from ..ast import types

    result = []
    for generic in generics:
        generic = generic if isinstance(generic, Generic) else Generic(generic)
        assert generic.is_static() or generic.value is not None, "generic must be static or a type"
        if generic.is_static_str():
            typ = types.StrLiteral(generic.value, cache=cache)
        elif generic.is_static():
            typ = types.IntLiteral(generic.value, cache=cache)
        else:
            typ = generic.value.get_actual().ast_type
        result.append(types.Link(type=typ, cache=cache))
    return result


# C++: codon/cir/module.cpp:35
def generate_dummy_names(types):
    """C++: module.cpp:35. AST argument types for overload lookup."""
    result = []
    for typ in types:
        assert typ.ast_type is not None, "must have an ast type"
        result.append(typ.ast_type)
    return result


# C++: codon/cir/module.cpp:44
def translate_args(cache, types):
    """C++: module.cpp:45. Include the unbound return type as argument zero."""
    from ..ast.types import Link

    result = [Link(cache=cache, id=0, level=0)]
    for typ in types:
        assert typ.ast_type is not None, "must have an ast type"
        # C++'s function-specific branch currently appends the same AST type.
        result.append(typ.ast_type)
    return result


@dataclass(init=False, eq=False)
class Module(Node):
    """C++: codon/cir/module.h:31. CIR object representing a program."""

    BOOL_NAME = "bool"
    FLOAT_NAME = "float"
    FLOAT32_NAME = "float32"
    FLOAT16_NAME = "float16"
    BFLOAT16_NAME = "bfloat16"
    FLOAT128_NAME = "float128"
    STRING_NAME = "str"
    BYTES_NAME = "bytes"
    EQ_MAGIC_NAME = "__eq__"
    NE_MAGIC_NAME = "__ne__"
    LT_MAGIC_NAME = "__lt__"
    GT_MAGIC_NAME = "__gt__"
    LE_MAGIC_NAME = "__le__"
    GE_MAGIC_NAME = "__ge__"
    POS_MAGIC_NAME = "__pos__"
    NEG_MAGIC_NAME = "__neg__"
    INVERT_MAGIC_NAME = "__invert__"
    ABS_MAGIC_NAME = "__abs__"
    ADD_MAGIC_NAME = "__add__"
    SUB_MAGIC_NAME = "__sub__"
    MUL_MAGIC_NAME = "__mul__"
    MATMUL_MAGIC_NAME = "__matmul__"
    TRUE_DIV_MAGIC_NAME = "__truediv__"
    FLOOR_DIV_MAGIC_NAME = "__floordiv__"
    MOD_MAGIC_NAME = "__mod__"
    POW_MAGIC_NAME = "__pow__"
    LSHIFT_MAGIC_NAME = "__lshift__"
    RSHIFT_MAGIC_NAME = "__rshift__"
    AND_MAGIC_NAME = "__and__"
    OR_MAGIC_NAME = "__or__"
    XOR_MAGIC_NAME = "__xor__"
    IADD_MAGIC_NAME = "__iadd__"
    ISUB_MAGIC_NAME = "__isub__"
    IMUL_MAGIC_NAME = "__imul__"
    IMATMUL_MAGIC_NAME = "__imatmul__"
    ITRUE_DIV_MAGIC_NAME = "__itruediv__"
    IFLOOR_DIV_MAGIC_NAME = "__ifloordiv__"
    IMOD_MAGIC_NAME = "__imod__"
    IPOW_MAGIC_NAME = "__ipow__"
    ILSHIFT_MAGIC_NAME = "__ilshift__"
    IRSHIFT_MAGIC_NAME = "__irshift__"
    IAND_MAGIC_NAME = "__iand__"
    IOR_MAGIC_NAME = "__ior__"
    IXOR_MAGIC_NAME = "__ixor__"
    RADD_MAGIC_NAME = "__radd__"
    RSUB_MAGIC_NAME = "__rsub__"
    RMUL_MAGIC_NAME = "__rmul__"
    RMATMUL_MAGIC_NAME = "__rmatmul__"
    RTRUE_DIV_MAGIC_NAME = "__rtruediv__"
    RFLOOR_DIV_MAGIC_NAME = "__rfloordiv__"
    RMOD_MAGIC_NAME = "__rmod__"
    RPOW_MAGIC_NAME = "__rpow__"
    RLSHIFT_MAGIC_NAME = "__rlshift__"
    RRSHIFT_MAGIC_NAME = "__rrshift__"
    RAND_MAGIC_NAME = "__rand__"
    ROR_MAGIC_NAME = "__ror__"
    RXOR_MAGIC_NAME = "__rxor__"
    INT_MAGIC_NAME = "__int__"
    FLOAT_MAGIC_NAME = "__float__"
    BOOL_MAGIC_NAME = "__bool__"
    STR_MAGIC_NAME = "__str__"
    REPR_MAGIC_NAME = "__repr__"
    CALL_MAGIC_NAME = "__call__"
    GETITEM_MAGIC_NAME = "__getitem__"
    SETITEM_MAGIC_NAME = "__setitem__"
    ITER_MAGIC_NAME = "__iter__"
    LEN_MAGIC_NAME = "__len__"
    NEW_MAGIC_NAME = "__new__"
    INIT_MAGIC_NAME = "__init__"

    cache: object
    main_func: BodiedFunc
    argv_var: Var
    argc_var: Var
    vars: List[Var]
    values: List[Value]
    types: List[Type]
    var_map: Dict[int, Var]
    value_map: Dict[int, Value]
    types_map: Dict[str, Type]
    arenas: List[object]
    intrinsics: Dict[object, int]

    # C++: codon/cir/module.cpp:143
    def __init__(self, cache, name=""):
        """C++: module.cpp:144. Main, argv and argc are outside the node stores."""
        super().__init__(name)
        self.cache = cache
        self.vars, self.values, self.types = [], [], []
        self.var_map, self.value_map, self.types_map = {}, {}, {}
        self.arenas, self.intrinsics = [], {}
        self.main_func = BodiedFunc("main")
        self.main_func.module, self.main_func.replaceable = self, False
        self.argv_var = Var(
            self.unsafe_get_pointer_type(self.get_string_type()), True, False, False, ".argv"
        )
        self.argv_var.module, self.argv_var.replaceable = self, False
        self.argc_var = Var(self.get_int_type(), True, False, False, ".argc")
        self.argc_var.module, self.argc_var.replaceable = self, False

    # C++: codon/cir/module.h:168
    def __iter__(self):
        """
        @return iterator to the first symbol
        """
        return iter(self.vars)

    # C++: codon/cir/module.h:186
    def get_var(self, identifier):
        """
        Gets a var by id.
        @param id the id
        @return the variable or nullptr
        """
        return self.var_map.get(identifier)

    # C++: codon/cir/module.h:224
    def get_value(self, identifier):
        """
        Gets a value by id.
        @param id the id
        @return the value or nullptr
        """
        return self.value_map.get(identifier)

    # C++: codon/cir/module.h:257
    def get_type(self, name):
        """
        @param name the type's name
        @return the type with the given name
        """
        return self.types_map.get(name)

    # C++: codon/cir/module.h:199
    def remove(self, node):
        """
        Remove a registered variable, value or type.

        Removes a given var.
        @param v the var
        """
        if isinstance(node, Type):
            self.types.remove(self.types_map.pop(node.name))
        elif isinstance(node, Value):
            self.values.remove(self.value_map.pop(node.id))
        else:
            self.vars.remove(self.var_map.pop(node.id))

    # C++ API: codon/cir/module.h:31
    def n(self, cls, source, *args, **kwargs):
        """C++: module.h:279. Construct/register a node with source information."""
        node = cls(*args, **kwargs)
        node.module = self
        node.info = source.info if hasattr(source, "info") else source
        self.store(node)
        return node

    # C++ API: codon/cir/module.h:31
    def nr(self, cls, *args, **kwargs):
        """Construct/register a node with no source information."""
        from ..ast.nodes import Node as ASTNode

        return self.n(cls, ASTNode.SrcInfo(), *args, **kwargs)

    # C++: codon/cir/module.h:285
    def store(self, node):
        if isinstance(node, Type):
            self.types.append(node)
            self.types_map[node.name] = node
            if self.arenas:
                self.arenas[-1][2].append(node.name)
        elif isinstance(node, Value):
            self.values.append(node)
            self.value_map[node.id] = node
            if self.arenas:
                self.arenas[-1][0].append(node.id)
        else:
            self.vars.append(node)
            self.var_map[node.id] = node
            if self.arenas:
                self.arenas[-1][1].append(node.id)

    # C++: codon/cir/module.cpp:161
    def parse_code(self, code):
        """Late parsed code runs after the runtime preamble, before user code.
        Call before optimization removes/restructures the user entry statement.
        """
        return self.cache.parse_code(code)

    # C++: codon/cir/module.cpp:163
    def get_or_realize_method(self, parent, name, args, generics=()):
        from ..error import TypecheckError

        cls = parent.ast_type.require_cls
        method = self.cache.find_method(cls, name, generate_dummy_names(args))
        if not method:
            return None
        try:
            return self.cache.realize_function(
                method,
                translate_args(self.cache, args),
                translate_generics(self.cache, generics),
                cls,
            )
        except TypecheckError:
            return None

    # C++: codon/cir/module.cpp:183
    def get_or_realize_func(self, name, args, generics=(), module=""):
        from ..error import TypecheckError

        name = f"{module}.{name}" if module else name
        fn = self.cache.find_function(name) or self.cache.find_function(name + ".0")
        if not fn:
            return None
        args, generics = translate_args(self.cache, args), translate_generics(self.cache, generics)
        try:
            return self.cache.realize_function(fn, args, generics)
        except TypecheckError:
            return None

    # C++: codon/cir/module.cpp:206
    def get_or_realize_type(self, name, generics=()):
        from ..error import TypecheckError

        cls = self.cache.find_class(name)
        if not cls:
            return None
        try:
            return self.cache.realize_type(cls, translate_generics(self.cache, generics))
        except TypecheckError:
            return None

    # C++: codon/cir/module.cpp:222
    def get_bool_type(self):
        return self.get_type(self.BOOL_NAME) or self.nr(BoolType)

    # C++: codon/cir/module.cpp:228
    def get_float_type(self):
        return self.get_type(self.FLOAT_NAME) or self.nr(FloatType)

    # C++: codon/cir/module.cpp:234
    def get_float32_type(self):
        return self.get_type(self.FLOAT32_NAME) or self.nr(Float32Type)

    # C++: codon/cir/module.cpp:240
    def get_float16_type(self):
        return self.get_type(self.FLOAT16_NAME) or self.nr(Float16Type)

    # C++ API: codon/cir/module.h:31
    def get_bfloat16_type(self):
        return self.get_type(self.BFLOAT16_NAME) or self.nr(BFloat16Type)

    # C++: codon/cir/module.cpp:252
    def get_float128_type(self):
        return self.get_type(self.FLOAT128_NAME) or self.nr(Float128Type)

    # C++: codon/cir/module.cpp:258
    def get_string_type(self):
        return self.get_type(self.STRING_NAME) or self.nr(
            RecordType,
            self.STRING_NAME,
            [self.unsafe_get_pointer_type(), self.unsafe_get_int_type()],
            ["_ptr", "_meta"],
        )

    # C++: codon/cir/module.cpp:266
    def get_bytes_type(self):
        return self.get_type(self.BYTES_NAME) or self.nr(
            RecordType,
            self.BYTES_NAME,
            [self.unsafe_get_pointer_type(), self.unsafe_get_int_type()],
            ["_ptr", "_len"],
        )

    # C++: codon/cir/module.cpp:274
    def get_pointer_type(self, base=None):
        return self.get_or_realize_type("Ptr", [base or self.get_int_type(8, False)])

    # C++: codon/cir/module.cpp:280
    def get_generator_type(self, base):
        return self.get_or_realize_type("Generator", [base])

    # C++: codon/cir/module.cpp:284
    def get_optional_type(self, base):
        """Canonicalize optional and NoneType layers through the typechecker."""
        return self.get_or_realize_type("Optional", [base])

    # C++: codon/cir/module.cpp:288
    def get_func_type(self, return_type, arg_types, variadic=False):
        from ..ast.types import Link

        args = translate_args(self.cache, arg_types)
        args[0] = Link(type=return_type.ast_type, cache=self.cache)
        result = self.cache.make_function(args)
        if variadic:
            # Type checker types have no concept of variadic functions: create
            # a new IR type here with the same AST type.
            fn = cast(FuncType, result)
            result = self.unsafe_get_func_type(
                fn.name + "$variadic", fn.return_type, fn.arg_types, True
            )
            result.ast_type = fn.ast_type
        return result

    # C++: codon/cir/module.cpp:304
    def get_int_type(self, length=64, sign=True):
        # Check for core 'int' type.
        return (
            self.unsafe_get_int_type(length, sign)
            if length == 64 and sign
            else self.get_or_realize_type("Int" if sign else "UInt", [length])
        )

    # C++: codon/cir/module.cpp:311
    def get_vector_type(self, count, base):
        return self.get_or_realize_type("Vec", [base, count])

    # C++: codon/cir/module.cpp:315
    def get_tuple_type(self, args):
        return self.cache.make_tuple(generate_dummy_names(args))

    # C++: codon/cir/module.cpp:324
    def get_union_type(self, types):
        return self.cache.make_union(generate_dummy_names(types))

    # C++: codon/cir/module.cpp:333
    def get_none_type(self):
        return self.get_or_realize_type("NoneType")

    # C++: codon/cir/module.cpp:335
    def get_int(self, value):
        return self.nr(IntConst, value, self.get_int_type())

    # C++: codon/cir/module.cpp:337
    def get_float(self, value):
        return self.nr(FloatConst, value, self.get_float_type())

    # C++: codon/cir/module.cpp:339
    def get_bool(self, value):
        return self.nr(BoolConst, value, self.get_bool_type())

    # C++: codon/cir/module.cpp:341
    def get_string(self, value):
        return self.nr(StringConst, value, self.get_string_type())

    # C++: codon/cir/module.cpp:345
    def get_bytes(self, value):
        return self.nr(BytesConst, value, self.get_bytes_type())

    # Unsafe factories do not generate typechecker information. Optional layers
    # are not canonicalized here. Use these only at the frontend boundary.
    # C++: codon/cir/module.cpp:349
    def unsafe_get_pointer_type(self, base=None):
        base = self.unsafe_get_int_type(8, False) if base is None else base
        return self.get_type(PointerType.get_instance_name(base)) or self.nr(PointerType, base)

    # C++: codon/cir/module.cpp:358
    def get_intrinsic(self, kind, typ, signature=None):
        """C++: module.cpp:366. Reuse a validated intrinsic declaration."""
        if signature is None:
            assert kind in (Intrinsic.SIZEOF, Intrinsic.IS_ATOMIC, Intrinsic.IS_CONTENT_ATOMIC), (
                "intrinsic requires an explicit signature"
            )
            result = self.get_int_type() if kind == Intrinsic.SIZEOF else self.get_bool_type()
            signature = self.unsafe_get_func_type(".intrinsic.query." + result.name, result, [])
        key = kind, typ.name if typ else "", signature.name
        if key in self.intrinsics:
            fn = cast(InternalFunc, self.get_var(self.intrinsics[key]))
            # A client may remove or rewrite a declaration after caching.
            if (
                fn
                and fn.intrinsic == kind
                and fn.intrinsic_type is typ
                and fn.type is signature
                and fn.has_valid_intrinsic_signature()
            ):
                return fn
        fn_type = cast(FuncType, signature)
        assert fn_type is not None, "intrinsic signature is not a function type"
        name = ".intrinsic." + kind.name.lower()
        for component in key[1:]:
            name += f".{len(component.encode())}:{component}"
        fn = self.nr(InternalFunc, name)
        fn.set_intrinsic(kind, typ)
        fn.global_ = True
        fn.realize(signature, [f"arg{idx}" for idx in range(len(fn_type.arg_types))])
        assert fn.has_valid_intrinsic_signature(), (
            f"invalid intrinsic signature: {fn.get_intrinsic_name()}"
        )
        self.intrinsics[key] = fn.id
        return fn

    # C++: codon/cir/module.cpp:396
    def unsafe_get_generator_type(self, base):
        return self.get_type(GeneratorType.get_instance_name(base)) or self.nr(GeneratorType, base)

    # C++: codon/cir/module.cpp:403
    def unsafe_get_optional_type(self, base):
        return self.get_type(OptionalType.get_instance_name(base)) or self.nr(OptionalType, base)

    # C++: codon/cir/module.cpp:410
    def unsafe_get_func_type(self, name, return_type, args, variadic=False):
        return self.get_type(name) or self.nr(FuncType, name, return_type, args, variadic)

    # C++: codon/cir/module.cpp:417
    def unsafe_get_membered_type(self, name, ref=False):
        result = self.get_type(name)
        if result is None:
            if ref:
                contents = self.get_type(name + ".contents") or self.nr(
                    RecordType, name + ".contents"
                )
                result = self.nr(RefType, name, cast(RecordType, contents))
            else:
                result = self.nr(RecordType, name)
        return result

    # C++: codon/cir/module.cpp:436
    def unsafe_get_int_type(self, length=64, sign=True):
        return self.get_type(IntType.get_instance_name(length, sign)) or self.nr(
            IntType, length, sign
        )

    # C++: codon/cir/module.cpp:443
    def unsafe_get_vector_type(self, count, base):
        primitive = cast(PrimitiveType, base)
        assert primitive is not None, "base type must be a primitive type"
        assert count > 0, "vector width must be positive"
        return self.get_type(VectorType.get_instance_name(count, primitive)) or self.nr(
            VectorType, count, primitive
        )

    # C++: codon/cir/module.cpp:453
    def unsafe_get_union_type(self, types):
        return self.get_type(UnionType.get_instance_name(types)) or self.nr(UnionType, types)

    # C++: codon/cir/module.cpp:460
    def push_arena(self):
        """Store all subsequently created nodes in a new arena."""
        self.arenas.append(([], [], []))

    # C++: codon/cir/module.cpp:462
    def pop_arena(self):
        """Deallocate nodes registered in the top arena."""
        values, variables, types = self.arenas[-1]
        for identifier in values:
            if node := self.get_value(identifier):
                self.remove(node)
        for identifier in variables:
            if node := self.get_var(identifier):
                self.remove(node)
        for name in types:
            if node := self.get_type(name):
                self.remove(node)
        self.arenas.pop()
        self.intrinsics = {
            key: identifier
            for key, identifier in self.intrinsics.items()
            if self.get_var(identifier) is not None
        }
