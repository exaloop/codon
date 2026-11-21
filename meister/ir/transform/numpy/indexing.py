# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass
from enum import Enum, auto

from ....ast.types import mangle
from ... import CallInstr, ImperativeForFlow, IntConst, Value, VarValue, cast
from ...analyze.dataflow.cfg import AssignKind, SyntheticAssignInstr
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, is_call_of
from ...util.operator import Operator
from .types import FUSION_MODULE, NumPyPass, array_generics, is_array_type


class Kind(Enum):
    INT = auto()
    VAR = auto()
    LEN = auto()


@dataclass(frozen=True)
class Term:
    kind: Kind
    value: int
    var: VarValue | None = None


def less_check(first, second, strict=False):
    # Compare the sum of the terms in second minus the sum in first.
    coefficients = {}
    for term, factor in (
        *((term, 1) for term in second),
        *((term, -1) for term in first),
    ):
        identifier = term.var.var.id if term.var else None
        key = term.kind, identifier
        coefficients[key] = coefficients.get(key, 0) + factor * term.value
    if any(value for (kind, _), value in coefficients.items() if kind == Kind.VAR):
        return False
    # Array lengths are nonnegative.
    total = coefficients.get((Kind.INT, None), 0)
    return total > 0 if strict else total >= 0


def replace_loop_variable(terms, var, replacement):
    result = []
    for term in terms:
        if term.kind == Kind.VAR and term.var.var.id == var.id:
            result.extend(Term(item.kind, item.value * term.value, item.var) for item in replacement)
        else:
            result.append(term)
    return result


def parse(value, terms, negate=False):
    sign = -1 if negate else 1
    if immediate := cast(IntConst, value):
        terms.append(Term(Kind.INT, immediate.val * sign))
        return True
    if variable := cast(VarValue, value):
        terms.append(Term(Kind.VAR, sign, variable))
        return True
    call_ = cast(CallInstr, value)
    if not call_:
        return False
    integer = value.module.get_int_type()
    for name in ("__add__", "__sub__"):
        if is_call_of(call_, name, [integer, integer], integer, method=True):
            return parse(call_.front(), terms, negate) and parse(
                call_.back(), terms, negate ^ (name == "__sub__")
            )
    if len(call_.args) != 1:
        return False
    arg = cast(VarValue, call_.front())
    if not arg or not is_array_type(arg.type):
        return False
    if is_call_of(call_, "size", [arg.type], integer, method=True) or (
        is_call_of(call_, "len", [arg.type], integer)
        and get_func(call_.callee).name.startswith(mangle("std.internal.builtin", func="len") + "[")
    ):
        terms.append(Term(Kind.LEN, sign, arg))
        return True
    return False


@dataclass
class IndexInfo:
    orig: CallInstr
    arr: VarValue
    idx: Value
    item: Value | None = None


@dataclass(init=False, eq=False)
class FindArrayIndex(Operator):
    indexes: list[IndexInfo]

    def __init__(self):
        super().__init__(children_first=True)
        self.indexes = []

    def handle_CallInstr(self, value):
        arr = cast(VarValue, value.front()) if value.args else None
        if not arr or not is_array_type(arr.type) or array_generics(arr.type)[1] != 1:
            return
        integer = value.module.get_int_type()
        if is_call_of(value, "__getitem__", [arr.type, integer], method=True):
            self.indexes.append(IndexInfo(value, arr, value.back()))
        elif is_call_of(value, "__setitem__", [arr.type, integer, value.back().type], method=True):
            self.indexes.append(IndexInfo(value, arr, value.args[1], value.back()))


def elide_bounds_check(index):
    module = index.orig.module
    clone = CloneVisitor(module)
    name = "_array1d_set_nocheck" if index.item else "_array1d_get_nocheck"
    types = [index.arr.type, module.get_int_type()]
    if index.item:
        types.append(index.item.type)
    fn = module.get_or_realize_func(name, types, [], FUSION_MODULE)
    assert fn, f"{name} function not found"
    args = [module.nr(VarValue, index.arr.var), clone.clone(index.idx)]
    if index.item:
        args.append(clone.clone(index.item))
    index.orig.replace_all(call(fn, args))


def is_original_loop_var(location, loop, reaching):
    # Initial assignment and increment are both represented by CFG synthetic definitions.
    assignments = [
        cast(SyntheticAssignInstr, definition.assignment)
        for definition in reaching.get_reaching_definitions(loop.var, location)
    ]
    return (
        len(assignments) == 2 and all(assignment and assignment.source.id == loop.id for assignment in assignments)
        and {assignment.kind for assignment in assignments} == {AssignKind.KNOWN, AssignKind.ADD}
    )


def alias_of_loop_var(value, loop, reaching):
    definitions = reaching.get_reaching_definitions(value.var, value)
    source = cast(VarValue, definitions[0].assignee) if len(definitions) == 1 else None
    if source and source.var.id == loop.var.id and is_original_loop_var(value, loop, reaching):
        return source
    return None


def can_elide_bounds_check(loop, index, start_terms, stop_terms, reaching):
    index_terms = []
    if not parse(index.idx, index_terms):
        return False

    # Confirm that each read of each variable refers to a consistent definition.
    reach = {}

    def check(value):
        identifier = value.var.id
        if identifier == loop.var.id:
            return is_original_loop_var(value, loop, reaching)
        definitions = reaching.get_reaching_definitions(value.var, value)
        return (
            len(definitions) == 1
            and reach.setdefault(identifier, definitions[0].id) == definitions[0].id
        )

    if not check(index.arr) or not all(
        check(term.var)
        for terms in (start_terms, stop_terms, index_terms)
        for term in terms
        if term.kind != Kind.INT
    ):
        return False

    # Compound assignments may copy the index into a temporary.
    index_terms = [
        Term(term.kind, term.value, alias_of_loop_var(term.var, loop, reaching) or term.var)
        if term.kind == Kind.VAR else term
        for term in index_terms
    ]
    start = replace_loop_variable(index_terms, loop.var, start_terms)
    stop = replace_loop_variable(index_terms, loop.var, stop_terms)
    limit = [Term(Kind.LEN, 1, index.arr)]
    if loop.step > 0:
        return less_check([Term(Kind.INT, 0)], start) and less_check(stop, limit)
    return less_check(start, limit, strict=True) and less_check([Term(Kind.INT, -1)], stop)


@dataclass(init=False, eq=False)
class NumPyBoundsCheckElisionPass(NumPyPass):
    KEY = "core-numpy-bounds-check-elision"

    reaching_def_key: str

    def __init__(self, reaching_def_key):
        super().__init__()
        self.reaching_def_key = reaching_def_key

    def visit(self, node):
        loop = cast(ImperativeForFlow, node)
        if not loop:
            return super().visit(node)
        if not loop.step or loop.var.global_:
            return
        finder = FindArrayIndex()
        finder.process(loop.body)
        start, stop = [], []
        if not finder.indexes or not parse(loop.start, start) or not parse(loop.end, stop):
            return
        result = self.get_analysis_result(self.reaching_def_key)
        reaching = result.results.get(self.get_parent_func().id)
        if reaching:
            for index in finder.indexes:
                if can_elide_bounds_check(loop, index, start, stop, reaching):
                    elide_bounds_check(index)
