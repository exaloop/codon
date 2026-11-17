# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ....bridge import List, dataclass
from ... import (
    ExtractInstr,
    FlowInstr,
    IntType,
    MemberedType,
    RecordType,
    SeriesFlow,
    StringConst,
    VarValue,
    cast,
)
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, get_return_type, make_tuple, make_var, tuple_get
from ..pass_ import OperatorPass

FORMAT_FLAG_ALT = 1
FORMAT_FLAG_ZERO = 2
FORMAT_FLAG_SPACE = 8
FORMAT_FLAG_PLUS = 16


@dataclass(init=False, eq=False)
class Directive:
    """C++: codon/cir/transform/pythonic/format.cpp:25. Parsed format directive, with C++ defaults."""

    flags: int
    width: int
    precision: int
    conversion: int
    alignment: int
    fill: int
    grouping: int
    locale: bool
    coerce_negative_zero: bool
    sign_minus: bool

    # C++ API: codon/cir/transform/pythonic/format.cpp:25
    def __init__(self):
        self.flags = self.width = self.conversion = self.alignment = self.grouping = 0
        self.precision, self.fill = -1, 0x20
        self.locale = self.coerce_negative_zero = self.sign_minus = False


@dataclass(init=False, eq=False)
class FieldAccess:
    """C++: codon/cir/transform/pythonic/format.cpp:38. Member, integer element or string element access."""

    MEMBER = 0
    INTEGER_ELEMENT = 1
    STRING_ELEMENT = 2
    kind: int
    name: str
    index: int

    # C++ API: codon/cir/transform/pythonic/format.cpp:38
    def __init__(self, kind, name, index=0):
        self.kind, self.name, self.index = kind, name, index


@dataclass(init=False, eq=False)
class Part:
    """C++: codon/cir/transform/pythonic/format.cpp:46. Literal text or a parsed replacement field."""

    literal: str
    index: int
    accesses: List[FieldAccess]
    field_conversion: int
    directive: Directive

    # C++ API: codon/cir/transform/pythonic/format.cpp:46
    def __init__(self):
        self.literal, self.index, self.accesses = "", -1, []
        self.field_conversion, self.directive = 0, Directive()

    # C++: codon/cir/transform/pythonic/format.cpp:53
    def is_field(self):
        return self.index >= 0


# C++: codon/cir/transform/pythonic/format.cpp:56
def is_alignment(codepoint):
    return codepoint in (ord("<"), ord(">"), ord("^"), ord("="))


# C++: codon/cir/transform/pythonic/format.cpp:58
def decode_codepoint(value, position):
    """C++: format.cpp:60. Strict UTF-8 decoder; reject overlongs/surrogates."""
    if position >= len(value):
        return False, position, 0
    first = value[position]
    position += 1
    if first < 0x80:
        return True, position, first
    if first & 0xE0 == 0xC0:
        count, result, minimum = 1, first & 0x1F, 0x80
    elif first & 0xF0 == 0xE0:
        count, result, minimum = 2, first & 0x0F, 0x800
    elif first & 0xF8 == 0xF0:
        count, result, minimum = 3, first & 0x07, 0x10000
    else:
        return False, position, 0
    for _ in range(count):
        if position >= len(value):
            return False, position, 0
        next_ = value[position]
        position += 1
        if next_ & 0xC0 != 0x80:
            return False, position, 0
        result = (result << 6) | (next_ & 0x3F)
    if result < minimum or result > 0x10FFFF or 0xD800 <= result <= 0xDFFF:
        return False, position, 0
    return True, position, result


# C++: codon/cir/transform/pythonic/format.cpp:103
def append_digit(value, digit):
    """C++: format.cpp:105. Accumulate without signed 64-bit overflow."""
    if value > ((1 << 63) - 1 - digit) // 10:
        return None
    return value * 10 + digit


