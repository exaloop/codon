# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Allocation passes from codon/cir/llvm/optimize.cpp."""

from collections import defaultdict

from .llvm import I, P, S, U, Function, Linkage, Predicate, Type, Value
from .llvm_cfg import FunctionCFG

ALLOCATORS = {
    "seq_alloc", "seq_alloc_atomic", "seq_alloc_uncollectable", "seq_alloc_atomic_uncollectable",
}
HEAP_ALLOCATORS = {"seq_alloc", "seq_alloc_atomic"}
MEMORY_INTRINSICS = {"llvm.memset", "llvm.memcpy", "llvm.memmove"}
NOOP_INTRINSICS = {
    "llvm.assume", "llvm.invariant.start", "llvm.invariant.end",
    "llvm.lifetime.start", "llvm.lifetime.end",
}
INVARIANT_GROUP_INTRINSICS = {"llvm.launder.invariant.group", "llvm.strip.invariant.group"}


def direct_callee(inst):
    if inst.is_call or inst.is_a("InvokeInst"):
        return inst.called_function
    return None


def intrinsic_name(callee):
    name = callee.name
    for intrinsic in MEMORY_INTRINSICS | NOOP_INTRINSICS | INVARIANT_GROUP_INTRINSICS:
        if name == intrinsic or name.startswith(intrinsic + "."):
            if name.startswith(("llvm.memcpy.inline.", "llvm.memset.inline.")):
                return name
            return intrinsic
    return name


def is_volatile_memory_intrinsic(inst):
    return inst.operand(inst.call_arg_count - 1).signed_int() != 0


def compares_unescaped_allocation(compare, value, alloc):
    predicate = compare._call("GetICmpPredicate", I, ())
    if predicate not in (Predicate.EQ, Predicate.NE):
        return False
    other = compare.operand(1 if compare.operand(0) == value else 0)
    if other.is_a("ConstantPointerNull"):
        return True
    if other.is_a("LoadInst"):
        return other.operand(0).is_a("GlobalVariable")
    callee = direct_callee(other)
    return bool(callee and callee.name in ALLOCATORS and other != alloc)


def removable(alloc):
    # Unused allocation sites can only reach writes and no-op intrinsics.
    pending = [alloc]
    users = {}
    visited = set()
    while pending:
        pointer = pending.pop()
        if pointer._as_parameter_ in visited:
            continue
        visited.add(pointer._as_parameter_)
        for inst in pointer.users():
            if inst.is_a("AddrSpaceCastInst") or inst.is_a("BitCastInst") or inst.is_a("GetElementPtrInst"):
                pending.append(inst)
            elif inst.is_a("ICmpInst"):
                if not compares_unescaped_allocation(inst, pointer, alloc):
                    return None
            elif inst.is_a("StoreInst"):
                if inst._call("GetVolatile", I, ()) or inst.operand(1) != pointer:
                    return None
            elif inst.is_call and (callee := inst.called_function):
                name = intrinsic_name(callee)
                if name in MEMORY_INTRINSICS:
                    if is_volatile_memory_intrinsic(inst) or inst.operand(0) != pointer:
                        return None
                elif name in INVARIANT_GROUP_INTRINSICS or name == "seq_realloc":
                    pending.append(inst)
                elif name not in NOOP_INTRINSICS and name != "seq_free":
                    return None
            else:
                return None
            users[inst._as_parameter_] = inst
    return users.values()


def gep_offset(inst, layout):
    # Same indexed offset as GEPOperator::accumulateConstantOffset.
    typ = Type(inst._owner, inst._call("GetGEPSourceElementType", P, ()))
    result = 0
    for i in range(1, inst._call("GetNumOperands", I, ())):
        index = inst.operand(i).signed_int()
        if index is None:
            return None
        if i == 1:
            result += index * layout.alloc_size(typ)
        elif typ._call("GetTypeKind", I, ()) == 10:  # LLVMStructTypeKind
            result += layout.offset_of(typ, index)
            typ = Type(inst._owner, typ._call("StructGetTypeAtIndex", P, (U,), index))
        else:
            typ = Type(inst._owner, typ._call("GetElementType", P, ()))
            result += index * layout.alloc_size(typ)
    return result


def return_alignment(call, callee):
    return max(call.enum_attribute("align", 0, call=True) or 1,
               callee.enum_attribute("align", 0) or 1 if callee else 1)


