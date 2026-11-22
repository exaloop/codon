"""Exercise the CIR extension hooks available to Python DSL nodes."""

import pytest

from .. import ir
from ..ir.dsl import CustomConst, CustomFlow, CustomInstr
from ..ir.util.cloning import CloneVisitor
from ..ir.util.matching import match
from ..ir.util.side_effect import SideEffectStatus


class DSLConstant(CustomConst):
    def __init__(self, typ, data, name=""):
        super().__init__(typ, name)
        self.data = data

    def do_clone(self, visitor):
        return visitor.make(self, self.type, self.data)

    def match(self, other):
        return self.type.is_type(other.type) and self.data == other.data


@pytest.mark.parametrize("base", [CustomInstr, CustomFlow])
def test_dsl_clone_and_match_hooks(base):
    class DSLNode(base):
        def __init__(self, operand, name=""):
            super().__init__(name)
            self.operand = operand

        def _get_used_values(self):
            return [self.operand]

        def do_clone(self, visitor):
            return visitor.make(self, visitor.clone(self.operand))

        def match(self, other):
            return match(self.operand, other.operand)

    module = ir.Module(None)
    constant = module.nr(DSLConstant, module.get_int_type(), 17, "custom")
    instruction = module.nr(DSLNode, constant)
    clone = CloneVisitor(module).clone(instruction)
    assert isinstance(clone, DSLNode)
    assert clone is not instruction
    assert clone.operand is not constant
    assert clone.operand.name == "custom"
    assert clone.operand.type is constant.type
    assert clone.get_used_values() == [clone.operand]
    assert match(instruction, clone)
    clone.operand.data = 18
    assert not match(instruction, clone)
    assert instruction.get_side_effect_status() == SideEffectStatus.UNKNOWN
