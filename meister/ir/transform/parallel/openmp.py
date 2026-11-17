# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy
import sys
import warnings
from enum import IntEnum

from ....ast.types import mangle
from ....bridge import Dict, List, Tuple, dataclass
from ... import (
    AssignInstr,
    BodiedFunc,
    BoolConst,
    CallInstr,
    Float32Type,
    FloatType,
    FlowInstr,
    IfFlow,
    ImperativeForFlow,
    IntConst,
    Intrinsic,
    PointerType,
    PointerValue,
    SeriesFlow,
    StackAllocInstr,
    TernaryInstr,
    TryCatchFlow,
    Var,
    VarValue,
    cast,
    is_a,
)
from ...util import irtools as util
from ...util.cloning import CloneVisitor
from ...util.operator import Operator
from ...util.outlining import OutlineResult, outline_region
from ..pass_ import OperatorPass

OMP_MODULE = "std.openmp"
GPU_MODULE = "std.internal.gpu"
BUILTIN_MODULE = "std.internal.builtin"


# C++: codon/cir/transform/parallel/openmp.cpp:24
def warn(message, value):
    """C++: openmp.cpp:26. Compilation warning at the loop's source location."""
    source = value.info
    warnings.warn_explicit(message, UserWarning, source.file, source.line)


@dataclass(init=False, eq=False)
class OMPTypes:
    """C++: codon/cir/transform/parallel/openmp.cpp:29. Common OpenMP ABI types."""

    i64: object
    i32: object
    i8ptr: object
    i32ptr: object

    # C++: codon/cir/transform/parallel/openmp.cpp:35
    def __init__(self, module):
        self.i64 = module.get_int_type()
        self.i32 = module.get_int_type(32, True)
        self.i8ptr = module.get_pointer_type()
        self.i32ptr = module.get_pointer_type(self.i32)


# C++: codon/cir/transform/parallel/openmp.cpp:43
def get_var_from_outlined_arg(arg):
    if isinstance(arg.get_actual(), (VarValue, PointerValue)):
        return arg.get_actual().var
    assert False, "unknown outline var"


# C++: codon/cir/transform/parallel/openmp.cpp:54
def ptr_from_func(fn):
    module = fn.module
    raw = module.get_or_realize_method(fn.type, "__raw__", [fn.type])
    assert raw, "cannot find function __raw__ method"
    return util.call(raw, [module.nr(VarValue, fn)])


_lock_counter = 1


@dataclass(init=False, eq=False)
class ReductionLocks:
    """C++: codon/cir/transform/parallel/openmp.cpp:63. Create locks lazily, only when needed."""

    main_lock: Var | None
    crit_lock: Var | None

    # C++ API: codon/cir/transform/parallel/openmp.cpp:63
    def __init__(self):
        # Main lock for _reduce_no_wait/_end_reduce_no_wait; critical-section lock.
        self.main_lock = self.crit_lock = None

    # C++: codon/cir/transform/parallel/openmp.cpp:68
    def create_lock(self, module):
        global _lock_counter

        typ = module.get_or_realize_type(mangle(OMP_MODULE, cls="Lock"))
        assert typ, "openmp.Lock type not found"
        var = module.nr(Var, typ, True)
        var.name = f".omp_lock.{_lock_counter}"
        _lock_counter += 1
        # Add to main so an IR pass cannot demote it.
        body = cast(SeriesFlow, module.main_func.body)
        init = typ()
        assert init, "could not initialize openmp.Lock"
        body.series.insert(0, module.nr(AssignInstr, var, init))
        return var

    # C++: codon/cir/transform/parallel/openmp.cpp:84
    def get_main_lock(self, module):
        if self.main_lock is None:
            self.main_lock = self.create_lock(module)
        return self.main_lock

    # C++: codon/cir/transform/parallel/openmp.cpp:90
    def get_crit_lock(self, module):
        if self.crit_lock is None:
            self.crit_lock = self.create_lock(module)
        return self.crit_lock


class ReductionKind(IntEnum):
    NONE = 0
    ADD = 1
    SUB = 2
    MUL = 3
    AND = 4
    OR = 5
    XOR = 6
    MIN = 7
    MAX = 8


