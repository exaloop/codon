# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import Dict, List, dataclass
from .base import ReplaceableNode, cast


@dataclass(init=False, eq=False)
class Generic:
    """C++: codon/cir/type.h:26. A static integer/string or an IR type."""

    STATIC = 1
    STATIC_STR = 2
    TYPE = 3
    value: object
    tag: int

    # C++: codon/cir/type.h:36
    def __init__(self, value):
        self.value = value
        self.tag = (
            self.STATIC_STR
            if isinstance(value, str)
            else self.STATIC
            if isinstance(value, int)
            else self.TYPE
        )
        # C++ intentionally leaves static-string deletion disabled.

    # C++: codon/cir/type.h:52
    def is_type(self):
        """
        @return true if the generic is a type
        """
        return self.tag == self.TYPE

    # C++: codon/cir/type.h:54
    def is_static(self):
        """
        @return true if the generic is static
        """
        return self.tag == self.STATIC

    # C++: codon/cir/type.h:56
    def is_static_str(self):
        """
        @return true if the generic is static
        """
        return self.tag == self.STATIC_STR

    # C++: codon/cir/type.h:59
    def get_static_value(self):
        """
        @return the static value
        """
        return self.value

    # C++: codon/cir/type.h:61
    def get_static_string_value(self):
        """
        @return the static string value
        """
        return self.value

    # C++: codon/cir/type.h:63
    def get_type_value(self):
        """
        @return the type value
        """
        return self.value


@dataclass(init=False, eq=False)
class RuntimeInfo:
    """C++: codon/cir/type.h:70. Runtime type descriptors and virtual method tables."""

    id: int
    raw_name: str
    nice_name: str
    base_name: str
    mro: List[Type]
    field_owners: List[Type]
    parameters: List[Type]
    methods: Dict[int, object]
    value_accessed: bool
    exception_base: Type | None
    system_exit_base: Type | None

    # C++ API: codon/cir/type.h:70
    def __init__(self):
        # Runtime ID (0 if unassigned), canonical realized name, human-readable
        # name, and canonical class name before generic realization.
        self.id = 0
        self.raw_name = self.nice_name = self.base_name = ""
        # MRO begins with this type; owners follow source-visible member order.
        self.mro, self.field_owners, self.parameters = [], [], []
        # Semantic method IDs map to virtual targets and erased value adapters.
        self.methods = {}
        # Accessing __id__ makes this type a value-callback discovery root.
        self.value_accessed = False
        # BaseException and SystemExit entries in the MRO, or null otherwise.
        self.exception_base = self.system_exit_base = None


