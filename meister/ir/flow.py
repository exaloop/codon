# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import List, dataclass
from .base import cast, replace_in_list
from .type import FuncType, GeneratorType
from .value import Value


@dataclass(init=False, eq=False)
class Flow(Value):
    """C++: codon/cir/flow.h:17. Base for flows representing control flow."""

    # C++: codon/cir/flow.cpp:27
    def _get_type(self):
        return self.module.get_none_type()


@dataclass(init=False, eq=False)
class SeriesFlow(Flow):
    """C++: codon/cir/flow.h:28. A series of flows or instructions."""

    series: List[Value]

    # C++ API: codon/cir/flow.h:28
    def __init__(self, name=""):
        super().__init__(name)
        self.series = []

    # C++: codon/cir/flow.h:38
    def __iter__(self):
        """
        @return an iterator to the first instruction/flow
        """
        return iter(self.series)

    # C++: codon/cir/flow.h:47
    def front(self):
        """
        @return a pointer to the first instruction/flow
        """
        return self.series[0]

    # C++: codon/cir/flow.h:49
    def back(self):
        """
        @return a pointer to the last instruction/flow
        """
        return self.series[-1]

    # C++: codon/cir/flow.h:59
    def insert(self, position, value):
        """
        Inserts an instruction/flow at the given position.
        @param pos the position
        @param v the flow or instruction
        @return an iterator to the newly added instruction/flow
        """
        self.series.insert(position, value)
        return position

    # C++ API: codon/cir/flow.h:28
    def push_back(self, value):
        self.series.append(value)

    # C++: codon/cir/flow.h:67
    def erase(self, position):
        """
        Erases the item at the supplied position.
        @param pos the position
        @return the iterator beyond the removed flow or instruction
        """
        del self.series[position]
        return position

    # C++: codon/cir/flow.h:70
    def _get_used_values(self):
        return list(self.series)

    # C++: codon/cir/flow.cpp:31
    def _replace_used_value(self, identifier, new):
        return replace_in_list(self.series, identifier, new)


@dataclass(init=False, eq=False)
class WhileFlow(Flow):
    """C++: codon/cir/flow.h:77, flow.cpp:37. While loop: condition and body."""

    cond: Value
    body: Flow

    # C++: codon/cir/flow.h:91
    def __init__(self, cond, body, name=""):
        """
        Constructs a while loop.
        @param cond the condition
        @param body the body
        @param name the flow's name
        """
        super().__init__(name)
        self.cond, self.body = cond, body

    # C++: codon/cir/flow.h:111
    def _get_used_values(self):
        return [self.cond, self.body]

    # C++: codon/cir/flow.cpp:37
    def _replace_used_value(self, identifier, new):
        count = 0
        if self.cond.id == identifier:
            self.cond = new
            count += 1
        if self.body.id == identifier:
            body = cast(Flow, new)
            assert body is not None, "not a flow"
            self.body = body
            count += 1
        return count


