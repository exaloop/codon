# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ..bridge import List, dataclass
from .base import cast, replace_in_list
from .type import FuncType, MemberedType
from .value import Value


@dataclass(init=False, eq=False)
class Instr(Value):
    """C++: codon/cir/instr.h:19. Discrete operation in the context of a block."""

    # C++: codon/cir/instr.cpp:26
    def _get_type(self):
        return self.module.get_none_type()


@dataclass(init=False, eq=False)
class AssignInstr(Instr):
    """C++: codon/cir/instr.h:30, instr.cpp:33. Set a memory location."""

    lhs: object
    rhs: Value

    # C++: codon/cir/instr.h:45
    def __init__(self, lhs, rhs, name=""):
        """
        Constructs an assign instruction.
        @param lhs the left-hand side
        @param rhs the right-hand side
        @param field the field being set, may be empty
        @param name the instruction's name
        """
        super().__init__(name)
        self.lhs, self.rhs = lhs, rhs

    # C++: codon/cir/instr.h:65
    def _get_used_values(self):
        return [self.rhs]

    # C++: codon/cir/instr.cpp:30
    def _replace_used_value(self, identifier, new):
        if self.rhs.id == identifier:
            self.rhs = new
            return 1
        return 0

    # C++: codon/cir/instr.h:68
    def _get_used_variables(self):
        return [self.lhs]

    # C++: codon/cir/instr.cpp:38
    def _replace_used_variable(self, identifier, new):
        if self.lhs.id == identifier:
            self.lhs = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class ExtractInstr(Instr):
    """C++: codon/cir/instr.h:73, instr.cpp:53. Load a field from a value."""

    val: Value
    field: str
    polymorphic: bool

    # C++: codon/cir/instr.h:90
    def __init__(self, val, field, name="", polymorphic=False):
        """
        Constructs a load instruction.
        @param val the value being manipulated
        @param field the field
        @param name the instruction's name
        @param polymorphic true if the accessed class is polymorphic
        """
        super().__init__(name)
        self.val, self.field, self.polymorphic = val, field, polymorphic

    # C++: codon/cir/instr.cpp:48
    def _get_type(self):
        typ = cast(MemberedType, self.val.type)
        assert typ is not None, "not a membered type"
        return typ.get_member_type(self.field)

    # C++: codon/cir/instr.h:117
    def _get_used_values(self):
        return [self.val]

    # C++: codon/cir/instr.cpp:54
    def _replace_used_value(self, identifier, new):
        if self.val.id == identifier:
            self.val = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class InsertInstr(Instr):
    """C++: codon/cir/instr.h:122, instr.cpp:69. Set a field of a value."""

    lhs: Value
    field: str
    rhs: Value
    polymorphic: bool

    # C++: codon/cir/instr.h:142
    def __init__(self, lhs, field, rhs, name="", polymorphic=False):
        """
        Constructs a load instruction.
        @param lhs the value being manipulated
        @param field the field
        @param rhs the new value
        @param name the instruction's name
        @param polymorphic true if the accessed class is polymorphic
        """
        super().__init__(name)
        self.lhs, self.field, self.rhs, self.polymorphic = lhs, field, rhs, polymorphic

    # C++: codon/cir/instr.h:176
    def _get_type(self):
        return self.lhs.type

    # C++: codon/cir/instr.h:177
    def _get_used_values(self):
        return [self.lhs, self.rhs]

    # C++: codon/cir/instr.cpp:64
    def _replace_used_value(self, identifier, new):
        count = 0
        for field in ("lhs", "rhs"):
            if getattr(self, field).id == identifier:
                setattr(self, field, new)
                count += 1
        return count


@dataclass(init=False, eq=False)
class CallInstr(Instr):
    """C++: codon/cir/instr.h:182, instr.cpp:86. Call a function."""

    callee: Value
    args: List[Value]

    # C++: codon/cir/instr.h:196
    def __init__(self, callee, args=None, name=""):
        """
        Constructs a call instruction.
        @param callee the function
        @param args the arguments
        @param name the instruction's name
        """
        super().__init__(name)
        self.callee = callee
        self.args = [] if args is None else list(args)

    # C++: codon/cir/instr.h:214
    def __iter__(self):
        """
        @return an iterator to the first argument
        """
        return iter(self.args)

    # C++: codon/cir/instr.h:223
    def front(self):
        """
        @return a pointer to the first argument
        """
        return self.args[0]

    # C++: codon/cir/instr.h:225
    def back(self):
        """
        @return a pointer to the last argument
        """
        return self.args[-1]

    # C++: codon/cir/instr.cpp:79
    def _get_type(self):
        typ = cast(FuncType, self.callee.type)
        assert typ is not None, "not a function type"
        return typ.return_type

    # C++: codon/cir/instr.cpp:85
    def _get_used_values(self):
        return [*self.args, self.callee]

    # C++: codon/cir/instr.cpp:91
    def _replace_used_value(self, identifier, new):
        count = 0
        if self.callee.id == identifier:
            self.callee = new
            count += 1
        return count + replace_in_list(self.args, identifier, new)


