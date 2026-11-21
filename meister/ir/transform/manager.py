# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ...bridge import Dict, List, dataclass


@dataclass(init=False, eq=False)
class KeyManager:
    """C++: codon/cir/transform/manager.h:25. Unique keys, suffixing duplicates with :<number>."""

    keys: Dict[str, int]

    # C++: codon/cir/transform/manager.h:31
    def __init__(self):
        self.keys = {}

    # C++: codon/cir/transform/manager.cpp:40
    def get_unique_key(self, key):
        # Ensure generated unique keys cannot collide with raw keys.
        assert ":" not in key, f"pass key '{key}' contains invalid character ':'"
        if key not in self.keys:
            self.keys[key] = 1
            return key
        self.keys[key] += 1
        return f"{key}:{self.keys[key]}"


@dataclass(init=False, eq=False)
class PassMetadata:
    """C++: codon/cir/transform/manager.h:41. Pass instance, required and invalidated analyses."""

    pass_: object
    reqs: List[str]
    invalidates: List[str]

    # C++: codon/cir/transform/manager.h:49
    def __init__(self, pass_, reqs, invalidates):
        self.pass_, self.reqs, self.invalidates = pass_, list(reqs), list(invalidates)


@dataclass(init=False, eq=False)
class AnalysisMetadata:
    """C++: codon/cir/transform/manager.h:60. Analysis instance and required analyses."""

    analysis: object
    reqs: List[str]
    invalidates: List[str]

    # C++: codon/cir/transform/manager.h:68
    def __init__(self, analysis, reqs):
        self.analysis, self.reqs = analysis, list(reqs)
        self.invalidates = []


