# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import Dict, dataclass


@dataclass(init=False, eq=False)
class IdMixin:
    """C++: codon/cir/base.h:27. Mixin class for IR nodes that need ids."""

    # The global id counter, shared by values and variables.
    current_id = 0
    _id: int

    # C++: codon/cir/base.h:40
    def __init__(self):
        self._id = IdMixin.current_id
        IdMixin.current_id += 1

    # C++: codon/cir/base.cpp:15
    @staticmethod
    def reset_id():
        """Reset the global id counter."""
        IdMixin.current_id = 0

    # C++: codon/cir/base.h:43
    @property
    def id(self):
        """
        @return the node's id.
        """
        return self.get_actual()._id


@dataclass(init=False, eq=False)
class Node:
    """C++: codon/cir/base.h:47. Base for named IR nodes."""

    # The node's name, module, replacement and key-value attribute store.
    _name: str
    _module: object
    replacement: Node | None
    attributes: Dict[int, object]

    # C++: codon/cir/base.cpp:23
    def __init__(self, name=""):
        self._name = name
        self._module = None
        self.replacement = None
        self.attributes = {}

    # C++ API: codon/cir/base.h:47
    def __bool__(self):
        # A non-null C++ pointer is true, including an empty flow or record.
        return True

    # C++: codon/cir/base.h:271
    def get_actual(self):
        return self.replacement.get_actual() if self.replacement is not None else self

    # C++: codon/cir/base.h:270
    def has_replacement(self):
        return self.replacement is not None

    # C++: codon/cir/base.h:97
    @property
    def name(self):
        """
        @return the node's name
        """
        return self.get_actual()._name

    # C++: codon/cir/base.h:100
    @name.setter
    def name(self, value):
        """
        Sets the node's name
        @param n the new name
        """
        self.get_actual()._name = value

    # C++: codon/cir/base.h:198
    @property
    def module(self):
        """
        @return the IR module
        """
        return self.get_actual()._module

    # C++: codon/cir/base.h:201
    @module.setter
    def module(self, value):
        """
        Sets the module.
        @param m the new module
        """
        self.get_actual()._module = value

    # C++: codon/cir/base.h:104
    def accept(self, visitor):
        # C++'s extensible RTTI/AcceptorExtend becomes Python class dispatch.
        """
        Accepts visitors.
        @param v the visitor
        """
        return visitor.visit(self.get_actual())

    # C++: codon/cir/base.h:112
    def set_attribute(self, value, key=None):
        """
        Sets an attribute
        @param the attribute key
        @param value the attribute
        """
        self.get_actual().attributes[value.ATTRIBUTE_ID if key is None else attribute_key(key)] = (
            value
        )

    # C++: codon/cir/base.h:124
    def has_attribute(self, key):
        """
        @param n attribute ID
        @return true if the attribute is in the store
        """
        return attribute_key(key) in self.get_actual().attributes

    # C++: codon/cir/base.h:135
    def get_attribute(self, key):
        """
        Gets the appropriate attribute.
        @param key the attribute key
        """
        return self.get_actual().attributes.get(attribute_key(key))

    # C++: codon/cir/base.h:165
    def erase_attribute(self, key):
        # C++ deliberately erases this node's store, not its replacement's store.
        self.attributes.pop(attribute_key(key), None)

    # C++: codon/cir/base.h:166
    def clone_attributes_from(self, node):
        self.attributes = {key: value.clone() for key, value in node.attributes.items()}

    # C++: codon/cir/base.h:189
    @property
    def info(self):
        """
        @return the src info
        """
        from ..ast.nodes import Node as ASTNode
        from .attribute import SrcInfoAttribute

        attr = self.get_attribute(SrcInfoAttribute)
        return attr.info if attr else ASTNode.SrcInfo()

    # C++: codon/cir/base.h:185
    @info.setter
    def info(self, value):
        """
        Helper to add source information.
        @param the source information
        """
        from .attribute import SrcInfoAttribute

        self.set_attribute(SrcInfoAttribute(value))

    # C++: codon/cir/base.h:195
    def reference_string(self):
        """
        @return a text representation of a reference to the object
        """
        return self.name

    # C++: codon/cir/base.h:208
    def get_used_values(self):
        """
        @return a vector of all the node's children
        """
        return self.get_actual()._get_used_values()

    # C++ API: codon/cir/base.h:47
    def _get_used_values(self):
        return []

    # C++: codon/cir/base.cpp:27
    def replace_used_value(self, old, new):
        return self.get_actual()._replace_used_value(old.id if isinstance(old, Node) else old, new)

    # C++ API: codon/cir/base.h:47
    def _replace_used_value(self, identifier, new):
        return 0

    # C++: codon/cir/base.h:223
    def get_used_types(self):
        """
        @return a vector of all the node's used types
        """
        return self.get_actual()._get_used_types()

    # C++ API: codon/cir/base.h:47
    def _get_used_types(self):
        return []

    # C++: codon/cir/base.cpp:31
    def replace_used_type(self, old, new):
        return self.get_actual()._replace_used_type(old.name if isinstance(old, Node) else old, new)

    # C++ API: codon/cir/base.h:47
    def _replace_used_type(self, name, new):
        return 0

    # C++: codon/cir/base.h:236
    def get_used_variables(self):
        """
        @return a vector of all the node's used variables
        """
        return self.get_actual()._get_used_variables()

    # C++ API: codon/cir/base.h:47
    def _get_used_variables(self):
        return []

    # C++: codon/cir/base.cpp:35
    def replace_used_variable(self, old, new):
        return self.get_actual()._replace_used_variable(
            old.id if isinstance(old, Node) else old, new
        )

    # C++ API: codon/cir/base.h:47
    def _replace_used_variable(self, identifier, new):
        return 0

    # C++ API: codon/cir/base.h:47
    def __str__(self):
        from .util.format import format_ir

        return format_ir(self)


@dataclass(init=False, eq=False)
class ReplaceableNode(Node):
    """C++: codon/cir/base.h:294. Nodes supporting lazy replacement."""

    replaceable: bool

    def __init__(self, name=""):
        super().__init__(name)
        self.replaceable = True

    def replace_all(self, node):
        """Lazily replace all instances of the node."""
        assert self.replaceable, "node not replaceable"
        self.replacement = node


def attribute_key(key):
    from ..bridge import Enum

    if isinstance(key, Enum):
        return key.value
    return key if isinstance(key, int) else key.ATTRIBUTE_ID


# C++: codon/cir/base.h:331
def cast(cls, node):
    """C++: codon/cir/base.h:329. Replacement-aware extensible RTTI cast."""
    if node is None:
        return None
    actual = node.get_actual()
    return actual if isinstance(actual, cls) else None


# C++: codon/cir/base.h:339
def is_a(cls, node):
    return cast(cls, node) is not None


def replace_in_list(values, identifier, new):
    """C++: instr.cpp:13, flow.cpp:14, func.cpp:18. Replace every matching id."""
    count = 0
    for idx, value in enumerate(values):
        if value.id == identifier:
            values[idx] = new
            count += 1
    return count