@dataclass(init=False, eq=False)
class ForFlow(Flow):
    """C++: codon/cir/flow.h:117, flow.cpp:56. Iterator-based for loop."""

    iter: Value
    body: Flow
    var: object
    # Parallel loop schedule, or null if none; async loop flag.
    schedule: object | None
    async_: bool

    # C++: codon/cir/flow.h:140
    def __init__(self, iterator, body, var, schedule=None, async_=False, name=""):
        """
        Constructs a for loop.
        @param iter the iterator
        @param body the body
        @param var the variable
        @param schedule the parallel schedule
        @param async true if loop is async
        @param name the flow's name
        """
        super().__init__(name)
        self.iter, self.body, self.var = iterator, body, var
        self.schedule, self.async_ = schedule, async_

    # C++: codon/cir/flow.h:171
    def is_parallel(self):
        """
        @return true if parallel
        """
        return self.schedule is not None

    # C++: codon/cir/flow.h:174
    def set_parallel(self, value=True):
        """
        Sets parallel status.
        @param a true if parallel
        """
        from .transform.parallel.schedule import OMPSched

        self.schedule = OMPSched() if value else None

    # C++: codon/cir/flow.cpp:55
    def _get_used_values(self):
        return [*(self.schedule.get_used_values() if self.schedule else []), self.iter, self.body]

    # C++: codon/cir/flow.cpp:64
    def _replace_used_value(self, identifier, new):
        count = self.schedule.replace_used_value(identifier, new) if self.schedule else 0
        if self.iter.id == identifier:
            self.iter = new
            count += 1
        if self.body.id == identifier:
            body = cast(Flow, new)
            assert body is not None, "not a flow"
            self.body = body
            count += 1
        return count

    # C++: codon/cir/flow.h:201
    def _get_used_variables(self):
        return [self.var]

    # C++: codon/cir/flow.cpp:81
    def _replace_used_variable(self, identifier, new):
        if self.var.id == identifier:
            self.var = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class ImperativeForFlow(Flow):
    """C++: codon/cir/flow.h:206, flow.cpp:95. Integer loop with constant step."""

    start: Value
    step: int
    end: Value
    body: Flow
    var: object
    schedule: object | None

    # C++: codon/cir/flow.h:231
    def __init__(self, start, step, end, body, var, schedule=None, name=""):
        """
        Constructs an imperative for loop.
        @param body the body
        @param start the start value
        @param step the step value
        @param end the end value
        @param var the end variable, must be integer
        @param name the flow's name
        """
        super().__init__(name)
        self.start, self.step, self.end, self.body, self.var = start, step, end, body, var
        self.schedule = schedule

    # C++: codon/cir/flow.h:272
    def is_parallel(self):
        """
        @return true if parallel
        """
        return self.schedule is not None

    # C++: codon/cir/flow.h:275
    def set_parallel(self, value=True):
        """
        Sets parallel status.
        @param a true if parallel
        """
        from .transform.parallel.schedule import OMPSched

        self.schedule = OMPSched() if value else None

    # C++: codon/cir/flow.cpp:91
    def _get_used_values(self):
        return [
            *(self.schedule.get_used_values() if self.schedule else []),
            self.start,
            self.end,
            self.body,
        ]

    # C++: codon/cir/flow.cpp:101
    def _replace_used_value(self, identifier, new):
        count = self.schedule.replace_used_value(identifier, new) if self.schedule else 0
        if self.body.id == identifier:
            body = cast(Flow, new)
            assert body is not None, "not a flow"
            self.body = body
            count += 1
        for field in ("start", "end"):
            if getattr(self, field).id == identifier:
                setattr(self, field, new)
                count += 1
        return count

    # C++: codon/cir/flow.h:296
    def _get_used_variables(self):
        return [self.var]

    # C++: codon/cir/flow.cpp:122
    def _replace_used_variable(self, identifier, new):
        if self.var.id == identifier:
            self.var = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class IfFlow(Flow):
    """C++: codon/cir/flow.h:301, flow.cpp:140. If statement and optional false branch."""

    cond: Value
    true_branch: Flow
    false_branch: Flow | None

    # C++: codon/cir/flow.h:318
    def __init__(self, cond, true_branch, false_branch=None, name=""):
        """
        Constructs an if.
        @param cond the condition
        @param trueBranch the true branch
        @param falseBranch the false branch
        @param name the flow's name
        """
        super().__init__(name)
        self.cond, self.true_branch, self.false_branch = cond, true_branch, false_branch

    # C++: codon/cir/flow.cpp:132
    def _get_used_values(self):
        return [self.cond, self.true_branch, *([self.false_branch] if self.false_branch else [])]

    # C++: codon/cir/flow.cpp:139
    def _replace_used_value(self, identifier, new):
        count = 0
        if self.cond.id == identifier:
            self.cond = new
            count += 1
        for field in ("true_branch", "false_branch"):
            value = getattr(self, field)
            if value is not None and value.id == identifier:
                flow = cast(Flow, new)
                assert flow is not None, "not a flow"
                setattr(self, field, flow)
                count += 1
        return count


@dataclass(init=False, eq=False)
class Catch:
    """C++: codon/cir/flow.h:356. Catch handler, type and variable (both nullable)."""

    handler: Flow
    type: object | None
    var: object | None

    # C++: codon/cir/flow.h:366
    def __init__(self, handler, typ=None, var=None):
        self.handler, self.type, self.var = handler, typ, var


