# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
from __future__ import annotations

from abc import abstractmethod
from dataclasses import dataclass
from enum import Enum
from typing import Any, Callable, Dict, List, Tuple

from . import nodes as ast


def mangle(
    module: str = "",
    cls: str = "",
    func: str = "",
    var: str = "",
    overload: int = 0,
    identifier: int = 0,
    no_core: bool = False,
):
    if module == "std.internal.core": module = ""

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
    Type = mangle("type")
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
                    value.static_kind = Type.Behaviour.Runtime


    cache: object
    info: ast.Node.SrcInfo

    def __init__(
        self, cache: object = None, info: ast.Node.SrcInfo | None = None, copy: Type | None = None
    ):
        self.cache = cache or getattr(copy, "cache", None)
        self.info = info or getattr(copy, "info", ast.Node.SrcInfo())

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

        def __init__(self, ctx):
            def incr():
                i = ctx.unbound_count
                ctx.unbound_count += 1
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
    def get_unbounds(self, include_generics: bool) -> List[Type]:
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

    def is_type(self, name: str):
        cls = self.follow()
        return isinstance(cls, Class) and cls.name == name

    def get_static_kind(self) -> Type.Behaviour:
        if isinstance(self, Literal):
            return self.get_static_kind()
        if isinstance((link := self.follow()), Link):
            return link.static_kind
        return Type.Behaviour.Runtime