def param_alignment(call, callee, index):
    return max(call.enum_attribute("align", index, call=True) or 1,
               callee.enum_attribute("align", index) or 1 if callee else 1)


def uncaptured_argument(call, index):
    return call.call_attribute("nocapture", index) is not None or call.call_attribute("captures", index) == 0


def safe_argument(call, index):
    return uncaptured_argument(call, index) and (
        call.call_attribute("nofree") is not None or call.call_attribute("nofree", index) is not None
    )


def demotable(alloc, callee, layout):
    size = alloc.operand(0).signed_int()
    if size is None or not 0 < size <= 1024 or alloc.type._call("GetPointerAddressSpace", U, ()):
        return None
    heap_alignment = 2 * layout.pointer_size()
    alignment = return_alignment(alloc, callee)
    stack_alignment = alignment if "atomic" in callee.name else max(alignment, layout.abi_alignment(alloc.type))
    pending = [(alloc, 0, max(heap_alignment, alignment))]
    visited = set()
    users = {}
    while pending:
        pointer, offset, alignment = pending.pop()
        if pointer._as_parameter_ in visited:
            continue
        visited.add(pointer._as_parameter_)
        for inst in pointer.users():
            if inst.is_a("BitCastInst"):
                pending.append((inst, offset, alignment))
            elif inst.is_a("GetElementPtrInst"):
                displacement = gep_offset(inst, layout) if offset is not None else None
                pending.append((inst, None if displacement is None else offset + displacement, alignment))
            elif inst.is_a("ICmpInst"):
                if not compares_unescaped_allocation(inst, pointer, alloc):
                    return None
            elif inst.is_a("StoreInst") or inst.is_a("LoadInst"):
                if inst._call("GetVolatile", I, ()) or (
                    inst.is_a("StoreInst") and inst.operand(1) != pointer
                ):
                    return None
                required = inst._call("GetAlignment", U, ())
                if required > alignment:
                    return None
                stack_alignment = max(stack_alignment, required)
            elif inst.is_call:
                if inst._call("GetNumOperandBundles", U, ()):
                    return None
                target = inst.called_function
                name = intrinsic_name(target) if target else ""
                args = [i + 1 for i in range(inst.call_arg_count) if inst.operand(i) == pointer]
                required = max((param_alignment(inst, target, i) for i in args), default=1)
                if required > alignment:
                    return None
                stack_alignment = max(stack_alignment, required)
                if name in MEMORY_INTRINSICS:
                    if is_volatile_memory_intrinsic(inst):
                        return None
                elif name in INVARIANT_GROUP_INTRINSICS:
                    required = return_alignment(inst, target)
                    if required > alignment:
                        return None
                    stack_alignment = max(stack_alignment, required)
                    pending.append((inst, offset, alignment))
                elif name == "seq_realloc":
                    new_size = inst.operand(1).signed_int()
                    if inst.operand(0) != pointer or offset != 0 or new_size is None or not 0 < new_size <= 1024:
                        return None
                    size = max(size, new_size)
                    required = return_alignment(inst, target)
                    stack_alignment = max(stack_alignment, required)
                    pending.append((inst, 0, max(heap_alignment, required)))
                elif name not in NOOP_INTRINSICS and name != "seq_free":
                    if not target or inst._call("GetTailCallKind", I, ()) == 2 or inst.type.is_pointer:
                        return None
                    if not args or not all(safe_argument(inst, i) for i in args):
                        return None
                    stack_alignment = max(stack_alignment, alignment)
                users[inst._as_parameter_] = (inst, name)
            else:
                return None
    return size, stack_alignment, users.values()


def first_insertion(block, *, past_allocas=False):
    return next(
        ins for ins in block.instructions()
        if not ins.is_a("PHINode") and (not past_allocas or not ins.is_a("AllocaInst"))
    )