# C++: codon/cir/transform/pythonic/format.cpp:110
def parse_directive(spec, directive):
    """C++: format.cpp:112. Parse one static format specifier."""
    try:
        spec = spec.encode("utf-8", "surrogateescape")
    except UnicodeEncodeError:
        return False
    position = 0
    if spec:
        valid, end, first = decode_codepoint(spec, 0)
        if not valid:
            return False
        if end < len(spec) and is_alignment(spec[end]):
            directive.fill, directive.alignment, position = first, spec[end], end + 1
        elif is_alignment(first):
            directive.alignment, position = first, end

    def take(char):
        nonlocal position
        if position < len(spec) and spec[position] == ord(char):
            position += 1
            return True
        return False

    if take("-"):
        directive.sign_minus = True
    elif take(" "):
        directive.flags |= FORMAT_FLAG_SPACE
    elif take("+"):
        directive.flags |= FORMAT_FLAG_PLUS
    directive.coerce_negative_zero = take("z")
    if take("#"):
        directive.flags |= FORMAT_FLAG_ALT
    if take("0"):
        directive.flags |= FORMAT_FLAG_ZERO
    while position < len(spec) and ord("0") <= spec[position] <= ord("9"):
        value = append_digit(directive.width, spec[position] - ord("0"))
        position += 1
        if value is None:
            return False
        directive.width = value
    if position < len(spec) and spec[position] in (ord(","), ord("_")):
        directive.grouping = spec[position]
        position += 1
    if take("."):
        directive.precision = 0
        while position < len(spec) and ord("0") <= spec[position] <= ord("9"):
            value = append_digit(directive.precision, spec[position] - ord("0"))
            position += 1
            if value is None:
                return False
            directive.precision = value
    if position == len(spec):
        return True
    conversion = spec[position]
    position += 1
    if position != len(spec):
        return False
    if conversion == ord("n"):
        directive.locale = True
        return True
    if conversion not in b"bsc%diuoxXeEfFgG":
        return False
    directive.conversion = conversion
    return True


# C++: codon/cir/transform/pythonic/format.cpp:185
def parse_index(text):
    if not text:
        return None
    index = 0
    for char in text:
        if not "0" <= char <= "9":
            return None
        index = append_digit(index, ord(char) - ord("0"))
        if index is None:
            return None
    return index


# C++: codon/cir/transform/pythonic/format.cpp:197
def parse_field(text, automatic_index, numbering_mode, part):
    """C++: format.cpp:205. Return success and updated numbering state."""
    position = 0
    while position < len(text) and text[position] not in ".[!:":
        position += 1
    index_text = text[:position]
    if not index_text:
        if numbering_mode == 2:
            return False, automatic_index, numbering_mode
        numbering_mode, part.index = 1, automatic_index
        automatic_index += 1
    else:
        index = parse_index(index_text)
        if numbering_mode == 1 or index is None:
            return False, automatic_index, numbering_mode
        numbering_mode, part.index = 2, index
    while position < len(text) and text[position] in ".[":
        if text[position] == ".":
            position += 1
            start = position
            while position < len(text) and text[position] not in ".[!:]":
                position += 1
            if position == start:
                return False, automatic_index, numbering_mode
            part.accesses.append(FieldAccess(FieldAccess.MEMBER, text[start:position]))
            continue
        start = position + 1
        end = text.find("]", start)
        if end < 0 or end == start:
            return False, automatic_index, numbering_mode
        key = text[start:end]
        index = parse_index(key)
        part.accesses.append(
            FieldAccess(FieldAccess.INTEGER_ELEMENT, "", index)
            if index is not None
            else FieldAccess(FieldAccess.STRING_ELEMENT, key)
        )
        position = end + 1
    if position < len(text) and text[position] == "!":
        position += 1
        if position >= len(text) or text[position] not in "sra":
            return False, automatic_index, numbering_mode
        part.field_conversion = ord(text[position])
        position += 1
    if position == len(text):
        return True, automatic_index, numbering_mode
    if text[position] != ":":
        return False, automatic_index, numbering_mode
    spec = text[position + 1 :]
    if "{" in spec or "}" in spec:
        return False, automatic_index, numbering_mode
    return parse_directive(spec, part.directive), automatic_index, numbering_mode


