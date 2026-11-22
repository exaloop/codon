"""Ports of the GoogleTest CIR node, type, and matching checks."""

import pytest

from .. import ast, ir
from ..ir.util.cloning import CloneVisitor
from ..ir.util.irtools import (
    call, get_func, get_var, is_export, is_gpu_kernel, set_export, virtual_call,
)
from ..ir.util.matching import AnyValue, match
from ..ir.util.operator import Operator
from ..ir.util.side_effect import SideEffectStatus
from ..ir.util.visitor import Visitor


@pytest.fixture
def module():
    return ir.Module(None)


def test_node_replacement_delegates(module):
    original = module.get_int(1)
    replacement = module.nr(ir.BoolConst, False, module.get_bool_type(), "replacement")
    attribute = ir.KeyValueAttribute()
    replacement.set_attribute(attribute)
    assert ir.cast(ir.Value, original) is original
    assert ir.cast(ir.Const, original) is original
    assert ir.cast(ir.Flow, original) is None
    assert not original.has_replacement()
    assert original.has_attribute(ir.SrcInfoAttribute)
    assert not original.has_attribute(ir.KeyValueAttribute)
    assert original.name == ""

    original.replace_all(replacement)
    assert original.has_replacement()
    assert ir.cast(ir.IntConst, original) is None
    assert ir.cast(ir.BoolConst, original) is replacement
    assert original.name == "replacement"
    assert original.get_attribute(ir.KeyValueAttribute) is attribute
    assert original.type is module.get_bool_type()
    assert original.get_used_types() == [module.get_bool_type()]

    class VisitorProbe(Visitor):
        def visit_BoolConst(self, node):
            return node

    assert original.accept(VisitorProbe()) is replacement


def test_nonreplaceable_node(module):
    original = module.get_int(1)
    original.replaceable = False
    with pytest.raises(AssertionError):
        original.replace_all(original)


def test_replace_used_type_through_replacement(module):
    integer, floating = module.get_int_type(), module.get_float_type()
    original, replacement = module.nr(ir.Var, integer), module.nr(ir.Var, floating)
    old_id = original.id
    original.replace_all(replacement)
    assert original.type is floating
    assert original.get_used_types() == [floating]
    assert original.replace_used_type(floating, integer) == 1
    assert original.id == replacement.id != old_id
    assert replacement.type is integer


@pytest.mark.parametrize("factory,value", [
    ("get_int", 1), ("get_float", 1.5), ("get_bool", False), ("get_string", "abc"),
])
def test_constant_clone_and_type_replacement(module, factory, value):
    node = getattr(module, factory)(value)
    clone = CloneVisitor(module).clone(node)
    assert clone is not node
    assert clone.type is node.type
    assert clone.val == value
    assert match(node, clone)
    assert node.get_used_types() == [node.type]
    new = module.get_float32_type()
    assert node.replace_used_type(node.type, new) == 1
    assert node.type is new


@pytest.mark.parametrize("ref", [False, True], ids=["record", "reference"])
def test_membered_type_queries(module, ref):
    integer, floating = module.get_int_type(), module.get_float_type()
    record = module.nr(ir.RecordType, "record", [integer])
    membered = module.nr(ir.RefType, "reference", record) if ref else record
    assert membered.front().name == "1"
    assert membered.get_member_type("1") is integer
    assert membered.get_member_index("missing") == -1
    assert membered.get_member_type("missing") is None
    membered.realize([integer, floating], ["first", "last"])
    assert [(field.name, field.type) for field in membered] == [
        ("first", integer), ("last", floating),
    ]
    assert membered.get_member_index("last") == 1


def test_union_identity(module):
    integer, floating = module.get_int_type(), module.get_float_type()
    numeric = module.unsafe_get_union_type([integer, floating])
    single = module.unsafe_get_union_type([integer])
    assert numeric.name == ir.UnionType.get_instance_name([integer, floating])
    assert single.name and single.name != numeric.name
    assert numeric is module.unsafe_get_union_type([integer, floating])
    assert single is module.unsafe_get_union_type([integer])
    assert module.unsafe_get_pointer_type(numeric) is not module.unsafe_get_pointer_type(single)


