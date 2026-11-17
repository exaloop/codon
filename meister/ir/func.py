# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import Enum, List, dataclass
from .base import cast, is_a, replace_in_list
from .type import FuncType, GeneratorType, PointerType, RecordType, RefType
from .var import Var


@dataclass(init=False, eq=False)
class Func(Var):
    """C++: codon/cir/func.h:14, func.cpp:33. Unrealized CIR function."""

    unmangled_name: str
    generator: bool
    async_: bool
    parent_type: object | None
    args: List[Var]

    # C++: codon/cir/func.h:40
    def __init__(self, name=""):
        """
        Constructs an unrealized CIR function.
        @param name the function's name
        """
        super().__init__(None, True, False, False, name)
        self.unmangled_name = ""
        self.generator = self.async_ = False
        self.parent_type = None
        self.args = []

    # C++: codon/cir/func.cpp:32
    def realize(self, typ, names):
        """Re-initialize the function with a new type and argument names."""
        signature = cast(FuncType, typ)
        assert signature is not None, "not a function type"
        self.type = signature
        self.args = [
            self.module.nr(Var, arg, False, False, False, names[idx])
            for idx, arg in enumerate(signature)
        ]

    # C++: codon/cir/func.cpp:46
    def get_arg_var(self, name):
        return next((arg for arg in self.args if arg.name == name), None)

    # C++: codon/cir/func.cpp:52
    def _get_used_variables(self):
        return list(self.args)

    # C++: codon/cir/func.cpp:57
    def _replace_used_variable(self, identifier, new):
        return replace_in_list(self.args, identifier, new)

    # C++: codon/cir/func.cpp:61
    def _get_used_types(self):
        return [*super()._get_used_types(), *([self.parent_type] if self.parent_type else [])]

    # C++: codon/cir/func.cpp:73
    def _replace_used_type(self, name, new):
        count = super()._replace_used_type(name, new)
        if self.parent_type is not None and self.parent_type.name == name:
            self.parent_type = new
            count += 1
        return count


@dataclass(init=False, eq=False)
class BodiedFunc(Func):
    """C++: codon/cir/func.h:96. Function with local symbols and an executable body."""

    symbols: List[Var]
    body: object | None
    jit: bool

    # C++ API: codon/cir/func.h:96
    def __init__(self, name=""):
        super().__init__(name)
        self.symbols = []
        self.body = None
        self.jit = False

    # C++: codon/cir/func.h:111
    def __iter__(self):
        """
        @return iterator to the first symbol
        """
        return iter(self.symbols)

    # C++ API: codon/cir/func.h:96
    def push_back(self, var):
        self.symbols.append(var)

    # C++: codon/cir/func.h:157
    def _get_used_values(self):
        return [self.body] if self.body is not None else []

    # C++: codon/cir/func.cpp:84
    def _replace_used_value(self, identifier, new):
        from .flow import Flow

        if self.body is not None and self.body.id == identifier:
            body = cast(Flow, new)
            assert body is not None, "not a flow"
            self.body = body
            return 1
        return 0

    # C++: codon/cir/func.cpp:94
    def _get_used_variables(self):
        return [*super()._get_used_variables(), *self.symbols]

    # C++: codon/cir/func.cpp:100
    def _replace_used_variable(self, identifier, new):
        return super()._replace_used_variable(identifier, new) + replace_in_list(
            self.symbols, identifier, new
        )


@dataclass(init=False, eq=False)
class ExternalFunc(Func):
    """C++: codon/cir/func.h:166. External function; its signature can be variadic."""

    # C++: codon/cir/func.h:173
    def is_variadic(self):
        """
        @return true if the function is variadic
        """
        return cast(FuncType, self.type).variadic


class Intrinsic(Enum):
    """C++: codon/cir/func.h:180. Backend-defined internal operations."""

    NONE = 0  # No intrinsic operation has been assigned.
    ALLOCATE = 1  # Allocate an instance of a reference type.
    ALLOCATION_SIZE = 2  # Allocation size of a reference type, in bytes.
    TYPEINFO = 3  # Runtime type descriptor for a type.
    TYPEINFO_LOOKUP = 4  # Runtime type descriptor by type ID.
    TYPEINFO_METHOD_LOOKUP = 5  # Method ID to function pointer in TypeInfo.
    VIRTUAL_LOOKUP = 6  # Virtual method by receiver and thunk ID.
    RECORD_NEW = 7  # Construct a record from its field values.
    REPRESENTATION_CAST = 8  # Reinterpret a value with the same LLVM type.
    GENERATOR_PROMISE = 9  # Pointer to a generator's yielded value.
    SIZEOF = 10  # Value size, not allocation size, in bytes.
    IS_ATOMIC = 11  # Whether a type is atomic.
    IS_CONTENT_ATOMIC = 12  # Whether its contents are atomic.


