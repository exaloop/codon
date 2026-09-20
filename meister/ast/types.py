# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy
from typing import TYPE_CHECKING

from ..bridge import Callable, Dict, Enum, List, abstractmethod, cast, dataclass
from . import nodes as ast

if TYPE_CHECKING:
    from ..cache import Cache


def mangle(
    module: str = "",
    cls: str = "",
    func: str = "",
    var: str = "",
    overload: int = 0,
    identifier: int = 0,
    no_core: bool = False,
):
    if module == "std.internal.core":
        module = ""

    if cls and func:
        assert not var
        method = func
        if not module:
            return f"{cls}.{method}:{overload}"
        number = f".{identifier}" if "." not in cls else ""
        return ("" if not module else f"{module}.") + cls + number + f".{method}:{overload}"
    elif func:
        assert not var
        if not (no_core or module):
            return f"{func}:{overload}"
        number = f".{identifier}" if "." not in func else ""
        return ("" if not module else f"{module}.") + func + number + f":{overload}"
    elif cls:
        assert not var
        if not module:
            return cls
        number = f".{identifier}" if "." not in cls else ""
        return ("" if not module else f"{module}.") + cls + number
    else:
        assert var
        number = f".{identifier}" if "." not in var else ""
        return ("" if not module else f"{module}.") + var + number


class Stdlib:
    Any = mangle(cls="Any")
    Array = mangle(cls="Array")
    BaseException = mangle(cls="BaseException", module="std.internal.types.error")
    Bool = mangle(cls="bool")
    Callable = mangle(cls="Callable")
    Capsule = mangle(cls="Capsule")
    CObj = mangle(cls="cobj")
    Complex = mangle(cls="complex", module="std.internal.types.complex")
    Complex64 = mangle(cls="complex64", module="std.internal.types.complex")
    Coroutine = mangle(cls="Coroutine")
    Dict = mangle(cls="Dict", module="std.internal.types.array")
    Float = mangle(cls="float")
    Float16 = mangle(cls="float16")
    Function = mangle(cls="Function")
    Generator = mangle(cls="Generator")
    Int = mangle(cls="Int")
    List = mangle(cls="List", module="std.internal.types.array")
    NamedTuple = mangle(cls="NamedTuple")
    NDArray = mangle(cls="ndarray", module="std.numpy.ndarray")
    NoneType = mangle(cls="NoneType")
    Object = mangle(cls="object")
    Optional = mangle(cls="Optional")
    Ptr = mangle(cls="Ptr")
    PyError = mangle(cls="PyError", module="std.internal.python")
    Range = mangle(cls="range", module="std.internal.types.range")
    Set = mangle(cls="Set", module="std.internal.types.collections.set")
    Slice = mangle(cls="Slice", module="std.internal.types.slice")
    String = mangle(cls="str")
    ThreadLocal = mangle(cls="ThreadLocal", module="std.threading")
    Tuple = mangle(cls="Tuple")
    Type = mangle(cls="type")
    TypeWrap = mangle(cls="TypeWrap")
    UInt = mangle(cls="UInt")
    Union = mangle(cls="Union")
    UnrealizedType = mangle(cls="unrealized_type")
    Vec = mangle(cls="Vec", module="std.simd")
    CallableTrait = "CallableTrait"
    TypeTrait = "TypeTrait"

    Argv = mangle(var="__argv__")
    OptionalUnwrap = mangle(func="unwrap", module="std.internal.types.optional")