@pytest.mark.parametrize("node_type", [ir.Var, ir.VarValue, ir.PointerValue])
def test_variable_matching(module, node_type):
    first = module.nr(ir.Var, module.get_int_type())
    second = module.nr(ir.Var, module.get_int_type())
    left, right = (
        (first, second) if node_type is ir.Var else
        (module.nr(node_type, first), module.nr(node_type, second))
    )
    assert match(left, right)
    second.type = module.get_float_type()
    assert not match(left, right)


def test_any_value_matches_variable_value(module):
    value = module.nr(ir.VarValue, module.nr(ir.Var, module.get_int_type()))
    wildcard = module.nr(AnyValue)
    assert match(value, wildcard)
    assert match(wildcard, value)


@pytest.mark.parametrize("function_type", [
    ir.BodiedFunc, ir.ExternalFunc, ir.InternalFunc, ir.LLVMFunc,
])
def test_function_matching(module, function_type):
    signature = module.unsafe_get_func_type("signature", module.get_int_type(), [])
    function = module.nr(function_type, "f")
    function.realize(signature, [])
    function.unmangled_name = "underlying"
    clone = CloneVisitor(module).force_clone(function)
    assert clone is not function
    assert clone.unmangled_name == "underlying"
    assert match(function, clone)
    if function_type is ir.BodiedFunc:
        clone.jit = True
    elif function_type is ir.ExternalFunc:
        clone.unmangled_name = "different"
    elif function_type is ir.InternalFunc:
        clone.set_intrinsic(ir.Intrinsic.SIZEOF, module.get_int_type())
    else:
        clone.llvm_literals = [ir.Generic(1)]
    assert not match(function, clone)


def test_module_stores_and_builtin_variables(module):
    integer = module.get_int_type()
    for node, store in [
        (module.nr(ir.OptionalType, integer), module.types),
        (module.get_int(1), module.values),
        (module.nr(ir.Var, integer), module.vars),
    ]:
        assert node.module is module
        assert node in store
        count = len(store)
        module.remove(node)
        assert node not in store
        assert len(store) == count - 1

    main = module.main_func
    main.realize(module.unsafe_get_func_type("main_signature", integer, []), [])
    assert list(main.type) == []
    assert not main.replaceable
    assert not module.argc_var.replaceable
    assert not module.argv_var.replaceable
    assert module.argc_var.type is integer
    assert module.argv_var.type is module.unsafe_get_pointer_type(module.get_string_type())

    record = module.unsafe_get_membered_type("TestType")
    assert isinstance(record, ir.RecordType)
    assert module.get_type("TestType") is record
    module.remove(record)
    ref = module.unsafe_get_membered_type("TestType", ref=True)
    assert isinstance(ref, ir.RefType)
    assert module.get_type("TestType") is ref