@dataclass(init=False, eq=False)
class InternalFunc(Func):
    """C++: codon/cir/func.h:177, func.cpp:116. Internal, LLVM-only function."""

    Intrinsic = Intrinsic
    intrinsic: Intrinsic
    intrinsic_type: object | None

    # C++ API: codon/cir/func.h:177
    def __init__(self, name=""):
        super().__init__(name)
        self.intrinsic, self.intrinsic_type = Intrinsic.NONE, None

    # C++: codon/cir/func.h:228
    def set_intrinsic(self, kind, typ=None):
        """
        Assign operation/type operand without realizing or validating its signature.

        Sets the intrinsic operation and its type operand.
        Does not realize or validate the function signature.
        @param kind the intrinsic operation
        @param type the intrinsic's type operand, or null if unused
        """
        self.intrinsic, self.intrinsic_type = kind, typ

    # C++: codon/cir/func.cpp:108
    def get_intrinsic_name(self):
        return self.intrinsic.name.lower() if isinstance(self.intrinsic, Intrinsic) else "invalid"

    # C++: codon/cir/func.cpp:140
    def is_type_query(self):
        return self.intrinsic in (
            Intrinsic.SIZEOF,
            Intrinsic.IS_ATOMIC,
            Intrinsic.IS_CONTENT_ATOMIC,
        )

    # C++: codon/cir/func.cpp:145
    def has_valid_intrinsic_signature(self):
        """C++: func.cpp:153. Validate signature and intrinsic type operand."""
        signature = cast(FuncType, self.type)
        if not signature or signature.variadic or self.generator or self.async_:
            return False
        result, args, typ, module = (
            signature.return_type,
            signature.arg_types,
            self.intrinsic_type,
            self.module,
        )
        match self.intrinsic:
            case Intrinsic.NONE:
                return False
            case Intrinsic.SIZEOF:
                return typ is not None and not args and result is module.get_int_type()
            case Intrinsic.IS_ATOMIC | Intrinsic.IS_CONTENT_ATOMIC:
                return typ is not None and not args and result is module.get_bool_type()
            case Intrinsic.ALLOCATE:
                return is_a(RefType, typ) and not args and result is typ
            case Intrinsic.ALLOCATION_SIZE:
                return is_a(RefType, typ) and not args and result is module.get_int_type()
            case Intrinsic.TYPEINFO:
                return (
                    typ is not None
                    and not args
                    and (is_a(RefType, result) or is_a(PointerType, result))
                )
            case Intrinsic.TYPEINFO_LOOKUP:
                return (
                    typ is None
                    and len(args) == 1
                    and args[0] is module.get_int_type()
                    and (is_a(RefType, result) or is_a(PointerType, result))
                )
            case Intrinsic.TYPEINFO_METHOD_LOOKUP:
                return (
                    typ is None
                    and len(args) == 2
                    and (is_a(RefType, args[0]) or is_a(PointerType, args[0]))
                    and args[1] is module.get_int_type()
                    and is_a(PointerType, result)
                )
            case Intrinsic.VIRTUAL_LOOKUP:
                return (
                    is_a(RefType, typ)
                    and len(args) == 2
                    and args[0] is typ
                    and args[1] is module.get_int_type()
                    and is_a(FuncType, result)
                )
            case Intrinsic.RECORD_NEW:
                record = cast(RecordType, typ)
                return (
                    record is not None
                    and result is record
                    and len(args) == len(record.fields)
                    and all(arg is field.type for arg, field in zip(args, record.fields))
                )
            case Intrinsic.REPRESENTATION_CAST:
                return typ is not None and result is typ and len(args) == 1
            case Intrinsic.GENERATOR_PROMISE:
                generator, pointer = cast(GeneratorType, typ), cast(PointerType, result)
                return (
                    generator is not None
                    and pointer is not None
                    and pointer.base is generator.base
                    and len(args) == 1
                    and args[0] is generator
                )
        return False

    # C++: codon/cir/func.cpp:201
    def get_intrinsic_side_effect_status(self):
        from .util.side_effect import SideEffectStatus

        match self.intrinsic:
            case (
                Intrinsic.SIZEOF
                | Intrinsic.IS_ATOMIC
                | Intrinsic.IS_CONTENT_ATOMIC
                | Intrinsic.ALLOCATION_SIZE
                | Intrinsic.TYPEINFO
                | Intrinsic.RECORD_NEW
                | Intrinsic.REPRESENTATION_CAST
                | Intrinsic.GENERATOR_PROMISE
            ):
                return SideEffectStatus.PURE
            case Intrinsic.ALLOCATE:
                # An unused allocation can disappear, but two allocations are not equal.
                return SideEffectStatus.NO_SIDE_EFFECT
            case _:
                # Lookups can trap and observe registry/method-table updates in the JIT.
                return SideEffectStatus.UNKNOWN

    # C++: codon/cir/func.cpp:226
    def intrinsic_derives_from_arguments(self):
        return self.intrinsic in (
            Intrinsic.RECORD_NEW,
            Intrinsic.REPRESENTATION_CAST,
            Intrinsic.GENERATOR_PROMISE,
        )

    # C++: codon/cir/func.h:258
    def _get_used_types(self):
        return [*super()._get_used_types(), *([self.intrinsic_type] if self.intrinsic_type else [])]

    # C++: codon/cir/func.h:264
    def _replace_used_type(self, name, new):
        count = super()._replace_used_type(name, new)
        if self.intrinsic_type is not None and self.intrinsic_type.name == name:
            self.intrinsic_type = new
            count += 1
        return count


@dataclass(init=False, eq=False)
class LLVMFunc(Func):
    """C++: codon/cir/func.h:275. LLVM function defined in Codon source (not LLVM generation)."""

    llvm_literals: List[object]
    llvm_declares: str
    llvm_body: str

    # C++ API: codon/cir/func.h:275
    def __init__(self, name=""):
        super().__init__(name)
        self.llvm_literals = []
        self.llvm_declares = self.llvm_body = ""

    # C++: codon/cir/func.cpp:234
    def _get_used_types(self):
        return [
            *super()._get_used_types(),
            *(literal.value for literal in self.llvm_literals if literal.is_type()),
        ]

    # C++: codon/cir/func.cpp:247
    def _replace_used_type(self, name, new):
        from .type import Generic

        count = super()._replace_used_type(name, new)
        for idx, literal in enumerate(self.llvm_literals):
            if literal.is_type() and literal.value.name == name:
                self.llvm_literals[idx] = Generic(new)
                count += 1
        return count