@dataclass(init=False, eq=False)
class Reduction:
    """C++: codon/cir/transform/parallel/openmp.cpp:97. Reduction identity, operator, and atomic fallback."""

    kind: ReductionKind
    shared: Var | None

    Kind = ReductionKind

    # C++ API: codon/cir/transform/parallel/openmp.cpp:97
    def __init__(self, kind=ReductionKind.NONE, shared=None):
        self.kind, self.shared = kind, shared

    # C++: codon/cir/transform/parallel/openmp.cpp:384
    def __bool__(self):
        return self.kind != ReductionKind.NONE

    # C++: codon/cir/transform/parallel/openmp.cpp:113
    def get_type(self):
        pointer = cast(PointerType, self.shared.type)
        assert pointer, "expected shared var to be of pointer type"
        return pointer.base

    # C++: codon/cir/transform/parallel/openmp.cpp:119
    def get_initial(self):
        if not self:
            return None
        module, typ, kind = self.shared.module, self.get_type(), self.kind
        if util.is_int(typ):
            values = {
                ReductionKind.ADD: 0,
                ReductionKind.SUB: 0,
                ReductionKind.MUL: 1,
                ReductionKind.AND: ~0,
                ReductionKind.OR: 0,
                ReductionKind.XOR: 0,
                ReductionKind.MIN: (1 << 63) - 1,
                ReductionKind.MAX: -(1 << 63),
            }
            return module.get_int(values[kind]) if kind in values else None
        if is_a(FloatType, typ):
            values = {
                ReductionKind.ADD: 0.0,
                ReductionKind.SUB: 0.0,
                ReductionKind.MUL: 1.0,
                ReductionKind.MIN: sys.float_info.max,
                ReductionKind.MAX: sys.float_info.min,
            }
            return module.get_float(values[kind]) if kind in values else None
        if is_a(Float32Type, typ):
            float32 = module.get_or_realize_type("float32")
            values = {
                ReductionKind.ADD: 0.0,
                ReductionKind.SUB: 0.0,
                ReductionKind.MUL: 1.0,
                ReductionKind.MIN: 3.4028234663852886e38,
                ReductionKind.MAX: 1.1754943508222875e-38,
            }
            return float32(module.get_float(values[kind])) if kind in values else None
        if typ.is_type(module.get_bool_type()):
            if kind == ReductionKind.AND:
                return module.get_bool(True)
            if kind in (ReductionKind.OR, ReductionKind.XOR):
                return module.get_bool(False)
            return None
        init = typ()
        return init if init and init.type.is_type(typ) else None

    # C++: codon/cir/transform/parallel/openmp.cpp:204
    def generate_non_atomic_reduction(self, pointer, arg):
        module, lhs = pointer.module, util.ptr_load(pointer)
        match self.kind:
            case ReductionKind.ADD | ReductionKind.SUB:
                result = lhs + arg
            case ReductionKind.MUL:
                result = lhs * arg
            case ReductionKind.AND:
                result = lhs & arg
            case ReductionKind.OR:
                result = lhs | arg
            case ReductionKind.XOR:
                result = lhs ^ arg
            case ReductionKind.MIN | ReductionKind.MAX:
                # Signature is (tuple of args, key, default).
                name = "min" if self.kind == ReductionKind.MIN else "max"
                tuple_ = util.make_tuple([lhs, arg])
                none = module.get_none_type()()
                fn = module.get_or_realize_func(
                    name, [tuple_.type, none.type, none.type], [], BUILTIN_MODULE
                )
                assert fn, f"{name} function not found"
                result = util.call(fn, [tuple_, none, none])
            case _:
                return None
        return util.ptr_store(pointer, result)

    # C++: codon/cir/transform/parallel/openmp.cpp:245
    def generate_atomic_reduction(self, pointer, arg, loc, gtid, locks):
        module, typ, name = pointer.module, self.get_type(), ""
        suffixes = {
            ReductionKind.ADD: "add",
            ReductionKind.SUB: "add",
            ReductionKind.MUL: "mul",
            ReductionKind.AND: "and",
            ReductionKind.OR: "or",
            ReductionKind.XOR: "xor",
            ReductionKind.MIN: "min",
            ReductionKind.MAX: "max",
        }
        if util.is_int(typ) and self.kind in suffixes:
            name = "_atomic_int_" + suffixes[self.kind]
        elif is_a(FloatType, typ) and self.kind in (
            ReductionKind.ADD,
            ReductionKind.MUL,
            ReductionKind.MIN,
            ReductionKind.MAX,
        ):
            name = "_atomic_float_" + suffixes[self.kind]
        elif is_a(Float32Type, typ) and self.kind in (
            ReductionKind.ADD,
            ReductionKind.SUB,
            ReductionKind.MUL,
            ReductionKind.MIN,
            ReductionKind.MAX,
        ):
            name = "_atomic_float32_" + suffixes[self.kind]
        if name:
            fn = module.get_or_realize_func(name, [pointer.type, arg.type], [], OMP_MODULE)
            assert fn, f"atomic op '{name}' not found"
            return util.call(fn, [pointer, arg])
        name = "__atomic_" + suffixes[self.kind] + "__" if self.kind in suffixes else ""
        if name:
            fn = module.get_or_realize_method(arg.type, name, [pointer.type, arg.type])
            if fn:
                return util.call(fn, [pointer, arg])
        assert loc and gtid, "loc and/or gtid are null"
        lock = locks.get_crit_lock(module)
        lock_type = module.get_pointer_type(lock.type)
        begin = module.get_or_realize_func(
            "_critical_begin", [loc.type, gtid.type, lock_type], [], OMP_MODULE
        )
        assert begin, "critical begin function not found"
        end = module.get_or_realize_func(
            "_critical_end", [loc.type, gtid.type, lock_type], [], OMP_MODULE
        )
        assert end, "critical end function not found"
        enter = util.call(
            begin,
            [module.nr(VarValue, loc), module.nr(VarValue, gtid), module.nr(PointerValue, lock)],
        )
        operation = self.generate_non_atomic_reduction(pointer, arg)
        leave = util.call(
            end,
            [module.nr(VarValue, loc), module.nr(VarValue, gtid), module.nr(PointerValue, lock)],
        )
        # Unlock in a finally block, including exceptional exits.
        return util.series(
            enter, module.nr(TryCatchFlow, util.series(operation), util.series(leave))
        )


