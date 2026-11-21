# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from dataclasses import dataclass

from ... import BoolConst, CallInstr, FloatConst, IntConst, RecordType, VarValue, cast
from ...util.cloning import CloneVisitor
from ...util.irtools import get_func
from ..pass_ import OperatorPass


def is_foldable_constant(value):
    return value is not None and isinstance(value.get_actual(), (IntConst, FloatConst, BoolConst))


@dataclass(init=False, eq=False)
class ConstPropPass(OperatorPass):
    """C++: codon/cir/transform/folding/const_prop.cpp."""

    KEY = "core-folding-const-prop"

    reaching_def_key: str
    global_vars_key: str

    def __init__(self, reaching_def_key, global_vars_key):
        super().__init__()
        self.reaching_def_key, self.global_vars_key = reaching_def_key, global_vars_key

    def reaching_value(self, var, location):
        result = self.get_analysis_result(self.reaching_def_key)
        inspector = result.results.get(self.get_parent_func().id) if result else None
        if inspector is not None:
            definitions = inspector.get_reaching_definitions(var, location)
            if len(definitions) == 1 and definitions[0].known():
                return definitions[0].assignee
        return None

    def handle_VarValue(self, value):
        var = value.var
        if var.tls:
            return
        if var.global_:
            globals_ = self.get_analysis_result(self.global_vars_key)
            if globals_ is None:
                return
            definition = value.module.get_value(globals_.assignments.get(var.id, -1))
        else:
            definition = self.reaching_value(var, value)
        if is_foldable_constant(definition):
            value.replace_all(CloneVisitor(value.module).clone(definition))

    def handle_ExtractInstr(self, value):
        # Propagate constant tuple fields through a local variable.
        ref = cast(VarValue, value.val)
        typ = cast(RecordType, ref.type) if ref else None
        if not typ or ref.var.global_:
            return
        constructor = cast(CallInstr, self.reaching_value(ref.var, value))
        fn = get_func(constructor.callee) if constructor else None
        if not fn or fn.unmangled_name != "__new__":
            return
        tuple_ = cast(RecordType, fn.parent_type)
        if not tuple_ or tuple_.name != "Tuple":
            return
        index = typ.get_member_index(value.field)
        if 0 <= index < len(constructor.args) and is_foldable_constant(constructor.args[index]):
            value.replace_all(CloneVisitor(value.module).clone(constructor.args[index]))
