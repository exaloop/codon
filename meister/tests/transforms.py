"""Test-only passes from the native file fixture runner."""

from ..ir import BodiedFunc, CallInstr, Func, cast
from ..ir.analyze.dataflow.capture import CaptureAnalysis
from ..ir.analyze.dataflow.dominator import DominatorAnalysis
from ..ir.analyze.dataflow.reaching import RDAnalysis
from ..ir.transform.pass_ import OperatorPass, Pass
from ..ir.util.inlining import inline_call
from ..ir.util.irtools import call, get_func, is_call_of
from ..ir.util.operator import Operator
from ..ir.util.outlining import outline_region


class FixtureOutliner(OperatorPass):
    KEY = "test-outliner-pass"

    def __init__(self):
        super().__init__()
        self.successes = self.failures = 0
        self.successes_return = self.failures_return = None

    def handle_SeriesFlow(self, flow):
        begin = next(
            (i for i, statement in enumerate(flow) if is_call_of(statement, "__outline_begin__")),
            None,
        )
        end = next(
            (i for i, statement in enumerate(flow) if is_call_of(statement, "__outline_end__")),
            None,
        )
        if begin is None or end is None:
            return
        result = outline_region(self.get_parent_func(), flow, begin, end)
        if result:
            self.successes += 1
        else:
            self.failures += 1
        if self.successes_return:
            self.successes_return.value = flow.module.get_int(self.successes)
        if self.failures_return:
            self.failures_return.value = flow.module.get_int(self.failures)

    def handle_ReturnInstr(self, statement):
        name = self.get_parent_func().unmangled_name
        if name == "__outline_successes__":
            statement.value = statement.module.get_int(self.successes)
            self.successes_return = statement
        if name == "__outline_failures__":
            statement.value = statement.module.get_int(self.failures)
            self.failures_return = statement


class FixtureInliner(OperatorPass):
    KEY = "test-inliner-pass"

    def handle_CallInstr(self, statement):
        target = cast(BodiedFunc, get_func(statement.callee))
        module = statement.module
        integer = module.get_int_type()
        negation = module.get_or_realize_method(integer, "__neg__", [integer])
        if not target or "inline_me" not in target.unmangled_name:
            return
        result = inline_call(statement, aggressive="aggressive" in target.unmangled_name)
        if result:
            self.get_parent_func().symbols.extend(result.new_vars)
            value = result.result
            statement.replace_all(call(negation, [value]) if value.type.is_type(integer) else value)


class PartitionArgsByEscape(Operator):
    def __init__(self):
        super().__init__()
        self.calls = []

    def handle_CallInstr(self, statement):
        fn = get_func(statement.callee)
        if fn and fn.unmangled_name == "expect_capture":
            self.calls.append(statement)


class EscapeValidator(Pass):
    KEY = "test-escape-validator-pass"

    def __init__(self, key):
        super().__init__()
        self.key = key

    def run(self, module):
        captures = self.get_analysis_result(self.key)
        for symbol in module:
            if not cast(Func, symbol):
                continue
            partition = PartitionArgsByEscape()
            partition.process(symbol)
            if not partition.calls:
                continue
            got = captures.results[symbol.id]
            assert len(got) == len(partition.calls), symbol.name
            for statement, info in zip(partition.calls, got):
                returned, external, arguments = (arg.get_actual() for arg in statement)
                expected = (
                    returned.val, external.val, sorted(arg.get_actual().val for arg in arguments)
                )
                actual = (info.return_captures, info.extern_captures, sorted(info.arg_captures))
                if expected != actual:
                    print(f"{symbol.name}: expected {expected}, got {actual}", flush=True)
                statement.replace_all(module.get_bool(expected == actual))


def register_test_passes(manager):
    manager.register_pass(FixtureOutliner())
    manager.register_pass(FixtureInliner())
    reaching, dominating = RDAnalysis.KEY, DominatorAnalysis.KEY
    capture = manager.register_analysis(
        CaptureAnalysis(reaching, dominating), (reaching, dominating)
    )
    manager.register_pass(EscapeValidator(capture), reqs=(capture,))

