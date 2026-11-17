# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import List, dataclass
from .base import IdMixin, ReplaceableNode
from .value import Value


@dataclass(init=False, eq=False)
class Var(ReplaceableNode, IdMixin):
    """C++: codon/cir/var.h:23, var.cpp:13. CIR variable."""

    _type: object
    _global: bool
    _external: bool
    _tls: bool

    # C++: codon/cir/var.cpp:12
    def __init__(self, typ, global_=False, external=False, tls=False, name=""):
        ReplaceableNode.__init__(self, name)
        IdMixin.__init__(self)
        self._type, self.global_, self.external, self.tls = typ, global_, external, tls

    # C++: codon/cir/var.h:76
    @property
    def type(self):
        """
        @return the type
        """
        return self.get_actual()._type

    # C++: codon/cir/var.h:79
    @type.setter
    def type(self, typ):
        """
        Sets the type.
        @param t the new type
        """
        self.get_actual()._type = typ

    # C++: codon/cir/var.h:82
    @property
    def global_(self):
        """
        @return true if the variable is global
        """
        return self.get_actual()._global

    # C++: codon/cir/var.h:85
    @global_.setter
    def global_(self, value):
        """
        Sets the global flag.
        @param v the new value
        """
        self.get_actual()._global = value

    # C++: codon/cir/var.h:88
    @property
    def external(self):
        """
        @return true if the variable is external
        """
        return self.get_actual()._external

    # C++: codon/cir/var.h:91
    @external.setter
    def external(self, value):
        """
        Sets the external flag.
        @param v the new value
        """
        self.get_actual()._external = value

    # C++: codon/cir/var.h:94
    @property
    def tls(self):
        """
        @return true if the variable is thread-local
        """
        return self.get_actual()._tls

    # C++: codon/cir/var.h:97
    @tls.setter
    def tls(self, value):
        """
        Sets the thread-local flag.
        @param v the new value
        """
        self.get_actual()._tls = value

    # C++: codon/cir/var.h:52
    def replace_used_value(self, old, new):
        # Unlike Value, C++ Var deliberately does not follow its replacement here.
        return self._replace_used_value(old.id if isinstance(old, Value) else old, new)

    # C++: codon/cir/var.h:65
    def get_used_variables(self):
        return self._get_used_variables()

    # C++: codon/cir/var.h:99
    def reference_string(self):
        return f"{self.name}.{self.id}"

    # C++: codon/cir/var.h:58
    def _get_used_types(self):
        return [self._type]

    # C++: codon/cir/var.cpp:16
    def _replace_used_type(self, name, new):
        if self._type.name == name:
            self._type = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class VarValue(Value):
    """C++: codon/cir/var.h:117. Value containing an unowned variable reference."""

    var: Var

    # C++: codon/cir/var.h:128
    def __init__(self, var, name=""):
        """
        Constructs a variable value.
        @param val the referenced value
        @param name the name
        """
        super().__init__(name)
        self.var = var

    # C++: codon/cir/var.h:140
    def _get_type(self):
        return self.var.type

    # C++: codon/cir/var.h:142
    def _get_used_variables(self):
        return [self.var]

    # C++: codon/cir/var.cpp:26
    def _replace_used_variable(self, identifier, new):
        if self.var.id == identifier:
            self.var = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class PointerValue(Value):
    """C++: codon/cir/var.h:147. Pointer to a variable, with an optional field sequence."""

    var: Var
    fields: List[str]

    # C++: codon/cir/var.h:161
    def __init__(self, var, fields=None, name=""):
        """
        Constructs a pointer value.
        @param val the referenced value
        @param fields the sequence of fields, or empty to get var pointer
        @param name the name
        """
        super().__init__(name)
        self.var = var
        self.fields = [] if fields is None else list(fields)

    # C++: codon/cir/var.cpp:36
    def _get_type(self):
        # C++ var.cpp:34 deliberately uses the variable's type, not field types.
        return self.module.get_pointer_type(self.var.type)

    # C++: codon/cir/var.h:188
    def _get_used_variables(self):
        return [self.var]

    # C++: codon/cir/var.cpp:40
    def _replace_used_variable(self, identifier, new):
        if self.var.id == identifier:
            self.var = new
            return 1
        return 0
