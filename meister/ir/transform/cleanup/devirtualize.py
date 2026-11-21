# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass, field

from ... import (
    BodiedFunc, CallInstr, Const, ExtractInstr, FlowInstr, Func, IntConst,
    InternalFunc, Intrinsic, RecordType, RefType, SeriesFlow, TernaryInstr, Value, VarValue,
    cast,
)
from ...analyze.dataflow.cfg import SyntheticAssignInstr
from ...analyze.dataflow.reaching import RDResult
from ...attribute import ExactTypeAttribute, KeyValueAttribute
from ...module import Module
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func, has_attribute, is_gpu_kernel, set_export
from ...util.operator import Operator
from ..pass_ import Pass


# Exhausting a proof or growth budget leaves the type unknown.
MAX_TYPE_DEPTH = 256
MAX_SUMMARY_DEPTH = 32
MAX_ARGUMENT_FIELDS = 64
MAX_SPECIALIZATION_SIZE = 256
MAX_SPECIALIZATION_GROWTH = 32768
MAX_VARIANTS = 8
SPECIALIZED_ATTR = ".devirtualized"

Path = tuple[int, ...]
Facts = dict[Path, RefType]
ContextKey = tuple[int, tuple[tuple[Path, str], ...]]
Query = tuple[int, Path]


def direct_callee(value):
    # Inspect the final callee while leaving its FlowInstr setup intact.
    while flow := cast(FlowInstr, value):
        value = flow.value
    return get_func(value)


@dataclass(init=False, eq=False)
class BodyInfo(Operator):
    size: int
    has_shared_values: bool
    operands: set[int]
    returns: list[Value | None]
    calls: list[CallInstr]
    fields: list[tuple[Value, Value]]

    def __init__(self):
        super().__init__()
        self.size = 0
        self.has_shared_values = False
        self.operands = set()
        self.returns, self.calls, self.fields = [], [], []

    def pre_hook(self, node):
        self.size += 1
        if self.has_shared_values:
            return
        for operand in node.get_used_values():
            if operand is not None and not cast(Const, operand) and not get_func(operand):
                if operand.id in self.operands:
                    self.has_shared_values = True
                    return
                self.operands.add(operand.id)

    def handle_ReturnInstr(self, value):
        self.returns.append(value.value)

    def handle_CallInstr(self, value):
        self.calls.append(value)

    def handle_ExtractInstr(self, value):
        self.fields.append((value, value.val))

    def handle_InsertInstr(self, value):
        self.fields.append((value, value.lhs))


@dataclass
class Context:
    # Paths project through zero-based record fields. Argument facts prefix
    # the field path with the argument index.
    source: BodiedFunc
    target: BodiedFunc
    arguments: Facts
    clone: CloneVisitor | None = None
    cache: dict[Query, RefType | None] = field(default_factory=dict)
    active: set[Query] = field(default_factory=set)
    returns: dict[Path, RefType | None] = field(default_factory=dict)
    active_returns: set[Path] = field(default_factory=set)
    fields: list[tuple[Value, RefType | None]] = field(default_factory=list)
    calls: list[tuple[CallInstr, Func]] = field(default_factory=list)
    lookups: list[tuple[CallInstr, Func]] = field(default_factory=list)


