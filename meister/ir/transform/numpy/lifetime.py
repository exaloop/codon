# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from collections import defaultdict
from dataclasses import dataclass

from ... import (
    AssignInstr, BodiedFunc, CallInstr, ExtractInstr, Flow, FlowInstr, ForFlow,
    IfFlow, ImperativeForFlow, Node, RecordType, ReturnInstr, SeriesFlow,
    TryCatchFlow, Value, Var, VarValue, WhileFlow, cast,
)
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, make_var, series
from ...util.operator import Operator
from .fusion import has_owned_result, is_basic_index_type
from .parse import is_named_function, parse
from .types import (
    FUSION_MODULE, NumPyPass, NumPyPrimitiveTypes, NumPyType,
    has_lifetime_tag, is_array_type, is_copy_index_type, is_owned_index, is_ufunc_type,
)


_REPLACEMENT_FREE_THRESHOLD = 512
_UPDATES = {
    "__iadd__", "__isub__", "__imul__", "__itruediv__", "__ifloordiv__", "__imod__",
    "__ipow__", "__iand__", "__ior__", "__ixor__", "__ilshift__", "__irshift__",
}
_METADATA = {"shape", "strides", "size", "ndim", "itemsize", "nbytes"}


def owns_array_result(value, types):
    if not NumPyType.get(value.type, types).is_array():
        return False
    if has_lifetime_tag(value, "numpy.lifetime.owned") or is_owned_index(value, types):
        return True
    expr = parse(value, [], types)
    return bool(expr and has_owned_result(expr))


def is_array_update(invocation, types):
    fn = get_func(invocation.callee)
    if (
        not fn or len(invocation.args) < 2 or not is_array_type(fn.parent_type)
        or not NumPyType.get(invocation.front().type, types).is_array()
    ):
        return False
    if fn.unmangled_name == "__setitem__" and len(invocation.args) == 3:
        typ = invocation.args[1].type
        return is_basic_index_type(typ, types) or is_copy_index_type(typ, types)
    return len(invocation.args) == 2 and fn.unmangled_name in _UPDATES


def release_helper(module, name, types):
    fn = module.get_or_realize_func(name, types, [], FUSION_MODULE)
    assert fn, "NumPy release function not found"
    return fn


@dataclass
class Definition:
    assignment: AssignInstr
    series: SeriesFlow


@dataclass
class Replacement:
    assignment: AssignInstr
    owned: bool
    in_loop: bool
    safe: bool


@dataclass
class Use:
    value: Value
    parents: list[Node]
    safe: bool

    def parent(self, level=0):
        return self.parents[-1 - level] if len(self.parents) > level else None


def inside_protected_expression(parents):
    return any(
        cast(TryCatchFlow, parent) or (
            cast(Value, parent) and has_lifetime_tag(parent, "numpy.lifetime.expression")
        )
        for parent in parents
    )


