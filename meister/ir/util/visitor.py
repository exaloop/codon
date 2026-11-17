# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ...bridge import dataclass


@dataclass(init=False, eq=False)
class Visitor:
    """C++: codon/cir/util/visitor.h, visitor.cpp. Extensible IR visitor.

    Every concrete C++ visit overload defaults directly to defaultVisit rather
    than dispatching through overloads for base classes.
    """

    # C++: codon/cir/util/visitor.cpp:11
    def visit(self, node):
        node = node.get_actual()
        method = getattr(self, "visit_" + type(node).__name__, self.default_visit)
        return method(node)

    # C++: codon/cir/util/visitor.h:91
    def default_visit(self, node):
        pass


ConstVisitor = Visitor