# C++: codon/cir/transform/pythonic/format.cpp:264
def parse_format(text):
    """C++: format.cpp:280. Parse a literal format; leave dynamic fields untouched."""
    parts, literal, automatic_index, numbering_mode, position = [], "", 0, 0, 0
    while position < len(text):
        char = text[position]
        if char == "{":
            if position + 1 < len(text) and text[position + 1] == "{":
                literal += "{"
                position += 2
                continue
            if literal:
                part = Part()
                part.literal = literal
                parts.append(part)
                literal = ""
            end = text.find("}", position + 1)
            if end < 0:
                return None
            field = text[position + 1 : end]
            if "{" in field:
                return None
            part = Part()
            success, automatic_index, numbering_mode = parse_field(
                field, automatic_index, numbering_mode, part
            )
            if not success:
                return None
            parts.append(part)
            position = end + 1
            continue
        if char == "}":
            if position + 1 >= len(text) or text[position + 1] != "}":
                return None
            literal += "}"
            position += 2
            continue
        literal += char
        position += 1
    if literal:
        part = Part()
        part.literal = literal
        parts.append(part)
    return parts


# C++: codon/cir/transform/pythonic/format.cpp:322
def supports_parsed_formatting(typ, module):
    return (
        cast(IntType, typ) is not None
        or typ.is_type(module.get_float_type())
        or typ.is_type(module.get_string_type())
    )


# C++: codon/cir/transform/pythonic/format.cpp:327
def parsed_helper_types(value_type, module):
    return [
        value_type,
        *[module.get_int_type() for _ in range(8)],
        *[module.get_bool_type() for _ in range(3)],
    ]


# C++: codon/cir/transform/pythonic/format.cpp:336
def parsed_helper_args(value, part, module):
    directive = part.directive
    return [
        value,
        *[
            module.get_int(item)
            for item in (
                part.field_conversion,
                directive.conversion,
                directive.flags,
                directive.width,
                directive.precision,
                directive.alignment,
                directive.fill,
                directive.grouping,
            )
        ],
        module.get_bool(directive.locale),
        module.get_bool(directive.coerce_negative_zero),
        module.get_bool(directive.sign_minus),
    ]


# C++: codon/cir/transform/pythonic/format.cpp:352
def get_parsed_helper(value_type, module):
    return module.get_or_realize_func(
        "_format_parsed", parsed_helper_types(value_type, module), [], "std.internal.format"
    )


@dataclass(init=False, eq=False)
class ResolvedPart:
    """C++: codon/cir/transform/pythonic/format.cpp:358. Resolved final type and element accessors."""

    type: object | None
    element_accessors: List[object | None]

    # C++ API: codon/cir/transform/pythonic/format.cpp:358
    def __init__(self):
        self.type, self.element_accessors = None, []


# C++: codon/cir/transform/pythonic/format.cpp:364
def resolve_part(part, element_types, module):
    if part.index < 0 or part.index >= len(element_types):
        return None
    resolved = ResolvedPart()
    resolved.type = element_types[part.index]
    for access in part.accesses:
        if access.kind == FieldAccess.MEMBER:
            membered = cast(MemberedType, resolved.type)
            if not membered:
                return None
            resolved.type = membered.get_member_type(access.name)
            if resolved.type is None:
                return None
            resolved.element_accessors.append(None)
            continue
        if access.kind == FieldAccess.INTEGER_ELEMENT:
            record = cast(RecordType, resolved.type)
            if record and record.name == "Tuple":
                resolved.type = record.get_member_type(f"item{access.index + 1}")
                if resolved.type is None:
                    return None
                resolved.element_accessors.append(None)
                continue
        key_type = (
            module.get_int_type()
            if access.kind == FieldAccess.INTEGER_ELEMENT
            else module.get_string_type()
        )
        getitem = module.get_or_realize_method(
            resolved.type, module.GETITEM_MAGIC_NAME, [resolved.type, key_type]
        )
        if not getitem:
            return None
        resolved.type = get_return_type(getitem)
        resolved.element_accessors.append(getitem)
    return resolved