def remove_allocations(module, function, allocations):
    builder = module.builder
    i8, i64 = module.int_type(8), module.int_type(64)
    for alloc, callee in allocations:
        if not alloc._pointer:
            continue
        dead = removable(alloc)
        if dead is not None:
            for inst in dead:
                if inst.is_a("ICmpInst"):
                    value = inst.type.const_int(inst._call("GetICmpPredicate", I, ()) == Predicate.NE)
                elif not inst.type.is_void:
                    value = Value(module, inst.type._call("GetPoison", P, ()))
                else:
                    continue
                inst.replace_all_uses(value)
            for inst in dead:
                inst.erase_instruction()
            alloc.erase_instruction()
            continue

        result = demotable(alloc, callee, module.data_layout)
        if result is None:
            continue
        size, alignment, users = result
        builder.position_before(first_insertion(function.entry_block))
        replacement = builder.create_array_alloca(i8, i64.const_int(size))
        replacement.set_alignment(alignment)
        alloc.replace_all_uses(replacement)
        for inst, name in users:
            if name == "seq_realloc":
                inst.replace_all_uses(replacement)
                inst.erase_instruction()
            elif name == "seq_free":
                inst.erase_instruction()
            elif inst._call("IsTailCall", I, ()):
                inst._call("SetTailCall", None, (I,), False)
        alloc.erase_instruction()
    builder.clear_insert_point()


def allocation_sites(module, names):
    # Start at the allocator declarations so unrelated functions do not need
    # instruction scans in large modules.
    owners = defaultdict(dict)
    for name in sorted(names):
        callee = module.function(name)
        if not callee:
            continue
        for inst in callee.users():
            if inst.is_call and inst.called_function == callee and inst.call_arg_count == 1:
                parent = inst.block._call("GetBasicBlockParent", P, ())
                owners[parent][inst._as_parameter_] = (inst, callee)
    for parent, calls in owners.items():
        function = Function(module, parent)
        yield function, [
            calls[inst._as_parameter_]
            for block in function.blocks() for inst in block.instructions()
            if inst._as_parameter_ in calls
        ]


def aggregate_paths(inst, value, path):
    """Propagate an aggregate field through insertvalue or extractvalue."""
    indices = inst.indices
    if inst.is_a("InsertValueInst"):
        if inst.operand(0) == value and path[:len(indices)] != indices:
            yield path
        if inst.operand(1) == value:
            yield indices + path
    elif path[:len(indices)] == indices:
        yield path[len(indices):]


def aggregate_field_unused(callee, argument, indices, budget, active):
    key = callee._as_parameter_
    if key in active or len(active) >= 8 or callee._call("IsDeclaration", I, ()):
        return False
    # An interposable definition cannot establish what an external call reads.
    if callee._call("GetLinkage", I, ()) not in (Linkage.EXTERNAL, Linkage.INTERNAL, Linkage.PRIVATE):
        return False
    active = active | {key}
    work = [(argument, indices)]
    visited = set()
    while work:
        value, path = work.pop()
        state = value._as_parameter_, path
        if state in visited:
            continue
        visited.add(state)
        for inst in value.users():
            if not budget[0]:
                return False
            budget[0] -= 1
            if inst.is_a("PHINode") or inst.is_a("FreezeInst"):
                work.append((inst, path))
            elif inst.is_a("SelectInst"):
                if inst.operand(0) == value:
                    return False
                work.append((inst, path))
            elif inst.is_a("InsertValueInst") or inst.is_a("ExtractValueInst"):
                work.extend((inst, member) for member in aggregate_paths(inst, value, path))
            elif target := direct_callee(inst):
                if not path or inst._call("GetNumOperandBundles", U, ()):
                    return False
                args = target.args
                for i in range(inst.call_arg_count):
                    if inst.operand(i) == value and (
                        i >= len(args) or not aggregate_field_unused(target, args[i], path, budget, active)
                    ):
                        return False
            else:
                return False
    return True


def strip_pointer_casts(value):
    while value.type.is_pointer:
        if value.is_a("BitCastInst") or value.is_a("AddrSpaceCastInst"):
            value = value.operand(0)
        elif value.is_a("GetElementPtrInst") and all(
            value.operand(i).signed_int() == 0
            for i in range(1, value._call("GetNumOperands", I, ()))
        ):
            value = value.operand(0)
        else:
            break
    return value