@dataclass(init=False, eq=False)
class Type(ReplaceableNode):
    """C++: codon/cir/type.h:67. CIR types are generally immutable."""

    RuntimeInfo = RuntimeInfo
    _ast_type: object
    _runtime_info: RuntimeInfo | None

    # C++: codon/cir/type.h:105
    def __init__(self, name=""):
        super().__init__(name)
        self.ast_type = None
        self.runtime_info = None

    # C++: codon/cir/type.h:142
    @property
    def ast_type(self):
        """
        @return the ast type
        """
        return self.get_actual()._ast_type

    # C++: codon/cir/type.h:145
    @ast_type.setter
    def ast_type(self, value):
        """
        Sets the ast type. Should not generally be used.
        @param t the new type
        """
        self.get_actual()._ast_type = value

    # C++: codon/cir/type.h:148
    @property
    def runtime_info(self):
        """
        @return the type's runtime metadata, or null if none has been attached
        """
        return self.get_actual()._runtime_info

    # C++: codon/cir/type.h:154
    @runtime_info.setter
    def runtime_info(self, value):
        """
        Sets the type's runtime metadata.
        Replaces existing metadata, invalidating pointers returned by getRuntimeInfo().
        @param info the new runtime metadata
        """
        self.get_actual()._runtime_info = value

    # C++: codon/cir/type.cpp:31
    def get_used_types(self):
        """C++: type.cpp:31. Structural and runtime metadata dependencies."""
        actual = self.get_actual()
        result = list(actual._get_used_types())
        if info := actual.runtime_info:
            for typ in [
                *info.mro,
                *info.field_owners,
                *info.parameters,
                info.exception_base,
                info.system_exit_base,
            ]:
                if typ is not None and typ is not actual and typ not in result:
                    result.append(typ)
        return result

    # C++: codon/cir/type.cpp:51
    def get_used_variables(self):
        info = self.get_actual().runtime_info
        return [fn for _, fn in sorted(info.methods.items())] if info else []

    # C++ API: codon/cir/type.h:67
    def _replace_used_variable(self, identifier, new):
        from .func import Func

        replacement = cast(Func, new)
        assert new is None or replacement is not None, "runtime method target must be a function"
        count = 0
        if info := self.runtime_info:
            for key, fn in sorted(info.methods.items()):
                if fn.id != identifier:
                    continue
                count += 1
                if replacement is not None:
                    info.methods[key] = replacement
                else:
                    del info.methods[key]
        return count

    # C++ API: codon/cir/type.h:67
    def _replace_used_type(self, name, new):
        assert False, "types not replaceable"

    # C++ API: codon/cir/type.h:67
    def is_type(self, other):
        return self.name == other.name

    # C++: codon/cir/type.h:134
    def is_atomic(self):
        """
        Atomic types contain no pointers to dynamically allocated memory.

        A type is "atomic" iff it contains no pointers to dynamically
        allocated memory. Atomic types do not need to be scanned during
        garbage collection.
        @return true if the type is atomic
        """
        return self.get_actual()._is_atomic()

    # C++: codon/cir/type.h:134
    def _is_atomic(self):
        """
        A type is "atomic" iff it contains no pointers to dynamically
        allocated memory. Atomic types do not need to be scanned during
        garbage collection.
        @return true if the type is atomic
        """
        raise NotImplementedError

    # C++: codon/cir/type.h:139
    def is_content_atomic(self):
        """
        Whether contents of an allocated memory block are atomic.

        Checks if the contents (i.e. within an allocated block of memory)
        of a type are atomic. Currently only meaningful for reference types.
        @return true if the type's content is atomic
        """
        return self.get_actual()._is_content_atomic()

    # C++: codon/cir/type.h:139
    def _is_content_atomic(self):
        """
        Checks if the contents (i.e. within an allocated block of memory)
        of a type are atomic. Currently only meaningful for reference types.
        @return true if the type's content is atomic
        """
        return True

    # C++: codon/cir/type.h:159
    def get_generics(self):
        """
        @return the generics used in the type
        """
        return self.get_actual()._get_generics()

    # C++: codon/cir/type.cpp:86
    def _get_generics(self):
        if not self.ast_type:
            return []
        return self._convert_generics(self.ast_type.require_cls.generics)

    # C++ API: codon/cir/type.h:67
    def _convert_generics(self, generics):
        result = []
        for generic in generics:
            typ = generic.type
            if typ.int is not None:
                result.append(Generic(typ.require_int))
            elif typ.bool is not None:
                result.append(Generic(int(typ.require_bool)))
            elif typ.str is not None:
                result.append(Generic(typ.require_str))
            elif cls := typ.cls:
                result.append(
                    Generic(self.module.cache.realize_type(cls, [g.type for g in cls.generics]))
                )
            else:
                assert False, f"IR only supports int, bool or str statics [{typ.info}]"
        return result

    # C++: codon/cir/type.h:164
    def construct(self, args):
        """
        Constructs an instance of the type given the supplied args.
        @param args the arguments
        @return the new value
        """
        return self.get_actual()._construct(list(args))

    # C++: codon/cir/type.h:167
    def __call__(self, *args):
        return self.construct(args)

    # C++: codon/cir/type.cpp:110
    def _construct(self, args):
        from .instr import CallInstr
        from .var import VarValue

        fn = self.module.get_or_realize_method(
            self, self.module.NEW_MAGIC_NAME, [arg.type for arg in args]
        )
        return self.module.nr(CallInstr, self.module.nr(VarValue, fn), args) if fn else None


@dataclass(init=False, eq=False)
class PrimitiveType(Type):
    """C++: codon/cir/type.h:184. Base for primitive atomic types."""

    # C++: codon/cir/type.h:191
    def _is_atomic(self):
        return True


