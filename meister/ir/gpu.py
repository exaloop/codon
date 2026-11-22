# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Extract NVPTX kernels, link device code, and embed PTX in the host module."""

import re
from ctypes import byref, string_at
from pathlib import Path

from .llvm import (
    A, I, N, P, S, U, BasicBlock, Function, LLVMError, Linkage, Metadata, Predicate, Type, Value,
)

TRIPLE = b"nvptx64-nvidia-cuda"
LAYOUT = (
    b"e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-"
    b"f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n16:32:64"
)
LIBDEVICE = "/usr/local/cuda/nvvm/libdevice/libdevice.10.bc"
GPU_NAME = "sm_30"
GPU_FEATURES = "+ptx42"
PTX_STUB = "__codon_ptx__"

INTRINSICS = (
    "ceil floor fabs exp exp2 log log2 log10 sqrt pow sin cos copysign trunc rint "
    "nearbyint round minnum maxnum fma"
).split()
MATH_FUNCTIONS = (
    "expm1 ldexp acos asin atan atan2 hypot tan cosh sinh tanh acosh asinh atanh "
    "erf erfc log1p tgamma lgamma remainder frexp modf"
).split()
VECTOR_UNARY = (
    "acos acosh asin asinh atan atanh exp exp2 expm1 log log10 log1p log2 "
    "sin sinh cos cosh tan tanh"
).split()
VECTOR_BINARY = ("atan2", "hypot")
NO_OPS = (
    "seq_register_finalizer", "seq_gc_add_roots", "seq_gc_remove_roots",
    "seq_gc_clear_roots", "seq_gc_exclude_static_roots",
)
ALLOCATORS = (
    "seq_alloc", "seq_alloc_uncollectable", "seq_alloc_atomic",
    "seq_alloc_atomic_uncollectable",
)


def kernels(module):
    for fn in module.functions():
        if fn._call("GetStringAttributeAtIndex", P, (U, S, U), -1, b"kernel", 6):
            yield fn


def patch_ptx(module, value=None):
    stub = module.function(PTX_STUB)
    if not stub:
        return
    if value is None:
        value = module.pointer_type().null()
    for call in list(stub.users()):
        if (call.is_call or call.is_a("InvokeInst")) and (
            call.called_function == stub and call.call_arg_count == 0
        ):
            call.replace_all_uses(value)
            if call.is_a("InvokeInst"):
                replace_invoke_with_branch(module, call)
            call.erase_instruction()
    module.builder.clear_insert_point()
    if stub.has_uses:
        raise LLVMError(f"some {PTX_STUB} calls were not replaced")
    stub._call("DeleteFunction", None, ())


def replace_invoke_with_branch(module, invoke):
    b = module.builder
    normal = BasicBlock(module, invoke._call("GetNormalDest", P, ()))
    unwind = BasicBlock(module, invoke._call("GetUnwindDest", P, ()))
    predecessor = invoke.block
    for phi in list(unwind.instructions()):
        if not phi.is_a("PHINode"):
            break
        incoming = [(value, block) for value, block in phi.phi_incoming() if block != predecessor]
        replacement = phi.type.undef()
        if incoming:
            name = phi.name
            phi.set_name("")
            b.position_before(phi)
            replacement = b.create_phi(phi.type, name=name)
            replacement.add_incoming(incoming)
        phi.replace_all_uses(replacement)
        phi.erase_instruction()
    b.position_before(invoke)
    b.create_br(normal)


def prepare(module, fast_math):
    if next(kernels(module), None) is None:
        patch_ptx(module)
        return None
    clone = module.clone()
    try:
        clone.lib.function("SetTarget", None, P, S)(clone, TRIPLE)
        clone.lib.function("SetDataLayout", None, P, S)(clone, LAYOUT)
        if fast_math:
            # LLVMModuleFlagBehaviorOverride
            clone.add_module_flag("nvvm-reflect-ftz", clone.int_type(32).const_int(1), behavior=3)
    except BaseException:
        clone.close()
        raise
    return clone


def return_type(fn):
    typ = fn.value_type
    return Type(fn._owner, typ._call("GetReturnType", P, ()))


def replace_function(old, new):
    old.replace_all_uses(new)
    old._call("DeleteFunction", None, ())