def hoistable(alloc, cfg, loop):
    def contains(inst):
        return inst.block._as_parameter_ in loop.blocks

    block = alloc.block._as_parameter_
    if not cfg.dom.dominates(loop.header, block) or block in cfg.irreducible:
        return None
    if any(value.is_a("Instruction") and contains(value)
           for value in (alloc.operand(i) for i in range(alloc.call_arg_count))):
        return None
    work, provenance, releases = [(alloc, ())], set(), {}
    while work:
        value, path = work.pop()
        state = value._as_parameter_, path
        if state in provenance:
            continue
        provenance.add(state)
        for inst in value.users():
            if not inst.is_a("Instruction") or not contains(inst):
                return None
            if inst.is_a("PHINode"):
                if inst.block._as_parameter_ == loop.header:
                    return None
                work.append((inst, path))
            elif inst.is_a("SelectInst") or inst.is_a("FreezeInst"):
                work.append((inst, path))
            elif inst.is_a("InsertValueInst") or inst.is_a("ExtractValueInst"):
                work.extend((inst, member) for member in aggregate_paths(inst, value, path))
            elif any(inst.is_a(kind) for kind in (
                "PtrToIntInst", "IntToPtrInst", "AddrSpaceCastInst", "BitCastInst", "GetElementPtrInst",
            )) or inst._call("GetInstructionOpcode", I, ()) in (8, 10):  # LLVMAdd, LLVMSub
                if path:
                    return None
                work.append((inst, ()))
            elif inst.is_a("ICmpInst"):
                continue
            elif inst.is_a("LoadInst") or inst.is_a("StoreInst"):
                if inst._call("GetVolatile", I, ()) or (
                    inst.is_a("StoreInst") and inst.operand(1) != value
                ):
                    return None
            elif inst.is_call or inst.is_a("InvokeInst"):
                target = inst.called_function
                name = intrinsic_name(target) if target else ""
                if name in MEMORY_INTRINSICS:
                    if is_volatile_memory_intrinsic(inst):
                        return None
                elif name in INVARIANT_GROUP_INTRINSICS:
                    work.append((inst, ()))
                elif name == "seq_free":
                    if (not inst.is_call or inst._call("GetTailCallKind", I, ()) == 2
                            or inst._call("GetNumOperandBundles", U, ()) or inst.call_arg_count != 1
                            or inst.has_uses or strip_pointer_casts(inst.operand(0)) != alloc):
                        return None
                    releases[inst._as_parameter_] = inst
                elif name not in NOOP_INTRINSICS:
                    if inst._call("GetNumOperandBundles", U, ()):
                        return None
                    arguments = [i for i in range(inst.call_arg_count) if inst.operand(i) == value]
                    if not arguments:
                        return None
                    for i in arguments:
                        if path:
                            args = target.args if target else ()
                            if i >= len(args) or not aggregate_field_unused(target, args[i], path, [256], set()):
                                return None
                        elif inst.call_attribute("byval", i + 1) is None and (
                            inst.type.is_pointer or not safe_argument(inst, i + 1)
                        ):
                            return None
            else:
                return None
    return list(releases.values())


def move_instruction(builder, inst):
    name = inst.name.encode()
    inst._call("InstructionRemoveFromParent", None, ())
    if name:
        builder._call("InsertIntoBuilderWithName", None, (P, S), inst, name)
    else:
        builder._call("InsertIntoBuilder", None, (P,), inst)


def split_allocation_block(module, function, alloc, cached, cache):
    # SplitBlockAndInsertIfThenElse. Repair the PHIs on the original outgoing
    # edges after moving the tail, then merge the old and new allocations.
    builder = module.builder
    block = alloc.block
    successors = list(block.successors())
    yes, no, tail = (function.append_block("") for _ in range(3))
    previous = block
    for new in (yes, no, tail):
        new._call("MoveBasicBlockAfter", None, (P,), previous)
        previous = new
    rest = list(block.instructions())
    builder.set_insert_point(tail)
    for inst in rest[rest.index(alloc) + 1:]:
        move_instruction(builder, inst)
    for successor in dict((s._as_parameter_, s) for s in successors).values():
        for phi in list(successor.instructions()):
            if not phi.is_a("PHINode"):
                break
            incoming = list(phi.phi_incoming())
            if not any(parent == block for _, parent in incoming):
                continue
            builder.position_before(phi)
            name = phi.name
            phi.set_name("")
            updated = builder.create_phi(phi.type, name=name)
            updated.add_incoming((value, tail if parent == block else parent) for value, parent in incoming)
            phi.replace_all_uses(updated)
            phi.erase_instruction()
    builder.position_before(alloc)
    condition = builder.create_is_null(cached)
    builder.create_cond_br(condition, yes, no)
    builder.position_before(next(tail.instructions()))
    merged = builder.create_phi(alloc.type)
    alloc.replace_all_uses(merged)
    builder.set_insert_point(yes)
    move_instruction(builder, alloc)
    builder.create_store(alloc, cache)
    builder.create_br(tail)
    builder.set_insert_point(no)
    builder.create_br(tail)
    merged.add_incoming(((alloc, yes), (cached, no)))