@dataclass(init=False)
class Type:
    """
    An abstract type class that describes methods needed for the type inference.

    Implements Hindley-Milner's Algorithm W inference.
    Heavily "inspired" by https://github.com/tomprimozic/type-systems
    """

    class Behaviour(Enum):
        Runtime = 0
        Int = 1
        String = 2
        Bool = 3

        @staticmethod
        def literal_from_string(value: str) -> Type.Behaviour:
            if value == "int":
                return Type.Behaviour.Int
            if value == "str":
                return Type.Behaviour.String
            if value == "bool":
                return Type.Behaviour.Bool
            return Type.Behaviour.Runtime

        def __str__(self):
            if self is Type.Behaviour.Int:
                return "int"
            if self is Type.Behaviour.String:
                return "str"
            if self is Type.Behaviour.Bool:
                return "bool"
            return "runtime"

    @dataclass(init=False)
    class UnifyContext:
        """
        A structure that keeps the list of unification steps that can be undone later.
        Needed because the unify() is destructive.
        """

        # List of unbound types that have been changed.
        linked: List[Type]
        # List of unbound types whose level has been changed.
        leveled: List[tuple[Type, int]]
        # List of assigned traits.
        traits: List[Type]
        # List of unbound types whose static status has been changed.
        statics: List[Type]

        def __init__(
            self,
            linked: List[Type] | None = None,
            leveled: List[tuple[Type, int]] | None = None,
            traits: List[Type] | None = None,
            statics: List[Type] | None = None,
        ):
            self.linked = [] if linked is None else linked
            self.leveled = [] if leveled is None else leveled
            self.traits = [] if traits is None else traits
            self.statics = [] if statics is None else statics

        def undo(self):
            for value in reversed(self.linked):
                if isinstance(value, Link):
                    value.kind = Link.Kind.Unbound
                    value.type = None
            for value, old_level in reversed(self.leveled):
                if isinstance(value, Link):
                    assert value.kind is Link.Kind.Unbound, f"not unbound [{value.info}]"
                    value.level = old_level
            for value in self.traits:
                if isinstance(value, Link):
                    value.trait = None
            for value in self.statics:
                if isinstance(value, Link):
                    value._static_kind = Type.Behaviour.Runtime

    cache: Cache
    info: ast.Node.SrcInfo | None

    def __init__(self, cache: Cache, info: ast.Node.SrcInfo | None = None):
        self.cache = cache
        self.info = info

    # Unifies a given type with the current type.
    # @param typ A given type.
    # @param undo A reference to Unification structure to track the unification steps
    # and allow later undoing of the unification procedure.
    # @return Unification score: -1 for failure, anything >= 0 for success.
    # Higher score translates to a "better" unification.
    # ⚠️ Destructive operation if undo is not null!
    # (both the current and a given type are modified).
    @abstractmethod
    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        pass

    # Generalize all unbound types whose level is below the provided level.
    # This method replaces all unbound types with a generic types (e.g. ?1 -> T1).
    # Note that the generalized type keeps the unbound type's ID.
    @abstractmethod
    def generalize(self, level: int) -> Type:
        pass

    @dataclass
    class InstantiateContext:
        cache: Dict[int, Type]
        next_unbound: Callable[[], int]

        def __init__(self, cache):
            def incr():
                i = cache.unbound_count
                cache.unbound_count += 1
                return i

            self.cache = {}
            self.next_unbound = incr

    # Instantiate all generic types. Inverse of generalize(): it replaces all
    # generic types with new unbound types (e.g. T1 -> ?1234).
    # Note that the instantiated type has a distinct and unique ID.
    # @param atLevel Level of the instantiation.
    # @param unboundCount A reference of the unbound counter to ensure that no two
    # unbound types share the same ID.
    # @param cache A reference to a lookup table to ensure that all instances of a
    # generic point to the same unbound type (e.g. dict[T, list[T]] should
    # be instantiated as dict[?1, list[?1]]).
    @abstractmethod
    def instantiate(self, level: int, ctx: InstantiateContext) -> Type:
        pass

    # Get the final type (follow through all Link links).
    # For example, for (a->b->c->d) it returns d.
    def follow(self) -> Type:
        return self

    # Check if type has unbound/generic types.
    @abstractmethod
    def has_unbounds(self, include_generics: bool) -> bool:
        return False

    # Obtain the list of internal unbound types.
    @abstractmethod
    def get_unbounds(self, include_generics: bool) -> List[Link]:
        pass

    # True if a type is realizable.
    @abstractmethod
    def can_realize(self) -> bool:
        pass

    # True if a type is completely instantiated (has no unbounds or generics).
    @abstractmethod
    def is_instantiated(self) -> bool:
        pass

    def __repr__(self):
        return self.to_string(2)

    def __bool__(self):
        # Always True; do not use __len__
        return True

    def __str__(self) -> str:
        return self.to_string(0)

    # Pretty-print facility. mode is [0: pretty, 1: llvm, 2: debug]
    @abstractmethod
    def to_string(self, mode: int) -> str:
        pass

    # Print the realization string.
    # Similar to toString, but does not print the data unnecessary for realization
    # (e.g. the function return type).
    @abstractmethod
    def realized_name(self) -> str:
        pass

    @property
    def cls(self) -> Class | None:
        t = self.follow()
        if isinstance(t, Class):
            return cast(Class, t)
        return None

    @property
    def require_cls(self) -> Class:
        if c := self.cls:
            return c
        raise ValueError("expected a Class")

    @property
    def func(self) -> Function | None:
        t = self.follow()
        if isinstance(t, Function):
            return cast(Function, t)
        return None

    @property
    def require_func(self) -> Function:
        if c := self.func:
            return c
        raise ValueError("expected a Function")

    @property
    def union(self) -> Union | None:
        t = self.follow()
        if isinstance(t, Union):
            return cast(Union, t)
        return None

    @property
    def require_union(self) -> Union:
        if c := self.union:
            return c
        raise ValueError("expected a Union")

    @property
    def literal(self) -> Literal | None:
        t = self.follow()
        if isinstance(t, Literal):
            return cast(Literal, t)
        return None

    @property
    def link(self) -> Link | None:
        # TODO: why not follow here?
        if isinstance(self, Link):
            return self
        return None

    @property
    def require_link(self) -> Link:
        if c := self.link:
            return c
        raise ValueError("expected a Link")

    @property
    def unbound(self) -> Link | None:
        t = self.follow()
        if isinstance(t, Link) and t.kind is Link.Kind.Unbound:
            return t
        return None

    @property
    def int(self) -> int | None:
        t = self.follow()
        if isinstance(t, IntLiteral):
            return t.value
        return None

    @property
    def str(self) -> str | None:
        t = self.follow()
        if isinstance(t, StrLiteral):
            return t.value
        return None

    @property
    def bool(self) -> bool | None:
        t = self.follow()
        if isinstance(t, BoolLiteral):
            return t.value
        return None

    @property
    def require_int(self) -> int:
        if (c := self.int) is not None:
            return c
        raise ValueError("expected an IntLiteral")

    @property
    def require_str(self) -> str:
        if (c := self.str) is not None:
            return c
        raise ValueError("expected an IntLiteral")

    @property
    def require_bool(self) -> bool:
        if (c := self.bool) is not None:
            return c
        raise ValueError("expected an IntLiteral")

    @property
    def partial(self) -> Class | None:
        return None

    def __eq__(self, value):
        if isinstance(value, str):
            if cls := self.follow().cls:
                return cls.name == value
            return False
        else:
            return super().__eq__(value)

    @property
    def static_kind(self) -> Type.Behaviour:
        if isinstance(self, Literal):
            return self.static_kind
        if isinstance((link := self.follow()), Link):
            return link.static_kind
        return Type.Behaviour.Runtime

    def __or__(self, other: Type):
        return self.unify(other, None) >= 0

    def __ior__(self, other: Type | None):
        if other:
            undo = Type.UnifyContext()
            if self.unify(other, undo) < 0:
                undo.undo()
                raise TypeError("cannot unify")
        return self

    @property
    def is_runtime(self):
        return self.static_kind is Type.Behaviour.Runtime

    def __hash__(self):
        return super().__hash__()


