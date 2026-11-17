# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from collections import deque
from enum import IntEnum

from ....bridge import Dict, Set, dataclass
from ... import (
    AwaitInstr,
    BodiedFunc,
    BoolConst,
    BreakInstr,
    CallInstr,
    ContinueInstr,
    Flow,
    ForFlow,
    FuncType,
    GeneratorType,
    IfFlow,
    ImperativeForFlow,
    IntConst,
    OptionalType,
    ReturnInstr,
    SeriesFlow,
    TernaryInstr,
    ThrowInstr,
    TryCatchFlow,
    UnionType,
    WhileFlow,
    YieldInInstr,
    YieldInstr,
    cast,
)
from ...dsl import CustomInstr
from ...util.irtools import call, get_func
from ...util.operator import Operator
from ..pass_ import Pass


class ExitKind(IntEnum):
    """C++: codon/cir/transform/lowering/return.cpp:19. Outcomes of a control-flow path."""

    Normal = 0
    Return = 1
    Throw = 2
    Break = 3
    Continue = 4


NO_TARGET = -1


# C++: codon/cir/transform/lowering/return.cpp:24
def exit_(kind):
    return {(kind, NO_TARGET)}


# C++: codon/cir/transform/lowering/return.cpp:26
def has(exits, kind):
    return (kind, NO_TARGET) in exits


# C++: codon/cir/transform/lowering/return.cpp:28
def is_nullable(typ, none_type):
    if typ.is_type(none_type) or cast(OptionalType, typ):
        return True
    union = cast(UnionType, typ)
    return union is not None and any(is_nullable(member, none_type) for member in union)


# C++: codon/cir/transform/lowering/return.cpp:38
def make_none(module, typ):
    if union := cast(UnionType, typ):
        idx = next(
            (
                idx
                for idx, candidate in enumerate(union.types)
                if is_nullable(candidate, module.get_none_type())
            ),
            None,
        )
        assert idx is not None, "union has no nullable member"
        member = union.types[idx]
        value = make_none(module, member)
        constructor = module.get_or_realize_method(
            typ, "_make", [module.get_int_type(), member], [typ]
        )
        assert constructor, "union constructor not found"
        return call(constructor, [module.get_int(idx), value])
    constructor = module.get_or_realize_method(typ, "__new__", [])
    assert constructor, f"null constructor not found for {typ.name}"
    return call(constructor, [])


# C++: codon/cir/transform/lowering/return.cpp:62
def sequence(first, second):
    first = set(first)
    if (ExitKind.Normal, NO_TARGET) in first:
        first.remove((ExitKind.Normal, NO_TARGET))
        first.update(second)
    return first


@dataclass(init=False, eq=False)
class ReturnAnalysis:
    """C++: codon/cir/transform/lowering/return.cpp:68. Analyze normal and abrupt exits, including finally."""

    non_returning: Set[BodiedFunc]

    # C++: codon/cir/transform/lowering/return.cpp:102
    def __init__(self, non_returning):
        self.non_returning = non_returning

    # C++: codon/cir/transform/lowering/return.cpp:71
    def branch(self, condition, true, false):
        result = self.analyze(condition)
        if not has(result, ExitKind.Normal):
            return result
        alternatives = set()
        constant = cast(BoolConst, condition)
        if constant is None or constant.val:
            alternatives.update(self.analyze(true))
        if constant is None or not constant.val:
            alternatives.update(self.analyze(false))
        return sequence(result, alternatives)

    # C++: codon/cir/transform/lowering/return.cpp:84
    def loop(self, flow, body, may_be_empty):
        result = exit_(ExitKind.Normal) if may_be_empty else set()
        for kind, target in body:
            local = target == NO_TARGET or target == flow.id
            if kind == ExitKind.Normal or (kind == ExitKind.Continue and local):
                continue
            result.add(
                (ExitKind.Normal, NO_TARGET) if kind == ExitKind.Break and local else (kind, target)
            )
        return result

    # C++: codon/cir/transform/lowering/return.cpp:72
    def analyze(self, value):
        if value is None:
            return exit_(ExitKind.Normal)
        value = value.get_actual()
        match value:
            case SeriesFlow():
                result = exit_(ExitKind.Normal)
                for child in value:
                    if not has(result, ExitKind.Normal):
                        break
                    result = sequence(result, self.analyze(child))
                return result
            case IfFlow():
                return self.branch(value.cond, value.true_branch, value.false_branch)
            case TernaryInstr():
                return self.branch(value.cond, value.true_value, value.false_value)
            case WhileFlow():
                result = self.analyze(value.cond)
                constant = cast(BoolConst, value.cond)
                if constant is not None and not constant.val:
                    return result
                return sequence(
                    result, self.loop(value, self.analyze(value.body), constant is None)
                )
            case ForFlow():
                result = self.analyze(value.iter)
                outcomes = self.loop(value, self.analyze(value.body), True)
                outcomes.add((ExitKind.Throw, NO_TARGET))
                return sequence(result, outcomes)
            case ImperativeForFlow():
                result = sequence(self.analyze(value.start), self.analyze(value.end))
                start, end = cast(IntConst, value.start), cast(IntConst, value.end)
                nonempty = (
                    start is not None
                    and end is not None
                    and (
                        (value.step > 0 and start.val < end.val)
                        or (value.step < 0 and start.val > end.val)
                    )
                )
                body = self.analyze(value.body)
                outcomes = self.loop(value, body, not nonempty)
                if value.is_parallel():
                    outcomes.add((ExitKind.Throw, NO_TARGET))
                if (
                    has(body, ExitKind.Normal)
                    or has(body, ExitKind.Continue)
                    or (ExitKind.Continue, value.id) in body
                ):
                    outcomes.add((ExitKind.Normal, NO_TARGET))
                return sequence(result, outcomes)
            case TryCatchFlow():
                body = self.analyze(value.body)
                result = sequence(body, self.analyze(value.else_))
                if has(body, ExitKind.Throw):
                    for handler in value:
                        result.update(self.analyze(handler.handler))
                if value.finally_ is not None and result:
                    cleanup = self.analyze(value.finally_)
                    if (ExitKind.Normal, NO_TARGET) not in cleanup:
                        result.clear()
                    cleanup.discard((ExitKind.Normal, NO_TARGET))
                    result.update(cleanup)
                return result
            case ReturnInstr():
                return sequence(self.analyze(value.value), exit_(ExitKind.Return))
            case ThrowInstr():
                return sequence(self.analyze(value.value), exit_(ExitKind.Throw))
            case BreakInstr():
                return {(ExitKind.Break, value.loop.id if value.loop else NO_TARGET)}
            case ContinueInstr():
                return {(ExitKind.Continue, value.loop.id if value.loop else NO_TARGET)}
            case YieldInstr():
                outcomes = exit_(ExitKind.Return if value.final else ExitKind.Normal)
                outcomes.add((ExitKind.Throw, NO_TARGET))
                return sequence(self.analyze(value.value), outcomes)
            case Flow() | CustomInstr():
                return {(kind, NO_TARGET) for kind in ExitKind}
        result = exit_(ExitKind.Normal)
        for operand in value.get_used_values():
            if not has(result, ExitKind.Normal):
                break
            result = sequence(result, self.analyze(operand))
        if call_ := cast(CallInstr, value):
            callee = cast(BodiedFunc, get_func(call_.callee))
            outcomes = exit_(ExitKind.Throw)
            if not callee or callee not in self.non_returning:
                outcomes.add((ExitKind.Normal, NO_TARGET))
            result = sequence(result, outcomes)
        elif isinstance(value, (AwaitInstr, YieldInInstr)):
            result = sequence(result, {(ExitKind.Normal, NO_TARGET), (ExitKind.Throw, NO_TARGET)})
        return result