def no_op(module, fn):
    name = ".codon.gpu.dummy." + fn.name
    if dummy := module.function(name):
        return dummy
    dummy = module.add_function(name, fn.value_type)
    dummy.set_linkage(Linkage.PRIVATE)
    b = module.builder
    b.set_insert_point(dummy.append_block("entry"))
    ret = return_type(dummy)
    if ret == module.primitive_type("Void"):
        b.create_ret_void()
    else:
        b.create_ret(ret.undef())
    b.clear_insert_point()
    return dummy


def normalize_kernel(module, fn):
    ret = return_type(fn)
    if (
        not fn._call("CountBasicBlocks", U, ())
        or ret._call("GetTypeKind", I, ()) != 10  # LLVMStructTypeKind
        or not ret._call("IsLiteralStruct", I, ())
        or ret._call("CountStructElementTypes", U, ())
    ):
        return fn
    if fn.has_uses:
        raise LLVMError("kernel still has uses after ABI normalization")
    name = fn.name
    fn.set_name("")
    args = fn.args
    new_type = module.primitive_type("Void").function(
        [arg.type for arg in args], fn.value_type._call("IsFunctionVarArg", I, ())
    )
    kernel = module.add_function(name, new_type)
    for prop in ("Linkage", "Visibility", "DLLStorageClass", "UnnamedAddress"):
        kernel._call("Set" + prop, None, (I,), fn._call("Get" + prop, I, ()))
    kernel._call("SetFunctionCallConv", None, (U,), fn._call("GetFunctionCallConv", U, ()))
    for index in (-1, 0, *range(1, len(args) + 1)):
        attrs = (P * fn._call("GetAttributeCountAtIndex", U, (U,), index))()
        if len(attrs):
            fn._call("GetAttributesAtIndex", None, (U, A), index, attrs)
            for attr in attrs:
                kernel._call("AddAttributeAtIndex", None, (U, P), index, attr)
    if fn._call("HasPersonalityFn", I, ()):
        kernel.set_personality(fn._call("GetPersonalityFn", P, ()))
    if fn.subprogram:
        kernel.set_subprogram(fn.subprogram)
    for old, new in zip(args, kernel.args, strict=True):
        new.set_name(old.name)
        old.replace_all_uses(new)
    for block in list(fn.blocks()):
        block._call("RemoveBasicBlockFromParent", None, ())
        kernel._call("AppendExistingBasicBlock", None, (P,), block)
        if (term := block.terminator) and term.is_a("ReturnInst"):
            module.builder.position_before(term)
            module.builder.create_ret_void()
            term.erase_instruction()
    module.builder.clear_insert_point()
    fn._call("DeleteFunction", None, ())
    return kernel


def annotate_kernel(module, kernel):
    lib, ctx = module.lib, module.context
    metadata = lib.function("ValueAsMetadata", P, P)
    node = module.metadata_node((
        Metadata(module, metadata(kernel)),
        Metadata(module, lib.function("MDStringInContext2", P, P, S, N)(ctx, b"kernel", 6)),
        Metadata(module, metadata(module.int_type(32).const_int(1))),
    ))
    value = lib.function("MetadataAsValue", P, P, P)(ctx, node)
    lib.function("AddNamedMetadataOperand", None, P, S, P)(module, b"nvvm.annotations", value)


def required_globals(roots):
    # C++ getRequiredGVs follows direct GlobalValue operands of instructions.
    keep, work = set(), list(roots)
    while work:
        value = work.pop()
        if value._pointer in keep:
            continue
        keep.add(value._pointer)
        if value.is_a("Function"):
            for block in value.blocks():
                for inst in block.instructions():
                    for i in range(inst._call("GetNumOperands", I, ())):
                        operand = inst.operand(i)
                        if operand.is_a("GlobalValue"):
                            if operand.is_a("Function"):
                                operand = Function(operand._owner, operand._pointer)
                            work.append(operand)
    return keep