@dataclass(init=False, eq=False)
class LocalLifetime(Operator):
    types: NumPyPrimitiveTypes
    candidates: list[Definition]
    replacements: dict[int, list[Replacement]]
    uses: dict[int, list[Use]]

    def __init__(self, fn):
        super().__init__()
        self.types = NumPyPrimitiveTypes(fn.module)
        self.candidates = []
        self.replacements, self.uses = defaultdict(list), defaultdict(list)

    def handle_AssignInstr(self, assignment):
        target = assignment.lhs
        parent = self.get_parent(cls=SeriesFlow) if self.depth() else None
        if not NumPyType.get(target.type, self.types).is_array():
            return
        owned = owns_array_result(assignment.rhs, self.types)
        protected = inside_protected_expression(self.node_stack)
        loops = [
            node for parent_ in self.node_stack
            if isinstance((node := parent_.get_actual()), (ForFlow, ImperativeForFlow, WhileFlow))
        ]
        eligible = bool(parent and not target.global_ and not protected)
        safe = eligible and all(
            not isinstance(loop, (ForFlow, ImperativeForFlow)) or not loop.is_parallel()
            for loop in loops
        )
        self.replacements[target.id].append(Replacement(assignment, owned, bool(loops), safe))
        if owned and eligible:
            self.candidates.append(Definition(assignment, parent))

    def safe_replacement_read(self, use):
        if any(cast(TryCatchFlow, parent) for parent in use.parents):
            return False
        if use.safe:
            return True
        read = cast(VarValue, use.value)
        parent = use.parent()
        if not read or not parent:
            return False
        # Returning the current buffer is safe for replacement cleanup only.
        if ret := cast(ReturnInstr, parent):
            return ret.value is read
        invocation = cast(CallInstr, parent)
        fn = get_func(invocation.callee) if invocation else None
        ret = cast(ReturnInstr, use.parent(1))
        if (
            fn and ret and ret.value is invocation and fn.unmangled_name == "__new__"
            and fn.parent_type and fn.parent_type.name == "Tuple"
        ):
            return True
        if not fn or not invocation.args or invocation.front() is not read:
            return False
        if len(invocation.args) == 1 and is_named_function(fn, "std.internal.builtin", "len"):
            return True
        if not is_array_type(fn.parent_type):
            return False
        if len(invocation.args) == 1 and fn.unmangled_name in _METADATA | {"__len__"}:
            return True
        return bool(is_array_update(invocation, self.types) and cast(SeriesFlow, use.parent(1)))

    def safe_read(self, read):
        if not self.depth():
            return False
        parent = self.get_parent()
        if assignment := cast(AssignInstr, parent):
            # Only compiler-generated borrowed input bindings are nonescaping.
            return assignment.rhs is read and has_lifetime_tag(assignment, "numpy.lifetime.input")
        invocation = cast(CallInstr, parent)
        if not invocation:
            return False
        if owns_array_result(invocation, self.types):
            return True
        fn = get_func(invocation.callee)
        if (
            fn and is_ufunc_type(fn.parent_type) and fn.unmangled_name == "__call__"
            and len(invocation.args) in (4, 5) and self.depth() >= 2
            and self.get_parent(1, SeriesFlow)
            and all(
                NumPyType.get(value.type, self.types) or value.type.is_type(self.types.none)
                for value in invocation.args[1:]
            )
        ):
            return True
        if is_array_update(invocation, self.types):
            return invocation.back() is read or (
                fn.unmangled_name == "__setitem__"
                and read in (invocation.front(), invocation.args[1])
            )
        output = NumPyType.get(invocation.type, self.types)
        return bool(
            fn and is_array_type(fn.parent_type) and output and not output.is_array()
            and fn.unmangled_name == "__getitem__" and len(invocation.args) == 2
            and invocation.front() is read and is_basic_index_type(invocation.back().type, self.types)
        )

    def pre_hook(self, node):
        if not isinstance(node, Value) or isinstance(node, AssignInstr):
            return
        safe = isinstance(node, VarValue) and self.safe_read(node)
        for var in node.get_used_variables():
            if is_array_type(var.type) or var.type.name.startswith("Tuple["):
                self.uses[var.id].append(Use(node, list(self.node_stack), safe))

    def transfer_tuple_ownership(self, reaching, graph):
        # Unpack a fresh gathered array only when each field is extracted once
        # and the tuple cannot retain another alias of it.
        for replacements in self.replacements.values():
            for replacement in replacements:
                extract = cast(ExtractInstr, replacement.assignment.rhs)
                read = cast(VarValue, extract.val) if extract else None
                if not read or not replacement.safe or reaching.is_invalid(read.var):
                    continue
                definitions = reaching.get_reaching_definitions(read.var, read)
                if len(definitions) != 1 or not definitions[0].known():
                    continue
                tuple_assignment = definitions[0].assignment
                constructor = cast(CallInstr, definitions[0].assignee)
                fn = get_func(constructor.callee) if constructor else None
                tuple_type = cast(RecordType, fn.parent_type) if fn else None
                if not tuple_type or tuple_type.name != "Tuple" or fn.unmangled_name != "__new__":
                    continue
                index = cast(RecordType, read.type).get_member_index(extract.field)
                if not 0 <= index < len(constructor.args) or not owns_array_result(
                    constructor.args[index], self.types
                ):
                    continue
                block = graph.get_block(tuple_assignment)
                if not block:
                    continue
                fields, safe = set(), True
                for use in self.uses[read.var.id]:
                    definitions = reaching.get_reaching_definitions(read.var, use.value)
                    if not any(definition.assignment is tuple_assignment for definition in definitions):
                        continue
                    member, assignment = cast(ExtractInstr, use.parent()), cast(AssignInstr, use.parent(1))
                    after_definition = False
                    for value in block:
                        if value is use.value:
                            break
                        after_definition |= value is tuple_assignment
                    if (
                        len(definitions) != 1 or not member or not assignment
                        or assignment.rhs is not member or not cast(SeriesFlow, use.parent(2))
                        or graph.get_block(use.value) is not block or not after_definition
                        or member.field in fields
                    ):
                        safe = False
                        break
                    fields.add(member.field)
                replacement.owned = safe and extract.field in fields

    def release_replacements(self, fn, reaching):
        # Replacement cleanup spans serial iterations. Last-use cleanup stays
        # within a basic block, so each variable uses only one protocol.
        replaced = set()
        body = cast(SeriesFlow, fn.body)
        if not body:
            return replaced
        module = fn.module
        for identifier, replacements in self.replacements.items():
            var = replacements[0].assignment.lhs
            if (
                not any(rep.in_loop and rep.owned for rep in replacements)
                or not all(rep.safe for rep in replacements)
                or not all(self.safe_replacement_read(use) for use in self.uses[identifier])
                or reaching.is_invalid(var)
            ):
                continue
            owned = module.nr(Var, module.get_bool_type())
            owner = module.nr(Var, var.type)
            fn.symbols.extend([owned, owner])
            body.insert(0, module.nr(AssignInstr, owned, module.get_bool(False)))
            replaced.add(identifier)
            release = release_helper(module, "_free_replacement", [var.type, module.get_int_type()])
            for replacement in replacements:
                assignment = replacement.assignment
                block = module.nr(SeriesFlow)
                # Finish the RHS before freeing the previous buffer, which it
                # can still read. A false flag protects borrowed storage.
                result = make_var(assignment.rhs, block, fn)
                free = call(release, [
                    module.nr(VarValue, owner), module.get_int(_REPLACEMENT_FREE_THRESHOLD),
                ])
                block.push_back(module.nr(IfFlow, module.nr(VarValue, owned), series(free)))
                if replacement.owned:
                    block.push_back(module.nr(AssignInstr, owner, module.nr(VarValue, result)))
                block.push_back(module.nr(AssignInstr, owned, module.get_bool(replacement.owned)))
                assignment.rhs = module.nr(FlowInstr, block, module.nr(VarValue, result))
        return replaced

    def get_release_insertions(self, fn, reaching, graph, replaced):
        positions = {}
        for candidate in self.candidates:
            if block := graph.get_block(candidate.assignment):
                positions.update((value.id, index) for index, value in enumerate(block))

        insertions = defaultdict(list)
        module = fn.module
        for candidate in self.candidates:
            assignment = candidate.assignment
            var = assignment.lhs
            if var.id in replaced or reaching.is_invalid(var):
                continue
            block = graph.get_block(assignment)
            if not block:
                continue
            original = positions[assignment.id]
            last, last_position, safe = assignment, original, True
            for use in self.uses[var.id]:
                definitions = reaching.get_reaching_definitions(var, use.value)
                if not any(definition.assignment.id == assignment.id for definition in definitions):
                    continue
                statement = next(
                    (node for parent, node in zip(use.parents, use.parents[1:])
                     if parent is candidate.series),
                    None,
                )
                position = positions.get(use.value.id)
                if (
                    not use.safe or len(definitions) != 1 or not cast(Value, statement)
                    or graph.get_block(use.value) is not block or position is None
                    or position <= original or cast(ReturnInstr, statement) or cast(Flow, statement)
                ):
                    safe = False
                    break
                if position > last_position:
                    last, last_position = statement, position
            if safe:
                owner = module.nr(Var, var.type)
                fn.push_back(owner)
                insertions[assignment].append(module.nr(AssignInstr, owner, module.nr(VarValue, var)))
                release = release_helper(module, "_free", [var.type])
                insertions[last].append(call(release, [module.nr(VarValue, owner)]))
        return insertions