@dataclass(init=False, eq=False)
class IntType(PrimitiveType):
    """C++: codon/cir/type.h:195. Type of a variably sized integer."""

    MAX_LEN = 2048
    len: int
    sign: bool

    # C++: codon/cir/type.h:210
    def __init__(self, length, sign):
        """
        Constructs a variably sized integer type.
        @param len the length of the integer
        @param sign true if signed, false otherwise
        """
        # C++ accepts unsigned int, including during speculative realizations.
        length &= 0xFFFFFFFF
        super().__init__(self.get_instance_name(length, sign))
        self.len, self.sign = length, sign

    # C++: codon/cir/type.h:219
    def opposite_sign_name(self):
        """
        @return the name of the opposite signed corresponding type
        """
        return self.get_instance_name(self.len, not self.sign)

    # C++: codon/cir/type.cpp:127
    @staticmethod
    def get_instance_name(length, sign):
        return f"{'' if sign else 'U'}Int{length & 0xFFFFFFFF}"


@dataclass(init=False, eq=False)
class FloatType(PrimitiveType):
    """C++: codon/cir/type.h:225. Float type (64-bit double)."""

    # C++: codon/cir/type.h:230
    def __init__(self):
        """
        Constructs a float type.
        """
        super().__init__("float")


@dataclass(init=False, eq=False)
class Float32Type(PrimitiveType):
    """C++: codon/cir/type.h:234. Float32 type (32-bit float)."""

    # C++: codon/cir/type.h:239
    def __init__(self):
        """
        Constructs a float32 type.
        """
        super().__init__("float32")


@dataclass(init=False, eq=False)
class Float16Type(PrimitiveType):
    """C++: codon/cir/type.h:243. Float16 type (16-bit float)."""

    # C++: codon/cir/type.h:248
    def __init__(self):
        """
        Constructs a float16 type.
        """
        super().__init__("float16")


@dataclass(init=False, eq=False)
class BFloat16Type(PrimitiveType):
    """C++: codon/cir/type.h:252. BFloat16 type (16-bit brain float)."""

    # C++: codon/cir/type.h:257
    def __init__(self):
        """
        Constructs a bfloat16 type.
        """
        super().__init__("bfloat16")


@dataclass(init=False, eq=False)
class Float128Type(PrimitiveType):
    """C++: codon/cir/type.h:261. Float128 type (128-bit float)."""

    # C++: codon/cir/type.h:266
    def __init__(self):
        """
        Constructs a float128 type.
        """
        super().__init__("float128")


@dataclass(init=False, eq=False)
class BoolType(PrimitiveType):
    """C++: codon/cir/type.h:270. Bool (8-bit unsigned integer; either 0 or 1)."""

    # C++: codon/cir/type.h:275
    def __init__(self):
        """
        Constructs a bool type.
        """
        super().__init__("bool")


@dataclass(init=False, eq=False)
class Field:
    """C++: codon/cir/type.h:284. Field in a membered type: name and type."""

    name: str
    type: Type

    # C++: codon/cir/type.h:295
    def __init__(self, name, typ):
        """
        Constructs a field.
        @param name the field's name
        @param type the field's type
        """
        self.name, self.type = name, typ


@dataclass(init=False, eq=False)
class MemberedType(Type):
    """C++: codon/cir/type.h:279. Base for membered types."""

    Field = Field

    # C++: codon/cir/type.h:313
    def get_member_type(self, name):
        """
        Gets a field type by name.
        @param name the field's name
        @return the type if it exists
        """
        raise NotImplementedError

    # C++: codon/cir/type.h:317
    def get_member_index(self, name):
        """
        Gets the index of a field by name.
        @param name the field's name
        @return 0-based field index, or -1 if not found
        """
        raise NotImplementedError

    # C++: codon/cir/type.h:331
    def realize(self, types, names):
        """
        Changes the body of the membered type.
        @param mTypes the new body
        @param mNames the new names
        """
        raise NotImplementedError

    # C++: codon/cir/type.h:324
    def front(self):
        """
        @return a reference to the first field
        """
        return next(iter(self))

    # C++: codon/cir/type.h:326
    def back(self):
        """
        @return a reference to the last field
        """
        return list(self)[-1]