@dataclass(init=False, eq=False)
class Link(Type):
    class Kind(Enum):
        Unbound = 0
        Generic = 1
        Link = 2

    # Enumeration describing the current state.
    kind: Link.Kind = Kind.Unbound
    # The unique identifier of an unbound or generic type.
    id: int = 0
    # The type-checking level of an unbound type.
    level: int = 0
    # The type to which Link points to.
    # nullptr if unknown (unbound or generic).
    type: Type | None = None
    _static_kind: Type.Behaviour = Type.Behaviour.Runtime
    # Optional trait that unbound type requires prior to unification.
    trait: Type | None = None
    # The generic name of a generic type, if applicable.
    # Used for pretty-printing.
    generic_name: str = ""
    # Type that will be used if an unbound is not resolved.
    default_type: Type | None = None
    # Set if this type can be used unrealized as function argument
    # during function realization.
    pass_through: bool = False

    # Convenience constructor for linked types.
    def __init__(
        self,
        kind: Link.Kind = Kind.Unbound,
        id: int = 0,
        level: int = 0,
        type: Type | None = None,
        static_kind: Type.Behaviour = Type.Behaviour.Runtime,
        trait: Type | None = None,
        generic_name: str = "",
        default_type: Type | None = None,
        pass_through: bool = False,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.kind = kind
        self.id = id
        self.level = level
        self.type = type
        self._static_kind = static_kind
        self.trait = trait
        self.generic_name = generic_name
        self.default_type = default_type
        self.pass_through = pass_through

        if self.type is not None and self.kind is Link.Kind.Unbound:
            self.kind = Link.Kind.Link
        assert (self.type is not None) == (self.kind is Link.Kind.Link), "inconsistent link state"

    # Checks if a current (unbound) type occurs within a given type.
    # Needed to prevent a recursive unification (e.g. ?1 with list[?1]).
    def occurs(self, what: Type, undo: Type.UnifyContext | None):
        match what:
            case Link(kind=Link.Kind.Unbound, id=self.id):
                return True
            case Link(kind=Link.Kind.Unbound, trait=trait) if trait and self.occurs(trait, undo):
                return True
            case Link(kind=Link.Kind.Unbound, level=level):
                if level > self.level:
                    if undo:
                        undo.leveled.append((what, what.level))
                    what.level = self.level
                return False
            case Link(kind=Link.Kind.Link):
                assert what.type, "type is None"
                return self.occurs(what.type, undo)
            case Literal():
                return False
            case Class(generics=generics):
                return any(g.type and self.occurs(g.type, undo) for g in generics)
            case _:
                return False

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if self.kind is Link.Kind.Link and self.type:
            # Case: Just follow the link
            return self.type.unify(what, undo)
        # Case: Unbound unification
        if self._static_kind is not what.static_kind:
            if self._static_kind is Type.Behaviour.Runtime:
                # other one is; move this to non-static equivalent
                if undo is not None:
                    undo.statics.append(self)
                    self._static_kind = what.static_kind
            else:
                return -1
        if isinstance(what, Link):
            if what.kind is Link.Kind.Link:
                assert what.type, "link is null"
                return what.type.unify(self, undo)
            if self.kind is not what.kind:
                # Identical unbound types get a score of 1
                return -1
            if self.id == what.id:
                # Generics must have matching IDs unless we are doing non-destructive unification
                return 1
            if self.kind is Link.Kind.Generic:
                return -1 if undo else 1
            if self.id < what.id:
                # Always merge a newer type into the older type
                # (e.g. keep the types with lower id around).
                return what.unify(self, undo)
        elif self.kind is Link.Kind.Generic:
            return -1

        # Generics must be handled by now; only unbounds can be unified!
        assert self.kind is Link.Kind.Unbound, "not an unbound"
        # Ensure that we do not have recursive unification! (e.g. unify ?1 with list[?1])
        if self.occurs(what, undo):
            return -1
        # Handle traits
        if self.trait and self.trait.unify(what, undo) == -1:
            return -1
        if undo:
            undo.linked.append(self)
            self.kind = Link.Kind.Link
            ## WARNING: destructive part!
            what = what.follow()
            assert (
                not isinstance(what, Link)
                or what.kind is not Link.Kind.Unbound
                or what.id <= self.id
            ), "type unification is not consistent"
            self.type = what
            # Link current type to what and ensure that this modification is recorded in undo.
            if (
                isinstance(self.type, Link)
                and self.trait
                and self.type.kind is Link.Kind.Unbound
                and not self.type.trait
            ):
                undo.traits.append(self.type)
                self.type.trait = self.trait
        return 0

    def generalize(self, level: int):
        if self.kind is Link.Kind.Generic:
            return self
        if self.kind is Link.Kind.Unbound:
            if self.level >= level:
                return Link(
                    kind=Link.Kind.Generic,
                    id=self.id,
                    static_kind=self._static_kind,
                    trait=None if self.trait is None else self.trait.generalize(level),
                    generic_name=self.generic_name,
                    default_type=None
                    if self.default_type is None
                    else self.default_type.generalize(level),
                    pass_through=self.pass_through,
                    cache=self.cache,
                    info=self.info,
                )
            return self
        if self.kind is Link.Kind.Link and self.type:
            return self.type.generalize(level)
        assert False, "link is null"

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        if self.kind is Link.Kind.Link and self.type:
            return self.type.instantiate(level, ctx)
        if self.kind is not Link.Kind.Generic:
            return self
        if self.id not in ctx.cache:
            link = copy.copy(self)
            link.kind = Link.Kind.Unbound
            link.id = ctx.next_unbound()
            link.level = level
            link.trait = None if self.trait is None else self.trait.instantiate(level, ctx)
            ctx.cache[self.id] = link
        return ctx.cache[self.id]

    def follow(self) -> Type:
        if self.kind is not Link.Kind.Link or self.type is None:
            return self
        return self.type.follow()

    def has_unbounds(self, include_generics: bool):
        if self.kind is Link.Kind.Link and self.type:
            return self.type.has_unbounds(include_generics)
        return self.kind is Link.Kind.Unbound or (
            include_generics and self.kind is Link.Kind.Generic
        )

    def get_unbounds(self, include_generics: bool) -> List[Link]:
        if self.kind is Link.Kind.Link and self.type:
            return self.type.get_unbounds(include_generics)
        return [self] if self.has_unbounds(include_generics) else []

    def can_realize(self):
        return self.kind is Link.Kind.Link and self.type is not None and self.type.can_realize()

    def is_instantiated(self):
        return self.kind is Link.Kind.Link and self.type is not None and self.type.is_instantiated()

    def to_string(self, mode: int) -> str:
        if self.kind is Link.Kind.Link and self.type:
            return self.type.to_string(mode)
        if mode == 2:
            generic = "" if not self.generic_name else f"{self.generic_name}:"
            prefix = "?" if self.kind is Link.Kind.Unbound else "#"
            trait = "" if self.trait is None else f":{self.trait.to_string(mode)}"
            static = (
                ""
                if self._static_kind is Type.Behaviour.Runtime
                else f":S{list(Type.Behaviour).index(self._static_kind)}"
            )
            return f"{generic}{prefix}{self.id}{trait}{static}"
        if self.trait:
            return self.trait.to_string(mode)
        if self.generic_name:
            return self.generic_name
        return "?" if mode else "<unknown type>"

    def realized_name(self) -> str:
        if self.kind in {Link.Kind.Unbound, Link.Kind.Generic}:
            return f"#{self.generic_name}"
        assert self.type, "unexpected generic link"
        return self.type.realized_name()

    @property
    def static_kind(self) -> Type.Behaviour:
        link = self.follow()
        if isinstance(link, Link):
            return link._static_kind
        else:
            return link.static_kind


@dataclass
class Generic:
    name: str
    type: Type
    id: int = 0
    static_kind: Type.Behaviour = Type.Behaviour.Runtime

    def generalize(self, level: int):
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            value = self.type.runtime_type.generalize(level)
        else:
            value = self.type.generalize(level)
        return Generic(self.name, value, self.id, self.static_kind)

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        value: Type | None = None
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            value = self.type.runtime_type.instantiate(level, ctx)
        else:
            value = self.type.instantiate(level, ctx)
        return Generic(self.name, value, self.id, self.static_kind)

    def to_string(self, mode: int) -> str:
        assert self.type, "generic type is null"
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            if mode != 2:
                return self.type.runtime_type.to_string(mode)
        return self.type.to_string(mode)

    def realized_name(self) -> str:
        assert self.type, "generic type is null"
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            return self.type.runtime_type.realized_name()
        return self.type.realized_name()

    def __str__(self):
        name = "" if not self.name else f"{self.name} = "
        return f"({name}{self.type})"

    @property
    def is_runtime(self):
        return self.static_kind is Type.Behaviour.Runtime


@dataclass(init=False, eq=False)
class Class(Type):
    class Flag(Enum):
        Missing = 0
        Included = 1
        Default = 2

    name: str = ""
    generics: List[Generic]
    hidden_generics: List[Generic]
    is_tuple: bool = False
    _cached_name: str = ""

    def __init__(
        self,
        name: str = "",
        generics: List[Generic] | None = None,
        hidden_generics: List[Generic] | None = None,
        is_tuple: bool = False,
        _cached_name: str = "",
        **kwargs,
    ):
        base = kwargs.pop("base", None)
        if base:
            kwargs.setdefault("cache", base.cache)
        super().__init__(**kwargs)
        if base:
            self.name = base.name
            self.generics = base.generics
            self.hidden_generics = base.hidden_generics
            self.is_tuple = base.is_tuple
            self._cached_name = base._cached_name
        else:
            self.name = name
            self.generics = [] if generics is None else generics
            self.hidden_generics = [] if hidden_generics is None else hidden_generics
            self.is_tuple = is_tuple
            self._cached_name = _cached_name

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, Class):
            if self.name == "int" and what.name == Stdlib.Int:
                return what.unify(self, undo)
            if what.name == "int" and self.name == Stdlib.Int:
                return self[0].unify(IntLiteral(value=64, cache=self.cache), undo)
            if self.name == what.name == Stdlib.UnrealizedType:
                left = self[0].instantiate(0, Type.InstantiateContext(self.cache))
                right = what[0].instantiate(0, Type.InstantiateContext(self.cache))
                return left.unify(right, undo)
            score = 3
            if self.name == what.name == "__NTuple__":
                self_n, what_n = self[0], what[0]
                self_t, what_t = self[1].require_cls, what[1].require_cls
                if (self_i := self_n.int) and (what_i := what_n.int):
                    count = self_i * len(self_t.generics)
                    if count != what_i * len(what_t.generics):
                        return -1
                    for i in range(count):
                        if (part := self_t[i].unify(what_t[i], undo)) < 0:
                            return part
                        score += part
                    return score
            elif what.name == "__NTuple__":
                return what.unify(self, undo)
            elif self.name == "__NTuple__" and what.name == Stdlib.Tuple:
                self_n, self_t = self[0].require_cls, self[1].require_cls
                if isinstance(self_n, IntLiteral):
                    count = self_n.value
                    if count * len(self_t.generics) != len(what.generics):
                        return -1
                    for idx in range(len(self_t.generics) * count):
                        if (part := self_t[idx % len(self_t.generics)].unify(what[idx], undo)) < 0:
                            return part
                        score += part
                else:
                    count = len(what.generics)
                    # If we are unifying NT[N, T] and T[X, X, ...], we assume that N is number of X
                    if (part := self_n.unify(IntLiteral(value=count, cache=self.cache), undo)) < 0:
                        return part

                    from ..passes.typecheck.utils import generate_tuple, instantiate

                    ctx = self.cache.type_ctx
                    assert ctx
                    tuple_type = generate_tuple(ctx, 1)
                    if count:
                        tup = instantiate(ctx, tuple_type, [what[0]]).require_cls
                        for generic in what.generics[1:]:
                            if (part := tup[0].unify(generic.type, undo)) < 0:
                                return part
                            score += part
                    else:
                        tup = instantiate(ctx, tuple_type).require_cls
                    if (part := self[1].unify(tup, undo)) < 0:
                        return part
                return score

            if self.name != what.name:
                return -1
            if len(self.generics) != len(what.generics):
                return -1
            for left, right in zip(self.generics, what.generics):
                if (part := left.type.unify(right.type, undo)) < 0:
                    return part
                score += part
            for left, right in zip(self.hidden_generics, what.hidden_generics):
                if (part := left.type.unify(right.type, undo)) < 0:
                    return part
                score += part
            return score
        elif isinstance(what, Link):
            return what.unify(self, undo)
        else:
            return -1

    def generalize(self, level: int):
        return Class(
            self.name,
            [g.generalize(level) for g in self.generics],
            [g.generalize(level) for g in self.hidden_generics],
            self.is_tuple,
            cache=self.cache,
            info=self.info,
        )

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        return Class(
            self.name,
            [g.instantiate(level, ctx) for g in self.generics],
            [g.instantiate(level, ctx) for g in self.hidden_generics],
            self.is_tuple,
            cache=self.cache,
            info=self.info,
        )

    def has_unbounds(self, include_generics: bool):
        if self.name == Stdlib.UnrealizedType:
            return False
        return any(
            generic.type.has_unbounds(include_generics)
            for generic in [*self.generics, *self.hidden_generics]
            if generic.type
        )

    def get_unbounds(self, include_generics: bool) -> List[Link]:
        result: List[Link] = []
        if self.name == Stdlib.UnrealizedType:
            return result
        for generic in [*self.generics, *self.hidden_generics]:
            if generic.type:
                result[0:0] = generic.type.get_unbounds(include_generics)
        return result

    def can_realize(self):
        if self.name == Stdlib.Type and not self.has_unbounds(include_generics=False):
            return True
        if self.name == Stdlib.UnrealizedType:
            return bool(isinstance(self[0], Class))
        return all(
            generic.type.can_realize()
            for generic in [*self.generics, *self.hidden_generics]
            if generic.type
        )

    def is_instantiated(self):
        if self.name == Stdlib.UnrealizedType:
            return bool(isinstance(self[0], Class))
        return all(
            generic.type.is_instantiated()
            for generic in [*self.generics, *self.hidden_generics]
            if generic.type
        )

    def to_string(self, mode: int) -> str:
        if self.name == Stdlib.NamedTuple:
            if (tid := self[0].int) is not None:
                assert 0 <= tid < len(self.cache.generated_tuple_names), f"bad id: {tid}"
                names = self.cache.generated_tuple_names[tid]
                if not names:
                    return self.name
                values = [
                    f"{field_name}={self[1].require_cls.generics[idx].to_string(mode)}"
                    for idx, field_name in enumerate(names)
                ]
                return f"{self.name}[{','.join(values)}]"
            else:
                return f"{self.name}[{self[0].to_string(mode)}]"
        elif self.name == "Partial" and self[3].cls and mode != 2:
            # Name: function[full_args](instantiated_args...)
            known = self.partial_mask
            function = self.partial_func
            positional = [generic.to_string(mode) for generic in self[1].require_cls.generics]

            values = []
            ai, gi = 0, 0
            for i in range(len(known)):
                if function.ast[i].is_value():
                    values.append(
                        (
                            positional[ai]
                            if known[i] is Class.Flag.Included
                            else ("..." + ("" if not mode else positional[ai]))
                        )
                        if ai < len(positional)
                        else "..."
                    )
                    ai += int(known[i] is Class.Flag.Included)
                else:
                    s = function.func_generics[gi].to_string(mode)
                    values.append(
                        (
                            s
                            if known[i] is Class.Flag.Included
                            else ("..." + ("" if not mode else s))
                        )
                        if ai < len(positional)
                        else "..."
                    )
                    gi += 1
            # unused *args (by default always 0 in mask)
            if positional and positional[-1] != Stdlib.Tuple:
                values.append(positional[-1])
            kwargs = self[2].to_string(mode)
            if len(kwargs) > 10:  # if **kwargs is used
                values.append(kwargs[11:-1])  # chop off NamedTuple[...]
            fn_name = function.ast.name
            if mode == 0:
                fn_name = self.cache.rev(fn_name)
            return f"{fn_name}({','.join(values)})"
        else:
            values = [g.to_string(mode) for g in self.generics if g.name]
            if mode == 2:
                values += [f"-{g.to_string(mode)}" for g in self.hidden_generics if g.name]
            name = self.name
            if mode == 0:
                name = self.cache.rev(name)
            return name if not values else f"{name}[{','.join(values)}]"

    def realized_name(self) -> str:
        if self._cached_name:
            return self._cached_name
        if self.name == "Partial":
            result = self.to_string(1)
        else:
            values = []
            if self.name == Stdlib.Union and isinstance(self[0], Class):
                values = [
                    "|".join(sorted({g.realized_name() for g in self[0].require_cls.generics}))
                ]
            else:
                values = [g.realized_name() for g in self.generics if g.name]
            result = self.name if not values else f"{self.name}[{','.join(values)}]"
        if self.can_realize():
            self._cached_name = result
        return result

    def __getitem__(self, key) -> Type:
        return self.generics[key].type

    def __iter__(self):
        for g in self.generics:
            yield g.type

    def __len__(self):
        return len(self.generics)

    @property
    def partial(self):
        if self.name == "Partial" and self[3].require_cls[0].func:
            return self
        return None

    @property
    def partial_func(self) -> Function:
        assert self.partial, "not a partial"
        return self[3].require_cls[0].require_func

    @property
    def partial_mask(self) -> List[Flag]:
        assert self.partial, "not a partial"
        return [Class.Flag(int(p)) for p in self[0].require_str]

    @property
    def is_partial_empty(self):
        assert self.partial, "not a partial"
        args, kwargs = self[1].require_cls, self[2].require_cls
        return (
            len(args) == 1
            and not args[0].require_cls.generics
            and not kwargs[1].require_cls.generics
        )