def expose(value):
    linkage = value._call("GetLinkage", I, ())
    if linkage in (Linkage.INTERNAL, Linkage.PRIVATE):
        value.set_linkage(Linkage.EXTERNAL)
        value._call("SetVisibility", None, (I,), 1)  # LLVMHiddenVisibility
    elif linkage in (Linkage.LINK_ONCE_ANY, Linkage.LINK_ONCE_ODR):
        value.set_linkage(
            Linkage.WEAK_ANY if linkage == Linkage.LINK_ONCE_ANY else Linkage.WEAK_ODR
        )


def prune(module, keep, run):
    lib = module.lib
    lib.function("SetModuleInlineAsm2", None, P, S, N)(module, b"", 0)
    for global_value in module.globals():
        if global_value._pointer in keep:
            expose(global_value)
        elif not global_value._call("IsDeclaration", I, ()):
            global_value.set_linkage(Linkage.EXTERNAL)
            global_value.set_initializer(None)
            global_value._call("SetComdat", None, (P,), None)
    for fn in module.functions():
        if fn._pointer in keep:
            expose(fn)
        elif fn.name.startswith("llvm.") or fn._call("IsDeclaration", I, ()):
            continue
        else:
            fn._call("SetVisibility", None, (I,), 0)
            fn._call("SetDLLStorageClass", None, (I,), 0)
            fn.set_linkage(Linkage.INTERNAL)
            fn._call("SetComdat", None, (P,), None)
    alias = lib.function("GetFirstGlobalAlias", P, P)(module)
    while alias:
        value = Value(module, alias)
        if alias in keep:
            expose(value)
        else:
            value._call("SetVisibility", None, (I,), 0)
            value.set_linkage(Linkage.INTERNAL)
        alias = value._call("GetNextGlobalAlias", P, ())
    run(b"globaldce,strip-dead-debug-info,strip-dead-prototypes")


def make_malloc(module):
    ptr, i64 = module.pointer_type(), module.int_type(64)
    malloc = module.get_or_insert_function("malloc", ptr.function((i64,)))
    malloc.set_linkage(Linkage.EXTERNAL)
    malloc.add_attribute("nounwind")
    malloc.add_attribute("noalias", index=0)
    # MemoryEffects encoding uses two bits per location. Only InaccessibleMem is ModRef.
    malloc.add_attribute("memory", 3 << 2)
    malloc.add_attribute("willreturn")
    return malloc


def allocation(module, fn, name):
    b, args = module.builder, fn.args
    if name in ALLOCATORS:
        b.create_ret(b.create_call(make_malloc(module), (args[0],)))
    elif name == "seq_realloc":
        ptr, i64 = module.pointer_type(), module.int_type(64)
        mem = b.create_call(make_malloc(module), (args[1],))
        memcpy = module.intrinsic("llvm.memcpy", (ptr, ptr, i64))
        b.create_call(memcpy, (mem, args[0], args[2], module.int_type(1).null()))
        b.create_ret(mem)
    else:
        ptr, i64 = module.pointer_type(), module.int_type(64)
        size = b.create_mul(args[0], args[1])
        mem = b.create_call(make_malloc(module), (size,))
        memset = module.intrinsic("llvm.memset", (ptr, i64))
        b.create_call(memset, (mem, module.int_type(8).null(), size, module.int_type(1).null()))
        b.create_ret(mem)


def decimal_point(module, fn):
    # Device code has no process locale. Use the C locale for number formatting.
    b = module.builder
    mem = b.create_call(make_malloc(module), (module.int_type(64).const_int(1),))
    b.create_store(module.int_type(8).const_int(ord(".")), mem)
    string = b.create_insert_value(return_type(fn).undef(), mem, 0)
    b.create_ret(b.create_insert_value(string, module.int_type(64).const_int(1), 1))


def memcmp(module, fn):
    b = module.builder
    lhs, rhs, size = fn.args
    entry = b.insert_block
    loop, next_byte, different, equal = (
        fn.append_block(name) for name in ("loop", "next", "different", "equal")
    )
    i8, ret = module.int_type(8), return_type(fn)
    zero, one = size.type.null(), size.type.const_int(1)
    b.create_cond_br(b.create_icmp(Predicate.EQ, size, zero), equal, loop)
    b.set_insert_point(loop)
    index = b.create_phi(size.type)
    a = b.create_load(i8, b.create_gep(i8, lhs, (index,), inbounds=True))
    c = b.create_load(i8, b.create_gep(i8, rhs, (index,), inbounds=True))
    b.create_cond_br(b.create_icmp(Predicate.EQ, a, c), next_byte, different)
    b.set_insert_point(next_byte)
    advance = b.create_add(index, one)
    index.add_incoming(((zero, entry), (advance, next_byte)))
    b.create_cond_br(b.create_icmp(Predicate.EQ, advance, size), equal, loop)
    b.set_insert_point(different)
    b.create_ret(b.create_sub(b.create_zext(a, ret), b.create_zext(c, ret)))
    b.set_insert_point(equal)
    b.create_ret(ret.null())


