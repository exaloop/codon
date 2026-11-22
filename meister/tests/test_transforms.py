"""Ports of the GoogleTest pass scheduling and CIR transformation checks."""

from collections import Counter, defaultdict
import re
import sys
from types import SimpleNamespace

import pytest

from .. import ir
from ..ast.types import Stdlib, mangle
from ..cache import FrontendOptions
from ..ir.analyze.analysis import Analysis
from ..ir.analyze.module.side_effect import SideEffectResult
from ..ir.codegen import emit_module
from ..ir.llvm import Library
from ..ir.transform import PassManager
from ..ir.transform.cleanup.canonical import CanonicalizationPass
from ..ir.transform.lowering.imperative import ImperativeForFlowLowering
from ..ir.transform.lowering.return_ import MissingReturnLowering
from ..ir.transform.pass_ import Pass
from ..ir.transform.pythonic.enumerate import EnumerateOptimization
from ..ir.util.irtools import call, get_func
from ..ir.util.operator import Operator
from ..ir.util.side_effect import SideEffectStatus
from ..test import TestCase as SourceCase
from .cases import ROOT
from .runner import compile_cir, prepare_compiler


@pytest.mark.parametrize("invalidate,skip,trace", [
    (False, False, ["base", "derived", "first", "second"]),
    (True, False, ["base", "derived", "first", "base", "derived", "second"]),
    (True, True, ["base", "derived", "second"]),
], ids=["cached", "invalidated", "skipped"])
def test_pass_manager_dependencies(invalidate, skip, trace):
    events = []

    class ProbeAnalysis(Analysis):
        def __init__(self, key):
            super().__init__()
            self.KEY = key

        def run(self, module):
            events.append(self.KEY)
            return object()

    class ProbePass(Pass):
        def __init__(self, label, enabled=True):
            super().__init__()
            self.KEY = label
            self.enabled = enabled

        def should_run(self, module):
            return self.enabled

        def run(self, module):
            assert self.get_analysis_result("derived") is not None
            events.append(self.KEY)

    manager = PassManager(SimpleNamespace(pmempty=True))
    base = manager.register_analysis(ProbeAnalysis("base"))
    derived = manager.register_analysis(ProbeAnalysis("derived"), reqs=[base])
    if skip:
        manager.register_pass(ProbePass("skipped", False), reqs=[derived], invalidates=[base])
        manager.run(ir.Module(None))
        assert events == []
    manager.register_pass(ProbePass("second"), reqs=[derived])
    manager.register_pass(
        ProbePass("first", not skip), insert_before="second",
        reqs=[derived], invalidates=[base] if invalidate else [],
    )
    manager.run(ir.Module(None))
    assert events == trace


def test_canonicalization_keeps_unchanged_calls():
    class PureAnalysis(Analysis):
        KEY = "pure"

        def run(self, module):
            return SideEffectResult({value.id: SideEffectStatus.PURE for value in module.values})

    module = ir.Module(None)
    integer = module.get_int_type()
    function = module.nr(ir.BodiedFunc, "int.combine")
    function.unmangled_name, function.parent_type = "combine", integer
    signature = module.unsafe_get_func_type("combine", integer, [integer, integer])
    function.realize(signature, ["left", "right"])
    first, second, third = (module.get_int(i) for i in range(1, 4))
    original = call(function, [first, second])
    body = module.main_func.body = module.nr(ir.SeriesFlow)
    body.push_back(original)
    manager = PassManager(SimpleNamespace(pmempty=True))
    pure = manager.register_analysis(PureAnalysis())
    manager.register_pass(CanonicalizationPass(pure), reqs=[pure], invalidates=[pure])

    size = len(module.values)
    manager.run(module)
    manager.run(module)
    assert len(module.values) == size
    assert body.front() is original

    attributes = ir.KeyValueAttribute()
    attributes.attributes[mangle("std.internal.attributes", func="associative")] = ""
    function.set_attribute(attributes)
    body.push_back(call(function, [original, third]))
    size = len(module.values)
    manager.run(module)
    assert len(module.values) == size

    right = call(function, [first, call(function, [second, third])])
    body.push_back(right)
    manager.run(module)
    canonical = ir.cast(ir.CallInstr, right)
    assert canonical is not right
    left = ir.cast(ir.CallInstr, canonical.front())
    assert list(left) == [first, second]
    assert canonical.back() is third

    named = call(function, [first, second])
    named.name = "named-call"
    body.push_back(named)
    manager.run(module)
    assert named.name == ""


@pytest.fixture
def compile_source(codon_library, monkeypatch):
    monkeypatch.setenv("CODON_PATH", str(ROOT / "stdlib"))
    recursion_limit = sys.getrecursionlimit()

    def compile_(fixture, *, debug=False, passes=True, code=None):
        options = FrontendOptions(debug=debug)
        options.pmempty = not passes
        cache, defines = prepare_compiler(codon_library, options)
        path = ROOT / "test/cir/fixtures" / fixture
        return compile_cir(cache, defines, SourceCase(path, code=code))

    try:
        yield compile_
    finally:
        sys.setrecursionlimit(recursion_limit)