@dataclass(init=False, eq=False)
class ReductionIdentifier(Operator):
    """C++: codon/cir/transform/parallel/openmp.cpp:393. Recognize consistent shared-variable reductions."""

    shareds: List[Var]
    loop_var_arg: Var | None
    reductions: Dict[int, Reduction]

    # C++: codon/cir/transform/parallel/openmp.cpp:398
    def __init__(self, shareds=None, loop_var_arg=None):
        super().__init__()
        self.shareds = [] if shareds is None else list(shareds)
        self.loop_var_arg, self.reductions = loop_var_arg, {}

    # C++: codon/cir/transform/parallel/openmp.cpp:405
    def is_shared(self, shared):
        if self.loop_var_arg and shared.id == self.loop_var_arg.id:
            return False
        return any(shared.id == var.id for var in self.shareds)

    # C++: codon/cir/transform/parallel/openmp.cpp:415
    def is_shared_deref(self, shared, value):
        module = value.module
        pointer = cast(PointerType, shared.type)
        assert pointer, "expected shared var to be of pointer type"
        if util.is_call_of(
            value, module.GETITEM_MAGIC_NAME, [pointer, module.get_int_type()], pointer.base, True
        ):
            call_ = cast(CallInstr, value)
            var = util.get_var(call_.args[0])
            index = cast(IntConst, call_.args[-1])
            return index is not None and index.val == 0 and var is not None and var.id == shared.id
        return False

    # C++: codon/cir/transform/parallel/openmp.cpp:432
    @staticmethod
    def extract_associative_op_chain(value, op, typ, result):
        if util.is_call_of(value, op, [typ, None], typ, True) or util.is_call_of(
            value, op, [None, typ], typ, True
        ):
            call_ = cast(CallInstr, value)
            ReductionIdentifier.extract_associative_op_chain(call_.args[0], op, typ, result)
            ReductionIdentifier.extract_associative_op_chain(call_.args[-1], op, typ, result)
        else:
            result.append(value)

    # C++: codon/cir/transform/parallel/openmp.cpp:444
    def is_shared_deref_for_reduction_cond(self, shared, value):
        if self.is_shared_deref(shared, value):
            return True
        # Unwrap only flow(series(assign tmp = shared_deref), tmp).
        if flow_ := cast(FlowInstr, value):
            body, ret = cast(SeriesFlow, flow_.flow), cast(VarValue, flow_.value)
            if not body or not ret or len(body.series) != 1:
                return False
            assignment = cast(AssignInstr, body.series[0])
            if not assignment or assignment.lhs.id != ret.var.id:
                return False
            return self.is_shared_deref(shared, assignment.rhs)
        return False

    # C++: codon/cir/transform/parallel/openmp.cpp:466
    @staticmethod
    def get_condition_result_var(value):
        if flow_ := cast(FlowInstr, value):
            if ret := cast(VarValue, flow_.value):
                return ret.var
        ret = cast(VarValue, value)
        return ret.var if ret else None

    # C++: codon/cir/transform/parallel/openmp.cpp:477
    @staticmethod
    def is_condition_result(cond, value):
        var, other = ReductionIdentifier.get_condition_result_var(cond), cast(VarValue, value)
        return var is not None and other is not None and var.id == other.var.id

    # C++: codon/cir/transform/parallel/openmp.cpp:483
    @staticmethod
    def is_logical_false_value(cond, value):
        constant = cast(BoolConst, value)
        return (
            constant is not None and not constant.val
        ) or ReductionIdentifier.is_condition_result(cond, value)

    # C++: codon/cir/transform/parallel/openmp.cpp:487
    @staticmethod
    def is_logical_true_value(cond, value):
        constant = cast(BoolConst, value)
        return (constant is not None and constant.val) or ReductionIdentifier.is_condition_result(
            cond, value
        )

    # C++: codon/cir/transform/parallel/openmp.cpp:491
    def get_reduction_from_ternary(self, shared, typ, item):
        if not typ.is_type(item.module.get_bool_type()):
            return Reduction()
        ternary = cast(TernaryInstr, item)
        if ternary is None:
            return Reduction()
        cond = ternary.cond
        kind = ReductionKind.NONE
        if (
            self.is_shared_deref_for_reduction_cond(shared, cond)
            and self.is_logical_false_value(cond, ternary.false_value)
            and ternary.true_value.type.is_type(typ)
        ):
            kind = ReductionKind.AND
        elif (
            self.is_shared_deref_for_reduction_cond(shared, cond)
            and self.is_logical_true_value(cond, ternary.true_value)
            and ternary.false_value.type.is_type(typ)
        ):
            kind = ReductionKind.OR
        elif self.is_shared_deref_for_reduction_cond(
            shared, ternary.true_value
        ) and self.is_logical_false_value(cond, ternary.false_value):
            kind = ReductionKind.AND
        elif self.is_shared_deref_for_reduction_cond(
            shared, ternary.false_value
        ) and self.is_logical_true_value(cond, ternary.true_value):
            kind = ReductionKind.OR
        reduction = Reduction(kind, shared)
        return reduction if reduction.get_initial() else Reduction()

    # C++: codon/cir/transform/parallel/openmp.cpp:531
    def get_reduction_from_call(self, value):
        module, fn = value.module, util.get_func(value.callee)
        if len(value.args) != 3 or not fn or fn.unmangled_name != module.SETITEM_MAGIC_NAME:
            return Reduction()
        self_, idx, item = value.args
        shared, constant = util.get_var(self_), cast(IntConst, idx)
        if not shared or not self.is_shared(shared) or constant is None or constant.val != 0:
            return Reduction()
        pointer = cast(PointerType, shared.type)
        assert pointer, "expected shared var to be of pointer type"
        typ, none = pointer.base, module.get_none_type()
        # Double-check the store call's exact signature.
        if not util.is_call_of(
            value,
            module.SETITEM_MAGIC_NAME,
            [self_.type, idx.type, item.type],
            module.get_none_type(),
            True,
        ):
            return Reduction()
        operators = [
            (module.ADD_MAGIC_NAME, ReductionKind.ADD, True),
            (module.SUB_MAGIC_NAME, ReductionKind.SUB, True),
            (module.MUL_MAGIC_NAME, ReductionKind.MUL, True),
            (module.AND_MAGIC_NAME, ReductionKind.AND, True),
            (module.OR_MAGIC_NAME, ReductionKind.OR, True),
            (module.XOR_MAGIC_NAME, ReductionKind.XOR, True),
            ("min", ReductionKind.MIN, False),
            ("max", ReductionKind.MAX, False),
        ]
        for name, kind, method in operators:
            if method:
                if not (
                    util.is_call_of(item, name, [typ, None], typ, True)
                    or util.is_call_of(item, name, [None, typ], typ, True)
                ):
                    continue
            elif not util.is_call_of(
                item, name, [module.get_tuple_type([typ, typ]), none, none], typ, False
            ):
                continue
            rhs = cast(CallInstr, item)
            if method:
                chain = []
                self.extract_associative_op_chain(rhs, name, typ, chain)
                if len(chain) < 2:
                    continue
            else:
                rhs = cast(CallInstr, rhs.args[0])  # Tuple.__new__
                if rhs is None:
                    continue
                chain = rhs.args
            deref = next((arg for arg in chain if self.is_shared_deref(shared, arg)), None)
            if deref is None:
                return Reduction()
            reduction = Reduction(kind, shared)
            return reduction if reduction.get_initial() else Reduction()
        return self.get_reduction_from_ternary(shared, typ, item)

    # C++: codon/cir/transform/parallel/openmp.cpp:622
    def get_reduction(self, shared):
        return self.reductions.get(shared.id, Reduction())

    # C++: codon/cir/transform/parallel/openmp.cpp:627
    def handle_CallInstr(self, value):
        reduction = self.get_reduction_from_call(value)
        if reduction:
            # Require consistency with every previously seen update. An empty
            # reduction permanently marks an inconsistent variable invalid.
            identifier = reduction.shared.id
            if identifier not in self.reductions:
                self.reductions[identifier] = reduction
            elif self.reductions[identifier] and self.reductions[identifier].kind != reduction.kind:
                self.reductions[identifier] = Reduction()


@dataclass(init=False, eq=False)
class SharedInfo:
    """C++: codon/cir/transform/parallel/openmp.cpp:641. Extra-tuple member, local value and reduction."""

    memb: int
    local: Var
    reduction: Reduction

    # C++ API: codon/cir/transform/parallel/openmp.cpp:641
    def __init__(self, memb, local, reduction):
        self.memb, self.local, self.reduction = memb, local, reduction


@dataclass(init=False, eq=False)
class LoopTemplateReplacer(Operator):
    parent: BodiedFunc
    replacement: CallInstr | None
    loop_var: Var

    # C++: codon/cir/transform/parallel/openmp.cpp:652
    def __init__(self, parent, replacement, loop_var):
        super().__init__()
        self.parent, self.replacement, self.loop_var = parent, replacement, loop_var


