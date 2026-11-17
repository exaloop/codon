# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy

from ..bridge import Dict, List, dataclass


@dataclass(init=False, eq=False)
class Attribute:
    """C++: codon/cir/attribute.h:28. Base for CIR attributes."""

    # C++: codon/cir/attribute.h:32
    def needs_clone(self):
        """
        Whether the attribute should be propagated across clones.

        @return true if the attribute should be propagated across clones
        """
        return True

    # C++: codon/cir/attribute.h:39
    def clone(self, visitor=None):
        """
        @return a clone of the attribute
        """
        return Attribute()

    # C++: codon/cir/attribute.h:49
    def force_clone(self, visitor):
        """
        @return a clone of the attribute
        """
        return self.clone(visitor)

    # C++: codon/cir/attribute.h:35
    def __str__(self):
        return ""


@dataclass(init=False, eq=False)
class ExactTypeAttribute(Attribute):
    """C++: codon/cir/attribute.h:91. Exact reference type information."""

    ATTRIBUTE_ID = 114
    type: object

    # C++: codon/cir/attribute.h:95
    def __init__(self, typ):
        self.type = typ

    # C++: codon/cir/attribute.h:96
    def needs_clone(self):
        return False

    # C++: codon/cir/attribute.cpp:28
    def __str__(self):
        return self.type.name


@dataclass(init=False, eq=False)
class SrcInfoAttribute(Attribute):
    """C++: codon/cir/attribute.h:103. Attribute containing SrcInfo."""

    ATTRIBUTE_ID = 107
    info: object

    # C++: codon/cir/attribute.h:109
    def __init__(self, info=None):
        from ..ast.nodes import Node

        self.info = Node.SrcInfo() if info is None else info

    # C++: codon/cir/attribute.h:114
    def clone(self, visitor=None):
        return copy.copy(self)

    # C++: codon/cir/attribute.h:119
    def __str__(self):
        return str(self.info)


@dataclass(init=False, eq=False)
class StringValueAttribute(Attribute):
    """C++: codon/cir/attribute.h:123. Attribute containing a string value."""

    ATTRIBUTE_ID = 101
    value: str

    # C++: codon/cir/attribute.h:128
    def __init__(self, value=""):
        self.value = value

    # C++: codon/cir/attribute.h:132
    def clone(self, visitor=None):
        return type(self)(self.value)

    # C++: codon/cir/attribute.h:137
    def __str__(self):
        return str(self.value)


@dataclass(init=False, eq=False)
class DocstringAttribute(Attribute):
    """C++: codon/cir/attribute.h:141. Attribute containing docstring from source."""

    ATTRIBUTE_ID = 108
    docstring: str

    # C++: codon/cir/attribute.h:147
    def __init__(self, docstring=""):
        self.docstring = docstring

    # C++: codon/cir/attribute.h:152
    def clone(self, visitor=None):
        return DocstringAttribute(self.docstring)

    # C++: codon/cir/attribute.h:157
    def __str__(self):
        return self.docstring


@dataclass(init=False, eq=False)
class KeyValueAttribute(Attribute):
    """C++: codon/cir/attribute.h:161. Attribute containing function information."""

    ATTRIBUTE_ID = 104
    attributes: Dict[str, str]

    # C++: codon/cir/attribute.h:167
    def __init__(self, attributes=None):
        self.attributes = {} if attributes is None else dict(attributes)

    # C++: codon/cir/attribute.cpp:37
    def has(self, key):
        return key in self.attributes

    # C++: codon/cir/attribute.cpp:41
    def get(self, key):
        return self.attributes.get(key, "")

    # C++: codon/cir/attribute.h:182
    def clone(self, visitor=None):
        return KeyValueAttribute(self.attributes)

    # C++: codon/cir/attribute.cpp:46
    def __str__(self):
        return ",".join(self.attributes.values())