class FunctionInstructions(Operator):
    def __init__(self):
        super().__init__()
        self.calls = defaultdict(list)
        self.returns = defaultdict(list)

    def handle_CallInstr(self, node):
        if callee := get_func(node.callee):
            self.calls[self.get_parent_func().unmangled_name].append((callee, node))

    def handle_ReturnInstr(self, node):
        self.returns[self.get_parent_func().unmangled_name].append(node.value)


@pytest.mark.compiler
def test_formatting_optimization(compile_source):
    module = compile_source("format_optimization_test.codon")
    inspection = FunctionInstructions()
    inspection.process(module)
    calls = Counter(
        callee.unmangled_name
        for name, calls in inspection.calls.items() if name.startswith("format_optimization_")
        for callee, _ in calls
    )
    assert calls["_format_parsed"] == 5
    assert calls["format"] + calls["__format__"] == 6


@pytest.mark.compiler
def test_folding_pure_sequences(compile_source):
    module = compile_source("folding_pure_sequence.codon")
    inspection = FunctionInstructions()
    inspection.process(module)
    for name in ("fold_variable", "fold_right_variable", "fold_call", "fold_assignment"):
        returned, = inspection.returns[name]
        constant = ir.cast(ir.IntConst, returned)
        assert constant is not None and constant.val == 3, name
    consume = [
        call.front() for calls in inspection.calls.values()
        for callee, call in calls if callee.unmangled_name == "consume"
    ]
    assert len(consume) == 2
    assert [ir.cast(ir.IntConst, arg).val for arg in consume] == [3, 3]
    assert sum(
        callee.unmangled_name == "record"
        for calls in inspection.calls.values() for callee, _ in calls
    ) == 1


@pytest.mark.compiler
def test_missing_return_only_guards_reachable_fallthrough(compile_source):
    module = compile_source("missing_return.codon", passes=False)
    ImperativeForFlowLowering().run(module)
    expected = Counter({
        "required": 1, "required_union": 1, "caught_error": 1, "finally_breaks": 1,
        "creates_generator": 1, "possibly_empty_loop": 1, "caught_return_expression": 1,
    })
    functions = expected.keys() | {
        "complete", "nullable", "nullable_union", "optional_union", "nothing", "calls_error",
        "finally_returns", "forever", "protected_return", "nonempty_loop", "complete_else",
    }
    lowering = MissingReturnLowering()
    for _ in range(2):
        lowering.run(module)
        inspection = FunctionInstructions()
        inspection.process(module)
        guarded = Counter(
            name for name in functions for callee, _ in inspection.calls[name]
            if callee.unmangled_name == "_missing_return_error"
        )
        assert guarded == expected


@pytest.mark.compiler
def test_missing_return_constructs_null_in_union_layouts(compile_source):
    module = compile_source("union_null.codon", passes=False, code="pass\n")
    integer = module.get_int_type()
    optional = module.get_optional_type(module.get_string_type())
    ref = module.get_or_realize_type(Stdlib.List, [integer])
    assert isinstance(ref, ir.RefType)
    nested = module.get_union_type([integer, module.get_optional_type(ref)])
    layouts = [
        (module.get_union_type([module.get_optional_type(integer), optional]), [0]),
        (module.get_union_type([integer, optional]), [1]),
        (nested, [1]),
        (module.get_union_type([integer, module.get_none_type()]), [1]),
        (module.get_union_type([integer, nested]), [1, 1]),
    ]
    functions = []
    for index, (typ, _) in enumerate(layouts):
        function = module.nr(ir.BodiedFunc, f"null_{index}")
        function.realize(module.unsafe_get_func_type(f"null_{index}.signature", typ, []), [])
        function.body = module.nr(ir.SeriesFlow)
        functions.append(function)

    lowering = MissingReturnLowering()
    lowering.run(module)
    count = len(module.vars)
    lowering.run(module)
    assert len(module.vars) == count
    for function, (typ, tags) in zip(functions, layouts, strict=True):
        returned, = function.body
        value = ir.cast(ir.ReturnInstr, returned).value
        for tag in tags:
            invocation = ir.cast(ir.CallInstr, value)
            assert invocation.type is typ
            actual_tag, value = invocation
            assert ir.cast(ir.IntConst, actual_tag).val == tag
            member = list(typ)[tag]
            assert get_func(invocation.callee) is module.get_or_realize_method(
                typ, "_make", [integer, member], [typ]
            )
            typ = member
        invocation = ir.cast(ir.CallInstr, value)
        assert invocation.type is typ
        assert list(invocation) == []
        assert get_func(invocation.callee) is module.get_or_realize_method(typ, "__new__", [])


