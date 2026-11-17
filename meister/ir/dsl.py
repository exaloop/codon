# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ..bridge import dataclass
from .const import Const
from .flow import Flow
from .instr import Instr
from .type import Type
from .util.side_effect import SideEffectStatus


@dataclass(init=False, eq=False)
class CustomType(Type):
    """C++: codon/cir/dsl/nodes.h:28. DSL type extension interface."""

    # C++: codon/cir/dsl/nodes.h:35
    def get_builder(self):
        """
        Return the type builder (LLVM builders are not ported).

        @return the type builder
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:40
    def match(self, other):
        """
        Compare DSL nodes.

        Compares DSL nodes.
        @param v the other node
        @return true if they match
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:44
    def do_format(self):
        """
        Format the DSL node.

        Format the DSL node.
        @param os the output stream
        """
        raise NotImplementedError


@dataclass(init=False, eq=False)
class CustomConst(Const):
    """C++: codon/cir/dsl/nodes.h:48. DSL constant extension interface."""

    # C++: codon/cir/dsl/nodes.h:55
    def get_builder(self):
        """
        @return the value builder
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:59
    def match(self, other):
        """
        Compares DSL nodes.
        @param v the other node
        @return true if they match
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:63
    def do_clone(self, visitor):
        """
        Clones the value.
        @param cv the clone visitor
        @return a clone of the object
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:67
    def do_format(self):
        """
        Format the DSL node.
        @param os the output stream
        """
        raise NotImplementedError


@dataclass(init=False, eq=False)
class CustomFlow(Flow):
    """C++: codon/cir/dsl/nodes.h:71. DSL flow extension interface."""

    # C++: codon/cir/dsl/nodes.h:78
    def get_builder(self):
        """
        @return the value builder
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:82
    def match(self, other):
        """
        Compares DSL nodes.
        @param v the other node
        @return true if they match
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:86
    def do_clone(self, visitor):
        """
        Clones the value.
        @param cv the clone visitor
        @return a clone of the object
        """
        raise NotImplementedError

    # C++ API: codon/cir/dsl/nodes.h:71
    def get_cf_builder(self):
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:97
    def get_side_effect_status(self, local=True):
        """
        Local status describes this node alone; nonlocal status describes
        functions containing it. A break has local effects, but its containing
        function can still be side-effect-free. Default is unknown.

        Query this custom node for its side effect properties. If "local"
        is true, then the return value should reflect this node and this
        node alone, otherwise the value should reflect functions containing
        this node in their bodies. For example, a "break" instruction has
        side effects locally, but functions containing "break" might still
        be side effect free, hence the distinction.
        @param local true if result should reflect only this node
        @return this node's side effect status
        """
        return SideEffectStatus.UNKNOWN

    # C++: codon/cir/dsl/nodes.h:103
    def do_format(self):
        """
        Format the DSL node.
        @param os the output stream
        """
        raise NotImplementedError


@dataclass(init=False, eq=False)
class CustomInstr(Instr):
    """C++: codon/cir/dsl/nodes.h:107. DSL instruction extension interface."""

    # C++: codon/cir/dsl/nodes.h:114
    def get_builder(self):
        """
        @return the value builder
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:118
    def match(self, other):
        """
        Compares DSL nodes.
        @param v the other node
        @return true if they match
        """
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:122
    def do_clone(self, visitor):
        """
        Clones the value.
        @param cv the clone visitor
        @return a clone of the object
        """
        raise NotImplementedError

    # C++ API: codon/cir/dsl/nodes.h:107
    def get_cf_builder(self):
        raise NotImplementedError

    # C++: codon/cir/dsl/nodes.h:133
    def get_side_effect_status(self, local=True):
        """
        Local effects can differ from effects of a function containing this node.

        Query this custom node for its side effect properties. If "local"
        is true, then the return value should reflect this node and this
        node alone, otherwise the value should reflect functions containing
        this node in their bodies. For example, a "break" instruction has
        side effects locally, but functions containing "break" might still
        be side effect free, hence the distinction.
        @param local true if result should reflect only this node
        @return this node's side effect status
        """
        return SideEffectStatus.UNKNOWN

    # C++: codon/cir/dsl/nodes.h:139
    def do_format(self):
        """
        Format the DSL node.
        @param os the output stream
        """
        raise NotImplementedError