def vector_loop(module, fn, name, typ, *, binary=False, complex_input=False):
    b, args = module.builder, fn.args
    ptr, i64, i8 = module.pointer_type(), module.int_type(64), module.int_type(8)
    operands = (typ, typ) if binary or complex_input else (typ,)
    target = module.get_or_insert_function(name, typ.function(operands))
    target.add_attribute("willreturn")
    count = args[-1]
    pointers, strides = args[:-1:2], args[1:-1:2]
    loop, exit_block = fn.append_block("loop"), fn.append_block("exit")

    slots = [b.create_alloca(ptr) for _ in pointers]
    index_slot = b.create_alloca(i64)
    zero, one = i64.null(), i64.const_int(1)
    for pointer, slot in zip(pointers, slots, strict=True):
        b.create_store(pointer, slot)
    b.create_store(zero, index_slot)
    b.create_cond_br(b.create_icmp(Predicate.SGT, count, zero), loop, exit_block)

    b.set_insert_point(loop)
    positions = [b.create_load(ptr, slot) for slot in slots]
    values = [b.create_load(typ, position) for position in positions[:-1]]
    if complex_input:
        values.append(b.create_load(typ, b.create_gep(typ, positions[0], (one,))))
    b.create_store(b.create_call(target, values), positions[-1])
    index = b.create_add(b.create_load(i64, index_slot), one)
    b.create_store(index, index_slot)
    for position, stride, slot in zip(positions, strides, slots, strict=True):
        b.create_store(b.create_gep(i8, position, (stride,)), slot)
    index = b.create_load(i64, index_slot)
    b.create_cond_br(b.create_icmp(Predicate.SLT, index, count), loop, exit_block)

    b.set_insert_point(exit_block)
    ret = return_type(fn)
    if ret == module.primitive_type("Void"):
        b.create_ret_void()
    else:
        b.create_ret(ret.undef())


def fill_in(module, name, emit, *args, **kwargs):
    if old := module.function(name):
        fn = module.add_function(".codon.gpu.fillin." + name, old.value_type)
        fn.set_linkage(Linkage.PRIVATE)
        module.builder.set_insert_point(fn.append_block("entry"))
        emit(module, fn, *args, **kwargs)
        module.builder.clear_insert_point()
        replace_function(old, fn)


def remap_functions(module):
    names = {"seq_free": "free", **dict.fromkeys(NO_OPS, "")}
    for width, suffix in ((64, ""), (32, "f")):
        names.update({
            f"llvm.{name}.f{width}": "__nv_" + {"minnum": "fmin", "maxnum": "fmax"}.get(
                name, name
            ) + suffix for name in INTRINSICS
        })
        names.update({name + suffix: "__nv_" + name + suffix for name in MATH_FUNCTIONS})
    for name, target in names.items():
        if old := module.function(name):
            fn = (
                module.get_or_insert_function(target, old.value_type)
                if target else no_op(module, old)
            )
            fn.add_attribute("willreturn")
            replace_function(old, fn)

    fill_in(module, "seq_locale_decimal_point", decimal_point)
    for name in ("seq_locale_thousands_sep", "seq_locale_grouping"):
        fill_in(module, name, lambda module, fn: module.builder.create_ret(return_type(fn).null()))
    fill_in(module, "memcmp", memcmp)
    for name in (*ALLOCATORS, "seq_realloc", "seq_calloc", "seq_calloc_atomic"):
        fill_in(module, name, allocation, name)
    for name in ("seq_alloc_exc", "seq_throw"):
        fill_in(module, name, lambda module, fn: module.builder.create_unreachable())
    for width, suffix, float_type in ((64, "", "Double"), (32, "f", "Float")):
        typ = module.primitive_type(float_type)
        for name in (*VECTOR_UNARY, *VECTOR_BINARY):
            fill_in(
                module, f"cnp_{name}_float{width}", vector_loop, "__nv_" + name + suffix, typ,
                binary=name in VECTOR_BINARY,
            )
        fill_in(
            module, f"cnp_abs_complex{width * 2}", vector_loop, "__nv_hypot" + suffix, typ,
            complex_input=True,
        )


