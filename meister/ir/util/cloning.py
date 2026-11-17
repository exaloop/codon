# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

import copy

from ...bridge import Dict, List, dataclass
from .. import flow, instr, var
from .operator import Operator
from .visitor import ConstVisitor


@dataclass(init=False, eq=False)
class GatherLocals(Operator):
    """C++: codon/cir/util/cloning.cpp:11. Gather non-global referenced variables."""

    locals: List[var.Var]

    # C++ API: codon/cir/util/cloning.cpp:11
    def __init__(self):
        super().__init__()
        self.locals = []

    # C++: codon/cir/util/cloning.cpp:13
    def pre_hook(self, node):
        self.locals.extend(v for v in node.get_used_variables() if not v.global_)


@dataclass(init=False, eq=False)
class CloneVisitor(ConstVisitor):
    """C++: codon/cir/util/cloning.h:14, cloning.cpp. Graph cloning with explicit remaps."""

    ctx: Dict[int, object]
    module: object
    clone_loop: bool
    result: object | None

    # C++: codon/cir/util/cloning.h:29
    def __init__(self, module, clone_loop=True):
        """
        Constructs a clone visitor.
        @param module the module
        @param cloneLoop true if break/continue loops should be cloned
        """
        self.ctx, self.result, self.module, self.clone_loop = {}, None, module, clone_loop

    # C++: codon/cir/util/cloning.cpp:22
    def clone(self, other, clone_to=None, remaps=None):
        if other is None:
            return None
        if isinstance(other, flow.Stage):
            return flow.Stage(
                self.clone(other.callee),
                [self.clone(arg) for arg in other.args],
                other.generator,
                other.parallel,
            )
        other = other.get_actual()
        if isinstance(other, var.Var):
            # A variable is unowned: return it unless it was force-cloned.
            return self.ctx.get(other.id, other)
        if clone_to is not None:
            gather = GatherLocals()
            gather.process(other)
            for local in gather.locals:
                if remaps and local.id in remaps:
                    self.force_remap(local, remaps[local.id])
                else:
                    cloned = clone_to.module.n(
                        var.Var,
                        local,
                        local.type,
                        local.global_,
                        local.external,
                        local.tls,
                        local.name,
                    )
                    clone_to.symbols.append(cloned)
                    self.force_remap(local, cloned)
        else:
            for identifier, value in (remaps or {}).items():
                self.force_remap(other.module.get_var(identifier), value)
        return self._clone(other, False)

    # C++: codon/cir/util/cloning.h:100
    def force_clone(self, other):
        """
        Forces a clone. No difference for values but ensures that variables are actually
        cloned.
        @param other the original
        @return the clone
        """
        return self._clone(other.get_actual(), True) if other is not None else None

    # C++ API: codon/cir/util/cloning.h:14
    def _clone(self, other, force):
        if other.id not in self.ctx:
            other.accept(self)
            self.ctx[other.id] = self.result
            for key, attr in other.attributes.items():
                if attr.needs_clone():
                    self.ctx[other.id].set_attribute(
                        attr.force_clone(self) if force else attr.clone(self), key
                    )
        return self.ctx[other.id]

    # C++: codon/cir/util/cloning.h:123
    def force_remap(self, original, new):
        """
        Remaps a clone.
        @param original the original
        @param newVal the clone
        """
        self.ctx[original.id] = new

    # C++ API: codon/cir/util/cloning.h:14
    def make(self, original, *args):
        return self.module.n(type(original), original, *args, original.name)

    # C++ API: codon/cir/util/cloning.h:14
    def default_visit(self, node):
        if hasattr(node, "do_clone"):
            self.result = node.do_clone(self)
            return
        assert False, f"cannot clone {type(node).__name__}"

    # C++: codon/cir/util/cloning.cpp:74
    def visit_Var(self, node):
        self.result = self.make(node, node.type, node.global_, node.external, node.tls)

    # C++ API: codon/cir/util/cloning.h:14
    def _function(self, node):
        result = self.make(node)
        result.unmangled_name, result.generator, result.async_ = (
            node.unmangled_name,
            node.generator,
            node.async_,
        )
        result.realize(node.type, [arg.name for arg in node.args])
        for old, new in zip(node.args, result.args):
            self.force_remap(old, new)
        return result

    # C++: codon/cir/util/cloning.cpp:79
    def visit_BodiedFunc(self, node):
        result = self.make(node)
        for local in node.symbols:
            result.symbols.append(self.force_clone(local))
        result.unmangled_name, result.generator, result.async_ = (
            node.unmangled_name,
            node.generator,
            node.async_,
        )
        result.realize(node.type, [arg.name for arg in node.args])
        for old, new in zip(node.args, result.args):
            self.force_remap(old, new)
        # The body might reference this function!
        self.force_remap(node, result)
        if node.body:
            result.body = self.clone(node.body)
        result.jit = node.jit
        self.result = result

    # C++: codon/cir/util/cloning.cpp:111
    def visit_ExternalFunc(self, node):
        self.result = self._function(node)

    # C++: codon/cir/util/cloning.cpp:132
    def visit_InternalFunc(self, node):
        result = self._function(node)
        result.set_intrinsic(node.intrinsic, node.intrinsic_type)
        result.parent_type = node.parent_type
        self.result = result

    # C++: codon/cir/util/cloning.cpp:155
    def visit_LLVMFunc(self, node):
        result = self._function(node)
        result.llvm_body, result.llvm_declares, result.llvm_literals = (
            node.llvm_body,
            node.llvm_declares,
            list(node.llvm_literals),
        )
        self.result = result

    # C++: codon/cir/util/cloning.cpp:179
    def visit_VarValue(self, node):
        self.result = self.make(node, self.clone(node.var))

    # C++: codon/cir/util/cloning.cpp:181
    def visit_PointerValue(self, node):
        self.result = self.make(node, self.clone(node.var), node.fields)

    # C++: codon/cir/util/cloning.cpp:185
    def visit_SeriesFlow(self, node):
        result = self.make(node)
        for child in node:
            result.push_back(self.clone(child))
        self.result = result

    # C++: codon/cir/util/cloning.cpp:192
    def visit_IfFlow(self, node):
        self.result = self.make(
            node, self.clone(node.cond), self.clone(node.true_branch), self.clone(node.false_branch)
        )

    # C++: codon/cir/util/cloning.cpp:197
    def visit_WhileFlow(self, node):
        result = self.make(node, None, None)
        self.force_remap(node, result)
        result.cond, result.body = self.clone(node.cond), self.clone(node.body)
        self.result = result

    # C++ API: codon/cir/util/cloning.h:14
    def _schedule(self, schedule):
        if schedule is None:
            return None
        result = copy.copy(schedule)
        for value in schedule.get_used_values():
            result.replace_used_value(value.id, self.clone(value))
        return result

    # C++: codon/cir/util/cloning.cpp:205
    def visit_ForFlow(self, node):
        result = self.make(node, None, None, None, None, False)
        self.force_remap(node, result)
        result.iter, result.body, result.var = (
            self.clone(node.iter),
            self.clone(node.body),
            self.clone(node.var),
        )
        result.schedule, result.async_ = self._schedule(node.schedule), node.async_
        self.result = result

    # C++: codon/cir/util/cloning.cpp:224
    def visit_ImperativeForFlow(self, node):
        result = self.make(node, None, node.step, None, None, None, None)
        self.force_remap(node, result)
        result.start, result.body, result.var, result.end = (
            self.clone(node.start),
            self.clone(node.body),
            self.clone(node.var),
            self.clone(node.end),
        )
        result.schedule = self._schedule(node.schedule)
        self.result = result

    # C++: codon/cir/util/cloning.cpp:242
    def visit_TryCatchFlow(self, node):
        result = self.make(
            node, self.clone(node.body), self.clone(node.finally_), self.clone(node.else_)
        )
        for catch in node.catches:
            result.catches.append(
                flow.Catch(self.clone(catch.handler), catch.type, self.clone(catch.var))
            )
        self.result = result

    # C++: codon/cir/util/cloning.cpp:250
    def visit_PipelineFlow(self, node):
        self.result = self.make(node, [self.clone(stage) for stage in node.stages])

    # C++: codon/cir/util/cloning.cpp:260
    def visit_IntConst(self, node):
        self.result = self.make(node, node.val, node.type)

    # C++: codon/cir/util/cloning.cpp:264
    def visit_FloatConst(self, node):
        self.result = self.make(node, node.val, node.type)

    # C++: codon/cir/util/cloning.cpp:268
    def visit_BoolConst(self, node):
        self.result = self.make(node, node.val, node.type)

    # C++: codon/cir/util/cloning.cpp:272
    def visit_StringConst(self, node):
        self.result = self.make(node, node.val, node.type)

    # C++: codon/cir/util/cloning.cpp:276
    def visit_BytesConst(self, node):
        self.result = self.make(node, node.val, node.type)

    # C++: codon/cir/util/cloning.cpp:282
    def visit_AssignInstr(self, node):
        self.result = self.make(node, self.clone(node.lhs), self.clone(node.rhs))

    # C++: codon/cir/util/cloning.cpp:286
    def visit_ExtractInstr(self, node):
        self.result = self.module.n(
            instr.ExtractInstr, node, self.clone(node.val), node.field, node.name, node.polymorphic
        )

    # C++: codon/cir/util/cloning.cpp:291
    def visit_InsertInstr(self, node):
        self.result = self.module.n(
            instr.InsertInstr,
            node,
            self.clone(node.lhs),
            node.field,
            self.clone(node.rhs),
            node.name,
            node.polymorphic,
        )

    # C++: codon/cir/util/cloning.cpp:296
    def visit_CallInstr(self, node):
        # C++ clones arguments before cloning the callee.
        args = [self.clone(arg) for arg in node.args]
        self.result = self.make(node, self.clone(node.callee), args)

    # C++: codon/cir/util/cloning.cpp:304
    def visit_StackAllocInstr(self, node):
        self.result = self.make(node, node.ptr_type, node.count)

    # C++: codon/cir/util/cloning.cpp:308
    def visit_YieldInInstr(self, node):
        self.result = self.make(node, node.type, node.suspend)

    # C++: codon/cir/util/cloning.cpp:312
    def visit_TernaryInstr(self, node):
        self.result = self.make(
            node, self.clone(node.cond), self.clone(node.true_value), self.clone(node.false_value)
        )

    # C++: codon/cir/util/cloning.cpp:317
    def visit_BreakInstr(self, node):
        self.result = self.make(node, self.clone(node.loop) if self.clone_loop else node.loop)

    # C++: codon/cir/util/cloning.cpp:321
    def visit_ContinueInstr(self, node):
        self.result = self.make(node, self.clone(node.loop) if self.clone_loop else node.loop)

    # C++: codon/cir/util/cloning.cpp:325
    def visit_ReturnInstr(self, node):
        self.result = self.make(node, self.clone(node.value))

    # C++: codon/cir/util/cloning.cpp:327
    def visit_YieldInstr(self, node):
        self.result = self.make(node, self.clone(node.value), node.final)

    # C++: codon/cir/util/cloning.cpp:331
    def visit_AwaitInstr(self, node):
        self.result = self.make(node, self.clone(node.value), node.type, node.generator)

    # C++: codon/cir/util/cloning.cpp:335
    def visit_ThrowInstr(self, node):
        result = self.make(node, self.clone(node.value))
        result.fallback = self.clone(node.fallback)
        self.result = result

    # C++: codon/cir/util/cloning.cpp:341
    def visit_FlowInstr(self, node):
        self.result = self.make(node, self.clone(node.flow), self.clone(node.value))