@dataclass(init=False, eq=False)
class Calls(Operator):
    """C++: codon/cir/transform/lowering/return.cpp:222. Build direct-call reverse dependencies."""

    callers: Dict[BodiedFunc, Set[BodiedFunc | None]]

    # C++ API: codon/cir/transform/lowering/return.cpp:222
    def __init__(self):
        super().__init__()
        self.callers = {}

    # C++: codon/cir/transform/lowering/return.cpp:224
    def handle_CallInstr(self, value):
        if callee := cast(BodiedFunc, get_func(value.callee)):
            self.callers.setdefault(callee, set()).add(cast(BodiedFunc, self.get_parent_func()))


@dataclass(init=False, eq=False)
class MissingReturnLowering(Pass):
    """C++: codon/cir/transform/lowering/return.h:12. Insert nullable returns or missing-return errors."""

    KEY = "core-missing-return-lowering"

    # C++: codon/cir/transform/lowering/return.cpp:214
    def run(self, module):
        functions = [module.main_func] if cast(BodiedFunc, module.main_func) else []
        functions.extend(fn for var in module if (fn := cast(BodiedFunc, var)) is not None)
        calls = Calls()
        calls.process(module)
        non_returning = set()
        analysis = ReturnAnalysis(non_returning)
        pending, queued = deque(functions), set(functions)
        while pending:
            fn = pending.popleft()
            queued.remove(fn)
            if fn.generator or fn.async_ or fn.body is None or fn in non_returning:
                continue
            outcomes = analysis.analyze(fn.body)
            if has(outcomes, ExitKind.Normal) or has(outcomes, ExitKind.Return):
                continue
            non_returning.add(fn)
            for caller in calls.callers.get(fn, ()):
                if caller and caller not in queued:
                    queued.add(caller)
                    pending.append(caller)
        helper, changed = None, False
        for fn in functions:
            if fn.generator or fn.body is None:
                continue
            result = cast(FuncType, fn.type).return_type
            if fn.async_:
                result = cast(GeneratorType, result).base
            if (
                result.is_type(module.get_none_type())
                or cast(OptionalType, result)
                or not has(analysis.analyze(fn.body), ExitKind.Normal)
            ):
                continue
            info = fn.info
            if is_nullable(result, module.get_none_type()):
                ending = module.n(ReturnInstr, info, make_none(module, result))
            else:
                if helper is None:
                    helper = module.get_or_realize_func(
                        "_missing_return_error",
                        [
                            module.get_string_type(),
                            module.get_string_type(),
                            module.get_string_type(),
                            module.get_int_type(),
                            module.get_int_type(),
                        ],
                        [],
                        "std.internal.types.error",
                    )
                assert helper, "missing-return helper not found"
                name = fn.unmangled_name
                message = f"function '{name}' reached its end without returning a value ({info.file}:{info.line}:{info.col})"
                exception = call(
                    helper,
                    [
                        module.get_string(message),
                        module.get_string(name),
                        module.get_string(info.file),
                        module.get_int(info.line),
                        module.get_int(info.col),
                    ],
                )
                ending = module.n(ThrowInstr, info, exception)
            body = cast(SeriesFlow, fn.body)
            if body is None:
                body = module.n(SeriesFlow, info)
                body.push_back(fn.body)
                fn.body = body
            body.push_back(ending)
            changed = True
        if changed:
            self.run(module)
