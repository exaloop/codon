# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from enum import IntEnum

from ...bridge import List, Set, Tuple, dataclass
from .. import (
    AssignInstr,
    BodiedFunc,
    ForFlow,
    Func,
    IfFlow,
    ImperativeForFlow,
    PointerValue,
    ReturnInstr,
    SeriesFlow,
    Var,
    VarValue,
    WhileFlow,
    is_a,
)
from .cloning import CloneVisitor
from .irtools import call, get_derived_func_name, ptr_load, ptr_store, series
from .operator import Operator


@dataclass(init=False, eq=False)
class OutlineResult:
    """C++: codon/cir/util/outlining.h:12. Result of an outlining operation."""

    class ArgKind(IntEnum):
        CONSTANT = 0  # Argument is not modified; passed by value.
        MODIFIED = 1  # Argument is modified; passed by pointer.

    func: BodiedFunc | None
    call: object | None
    arg_kinds: List[ArgKind]
    # Externally handled control flows return an integer code checked at the
    # call site. Zero means void/no generated control-flow checks.
    num_out_flows: int

    # C++ API: codon/cir/util/outlining.h:12
    def __init__(self, fn=None, call_=None, arg_kinds=None, num_out_flows=0):
        self.func, self.call = fn, call_
        self.arg_kinds = [] if arg_kinds is None else list(arg_kinds)
        self.num_out_flows = num_out_flows

    # C++: codon/cir/util/outlining.h:43
    def __bool__(self):
        return self.func is not None


@dataclass(init=False, eq=False)
class OutlineReplacer(Operator):
    """C++: codon/cir/util/outlining.cpp:17. Remap variables and external control flows."""

    mod_vars: Set[int]
    remap: List[Tuple[Var, Var]]
    out_flows: List[object]
    cv: CloneVisitor

    # C++: codon/cir/util/outlining.cpp:23
    def __init__(self, module, mod_vars, remap, out_flows):
        super().__init__()
        self.mod_vars, self.remap, self.out_flows = mod_vars, remap, out_flows
        self.cv = CloneVisitor(module, False)

    # C++: codon/cir/util/outlining.cpp:29
    def post_hook(self, node):
        # Replace all used vars based on remapping.
        for old, new in self.remap:
            node.replace_used_variable(old, new)

    # C++: codon/cir/util/outlining.cpp:35
    def mapped_var(self, var):
        return next((new for old, new in self.remap if old.id == var.id), None)

    # C++: codon/cir/util/outlining.cpp:46
    def replace_out_flow_with_return(self, value):
        # Returns and breaks/continues of non-outlined loops become status codes
        # telling the original call site which action to perform.
        for idx, original in enumerate(self.out_flows):
            if original.id == value.id:
                cloned = self.cv.clone(value)
                value.replace_all(value.module.nr(ReturnInstr, value.module.get_int(idx + 1)))
                self.out_flows[idx] = cloned
                break

    # C++: codon/cir/util/outlining.cpp:58
    def handle_ReturnInstr(self, value):
        self.replace_out_flow_with_return(value)

    # C++: codon/cir/util/outlining.cpp:60
    def handle_BreakInstr(self, value):
        self.replace_out_flow_with_return(value)

    # C++: codon/cir/util/outlining.cpp:62
    def handle_ContinueInstr(self, value):
        self.replace_out_flow_with_return(value)

    # C++: codon/cir/util/outlining.cpp:66
    def handle_VarValue(self, value):
        # A pointer-passed (modified) variable reference becomes a dereference.
        if value.var.id in self.mod_vars:
            deref = ptr_load(value.module.nr(VarValue, self.mapped_var(value.var)))
            self.saw(deref)  # C++ calls saw(), not see(), here.
            value.replace_all(deref)

    # C++: codon/cir/util/outlining.cpp:78
    def handle_PointerValue(self, value):
        # A pointer-passed variable's pointer is the argument itself.
        if value.var.id in self.mod_vars:
            ref = value.module.nr(VarValue, self.mapped_var(value.var))
            self.saw(ref)
            value.replace_all(ref)

    # C++: codon/cir/util/outlining.cpp:90
    def handle_AssignInstr(self, value):
        # Store through the pointer for a modified argument.
        if value.lhs.id in self.mod_vars:
            new = self.mapped_var(value.lhs)
            store = ptr_store(value.module.nr(VarValue, new), value.rhs)
            self.saw(store)
            value.replace_all(store)


