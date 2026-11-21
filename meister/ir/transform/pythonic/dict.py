# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ... import CallInstr, Const, VarValue, cast
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func
from ...util.matching import match
from ..pass_ import OperatorPass


class DictArithmeticOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/dict.cpp."""

    KEY = "core-pythonic-dict-arithmetic-opt"

    def handle_CallInstr(self, value):
        set_func = get_func(value.callee)
        if not set_func or set_func.unmangled_name != "__setitem__" or len(value.args) != 3:
            return
        dictionary, key, operation = value.args
        if cast(CallInstr, dictionary) or cast(CallInstr, key):
            return
        operation = cast(CallInstr, operation)
        if not operation or len(operation.args) != 2:
            return
        op_func = get_func(operation.callee)
        get = cast(CallInstr, operation.front())
        if not op_func or not op_func.parent_type or not get:
            return
        module = value.module
        if not any(
            op_func.parent_type.is_type(typ)
            for typ in (module.get_int_type(), module.get_float_type())
        ):
            return
        get_func_ = get_func(get.callee)
        if not get_func_ or len(get.args) < 2:
            return
        if any(cast(CallInstr, arg) for arg in get.args[:2]):
            return
        default = None
        if get_func_.unmangled_name == "get" and len(get.args) == 3:
            default = cast(Const, get.args[2])
        elif get_func_.unmangled_name != "__getitem__" or len(get.args) != 2:
            return
        if not (match(dictionary, get.args[0]) and match(key, get.args[1])):
            return

        second = operation.back()
        name = "__dict_do_op__" if default else "__dict_do_op_throws__"
        types = [dictionary.type, key.type, second.type]
        if default:
            types.append(default.type)
        types.append(op_func.type)
        replacement = module.get_or_realize_method(dictionary.type, name, types)
        if replacement:
            clone = CloneVisitor(module)
            args = [clone.clone(arg) for arg in (dictionary, key, second)]
            if default:
                args.append(clone.clone(default))
            if len(args) != len(replacement.args):
                args.append(module.n(VarValue, value, op_func))
            value.replace_all(call(replacement, args))