def test_intrinsic_factory_rolls_back_with_arena(module):
    integer, boolean = module.get_int_type(), module.get_bool_type()
    query = module.get_intrinsic(ir.Intrinsic.SIZEOF, integer)
    assert query.has_valid_intrinsic_signature()
    assert query.is_type_query()
    assert query.global_
    assert query is module.get_intrinsic(ir.Intrinsic.SIZEOF, integer)
    assert module.nr(ir.CallInstr, module.nr(ir.VarValue, query)).type is integer
    assert query.get_intrinsic_side_effect_status() == SideEffectStatus.PURE
    assert not query.intrinsic_derives_from_arguments()
    atomic = module.get_intrinsic(ir.Intrinsic.IS_ATOMIC, integer)
    assert atomic is not query
    assert atomic.type.return_type is boolean
    assert atomic.has_valid_intrinsic_signature()

    query.set_intrinsic(ir.Intrinsic.SIZEOF, boolean)
    replacement = module.get_intrinsic(ir.Intrinsic.SIZEOF, integer)
    assert replacement is not query
    module.remove(replacement)
    new = module.get_intrinsic(ir.Intrinsic.SIZEOF, integer)
    assert new.id != replacement.id

    module.push_arena()
    temporary = module.unsafe_get_membered_type("temporary")
    temporary_id = module.get_intrinsic(ir.Intrinsic.SIZEOF, temporary).id
    module.pop_arena()
    assert module.get_var(temporary_id) is None
    assert module.get_type("temporary") is None
    assert module.get_intrinsic(ir.Intrinsic.SIZEOF, integer) is new
    temporary = module.unsafe_get_membered_type("temporary")
    assert module.get_intrinsic(ir.Intrinsic.SIZEOF, temporary).id != temporary_id


def test_intrinsic_signatures_and_effects(module):
    integer = module.get_int_type()
    ref = module.unsafe_get_membered_type("intrinsic.reference", ref=True)
    record = module.unsafe_get_membered_type("intrinsic.record")
    generator = module.unsafe_get_generator_type(integer)
    pointer = module.unsafe_get_pointer_type(integer)
    for kind, typ, result, arguments, status, derives in [
        (ir.Intrinsic.ALLOCATE, ref, ref, [], SideEffectStatus.NO_SIDE_EFFECT, False),
        (ir.Intrinsic.RECORD_NEW, record, record, [], SideEffectStatus.PURE, True),
        (ir.Intrinsic.GENERATOR_PROMISE, generator, pointer, [generator],
         SideEffectStatus.PURE, True),
        (ir.Intrinsic.TYPEINFO_LOOKUP, None, ref, [integer], SideEffectStatus.UNKNOWN, False),
    ]:
        signature = module.unsafe_get_func_type(f"{kind.name}.signature", result, arguments)
        intrinsic = module.get_intrinsic(kind, typ, signature)
        assert intrinsic.has_valid_intrinsic_signature()
        assert intrinsic.get_intrinsic_side_effect_status() == status
        assert intrinsic.intrinsic_derives_from_arguments() is derives

    lookup = module.get_intrinsic(ir.Intrinsic.TYPEINFO_LOOKUP, None, signature)
    lookup.set_intrinsic(ir.Intrinsic.NONE)
    assert not lookup.has_valid_intrinsic_signature()
    lookup.set_intrinsic(ir.Intrinsic.SIZEOF, ref)
    assert not lookup.has_valid_intrinsic_signature()


def test_intrinsic_query_clone_and_replacement(module):
    pointer = module.unsafe_get_pointer_type(module.get_int_type())
    query = module.get_intrinsic(ir.Intrinsic.IS_ATOMIC, pointer)
    invocation = call(query, [])
    assert invocation.type is module.get_bool_type()
    assert list(invocation) == []
    assert match(invocation, CloneVisitor(module).clone(invocation))
    clone = CloneVisitor(module).force_clone(query)
    assert clone is not query and match(query, clone)
    assert pointer in clone.get_used_types()
    new_pointer = module.unsafe_get_pointer_type(module.get_float_type())
    assert clone.replace_used_type(pointer, new_pointer) == 1
    assert clone.intrinsic_type is new_pointer
    assert clone.has_valid_intrinsic_signature()
    assert not match(query, clone)

    size = module.get_intrinsic(ir.Intrinsic.SIZEOF, pointer)
    assert invocation.replace_used_value(invocation.callee, module.nr(ir.VarValue, size)) == 1
    assert invocation.type is module.get_int_type()


