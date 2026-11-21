# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass


@dataclass(init=False, eq=False)
class Analysis:
    """C++: codon/cir/analyze/analysis.h. Analysis managed by the CIR pass pipeline."""

    manager: object | None

    def __init__(self):
        self.manager = None

    def get_key(self):
        return self.KEY

    def set_manager(self, manager):
        self.manager = manager

    def get_analysis_result(self, key):
        return self.manager.get_analysis_result(key)
