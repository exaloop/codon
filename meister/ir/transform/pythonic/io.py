# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from ... import CallInstr, RecordType, cast
from ...util.cloning import CloneVisitor
from ...util.irtools import call, get_func, get_return_type, get_stdlib_func
from ..pass_ import OperatorPass


def optimize_print(value):
    module = value.module
    inner = cast(CallInstr, value.front())
    fn = get_func(inner.callee) if inner else None
    if not fn or fn.unmangled_name != "__new__" or len(inner.args) != 1:
        return
    cat = cast(CallInstr, inner.front())
    cat_fn = get_func(cat.callee) if cat else None
    if not cat_fn or cat_fn.unmangled_name != "cat":
        return
    real_cat = module.get_or_realize_method(module.get_string_type(), "cat", [cat.front().type])
    if not real_cat or real_cat.id != cat_fn.id:
        return

    clone = CloneVisitor(module)
    args = [clone.clone(arg) for arg in value]
    types = [arg.type for arg in value]
    args[0] = clone.clone(cat.front())
    types[0] = args[0].type
    args[1] = module.get_string("")
    replacement = module.get_or_realize_func("print", types, [], "std.internal.builtin")
    if replacement:
        value.replace_all(call(replacement, args))


def optimize_write(value, write):
    if len(value.args) != 2:
        return
    module = value.module
    file, cat = value.args
    if not write.parent_type or not write.parent_type.is_type(file.type):
        return
    cat = cast(CallInstr, cat)
    if not cat or len(cat.args) != 1:
        return
    cat_fn = get_func(cat.callee)
    string = module.get_string_type()
    if (
        not cat_fn or cat_fn.unmangled_name != "cat" or not cat_fn.parent_type
        or not cat_fn.parent_type.is_type(string)
    ):
        return
    pieces = cast(RecordType, cat.front().type)
    if not pieces or not all(field.type.is_type(string) for field in pieces):
        return
    if not pieces.is_type(module.get_tuple_type([field.type for field in pieces])):
        return

    real_cat = module.get_or_realize_method(string, "cat", [pieces])
    if not real_cat or cat_fn.id != real_cat.id:
        return
    real_write = module.get_or_realize_method(file.type, "write", [file.type, cat.type])
    if not real_write or write.id != real_write.id:
        return

    clone = CloneVisitor(module)
    iterator = clone.clone(cat.front()).iter()
    if iterator is None:
        return
    args = [clone.clone(file), iterator]
    replacement = module.get_or_realize_method(
        file.type, "__file_write_gen__", [arg.type for arg in args]
    )
    if replacement and get_return_type(replacement).is_type(value.type):
        value.replace_all(call(replacement, args))


class IOCatOptimization(OperatorPass):
    """C++: codon/cir/transform/pythonic/io.cpp."""

    KEY = "core-pythonic-io-cat-opt"

    def handle_CallInstr(self, value):
        if get_stdlib_func(value.callee, "print", ""):
            optimize_print(value)
        elif (fn := get_func(value.callee)) and fn.unmangled_name == "write":
            optimize_write(value, fn)