@dataclass(init=False, eq=False)
class Outliner(Operator):
    """C++: codon/cir/util/outlining.cpp:102. Discover variables/control flow in an IR region."""

    parent: BodiedFunc
    flow_region: SeriesFlow
    begin: int
    end: int
    outline_globals: bool
    all_by_value: bool
    in_region: bool
    invalid: bool
    in_vars: Set[int]
    out_vars: Set[int]
    modified_in_vars: Set[int]
    globals_to_outline: Set[int]
    in_loops: Set[int]
    out_flows: List[object]

    # C++: codon/cir/util/outlining.cpp:119
    def __init__(self, parent, region, begin, end, outline_globals, all_by_value):
        super().__init__()
        self.parent, self.flow_region, self.begin, self.end = parent, region, begin, end
        # Whether to outline modified globals; forcing by-value can change semantics.
        self.outline_globals, self.all_by_value = outline_globals, all_by_value
        self.in_region, self.invalid = False, False
        self.in_vars, self.out_vars, self.modified_in_vars = set(), set(), set()
        self.globals_to_outline, self.in_loops, self.out_flows = set(), set(), []

    # C++: codon/cir/util/outlining.cpp:127
    def is_enclosing_loop_in_region(self, identifier=-1):
        for node in reversed(self.node_stack):
            if isinstance(node.get_actual(), (WhileFlow, ForFlow, ImperativeForFlow)) and (
                identifier == -1 or identifier == node.id
            ):
                return node.id in self.in_loops
        return False

    # C++: codon/cir/util/outlining.cpp:142
    def handle_WhileFlow(self, value):
        if self.in_region:
            self.in_loops.add(value.id)

    # C++: codon/cir/util/outlining.cpp:147
    def handle_ForFlow(self, value):
        if self.in_region:
            self.in_loops.add(value.id)

    # C++: codon/cir/util/outlining.cpp:152
    def handle_ImperativeForFlow(self, value):
        if self.in_region:
            self.in_loops.add(value.id)

    # C++: codon/cir/util/outlining.cpp:157
    def handle_ReturnInstr(self, value):
        if self.in_region:
            self.out_flows.append(value)

    # C++: codon/cir/util/outlining.cpp:162
    def handle_BreakInstr(self, value):
        if self.in_region and not self.is_enclosing_loop_in_region(
            value.loop.id if value.loop else -1
        ):
            self.out_flows.append(value)

    # C++: codon/cir/util/outlining.cpp:168
    def handle_ContinueInstr(self, value):
        if self.in_region and not self.is_enclosing_loop_in_region(
            value.loop.id if value.loop else -1
        ):
            self.out_flows.append(value)

    # C++: codon/cir/util/outlining.cpp:174
    def handle_YieldInstr(self, value):
        if self.in_region:
            self.invalid = True

    # C++: codon/cir/util/outlining.cpp:179
    def handle_AwaitInstr(self, value):
        if self.in_region:
            self.invalid = True

    # C++: codon/cir/util/outlining.cpp:184
    def handle_YieldInInstr(self, value):
        if self.in_region:
            self.invalid = True

    # C++: codon/cir/util/outlining.cpp:189
    def handle_StackAllocInstr(self, value):
        if self.in_region:
            self.invalid = True

    # C++: codon/cir/util/outlining.cpp:194
    def handle_AssignInstr(self, value):
        if self.in_region:
            var = value.lhs
            self.modified_in_vars.add(var.id)
            if self.outline_globals and var.global_ and not var.tls:
                self.globals_to_outline.add(var.id)

    # C++: codon/cir/util/outlining.cpp:203
    def handle_PointerValue(self, value):
        if self.in_region:
            var = value.var
            self.modified_in_vars.add(var.id)
            if self.outline_globals and var.global_ and not var.tls:
                self.globals_to_outline.add(var.id)

    # C++: codon/cir/util/outlining.cpp:212
    def visit(self, value):
        if isinstance(value, SeriesFlow) and value.id == self.flow_region.id:
            for child in value.series[: self.begin]:
                self.process(child)
            self.in_region = True
            for child in value.series[self.begin : self.end]:
                self.process(child)
            self.in_region = False
            for child in value.series[self.end :]:
                self.process(child)
            return
        if isinstance(value, BodiedFunc):
            self.out_vars.update(arg.id for arg in value.args)
        return super().visit(value)

    # C++: codon/cir/util/outlining.cpp:241
    def pre_hook(self, node):
        identifiers = self.in_vars if self.in_region else self.out_vars
        for var in node.get_used_variables():
            if not var.global_:
                identifiers.add(var.id)
            elif self.in_region and self.all_by_value and not is_a(Func, var):
                self.globals_to_outline.add(var.id)

    # C++: codon/cir/util/outlining.cpp:253
    def get_private_vars(self):
        # Private = used inside AND NOT outside the region.
        return self.in_vars - self.out_vars

    # C++: codon/cir/util/outlining.cpp:263
    def get_shared_vars(self):
        # Shared = used inside AND outside the region.
        return self.in_vars & self.out_vars

    # C++: codon/cir/util/outlining.cpp:273
    def get_mod_vars(self):
        # Modified = shared AND modified inside the region.
        return (
            set()
            if self.all_by_value
            else self.modified_in_vars & (self.globals_to_outline | self.get_shared_vars())
        )

    # C++: codon/cir/util/outlining.cpp:285
    def outline(self, allow_out_flows=True):
        if self.invalid:
            return OutlineResult()
        module = self.flow_region.module
        remap, arg_types, arg_names, arg_kinds = [], [], [], []
        # Private variables become locals; shared variables are arguments;
        # modified shared variables are pointers.
        shared = self.get_shared_vars() | self.globals_to_outline
        modified = self.get_mod_vars()

        # Capture order defines the outlined ABI and downstream OpenMP tuple layout.
        # Names are stable across frontend allocation orders; IDs break local name ties.
        def ordered_vars(variables):
            return sorted(
                variables, key=lambda identifier: (module.get_var(identifier).name, identifier)
            )

        for identifier in ordered_vars(shared):
            var = module.get_var(identifier)
            assert var, "unknown var id"
            remap.append((var, None))
            is_modified = identifier in modified
            arg_types.append(module.get_pointer_type(var.type) if is_modified else var.type)
            arg_names.append(var.name)
            arg_kinds.append(
                OutlineResult.ArgKind.MODIFIED if is_modified else OutlineResult.ArgKind.CONSTANT
            )
        # A function with external control flow returns its status as an int.
        controls = bool(self.out_flows)
        if controls and not allow_out_flows:
            return OutlineResult()
        signature = module.get_func_type(
            module.get_int_type() if controls else module.get_none_type(), arg_types
        )
        first = self.flow_region.series[self.begin]
        fn = module.n(BodiedFunc, first, get_derived_func_name("__outlined", self.parent, first))
        fn.realize(signature, arg_names)
        remap = [(old, arg) for (old, _), arg in zip(remap, fn.args)]
        # Make private variables locals of the new function.
        for identifier in ordered_vars(self.get_private_vars()):
            var = module.get_var(identifier)
            assert var, "unknown var id"
            new = module.n(Var, var.info, var.type, False, False, False, var.name)
            remap.append((var, new))
            fn.symbols.append(new)
        # Move the region out of its parent and into the outlined function.
        body = module.n(SeriesFlow, first.info)
        body.series.extend(self.flow_region.series[self.begin : self.end])
        del self.flow_region.series[self.begin : self.end]
        fn.body = body
        OutlineReplacer(module, modified, remap, self.out_flows).process(body)
        args = [
            module.nr(PointerValue, old) if old.id in modified else module.nr(VarValue, old)
            for old, _ in remap[: len(shared)]
        ]
        outlined_call = call(fn, args)
        position = self.begin
        if controls:
            code_var = module.nr(Var, module.get_int_type())
            self.parent.symbols.append(code_var)
            self.flow_region.series.insert(
                position, module.nr(AssignInstr, code_var, outlined_call)
            )
            # Zero is normal return. Generate if (result == code) { action }
            # for each other return code, 1-based by convention.
            for idx, out_flow in enumerate(self.out_flows):
                check = module.get_int(idx + 1).eq(module.nr(VarValue, code_var))
                position += 1
                self.flow_region.series.insert(position, module.nr(IfFlow, check, series(out_flow)))
        else:
            self.flow_region.series.insert(position, outlined_call)
        return OutlineResult(fn, outlined_call, arg_kinds, len(self.out_flows))


# C++: codon/cir/util/outlining.cpp:392
def outline_region(
    parent,
    series_,
    begin=0,
    end=None,
    allow_out_flows=True,
    outline_globals=False,
    all_by_value=False,
):
    """C++: outlining.cpp:390. Outline [begin,end), replacing it with a call."""
    end = len(series_.series) if end is None else end
    if begin == end:
        return OutlineResult()
    outliner = Outliner(parent, series_, begin, end, outline_globals, all_by_value)
    outliner.process(parent)
    return outliner.outline(allow_out_flows)