@dataclass(init=False, eq=False)
class InsertReleases(Operator):
    insertions: dict[Value, list[Value]]

    def __init__(self, insertions):
        super().__init__()
        self.insertions = insertions

    def handle_SeriesFlow(self, flow):
        steps = []
        for statement in flow:
            steps.append(statement)
            steps.extend(self.insertions.get(statement, ()))
        flow.series[:] = steps


@dataclass(init=False, eq=False)
class ReleaseArguments(Operator):
    func: BodiedFunc
    types: NumPyPrimitiveTypes

    def __init__(self, func):
        super().__init__()
        self.func, self.types = func, NumPyPrimitiveTypes(func.module)

    def handle_CallInstr(self, invocation):
        if not is_array_update(invocation, self.types) or not owns_array_result(
            invocation.back(), self.types
        ):
            return
        module = invocation.module
        block = module.nr(SeriesFlow)
        clone = CloneVisitor(module)
        # Bind receiver, index and RHS in that order, once each.
        args = [
            module.nr(VarValue, make_var(clone.clone(arg), block, self.func))
            for arg in invocation
        ]
        result = make_var(call(get_func(invocation.callee), args), block, self.func)
        release = release_helper(module, "_free", [args[-1].type])
        block.push_back(call(release, [args[-1]]))
        invocation.replace_all(module.nr(FlowInstr, block, module.nr(VarValue, result)))


@dataclass(init=False, eq=False)
class NumPyLifetimePass(NumPyPass):
    KEY = "core-numpy-lifetime"

    reaching_def_key: str

    def __init__(self, reaching_def_key):
        super().__init__()
        self.reaching_def_key = reaching_def_key

    def visit(self, node):
        func = cast(BodiedFunc, node)
        if not func:
            return super().visit(node)
        # Use fresh CFG/RD after fusion has transferred ownership.
        result = self.get_analysis_result(self.reaching_def_key)
        reaching = result.results.get(func.id)
        if reaching is None:
            return
        graph = result.cfg_result.graphs[func.id]
        lifetimes = LocalLifetime(func)
        lifetimes.process(func)
        lifetimes.transfer_tuple_ownership(reaching, graph)
        replaced = lifetimes.release_replacements(func, reaching)
        insertions = lifetimes.get_release_insertions(func, reaching, graph, replaced)
        InsertReleases(insertions).process(func)
        ReleaseArguments(func).process(func)