@dataclass
class Specializer:
    module: Module
    reaching: RDResult
    jit: bool
    growth: int = 0
    summary_depth: int = 0
    bodies: dict[int, BodyInfo] = field(default_factory=dict)
    variants: dict[int, int] = field(default_factory=dict)
    contexts: dict[ContextKey, Context] = field(default_factory=dict)
    pending: list[Context] = field(default_factory=list)

    def body(self, fn):
        if fn.id not in self.bodies:
            info = BodyInfo()
            info.process(fn.body)
            self.bodies[fn.id] = info
        return self.bodies[fn.id]

    def context(self, fn, arguments=None):
        if (
            not fn or fn.generator or fn.async_ or fn.id not in self.reaching.results
            or has_attribute(fn, SPECIALIZED_ATTR)
        ):
            return None
        arguments = arguments or {}
        facts = sorted(arguments.items())
        key = fn.id, tuple((path, typ.name) for path, typ in facts)
        if key in self.contexts:
            return self.contexts[key]
        if arguments:
            body = self.body(fn)
            if (
                body.has_shared_values or self.variants.get(fn.id, 0) >= MAX_VARIANTS
                or body.size > MAX_SPECIALIZATION_SIZE
                or self.growth + body.size > MAX_SPECIALIZATION_GROWTH
                or is_gpu_kernel(fn)
            ):
                return self.context(fn)

        # Publish the context before inspecting calls so recursion can find it.
        result = Context(fn, fn, arguments)
        self.contexts[key] = result
        self.pending.append(result)
        if arguments:
            self.variants[fn.id] = self.variants.get(fn.id, 0) + 1
            self.growth += self.body(fn).size
            clone = result.clone = CloneVisitor(self.module)
            name = fn.name + ".devirtualized"
            for path, typ in facts:
                indices = "".join(f"{index}_" for index in path)
                name += f".{indices}{len(typ.name.encode('utf-8', 'surrogateescape'))}:{typ.name}"
            target = result.target = self.module.n(BodiedFunc, fn, name)
            target.unmangled_name, target.parent_type = fn.unmangled_name, fn.parent_type
            target.realize(fn.type, [arg.name for arg in fn.args])
            for original, parameter in zip(fn.args, target.args):
                clone.force_remap(original, parameter)
            target.symbols.extend(clone.force_clone(var) for var in fn)
            # Recursive calls retain the original function and need their own proof.
            target.body = clone.clone(fn.body)
            for attr_id, attr in fn.attributes.items():
                if attr.needs_clone():
                    target.set_attribute(attr.clone(clone), attr_id)
            attributes = target.get_attribute(KeyValueAttribute)
            if attributes is None:
                attributes = KeyValueAttribute()
                target.set_attribute(attributes)
            set_export(target, False)
            attributes.attributes[SPECIALIZED_ATTR] = ""
        return result

    def collect(self, caller, value, typ, path, facts, remaining):
        if not remaining or len(path) >= MAX_SUMMARY_DEPTH:
            return remaining
        remaining -= 1
        if record := cast(RecordType, typ):
            for index, member in enumerate(record):
                remaining = self.collect(caller, value, member.type, (*path, index), facts, remaining)
                if not remaining:
                    break
        elif (exact := self.get(caller, value, path[1:])) and exact.polymorphic:
            facts[path] = exact
        return remaining

    def callee_context(self, caller, call, callee):
        facts, remaining = {}, MAX_ARGUMENT_FIELDS
        # Follow value records, including partials, but never mutable ref fields.
        for index, arg in enumerate(call):
            remaining = self.collect(caller, arg, arg.type, (index,), facts, remaining)
        return self.context(callee, facts)

    def returned(self, context, fields):
        if fields in context.returns:
            return context.returns[fields]
        if self.summary_depth >= MAX_SUMMARY_DEPTH or fields in context.active_returns:
            return None
        self.summary_depth += 1
        context.active_returns.add(fields)
        result = None
        for value in self.body(context.source).returns:
            typ = self.get(context, value, fields)
            if not typ or (result and result is not typ):
                result = None
                break
            result = typ
        self.summary_depth -= 1
        context.active_returns.remove(fields)
        context.returns[fields] = result
        return result

    def infer(self, context, value, fields):
        match value:
            case VarValue():
                analysis = self.reaching.results[context.source.id]
                var = value.var
                if var.global_ or analysis.is_invalid(var):
                    return None
                result = None
                for definition in analysis.get_reaching_definitions(var, value):
                    entry = definition.assignment
                    if isinstance(entry, SyntheticAssignInstr) and entry.source is context.source:
                        # Only synthetic entry assignments represent incoming argument facts.
                        index = context.source.args.index(var)
                        typ = context.arguments.get((index, *fields))
                    else:
                        typ = self.get(context, definition.assignee, fields)
                    if not typ or (result and result is not typ):
                        return None
                    result = typ
                return result
            case FlowInstr():
                return self.get(context, value.value, fields)
            case ExtractInstr():
                if record := cast(RecordType, value.val.type):
                    index = record.get_member_index(value.field)
                    return self.get(context, value.val, (index, *fields))
            case TernaryInstr():
                first = self.get(context, value.true_value, fields)
                if first and first is self.get(context, value.false_value, fields):
                    return first
            case CallInstr():
                callee = direct_callee(value.callee)
                if intrinsic := cast(InternalFunc, callee):
                    if not intrinsic.has_valid_intrinsic_signature():
                        return None
                    if intrinsic.intrinsic == Intrinsic.ALLOCATE and not fields:
                        return cast(RefType, intrinsic.intrinsic_type)
                    if intrinsic.intrinsic == Intrinsic.REPRESENTATION_CAST:
                        return self.get(context, value.front(), fields)
                    if intrinsic.intrinsic == Intrinsic.RECORD_NEW and fields:
                        index = fields[0]
                        if 0 <= index < len(value.args):
                            return self.get(context, value.args[index], fields[1:])
                elif fn := cast(BodiedFunc, callee):
                    if target := self.callee_context(context, value, fn):
                        return self.returned(target, fields)
        return None

    def get(self, context, value, fields=()):
        if value is None or self.body(context.source).has_shared_values:
            return None
        query = value.id, fields
        if query in context.cache:
            return context.cache[query]
        if len(context.active) >= MAX_TYPE_DEPTH or len(fields) >= MAX_TYPE_DEPTH or query in context.active:
            return None
        context.active.add(query)
        result = self.infer(context, value.get_actual(), fields)
        context.active.remove(query)
        context.cache[query] = result
        return result

    def virtual_target(self, context, value):
        while flow := cast(FlowInstr, value):
            value = flow.value
        call = cast(CallInstr, value)
        fn = cast(InternalFunc, direct_callee(call.callee)) if call else None
        # JIT cells can refresh method tables, so only bind a slot in AOT.
        if self.jit or not fn or fn.intrinsic != Intrinsic.VIRTUAL_LOOKUP or len(call.args) != 2:
            return None
        typ = self.get(context, call.front())
        method = cast(IntConst, call.back())
        if typ and typ.runtime_info and method:
            target = typ.runtime_info.methods.get(method.val)
            if target and target.type is call.type:
                return target
        return None

    def plan(self, context):
        info = self.body(context.source)

        def mapped(value):
            # Replacing a source node can change its ID, so resolve clones now.
            return context.clone.clone(value) if context.clone else value

        for field_access, receiver in info.fields:
            exact = None
            declared = cast(RefType, receiver.type)
            if declared and declared.polymorphic:
                typ = self.get(context, receiver)
                if typ and typ.runtime_info and declared in typ.runtime_info.mro:
                    exact = typ
            context.fields.append((mapped(field_access), exact))
        for call in info.calls:
            destination = cast(CallInstr, mapped(call))
            if target := self.virtual_target(context, call):
                context.lookups.append((destination, target))
            callee = direct_callee(call.callee) or self.virtual_target(context, call.callee)
            if fn := cast(BodiedFunc, callee):
                target = self.callee_context(context, call, fn)
                if target and target.target is not fn:
                    context.calls.append((destination, target.target))

    def apply(self, context):
        module = self.module
        for value, exact in context.fields:
            value.erase_attribute(ExactTypeAttribute)
            if exact:
                value.set_attribute(ExactTypeAttribute(exact))
        for call, target in context.calls:
            prefix = module.n(SeriesFlow, call)
            prefix.push_back(call.callee)
            call.callee = module.n(FlowInstr, call, prefix, module.n(VarValue, call, target))
        for call, target in context.lookups:
            prefix = module.n(SeriesFlow, call)
            prefix.push_back(call.callee)
            for argument in call:
                prefix.push_back(argument)
            call.replace_all(module.n(FlowInstr, call, prefix, module.n(VarValue, call, target)))

    def run(self):
        for value in (self.module.main_func, *self.module):
            if function := cast(BodiedFunc, value):
                self.context(function)
        # Finish every proof against the original CIR before changing it.
        for context in self.pending:
            self.plan(context)
        for context in self.pending:
            self.apply(context)


@dataclass(init=False, eq=False)
class DevirtualizationPass(Pass):
    """C++: codon/cir/transform/cleanup/devirtualize.cpp."""

    KEY = "core-devirtualize"

    reaching_key: str

    def __init__(self, reaching_key):
        super().__init__()
        self.reaching_key = reaching_key

    def run(self, module):
        reaching = self.get_analysis_result(self.reaching_key)
        Specializer(module, reaching, self.get_options().jit).run()
