"""Ports of the native dominator and reaching-definition checks."""

import pytest

from .. import ir
from ..ir.analyze.dataflow.cfg import build_cf_graph
from ..ir.analyze.dataflow.dominator import DominatorInspector
from ..ir.analyze.dataflow.reaching import RDInspector


@pytest.fixture
def program():
    module = ir.Module(None)
    function = module.nr(ir.BodiedFunc, "f")
    function.body = module.nr(ir.SeriesFlow)
    variable = module.nr(ir.Var, module.get_int_type())
    function.push_back(variable)
    return module, function, variable


def series(module, *values):
    body = module.nr(ir.SeriesFlow)
    for value in values:
        body.push_back(value)
    return body


@pytest.mark.parametrize("branching", [False, True], ids=["straight-line", "ternary"])
def test_dominance(program, branching):
    module, function, _ = program
    start, middle, end = (module.get_bool(False) for _ in range(3))
    if branching:
        end = module.nr(ir.TernaryInstr, module.get_bool(True), middle, end)
    function.body = series(module, start, end)
    inspector = DominatorInspector(build_cf_graph(function))
    inspector.analyze()
    assert inspector.is_dominated(end, start)
    assert not inspector.is_dominated(start, end)
    if branching:
        assert inspector.is_dominated(middle, start)
        assert not inspector.is_dominated(end, middle)


def definitions(inspector, variable, location):
    return {
        definition.assignee for definition in inspector.get_reaching_definitions(variable, location)
        if definition.known()
    }


@pytest.mark.parametrize("control", ["straight-line", "if", "while", "try"])
def test_reaching_definitions(program, control):
    module, function, variable = program
    start, middle, end = (module.get_bool(False) for _ in range(3))
    first, second, third, initial = (module.get_int(value) for value in (1, 2, 3, 0))
    assign_first = module.nr(ir.AssignInstr, variable, first)
    assign_second = module.nr(ir.AssignInstr, variable, second)
    condition = module.get_bool(False)
    match control:
        case "straight-line":
            function.body = series(module, start, assign_first, assign_second, end)
            expected = {second}
        case "if":
            branch = module.nr(ir.IfFlow, condition, series(module, assign_first),
                               series(module, assign_second))
            function.body = series(module, start, branch, end)
            expected = {first, second}
        case "while":
            loop = module.nr(ir.WhileFlow, condition, series(module, assign_second))
            function.body = series(module, start, assign_first, loop, end)
            expected = {first, second}
        case "try":
            handling = module.nr(ir.TryCatchFlow, series(module, assign_first),
                                 series(module, middle, assign_second))
            handling.catches.append(ir.Catch(
                series(module, module.nr(ir.AssignInstr, variable, third))
            ))
            function.body = series(
                module, start, module.nr(ir.AssignInstr, variable, initial), handling, end
            )
            expected = {second}

    inspector = RDInspector(build_cf_graph(function))
    inspector.analyze()
    assert definitions(inspector, variable, start) == set()
    assert definitions(inspector, variable, end) == expected
    if control == "straight-line":
        assert definitions(inspector, variable, first) == set()
        assert definitions(inspector, variable, assign_first) == {first}
        assert definitions(inspector, variable, second) == {first}
        assert definitions(inspector, variable, assign_second) == {second}
    if control == "try":
        assert definitions(inspector, variable, middle) == {initial, first, third}


def test_reaching_definitions_indexed_locations(program):
    module, function, variable = program
    first, second, replacement = (module.get_int(value) for value in (1, 2, 3))
    shared, missing = module.get_bool(False), module.get_bool(True)
    assign_first = module.nr(ir.AssignInstr, variable, first)
    assign_second = module.nr(ir.AssignInstr, variable, second)
    function.body = series(module, shared, assign_first, shared, assign_second)
    graph = build_cf_graph(function)
    inspector = RDInspector(graph)
    inspector.analyze()

    assert definitions(inspector, variable, shared) == set()
    assert definitions(inspector, variable, assign_first) == {first}
    assert definitions(inspector, variable, assign_second) == {second}
    assert definitions(inspector, variable, missing) == set()
    graph.remap_value(missing, assign_first)
    assert definitions(inspector, variable, missing) == {second}
    assign_second.rhs = replacement
    assert definitions(inspector, variable, assign_second) == {replacement}