def test_function_realization_and_clone(module):
    integer, floating = module.get_int_type(), module.get_float_type()
    signature = module.unsafe_get_func_type("f_signature", floating, [integer])
    function = module.nr(ir.BodiedFunc, "f")
    function.realize(signature, ["x"])
    argument = function.get_arg_var("x")
    local = module.nr(ir.Var, floating, name="local")
    function.push_back(local)
    function.body = module.nr(ir.SeriesFlow)
    function.body.push_back(module.nr(ir.ReturnInstr, module.nr(ir.VarValue, local)))
    assert argument.type is integer
    assert list(function) == [local]
    assert function.get_used_types() == [signature]
    assert function.get_used_values() == [function.body]
    clone = CloneVisitor(module).force_clone(function)
    assert clone is not function
    assert clone.type is signature
    assert clone.body is not function.body
    assert clone.get_arg_var("x") is not argument
    assert list(clone)[0] is not local
    assert clone.body.front().value.var is list(clone)[0]


def test_series_insert_and_erase(module):
    flow = module.nr(ir.SeriesFlow)
    first, second = module.get_int(1), module.get_int(2)
    flow.push_back(second)
    flow.insert(0, first)
    assert list(flow) == [first, second]
    assert flow.front() is first
    assert flow.back() is second
    flow.erase(0)
    assert list(flow) == [second]


@pytest.mark.parametrize("instruction", [ir.ReturnInstr, ir.YieldInstr, ir.ThrowInstr])
def test_control_instruction_query_replace_and_clone(module, instruction):
    initial, replacement = module.get_int(1), module.get_int(2)
    node = module.nr(instruction, initial)
    assert node.get_used_values() == [initial]
    assert node.replace_used_value(initial, replacement) == 1
    clone = CloneVisitor(module).clone(node)
    assert clone is not node
    assert match(node, clone)
    assert clone.value is not replacement
    assert clone.value.val == 2
    assert node.replace_used_value(replacement, None) == 1
    assert node.get_used_values() == []
    assert match(node, CloneVisitor(module).clone(node))


def test_throw_fallback_is_a_dependency(module):
    bare = module.nr(ir.ThrowInstr)
    fallback = module.get_int(1)
    bare.fallback = fallback
    assert bare.get_used_values() == [fallback]
    plain = module.nr(ir.ThrowInstr)
    assert not match(bare, plain)
    clone = CloneVisitor(module).clone(bare)
    assert match(bare, clone)
    assert match(clone.fallback, fallback)
    assert bare.replace_used_value(fallback, None) == 1
    assert bare.fallback is None
    assert bare.get_used_values() == []
    assert match(bare, plain)


@pytest.mark.parametrize("instruction", [ir.BreakInstr, ir.ContinueInstr])
def test_loop_target_query_replace_and_clone(module, instruction):
    condition = module.get_bool(True)
    loop = module.nr(ir.WhileFlow, condition, module.nr(ir.SeriesFlow))
    new = module.nr(ir.WhileFlow, condition, module.nr(ir.SeriesFlow))
    node = module.nr(instruction, loop)
    assert node.get_used_values() == [loop]
    assert node.replace_used_value(loop, new) == 1
    clone = CloneVisitor(module, clone_loop=False).clone(node)
    assert clone.loop is new