@pytest.mark.compiler
def test_checked_optional_types_are_canonical(compile_source):
    module = compile_source("optional_types.codon", passes=False, code="pass\n")
    none = module.get_none_type()
    assert isinstance(none, ir.RecordType)
    assert module.get_optional_type(none) is none
    integer = module.get_int_type()
    optional = module.get_optional_type(integer)
    assert isinstance(optional, ir.OptionalType)
    assert optional.base is integer
    assert module.get_optional_type(optional) is optional


@pytest.mark.compiler
def test_parse_code_definitions_and_import_initialization(compile_source):
    module = compile_source("parse_code_test.codon", passes=False, code="print(42)\n")
    context = module.cache.codegen_ctx
    body = context.main_func.body
    before = list(body)
    series, bases = context.series, context.bases
    integer = module.get_int_type()

    module.parse_code("def __pc_plain(x: int) -> int:\n    return x + 1\n")
    assert module.get_or_realize_func("__pc_plain", [integer])
    assert list(body) == before

    module.parse_code(
        "import bisect\n"
        "def __pc_bisect(x: int) -> int:\n"
        "    return bisect.bisect_left([1, 5, 9], x)\n"
    )
    assert module.get_or_realize_func("__pc_bisect", [integer])
    assert context.series is series
    assert context.bases is bases
    position = before.index(context.main_user_start)
    initialization = body.series[position]
    assert isinstance(initialization, ir.SeriesFlow) and initialization.series
    assert body.series[:position] + body.series[position + 1:] == before


@pytest.mark.compiler
@pytest.mark.parametrize("debug", [True, False], ids=["debug", "release"])
def test_llvm_type_queries_are_constants(compile_source, codon_library, debug):
    module = compile_source("intrinsic_queries.codon", debug=debug, code="""\
class Payload:
    left: int
    right: int

@export
def query_size(): return Payload.__elemsize__

@export
def query_atomic(): return int.__atomic__

@export
def query_contents(): return Payload.__contents_atomic__
""")
    cache = module.cache
    with emit_module(cache, Library(codon_library), "intrinsic_queries.codon",
                     cache.options.get_early_defines()) as emitted:
        text = str(emitted)
        pointer_size = emitted.data_layout.alloc_size(emitted.pointer_type())

    assert not re.search(r"intrinsic\.(?:sizeof|is_atomic|is_content_atomic)", text)
    for name, expected in {
        "query_size": f"i64 {pointer_size}",
        "query_atomic": "i8 1",
        "query_contents": "i8 1",
    }.items():
        function = re.search(rf"^define [^\n]* @{name}\(\).*?^}}", text, re.MULTILINE | re.DOTALL)
        assert function is not None, name
        assert not re.search(r"\b(?:call|invoke)\b", function[0]), function[0]
        assert f"ret {expected}" in function[0], function[0]


class EnumerateInspector(Operator):
    def __init__(self, mark_schedules=False):
        super().__init__()
        self.mark_schedules = mark_schedules
        self.counts = defaultdict(Counter)

    def handle_ForFlow(self, loop):
        name = self.get_parent_func().unmangled_name
        self.counts[name]["generators"] += 1
        if loop.var.global_:
            self.counts[name]["global_targets"] += 1
        if self.mark_schedules and name == "enum_parallel":
            loop.set_parallel()
        if self.mark_schedules and name == "enum_async":
            loop.async_ = True

    def handle_ImperativeForFlow(self, loop):
        self.counts[self.get_parent_func().unmangled_name]["indexed"] += 1

    def handle_CallInstr(self, node):
        if fn := get_func(node.callee):
            counts = self.counts[self.get_parent_func().unmangled_name]
            if fn.name.startswith(mangle("std.internal.builtin", func="enumerate")):
                counts["enumerates"] += 1
            if isinstance(fn, ir.InternalFunc) and fn.intrinsic == ir.Intrinsic.VIRTUAL_LOOKUP:
                counts["virtual_lookups"] += 1


@pytest.mark.compiler
@pytest.mark.numpy
def test_enumerate_optimization(compile_source):
    module = compile_source("enumerate_optimization_test.codon", passes=False)
    before = EnumerateInspector(mark_schedules=True)
    before.process(module)
    for suffix in ("generator", "list", "array", "matrix", "virtual", "default"):
        assert before.counts[f"enum_{suffix}"]["enumerates"] == 1, suffix
    assert before.counts["enum_global"]["global_targets"] == 1

    EnumerateOptimization().run(module)
    ImperativeForFlowLowering().run(module)
    after = EnumerateInspector()
    after.process(module)

    assert after.counts["enum_virtual"]["virtual_lookups"] == 1
    assert after.counts["enum_global"]["global_targets"] == 1
    for cases, expected in [
        (("generator", "virtual", "default", "shadowed"), (0, 1, 0)),
        (("list", "array", "matrix"), (0, 0, 1)),
        (("tuple", "escaping", "parallel", "async", "global"), (1, 1, 0)),
    ]:
        for suffix in cases:
            counts = after.counts[f"enum_{suffix}"]
            actual = counts["enumerates"], counts["generators"], counts["indexed"]
            assert actual == expected, suffix
