# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ...bridge import List, dataclass
from ..util.operator import Operator


@dataclass(init=False, eq=False)
class Pass:
    """C++: codon/cir/transform/pass.h:21. General pass base class."""

    manager: object | None

    # C++: codon/cir/transform/pass.h:26
    def __init__(self):
        self.manager = None

    # C++: codon/cir/transform/pass.h:29
    def get_key(self):
        """
        @return a unique key for this pass
        """
        return self.KEY

    # C++: codon/cir/transform/pass.h:31
    def should_run(self, module):
        return True

    # C++: codon/cir/transform/pass.h:35
    def run(self, module):
        """
        Execute the pass.
        @param module the module
        """
        raise NotImplementedError

    # C++: codon/cir/transform/pass.h:40
    def should_repeat(self, count):
        """
        Determine if pass should repeat.
        @param num how many times this pass has already run
        @return true if pass should repeat
        """
        return False

    # C++: codon/cir/transform/pass.cpp:23
    def set_manager(self, manager):
        self.manager = manager

    # C++: codon/cir/transform/pass.h:49
    def get_analysis_result(self, key):
        """
        Returns the result of a given analysis.
        @param key the analysis key
        @return the analysis result
        """
        return self.manager.results.get(key) if self.manager else None

    # C++: codon/cir/transform/pass.cpp:15
    def get_options(self):
        return self.manager.options if self.manager else None


@dataclass(init=False, eq=False)
class PassGroup(Pass):
    """C++: codon/cir/transform/pass.h:61, pass.cpp:18. Ordered group with a repeat limit."""

    repeat: int
    passes: List[Pass]

    # C++: codon/cir/transform/pass.h:67
    def __init__(self, repeat=0, passes=None):
        super().__init__()
        self.repeat, self.passes = repeat, [] if passes is None else list(passes)

    # C++: codon/cir/transform/pass.h:81
    def should_repeat(self, count):
        return count < self.repeat

    # C++: codon/cir/transform/pass.cpp:17
    def run(self, module):
        for pass_ in self.passes:
            pass_.run(module)

    # C++: codon/cir/transform/pass.cpp:22
    def set_manager(self, manager):
        super().set_manager(manager)
        for pass_ in self.passes:
            pass_.set_manager(manager)


@dataclass(init=False, eq=False)
class OperatorPass(Pass, Operator):
    """C++: codon/cir/transform/pass.h:89. Pass that runs a single Operator."""

    # C++: codon/cir/transform/pass.h:93
    def __init__(self, children_first=False):
        """
        Constructs an operator pass.
        @param childrenFirst true if children should be iterated first
        """
        Pass.__init__(self)
        Operator.__init__(self, children_first)

    # C++: codon/cir/transform/pass.h:95
    def run(self, module):
        self.reset()
        self.process(module)