@dataclass(init=False, eq=False)
class TryCatchFlow(Flow):
    """C++: codon/cir/flow.h:353, flow.cpp:179. Try/catch with else and finally."""

    Catch = Catch
    catches: List[Catch]
    body: Flow
    else_: Flow | None
    finally_: Flow | None

    # C++: codon/cir/flow.h:410
    def __init__(self, body, finally_=None, else_=None, name=""):
        """
        Constructs an try-catch.
        @param name the's name
        @param body the body
        @param finally the finally
        """
        super().__init__(name)
        self.body, self.else_, self.finally_ = body, else_, finally_
        self.catches = []

    # C++: codon/cir/flow.h:439
    def __iter__(self):
        """
        @return an iterator to the first catch
        """
        return iter(self.catches)

    # C++: codon/cir/flow.cpp:164
    def _get_used_values(self):
        return [
            self.body,
            *([self.else_] if self.else_ else []),
            *([self.finally_] if self.finally_ else []),
            *(catch.handler for catch in self.catches),
        ]

    # C++: codon/cir/flow.cpp:176
    def _replace_used_value(self, identifier, new):
        count = 0
        for field in ("body", "else_", "finally_"):
            value = getattr(self, field)
            if value is not None and value.id == identifier:
                flow = cast(Flow, new)
                assert flow is not None, "not a flow"
                setattr(self, field, flow)
                count += 1
        for catch in self.catches:
            if catch.handler.id == identifier:
                flow = cast(Flow, new)
                assert flow is not None, "not a flow"
                catch.handler = flow
                count += 1
        return count

    # C++: codon/cir/flow.cpp:210
    def _get_used_types(self):
        return [catch.type for catch in self.catches if catch.type is not None]

    # C++: codon/cir/flow.cpp:219
    def _replace_used_type(self, name, new):
        count = 0
        for catch in self.catches:
            if catch.type.name == name:
                catch.type = new
                count += 1
        return count

    # C++: codon/cir/flow.cpp:230
    def _get_used_variables(self):
        return [catch.var for catch in self.catches if catch.var is not None]

    # C++: codon/cir/flow.cpp:239
    def _replace_used_variable(self, identifier, new):
        count = 0
        for catch in self.catches:
            if catch.var.id == identifier:
                catch.var = new
                count += 1
        return count


@dataclass(init=False, eq=False)
class Stage:
    """C++: codon/cir/flow.h:497, flow.cpp:261. A single pipeline stage."""

    callee: Value
    # Null argument denotes where previous pipeline output should go.
    args: List[Value | None]
    generator: bool
    parallel: bool

    # C++: codon/cir/flow.h:515
    def __init__(self, callee, args, generator, parallel):
        """
        Constructs a pipeline stage.
        @param callee the function being called
        @param args call arguments, with exactly one null entry
        @param generator whether this stage is a generator stage
        @param parallel whether this stage is parallel
        """
        self.callee, self.args, self.generator, self.parallel = (
            callee,
            list(args),
            generator,
            parallel,
        )

    # C++: codon/cir/flow.cpp:252
    def get_output_type(self):
        if not self.args:
            return self.callee.type
        typ = cast(FuncType, self.callee.type)
        assert typ is not None, "not a function type"
        return typ.return_type

    # C++: codon/cir/flow.cpp:262
    def get_output_element_type(self):
        if self.generator:
            if not self.args:
                return cast(GeneratorType, self.callee.type).base
            typ = cast(FuncType, self.callee.type)
            assert typ is not None, "not a function type"
            generator = cast(GeneratorType, typ.return_type)
            assert generator is not None, "generator type not found"
            return generator.base
        return self.get_output_type()


@dataclass(init=False, eq=False)
class PipelineFlow(Flow):
    """C++: codon/cir/flow.h:494. Pipelines with only function stages are expressions;
    generator stages are separately flagged, including generators not iterated.
    """

    Stage = Stage
    stages: List[Stage]

    # C++: codon/cir/flow.h:587
    def __init__(self, stages=None, name=""):
        """
        Constructs a pipeline flow.
        @param stages vector of pipeline stages
        @param name the name
        """
        super().__init__(name)
        self.stages = [] if stages is None else list(stages)

    # C++: codon/cir/flow.h:520
    def __iter__(self):
        """
        @return an iterator to the first argument
        """
        return iter(self.stages)

    # C++: codon/cir/flow.cpp:284
    def _get_used_values(self):
        result = []
        for stage in self.stages:
            result.append(stage.callee)
            result.extend(arg for arg in stage.args if arg is not None)
        return result

    # C++: codon/cir/flow.cpp:295
    def _replace_used_value(self, identifier, new):
        count = 0
        for stage in self.stages:
            if stage.callee.id == identifier:
                stage.callee = new
                count += 1
            for idx, arg in enumerate(stage.args):
                if arg is not None and arg.id == identifier:
                    stage.args[idx] = new
                    count += 1
        return count