@dataclass(init=False, eq=False)
class PassManager:
    """C++: codon/cir/transform/manager.h:22, manager.cpp. Pass scheduling and cached analyses."""

    options: object
    km: KeyManager
    passes: Dict[str, PassMetadata]
    analyses: Dict[str, AnalysisMetadata]
    deps: Dict[str, List[str]]
    execution_order: List[str]
    results: Dict[str, object]

    # C++: codon/cir/transform/manager.h:96
    def __init__(self, options):
        self.options, self.km = options, KeyManager()
        self.passes, self.analyses, self.deps = {}, {}, {}
        self.execution_order, self.results = [], {}
        self.register_standard_passes()

    # C++: codon/cir/transform/manager.h:108
    def has_pass(self, key):
        """
        Checks if the given pass is included in this manager.
        @param key the pass key
        @return true if manager has the given pass
        """
        return key in self.passes

    # C++: codon/cir/transform/manager.h:119
    def has_analysis(self, key):
        """
        Checks if the given analysis is included in this manager.
        @param key the analysis key
        @return true if manager has the given analysis
        """
        return key in self.analyses

    # C++: codon/cir/transform/manager.h:160
    def is_disabled(self, key):
        """
        Returns whether a given pass or analysis is disabled.
        @param key the (unique'd) pass or analysis key
        @return true if the pass or analysis is disabled
        """
        return key in getattr(self.options, "disabled", ())

    # C++: codon/cir/transform/manager.h:152
    def get_analysis_result(self, key):
        """
        Gets the result of a given analysis.
        @param key the (unique'd) analysis key
        @return the result
        """
        return self.results.get(key)

    # C++: codon/cir/transform/manager.cpp:54
    def register_pass(self, pass_, insert_before="", reqs=(), invalidates=()):
        key = pass_.get_key()
        if self.is_disabled(key):
            return ""
        key = self.km.get_unique_key(key)
        for req in reqs:
            assert req in self.deps, f"required key '{req}' not found"
            self.deps[req].append(key)
        self.passes[key] = PassMetadata(pass_, reqs, invalidates)
        pass_.set_manager(self)
        if not insert_before:
            self.execution_order.append(key)
        else:
            assert insert_before in self.execution_order, (
                f"pass with key '{insert_before}' not found in manager"
            )
            self.execution_order.insert(self.execution_order.index(insert_before), key)
        return key

    # C++: codon/cir/transform/manager.cpp:82
    def register_analysis(self, analysis, reqs=()):
        key = analysis.get_key()
        if self.is_disabled(key):
            return ""
        key = self.km.get_unique_key(key)
        for req in reqs:
            assert req in self.deps, f"required key '{req}' not found"
            self.deps[req].append(key)
        self.analyses[key] = AnalysisMetadata(analysis, reqs)
        analysis.set_manager(self)
        self.deps[key] = []
        return key

    # C++: codon/cir/transform/manager.cpp:102
    def run(self, module):
        for name in self.execution_order:
            self.run_pass(module, name)

    # C++: codon/cir/transform/manager.cpp:108
    def run_pass(self, module, name):
        metadata, count, repeat = self.passes[name], 0, True
        while repeat:
            if not metadata.pass_.should_run(module):
                break
            for dependency in metadata.reqs:
                self.run_analysis(module, dependency)
            metadata.pass_.run(module)
            for invalidated in metadata.invalidates:
                self.invalidate(invalidated)
            count += 1
            repeat = metadata.pass_.should_repeat(count)

    # C++: codon/cir/transform/manager.cpp:132
    def run_analysis(self, module, name):
        if name in self.results:
            return
        metadata = self.analyses[name]
        for dependency in metadata.reqs:
            self.run_analysis(module, dependency)
        self.results[name] = metadata.analysis.run(module)

    # C++: codon/cir/transform/manager.cpp:146
    def invalidate(self, key):
        open_ = {key}
        while open_:
            new_open = set()
            for dependency in open_:
                if dependency in self.results:
                    del self.results[dependency]
                    new_open.update(self.deps[dependency])
            open_ = new_open

    # C++: codon/cir/transform/manager.cpp:161
    def register_standard_passes(self):
        """C++: manager.cpp:161. Register the standard CIR pipeline."""
        if getattr(self.options, "pmempty", False):
            return
        from .lowering.async_for import AsyncForLowering
        from .lowering.await_ import AwaitLowering
        from .lowering.imperative import ImperativeForFlowLowering
        from .lowering.pipeline import PipelineLowering
        from .lowering.return_ import MissingReturnLowering
        from .lowering.throw import ThrowLowering
        from .parallel.openmp import OpenMPPass
        from .pythonic.format import FormattingOptimization

        lowerings = (PipelineLowering, ImperativeForFlowLowering, MissingReturnLowering, ThrowLowering)
        if self.options.debug and not getattr(self.options, "test", False):
            for cls in (*lowerings, AsyncForLowering, AwaitLowering, OpenMPPass, FormattingOptimization):
                self.register_pass(cls())
            return

        from ..analyze.dataflow.capture import CaptureAnalysis
        from ..analyze.dataflow.cfg import CFAnalysis
        from ..analyze.dataflow.dominator import DominatorAnalysis
        from ..analyze.dataflow.reaching import RDAnalysis
        from ..analyze.module.global_vars import GlobalVarsAnalyses
        from ..analyze.module.side_effect import SideEffectAnalysis
        from .cleanup.devirtualize import DevirtualizationPass
        from .folding.folding import FoldingPassGroup
        from .numpy.fusion import NumPyExpressionAnalysis, NumPyFusionPass
        from .numpy.indexing import NumPyBoundsCheckElisionPass
        from .numpy.inline import NumPyInlinePass
        from .numpy.lifetime import NumPyLifetimePass
        from .pythonic.dict import DictArithmeticOptimization
        from .pythonic.enumerate import EnumerateOptimization
        from .pythonic.generator import GeneratorArgumentOptimization, GeneratorLoopFusion
        from .pythonic.io import IOCatOptimization
        from .pythonic.list import ListAdditionOptimization
        from .pythonic.str import StrAdditionOptimization

        # Pythonic optimizations and lowering.
        for cls in (
            DictArithmeticOptimization,
            ListAdditionOptimization,
            StrAdditionOptimization,
            GeneratorArgumentOptimization,
            EnumerateOptimization,
            IOCatOptimization,
            FormattingOptimization,
            *lowerings,
        ):
            self.register_pass(cls())

        # Folding and array analyses.
        cfg = self.register_analysis(CFAnalysis())
        rd = self.register_analysis(RDAnalysis(cfg), (cfg,))
        dom = self.register_analysis(DominatorAnalysis(cfg), (cfg,))
        cap = self.register_analysis(CaptureAnalysis(rd, dom), (rd, dom))
        globals_ = self.register_analysis(GlobalVarsAnalyses())
        se1 = self.register_analysis(SideEffectAnalysis(cap, True), (cap,))
        se2 = self.register_analysis(SideEffectAnalysis(cap, False), (cap,))
        numpy = self.register_analysis(NumPyExpressionAnalysis(rd, se2), (rd, se2))

        analysis_keys = (rd, cfg, globals_, cap)
        self.register_pass(NumPyInlinePass(numpy), reqs=(numpy,), invalidates=(se1, se2, *analysis_keys))
        self.register_pass(
            FoldingPassGroup(se1, rd, globals_, 5, False),
            reqs=(se1, rd, globals_),
            invalidates=(se1, *analysis_keys),
        )
        self.register_pass(NumPyFusionPass(numpy, se2), reqs=(numpy, se2), invalidates=(se1, *analysis_keys))
        self.register_pass(GeneratorLoopFusion(), invalidates=(se1, se2, *analysis_keys))
        self.register_pass(NumPyLifetimePass(rd), reqs=(rd,), invalidates=(se1, se2, *analysis_keys))
        self.register_pass(ImperativeForFlowLowering())
        self.register_pass(NumPyBoundsCheckElisionPass(rd), reqs=(rd,), invalidates=(se1, *analysis_keys))

        # Async and parallel lowering.
        self.register_pass(AsyncForLowering())
        self.register_pass(AwaitLowering())
        self.register_pass(OpenMPPass(), invalidates=(cfg, globals_))

        if not self.options.jit:
            # Don't demote globals in JIT mode, since a later input might use them.
            self.register_pass(
                FoldingPassGroup(se2, rd, globals_, 5, True),
                reqs=(se2, rd, globals_),
                invalidates=(se2, rd, cfg, globals_),
            )
        self.register_pass(DevirtualizationPass(rd), reqs=(rd,), invalidates=(cfg, rd, globals_))