@dataclass(init=False, eq=False)
class ParallelLoopTemplateReplacer(LoopTemplateReplacer):
    """C++: codon/cir/transform/parallel/openmp.cpp:656. Materialize shared storage and reduction callbacks."""

    reds: ReductionIdentifier
    shared_info: List[SharedInfo]
    locks: ReductionLocks
    loc_ref: Var | None
    reduction_loc_ref: Var | None
    gtid: Var | None

    # C++: codon/cir/transform/parallel/openmp.cpp:664
    def __init__(self, parent, replacement, loop_var, reds):
        super().__init__(parent, replacement, loop_var)
        self.reds, self.shared_info, self.locks = reds, [], ReductionLocks()
        self.loc_ref = self.reduction_loc_ref = self.gtid = None

    # C++: codon/cir/transform/parallel/openmp.cpp:669
    def num_reductions(self):
        return sum(bool(info.reduction) for info in self.shared_info)

    # C++: codon/cir/transform/parallel/openmp.cpp:678
    def get_reduction_tuple(self):
        module = self.parent.module
        return util.make_tuple(
            [module.nr(PointerValue, info.local) for info in self.shared_info if info.reduction],
            module,
        )

    # C++: codon/cir/transform/parallel/openmp.cpp:688
    def make_reduction_func(self):
        module = self.parent.module
        arg_type = module.get_pointer_type(self.get_reduction_tuple().type)
        signature = module.get_func_type(module.get_none_type(), [arg_type, arg_type])
        reducer = module.nr(BodiedFunc, util.get_derived_func_name("__omp_reducer", self.parent))
        reducer.realize(signature, ["lhs", "rhs"])
        lhs_var, rhs_var = reducer.args[0], reducer.args[-1]
        body, idx = module.nr(SeriesFlow), 0
        for info in self.shared_info:
            if info.reduction:
                lhs = util.ptr_load(module.nr(VarValue, lhs_var))
                rhs = util.ptr_load(module.nr(VarValue, rhs_var))
                lhs_elem, rhs_elem = util.tuple_get(lhs, idx), util.tuple_get(rhs, idx)
                body.push_back(
                    info.reduction.generate_non_atomic_reduction(lhs_elem, util.ptr_load(rhs_elem))
                )
                idx += 1
        reducer.body = body
        return reducer

    # C++: codon/cir/transform/parallel/openmp.cpp:716
    def handle_CallInstr(self, value):
        module, fn = value.module, util.get_func(value.callee)
        if not fn:
            return
        name = fn.unmangled_name
        if name == "_loop_loc_and_gtid":
            assert len(value.args) == 3 and all(is_a(VarValue, arg) for arg in value.args), (
                "unexpected loop loc and gtid stub"
            )
            self.loc_ref, self.reduction_loc_ref, self.gtid = [
                util.get_var(arg) for arg in value.args
            ]
        if name == "_loop_reductions":
            assert self.reduction_loc_ref and self.gtid, "bad visit order in template"
            assert len(value.args) == 1 and is_a(VarValue, value.args[0]), (
                "unexpected shared updates stub"
            )
            if self.num_reductions() == 0:
                return
            module, extras = self.parent.module, util.get_var(value.args[0])
            prefix = module.nr(SeriesFlow)
            for info in self.shared_info:
                if info.reduction:
                    info.local = util.make_var(module.nr(VarValue, info.local), prefix, self.parent)
            reduction_tuple = self.get_reduction_tuple()
            reducer = self.make_reduction_func()
            lock = self.locks.get_main_lock(module)
            raw_reducer = ptr_from_func(reducer)
            lock_type = module.get_pointer_type(lock.type)
            reduce = module.get_or_realize_func(
                "_reduce_nowait",
                [
                    self.reduction_loc_ref.type,
                    self.gtid.type,
                    reduction_tuple.type,
                    raw_reducer.type,
                    lock_type,
                ],
                [],
                OMP_MODULE,
            )
            assert reduce, "reduce nowait function not found"
            end = module.get_or_realize_func(
                "_end_reduce_nowait",
                [self.reduction_loc_ref.type, self.gtid.type, lock_type],
                [],
                OMP_MODULE,
            )
            assert end, "end reduce nowait function not found"
            tuple_var = util.make_var(reduction_tuple, prefix, self.parent)
            code = util.call(
                reduce,
                [
                    module.nr(VarValue, self.reduction_loc_ref),
                    module.nr(VarValue, self.gtid),
                    module.nr(VarValue, tuple_var),
                    raw_reducer,
                    module.nr(PointerValue, lock),
                ],
            )
            code_var = util.make_var(code, prefix, self.parent)
            assert code_var.type.is_type(module.get_int_type()), "wrong reduce code type"
            non_atomic, atomic = module.nr(SeriesFlow), module.nr(SeriesFlow)
            for info in self.shared_info:
                if info.reduction:
                    ptr, arg = (
                        util.tuple_get(module.nr(VarValue, extras), info.memb),
                        module.nr(VarValue, info.local),
                    )
                    non_atomic.push_back(info.reduction.generate_non_atomic_reduction(ptr, arg))
            non_atomic.push_back(
                util.call(
                    end,
                    [
                        module.nr(VarValue, self.reduction_loc_ref),
                        module.nr(VarValue, self.gtid),
                        module.nr(PointerValue, lock),
                    ],
                )
            )
            for info in self.shared_info:
                if info.reduction:
                    ptr, arg = (
                        util.tuple_get(module.nr(VarValue, extras), info.memb),
                        module.nr(VarValue, info.local),
                    )
                    atomic.push_back(
                        info.reduction.generate_atomic_reduction(
                            ptr, arg, self.loc_ref, self.gtid, self.locks
                        )
                    )
            # if code == 1: non_atomic; elif code == 2: atomic
            prefix.push_back(
                module.nr(
                    IfFlow,
                    module.nr(VarValue, code_var).eq(module.get_int(1)),
                    non_atomic,
                    util.series(
                        module.nr(
                            IfFlow, module.nr(VarValue, code_var).eq(module.get_int(2)), atomic
                        )
                    ),
                )
            )
            value.replace_all(prefix)


@dataclass(init=False, eq=False)
class ImperativeLoopTemplateReplacer(ParallelLoopTemplateReplacer):
    """C++: codon/cir/transform/parallel/openmp.cpp:808. Integer-loop template stubs and lastprivate updates."""

    sched: object
    step: int

    # C++: codon/cir/transform/parallel/openmp.cpp:812
    def __init__(self, parent, replacement, loop_var, reds, schedule, step):
        super().__init__(parent, replacement, loop_var, reds)
        self.sched, self.step = schedule, step

    # C++: codon/cir/transform/parallel/openmp.cpp:818
    def handle_CallInstr(self, value):
        super().handle_CallInstr(value)
        module, fn = value.module, util.get_func(value.callee)
        if not fn:
            return
        name = fn.unmangled_name
        if name == "_loop_step":
            value.replace_all(module.get_int(self.step))
        if name == "_loop_body_stub":
            assert self.replacement, "unexpected double replacement"
            assert (
                len(value.args) == 2
                and is_a(VarValue, value.args[0])
                and is_a(VarValue, value.args[-1])
            ), "unexpected loop body stub"
            outlined = util.get_func(self.replacement.callee)
            # Template passes the new loop variable and extra arguments.
            new_loop_var, extras = util.get_var(value.args[0]), util.get_var(value.args[-1])
            args, idx = [], 0
            for arg, outlined_arg in zip(self.replacement.args, outlined.args):
                if get_var_from_outlined_arg(arg).id != self.loop_var.id:
                    if is_a(PointerValue, arg):
                        base = cast(PointerType, arg.type).base
                        # Read extras again: the new local is prepended before
                        # the template's own extras local initialization.
                        last_arg = self.parent.args[-1]
                        val = util.tuple_get(util.ptr_load(module.nr(VarValue, last_arg)), 3)
                        init = util.ptr_load(util.tuple_get(val, idx))
                        reduction = self.reds.get_reduction(outlined_arg)
                        if reduction:
                            init = reduction.get_initial()
                            assert init and init.type.is_type(base), "unknown reduction init value"
                        new_var = util.make_var(
                            init, cast(SeriesFlow, self.parent.body), self.parent, True
                        )
                        self.shared_info.append(SharedInfo(idx, new_var, reduction))
                        new_arg = module.nr(PointerValue, new_var)
                    else:
                        new_arg = util.tuple_get(module.nr(VarValue, extras), idx)
                    idx += 1
                    args.append(new_arg)
                elif is_a(VarValue, arg):
                    args.append(module.nr(VarValue, new_loop_var))
                elif is_a(PointerValue, arg):
                    args.append(module.nr(PointerValue, new_loop_var))
                else:
                    assert False, "unknown outline var"
            value.replace_all(util.call(outlined, args))
            self.replacement = None
        if name == "_loop_shared_updates":
            # Non-reduction shareds receive final values, like OpenMP lastprivate.
            assert len(value.args) == 1 and is_a(VarValue, value.args[0]), (
                "unexpected shared updates stub"
            )
            extras = util.get_var(value.args[0])
            prefix = module.nr(SeriesFlow)
            for info in self.shared_info:
                if not info.reduction:
                    final = module.nr(VarValue, info.local)
                    original = util.tuple_get(module.nr(VarValue, extras), info.memb)
                    prefix.push_back(util.ptr_store(original, final))
            value.replace_all(prefix)
        if name == "_loop_schedule":
            value.replace_all(module.get_int(self.sched.code))
        if name == "_loop_ordered":
            value.replace_all(module.get_bool(self.sched.ordered))