@dataclass(init=False, eq=False)
class Literal(Class):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def can_realize(self):
        return True

    def is_instantiated(self):
        return True

    def realized_name(self) -> str:
        return self.to_string(0)

    @abstractmethod
    def get_static_expr(self) -> ast.Expr:
        pass

    @property
    def static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Runtime

    @property
    def runtime_type(self) -> Class:
        cls = self.cache.find_class(self.name)
        assert cls
        return cls


@dataclass(init=False, eq=False)
class IntLiteral(Literal):
    value: int = 0

    def __init__(self, value: int = 0, **kwargs):
        super().__init__(**kwargs)
        self.name = "int"
        self.value = value

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, IntLiteral):
            return 1 if self.value == what.value else -1
        elif isinstance(what, Class):
            return super().unify(what, undo)
        elif isinstance(what, Link):
            return what.unify(self, undo)
        else:
            return -1

    def generalize(self, level: int):
        return copy.copy(self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        return copy.copy(self)

    def to_string(self, mode: int):
        return f"{self.value}" if mode < 2 else f"Literal[{self.value}]"

    def get_static_expr(self) -> ast.Expr:
        return ast.IntExpr(self.value)

    @property
    def static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Int


@dataclass(init=False, eq=False)
class StrLiteral(Literal):
    value: str = ""

    def __init__(self, value: str = "", **kwargs):
        super().__init__(**kwargs)
        self.name = "str"
        self.value = value

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, StrLiteral):
            return 1 if self.value == what.value else -1
        elif isinstance(what, Class):
            return super().unify(what, undo)
        elif isinstance(what, Link):
            return what.unify(self, undo)
        else:
            return -1

    def generalize(self, level: int):
        return copy.copy(self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        return copy.copy(self)

    def to_string(self, mode: int):
        # TODO: use value!r after cpp equality pass
        escapes = {
            7: "\\a",
            8: "\\b",
            12: "\\f",
            10: "\\n",
            13: "\\r",
            9: "\\t",
            11: "\\v",
            39: "\\'",
            92: "\\\\",
        }
        value = "".join(
            escapes.get(c, f"\\x{c:x}" if c < 32 or c >= 127 else chr(c))
            for c in self.value.encode("utf-8", errors="surrogateescape")
        )
        return f"'{value}'" if mode < 2 else f"Literal['{value}']"

    def get_static_expr(self) -> ast.Expr:
        return ast.StringExpr(value=self.value)

    @property
    def static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.String


@dataclass(init=False, eq=False)
class BoolLiteral(Literal):
    value: bool = False

    def __init__(self, value: bool = False, **kwargs):
        super().__init__(**kwargs)
        self.name = "bool"
        self.value = value

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, BoolLiteral):
            return 1 if self.value == what.value else -1
        elif isinstance(what, Class):
            return super().unify(what, undo)
        elif isinstance(what, Link):
            return what.unify(self, undo)
        else:
            return -1

    def generalize(self, level: int):
        return copy.copy(self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        return copy.copy(self)

    def to_string(self, mode: int):
        return f"{self.value}" if mode < 2 else f"Literal[{self.value}]"

    def get_static_expr(self) -> ast.Expr:
        return ast.BoolExpr(self.value)

    @property
    def static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Bool


@dataclass(init=False, eq=False)
class Function(Class):
    """
    A generic type that represents a Codon function instantiation.
    Handles Function[] class.
    """

    ast: ast.FunctionStmt
    # Function generics (e.g. T in def foo[T](...)).
    func_generics: List[Generic]
    # Enclosing class or a function.
    func_parent: Type | None = None

    def __init__(
        self,
        ast: ast.FunctionStmt,
        func_generics: List[Generic] | None = None,
        func_parent: Type | None = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.ast = ast
        self.func_generics = [] if func_generics is None else func_generics
        self.func_parent = func_parent

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if self is what:
            return 0
        score = 2
        if isinstance(what, Function):
            # Check if names and parents match.
            if self.func_name != what.func_name or (
                (self.func_parent is None) != (what.func_parent is None)
            ):
                return -1
            part = 0
            if self.func_parent and what.func_parent:
                if (part := self.func_parent.unify(what.func_parent, undo)) < 0:
                    return part
                score += part
            # Check if function generics match.
            assert len(self.func_generics) == len(what.func_generics), (
                f"generic size mismatch for {self.func_name}"
            )
            for left, right in zip(self.func_generics, what.func_generics):
                if (part := left.type.unify(right.type, undo)) < 0:
                    return part
                score += part
        part = super().unify(what, undo)
        return part if part < 0 else score + part

    def generalize(self, level: int):
        return Function(
            self.ast,
            [g.generalize(level) for g in self.func_generics],
            None if self.func_parent is None else self.func_parent.generalize(level),
            name=self.name,
            generics=[g.generalize(level) for g in self.generics],
            hidden_generics=[g.generalize(level) for g in self.hidden_generics],
            is_tuple=self.is_tuple,
            cache=self.cache,
            info=self.info,
        )

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        func_generics = []
        for g in self.func_generics:
            func_generics.append(t := g.instantiate(level, ctx))
            if ctx.cache and t and g.id in ctx.cache:
                ctx.cache[g.id] = t.type
        return Function(
            self.ast,
            func_generics,
            None if self.func_parent is None else self.func_parent.instantiate(level, ctx),
            name=self.name,
            generics=[g.instantiate(level, ctx) for g in self.generics],
            hidden_generics=[g.instantiate(level, ctx) for g in self.hidden_generics],
            is_tuple=self.is_tuple,
            cache=self.cache,
            info=self.info,
        )

    def has_unbounds(self, include_generics: bool):
        if any(g.type.has_unbounds(include_generics) for g in self.func_generics if g.type):
            return True
        if self.func_parent and self.func_parent.has_unbounds(include_generics):
            return True
        if any(g.has_unbounds(include_generics) for g in self if g):
            return True
        ret = self.ret_type
        return ret is not None and ret.has_unbounds(include_generics)

    def get_unbounds(self, include_generics: bool) -> List[Link]:
        result: List[Link] = []
        for generic in self.func_generics:
            if generic.type:
                result[0:0] = generic.type.get_unbounds(include_generics)
        if self.func_parent:
            result[0:0] = self.func_parent.get_unbounds(include_generics)
        # Important: return type unbounds are not important, so skip them.
        for generic in self:
            if generic:
                result[0:0] = generic.get_unbounds(include_generics)
        return result

    def can_realize(self):
        allow_passthrough = self.ast.has(ast.Attr.AllowPassThrough)

        # Important: return type does not have to be realized.
        for arg in self:
            if not isinstance(arg, Function) and not arg.can_realize():
                if not allow_passthrough:
                    return False
                for unbound in arg.get_unbounds(include_generics=True):
                    if unbound.kind is Link.Kind.Generic or not unbound.pass_through:
                        return False
        result = all(g.type.can_realize() for g in self.func_generics if g.type)
        if result and self.func_parent and not self.func_parent.can_realize():
            if not allow_passthrough:
                return False
            for unbound in self.func_parent.get_unbounds(include_generics=True):
                if unbound.kind is Link.Kind.Generic or not unbound.pass_through:
                    return False
        return result

    def is_instantiated(self):
        ret_type = self.ret_type
        removed = None
        if isinstance(ret_type, Function) and ret_type.func_parent is self:
            removed = ret_type.func_parent
            ret_type.func_parent = None
        result = all(g.type.is_instantiated() for g in self.func_generics if g.type)
        if self.func_parent:
            result = result and self.func_parent.is_instantiated()
        result = result and super().is_instantiated()
        if removed:
            assert isinstance(ret_type, Function)
            ret_type.func_parent = removed
        return result

    def to_string(self, mode: int) -> str:
        generic_values = []
        for generic in self.func_generics:
            if generic.name:
                assert generic.type
                if mode < 2:
                    generic_values.append(generic.type.to_string(mode))
                else:
                    generic_values.append(
                        f"{self.cache.rev(generic.name)}={generic.type.to_string(mode)}"
                    )
        values = []
        ret = self.ret_type
        # Important: return type does not have to be realized.
        if mode == 2:
            assert ret, "function return type is null"
            values.append(f"RET={ret.to_string(mode)}")
        if mode < 2 or self.ast is None:
            for arg in self:
                values.append(arg.to_string(mode))
        else:
            sig_idx = 0
            for param in self.ast:
                if param.is_generic():
                    continue
                values.append(f"{param.name}={self[sig_idx].to_string(mode)}")
                sig_idx += 1
        merged = ",".join(generic_values)
        args = ",".join(values)
        merged = args if not merged else f"{merged};{args}"
        name = self.ast.name
        if mode == 0:
            name = self.cache.rev(name)
        if mode == 2 and self.func_parent:
            merged += f";{self.func_parent.to_string(mode)}"
        return name if not merged else f"{name}[{merged}]"

    def realized_name(self):
        generic_values = ",".join(g.realized_name() for g in self.func_generics if g.name)
        arg_values = []
        for arg in self.generics[0].type.require_cls.generics:
            arg_values.append(
                arg.type.realized_name() if isinstance(arg.type, Function) else arg.realized_name()
            )
        arguments = ",".join(arg_values)
        values = arguments if not generic_values else f"{arguments},{generic_values}"
        parent = "" if self.func_parent is None else f"{self.func_parent.realized_name()}:"
        suffix = "" if not values else f"[{values}]"
        return f"{parent}{self.func_name}{suffix}"

    @property
    def arg_type(self):
        return self.generics[0].type.require_cls

    @property
    def ret_type(self) -> Type:
        return self.generics[1].type

    @property
    def func_name(self) -> str:
        assert self.ast
        return self.ast.name

    def __getitem__(self, index: int) -> Type:
        return self.arg_type[index]

    def __iter__(self):
        for g in self.arg_type.generics:
            yield g.type

    def __len__(self):
        return len(self.arg_type.generics)


@dataclass(init=False, eq=False)
class Union(Class):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.name = Stdlib.Union
        self.is_tuple = True

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, Union):
            if not self.can_realize() or not what.can_realize():
                # Do not hard-unify if we have unbounds
                return 0
            self_t = self.get_realization_types()
            what_t = what.get_realization_types()
            if len(self_t) != len(what_t):
                return -1
            score = 2
            for s, t in zip(self_t, what_t):
                if (part := s.unify(t, undo)) < 0:
                    return part
                score += part
            return score
        elif isinstance(what, Link):
            return what.unify(self, undo)
        else:
            return -1

    def to_string(self, mode: int) -> str:
        if mode == 2 or not self.generics or not self[0].cls:
            return super().to_string(mode)
        if not isinstance(self[0], Class):
            return super().to_string(mode)
        values = sorted({g.to_string(mode) for g in self[0].require_cls.generics})
        joined = "|".join(values)
        return self.name if not joined else f"{self.name}[{joined}]"

    def realized_name(self) -> str:
        assert self.can_realize(), f"cannot realize {self.to_string(2)}"
        return super().realized_name()

    def get_realization_types(self) -> List[Class]:
        assert self.can_realize(), f"cannot realize {self.to_string(2)}"
        assert self.generics and self[0].cls, "union realization tuple is null"
        realization = {}
        for generic in self[0].require_cls.generics:
            if generic.type:
                realization[generic.type.realized_name()] = generic.type
        return [realization[key] for key in sorted(realization)]


@dataclass(init=False, eq=False)
class Trait(Type):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def can_realize(self):
        return False

    def is_instantiated(self):
        return False

    def realized_name(self):
        return ""


@dataclass(init=False, eq=False)
class CallableTrait(Trait):
    args: List[Type]

    def __init__(self, args: List[Type] | None = None, **kwargs):
        super().__init__(**kwargs)
        self.args = [] if args is None else args

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        # TODO: this fn is total mess. one day merge with the CallExpr's logic...

        from ..passes.typecheck import classes, infer, utils

        ctx = self.cache.type_ctx
        assert ctx

        if what_cls := what.cls:
            if what_cls == Stdlib.TypeWrap:
                methods = utils.find_method(ctx, what_cls, "__call_no_self__")
                what_cls = utils.instantiate(ctx, methods[0])

            if what_cls == Stdlib.NoneType:
                return 1
            if what_cls.name != Stdlib.Function and not what_cls.partial:
                return -1
            if not what_cls.is_tuple:
                return -1
            if not self.args:
                return 1

            known = ""
            what_fn = what_cls
            if partial := what_cls.partial:
                what_fn = partial.partial_func.instantiate(0, Type.InstantiateContext(ctx.cache))
                known = partial.partial_mask

                known_type = partial[1].require_cls
                generic_idx, known_idx = 0, 0
                for param_idx in range(len(known)):
                    param = what_fn.ast.items[param_idx]
                    if param.is_generic:
                        generic_idx += 1
                    elif known[param_idx] is Class.Flag.Included:
                        fn_arg = what_fn[param_idx - generic_idx]
                        known_arg = known_type.generics[known_idx].type
                        if (
                            fn_arg is None
                            or known_arg is None
                            or fn_arg.unify(known_arg, undo) == -1
                        ):
                            return -1
                        known_idx += 1
            else:
                count = len(what_cls.generics[0].type.require_cls)
                known = [Class.Flag.Missing] * count

            input_args = self.args[0].require_cls
            tr_input_args = what_fn.generics[0].type.require_cls
            tr_func = what_fn.func
            tr_ast = tr_func.ast if tr_func else None
            star_idx, kwargs_idx = 0, len(tr_input_args.generics)
            total = 0
            if tr_ast:
                star_idx = tr_ast.get_star_arg()
                kwargs_idx = tr_ast.get_kwstar_arg()
                for fn_idx in range(len(tr_ast)):
                    if fn_idx < star_idx and not tr_ast[fn_idx].is_value():
                        star_idx -= 1
                    if fn_idx < kwargs_idx and not tr_ast[fn_idx].is_value():
                        kwargs_idx -= 1
                if kwargs_idx < len(tr_ast) and star_idx >= len(tr_input_args.generics):
                    star_idx -= 1
                pre_star = 0
                for fn_idx in range(len(tr_ast)):
                    if (
                        fn_idx != kwargs_idx
                        and known[fn_idx] is not Class.Flag.Included
                        and tr_ast.items[fn_idx].is_value
                        and not tr_ast[fn_idx].name.startswith("$")
                    ):
                        total += 1
                        if fn_idx < star_idx:
                            pre_star += 1
                if pre_star < total:
                    if len(input_args.generics) < pre_star:
                        return -1
                elif len(input_args.generics) != total:
                    return -1
            else:
                total = len(tr_input_args.generics)
                star_idx = total
                if len(input_args.generics) != total:
                    return -1

            input_idx, fn_idx = 0, 0
            while input_idx < len(input_args.generics) and fn_idx < star_idx:
                if (
                    known[fn_idx] is not Class.Flag.Included
                    and tr_ast is not None
                    and tr_ast[fn_idx].is_value()
                    and not tr_ast[fn_idx].name.startswith("$")
                ):
                    input_type = input_args.generics[input_idx].type
                    target_type = tr_input_args.generics[fn_idx].type
                    input_idx += 1
                    if (
                        input_type is None
                        or target_type is None
                        or input_type.unify(target_type, undo) == -1
                    ):
                        return -1
                fn_idx += 1

            if tr_func:
                # Make sure to set types of *args/**kwargs so that the function that
                # is being unified with Callable[] can be realized
                if star_idx < len(tr_input_args.generics) - int(
                    kwargs_idx < len(tr_input_args.generics)
                ):
                    star_arg_types = []
                    if partial:
                        final = partial[1].require_cls[-1].require_cls
                        for generic in final.generics:
                            star_arg_types.append(generic.type)
                    while input_idx < len(input_args.generics):
                        star_arg_types.append(input_args[input_idx])
                        input_idx += 1
                    star_param = tr_func.ast[star_idx]
                    if star_param.type:
                        transformed = ctx.cache.typecheck(star_param.type.clone(), ctx=ctx)
                        assert isinstance(transformed, ast.Expr), "bad *args annotation"
                        # if we have *args: type, use those types
                        star_type = utils.extract_type(ctx, transformed)
                        star_arg_types = [star_type for _ in star_arg_types]
                    tuple_type = utils.instantiate(
                        ctx, classes.generate_tuple(ctx, len(star_arg_types)), star_arg_types
                    )
                    target_type = tr_input_args[star_idx]
                    if target_type is None or tuple_type.unify(target_type, undo) == -1:
                        return -1
                if kwargs_idx < len(tr_input_args.generics):
                    tuple_type = classes.generate_tuple(ctx, 0)
                    tuple_id = 0
                    if partial:
                        tuple_id = partial[2].require_cls[0].require_int
                        tuple_type = partial[2].require_cls[1].require_cls
                    kw_type = utils.instantiate(
                        ctx,
                        utils.get_stdlib_type(ctx, Stdlib.NamedTuple),
                        [IntLiteral(tuple_id, cache=ctx.cache), tuple_type],
                    )
                    target_type = tr_input_args.generics[kwargs_idx].type
                    if target_type is None or kw_type.unify(target_type, undo) == -1:
                        return -1

                if undo is not None and tr_func.can_realize():
                    # Realize if possible to allow deduction of return type
                    realized = infer.realize(ctx, tr_func)
                    assert realized
                    tr_func.unify(realized, undo)
                if not tr_func.ret_type or self.args[1].unify(tr_func.ret_type, undo) == -1:
                    return -1
            return 1
        elif link := what.link:
            if link.kind is Link.Kind.Link and link.type:
                return self.unify(link.type, undo)
            if link.kind is Link.Kind.Unbound:
                if link.trait:
                    if isinstance(link.trait, CallableTrait):
                        if len(link.trait.args) != len(self.args):
                            return -1
                        for left, right in zip(self.args, link.trait.args):
                            if left.unify(right, undo) == -1:
                                return -1
                    else:
                        return -1
                return 1
        return -1

    def generalize(self, level: int):
        ret = copy.copy(self)
        ret.args = [arg.generalize(level) for arg in self.args]
        return ret

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        ret = copy.copy(self)
        ret.args = [arg.instantiate(level, ctx) for arg in self.args]
        return ret

    def to_string(self, mode: int):
        value = self.args[0].to_string(mode)
        input_value = value.removeprefix("Tuple")
        return f"CallableTrait[{input_value},{self.args[1].to_string(mode)}]"


@dataclass(init=False, eq=False)
class TypeTrait(Trait):
    type: Type

    def __init__(self, type: Type, **kwargs):
        super().__init__(**kwargs)
        self.type = type

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if self.type is None:
            return -1
        if isinstance(what, Class):
            # does not make sense otherwise and results in infinite cycles
            return what.unify(self.type, undo)
        if isinstance(what, Link) and what.kind == Link.Kind.Unbound:
            return 0
        return -1

    def generalize(self, level: int):
        ret = copy.copy(self)
        ret.type = self.type.generalize(level)
        return ret

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        ret = copy.copy(self)
        ret.type = self.type.instantiate(level, ctx)
        return ret

    def to_string(self, mode: int):
        name = self.type.name if isinstance(self.type, Class) else "-"
        return f"Trait[{name}]"