@dataclass(init=False, eq=False)
class StringListAttribute(Attribute):
    """C++: codon/cir/attribute.h:191. Attribute containing function information."""

    ATTRIBUTE_ID = 103
    values: List[str]

    # C++: codon/cir/attribute.h:197
    def __init__(self, values=None):
        self.values = [] if values is None else list(values)

    # C++: codon/cir/attribute.h:203
    def clone(self, visitor=None):
        return StringListAttribute(self.values)

    # C++: codon/cir/attribute.cpp:32
    def __str__(self):
        return ",".join(self.values)


@dataclass(init=False, eq=False)
class MemberAttribute(Attribute):
    """C++: codon/cir/attribute.h:212. Attribute containing type member information."""

    ATTRIBUTE_ID = 105
    member_src_info: Dict[str, object]

    # C++: codon/cir/attribute.h:218
    def __init__(self, member_src_info=None):
        self.member_src_info = {} if member_src_info is None else dict(member_src_info)

    # C++: codon/cir/attribute.h:224
    def clone(self, visitor=None):
        return MemberAttribute(self.member_src_info)

    # C++: codon/cir/attribute.cpp:54
    def __str__(self):
        return (
            "("
            + ",".join(f"{key}={value}" for key, value in sorted(self.member_src_info.items()))
            + ")"
        )


@dataclass(init=False, eq=False)
class PythonWrapperAttribute(Attribute):
    """C++: codon/cir/attribute.h:233. Marks Python wrappers of Codon functions."""

    ATTRIBUTE_ID = 106
    original: object

    # C++: codon/cir/attribute.h:241
    def __init__(self, original):
        """
        Constructs a PythonWrapperAttribute.
        @param original the function being wrapped
        """
        self.original = original

    # C++: codon/cir/attribute.h:243
    def needs_clone(self):
        return False

    # C++: codon/cir/attribute.cpp:62
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return PythonWrapperAttribute(visitor.clone(self.original))

    # C++: codon/cir/attribute.cpp:67
    def force_clone(self, visitor):
        return PythonWrapperAttribute(visitor.force_clone(self.original))

    # C++: codon/cir/attribute.cpp:71
    def __str__(self):
        return f"(pywrap {self.original.reference_string()})"


@dataclass(init=False, eq=False)
class TupleLiteralAttribute(Attribute):
    """C++: codon/cir/attribute.h:257. IR structures corresponding to tuple literals."""

    ATTRIBUTE_ID = 109
    # Values contained in tuple literal.
    elements: List[object]

    # C++: codon/cir/attribute.h:263
    def __init__(self, elements):
        self.elements = list(elements)

    # C++: codon/cir/attribute.cpp:76
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return type(self)([visitor.clone(value) for value in self.elements])

    # C++: codon/cir/attribute.cpp:84
    def force_clone(self, visitor):
        return type(self)([visitor.force_clone(value) for value in self.elements])

    # C++: codon/cir/attribute.cpp:91
    def __str__(self):
        return "(" + ",".join(str(value) for value in self.elements) + ")"


@dataclass(init=False, eq=False)
class LiteralElement:
    """C++: codon/cir/attribute.h:278. Information about an element in a collection literal."""

    # The element value; star is true if preceded by "*", as in "[*x]".
    value: object
    star: bool

    # C++ API: codon/cir/attribute.h:278
    def __init__(self, value, star=False):
        self.value, self.star = value, star


@dataclass(init=False, eq=False)
class ListLiteralAttribute(Attribute):
    """C++: codon/cir/attribute.h:286. IR structures corresponding to list literals."""

    ATTRIBUTE_ID = 111
    # Elements contained in list literal.
    elements: List[LiteralElement]

    # C++: codon/cir/attribute.h:292
    def __init__(self, elements):
        self.elements = list(elements)

    # C++: codon/cir/attribute.cpp:99
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return type(self)([LiteralElement(visitor.clone(e.value), e.star) for e in self.elements])

    # C++: codon/cir/attribute.cpp:107
    def force_clone(self, visitor):
        return type(self)(
            [LiteralElement(visitor.force_clone(e.value), e.star) for e in self.elements]
        )

    # C++: codon/cir/attribute.cpp:114
    def __str__(self):
        return "[" + ",".join(f"{'*' if e.star else ''}{e.value}" for e in self.elements) + "]"