def clean_name(name):
    return re.sub(r"^[^A-Za-z_]|[^A-Za-z_0-9]", "_", name)


def emit_ptx(module, roots, libdevice, cpu, features):
    lib = module.lib
    lib.initialize_target("NVPTX", asm_parser=False)
    machine = lib.target_machine(triple=TRIPLE, cpu=cpu.encode(), features=features.encode())
    options = lib.function("CreatePassBuilderOptions", P)()
    try:
        for fn in module.functions():
            fn.add_attribute("nounwind")
            if fn._call("HasPersonalityFn", I, ()):
                fn.set_personality(None)

        layout = lib.function("CreateTargetDataLayout", P, P)(machine)
        try:
            lib.function("SetModuleDataLayout", None, P, P)(module, layout)
        finally:
            lib.function("DisposeTargetData", None, P)(layout)

        def run(pipeline, target=None):
            lib.check_error(lib.function("RunPasses", P, P, S, P, P)(
                module, pipeline, target, options
            ))

        keep = required_globals(roots)
        prune(module, keep, run)
        module.link_ir_file(libdevice)
        remap_functions(module)
        noinline = lib.function("GetEnumAttributeKindForName", U, S, N)(b"noinline", 8)
        for fn in module.functions():
            fn._call("RemoveEnumAttributeAtIndex", None, (U, U), -1, noinline)
            fn.add_attribute("willreturn")
            if fn._pointer not in keep and not fn._call("IsDeclaration", I, ()):
                fn._call("SetVisibility", None, (I,), 0)
                fn.set_linkage(Linkage.INTERNAL)
        lib.function("StripModuleDebugInfo", I, P)(module)

        run(b"default<O3>", machine)
        prune(module, required_globals(roots), run)
        for value in module.globals():
            value.set_name(clean_name(value.name))
        for fn in module.functions():
            if fn._call("CountBasicBlocks", U, ()):
                fn.set_name(clean_name(fn.name))

        module.verify()
        message, buffer = P(), P()
        if lib.function("TargetMachineEmitToMemoryBuffer", I, P, P, I, A, A)(
            machine, module, 0, byref(message), byref(buffer)
        ):
            raise LLVMError(lib.message(message))
        try:
            start = lib.function("GetBufferStart", P, P)(buffer)
            size = lib.function("GetBufferSize", N, P)(buffer)
            return string_at(start, size)
        finally:
            lib.function("DisposeMemoryBuffer", None, P)(buffer)
    finally:
        lib.function("DisposePassBuilderOptions", None, P)(options)
        lib.function("DisposeTargetMachine", None, P)(machine)


def embed(module, device):
    roots = [normalize_kernel(device, fn) for fn in list(kernels(device))]
    if not roots:
        patch_ptx(module)
        return
    for fn in roots:
        annotate_kernel(device, fn)

    options = module.options
    ptx = emit_ptx(
        device, roots, options.libdevice if options else LIBDEVICE,
        options.gpu_name if options else GPU_NAME, options.gpu_features if options else GPU_FEATURES,
    )
    for fn in list(module.functions()):
        if fn.name.startswith("llvm.nvvm") and fn._call("GetIntrinsicID", U, ()):
            replace_function(fn, no_op(module, fn))
    if options and options.ptx:
        Path(options.ptx).write_bytes(ptx)

    value = Value(module, module.lib.function("ConstStringInContext2", P, P, S, N, I)(
        module.context, ptx, len(ptx), False
    ))
    global_value = module.add_global(
        ".ptx", value.type, value, constant=True, linkage=Linkage.PRIVATE
    )
    global_value.set_unnamed_addr()
    patch_ptx(module, global_value)