@dataclass(init=False, eq=False)
class StackAllocInstr(Instr):
    """C++: codon/cir/instr.h:254, instr.cpp:112. Allocate an array on the stack."""

    ptr_type: object
    count: int

    # C++: codon/cir/instr.h:268
    def __init__(self, ptr_type, count, name=""):
        """
        Constructs a stack allocation instruction.
        @param ptrType the type of the array pointer
        @param count the number of elements
        @param name the name
        """
        super().__init__(name)
        self.ptr_type, self.count = ptr_type, count

    # C++: codon/cir/instr.h:286
    def _get_type(self):
        return self.ptr_type

    # C++: codon/cir/instr.h:287
    def _get_used_types(self):
        return [self.ptr_type]

    # C++: codon/cir/instr.cpp:103
    def _replace_used_type(self, name, new):
        if self.ptr_type.name == name:
            self.ptr_type = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class YieldInInstr(Instr):
    """C++: codon/cir/instr.h:292, instr.cpp:124. Python yield expression."""

    _type: object
    suspend: bool

    # C++: codon/cir/instr.h:306
    def __init__(self, typ, suspend=True, name=""):
        """
        Constructs a yield in instruction.
        @param type the type of the value being yielded in
        @param suspend whether to suspend
        @param name the instruction's name
        """
        super().__init__(name)
        self._type, self.suspend = typ, suspend

    # C++: codon/cir/instr.h:320
    def _get_type(self):
        return self._type

    # C++: codon/cir/instr.h:321
    def _get_used_types(self):
        return [self._type]

    # C++: codon/cir/instr.cpp:113
    def _replace_used_type(self, name, new):
        if self._type.name == name:
            self._type = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class TernaryInstr(Instr):
    """C++: codon/cir/instr.h:326, instr.cpp:153. Ternary operator."""

    cond: Value
    true_value: Value
    false_value: Value

    # C++: codon/cir/instr.h:343
    def __init__(self, cond, true_value, false_value, name=""):
        """
        Constructs a ternary instruction.
        @param cond the condition
        @param trueValue the true value
        @param falseValue the false value
        @param name the instruction's name
        """
        super().__init__(name)
        self.cond, self.true_value, self.false_value = cond, true_value, false_value

    # C++: codon/cir/instr.h:372
    def _get_type(self):
        return self.true_value.type

    # C++: codon/cir/instr.h:373
    def _get_used_values(self):
        return [self.cond, self.true_value, self.false_value]

    # C++: codon/cir/instr.cpp:141
    def _replace_used_value(self, identifier, new):
        count = 0
        for field in ("cond", "true_value", "false_value"):
            if getattr(self, field).id == identifier:
                setattr(self, field, new)
                count += 1
        return count


@dataclass(init=False, eq=False)
class ControlFlowInstr(Instr):
    """C++: codon/cir/instr.h:380. Base for control flow instructions."""


@dataclass(init=False, eq=False)
class BreakInstr(ControlFlowInstr):
    """C++: codon/cir/instr.h:387, instr.cpp:178. Break, null loop means immediate ancestor."""

    loop: Value | None

    # C++: codon/cir/instr.h:398
    def __init__(self, loop=None, name=""):
        """
        Constructs a break instruction.
        @param loop the loop being broken, nullptr if immediate ancestor
        @param name the instruction's name
        """
        super().__init__(name)
        self.loop = loop

    # C++: codon/cir/instr.cpp:162
    def _get_used_values(self):
        return [self.loop] if self.loop is not None else []

    # C++: codon/cir/instr.cpp:168
    def _replace_used_value(self, identifier, new):
        from .flow import Flow

        if self.loop is not None and self.loop.id == identifier:
            flow = cast(Flow, new)
            assert flow is not None, "not a flow"
            self.loop = flow
            return 1
        return 0