@dataclass(init=False, eq=False)
class SetLiteralAttribute(Attribute):
    """C++: codon/cir/attribute.h:307. IR structures corresponding to set literals."""

    # Deliberately the same ID as ListLiteralAttribute, as in C++.
    ATTRIBUTE_ID = 111
    # Elements contained in set literal.
    elements: List[LiteralElement]

    # C++: codon/cir/attribute.h:313
    def __init__(self, elements):
        self.elements = list(elements)

    # C++: codon/cir/attribute.cpp:122
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return SetLiteralAttribute(
            [LiteralElement(visitor.clone(e.value), e.star) for e in self.elements]
        )

    # C++: codon/cir/attribute.cpp:130
    def force_clone(self, visitor):
        return SetLiteralAttribute(
            [LiteralElement(visitor.force_clone(e.value), e.star) for e in self.elements]
        )

    # C++: codon/cir/attribute.cpp:137
    def __str__(self):
        return "set([" + ",".join(f"{'*' if e.star else ''}{e.value}" for e in self.elements) + "])"


@dataclass(init=False, eq=False)
class KeyValuePair:
    """C++: codon/cir/attribute.h:329. A literal key and value (null for star unpacking)."""

    key: object
    value: object

    # C++ API: codon/cir/attribute.h:329
    def __init__(self, key, value=None):
        self.key, self.value = key, value


@dataclass(init=False, eq=False)
class DictLiteralAttribute(Attribute):
    """C++: codon/cir/attribute.h:328. IR structures corresponding to dict literals."""

    ATTRIBUTE_ID = 112
    KeyValuePair = KeyValuePair
    # Keys and values contained in dict literal.
    elements: List[KeyValuePair]

    # C++: codon/cir/attribute.h:341
    def __init__(self, elements):
        self.elements = list(elements)

    # C++: codon/cir/attribute.cpp:146
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return type(self)(
            [
                KeyValuePair(visitor.clone(e.key), visitor.clone(e.value) if e.value else None)
                for e in self.elements
            ]
        )

    # C++: codon/cir/attribute.cpp:155
    def force_clone(self, visitor):
        return type(self)(
            [
                KeyValuePair(
                    visitor.force_clone(e.key), visitor.force_clone(e.value) if e.value else None
                )
                for e in self.elements
            ]
        )

    # C++: codon/cir/attribute.cpp:163
    def __str__(self):
        return (
            "dict(["
            + ",".join(f"{e.key}:{e.value}" if e.value else f"**{e.key}" for e in self.elements)
            + "])"
        )


@dataclass(init=False, eq=False)
class PartialFunctionAttribute(Attribute):
    """C++: codon/cir/attribute.h:356. IR structures corresponding to partial functions."""

    ATTRIBUTE_ID = 113
    name: str
    # Partial arguments, or null if none: f(a, ..., b) has elements [a, null, b].
    args: List[object]

    # C++: codon/cir/attribute.h:366
    def __init__(self, name, args):
        self.name, self.args = name, list(args)

    # C++: codon/cir/attribute.cpp:178
    def clone(self, visitor=None):
        assert visitor is not None, "cannot operate without CloneVisitor"
        return PartialFunctionAttribute(self.name, [visitor.clone(value) for value in self.args])

    # C++: codon/cir/attribute.cpp:186
    def force_clone(self, visitor):
        return PartialFunctionAttribute(
            self.name, [visitor.force_clone(value) for value in self.args]
        )

    # C++: codon/cir/attribute.cpp:193
    def __str__(self):
        return (
            self.name + "(" + ",".join(str(value) if value else "..." for value in self.args) + ")"
        )


@dataclass(init=False, eq=False)
class IntValueAttribute(Attribute):
    """C++: codon/cir/attribute.h:380. Attribute containing an integer value."""

    ATTRIBUTE_ID = 102
    value: int

    # C++: codon/cir/attribute.h:385
    def __init__(self, value=0):
        self.value = value

    # C++: codon/cir/attribute.h:389
    def clone(self, visitor=None):
        return IntValueAttribute(self.value)

    # C++: codon/cir/attribute.h:394
    def __str__(self):
        return str(self.value)