@pytest.mark.parametrize("form", ["series", "while", "for", "if", "try"])
def test_flow_query_replace_and_clone(module, form):
    test = module.get_bool(True)
    body, replacement = module.nr(ir.SeriesFlow), module.nr(ir.SeriesFlow)
    variable = module.nr(ir.Var, module.get_int_type())
    other = module.nr(ir.SeriesFlow)
    match form:
        case "series":
            flow = module.nr(ir.SeriesFlow)
            flow.push_back(body)
            used = [body]
        case "while":
            flow = module.nr(ir.WhileFlow, test, body)
            used = [test, body]
        case "for":
            flow = module.nr(ir.ForFlow, test, body, variable)
            used = [test, body]
            assert flow.get_used_variables() == [variable]
            new_variable = module.nr(ir.Var, variable.type)
            assert flow.replace_used_variable(variable, new_variable) == 1
            assert flow.get_used_variables() == [new_variable]
        case "if":
            flow = module.nr(ir.IfFlow, test, body, other)
            used = [test, body, other]
        case "try":
            flow = module.nr(ir.TryCatchFlow, body, other)
            handler = module.nr(ir.SeriesFlow)
            flow.catches.append(ir.Catch(handler, variable.type, variable))
            used = [body, other, handler]
            assert flow.get_used_types() == [variable.type]
            assert flow.get_used_variables() == [variable]
    assert flow.get_used_values() == used
    assert flow.replace_used_value(body, replacement) == 1
    assert replacement in flow.get_used_values()
    clone = CloneVisitor(module).clone(flow)
    assert clone is not flow
    if form == "try":
        assert clone.catches[0].handler is not handler
        assert clone.catches[0].var is variable
        assert clone.catches[0].type is variable.type
    else:
        assert match(flow, clone)
    assert replacement not in clone.get_used_values()


@pytest.mark.parametrize("form", ["assign", "extract", "insert", "call", "ternary", "flow"])
def test_instruction_operands_and_clone(module, form):
    integer = module.get_int_type()
    initial, new = module.get_int(1), module.get_int(2)
    record = module.nr(ir.RecordType, "record", [integer], ["value"])
    variable = module.nr(ir.Var, record)
    address = module.nr(ir.VarValue, variable)
    match form:
        case "assign":
            node = module.nr(ir.AssignInstr, variable, initial)
            assert node.get_used_variables() == [variable]
            assert node.replace_used_variable(variable, module.nr(ir.Var, record)) == 1
        case "extract":
            node = module.nr(ir.ExtractInstr, address, "value", polymorphic=True)
            assert node.type is integer
            initial, new = address, module.nr(ir.VarValue, variable)
        case "insert":
            node = module.nr(ir.InsertInstr, address, "value", initial, polymorphic=True)
        case "call":
            function = module.nr(ir.ExternalFunc, "f")
            function.realize(module.unsafe_get_func_type("f_signature", integer, [integer]), ["x"])
            node = module.nr(ir.CallInstr, module.nr(ir.VarValue, function), [initial])
            assert node.front() is initial and node.back() is initial
            assert node.type is integer
        case "ternary":
            node = module.nr(ir.TernaryInstr, module.get_bool(False), initial, module.get_int(3))
            assert node.type is integer
        case "flow":
            node = module.nr(ir.FlowInstr, module.nr(ir.SeriesFlow), initial)
            assert node.type is integer
    assert initial in node.get_used_values()
    assert node.replace_used_value(initial, new) == 1
    assert new in node.get_used_values()
    clone = CloneVisitor(module).clone(node)
    assert clone is not node
    assert match(node, clone)
    if form in {"extract", "insert"}:
        assert clone.polymorphic and clone.field == "value"


@pytest.mark.parametrize("instruction", [ir.YieldInInstr, ir.StackAllocInstr])
def test_typed_instruction_query_replace_and_clone(module, instruction):
    integer = module.unsafe_get_pointer_type(module.get_int_type())
    floating = module.unsafe_get_pointer_type(module.get_float_type())
    operands = (integer, 3) if instruction is ir.StackAllocInstr else (integer,)
    node = module.nr(instruction, *operands)
    assert node.get_used_types() == [integer]
    assert node.replace_used_type(integer, floating) == 1
    clone = CloneVisitor(module).clone(node)
    assert clone is not node
    assert clone.type is floating
    assert match(node, clone)


def test_function_export_attributes(module):
    function = module.nr(ir.BodiedFunc)
    assert not is_export(function)
    assert not is_gpu_kernel(function)
    set_export(function, False)
    assert not function.has_attribute(ir.KeyValueAttribute)

    set_export(function, True)
    assert is_export(function)
    attributes = function.get_attribute(ir.KeyValueAttribute)
    attributes.attributes["custom.attribute"] = "preserved"
    set_export(function, True)
    set_export(function, False)
    set_export(function, False)
    assert not is_export(function)
    assert not is_gpu_kernel(function)
    assert attributes.get("custom.attribute") == "preserved"