@dataclass(init=False)
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
    static_kind: Type.Behaviour = Type.Behaviour.Runtime
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
        self.static_kind = static_kind
        self.trait = trait
        self.generic_name = generic_name
        self.default_type = default_type
        self.pass_through = pass_through

        if self.type is not None and self.kind is Link.Kind.Unbound:
            self.kind = Link.Kind.Link
        assert (self.type is None) == (self.kind is Link.Kind.Link), "inconsistent link state"

    # Checks if a current (unbound) type occurs within a given type.
    # Needed to prevent a recursive unification (e.g. ?1 with list[?1]).
    def occurs(self, what: Type, undo: Type.UnifyContext | None):
        if isinstance(what, Link):
            if what.kind is Link.Kind.Unbound:
                if what.id == self.id:
                    return True
                if what.trait and self.occurs(what.trait, undo):
                    return True
                if undo and what.level > self.level:
                    undo.leveled.append((what, what.level))
                    what.level = self.level
                return False
            elif what.kind is Link.Kind.Link:
                assert what.type, "type is None"
                return self.occurs(what.type, undo)
            else:
                return False
        elif isinstance(what, Literal):
            return False
        elif isinstance(what, Class):
            return any(g.type and self.occurs(g.type, undo) for g in what.generics)
        else:
            return False

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if self.kind is Link.Kind.Link and self.type:
            # Case: Just follow the link
            return self.type.unify(what, undo)
        # Case: Unbound unification
        if self.get_static_kind() is not what.get_static_kind():
            if self.get_static_kind() is Type.Behaviour.Runtime:
                # other one is; move this to non-static equivalent
                if undo is not None:
                    undo.statics.append(self)
                    self.static_kind = what.get_static_kind()
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
                isinstance(what, Link) and what.kind is Link.Kind.Unbound and what.id <= self.id
            ), "type unification is not consistent"
            self.type = what
            # Link current type to what and ensure that this modification is recorded in undo.
            if (
                isinstance(self.type, Link)
                and self.trait
                and self.type.kind is Link.Kind.Unbound
                and self.type.trait
            ):
                undo.traits.append(self.type)
                self.type.trait = self.trait
        return 0

    def generalize(self, level: int) -> Type:
        if self.kind is Link.Kind.Generic:
            return self
        if self.kind is Link.Kind.Unbound:
            if self.level >= level:
                return Link(
                    kind=Link.Kind.Generic,
                    id=self.id,
                    static_kind=self.static_kind,
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

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        if self.kind is Link.Kind.Link and self.type:
            return self.type.instantiate(level, ctx)
        if self.kind is not Link.Kind.Generic:
            return self
        if self.id not in ctx.cache:
            ctx.cache[self.id] = Link(
                kind=Link.Kind.Unbound,
                id=ctx.next_unbound(),
                level=level,
                trait=None if self.trait is None else self.trait.instantiate(level, ctx),
                copy=self
            )
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

    def get_unbounds(self, include_generics: bool) -> List[Type]:
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
                if self.static_kind is Type.Behaviour.Runtime
                else f":S{list(Type.Behaviour).index(self.static_kind)}"
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


@dataclass
class Generic:
    name: str
    type: Type
    id: int = 0
    static_kind: Type.Behaviour = Type.Behaviour.Runtime

    def generalize(self, level: int):
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            value = self.type.get_non_static_type().generalize(level)
        else:
            value = self.type.generalize(level)
        return Generic(self.name, value, self.id, self.static_kind)

    def instantiate(self, level: int, ctx: Type.InstantiateContext):
        value: Type | None = None
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            value = self.type.get_non_static_type().instantiate(level, ctx)
        else:
            value = self.type.generalize(level)
        return Generic(self.name, value, self.id, self.static_kind)

    def to_string(self, mode: int) -> str:
        assert self.type, "generic type is null"
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            if mode != 2:
                return self.type.get_non_static_type().to_string(mode)
        return self.type.to_string(mode)

    def realized_name(self) -> str:
        assert self.type, "generic type is null"
        if self.static_kind is Type.Behaviour.Runtime and isinstance(self.type, Literal):
            return self.type.get_non_static_type().realized_name()
        return self.type.realized_name()

    def __str__(self):
        name = "" if not self.name else f"{self.name} = "
        return f"({name}{self.type})"


@dataclass(init=False)
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
        super().__init__(**kwargs)
        self.name = name
        self.generics = [] if generics is None else generics
        self.hidden_generics = [] if hidden_generics is None else hidden_generics
        self.is_tuple = is_tuple
        self._cached_name = _cached_name

    def __getitem__(self, key):
        return self.generics[key].type

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        if isinstance(what, Class):
            if self.name == "int" and what.name == Stdlib.Int:
                return what.unify(self, undo)
            if what.name == "int" and self.name == Stdlib.Int:
                return self[0].unify(IntLiteral(value=64, cache=self.cache), undo)
            if self.name == what.name == Stdlib.UnrealizedType:
                left = self[0].instantiate(Type.InstantiateContext(self.ctx))
                right = what[0].instantiate(Type.InstantiateContext(self.ctx))
                return left.unify(right, undo)
            score = 3
            if self.name == what.name == "__NTuple__":
                self_n, what_n = self[0], what[0]
                self_t, what_t = self[1], what[1]
                if isinstance(self_n, IntLiteral) and isinstance(what_n, IntLiteral):
                    count = self_n.value * len(self_t.generics)
                    if count != what_n.value * len(what_t.generics):
                        return -1
                    for i in range(count):
                        if (part := self_t[i].unify(what_t[i], undo)) < 0:
                            return part
                        score += part
                    return score
            elif what.name == "__NTuple__":
                return what.unify(self, undo)
            elif self.name == "__NTuple__" and what.name == Stdlib.Tuple:
                self_n, self_t = self[0], self[1]
                if isinstance(self_n, IntLiteral):
                    count = self_n.value
                    if count * len(self_t.generics) != len(what.generics):
                        return -1
                    for index in range(len(self_t.generics) * count):
                        if (
                            part := self_t[index % len(self_t.generics)].unify(what[index], undo)
                        ) < 0:
                            return part
                        score += part
                else:
                    count = len(what.generics)
                    # If we are unifying NT[N, T] and T[X, X, ...], we assume that N is number of X's
                    if (part := self_n.unify(IntLiteral(value=count, cache=self.cache), undo)) < 0:
                        return part

                    tv = TypecheckVisitor(self.cache.type_ctx)
                    if count:
                        tup = tv.instantiate_type(tv.generate_tuple(1), [what[0]])
                        for generic in what.generics[1:]:
                            if (part := tup[0].unify(generic.type, undo)) < 0:
                                return part
                            score += part
                    else:
                        tup = tv.instantiate_type(tv.generate_tuple(1))
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

    def generalize(self, level: int) -> Type:
        return Class(
            self.name,
            [g.generalize(level) for g in self.generics],
            [g.generalize(level) for g in self.hidden_generics],
            self.is_tuple,
            cache=self.cache,
            info=self.info,
        )

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
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

    def get_unbounds(self, include_generics: bool) -> List[Type]:
        result: List[Type] = []
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
            if isinstance(self[0], IntLiteral):
                tid = self[0].value
                assert 0 <= tid < len(self.cache.generated_tuple_names), f"bad id: {tid}"
                names = self.cache.generated_tuple_names[tid]
                if not names:
                    return self.name
                values = [
                    f"{field_name}={self[1].generics[index].to_string(mode)}"
                    for index, field_name in enumerate(names)
                ]
                return f"{self.name}[{','.join(values)}]"
            else:
                return f"{self.name}[{self[0].to_string(mode)}]"
        elif self.name == "Partial" and isinstance(self[3], Class):
            # Name: function[full_args](instantiated_args...)
            known = self.get_partial_mask()
            function = self.get_partial_func()
            positional = [generic.to_string(mode) for generic in self[1].generics]

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
            function_name = function.ast.name
            if mode == 0:
                function_name = self.cache.rev(function_name)
            return f"{function_name}({','.join(values)})"
        else:
            values = [g.to_string(mode) for g in self.generics if g.name]
            if mode == 2:
                values += [f"-{g.to_string(mode)}" for g in self.generics if g.name]
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
                values = ["|".join(sorted({g.realized_name() for g in self[0].generics}))]
            else:
                values = [g.realized_name() for g in self.generics if g.name]
            result = self.name if not values else f"{self.name}[{','.join(values)}]"
        if self.can_realize():
            self._cached_name = result
        return result

    def get_partial_func(self):
        assert self.name == "Partial" and isinstance(self[3][0], Function), "not a partial"
        return self[3][0]

    def get_partial_mask(self) -> str:
        assert self.name == "Partial" and isinstance(self[3][0], StrLiteral), "not a partial"
        return self[0].value

    def is_partial_empty(self):
        args, kwargs = self[1], self[2]
        return len(args.generics) == 1 and not args[0].generics and not kwargs[1].generics


@dataclass(init=False)
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
    def get_static_expr(self):
        # C++ source: codon/parser/ast/types/static.h:34
        raise NotImplementedError

    def get_static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Runtime

    def get_non_static_type(self) -> Type:
        return self.cache.find_class(self.name)


@dataclass(init=False)
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

    def generalize(self, level: int) -> Type:
        return IntLiteral(self.value, copy=self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        return IntLiteral(self.value, copy=self)

    def to_string(self, mode: int):
        return f"{self.value}" if mode < 2 else f"Literal[{self.value}]"

    def get_static_expr(self) -> Expr:
        return IntExpr(int_value=self.value)

    def get_static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Int


@dataclass(init=False)
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

    def generalize(self, level: int) -> Type:
        return StrLiteral(self.value, copy=self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        return StrLiteral(self.value, copy=self)

    def to_string(self, mode: int):
        return f"'{self.value!r}'" if mode < 2 else f"Literal['{self.value!r}']"

    def get_static_expr(self) -> Expr:
        return StringExpr(strings=[StringExpr.String(value=self.value)])

    def get_static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.String


@dataclass(init=False)
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

    def generalize(self, level: int) -> Type:
        return BoolLiteral(self.value, copy=self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        return BoolLiteral(self.value, copy=self)

    def to_string(self, mode: int):
        return f"{self.value}'" if mode < 2 else f"Literal[{self.value}]"

    def get_static_expr(self) -> Expr:
        return BoolExpr(value=self.value)

    def get_static_kind(self) -> Type.Behaviour:
        return Type.Behaviour.Bool


@dataclass(init=False)
class Function(Class):
    """
    A generic type that represents a Codon function instantiation.
    Handles Function[] class.
    """

    ast: FunctionStmt
    # Function generics (e.g. T in def foo[T](...)).
    func_generics: List[Generic]
    # Enclosing class or a function.
    func_parent: Type | None = None

    def __init__(
        self,
        ast: FunctionStmt,
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
            if self.get_func_name() != what.get_func_name() or (
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
                f"generic size mismatch for {self.get_func_name()}"
            )
            for left, right in zip(self.func_generics, what.func_generics):
                if (part := left.type.unify(right.type, undo)) < 0:
                    return part
                score += part
        part = super().unify(what, undo)
        return part if part < 0 else score + part

    def generalize(self, level: int) -> Type:
        return Function(
            self.ast,
            [g.generalize(level) for g in self.func_generics],
            None if self.func_parent is None else self.func_parent.generalize(level),
            copy=super().generalize(level),
        )

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        func_generics = []
        for g in self.func_generics:
            func_generics.append(t := g.instantiate(level, ctx))
            if ctx.cache and t and g.id in ctx.cache:
                ctx.cache[g.id] = t
        return Function(
            self.ast,
            func_generics,
            None if self.func_parent is None else self.func_parent.generalize(level),
            copy=super().generalize(level),
        )

    def has_unbounds(self, include_generics: bool):
        if any(g.type.has_unbounds(include_generics) for g in self.func_generics if g.type):
            return True
        if self.func_parent and self.func_parent.has_unbounds(include_generics):
            return True
        if any(g.type.has_unbounds(include_generics) for g in self if g.type):
            return True
        ret = self.get_ret_type()
        return ret is not None and ret.has_unbounds(include_generics)

    def get_unbounds(self, include_generics: bool) -> List[Type]:
        result: List[Type] = []
        for generic in self.func_generics:
            if generic.type:
                result[0:0] = generic.type.get_unbounds(include_generics)
        if self.func_parent:
            result[0:0] = self.func_parent.get_unbounds(include_generics)
        # Important: return type unbounds are not important, so skip them.
        for generic in self:
            if generic.type:
                result[0:0] = generic.type.get_unbounds(include_generics)
        return result

    def can_realize(self):
        allow_passthrough = self.ast.has(Attr.AllowPassThrough)

        # Important: return type does not have to be realized.
        for arg in self:
            if not isinstance(arg, Function) and not arg.type.can_realize():
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
        ret_type = self.get_ret_type()
        removed = None
        if isinstance(ret_type, Function) and ret_type.func_parent is self:
            removed = ret_type.func_parent
            ret_type.func_parent = None
        result = all(g.type.is_instantiated() for g in self.func_generics if g.type)
        if self.func_parent:
            result = result and self.func_parent.is_instantiated()
        result = result and super().is_instantiated()
        if removed:
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
        ret = self.get_ret_type()
        # Important: return type does not have to be realized.
        if mode == 2:
            assert ret, "function return type is null"
            values.append(f"ret={ret.to_string(mode)}")
        if mode < 2 or self.ast is None:
            for argument in self:
                values.append(argument.to_string(mode))
        else:
            sig_idx = 0
            for param in self.ast:
                if param.is_generic():
                    continue
                values.append(f"{param.name}={self[sig_idx].to_string(mode)}")
                sig_idx += 1
        merged = ",".join(generic_values)
        arguments = ",".join(values)
        merged = arguments if not merged else f"{merged};{arguments}"
        name = self.ast.name
        if mode == 0:
            name = self.cache.rev(name)
        if mode == 2 and self.func_parent:
            merged += f";{self.func_parent.to_string(mode)}"
        return name if not merged else f"{name}[{merged}]"

    def realized_name(self):
        generic_values = [g.realized_name() for g in self.func_generics if g.name]
        argument_values = []
        for arg in self.generics[0].type.generics:
            argument_values.append(
                arg.type.realized_name() if isinstance(arg.type, Function) else arg.realized_name()
            )
        values = ",".join(argument_values + generic_values)
        parent = "" if self.func_parent is None else f"{self.func_parent.realized_name()}:"
        suffix = "" if not values else f"[{values}]"
        return f"{parent}{self.get_func_name()}{suffix}"

    def get_ret_type(self) -> Type | None:
        return self[1]

    def get_func_name(self) -> str:
        return self.ast.name

    def __getitem__(self, index: int) -> Type | None:
        return self.generics[0].type[index].type

    def __iter__(self):
        yield from self.generics[0].type.generics


@dataclass(init=False)
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
        if mode == 2 or not self.generics or not self[0]:
            return super().to_string(mode)
        if not isinstance(self[0], Class):
            return super().to_string(mode)
        values = sorted({g.to_string(mode) for g in self[0].generics})
        joined = "|".join(values)
        return self.name if not joined else f"{self.name}[{joined}]"

    def realized_name(self) -> str:
        assert self.can_realize(), f"cannot realize {self.to_string(2)}"
        return super().realized_name()

    def get_realization_types(self) -> List[Type]:
        assert self.can_realize(), f"cannot realize {self.to_string(2)}"
        assert self.generics and isinstance(self[0], Class), "union realization tuple is null"
        realization = {}
        for generic in self[0].generics:
            if generic.type:
                realization[generic.type.realized_name()] = generic.type
        return [realization[key] for key in sorted(realization)]


@dataclass(init=False)
class Trait(Type):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def can_realize(self):
        return False

    def is_instantiated(self):
        return False

    def realized_name(self):
        return ""


@dataclass(init=False)
class Callable(Trait):
    args: List[Type]

    def __init__(self, args: List[Type] | None = None, **kwargs):
        super().__init__(**kwargs)
        self.args = [] if args is None else args

    def unify(self, what: Type, undo: Type.UnifyContext | None = None) -> int:
        raise NotImplementedError()
        """
        # TODO: one day merge with the CallExpr's logic...
        class_value = typ.get_class()
        if isinstance(class_value, ClassType):
            tr = class_value
            function_holder: Type | None = None
            cache_value = cast(Cache, self.cache)
            type_context = cast(TypeContext, cache_value.type_ctx)
            if typ.is_type(StdlibTypes.TypeWrap):
                type_visitor = TypecheckVisitor(type_context)
                methods = type_visitor.find_method(typ.get_class(), "__call_no_self__")
                function_holder = type_visitor.instantiate_type(methods[0])
                wrapped_class = function_holder.get_class()
                assert isinstance(wrapped_class, ClassType), "bad type wrapper callable"
                tr = wrapped_class

            if tr.name == StdlibTypes.NoneType:
                return 1
            if tr.name != StdlibTypes.Function and tr.get_partial() is None:
                return -1
            if not tr.is_record():
                return -1
            if not self.args:
                return 1

            # C++ comment: codon/parser/ast/types/traits.cpp:47
            # trFun can point to it
            known = ""
            tr_function = tr
            partial = tr.get_partial()
            if partial is not None:
                unbound_count = [0]
                generic_cache: Dict[int, Type] = {}
                partial_function_value = partial.get_partial_func()
                assert isinstance(partial_function_value, FuncType), "bad partial function"
                function_holder = partial_function_value.instantiate(
                    0, unbound_count, generic_cache
                )
                instantiated_class = function_holder.get_class()
                assert isinstance(instantiated_class, ClassType), (
                    "bad instantiated partial function"
                )
                tr_function = instantiated_class
                known = partial.get_partial_mask()

                known_arg_value = partial.generics[1].type
                known_arg_class = None if known_arg_value is None else known_arg_value.get_class()
                assert isinstance(known_arg_class, ClassType), "bad partial arguments"
                partial_func = function_holder.get_func()
                assert not (
                    not isinstance(partial_func, FuncType)
                    or not isinstance(partial_func.ast, FunctionStmt)
                ), "bad partial function AST"
                generic_index = 0
                known_index = 0
                for parameter_index in range(len(known)):
                    parameter = partial_func.ast[parameter_index]
                    if parameter.is_generic():
                        generic_index += 1
                    elif known[parameter_index] == PartialFlag.Included.value:
                        function_arg = partial_func[parameter_index - generic_index]
                        known_arg = known_arg_class.generics[known_index].type
                        if (
                            function_arg is None
                            or known_arg is None
                            or function_arg.unify(known_arg, undo) == -1
                        ):
                            return -1
                        known_index += 1
            else:
                first_generic = tr.generics[0].type
                first_class = None if first_generic is None else first_generic.get_class()
                assert isinstance(first_class, ClassType), "bad function argument tuple"
                known = PartialFlag.Missing.value * len(first_class.generics)

            input_args_value = self.args[0].get_class()
            function_input_value = tr_function.generics[0].type
            function_input_args = (
                None if function_input_value is None else function_input_value.get_class()
            )
            assert not (
                not isinstance(input_args_value, ClassType)
                or not isinstance(function_input_args, ClassType)
            ), "bad callable argument tuple"
            input_args = input_args_value
            tr_input_args = function_input_args
            tr_func_value = tr_function.get_func()
            tr_func = tr_func_value if isinstance(tr_func_value, FuncType) else None
            tr_ast = (
                tr_func.ast
                if tr_func is not None and isinstance(tr_func.ast, FunctionStmt)
                else None
            )
            star = 0
            kw_star = len(tr_input_args.generics)
            total = 0
            if tr_ast is not None:
                star = tr_ast.get_star_args()
                kw_star = tr_ast.get_kw_star_args()
                for function_index in range(len(tr_ast)):
                    if function_index < star and not tr_ast[function_index].is_value():
                        star -= 1
                    if function_index < kw_star and not tr_ast[function_index].is_value():
                        kw_star -= 1
                if kw_star < len(tr_ast) and star >= len(tr_input_args.generics):
                    star -= 1
                pre_star = 0
                for function_index in range(len(tr_ast)):
                    if (
                        function_index != kw_star
                        and known[function_index] != PartialFlag.Included.value
                        and tr_ast[function_index].is_value()
                        and not tr_ast[function_index].name.startswith("$")
                    ):
                        total += 1
                        if function_index < star:
                            pre_star += 1
                if pre_star < total:
                    if len(input_args.generics) < pre_star:
                        return -1
                elif len(input_args.generics) != total:
                    return -1
            else:
                total = len(tr_input_args.generics)
                star = total
                if len(input_args.generics) != total:
                    return -1

            input_index = 0
            function_index = 0
            while input_index < len(input_args.generics) and function_index < star:
                if (
                    known[function_index] != PartialFlag.Included.value
                    and tr_ast is not None
                    and tr_ast[function_index].is_value()
                    and not tr_ast[function_index].name.startswith("$")
                ):
                    input_type = input_args.generics[input_index].type
                    target_type = tr_input_args.generics[function_index].type
                    input_index += 1
                    if (
                        input_type is None
                        or target_type is None
                        or input_type.unify(target_type, undo) == -1
                    ):
                        return -1
                function_index += 1

            type_visitor = TypecheckVisitor(type_context)
            if tr_func is not None:
                # C++ comment: codon/parser/ast/types/traits.cpp:118
                # Make sure to set types of *args/**kwargs so that the function that
                # C++ comment: codon/parser/ast/types/traits.cpp:119
                # is being unified with Callable[] can be realized
                if star < len(tr_input_args.generics) - int(kw_star < len(tr_input_args.generics)):
                    star_arg_types: List[Type] = []
                    if partial is not None:
                        positional_value = partial.generics[1].type
                        positional_class = (
                            None if positional_value is None else positional_value.get_class()
                        )
                        assert not (
                            not isinstance(positional_class, ClassType)
                            or not positional_class.generics
                        ), "bad partial *args/**kwargs"
                        final_value = positional_class.generics[-1].type
                        final_class = None if final_value is None else final_value.get_class()
                        assert isinstance(final_class, ClassType), "bad partial *args/**kwargs"
                        for generic in final_class.generics:
                            assert not (generic.type is None), "bad partial *args"
                            star_arg_types.append(generic.type)
                    while input_index < len(input_args.generics):
                        input_type = input_args.generics[input_index].type
                        assert not (input_type is None), "bad callable argument"
                        star_arg_types.append(input_type)
                        input_index += 1
                    assert isinstance(tr_func.ast, FunctionStmt), "bad callable function AST"
                    star_parameter = tr_func.ast[star]
                    if star_parameter.type is not None:
                        transformed = type_visitor.transform(
                            cast(Expr, star_parameter.type.clone())
                        )
                        assert isinstance(transformed, Expr), "bad *args annotation"
                        # C++ comment: codon/parser/ast/types/traits.cpp:134
                        # if we have *args: type, use those types
                        star_type = type_visitor.extract_type(transformed)
                        star_arg_types = [star_type for _ in star_arg_types]
                    tuple_type = type_visitor.instantiate_type(
                        type_visitor.generate_tuple(len(star_arg_types)), star_arg_types
                    )
                    target_type = tr_input_args.generics[star].type
                    if target_type is None or tuple_type.unify(target_type, undo) == -1:
                        return -1
                if kw_star < len(tr_input_args.generics):
                    tuple_class = type_visitor.generate_tuple(0)
                    tuple_id = 0
                    if partial is not None:
                        kwargs_value = partial.generics[2].type
                        kwargs_class_value = (
                            None if kwargs_value is None else kwargs_value.get_class()
                        )
                        assert not (
                            not isinstance(kwargs_class_value, ClassType)
                            or not kwargs_class_value.is_type(StdlibTypes.NamedTuple)
                        ), "bad partial *args/**kwargs"
                        identifier_value = kwargs_class_value.generics[0].type
                        identifier_static = (
                            None if identifier_value is None else identifier_value.get_int_static()
                        )
                        tuple_id = int(identifier_static.value)
                        tuple_value = kwargs_class_value.generics[1].type
                        tuple_class_value = None if tuple_value is None else tuple_value.get_class()
                        assert isinstance(tuple_class_value, ClassType), "bad partial keyword tuple"
                        tuple_class = tuple_class_value
                    identifier_type = IntStaticType(cache=self.cache, value=tuple_id)
                    keyword_type = type_visitor.instantiate_type(
                        type_visitor.get_stdlib_type(StdlibTypes.NamedTuple),
                        [identifier_type, tuple_class],
                    )
                    target_type = tr_input_args.generics[kw_star].type
                    if target_type is None or keyword_type.unify(target_type, undo) == -1:
                        return -1

                if undo is not None and tr_func.can_realize():
                    # C++ comment: codon/parser/ast/types/traits.cpp:164
                    # Realize if possible to allow deduction of return type
                    realized = type_visitor.realize(tr_func)
                    assert not (realized is None), "cannot realize callable"
                    tr_func.unify(realized, undo)
                return_type = tr_func.get_ret_type()
                if return_type is None or self.args[1].unify(return_type, undo) == -1:
                    return -1
            return 1
        link = typ.get_link()
        from .link import LinkKind, LinkType

        if isinstance(link, LinkType):
            if link.kind is LinkKind.Link and link.type is not None:
                return self.unify(link.type, undo)
            if link.kind is LinkKind.Unbound:
                if link.trait is not None:
                    if not isinstance(link.trait, Callable) or len(link.trait.args) != len(
                        self.args
                    ):
                        return -1
                    for left, right in zip(self.args, link.trait.args):
                        if left.unify(right, undo) == -1:
                            return -1
                return 1
        return -1
        """

    def generalize(self, level: int) -> Type:
        return CallableTrait([arg.generalize(level) for arg in self.args], copy=self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        return CallableTrait([arg.instantiate(level, ctx) for arg in self.args], copy=self)

    def to_string(self, mode: int):
        value = self.args[0].to_string(mode)
        input_value = value.removeprefix("Tuple")
        return f"CallableTrait[{input_value},{self.args[1].to_string(mode)}]"


@dataclass(init=False)
class TypeTrait(Trait):
    type: Type | None = None

    def __init__(self, type: Type | None = None, **kwargs):
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

    def generalize(self, level: int) -> Type:
        return TypeTrait(None if self.type is None else self.type.generalize(level), copy=self)

    def instantiate(self, level: int, ctx: Type.InstantiateContext) -> Type:
        return TypeTrait(
            None if self.type is None else self.type.instantiate(level, ctx), copy=self
        )

    def to_string(self, mode: int):
        name = self.type.name if isinstance(self.type, Class) else "-"
        return f"Trait[{name}]"
