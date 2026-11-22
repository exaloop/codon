# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Coroutine passes from codon/cir/llvm/optimize.cpp."""

from ctypes import POINTER, byref, string_at
from itertools import combinations

from .llvm import A, I, N, P, Q, S, U, Function, Linkage, Metadata, Predicate
from .llvm_alloc import direct_callee, strip_pointer_casts, uncaptured_argument
from .llvm_cfg import FunctionCFG

UNROLL_GUARD = b"codon.coro.unroll"
UNROLL_DISABLE = b"llvm.loop.unroll.disable"


def nonnull_operand(left, right):
    if not left.type.is_pointer:
        return None
    if left.is_a("ConstantPointerNull"):
        return None if right.is_a("ConstantPointerNull") else right
    return left if right.is_a("ConstantPointerNull") else None


def merge_guarded_state(cfg, loop):
    states = []
    for phi in cfg.blocks[loop.header].instructions():
        if not phi.is_a("PHINode"):
            break
        if phi.type.is_pointer:
            states.append(phi)
    if len(states) > 8:
        return False
    for left_root, right_root in combinations(states, 2):
        visited = set()

        def equivalent(left, right, predecessor=None, successor=None):
            if left == right:
                return True
            if predecessor:
                branch = predecessor.terminator
                if branch.is_a("BranchInst") and branch._call("IsConditional", I, ()):
                    compare = branch.operand(0)
                    if compare.is_a("ICmpInst"):
                        predicate = compare._call("GetICmpPredicate", I, ())
                        if predicate in (Predicate.EQ, Predicate.NE):
                            tested = nonnull_operand(compare.operand(0), compare.operand(1))
                            other = nonnull_operand(left, right)
                            same = tested and other and (
                                tested == other or tested == left_root and other == right_root
                                or tested == right_root and other == left_root
                            )
                            null_edge = 0 if predicate == Predicate.EQ else 1
                            targets = list(predecessor.successors())
                            if (
                                same and targets[null_edge] == successor
                                and targets[1 - null_edge] != successor
                            ):
                                return True
            if not (left.is_a("PHINode") and right.is_a("PHINode") and left.block == right.block):
                return False
            pair = left._as_parameter_, right._as_parameter_
            if pair in visited:
                return True
            if len(visited) == 16:
                return False
            visited.add(pair)
            right_values = {block._as_parameter_: value for value, block in right.phi_incoming()}
            return all(
                equivalent(value, right_values[block._as_parameter_], block, left.block)
                for value, block in left.phi_incoming()
            )

        if equivalent(left_root, right_root):
            right_root.replace_all_uses(left_root)
            right_root.erase_instruction()
            return True
    return False


def known_nonzero(value):
    if value.is_a("AllocaInst"):
        return True
    if value.is_a("GlobalValue"):
        return value._call("GetLinkage", I, ()) != Linkage.EXTERNAL_WEAK
    if direct_callee(value):
        return value.call_attribute("nonnull", 0) is not None
    return False


def simplify_branches(module, cfg, loop):
    changed = merge_guarded_state(cfg, loop)
    builder = module.builder
    for key in cfg.blocks:
        if key not in loop.blocks:
            continue
        for compare in list(cfg.blocks[key].instructions()):
            if not compare.is_a("ICmpInst"):
                continue
            pred = compare._call("GetICmpPredicate", I, ())
            if pred not in (Predicate.EQ, Predicate.NE):
                continue
            phi = nonnull_operand(compare.operand(0), compare.operand(1))
            if not phi or not phi.is_a("PHINode"):
                continue
            incoming = list(phi.phi_incoming())
            if len(incoming) > 8:
                continue
            values = []
            for value, block in incoming:
                is_null = value.is_a("ConstantPointerNull")
                if not is_null and not known_nonzero(value):
                    break
                values.append((compare.type.const_int(is_null == (pred == Predicate.EQ)), block))
            if len(values) != len(incoming):
                continue
            builder.position_before(phi)
            condition = builder.create_phi(compare.type, name="coro.done")
            condition.add_incoming(values)
            compare.replace_all_uses(condition)
            compare.erase_instruction()
            changed = True
    builder.clear_insert_point()
    if changed:
        return

    exiting = [key for key in loop.blocks if any(
        target not in loop.blocks for target in cfg.successors[key]
    )]
    if len(exiting) != 1:
        return
    key = exiting[0]
    branch = cfg.blocks[key].terminator
    edges = cfg.successors[key]
    if (
        not branch.is_a("BranchInst") or len(edges) != 2
        or edges[0] in loop.blocks or edges[1] not in loop.blocks
    ):
        return
    compare = branch.operand(0)
    if not compare.is_a("ICmpInst") or compare._call("GetICmpPredicate", I, ()) != Predicate.EQ:
        return
    select = nonnull_operand(compare.operand(0), compare.operand(1))
    if not select or not select.is_a("SelectInst"):
        return
    phi = nonnull_operand(select.operand(1), select.operand(2))
    if not phi or not phi.is_a("PHINode"):
        return
    for value, block in phi.phi_incoming():
        if value.is_a("Constant"):
            if value.is_a("ConstantPointerNull"):
                return
        elif block._as_parameter_ != key or value != select:
            return
    branch._call("SetCondition", None, (P,), select.operand(0))


def metadata_operands(lib, node):
    operands = (P * lib.function("GetMDNodeNumOperands", U, P)(node))()
    lib.function("GetMDNodeOperands", None, P, A)(node, operands)
    return operands