@dataclass(init=False, eq=False)
class RecordType(MemberedType):
    """C++: codon/cir/type.h:335. Membered type equivalent to C structs/C++ PODs."""

    fields: List[Field]

    # C++: codon/cir/type.cpp:147
    def __init__(self, name, types=None, names=None):
        super().__init__(name)
        types = [] if types is None else types
        # Unnamed constructor fields are "1", "2", ...
        names = [str(idx + 1) for idx in range(len(types))] if names is None else names
        self.fields = [Field(names[idx], typ) for idx, typ in enumerate(types)]

    # C++: codon/cir/type.h:359
    def __iter__(self):
        return iter(self.fields)

    # C++: codon/cir/type.cpp:169
    def get_member_type(self, name):
        return next((field.type for field in self.fields if field.name == name), None)

    # C++: codon/cir/type.cpp:175
    def get_member_index(self, name):
        return next((idx for idx, field in enumerate(self.fields) if field.name == name), -1)

    # C++: codon/cir/type.cpp:182
    def realize(self, types, names):
        self.fields = [Field(names[idx], typ) for idx, typ in enumerate(types)]

    # C++: codon/cir/type.cpp:162
    def _get_used_types(self):
        return [field.type for field in self.fields]

    # C++: codon/cir/type.h:369
    def _is_atomic(self):
        return all(field.type.is_atomic() for field in self.fields)


@dataclass(init=False, eq=False)
class RefType(MemberedType):
    """C++: codon/cir/type.h:376. Membered type passed by reference, like Python classes."""

    contents: RecordType
    polymorphic: bool

    # C++: codon/cir/type.h:390
    def __init__(self, name, contents, polymorphic=False):
        """
        Constructs a reference type.
        @param name the type's name
        @param contents the type's contents
        @param polymorphic true if type is polymorphic
        """
        super().__init__(name)
        self.contents, self.polymorphic = contents, polymorphic

    # C++: codon/cir/type.h:399
    def get_member_type(self, name):
        return self.contents.get_member_type(name)

    # C++: codon/cir/type.h:402
    def get_member_index(self, name):
        return self.contents.get_member_index(name)

    # C++: codon/cir/type.h:406
    def __iter__(self):
        return iter(self.contents)

    # C++: codon/cir/type.h:417
    def realize(self, types, names):
        self.contents.realize(types, names)

    # C++: codon/cir/type.h:422
    def _get_used_types(self):
        return [self.contents]

    # C++: codon/cir/type.h:424
    def _is_atomic(self):
        return False

    # C++: codon/cir/type.cpp:191
    def _is_content_atomic(self):
        return all(field.type.is_atomic() for field in self.contents)

    # C++: codon/cir/type.cpp:197
    def _construct(self, args):
        from .instr import CallInstr
        from .util.irtools import make_tuple
        from .var import VarValue

        args_tuple = make_tuple(args, self.module)
        fn = self.module.get_or_realize_func(
            "construct_ref", [args_tuple.type], [self], "std.internal.gc"
        )
        return self.module.nr(CallInstr, self.module.nr(VarValue, fn), [args_tuple]) if fn else None


@dataclass(init=False, eq=False)
class FuncType(Type):
    """C++: codon/cir/type.h:432. Type associated with a CIR function."""

    return_type: Type
    arg_types: List[Type]
    variadic: bool

    # C++: codon/cir/type.h:451
    def __init__(self, name, return_type, arg_types, variadic=False):
        """
        Constructs a function type.
        @param rType the function's return type
        @param argTypes the function's arg types
        """
        super().__init__(name)
        self.return_type, self.arg_types, self.variadic = return_type, list(arg_types), variadic

    # C++: codon/cir/type.h:462
    def __iter__(self):
        """
        @return iterator to the first argument
        """
        return iter(self.arg_types)

    # C++: codon/cir/type.h:466
    def front(self):
        """
        @return a reference to the first argument
        """
        return self.arg_types[0]

    # C++: codon/cir/type.h:468
    def back(self):
        """
        @return a reference to the last argument
        """
        return self.arg_types[-1]

    # C++: codon/cir/type.cpp:211
    def _get_generics(self):
        fn = self.ast_type.func if self.ast_type else None
        return self._convert_generics(fn.func_generics) if fn else []

    # C++: codon/cir/type.cpp:240
    def _get_used_types(self):
        return [*self.arg_types, self.return_type]

    # C++: codon/cir/type.h:475
    def _is_atomic(self):
        return False


