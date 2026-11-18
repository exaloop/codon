# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ... import ast
from ...bridge import Dict, Enum, List, Set, dataclass
from .. import attribute, flow, func, instr
from .. import type as types
from ..base import Node, cast
from ..const import Const
from ..flow import SeriesFlow
from ..func import BodiedFunc, Func
from ..instr import AssignInstr, CallInstr
from ..module import Module
from ..value import Value
from ..var import Var, VarValue


@dataclass(init=False, eq=False)
class FormatContext:
    """C++: codon/cir/util/format.cpp. Dump-local, allocation-independent names.

    Module storage includes function arguments and locals, whose allocation
    order can depend on unordered frontend caches. Order declarations by their
    semantic owner and number nodes by a deterministic graph traversal instead.
    This never changes an IR node, signature, call, or compiler counter.
    """

    canonical: bool
    ids: Dict[int, int]
    vars: List[Var]
    types: List[types.Type]
    capture_order: Dict[str, List[int]]
    aliases: Dict[str, str]

    # C++: codon/cir/util/format.cpp:30
    def __init__(self, node):
        self.canonical = isinstance(node, Module)
        self.ids, self.capture_order, self.aliases = {}, {}, {}
        self.vars, self.types = [], []
        if not self.canonical:
            return
        self.generator_aliases(node)
        keys = {}
        for fn in [node.main_func, *(var for var in node.vars if isinstance(var, Func))]:
            owner = self.name(fn)
            keys[fn.id] = "func:" + owner
            args = list(fn.args)
            # Generated comprehension closures capture an unordered identifier
            # set. Display their bindings in name order, consistently at the
            # declaration, signature, and call. Ordinary positional arguments
            # and the compiler's actual capture ABI are left untouched.
            if fn.generator and fn.type and fn.type.ast_type is None:
                order = sorted(range(len(args)), key=lambda idx: args[idx].name)
                self.capture_order[fn.type.name] = order
                args = [args[idx] for idx in order]
            for idx, arg in enumerate(args):
                keys[arg.id] = f"arg:{owner}:{arg.name}:{idx}"
            if isinstance(fn, BodiedFunc):
                for idx, var in enumerate(fn.symbols):
                    keys[var.id] = f"local:{owner}:{var.name}:{idx}"
        self.vars = sorted(node.vars, key=lambda var: keys.get(var.id, "var:" + self.name(var)))
        self.types = sorted(node.types, key=self.name)
        for var in [node.argv_var, node.argc_var, node.main_func, *self.vars]:
            self.node_id(var)
        for var in [*self.vars, node.main_func]:
            if isinstance(var, BodiedFunc):
                self.number_values(var.body)

    def generator_aliases(self, module):
        """Generated ordinals depend on unordered function-cache traversal, not semantics."""
        functions = [module.main_func, *(var for var in module.vars if isinstance(var, Func))]
        generators = {
            fn.id: fn
            for fn in functions
            if fn.generator
            and fn.name.startswith("_generator.")
            and fn.name[len("_generator.") :].isdigit()
        }
        owners, positions = {}, {}
        for fn in functions:
            if not isinstance(fn, BodiedFunc):
                continue
            pending, seen, ordinal = [fn.body], set(), 0
            while pending:
                node = pending.pop()
                if node is None:
                    continue
                node = node.get_actual()
                if node.id in seen:
                    continue
                seen.add(node.id)
                if isinstance(node, VarValue) and node.var.id in generators:
                    if node.var.id not in positions:
                        owners[node.var.id] = fn
                        positions[node.var.id] = ordinal
                        ordinal += 1
                pending.extend(node.get_used_values())

        def alias(fn):
            if fn.name in self.aliases:
                return self.aliases[fn.name]
            parent = owners.get(fn.id)
            if parent is None:
                # Do not collapse distinct orphan declarations onto one name.
                self.aliases[fn.name] = fn.name
                return fn.name
            owner = alias(parent) if parent.id in generators else parent.name
            name = f"_generator.context.{len(owner.encode())}:{owner}.{positions[fn.id]}"
            self.aliases[fn.name] = name
            return name

        for fn in generators.values():
            alias(fn)

    def name(self, node):
        return self.aliases.get(node.name, node.name)

    # C++: codon/cir/util/format.cpp:88
    def node_id(self, node):
        if not self.canonical:
            return node.id
        if node.id not in self.ids:
            self.ids[node.id] = len(self.ids)
        return self.ids[node.id]

    # C++: codon/cir/util/format.cpp:95
    def reference(self, node):
        return (
            f"{node.name}.{self.node_id(node)}"
            if isinstance(node, (Value, Var))
            else node.reference_string()
        )

    # C++: codon/cir/util/format.cpp:118
    def call_args(self, node):
        callee = cast(VarValue, node.callee)
        fn = cast(Func, callee.var) if callee else None
        order = self.capture_order.get(fn.type.name) if fn and fn.type else None
        return [node.args[idx] for idx in order] if order is not None else node.args

    # C++: codon/cir/util/format.cpp:125
    def series_values(self, node):
        values = list(node.series)
        if not self.canonical:
            return values
        # Adjacent literal stores to distinct variables commute. In particular,
        # the frontend emits compile-time defines from an unordered map. Keep
        # all evaluation order otherwise, including writes to the same variable.
        start = 0
        while start < len(values):
            end, seen = start, set()
            while end < len(values):
                assignment = cast(AssignInstr, values[end])
                if (
                    not assignment
                    or cast(Const, assignment.rhs) is None
                    or assignment.lhs.id in seen
                ):
                    break
                seen.add(assignment.lhs.id)
                end += 1
            values[start:end] = sorted(
                values[start:end], key=lambda value: cast(AssignInstr, value).lhs.name
            )
            start = max(end, start + 1)
        return values

    # C++: codon/cir/util/format.cpp:155
    def number_values(self, node):
        if node is None:
            return
        node = node.get_actual()
        if node.id in self.ids:
            return
        self.node_id(node)
        match node:
            case CallInstr():
                children = [*self.call_args(node), node.callee]
            case SeriesFlow():
                children = self.series_values(node)
            case _:
                children = node.get_used_values()
        for child in children:
            self.number_values(child)