def hoist_allocations(module, function, allocations):
    builder = module.builder
    cfg = FunctionCFG(function)
    ptr = module.pointer_type()
    for alloc, _ in allocations:
        block = alloc.block._as_parameter_
        loop = cfg.loop_for(block)
        if not loop or loop.preheader is None:
            continue
        releases = hoistable(alloc, cfg, loop)
        if releases is None or releases and (
            not loop.dedicated_exits or not loop.exits or any(
                any(first_insertion(cfg.blocks[key]).is_a(kind) for kind in (
                    "LandingPadInst", "CatchPadInst", "CleanupPadInst", "CatchSwitchInst",
                ))
                for key in loop.exits
            )
        ):
            continue
        preheader = cfg.blocks[loop.preheader]
        cache = None
        if cfg.postdominators().dominates(block, loop.preheader):
            builder.position_before(preheader.terminator)
            move_instruction(builder, alloc)
        else:
            size = alloc.operand(0).signed_int()
            if size is not None and 0 <= size <= 1024:
                continue
            builder.position_before(first_insertion(function.entry_block, past_allocas=True))
            cache = builder.create_alloca(ptr, name="alloc_hoist.cache")
            builder.position_before(preheader.terminator)
            builder.create_store(ptr.null(), cache)
            builder.position_before(alloc)
            cached = builder.create_load(ptr, cache)
            split_allocation_block(module, function, alloc, cached, cache)
        if releases:
            target = releases[0].called_function
            calling_conv = releases[0]._call("GetInstructionCallConv", U, ())
            for key in loop.exits:
                exit_ = cfg.blocks[key]
                builder.position_before(first_insertion(exit_))
                owner = builder.create_load(ptr, cache) if cache else alloc
                cleanup = builder.create_call(target, (owner,))
                cleanup._call("SetInstructionCallConv", None, (U,), calling_conv)
            for release in releases:
                release.erase_instruction()
        if cache:
            cfg = FunctionCFG(function)
    builder.clear_insert_point()


def free_noncaptured_allocations(module, function, allocations):
    cfg = FunctionCFG(function)
    postdom = cfg.postdominators()
    builder = module.builder
    for alloc, callee in allocations:
        if not callee._call("IsDeclaration", I, ()):
            continue
        alloc_block = alloc.block._as_parameter_
        target = alloc_block
        visited = set()
        work = [alloc]
        while work and target:
            value = work.pop()
            if value._as_parameter_ in visited:
                continue
            visited.add(value._as_parameter_)
            for inst in value.users():
                if any(inst.is_a(kind) for kind in ("GetElementPtrInst", "BitCastInst", "PHINode", "SelectInst")):
                    work.append(inst)
                    continue
                if inst.is_a("StoreInst"):
                    if inst.operand(1) != value:
                        target = None
                        break
                elif inst.is_call:
                    if inst._call("GetNumOperandBundles", U, ()):
                        target = None
                        break
                    callee = inst.called_function
                    arguments = [i + 1 for i in range(inst.call_arg_count) if inst.operand(i) == value]
                    if not callee or callee.name in {"seq_free", "seq_realloc"} or not arguments or not all(
                        safe_argument(inst, i) for i in arguments
                    ):
                        target = None
                        break
                elif not inst.is_a("LoadInst") and not inst.is_a("ICmpInst"):
                    target = None
                    break
                target = postdom.common_dominator(target, inst.block._as_parameter_)
        if not target:
            continue
        alloc_loop, free_loop = cfg.loop_for(alloc_block), cfg.loop_for(target)
        while alloc_loop is not free_loop:
            if not free_loop or len(free_loop.exits) != 1:
                target = None
                break
            target = free_loop.exits[0]
            free_loop = cfg.loop_for(target)
        if not cfg.dom.dominates(alloc_block, target):
            continue
        function_type = module.primitive_type("Void").function((module.pointer_type(),))
        release = module.function("seq_free")
        if release is None:
            release = module.add_function("seq_free", function_type)
            release.add_attribute("willreturn")
            release.add_attribute("nounwind")
        builder.position_before(cfg.blocks[target].terminator)
        builder.create_call(release, (alloc,))
    builder.clear_insert_point()
