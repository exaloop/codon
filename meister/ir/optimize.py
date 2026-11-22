# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Run the LLVM portion of the compilation pipeline."""

from .llvm import I, N, P, S, U, Linkage, Value


def prepare_debug(module, debug, ctor):
    if not debug:
        module.lib.function("StripModuleDebugInfo", I, P)(module)
        return

    lib = module.lib
    attribute_kind = lib.function("GetEnumAttributeKindForName", U, S, N)
    alwaysinline = attribute_kind(b"alwaysinline", len(b"alwaysinline"))
    for fn in module.functions():
        if fn.name != ctor:
            # Keep functions visible to native debug symbol lookup.
            fn.set_linkage(Linkage.EXTERNAL)
        if not fn._call("GetEnumAttributeAtIndex", P, (U, U), -1, alwaysinline):
            fn.add_attribute("noinline")
        fn.add_attribute("uwtable", 2)
        fn.add_attribute("no-frame-pointer-elim", "true")
        fn.add_attribute("no-frame-pointer-elim-non-leaf", "")
        fn.add_attribute("no-jump-tables", "false")
        for block in fn.blocks():
            for inst in block.instructions():
                if inst.is_call:
                    inst._call("SetTailCall", None, (I,), False)


def apply_fast_math(module):
    for fn in module.functions():
        for block in fn.blocks():
            for inst in block.instructions():
                # LLVMFloatTypeKind through LLVMPPC_FP128TypeKind, and bfloat.
                if inst.type._call("GetTypeKind", I, ()) not in {1, 2, 3, 4, 5, 6, 18}:
                    continue
                callee = inst.called_function if inst.is_call else None
                if inst._call("CanValueUseFastMathFlags", I, ()) and (
                    inst._call("IsABinaryOperator", P, ())
                    or callee and callee._call("GetIntrinsicID", U, ())
                ):
                    inst._call("SetFastMathFlags", None, (U,), 0x7F)


def apply_native_target(module):
    # C++ ArchNativePass applies these to optimized functions, independent of
    # the machine's explicit target options.
    lib = module.lib
    if lib.arch not in {"X86", "AArch64", "ARM"}:
        return
    cpu = lib.message(lib.function("GetHostCPUName", P)())
    features = lib.message(lib.function("GetHostCPUFeatures", P)())
    frame_pointer = "non-leaf" if lib.arch == "AArch64" and "linux" in module.target else "none"
    for fn in module.functions():
        if cpu:
            fn.add_attribute("target-cpu", cpu)
        if features:
            fn.add_attribute("target-features", features)
        fn.add_attribute("frame-pointer", frame_pointer)


def openmp_callback_uses(module, fn, fork):
    lib = module.lib
    expected_name, argument = ("__kmpc_fork_call", 2) if fork else ("__kmpc_omp_task_alloc", 5)
    use = fn._call("GetFirstUse", P, ())
    while use:
        call = Value(module, lib.function("GetUser", P, P)(use))
        if not (call.is_call or call._call("IsAInvokeInst", P, ())):
            return False
        callee = call.called_function
        if not callee or callee.name != expected_name or (
            call.call_arg_count <= argument
            or call._call("GetOperandUse", P, (U,), argument) != use
        ):
            return False
        if not fork and not ((call.operand(2).signed_int() or 0) & 1):
            return False
        use = lib.function("GetNextUse", P, P)(use)
    return True


def optimize_openmp_thread_ids(module):
    # C++: optimize.cpp, OpenMPThreadIdOptimizer.
    if not (module.function("__kmpc_fork_call") or module.function("__kmpc_omp_task_alloc")):
        return
    ptr, i32 = module.pointer_type(), module.int_type(32)
    for fn in module.functions():
        if not fn.has_uses or fn._call("GetLinkage", I, ()) not in (
            Linkage.INTERNAL, Linkage.PRIVATE,
        ):
            continue
        args = fn.args
        if len(args) < 2 or args[1].type != ptr:
            continue
        fork = args[0].type == ptr
        if (not fork and args[0].type != i32) or not openmp_callback_uses(module, fn, fork):
            continue

        global_queries, team_queries = [], []
        for block in fn.blocks():
            for inst in block.instructions():
                if not inst.is_call or inst.type != i32 or not (callee := inst.called_function):
                    continue
                if callee.name == "__kmpc_global_thread_num" and inst.call_arg_count == 1:
                    global_queries.append(inst)
                elif fork and callee.name == "omp_get_thread_num" and not inst.call_arg_count:
                    team_queries.append(inst)

        if global_queries or team_queries:
            first = next(
                inst for inst in fn.entry_block.instructions()
                if not inst._call("IsAPHINode", P, ())
                and not inst._call("IsALandingPadInst", P, ())
            )
            module.builder.position_before(first)
            for queries, arg, name in (
                (global_queries, args[0], "omp.gtid"), (team_queries, args[1], "omp.btid"),
            ):
                if queries:
                    value = module.builder.create_load(i32, arg, name=name) if fork else arg
                    for query in queries:
                        query._call("ReplaceAllUsesWith", None, (P,), value)
                        query.erase_instruction()
            module.builder.clear_insert_point()


def optimize(module, *, debug, ctor, fast_math, native=True):
    # C++ runs the default O0 pipeline once and the default O3 pipeline twice.
    lib = module.lib
    machine = lib.target_machine() if native else None
    options = lib.function("CreatePassBuilderOptions", P)()
    try:
        run = lib.function("RunPasses", P, P, S, P, P)
        if native and not debug:
            apply_native_target(module)
        for _ in range(1 if debug else 2):
            prepare_debug(module, debug, ctor)
            if not debug:
                optimize_openmp_thread_ids(module)
            if fast_math:
                apply_fast_math(module)
            lib.check_error(run(module, b"default<O0>" if debug else b"default<O3>", machine, options))
            prepare_debug(module, debug, ctor)
    finally:
        if machine:
            lib.function("DisposeTargetMachine", None, P)(machine)
        lib.function("DisposePassBuilderOptions", None, P)(options)
    module.verify()