@dataclass(init=False, eq=False)
class Definition:
    """Request a full declaration rather than a graph reference."""

    value: Node

    def __init__(self, value):
        self.value = value


@dataclass(init=False, eq=False)
class Reference:
    """Non-structural edges (including attributes) must not consume definitions."""

    value: Node | None

    def __init__(self, value):
        self.value = value


@dataclass(init=False, eq=False)
class FormatVisitor:
    """C++: codon/cir/util/format.cpp. The same record/list layout as ast.dump()."""

    seen_nodes: Set[int]
    seen_types: Set[str]
    ctx: FormatContext
    annotate_fields: bool
    include_attributes: bool
    indent: int | str | None
    types: bool

    # C++: codon/cir/util/format.cpp:746
    def __init__(
        self, node, annotate_fields=True, include_attributes=False, indent=None, types=False
    ):
        self.seen_nodes, self.seen_types = set(), set()
        self.ctx = FormatContext(node)
        self.annotate_fields, self.include_attributes = annotate_fields, include_attributes
        self.indent, self.types = indent, types

    # C++: codon/cir/util/format.cpp:202
    def items(self, items, simple=True, level=0):
        # C++: formatItems; ast.nodes.dump.format_items. Empty values are omitted,
        # one simple item stays inline, and closing brackets follow the last item.
        items = [value for value in items if value]
        if not self.indent or (simple and len(items) <= 1):
            return ", ".join(items), True
        padding = (" " * self.indent if isinstance(self.indent, int) else self.indent) * level
        return "\n" + padding + (",\n" + padding).join(items), False

    # C++: codon/cir/util/format.cpp:314
    def record(self, name, fields, level=0, node=None, references=False):
        child = level + bool(self.indent)
        args, simple = [], True
        for key, value in fields:
            text, inline = self.value(value, child, references=references)
            simple &= inline
            if text:
                args.append(f"{key}={text}" if self.annotate_fields else text)
        if node is not None and self.include_attributes:
            text, inline = self.attributes(node, child)
            simple &= inline
            if text:
                args.append(text)
        text, inline = self.items(args, simple, child)
        return f"{name}({text})", inline

    # C++: codon/cir/util/format.cpp:334
    def reference(self, node, level):
        if node is None:
            return "", True
        node = node.get_actual()
        fields = [("name", self.ctx.name(node))]
        if isinstance(node, (Var, Value)):
            fields.append(("id", self.ctx.node_id(node)))
        name = (
            "TypeRef"
            if isinstance(node, types.Type)
            else "VarRef"
            if isinstance(node, Var)
            else "ValueRef"
        )
        return self.record(name, fields, level)

    # C++: codon/cir/util/format.cpp:577
    def attributes(self, node, level):
        values = []
        for key, value in node.get_actual().attributes.items():
            key = key.value if isinstance(key, Enum) else key
            # AST dumps omit source locations, including with attributes enabled.
            if key == attribute.SrcInfoAttribute.ATTRIBUTE_ID:
                continue
            try:
                name = ast.Attr(key).name
            except ValueError:
                name = (
                    (type(value).__name__.removesuffix("Attribute") or str(key))
                    if isinstance(value, attribute.Attribute)
                    else str(key)
                )
            if isinstance(value, (attribute.StringValueAttribute, attribute.IntValueAttribute)):
                value = value.value
            elif isinstance(value, attribute.StringListAttribute):
                value = value.values
            text, inline = (
                ("", True) if value is True else self.value(value, level + 2, references=True)
            )
            values.append((name + ("=" + text if text else ""), inline))
        if not values:
            return "", True
        values.sort()
        text, inline = self.items(
            [text for text, _ in values], all(simple for _, simple in values), level + 1
        )
        return f"attrs=({text})", inline

    # C++: codon/cir/util/format.cpp:605
    def fields(self, node):
        fields = [("name", self.ctx.name(node))]
        if isinstance(node, (Var, Value)):
            fields.append(("id", self.ctx.node_id(node)))
        if isinstance(node, Value) and self.types:
            fields.append(("type", node.type))
        if isinstance(node, Var):
            fields.extend(
                (key, getattr(node, key)) for key in ("type", "global_", "external", "tls")
            )
        if isinstance(node, Func):
            order = self.ctx.capture_order.get(node.type.name) if node.type else None
            args = [node.args[idx] for idx in order] if order is not None else node.args
            fields.extend(
                (key, getattr(node, key))
                for key in ("unmangled_name", "generator", "async_", "parent_type")
            )
            fields.append(("args", [Definition(value) for value in args]))
        if isinstance(node, types.Type):
            if self.types:
                fields.append(("ast_type", node.ast_type))
            fields.append(("runtime_info", node.runtime_info))
        match node:
            case Module():
                fields.extend(
                    [
                        ("argv_var", Definition(node.argv_var)),
                        ("argc_var", Definition(node.argc_var)),
                        ("types", [Definition(value) for value in self.ctx.types]),
                        ("vars", [Definition(value) for value in self.ctx.vars]),
                        ("main_func", Definition(node.main_func)),
                    ]
                )
            case BodiedFunc():
                fields.extend(
                    [
                        ("symbols", [Definition(value) for value in node.symbols]),
                        ("body", node.body),
                        ("jit", node.jit),
                    ]
                )
            case func.InternalFunc():
                fields.extend(
                    [("intrinsic", node.intrinsic), ("intrinsic_type", node.intrinsic_type)]
                )
            case func.LLVMFunc():
                fields.extend(
                    (key, getattr(node, key))
                    for key in ("llvm_literals", "llvm_declares", "llvm_body")
                )
            case Const():
                fields.append(("val", node.val))
            case SeriesFlow():
                fields.append(("series", self.ctx.series_values(node)))
            case CallInstr():
                fields.extend([("callee", node.callee), ("args", self.ctx.call_args(node))])
            case instr.BreakInstr() | instr.ContinueInstr():
                fields.append(("loop", Reference(node.loop)))
            case types.FuncType():
                order = self.ctx.capture_order.get(node.name)
                args = (
                    [node.arg_types[idx] for idx in order] if order is not None else node.arg_types
                )
                fields.extend(
                    [
                        ("return_type", node.return_type),
                        ("arg_types", args),
                        ("variadic", node.variadic),
                    ]
                )
            case _:
                schema = {
                    "VarValue": ("var",),
                    "PointerValue": ("var", "fields"),
                    "WhileFlow": ("cond", "body"),
                    "ForFlow": ("iter", "body", "var", "schedule", "async_"),
                    "ImperativeForFlow": ("start", "step", "end", "body", "var", "schedule"),
                    "IfFlow": ("cond", "true_branch", "false_branch"),
                    "TryCatchFlow": ("catches", "body", "else_", "finally_"),
                    "PipelineFlow": ("stages",),
                    "AssignInstr": ("lhs", "rhs"),
                    "ExtractInstr": ("val", "field", "polymorphic"),
                    "InsertInstr": ("lhs", "field", "rhs", "polymorphic"),
                    "StackAllocInstr": ("ptr_type", "count"),
                    "YieldInInstr": ("suspend",),
                    "TernaryInstr": ("cond", "true_value", "false_value"),
                    "ReturnInstr": ("value",),
                    "YieldInstr": ("value", "final"),
                    "AwaitInstr": ("value", "generator"),
                    "ThrowInstr": ("value", "fallback"),
                    "FlowInstr": ("flow", "value"),
                    "IntType": ("len", "sign"),
                    "RecordType": ("fields",),
                    "RefType": ("contents", "polymorphic"),
                    "PointerType": ("base",),
                    "OptionalType": ("base",),
                    "GeneratorType": ("base",),
                    "VectorType": ("count", "base"),
                    "UnionType": ("types",),
                }
                fields.extend(
                    (key, getattr(node, key)) for key in schema.get(type(node).__name__, ())
                )
        return fields

    # C++: codon/cir/util/format.cpp:352
    def value(self, value, level=0, full=False, references=False):
        if value is None:
            return "", True
        if isinstance(value, Definition):
            return self.value(value.value, level, full=True)
        if isinstance(value, Reference):
            return self.reference(value.value, level)
        if isinstance(value, Node):
            value = value.get_actual()
            if references:
                return self.reference(value, level)
            seen = self.seen_types if isinstance(value, types.Type) else self.seen_nodes
            key = (
                value.name
                if isinstance(value, types.Type)
                else value.id
                if isinstance(value, (Var, Value))
                else id(value)
            )
            if key in seen or (isinstance(value, (Var, types.Type)) and not full):
                return self.reference(value, level)
            seen.add(key)
            return self.record(type(value).__name__, self.fields(value), level, value)
        if isinstance(value, ast.types.Type):
            return value.to_string(mode=2), True
        if isinstance(value, Enum):
            return value.name, True
        if isinstance(value, str):
            return (ast.nodes._quote_dump_string(value) if value else ""), True
        if isinstance(value, (int, float, bool)):
            return repr(value), True
        if isinstance(value, (list, tuple, dict)):
            if not value and not isinstance(value, tuple):
                return "", True
            child = level + bool(self.indent)
            if isinstance(value, dict):
                items = [
                    repr(key) + "=" + self.value(item, child, references=references)[0]
                    for key, item in sorted(value.items())
                ]
                braces = "{}"
            else:
                items = [
                    "None" if item is None else self.value(item, child, references=references)[0]
                    for item in value
                ]
                braces = "()" if isinstance(value, tuple) else "[]"
            text, inline = self.items(items, level=child)
            return braces[0] + text + braces[1], inline
        if isinstance(value, attribute.KeyValueAttribute):
            quote = ast.nodes._quote_dump_string
            items = ", ".join(
                f"{quote(str(key))}: {quote(str(item))}"
                for key, item in sorted(value.attributes.items())
            )
            return f"KeyValueAttribute(attributes={{{items}}})", True
        if isinstance(value, attribute.MemberAttribute):
            # Source positions are not part of ast.dump's data model.
            return self.record(
                "MemberAttribute", [("members", sorted(value.member_src_info))], level
            )
        if isinstance(value, types.Generic):
            return self.record(
                "Generic",
                [("value", Reference(value.value) if value.is_type() else value.value)],
                level,
            )
        if isinstance(value, ast.Node.SrcInfo):
            return "", True
        if type(value) is attribute.Attribute:
            return "", True
        if (
            isinstance(
                value,
                (
                    types.RuntimeInfo,
                    types.Field,
                    flow.Catch,
                    flow.Stage,
                    attribute.Attribute,
                    attribute.LiteralElement,
                    attribute.KeyValuePair,
                ),
            )
            or type(value).__name__ == "OMPSched"
        ):
            return self.record(
                type(value).__name__, list(vars(value).items()), level, references=references
            )
        raise TypeError(f"unsupported IR dump value {type(value).__name__}")