@dataclass(init=False, eq=False)
class TaskLoopReductionVarReplacer(Operator):
    """C++: codon/cir/transform/parallel/openmp.cpp:926. Resolve task reduction storage without remapping RHS."""

    reduction_args: List[Var]
    reduction_remap: List[Tuple[Var, Var]]
    parent: BodiedFunc

    # C++: codon/cir/transform/parallel/openmp.cpp:940
    def __init__(self, reduction_args, parent):
        super().__init__()
        self.reduction_args, self.parent, self.reduction_remap = list(reduction_args), parent, []
        self.setup_reduction_remap()

    # C++: codon/cir/transform/parallel/openmp.cpp:931
    def setup_reduction_remap(self):
        module = self.parent.module
        for var in self.reduction_args:
            self.reduction_remap.append((var, module.nr(Var, var.type, False)))

    # C++: codon/cir/transform/parallel/openmp.cpp:946
    def pre_hook(self, value):
        for old, new in self.reduction_remap:
            value.replace_used_variable(old.id, new)

    # C++: codon/cir/transform/parallel/openmp.cpp:954
    def finalize(self):
        # Separate step: replacing the old variable in these RHSs is undesired.
        module, body, gtid = (
            self.parent.module,
            cast(SeriesFlow, self.parent.body),
            self.parent.args[-1],
        )
        for old, new in self.reduction_remap:
            data = module.get_or_realize_func(
                "_taskred_data", [module.get_int_type(), old.type], [], OMP_MODULE
            )
            assert data, "could not find '_taskred_data'"
            assignment = module.nr(
                AssignInstr,
                new,
                util.call(data, [module.nr(VarValue, gtid), module.nr(VarValue, old)]),
            )
            body.series.insert(0, assignment)
            self.parent.symbols.append(new)


@dataclass(init=False, eq=False)
class TaskLoopBodyStubReplacer(Operator):
    """C++: codon/cir/transform/parallel/openmp.cpp:973. Task body template's private/shared arguments."""

    replacement: CallInstr | None
    reduce_args: List[bool]

    # C++: codon/cir/transform/parallel/openmp.cpp:977
    def __init__(self, replacement, reduce_args):
        super().__init__()
        self.replacement, self.reduce_args = replacement, list(reduce_args)

    # C++: codon/cir/transform/parallel/openmp.cpp:980
    def handle_CallInstr(self, value):
        fn = util.get_func(value.callee)
        if not fn or fn.unmangled_name != "_task_loop_body_stub":
            return
        assert self.replacement, "unexpected double replacement"
        assert (
            len(value.args) == 3
            and is_a(VarValue, value.args[0])
            and is_a(VarValue, value.args[-1])
        ), "unexpected loop body stub"
        # Template passes gtid, private tuple and shared tuple for convenience.
        gtid, privates, shareds = value.args
        private_idx, shared_idx, args = 0, 0, []
        reductions = any(self.reduce_args)
        for arg in self.replacement.args:
            if is_a(VarValue, arg):
                args.append(util.tuple_get(privates, private_idx))
                private_idx += 1
            elif is_a(PointerValue, arg):
                args.append(util.tuple_get(shareds, shared_idx))
                shared_idx += 1
            else:
                # Only the final gtid argument can be neither a value nor pointer.
                assert reductions and arg is self.replacement.args[-1], "unknown outline var"
        outlined = cast(BodiedFunc, util.get_func(self.replacement.callee))
        if reductions:
            args.append(gtid)
            reduction_args = [arg for arg, reduce in zip(outlined.args, self.reduce_args) if reduce]
            replacer = TaskLoopReductionVarReplacer(reduction_args, outlined)
            replacer.process(outlined)
            replacer.finalize()
        value.replace_all(util.call(outlined, args))
        self.replacement = None