def test_runtime_metadata_dependencies(module):
    typ = module.unsafe_get_membered_type("runtime.dependencies", ref=True)
    method = module.nr(ir.BodiedFunc, "method")
    replacement = module.nr(ir.BodiedFunc, "replacement")
    info = ir.RuntimeInfo()
    info.mro = [typ, module.get_int_type()]
    info.parameters = [module.get_bool_type()]
    info.methods[1] = method
    typ.runtime_info = info
    assert {module.get_int_type(), module.get_bool_type()} <= set(typ.get_used_types())
    assert typ.get_used_variables() == [method]
    assert typ.replace_used_variable(method, replacement) == 1
    assert typ.runtime_info.methods[1] is replacement
    assert typ.replace_used_variable(replacement, None) == 1
    assert typ.get_used_variables() == []


def test_llvm_function_type_dependencies_through_aliases(module):
    integer = module.get_int_type()
    floating = module.get_float_type()
    boolean = module.get_bool_type()
    function = module.nr(ir.LLVMFunc, "f")
    function.realize(module.unsafe_get_func_type("signature", integer, []), [])
    function.llvm_body, function.llvm_declares = "body", "decl"
    function.parent_type = integer
    function.llvm_literals = [ir.Generic(1), ir.Generic(integer)]
    assert function.get_used_types() == [function.type, integer, integer]
    assert function.replace_used_type(integer, floating) == 2
    alias, forwarder = module.nr(ir.Var, boolean), module.nr(ir.Var, integer)
    alias.replace_all(forwarder)
    forwarder.replace_all(function)
    assert alias.get_used_types() == [function.type, floating, floating]
    assert alias.replace_used_type(floating, boolean) == 2
    assert function.parent_type is boolean
    assert function.llvm_literals[-1].get_type_value() is boolean
    new_type = module.unsafe_get_func_type("new_signature", boolean, [])
    assert alias.replace_used_type(function.type, new_type) == 1
    assert alias.get_used_types() == [new_type, boolean, boolean]


def test_representation_cast_intrinsic(module):
    source = module.unsafe_get_membered_type("source", ref=True)
    target = module.unsafe_get_membered_type("target", ref=True)
    function = module.nr(ir.InternalFunc, "representation")
    function.realize(module.unsafe_get_func_type("cast.valid", target, [source]), ["value"])
    function.set_intrinsic(ir.Intrinsic.REPRESENTATION_CAST, target)
    assert function.has_valid_intrinsic_signature()
    assert function.get_intrinsic_name() == "representation_cast"
    assert function.intrinsic_derives_from_arguments()
    assert function.get_intrinsic_side_effect_status() == SideEffectStatus.PURE
    assert match(function, CloneVisitor(module).force_clone(function))
    function.realize(
        module.unsafe_get_func_type("cast.structural", target, [module.get_int_type()]), ["value"]
    )
    assert function.has_valid_intrinsic_signature()
    function.set_intrinsic(ir.Intrinsic.REPRESENTATION_CAST, source)
    assert not function.has_valid_intrinsic_signature()
    function.set_intrinsic(ir.Intrinsic.REPRESENTATION_CAST, target)
    function.realize(module.unsafe_get_func_type("cast.empty", target, []), [])
    assert not function.has_valid_intrinsic_signature()
    signature = module.unsafe_get_func_type("cast.extra", target, [source, source])
    function.realize(signature, ["a", "b"])
    assert not function.has_valid_intrinsic_signature()


