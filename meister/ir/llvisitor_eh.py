# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""LLVM lowering for try, except, else and finally."""

from enum import IntEnum
from types import SimpleNamespace

from . import RefType, SeriesFlow, cast
from .llvm import Predicate


# C++: codon/cir/llvm/llvisitor.h:52
class ExceptionState(IntEnum):
    NOT_THROWN = 0
    THROWN = 1
    CAUGHT = 2
    RETURN = 3
    BREAK = 4
    CONTINUE = 5
    RETHROW = 6


# C++: codon/cir/llvm/llvisitor.cpp:2851
def lower_try_catch(visitor, node):
    b, fn = visitor.b, visitor.fn
    i8, i32, i64, ptr = visitor.i8, visitor.i32, visitor.i64, visitor.ptr
    parent = visitor.innermost_try_catch()
    is_root = parent is None
    parent_finally_sequence = visitor.finally_[-1].sequence_number if visitor.finally_ else -1
    support_loop_control = bool(visitor.loops)
    finally_flow = cast(SeriesFlow, node.finally_)
    have_finally = bool(finally_flow and finally_flow.series)
    not_thrown, thrown, caught, ret, brk, cont, rethrow = (
        i8.const_int(state) for state in ExceptionState
    )
    zero = i64.null()
    ret_type = fn.value_type.return_type

    def new_block(name):
        return fn.append_block("trycatch." + name)

    b.set_insert_point(visitor.block)
    entry = new_block("entry")
    b.create_br(entry)
    tc = SimpleNamespace(
        exception_block=new_block("exception"),
        exception_route_block=new_block("exception_route"),
        finally_block=new_block("finally"),
        finally_exception_block=new_block("finally.exception"),
        catch_types=[],
        handlers=[],
    )
    external, resume, rethrow_block, end = map(
        new_block, ("exception_external", "unwind_resume", "rethrow", "end")
    )

    b.position_before(fn.entry_block.terminator)
    pad_type = visitor.pad_type()
    unwind_type = visitor.mod.struct_type((i64,))  # Header only.
    if is_root:
        tc.exc_flag = b.create_alloca(i8)
        tc.catch_store = b.create_alloca(pad_type)
        tc.delegate_depth = b.create_alloca(i64)
        tc.ret_store = None if visitor.coro or ret_type.is_void else b.create_alloca(ret_type)
        tc.loop_sequence = b.create_alloca(i64)
        b.create_store(not_thrown, tc.exc_flag)
        b.create_store(pad_type.null(), tc.catch_store)
        b.create_store(zero, tc.delegate_depth)
        b.create_store(i64.const_int(-1), tc.loop_sequence)
    else:
        tc.exc_flag = parent.exc_flag
        tc.catch_store = parent.catch_store
        tc.delegate_depth = parent.delegate_depth
        tc.ret_store = parent.ret_store
        tc.loop_sequence = parent.loop_sequence

    # Emit finally before entering this try region: a return or exception
    # raised by the finally body is handled by the enclosing region.
    visitor.block = tc.finally_block
    b.set_insert_point(visitor.block)
    saved = []
    if have_finally:

        def save(storage, typ):
            value = b.create_load(typ, storage)
            saved.append((storage, value))
            return value

        pending_state = save(tc.exc_flag, i8)
        pending_exception = save(tc.catch_store, pad_type)
        save(tc.delegate_depth, i64)
        save(tc.loop_sequence, i64)
        if tc.ret_store:
            save(tc.ret_store, ret_type)
        b.create_store(not_thrown, tc.exc_flag)
        b.create_store(zero, tc.delegate_depth)

        active, body = map(new_block, ("finally.active", "finally.body"))
        has_exception = b.create_or(
            b.create_icmp(Predicate.EQ, pending_state, thrown),
            b.create_icmp(Predicate.EQ, pending_state, rethrow),
        )
        b.create_cond_br(has_exception, active, body)
        b.set_insert_point(active)
        unwind_exception = b.create_extract_value(pending_exception, 0)
        offset = i64.const_int(visitor.mod.lib.exception_offset())
        exception = b.create_load(ptr, b.create_gep(i8, unwind_exception, (offset,)))
        b.create_br(body)
        b.set_insert_point(body)
        active_exception = b.create_phi(ptr)
        outer = visitor.catches[-1].exception if visitor.catches else ptr.null()
        active_exception.add_incoming(((exception, active), (outer, tc.finally_block)))
        visitor.enter_region(visitor.catches, SimpleNamespace(exception=active_exception))
        visitor.block = body
    if node.finally_:
        visitor.process(node.finally_)
    if have_finally:
        visitor.catches.pop()
    finally_block = visitor.block
    b.set_insert_point(finally_block)
    for storage, value in saved:
        b.create_store(value, storage)
    exc_flag = b.create_load(i8, tc.exc_flag)

    if visitor.trycatch:
        depth = b.create_load(i64, tc.delegate_depth)
        delegate = b.create_icmp(Predicate.SGT, depth, zero)
        normal, delegated = map(new_block, ("finally.normal", "finally.delegate"))
        b.create_cond_br(delegate, delegated, normal)
        b.set_insert_point(delegated)
        new_depth = b.create_sub(depth, i64.const_int(1))
        delegate_again = b.create_icmp(Predicate.SGT, new_depth, zero)
        b.create_store(new_depth, tc.delegate_depth)
        enclosing_try = visitor.trycatch[-1]
        b.create_cond_br(
            delegate_again, enclosing_try.finally_block, enclosing_try.exception_route_block
        )
        finally_block = normal

    # Exceptions raised in except/else must run this finally before propagating.
    personality = visitor.personality()
    b.set_insert_point(tc.finally_exception_block)
    finally_caught = b.create_landing_pad(pad_type, personality, 1)
    finally_caught.set_cleanup()
    finally_caught.add_clause(visitor.type_idx_var(None))
    b.create_store(finally_caught, tc.catch_store)
    b.create_store(rethrow, tc.exc_flag)
    b.create_store(zero, tc.delegate_depth)
    b.create_br(tc.finally_block)

    b.set_insert_point(finally_block)
    switch = b.create_switch(exc_flag, end, 6 if support_loop_control else 4)
    switch.add_case(caught, end)
    switch.add_case(thrown, resume)
    switch.add_case(rethrow, rethrow_block)
    if is_root:
        finally_return = new_block("finally.return")
        switch.add_case(ret, finally_return)
        b.set_insert_point(finally_return)
        if visitor.coro:
            b.create_br(visitor.coro.exit)
        elif tc.ret_store:
            b.create_ret(b.create_load(ret_type, tc.ret_store))
        else:
            b.create_ret_void()
    else:
        switch.add_case(ret, parent.finally_block)

    if support_loop_control:
        previous_sequence = -1 if is_root else parent.sequence_number
        break_block, break_done, continue_block, continue_done = map(
            new_block,
            ("finally.break", "finally.break.done", "finally.continue", "finally.continue.done"),
        )

        def emit_loop_switch(start, done):
            b.set_insert_point(start)
            start_switch = b.create_switch(b.create_load(i64, tc.loop_sequence), end, 0)
            b.set_insert_point(done)
            b.create_store(not_thrown, tc.exc_flag)
            done_switch = b.create_switch(b.create_load(i64, tc.loop_sequence), end, 0)
            return start_switch, done_switch

        break_switch, break_done_switch = emit_loop_switch(break_block, break_done)
        continue_switch, continue_done_switch = emit_loop_switch(continue_block, continue_done)
        for loop in visitor.loops:
            seq = i64.const_int(loop.sequence_number)
            if not is_root and loop.sequence_number < previous_sequence:
                break_switch.add_case(seq, parent.finally_block)
                continue_switch.add_case(seq, parent.finally_block)
            else:
                break_switch.add_case(seq, break_done)
                break_done_switch.add_case(seq, loop.done)
                continue_switch.add_case(seq, continue_done)
                continue_done_switch.add_case(seq, loop.update)
        switch.add_case(brk, break_block)
        switch.add_case(cont, continue_block)

    catch_all = None
    for catch in node:
        handler = new_block("catch")
        tc.catch_types.append(catch.type)
        tc.handlers.append(handler)
        if not catch.type:
            assert catch_all is None, "duplicate catch-all"
            catch_all = handler

    if visitor.loops:
        # Reset pending state on re-entry from a loop.
        b.set_insert_point(entry)
        b.create_store(not_thrown, tc.exc_flag)
    visitor.block = entry
    if have_finally:
        visitor.enter_region(visitor.finally_, tc)
    visitor.enter_region(visitor.trycatch, tc)
    visitor.process(node.body)
    visitor.trycatch.pop()
    if node.else_:
        b.set_insert_point(visitor.block)
        else_block = new_block("else")
        b.create_br(else_block)
        visitor.block = else_block
        visitor.process(node.else_)
    b.set_insert_point(visitor.block)
    b.create_br(tc.finally_block)

    b.set_insert_point(resume)
    if is_root:
        b.create_resume(b.create_load(pad_type, tc.catch_store))
    else:
        b.create_br(rethrow_block)

    # Collect enclosing catch clauses, without delegating across a pending
    # finally or a catch-all.
    catch_types = list(tc.catch_types)
    handlers = list(tc.handlers)
    depths = [0] * len(catch_types)
    seen = {typ.name if typ else None for typ in catch_types}
    catch_all_depth = 0
    for depth, region in enumerate(reversed(visitor.trycatch), 1):
        if region.sequence_number < parent_finally_sequence or catch_all:
            break
        for typ, handler in zip(region.catch_types, region.handlers, strict=True):
            key = typ.name if typ else None
            if key in seen:
                continue
            seen.add(key)
            catch_types.append(typ)
            depths.append(depth)
            if typ is None and catch_all is None:
                catch_all = new_block("fdepth_catchall")
                b.set_insert_point(catch_all)
                b.create_store(i64.const_int(depth), tc.delegate_depth)
                b.create_br(handler)
                handlers.append(catch_all)
                catch_all_depth = depth
            else:
                handlers.append(handler)

    # Reject foreign exceptions and route Codon exceptions by selector.
    b.set_insert_point(tc.exception_block)
    result = b.create_landing_pad(pad_type, personality, len(node.catches))
    result.set_cleanup()
    for typ in catch_types:
        assert not typ or cast(RefType, typ), "invalid catch type"
        result.add_clause(visitor.type_idx_var(typ))
    unwind_exception = b.create_extract_value(result, 0)
    b.create_store(result, tc.catch_store)
    b.create_store(thrown, tc.exc_flag)
    b.create_store(zero, tc.delegate_depth)
    exception_class = b.create_load(i64, b.create_struct_gep(unwind_type, unwind_exception, 0))
    # codon/runtime/lib.h: SEQ_EXCEPTION_CLASS
    is_codon = b.create_icmp(Predicate.EQ, exception_class, i64.const_int(0x6F626A0073657100))
    b.create_cond_br(is_codon, tc.exception_route_block, external)
    b.set_insert_point(external)
    b.create_unreachable()

    b.set_insert_point(tc.exception_route_block)
    unwind_exception = b.create_extract_value(b.create_load(pad_type, tc.catch_store), 0)
    offset = i64.const_int(visitor.mod.lib.exception_offset())
    loaded_exc = b.create_load(ptr, b.create_gep(i8, unwind_exception, (offset,)))

    route = new_block("fdepth")
    b.set_insert_point(route)
    if catch_all:
        b.create_store(i64.const_int(catch_all_depth), tc.delegate_depth)
    b.create_br(catch_all if catch_all and catch_all_depth == 0 else tc.finally_block)
    b.set_insert_point(tc.exception_route_block)
    obj_type = b.create_extract_value(b.create_load(pad_type, tc.catch_store), 1)
    route_switch = b.create_switch(obj_type, route, len(handlers))
    for idx, (typ, handler, depth) in enumerate(zip(catch_types, handlers, depths, strict=True)):
        depth_set = new_block("fdepth")
        b.set_insert_point(depth_set)
        b.create_store(i64.const_int(depth), tc.delegate_depth)
        ours = idx < len(node.catches)
        b.create_br(handler if ours else tc.finally_block)
        if typ:
            route_switch.add_case(i32.const_int(visitor.type_idx(typ)), depth_set)
        if ours:
            catch = node.catches[idx]
            visitor.block = handler
            b.set_insert_point(handler)
            if catch.var:
                b.create_store(loaded_exc, visitor.vars[catch.var.id])
            b.create_store(caught, tc.exc_flag)
            visitor.enter_region(
                visitor.catches, SimpleNamespace(exception=loaded_exc, type_id=obj_type)
            )
            visitor.process(catch.handler)
            visitor.catches.pop()
            b.set_insert_point(visitor.block)
            b.create_br(tc.finally_block)

    if have_finally:
        visitor.finally_.pop()
    b.set_insert_point(rethrow_block)
    if have_finally or not is_root:
        unwind_exception = b.create_extract_value(b.create_load(pad_type, tc.catch_store), 0)
        visitor.block = rethrow_block
        visitor.call(visitor.throw_func(), (unwind_exception,))
        b.set_insert_point(visitor.block)
    b.create_unreachable()
    visitor.block = end