@dataclass(init=False, eq=False)
class ContinueInstr(ControlFlowInstr):
    """C++: codon/cir/instr.h:412, instr.cpp:197. Continue, null loop means immediate ancestor."""

    loop: Value | None

    # C++: codon/cir/instr.h:423
    def __init__(self, loop=None, name=""):
        """
        Constructs a continue instruction.
        @param loop the loop being continued, nullptr if immediate ancestor
        @param name the instruction's name
        """
        super().__init__(name)
        self.loop = loop

    # C++: codon/cir/instr.cpp:180
    def _get_used_values(self):
        return [self.loop] if self.loop is not None else []

    # C++: codon/cir/instr.cpp:186
    def _replace_used_value(self, identifier, new):
        from .flow import Flow

        if self.loop is not None and self.loop.id == identifier:
            loop = cast(Flow, new)
            assert loop is not None, "not a flow"
            self.loop = loop
            return 1
        return 0


@dataclass(init=False, eq=False)
class ReturnInstr(ControlFlowInstr):
    """C++: codon/cir/instr.h:437, instr.cpp:216. Return statement."""

    value: Value | None

    # C++: codon/cir/instr.h:445
    def __init__(self, value=None, name=""):
        super().__init__(name)
        self.value = value

    # C++: codon/cir/instr.cpp:198
    def _get_used_values(self):
        return [self.value] if self.value is not None else []

    # C++: codon/cir/instr.cpp:204
    def _replace_used_value(self, identifier, new):
        if self.value is not None and self.value.id == identifier:
            self.value = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class YieldInstr(Instr):
    """C++: codon/cir/instr.h:462, instr.cpp:234. Yield, optionally final."""

    value: Value | None
    final: bool

    # C++: codon/cir/instr.h:472
    def __init__(self, value=None, final=False, name=""):
        super().__init__(name)
        self.value, self.final = value, final

    # C++: codon/cir/instr.cpp:215
    def _get_used_values(self):
        return [self.value] if self.value is not None else []

    # C++: codon/cir/instr.cpp:221
    def _replace_used_value(self, identifier, new):
        if self.value is not None and self.value.id == identifier:
            self.value = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class AwaitInstr(Instr):
    """C++: codon/cir/instr.h:495, instr.cpp:136. Await, with explicit result type."""

    value: Value
    _type: object
    generator: bool

    # C++: codon/cir/instr.h:507
    def __init__(self, value, typ, generator=False, name=""):
        super().__init__(name)
        self.value, self._type, self.generator = value, typ, generator

    # C++: codon/cir/instr.h:531
    def _get_type(self):
        return self._type

    # C++: codon/cir/instr.h:533
    def _get_used_types(self):
        return [self._type]

    # C++: codon/cir/instr.h:532
    def _get_used_values(self):
        return [self.value]

    # C++: codon/cir/instr.cpp:123
    def _replace_used_value(self, identifier, new):
        if self.value.id == identifier:
            self.value = new
            return 1
        return 0

    # C++: codon/cir/instr.cpp:131
    def _replace_used_type(self, name, new):
        if self._type.name == name:
            self._type = new
            return 1
        return 0


@dataclass(init=False, eq=False)
class ThrowInstr(Instr):
    """C++: codon/cir/instr.h:538, instr.cpp:251. Throw with optional bare-throw fallback."""

    value: Value | None
    # Must not return normally; ignored when an explicit value is supplied.
    fallback: Value | None

    # C++: codon/cir/instr.h:549
    def __init__(self, value=None, name=""):
        super().__init__(name)
        self.value, self.fallback = value, None

    # C++: codon/cir/instr.cpp:231
    def _get_used_values(self):
        return [value for value in (self.value, self.fallback) if value is not None]

    # C++: codon/cir/instr.cpp:240
    def _replace_used_value(self, identifier, new):
        count = 0
        for field in ("value", "fallback"):
            value = getattr(self, field)
            if value is not None and value.id == identifier:
                setattr(self, field, new)
                count += 1
        return count


@dataclass(init=False, eq=False)
class FlowInstr(Instr):
    """C++: codon/cir/instr.h:574, instr.cpp:273. A flow and its output value."""

    flow: Value
    value: Value

    # C++: codon/cir/instr.h:588
    def __init__(self, flow, value, name=""):
        """
        Constructs a flow value.
        @param flow the flow
        @param val the output value
        @param name the name
        """
        super().__init__(name)
        self.flow, self.value = flow, value

    # C++: codon/cir/instr.h:608
    def _get_type(self):
        return self.value.type

    # C++: codon/cir/instr.h:609
    def _get_used_values(self):
        return [self.flow, self.value]

    # C++: codon/cir/instr.cpp:255
    def _replace_used_value(self, identifier, new):
        from .flow import Flow

        count = 0
        if self.flow.id == identifier:
            flow = cast(Flow, new)
            assert flow is not None, "not a flow"
            self.flow = flow
            count += 1
        if self.value.id == identifier:
            self.value = new
            count += 1
        return count