@dataclass(init=False, eq=False)
class DerivedType(Type):
    """C++: codon/cir/type.h:479. Base for simple derived types."""

    base: Type

    # C++: codon/cir/type.h:490
    def __init__(self, name, base):
        """
        Constructs a derived type.
        @param name the type's name
        @param base the type's base
        """
        super().__init__(name)
        self.base = base

    # C++: codon/cir/type.h:497
    def _is_atomic(self):
        return self.base.is_atomic()

    # C++: codon/cir/type.h:499
    def _get_used_types(self):
        return [self.base]


@dataclass(init=False, eq=False)
class PointerType(DerivedType):
    """C++: codon/cir/type.h:503. Type of a pointer to another CIR type."""

    # C++: codon/cir/type.h:509
    def __init__(self, base):
        """
        Constructs a pointer type.
        @param base the type's base
        """
        super().__init__(self.get_instance_name(base), base)

    # C++: codon/cir/type.cpp:250
    @staticmethod
    def get_instance_name(base):
        return f"Pointer[{base.reference_string()}]"

    # C++: codon/cir/type.h:514
    def _is_atomic(self):
        return False


@dataclass(init=False, eq=False)
class OptionalType(DerivedType):
    """C++: codon/cir/type.h:518. Type of an optional containing another CIR type."""

    # C++: codon/cir/type.h:524
    def __init__(self, base):
        """
        Constructs an optional type.
        @param base the type's base
        """
        super().__init__(self.get_instance_name(base), base)

    # C++: codon/cir/type.cpp:256
    @staticmethod
    def get_instance_name(base):
        return f"Optional[{base.reference_string()}]"


@dataclass(init=False, eq=False)
class GeneratorType(DerivedType):
    """C++: codon/cir/type.h:533. Type of a generator yielding another CIR type."""

    # C++: codon/cir/type.h:539
    def __init__(self, base):
        """
        Constructs a generator type.
        @param base the type's base
        """
        super().__init__(self.get_instance_name(base), base)

    # C++: codon/cir/type.cpp:262
    @staticmethod
    def get_instance_name(base):
        return f"Generator[{base.reference_string()}]"

    # C++: codon/cir/type.h:544
    def _is_atomic(self):
        return False


@dataclass(init=False, eq=False)
class VectorType(PrimitiveType):
    """C++: codon/cir/type.h:548. Type of a vector of primitives."""

    count: int
    base: PrimitiveType

    # C++: codon/cir/type.h:561
    def __init__(self, count, base):
        """
        Constructs a vector type.
        @param count the number of elements
        @param base the base type
        """
        super().__init__(self.get_instance_name(count, base))
        self.count, self.base = count, base

    # C++: codon/cir/type.cpp:268
    @staticmethod
    def get_instance_name(count, base):
        return f"Vector[{count}, {base.reference_string()}]"


@dataclass(init=False, eq=False)
class UnionType(Type):
    """C++: codon/cir/type.h:572. Alternative types must be sorted by the caller."""

    types: List[Type]

    # C++: codon/cir/type.h:585
    def __init__(self, types):
        """
        Constructs a UnionType.
        @param types the alternative types (must be sorted by caller)
        """
        super().__init__(self.get_instance_name(types))
        self.types = list(types)

    # C++: codon/cir/type.h:588
    def __iter__(self):
        return iter(self.types)

    # C++: codon/cir/type.h:590
    def front(self):
        return self.types[0]

    # C++: codon/cir/type.h:591
    def back(self):
        return self.types[-1]

    # C++: codon/cir/type.cpp:274
    @staticmethod
    def get_instance_name(types):
        return "Union[" + ", ".join(typ.reference_string() for typ in types) + "]"

    # C++: codon/cir/type.h:596
    def _get_used_types(self):
        return list(self.types)

    # C++: codon/cir/type.h:598
    def _is_atomic(self):
        return all(typ.is_atomic() for typ in self.types)