def metadata_name(lib, node):
    if node and lib.function("IsAMDNode", P, P)(node):
        operands = metadata_operands(lib, node)
        if len(operands) and operands[0]:
            length = U()
            pointer = lib.function("GetMDString", P, P, POINTER(U))
            if data := pointer(operands[0], byref(length)):
                return string_at(data, length.value)
    return b""


def protected_loop(module, term, kind, suspends):
    lib = module.lib
    original = term._call("GetMetadata", P, (U,), kind)
    if not original and not suspends:
        return
    operands = metadata_operands(lib, original)[1:] if original else ()
    names = [metadata_name(lib, operand) for operand in operands]
    # Preserve explicit unroll hints. Only remove disables owned by this pass.
    if suspends and UNROLL_DISABLE in names or not suspends and UNROLL_GUARD not in names:
        return
    metadata = lib.function("ValueAsMetadata", P, P)
    elements = [
        Metadata(module, metadata(operand)) if operand else None
        for operand, name in zip(operands, names)
        if name not in (UNROLL_GUARD, UNROLL_DISABLE)
    ]
    if suspends:
        for hint in (UNROLL_GUARD, UNROLL_DISABLE):
            string = lib.function("MDStringInContext2", P, P, S, N)(module.context, hint, len(hint))
            elements.append(module.metadata_node((Metadata(module, string),)))
    replacement = None
    if elements:
        node = module.metadata_node((None, *elements))
        replacement = lib.function("MetadataAsValue", P, P, P)(module.context, node)
        lib.function("ReplaceMDNodeOperandWith", None, P, U, P)(replacement, 0, node)
    term._call("SetMetadata", None, (U, P), kind, replacement)


def suspend_blocks(module):
    blocks = {}
    if fn := module.function("llvm.coro.suspend"):
        for call in fn.users():
            if call.is_call:
                block = call.block
                parent = block._call("GetBasicBlockParent", P, ())
                blocks.setdefault(parent, set()).add(block._as_parameter_)
    return blocks


def optimize_loops(module):
    kind = module.metadata_kind("llvm.loop")
    suspensions = suspend_blocks(module)
    for fn in module.functions():
        if fn._call("IsDeclaration", I, ()):
            continue
        suspended = suspensions.get(fn._as_parameter_, set())
        cfg = FunctionCFG(fn)
        for loop in cfg.loops:
            suspends = not suspended.isdisjoint(loop.blocks)
            for latch in loop.latches:
                protected_loop(module, cfg.blocks[latch].terminator, kind, suspends)
            simplify_branches(module, cfg, loop)


def cleanup_guards(module):
    kind = module.metadata_kind("llvm.loop")
    suspended = suspend_blocks(module)
    for fn in module.functions():
        if fn._as_parameter_ in suspended:
            continue
        for block in fn.blocks():
            for inst in block.instructions():
                protected_loop(module, inst, kind, False)


def noncaptured_frame(value):
    pending, visited = [value], set()
    budget = 8192
    while pending:
        pointer = pending.pop()
        if pointer._as_parameter_ in visited:
            continue
        visited.add(pointer._as_parameter_)
        for use in pointer.users():
            budget -= 1
            if budget < 0:
                return False
            if any(use.is_a(kind) for kind in (
                "BitCastInst", "AddrSpaceCastInst", "GetElementPtrInst", "SelectInst", "PHINode",
            )):
                pending.append(use)
            elif use.is_a("LoadInst") or use.is_a("ICmpInst"):
                continue
            elif use.is_a("StoreInst"):
                if use.operand(0) == pointer:
                    return False
            elif direct_callee(use):
                if use._call("GetNumOperandBundles", U, ()):
                    return False
                indices = [i + 1 for i in range(use.call_arg_count) if use.operand(i) == pointer]
                if not indices or not all(uncaptured_argument(use, i) for i in indices):
                    return False
            else:
                return False
    return True


def inline_resumes(module, run):
    lib = module.lib
    name = b"codon.generator"
    alwaysinline = b"alwaysinline"
    kind = lib.function("GetEnumAttributeKindForName", U, S, N)(alwaysinline, len(alwaysinline))
    attr = lib.function("CreateEnumAttribute", P, P, U, Q)(module.context, kind, 0)
    budget = {}
    marked = False
    captures = {}
    for callee in module.functions():
        if not callee.name.endswith(".resume") or callee._call("GetLinkage", I, ()) not in (
            Linkage.INTERNAL, Linkage.PRIVATE,
        ):
            continue
        if not callee._call("GetStringAttributeAtIndex", P, (U, S, U), -1, name, len(name)):
            continue
        size = sum(1 for block in callee.blocks() for _ in block.instructions())
        if size > 128:
            continue
        for call in callee.users():
            if not direct_callee(call) or call.called_function != callee or call.call_arg_count != 1:
                continue
            if call.call_attribute("noinline") is not None:
                continue
            parent = call.block._call("GetBasicBlockParent", P, ())
            if parent == callee._as_parameter_:
                continue
            if parent not in budget:
                budget[parent] = 8192 - sum(
                    1 for block in Function(module, parent).blocks() for _ in block.instructions()
                )
            if budget[parent] < size:
                continue
            frame = strip_pointer_casts(call.operand(0))
            while frame.is_a("GetElementPtrInst"):
                frame = strip_pointer_casts(frame.operand(0))
            allocator = direct_callee(frame)
            if not frame.is_a("AllocaInst") and (not allocator or allocator.name != "seq_alloc"):
                continue
            key = frame._as_parameter_
            if key not in captures:
                captures[key] = noncaptured_frame(frame)
            if captures[key]:
                call._call("AddCallSiteAttribute", None, (U, P), -1, attr)
                budget[parent] -= size
                marked = True
    if marked:
        run(b"always-inline")
