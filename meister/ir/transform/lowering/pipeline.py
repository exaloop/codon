# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ....bridge import dataclass
from ... import CallInstr, ForFlow, Var, VarValue
from ...util.irtools import series
from ..pass_ import OperatorPass


# C++: codon/cir/transform/lowering/pipeline.cpp:16
def call_stage(module, stage, last):
    """C++: pipeline.cpp:17. Substitute previous output for null arguments."""
    args = [arg if arg is not None else last for arg in stage.args]
    return module.n(CallInstr, stage.callee.info, stage.callee, args)


# C++: codon/cir/transform/lowering/pipeline.cpp:24
def convert_pipeline_to_for_loops_helper(module, parent, stages, idx=0, last=None):
    """C++: pipeline.cpp:25. Recursively lower generator stages into loops."""
    if idx >= len(stages):
        return last
    stage = stages[idx]
    if idx == 0:
        return convert_pipeline_to_for_loops_helper(module, parent, stages, idx + 1, stage.callee)
    prev = stages[idx - 1]
    if prev.generator:
        var = module.nr(Var, prev.get_output_element_type())
        parent.symbols.append(var)
        body = convert_pipeline_to_for_loops_helper(
            module, parent, stages, idx + 1, call_stage(module, stage, module.nr(VarValue, var))
        )
        loop = module.n(ForFlow, last.info, last, series(body), var)
        if stage.parallel:
            loop.set_parallel()
        return loop
    return convert_pipeline_to_for_loops_helper(
        module, parent, stages, idx + 1, call_stage(module, stage, last)
    )


# C++: codon/cir/transform/lowering/pipeline.cpp:51
def convert_pipeline_to_for_loops(pipeline, parent):
    return convert_pipeline_to_for_loops_helper(pipeline.module, parent, list(pipeline.stages))


@dataclass(init=False, eq=False)
class PipelineLowering(OperatorPass):
    """C++: codon/cir/transform/lowering/pipeline.h:13. Core pipeline lowering."""

    KEY = "core-pipeline-lowering"

    # C++: codon/cir/transform/lowering/pipeline.cpp:62
    def handle_PipelineFlow(self, value):
        value.replace_all(convert_pipeline_to_for_loops(value, self.get_parent_func()))