@dataclass(init=False, eq=False)
class TaskLoopRoutineStubReplacer(ParallelLoopTemplateReplacer):
    """C++: codon/cir/transform/parallel/openmp.cpp:1034. Task-group setup, inputs, routine and teardown."""

    privates: List[object]
    shareds: List[object]
    array: Var | None
    tskgrp: Var | None

    # C++: codon/cir/transform/parallel/openmp.cpp:1055
    def __init__(self, parent, replacement, loop_var, reds, privates, shareds, shared_redux):
        super().__init__(parent, replacement, loop_var, reds)
        self.privates, self.shareds = list(privates), list(shareds)
        # Task reduction input array and task group identifier.
        self.array = self.tskgrp = None
        self.setup_shared_info(shared_redux)

    # C++: codon/cir/transform/parallel/openmp.cpp:1040
    def setup_shared_info(self, shared_redux):
        for idx, value in enumerate(self.shareds):
            if get_var_from_outlined_arg(value).id != self.loop_var.id:
                reduction = shared_redux[idx]
                if reduction:
                    local = util.make_var(
                        reduction.get_initial(),
                        cast(SeriesFlow, self.parent.body),
                        self.parent,
                        True,
                    )
                    self.shared_info.append(SharedInfo(idx, local, reduction))

    # C++: codon/cir/transform/parallel/openmp.cpp:1065
    def make_task_red_init_func(self, reduction):
        module = self.parent.module
        arg_type = module.get_pointer_type(reduction.get_type())
        signature = module.get_func_type(module.get_none_type(), [arg_type, arg_type])
        fn = module.nr(
            BodiedFunc, util.get_derived_func_name("__red_init", self.parent, reduction.shared)
        )
        fn.realize(signature, ["lhs", "rhs"])
        body = module.nr(SeriesFlow)
        body.push_back(util.ptr_store(module.nr(VarValue, fn.args[0]), reduction.get_initial()))
        fn.body = body
        return fn

    # C++: codon/cir/transform/parallel/openmp.cpp:1081
    def make_task_red_comb_func(self, reduction):
        module = self.parent.module
        arg_type = module.get_pointer_type(reduction.get_type())
        signature = module.get_func_type(module.get_none_type(), [arg_type, arg_type])
        fn = module.nr(
            BodiedFunc, util.get_derived_func_name("__red_comb", self.parent, reduction.shared)
        )
        fn.realize(signature, ["lhs", "rhs"])
        lhs, rhs = fn.args[0], fn.args[-1]
        body = module.nr(SeriesFlow)
        lhs_ptr, rhs_ptr = module.nr(VarValue, lhs), module.nr(VarValue, rhs)
        body.push_back(reduction.generate_non_atomic_reduction(lhs_ptr, util.ptr_load(rhs_ptr)))
        fn.body = body
        return fn

    # C++: codon/cir/transform/parallel/openmp.cpp:1100
    def make_task_red_input(self, reduction, shared, original):
        module = shared.module
        size = util.call(module.get_intrinsic(Intrinsic.SIZEOF, reduction.get_type()), [])
        init = ptr_from_func(self.make_task_red_init_func(reduction))
        combine = ptr_from_func(self.make_task_red_comb_func(reduction))
        typ = module.get_or_realize_type(mangle(OMP_MODULE, cls="TaskReductionInput"))
        assert typ, "could not find 'TaskReductionInput' type"
        result = typ.construct([shared, original, size, init, combine])
        assert result, "bad construction of 'TaskReductionInput' type"
        return result

    # C++: codon/cir/transform/parallel/openmp.cpp:1115
    def handle_VarValue(self, value):
        module, fn = value.module, util.get_func(value)
        if not fn or fn.unmangled_name != "_routine_stub":
            return
        reduce_args, shared_idx, info_idx = [], 0, 0
        for arg in self.replacement.args:
            if is_a(VarValue, arg):
                reduce_args.append(False)
            elif is_a(PointerValue, arg):
                if (
                    info_idx < len(self.shared_info)
                    and self.shared_info[info_idx].memb == shared_idx
                    and self.shared_info[info_idx].reduction
                ):
                    reduce_args.append(True)
                    info_idx += 1
                else:
                    reduce_args.append(False)
                shared_idx += 1
            else:
                # The only other argument is the final gtid for reductions.
                assert self.num_reductions() > 0 and arg is self.replacement.args[-1], (
                    "unknown outline var"
                )
                reduce_args.append(False)
        visitor = CloneVisitor(module)
        routine = visitor.force_clone(fn)
        routine.name = util.get_derived_func_name(fn.name, self.parent)
        TaskLoopBodyStubReplacer(self.replacement, reduce_args).process(routine)
        value.var = routine

    # C++: codon/cir/transform/parallel/openmp.cpp:1155
    def handle_CallInstr(self, value):
        super().handle_CallInstr(value)
        module, fn = value.module, util.get_func(value.callee)
        if not fn:
            return
        name = fn.unmangled_name
        if name == "_taskred_setup":
            assert self.reduction_loc_ref and self.gtid, "bad visit order in template"
            assert len(value.args) == 1 and is_a(VarValue, value.args[0]), (
                "unexpected shared updates stub"
            )
            count = self.num_reductions()
            if count == 0:
                return
            module, extras = self.parent.module, util.get_var(value.args[0])
            # Add task reduction inputs.
            prefix = module.nr(SeriesFlow)
            typ = module.get_or_realize_type(mangle(OMP_MODULE, cls="TaskReductionInput"))
            assert typ, "could not find 'TaskReductionInput' type"
            array_type = module.get_pointer_type(typ)
            self.array = util.make_var(
                module.nr(StackAllocInstr, array_type, count), prefix, self.parent
            )
            setitem = module.get_or_realize_method(
                array_type, module.SETITEM_MAGIC_NAME, [array_type, module.get_int_type(), typ]
            )
            assert setitem, "could not find 'TaskReductionInputArray.__setitem__' method"
            idx = 0
            for info in self.shared_info:
                if info.reduction:
                    shared = module.nr(PointerValue, info.local)
                    original = util.tuple_get(module.nr(VarValue, extras), info.memb)
                    input_ = self.make_task_red_input(info.reduction, shared, original)
                    prefix.push_back(
                        util.call(
                            setitem, [module.nr(VarValue, self.array), module.get_int(idx), input_]
                        )
                    )
                    idx += 1
            init = module.get_or_realize_func(
                "_taskred_init",
                [
                    self.reduction_loc_ref.type,
                    self.gtid.type,
                    module.get_int_type(),
                    self.array.type,
                ],
                [],
                OMP_MODULE,
            )
            assert init, "task red init function not found"
            self.tskgrp = util.make_var(
                util.call(
                    init,
                    [
                        module.nr(VarValue, self.reduction_loc_ref),
                        module.nr(VarValue, self.gtid),
                        module.get_int(count),
                        module.nr(VarValue, self.array),
                    ],
                ),
                prefix,
                self.parent,
            )
            value.replace_all(prefix)
        if name == "_fix_privates_and_shareds":
            assert len(value.args) == 3, "invalid _fix_privates_and_shareds call found"
            count = self.num_reductions()
            loop_var, privates, shareds = value.args
            new_privates, new_shareds = [], []
            need_privates, need_shareds, info_idx = False, False, 0
            for idx, original in enumerate(self.privates):
                if count > 0 and original is self.privates[-1]:
                    # Final private value is the task group identifier.
                    assert self.tskgrp, "tskgrp var not set"
                    new_privates.append(module.nr(VarValue, self.tskgrp))
                    need_privates = True
                elif get_var_from_outlined_arg(original).id != self.loop_var.id:
                    new_privates.append(util.tuple_get(privates, idx))
                else:
                    new_privates.append(loop_var)
                    need_privates = True
            for idx, original in enumerate(self.shareds):
                if get_var_from_outlined_arg(original).id != self.loop_var.id:
                    if (
                        info_idx < len(self.shared_info)
                        and self.shared_info[info_idx].memb == idx
                        and self.shared_info[info_idx].reduction
                    ):
                        new_shareds.append(
                            module.nr(PointerValue, self.shared_info[info_idx].local)
                        )
                        need_shareds = True
                        info_idx += 1
                    else:
                        new_shareds.append(util.tuple_get(shareds, idx))
                else:
                    new_shareds.append(module.nr(PointerValue, util.get_var(loop_var)))
                    need_shareds = True
            privates = util.make_tuple(new_privates, module) if need_privates else privates
            shareds = util.make_tuple(new_shareds, module) if need_shareds else shareds
            value.replace_all(util.make_tuple([privates, shareds], module))
        if name == "_taskred_finish":
            assert self.reduction_loc_ref and self.gtid, "bad visit order in template"
            if self.num_reductions() == 0:
                return
            finish = module.get_or_realize_func(
                "_taskred_fini", [self.reduction_loc_ref.type, self.gtid.type], [], OMP_MODULE
            )
            assert finish, "taskred finish function not found not found"
            value.replace_all(
                util.call(
                    finish,
                    [module.nr(VarValue, self.reduction_loc_ref), module.nr(VarValue, self.gtid)],
                )
            )


@dataclass(init=False, eq=False)
class GPULoopBodyStubReplacer(Operator):
    """C++: codon/cir/transform/parallel/openmp.cpp:1287. Prepare the GPU kernel body (no LLVM generation)."""

    replacement: CallInstr | None
    loop_var: Var
    step: int

    # C++: codon/cir/transform/parallel/openmp.cpp:1292
    def __init__(self, replacement, loop_var, step):
        super().__init__()
        self.replacement, self.loop_var, self.step = replacement, loop_var, step

    # C++: codon/cir/transform/parallel/openmp.cpp:1295
    def handle_CallInstr(self, value):
        module, fn = value.module, util.get_func(value.callee)
        if not fn:
            return
        name = fn.unmangled_name
        if name == "_gpu_loop_body_stub":
            assert self.replacement, "unexpected double replacement"
            assert len(value.args) == 2, "unexpected loop body stub"
            # Template passes index and extra arguments for convenience.
            index, extras = value.args
            args, idx = [], 0
            for arg in self.replacement.args:
                if get_var_from_outlined_arg(arg).id == self.loop_var.id:
                    args.append(index)
                else:
                    args.append(util.tuple_get(extras, idx))
                    idx += 1
            outlined = cast(BodiedFunc, util.get_func(self.replacement.callee))
            value.replace_all(util.call(outlined, args))
            self.replacement = None
        if name == "_loop_step":
            value.replace_all(module.get_int(self.step))