# C++: codon/cir/transform/pythonic/format.cpp:404
def apply_accesses(value, part, resolved, module):
    for access, accessor in zip(part.accesses, resolved.element_accessors):
        if access.kind == FieldAccess.MEMBER:
            value = module.nr(ExtractInstr, value, access.name)
        elif accessor is None:
            value = tuple_get(value, access.index)
        else:
            key = (
                module.get_int(access.index)
                if access.kind == FieldAccess.INTEGER_ELEMENT
                else module.get_string(access.name)
            )
            value = call(accessor, [value, key])
    return value


@dataclass(init=False, eq=False)
class FormattingOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/format.h:13. Literal formatting to parsed formatter calls."""

    KEY = "core-pythonic-formatting-opt"

    # C++: codon/cir/transform/pythonic/format.cpp:426
    def handle_CallInstr(self, value):
        module = value.module
        fn = get_func(value.callee)
        if not fn or not fn.parent_type:
            return
        name = fn.unmangled_name
        if (
            name == "format"
            and fn.parent_type.is_type(module.get_string_type())
            and len(value.args) == 2
        ):
            format_ = cast(StringConst, value.args[0])
            tuple_type = cast(RecordType, value.args[-1].type)
            if format_ is None or tuple_type is None:
                return
            parsed = parse_format(format_.val)
            if parsed is None:
                return
            element_types = [field.type for field in tuple_type]
            resolved_parts = [ResolvedPart() for _ in parsed]
            helpers = []
            for idx, part in enumerate(parsed):
                if not part.is_field():
                    helpers.append(None)
                    continue
                resolved = resolve_part(part, element_types, module)
                if resolved is None or not supports_parsed_formatting(resolved.type, module):
                    return
                resolved_parts[idx] = resolved
                helper = get_parsed_helper(resolved.type, module)
                if helper is None:
                    return
                helpers.append(helper)
            piece_types = [module.get_string_type() for _ in parsed]
            cat = None
            if len(piece_types) > 1:
                pieces_type = module.get_tuple_type(piece_types)
                cat = module.get_or_realize_method(module.get_string_type(), "cat", [pieces_type])
                if cat is None:
                    return
            prefix = module.nr(SeriesFlow)
            visitor = CloneVisitor(module)
            tuple_var = make_var(visitor.clone(value.args[-1]), prefix, self.get_parent_func())
            pieces = []
            for idx, part in enumerate(parsed):
                if not part.is_field():
                    pieces.append(module.get_string(part.literal))
                    continue
                item = tuple_get(module.nr(VarValue, tuple_var), part.index)
                item = apply_accesses(item, part, resolved_parts[idx], module)
                pieces.append(call(helpers[idx], parsed_helper_args(item, part, module)))
            result = (
                module.get_string("")
                if not pieces
                else pieces[0]
                if len(pieces) == 1
                else call(cat, [make_tuple(pieces, module)])
            )
            replacement = module.nr(FlowInstr, prefix, result)
            replacement.info = value.info
            value.replace_all(replacement)
            return
        if (
            name == "__format__"
            and len(value.args) == 2
            and supports_parsed_formatting(value.args[0].type, module)
        ):
            spec = cast(StringConst, value.args[-1])
            if spec is None:
                return
            directive = Directive()
            if not parse_directive(spec.val, directive):
                return
            part = Part()
            part.index, part.directive = 0, directive
            helper = get_parsed_helper(value.args[0].type, module)
            if helper is None:
                return
            visitor = CloneVisitor(module)
            replacement = call(
                helper, parsed_helper_args(visitor.clone(value.args[0]), part, module)
            )
            replacement.info = value.info
            value.replace_all(replacement)