# C++: codon/cir/util/format.cpp:750
def iter_dump(node, annotate_fields=True, include_attributes=False, indent=None, types=False):
    """C++: formatChunks. Emit identical compact module text per declaration."""
    visitor = FormatVisitor(node, annotate_fields, include_attributes, indent, types)
    if not isinstance(node, Module) or indent:
        yield visitor.value(node, full=True)[0]
        return
    visitor.seen_nodes.add(id(node))
    yield "Module("
    first = True
    for key, value in visitor.fields(node):
        if key in ("types", "vars"):
            if not value:
                continue
            if not first:
                yield ", "
            first = False
            if annotate_fields:
                yield key + "="
            yield "["
            for idx, item in enumerate(value):
                if idx:
                    yield ", "
                yield visitor.value(item)[0]
            yield "]"
        else:
            text = visitor.value(value)[0]
            if not text:
                continue
            if not first:
                yield ", "
            first = False
            yield (key + "=" if annotate_fields else "") + text
    if include_attributes:
        text = visitor.attributes(node, 0)[0]
        if text:
            if not first:
                yield ", "
            yield text
    yield ")"


# C++: codon/cir/util/format.cpp:750
def dump(node, annotate_fields=True, include_attributes=False, indent=None, types=False):
    """Print CIR using ast.dump's fields, omissions, quoting and indentation.

    Shared/cyclic edges use TypeRef, VarRef and ValueRef records. Whole modules
    retain deterministic dump-local IDs and declaration/capture ordering.
    Attributes use AST enum names or their concrete IR attribute names; source
    locations and ownership/cache back-pointers are omitted, as in ast.dump.
    """
    return FormatVisitor(node, annotate_fields, include_attributes, indent, types).value(
        node, full=True
    )[0]


# C++: codon/cir/util/format.cpp:750
def format_ir(node, annotate_fields=True, include_attributes=False, indent=None, types=False):
    """Compatibility entry point for the CIR formatter."""
    return dump(node, annotate_fields, include_attributes, indent, types)