@dataclass(init=False, eq=False)
class GPULoopTemplateReplacer(LoopTemplateReplacer):
    """C++: codon/cir/transform/parallel/openmp.cpp:1331. Substitute the GPU loop template's constant step."""

    step: int

    # C++: codon/cir/transform/parallel/openmp.cpp:1334
    def __init__(self, parent, replacement, loop_var, step):
        super().__init__(parent, replacement, loop_var)
        self.step = step

    # C++: codon/cir/transform/parallel/openmp.cpp:1338
    def handle_CallInstr(self, value):
        fn = util.get_func(value.callee)
        if fn and fn.unmangled_name == "_loop_step":
            value.replace_all(value.module.get_int(self.step))


@dataclass(init=False, eq=False)
class OpenMPTransformData:
    outline: OutlineResult
    shared_vars: List[Var]
    reds: ReductionIdentifier

    # C++ API: codon/cir/transform/parallel/openmp.cpp:1351
    def __init__(self, outline=None, shared_vars=None, reds=None):
        self.outline = OutlineResult() if outline is None else outline
        self.shared_vars = [] if shared_vars is None else list(shared_vars)
        self.reds = ReductionIdentifier() if reds is None else reds


# C++: codon/cir/transform/parallel/openmp.cpp:1357
def unpar(value):
    value.set_parallel(False)
    return OpenMPTransformData()


def setup_openmp_transform(value, parent, gpu):
    """C++: openmp.cpp:1388. Outline a parallel body and discover reductions."""
    if not value.is_parallel():
        return unpar(value)
    body = cast(SeriesFlow, value.body)
    if not parent or not body:
        return unpar(value)
    outline = outline_region(
        parent, body, allow_out_flows=False, outline_globals=True, all_by_value=gpu
    )
    if not outline:
        return unpar(value)
    # Set up fork_call arguments, including shared argument vars.
    shared_vars, loop_var_arg = [], None
    for arg, original, kind in zip(outline.func.args, outline.call.args, outline.arg_kinds):
        # Tell the reduction identifier which argument is the loop variable, so
        # it cannot accidentally reduce over that argument.
        if get_var_from_outlined_arg(original).id == value.var.id:
            loop_var_arg = arg
        if kind == OutlineResult.ArgKind.MODIFIED:
            shared_vars.append(arg)
    reds = ReductionIdentifier(shared_vars, loop_var_arg)
    reds.process(outline.func)
    return OpenMPTransformData(outline, shared_vars, reds)


@dataclass(init=False, eq=False)
class ForkCallData:
    fork: CallInstr | None
    push_num_threads: CallInstr | None

    # C++ API: codon/cir/transform/parallel/openmp.cpp:1398
    def __init__(self):
        self.fork = self.push_num_threads = None


# C++: codon/cir/transform/parallel/openmp.cpp:1403
def create_fork_call(module, types, raw_template, extra_args, schedule):
    """C++: openmp.cpp:1432. Fork the specialized template, optionally setting threads."""
    result = ForkCallData()
    extras = util.make_tuple(extra_args, module)
    fn = module.get_or_realize_func("_fork_call", [types.i8ptr, extras.type], [], OMP_MODULE)
    assert fn, "fork call function not found"
    result.fork = util.call(fn, [raw_template, extras])
    if schedule.threads and schedule.threads.type.is_type(types.i64):
        push = module.get_or_realize_func("_push_num_threads", [types.i64], [], OMP_MODULE)
        assert push, "push num threads func not found"
        result.push_num_threads = util.call(push, [schedule.threads])
    return result


@dataclass(init=False, eq=False)
class CollapseResult:
    collapsed: ImperativeForFlow | None
    setup: SeriesFlow | None
    error: str

    # C++ API: codon/cir/transform/parallel/openmp.cpp:1422
    def __init__(self, error=""):
        self.collapsed = self.setup = None
        self.error = error

    # C++: codon/cir/transform/parallel/openmp.cpp:1427
    def __bool__(self):
        return self.collapsed is not None


@dataclass(init=False, eq=False)
class LoopRange:
    loop: ImperativeForFlow
    start: Var
    stop: Var
    step: int
    len: Var

    # C++ API: codon/cir/transform/parallel/openmp.cpp:1430
    def __init__(self, loop, start, stop, step, length):
        self.loop, self.start, self.stop, self.step, self.len = loop, start, stop, step, length


# C++: codon/cir/transform/parallel/openmp.cpp:1438
def collapse_loop(parent, value, levels):
    """C++: openmp.cpp:1473. Flatten rectangular integer-loop nests."""
    if levels < 1:
        return CollapseResult("'collapse' must be at least 1")
    module, loops, current = value.module, [value], value
    for _ in range(levels - 1):
        body = cast(SeriesFlow, current.body)
        assert body, "unexpected loop body"
        if len(body.series) != 1 or not is_a(ImperativeForFlow, body.series[0]):
            return CollapseResult("loop nest not collapsible")
        current = cast(ImperativeForFlow, body.series[0])
        loops.append(current)
    ranges, setup = [], module.nr(SeriesFlow)
    typ = module.get_int_type()
    length = module.get_or_realize_func("_range_len", [typ, typ, typ], [], OMP_MODULE)
    assert length, "range length calculation function not found"
    for loop in loops:
        start, stop = (
            util.make_var(loop.start, setup, parent),
            util.make_var(loop.end, setup, parent),
        )
        size = util.make_var(
            util.call(
                length,
                [module.nr(VarValue, start), module.nr(VarValue, stop), module.get_int(loop.step)],
            ),
            setup,
            parent,
        )
        ranges.append(LoopRange(loop, start, stop, loop.step, size))
    iterations = module.get_int(1)
    for range_ in ranges:
        iterations = iterations * module.nr(VarValue, range_.len)
    var = module.nr(Var, module.get_int_type(), False)
    parent.symbols.append(var)
    body = module.nr(SeriesFlow)
    schedule = copy.copy(value.schedule)
    schedule.collapse = 0
    collapsed = module.nr(ImperativeForFlow, module.get_int(0), 1, iterations, body, var, schedule)
    # Reconstruct indices by successive divmods.
    last_div = None
    for range_ in reversed(ranges):
        current = last_div if last_div else var
        div = util.make_var(
            module.nr(VarValue, current) // module.nr(VarValue, range_.len), body, parent
        )
        mod = util.make_var(
            module.nr(VarValue, current) % module.nr(VarValue, range_.len), body, parent
        )
        index = module.nr(VarValue, range_.start) + (
            module.nr(VarValue, mod) * module.get_int(range_.step)
        )
        body.push_back(module.nr(AssignInstr, range_.loop.var, index))
        last_div = div
    body.series.extend(cast(SeriesFlow, loops[-1].body).series)
    result = CollapseResult()
    result.collapsed, result.setup = collapsed, setup
    return result