def test_typeinfo_method_lookup_intrinsic(module):
    descriptor = module.unsafe_get_membered_type("lookup.descriptor", ref=True)
    integer = module.get_int_type()
    pointer = module.unsafe_get_pointer_type(integer)
    lookup = module.nr(ir.InternalFunc, "lookup.method")
    signature = module.unsafe_get_func_type("lookup.signature", pointer, [descriptor, integer])
    lookup.realize(signature, ["info", "id"])
    lookup.set_intrinsic(ir.Intrinsic.TYPEINFO_METHOD_LOOKUP)
    assert lookup.has_valid_intrinsic_signature()
    assert lookup.get_intrinsic_name() == "typeinfo_method_lookup"
    assert lookup.get_intrinsic_side_effect_status() == SideEffectStatus.UNKNOWN
    assert match(lookup, CloneVisitor(module).force_clone(lookup))
    lookup.realize(
        module.unsafe_get_func_type("lookup.invalid", pointer, [integer, integer]), ["info", "id"]
    )
    assert not lookup.has_valid_intrinsic_signature()


def test_call_operand_traversal_snapshots_replaced_arguments(module):
    typ = module.unsafe_get_func_type("snapshot", module.get_int_type(), [])
    callee = module.nr(ir.VarValue, module.nr(ir.Var, typ))
    first, second, replacement = (module.get_int(i) for i in range(1, 4))
    invocation = module.nr(ir.CallInstr, callee, [first, second, first])

    class Inspect(Operator):
        def __init__(self):
            super().__init__()
            self.visited = []

        def pre_hook(self, node):
            self.visited.append(node)
            if node is first:
                invocation.replace_used_value(second, replacement)

    first_walk = Inspect()
    first_walk.process(invocation)
    assert first_walk.visited == [invocation, first, second, callee]
    second_walk = Inspect()
    second_walk.process(invocation)
    assert second_walk.visited == [invocation, first, replacement, callee]


def test_virtual_call_evaluates_and_clones_receiver_once(module):
    typ = module.unsafe_get_membered_type("virtual.receiver", ref=True)
    integer = module.get_int_type()
    typ.realize([integer], ["field"])
    typ.polymorphic = True
    receiver = module.nr(ir.VarValue, module.nr(ir.Var, typ))
    argument = module.get_int(42)
    method = module.nr(ir.BodiedFunc)
    method.realize(
        module.unsafe_get_func_type("virtual.method", integer, [typ, integer]), ["self", "value"]
    )
    callee = module.nr(ir.FlowInstr, module.nr(ir.SeriesFlow), module.nr(ir.VarValue, method))
    parent = module.nr(ir.BodiedFunc)
    invocation = virtual_call(callee, [receiver, argument], 7, parent)
    invocation.name = "virtual.call"
    invocation.info = ast.Node.SrcInfo("virtual.codon", 12, 3)
    assert invocation.type is integer
    assert invocation.back() is argument
    original, assignment = invocation.callee.flow
    assert original is callee
    assert assignment.rhs is receiver
    temporary = assignment.lhs
    assert get_var(invocation.front()) is temporary
    assert temporary in parent.symbols

    lookup = invocation.callee.value
    intrinsic = get_func(lookup.callee)
    assert intrinsic.intrinsic == ir.Intrinsic.VIRTUAL_LOOKUP
    assert get_var(lookup.front()) is temporary
    assert lookup.back().val == 7
    assert not match(lookup, call(intrinsic, [lookup.front(), module.get_int(8)]))

    cloned_parent = module.nr(ir.BodiedFunc)
    cloned = CloneVisitor(module).clone(invocation, cloned_parent)
    assert cloned.name == "virtual.call"
    assert cloned.info.line == 12
    cloned_temporary = get_var(cloned.front())
    assert cloned_temporary is not temporary
    assert cloned_temporary in cloned_parent.symbols
    assert get_var(cloned.callee.value.front()) is cloned_temporary
    assert cloned.callee.value.back().val == 7
    assert cloned.callee.flow.back().lhs is cloned_temporary