_gpu_instance = 0


@dataclass(init=False, eq=False)
class OpenMPPass(OperatorPass):
    """C++: codon/cir/transform/parallel/openmp.h:12, openmp.h. Children-first OpenMP lowering."""

    KEY = "core-parallel-openmp"

    # C++: codon/cir/transform/parallel/openmp.h:15
    def __init__(self):
        """
        Constructs an OpenMP pass.
        """
        super().__init__(True)

    # C++: codon/cir/transform/parallel/openmp.cpp:1526
    def handle_ForFlow(self, value):
        data = setup_openmp_transform(value, cast(BodiedFunc, self.get_parent_func()), False)
        if not value.is_parallel():
            return
        outline, shared_vars, reds = data.outline, data.shared_vars, data.reds
        module, loop_var, schedule = value.module, value.var, value.schedule
        types = OMPTypes(module)
        # Separate private and shared arguments; reductions correspond to shareds.
        privates, shareds, shared_redux, idx = [], [], [], 0
        for arg in outline.call.args:
            if is_a(VarValue, arg):
                privates.append(arg)
            else:
                shareds.append(arg)
                shared_redux.append(reds.get_reduction(shared_vars[idx]))
                idx += 1
        visitor = CloneVisitor(module)
        # Pass the __kmpc_taskred_modifier_init task-group identifier as private
        # data (initially a null void pointer), and append gtid to the function.
        if reds.reductions:
            privates.append(types.i8ptr.construct([]))
            signature = outline.func.type
            arg_types = [*signature.arg_types, module.get_int_type()]
            old_args = list(outline.func.args)
            names = [arg.name for arg in old_args] + ["gtid"]
            fn = module.nr(BodiedFunc, util.get_derived_func_name("__outlined_new", outline.func))
            fn.realize(module.get_func_type(signature.return_type, arg_types), names)
            remaps = {old.id: new for old, new in zip(old_args, fn.args)}
            fn.body = cast(SeriesFlow, visitor.clone(outline.func.body, fn, remaps))
            # Update outline data and its call.
            outline.func = fn
            outline.call.callee = module.nr(VarValue, fn)
            outline.call.args.append(module.get_int(0))
            outline.arg_kinds.append(OutlineResult.ArgKind.CONSTANT)
        privates_tuple, shareds_tuple = (
            util.make_tuple(privates, module),
            util.make_tuple(shareds, module),
        )
        # Instantiate and specialize the task-loop template.
        template_args = [
            types.i32ptr,
            types.i32ptr,
            module.get_pointer_type(
                module.get_tuple_type([value.iter.type, privates_tuple.type, shareds_tuple.type])
            ),
        ]
        template = module.get_or_realize_func(
            "_task_loop_outline_template", template_args, [], OMP_MODULE
        )
        assert template, "task loop outline template not found"
        template = visitor.force_clone(template)
        template.name = util.get_derived_func_name(template.name, outline.func)
        TaskLoopRoutineStubReplacer(
            cast(BodiedFunc, template),
            outline.call,
            loop_var,
            reds,
            privates,
            shareds,
            shared_redux,
        ).process(template)
        raw = ptr_from_func(template)
        # Fork call.
        fork = create_fork_call(
            module, types, raw, [value.iter, privates_tuple, shareds_tuple], schedule
        )
        if fork.push_num_threads:
            self.insert_before(fork.push_num_threads)
        value.replace_all(fork.fork)

    # C++: codon/cir/transform/parallel/openmp.cpp:1629
    def handle_ImperativeForFlow(self, value):
        global _gpu_instance

        parent = cast(BodiedFunc, self.get_parent_func())
        if value.is_parallel() and value.schedule.collapse != 0:
            collapse = collapse_loop(parent, value, value.schedule.collapse)
            if collapse:
                value.replace_all(collapse.collapsed)
                value = collapse.collapsed
                self.insert_before(collapse.setup)
            elif collapse.error:
                warn("could not collapse loop: " + collapse.error, value)
        data = setup_openmp_transform(value, parent, value.is_parallel() and value.schedule.gpu)
        if not value.is_parallel():
            return
        outline, shared_vars, reds = data.outline, data.shared_vars, data.reds
        module, loop_var, schedule = value.module, value.var, value.schedule
        types = OMPTypes(module)
        # Shared variables are disabled for GPU loops.
        assert not (schedule.gpu and shared_vars), "GPU-parallel loop had shared vars"
        extras, extra_types = [], []
        for arg in outline.call.args:
            if get_var_from_outlined_arg(arg).id != loop_var.id:
                extras.append(arg)
                extra_types.append(arg.type)
        name = (
            "_gpu_loop_outline_template"
            if schedule.gpu
            else "_dynamic_loop_outline_template"
            if schedule.dynamic
            else "_static_chunked_loop_outline_template"
            if schedule.chunk
            else "_static_loop_outline_template"
        )
        if schedule.gpu:
            attribute = mangle(GPU_MODULE, func="kernel")
            kernels = {
                fn.id
                for var in module
                if (fn := cast(BodiedFunc, var)) and util.has_attribute(fn, attribute)
            }
            args = [types.i64, types.i64, module.get_tuple_type(extra_types)]
            template = module.get_or_realize_func(name, args, [_gpu_instance], GPU_MODULE)
            _gpu_instance += 1
            if not template:
                warn("loop not compilable for GPU; ignoring", value)
                value.set_parallel(False)
                return
            kernel = None
            for var in module:
                fn = cast(BodiedFunc, var)
                if fn and util.has_attribute(fn, attribute) and fn.id not in kernels:
                    assert kernel is None, "multiple new kernels found after instantiation"
                    kernel = fn
            assert kernel, "no new kernel found"
            GPULoopBodyStubReplacer(outline.call, loop_var, value.step).process(kernel)
            visitor = CloneVisitor(module)
            template = visitor.force_clone(template)
            GPULoopTemplateReplacer(
                cast(BodiedFunc, template), outline.call, loop_var, value.step
            ).process(template)
            value.replace_all(
                util.call(template, [value.start, value.end, util.make_tuple(extras, module)])
            )
        else:
            args = [
                types.i32ptr,
                types.i32ptr,
                module.get_pointer_type(
                    module.get_tuple_type(
                        [types.i64, types.i64, types.i64, module.get_tuple_type(extra_types)]
                    )
                ),
            ]
            template = module.get_or_realize_func(name, args, [], OMP_MODULE)
            assert template, "imperative loop outline template not found"
            visitor = CloneVisitor(module)
            template = visitor.force_clone(template)
            template.name = util.get_derived_func_name(template.name, outline.func)
            ImperativeLoopTemplateReplacer(
                cast(BodiedFunc, template), outline.call, loop_var, reds, schedule, value.step
            ).process(template)
            raw = ptr_from_func(template)
            chunk = (
                schedule.chunk
                if schedule.chunk and schedule.chunk.type.is_type(types.i64)
                else module.get_int(1)
            )
            # Fork call.
            fork = create_fork_call(
                module, types, raw, [chunk, value.start, value.end, *extras], schedule
            )
            if fork.push_num_threads:
                self.insert_before(fork.push_num_threads)
            value.replace_all(fork.fork)
