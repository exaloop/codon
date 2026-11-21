# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Lower Meister CIR into live LLVM IR using Codon's LLVM distribution."""

from collections import Counter
from copy import copy
from types import SimpleNamespace

from .. import ir
from ..ast.nodes import Node
from . import llvm
from .llvisitor_eh import ExceptionState, lower_try_catch
from .util import irtools
from .util.visitor import ConstVisitor


class LLVMVisitor(ConstVisitor):
    mod: llvm.Module
    b: llvm.IRBuilder
    layout: llvm.DataLayout
    debug: bool
    standalone: bool
    fn: llvm.Function | None
    block: llvm.BasicBlock | None
    vars: dict[int, llvm.Value]
    funcs: dict[int, llvm.Function]
    types: dict[ir.Type, llvm.Type]
    function_names: dict[int, str]
    function_name_counts: Counter[str]
    field_offset_resolvers: dict[tuple[int, int], int | llvm.Function]
    virtual_lookup_types: dict[str, ir.RefType]
    virtual_targets: dict[tuple[int, int], list[tuple[int, llvm.Function]] | tuple[()]]
    loops: list[SimpleNamespace]
    trycatch: list[SimpleNamespace]
    finally_: list[SimpleNamespace]
    catches: list[SimpleNamespace]
    next_sequence_number: int
    coro: SimpleNamespace | None
    version: str
    db: llvm.DebugBuilder | None
    unit: llvm.Metadata | None
    default_source: Node.SrcInfo
    i1: llvm.Type
    i8: llvm.Type
    i16: llvm.Type
    i32: llvm.Type
    i64: llvm.Type
    ptr: llvm.Type
    void: llvm.Type
    token_none: llvm.Value
    str_type: llvm.Type

    # C++: codon/cir/llvm/llvisitor.cpp:95
    def __init__(self, module, *, debug=True, standalone=False, version):
        self.mod = module
        self.b = module.builder
        self.layout = module.data_layout
        self.debug = debug
        self.standalone = standalone
        self.fn = self.block = None
        self.vars, self.funcs, self.types = {}, {}, {}
        self.function_names, self.function_name_counts = {}, Counter()
        self.field_offset_resolvers = {}
        self.virtual_lookup_types, self.virtual_targets = {}, {}
        self.loops = []
        self.trycatch, self.finally_, self.catches = [], [], []
        self.next_sequence_number = 0
        self.coro = None
        self.version = version
        self.db = self.unit = None
        self.default_source = Node.SrcInfo("<internal>")
        self.i1, self.i8, self.i16, self.i32, self.i64 = (
            module.int_type(n) for n in (1, 8, 16, 32, 64)
        )
        self.ptr = module.pointer_type()
        self.void = module.primitive_type("Void")
        self.token_none = module.primitive_type("Token").null()
        self.str_type = module.struct_type((self.ptr, self.i64))

    # C++: codon/cir/llvm/llvisitor.cpp:413
    def process(self, node):
        self.set_debug_info_for_node(node)
        return self.visit(node)

    def default_visit(self, node):
        raise NotImplementedError(f"LLVM lowering for {type(node).__name__}")

    # C++: codon/cir/llvm/llvisitor.cpp:403
    def set_debug_info_for_node(self, node):
        scope = self.fn.subprogram if node and self.fn else None
        loc = None
        if scope:
            src = self.source(node)
            loc = self.db.location(src.line, src.col, scope)
        self.b.set_debug_location(loc)

    # C++: codon/cir/llvm/llvisitor.h:346
    def source(self, node):
        attr = node.get_attribute(ir.SrcInfoAttribute)
        return attr.info if attr else self.default_source

    # C++: codon/cir/llvm/llvisitor.cpp:42
    def debug_file(self, src):
        directory, sep, filename = src.file.rpartition("/")
        return self.db.file(filename, directory if sep else ".")

    # C++: codon/cir/llvm/llvisitor.cpp:2323
    def get_di_type(self, typ, cache):
        typ = typ.get_actual()
        ty = self.get_llvm_type(typ)
        size = self.layout.alloc_size(ty) * 8
        match typ:
            case ir.IntType():
                return self.db.basic_type(typ.name, size, 5 if typ.sign else 7)
            case ir.FloatType() | ir.Float32Type() | ir.Float16Type() | ir.BFloat16Type():
                return self.db.basic_type(typ.name, size, 4)  # DW_ATE_float
            case ir.Float128Type():
                return self.db.basic_type(typ.name, size, 0x82)  # DW_ATE_HP_float128
            case ir.BoolType():
                return self.db.basic_type(typ.name, size, 2)  # DW_ATE_boolean
            case ir.RecordType():
                return self.get_di_struct(typ, ty, cache)
            case ir.RefType():
                base = (
                    self.get_di_struct(typ, self.object_layout(typ), cache)
                    if typ.polymorphic
                    else self.get_di_type(typ.contents, cache)
                )
                return self.db.reference_type(base)
            case ir.FuncType():
                return self.db.pointer_type(self.get_di_function_type(typ, cache), size)
            case ir.OptionalType():
                if isinstance(typ.base, ir.RefType):
                    return self.get_di_type(typ.base, cache)
                src = self.source(typ)
                file = self.debug_file(src)
                result = self.db.struct_type(typ.name, file, src.line, size)
                flag_size = self.layout.alloc_size(self.i1) * 8
                flag = self.db.basic_type("i1", flag_size, 2)
                members = [
                    self.db.member_type(
                        result,
                        "has",
                        file,
                        src.line,
                        flag_size,
                        self.layout.offset_of(ty, 0) * 8,
                        flag,
                    )
                ]
                members.append(
                    self.db.member_type(
                        result,
                        "val",
                        file,
                        src.line,
                        self.layout.alloc_size(self.get_llvm_type(typ.base)) * 8,
                        self.layout.offset_of(ty, 1) * 8,
                        self.get_di_type(typ.base, cache),
                    )
                )
                self.db.replace_members(result, members)
                return result
            case ir.PointerType():
                return self.db.pointer_type(self.get_di_type(typ.base, cache), size)
            case ir.GeneratorType():
                return self.db.basic_type(typ.name, size, 1)  # DW_ATE_address
            case ir.VectorType() | ir.UnionType():
                return self.db.basic_type(typ.name, size, 7)  # DW_ATE_unsigned
        raise NotImplementedError(f"debug type for {typ.name}")

    def get_di_struct(self, typ, layout, cache):
        if typ.name in cache:
            return cache[typ.name]
        src = self.source(typ)
        file = self.debug_file(src)
        size = self.layout.alloc_size(layout) * 8
        # Cache the incomplete type before visiting fields to break recursion.
        result = cache[typ.name] = self.db.struct_type(typ.name, file, src.line, size)
        members = []
        polymorphic = isinstance(typ, ir.RefType)
        if polymorphic:
            ptr_bits = self.layout.pointer_size() * 8
            members.append(
                self.db.member_type(
                    result,
                    "typeinfo",
                    file,
                    src.line,
                    ptr_bits,
                    0,
                    self.db.pointer_type(None, ptr_bits),
                    flags=1 << 6,  # artificial
                )
            )
        member_info = typ.get_attribute(ir.MemberAttribute)
        for idx, field in enumerate(typ):
            offset = (
                self.object_field_offset(typ, idx)
                if polymorphic
                else self.layout.offset_of(layout, idx)
            )
            field_src = (
                member_info.member_src_info.get(field.name, src)
                if member_info and not polymorphic
                else src
            )
            if field_src is None:
                # C++ Type::getSrcInfo() is a value, even for inferred types
                # with no source location. Python stores None in that case.
                field_src = Node.SrcInfo()
            members.append(
                self.db.member_type(
                    result,
                    field.name,
                    self.debug_file(field_src),
                    field_src.line,
                    self.layout.alloc_size(self.get_llvm_type(field.type)) * 8,
                    offset * 8,
                    self.get_di_type(field.type, cache),
                )
            )
        self.db.replace_members(result, members)
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:2414
    def get_di_function_type(self, typ, cache):
        return self.db.subroutine_type(self.get_di_type(t, cache) for t in (typ.return_type, *typ))

    # C++: codon/cir/llvm/llvisitor.cpp:1740
    def get_di_subprogram(self, fn):
        src = self.source(fn)
        file = self.debug_file(src)
        name = f"{fn.parent_type.name}.{fn.unmangled_name}" if fn.parent_type else fn.unmangled_name
        typ = self.get_di_function_type(fn.type, {})
        return self.db.function(file, name, self.function_name(fn), src.line, typ, not self.debug)

    # C++: codon/cir/llvm/llvisitor.h:339
    @staticmethod
    def debug_variable_name(var):
        pos = var.name.find(".")
        return var.name[:pos] if pos > 0 else var.name

    # C++: codon/cir/llvm/llvisitor.cpp:2007
    def declare_local(self, var, storage, entry, arg=0):
        src = self.source(var)
        scope = self.fn.subprogram
        meta = self.db.local_variable(
            scope,
            self.debug_variable_name(var),
            self.debug_file(src),
            src.line,
            self.get_di_type(var.type, {}),
            self.debug,
            arg,
        )
        self.db.insert_declare(storage, meta, self.db.location(src.line, src.col, scope), entry)

    # C++: codon/cir/llvm/llvisitor.cpp:81
    def function_name(self, fn):
        if isinstance(fn, ir.ExternalFunc) or irtools.is_export(fn):
            return fn.unmangled_name
        if irtools.is_gpu_kernel(fn):
            return fn.name
        if fn.id not in self.function_names:
            self.function_names[fn.id] = f"{fn.name}.{self.function_name_counts[fn.name]}"
            self.function_name_counts[fn.name] += 1
        return self.function_names[fn.id]

    # C++: codon/cir/llvm/llvisitor.cpp:2220
    def get_llvm_type(self, typ):
        typ = typ.get_actual()
        if typ not in self.types:
            self.types[typ] = self._lower_type(typ)
        return self.types[typ]

    def _lower_type(self, typ):
        match typ:
            case ir.IntType():
                return self.mod.int_type(typ.len)
            case ir.FloatType():
                return self.mod.primitive_type("Double")
            case ir.Float32Type():
                return self.mod.primitive_type("Float")
            case ir.Float16Type():
                return self.mod.primitive_type("Half")
            case ir.BFloat16Type():
                return self.mod.primitive_type("BFloat")
            case ir.Float128Type():
                return self.mod.primitive_type("FP128")
            case ir.BoolType():
                return self.i8
            case ir.RecordType():
                return self.mod.struct_type(self.get_llvm_type(field.type) for field in typ)
            case ir.RefType() | ir.FuncType() | ir.PointerType() | ir.GeneratorType():
                return self.ptr
            case ir.OptionalType():
                base = self.get_llvm_type(typ.base)
                if isinstance(typ.base.get_actual(), ir.RefType):
                    return base
                return self.mod.struct_type((self.i1, base))
            case ir.VectorType():
                return self.get_llvm_type(typ.base).vector(typ.count)
            case ir.UnionType():
                alignment_type, max_size = self.i8, 0
                for member in typ:
                    ty = self.get_llvm_type(member)
                    max_size = max(max_size, self.layout.alloc_size(ty))
                    if self.layout.abi_alignment(ty) > self.layout.abi_alignment(alignment_type):
                        alignment_type = ty
                storage = self.mod.struct_type((alignment_type.array(0), self.i8.array(max_size)))
                return self.mod.struct_type((self.i8, storage))
        raise NotImplementedError(f"LLVM type for {typ.name}")

    # C++: codon/cir/llvm/llvisitor.cpp:2312
    def get_llvm_func_type(self, typ):
        return self.get_llvm_type(typ.return_type).function(
            [self.get_llvm_type(arg) for arg in typ], typ.variadic
        )

    # C++: codon/cir/llvm/llvisitor.cpp:2206
    def is_storable(self, typ):
        return not typ.is_void and self.layout.alloc_size(typ) != 0

    # C++: codon/cir/llvm/llvisitor.cpp:2210
    def dummy(self, typ):
        return self.token_none if typ.is_void else typ.undef()

    # C++: codon/cir/llvm/llvisitor.cpp:159
    def register_global(self, var):
        if not var.global_:
            return
        if isinstance(var, ir.Func):
            if not isinstance(var, ir.InternalFunc) or not var.is_type_query():
                fn = self.funcs[var.id] = self.make_llvm_function(var)
                if (
                    isinstance(var, ir.InternalFunc)
                    and var.intrinsic == ir.Intrinsic.VIRTUAL_LOOKUP
                ):
                    self.virtual_lookup_types[fn.name] = var.intrinsic_type
            return
        typ = self.get_llvm_type(var.type)
        if not self.is_storable(typ):
            self.vars.setdefault(var.id, self.dummy(typ))
            return
        name = var.name if var.external else ("._" if var.name.startswith("L") else ".") + var.name
        storage = self.mod.add_global(
            name,
            typ,
            None if var.external else typ.null(),
            linkage=llvm.Linkage.EXTERNAL if var.external else llvm.Linkage.PRIVATE,
        )
        self.vars.setdefault(var.id, storage)
        if var.external:
            storage.set_unnamed_addr(local=True)
        elif not (var.tls and "windows" in self.mod.target):
            # CodeView cannot describe LLVM's emulated TLS storage.
            # Referring to the original symbol leaves an undefined COFF label.
            src = self.source(var)
            expr = self.db.global_variable(
                self.unit,
                self.debug_variable_name(var),
                var.name,
                self.debug_file(src),
                src.line,
                self.get_di_type(var.type, {}),
                True,
            )
            storage.set_global_metadata("dbg", expr)
        if var.tls:
            storage.set_thread_local()

    # C++: codon/cir/llvm/llvisitor.cpp:1759
    def make_llvm_function(self, fn):
        if isinstance(fn, ir.LLVMFunc):
            result = self.visit_LLVMFunc(fn)
            self.set_debug_info_for_node(None)
            return result
        result = self.mod.get_or_insert_function(
            self.function_name(fn), self.get_llvm_func_type(fn.type)
        )
        if not isinstance(fn, ir.ExternalFunc):
            result.set_subprogram(self.get_di_subprogram(fn))
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:232
    def get_func(self, fn):
        if fn.id not in self.funcs and isinstance(fn, ir.InternalFunc) and fn.is_type_query():
            wrapper = self.make_llvm_function(fn)
            wrapper.set_linkage(llvm.Linkage.PRIVATE)
            for attr in ("alwaysinline", "nounwind", "willreturn"):
                wrapper.add_attribute(attr)
            wrapper.add_attribute("memory", 0)
            old_block = self.b.insert_block
            self.b.set_insert_point(wrapper.append_block("entry"))
            self.b.create_ret(self.codegen_type_query(fn))
            if old_block:
                self.b.set_insert_point(old_block)
            else:
                self.b.clear_insert_point()
            self.funcs[fn.id] = wrapper
        return self.funcs[fn.id]

    # C++: codon/cir/llvm/llvisitor.cpp:1807
    def visit_ExternalFunc(self, fn):
        result = self.get_func(fn)
        result.add_attribute("nounwind")
        result.add_attribute("willreturn")
        if irtools.has_attribute(fn, irtools._attribute_name("llvm_memory_none")):
            result.add_attribute("memory", 0)
        if irtools.has_attribute(fn, irtools._attribute_name("llvm_nofree")):
            result.add_attribute("nofree")
        if irtools.has_attribute(fn, irtools._attribute_name("llvm_nocapture")):
            for idx, arg in enumerate(result.args, 1):
                if arg.type.is_pointer:
                    result.add_attribute("nocapture", index=idx)

    # C++: codon/cir/llvm/llvisitor.cpp:1895
    def visit_LLVMFunc(self, fn):
        name = self.function_name(fn)
        if existing := self.mod.function(name):
            return existing
        args = ", ".join(f"{self.get_llvm_type(arg.type)} %{arg.name}" for arg in fn.args)
        ret = self.get_llvm_type(fn.type.return_type)
        # The braces in the signature and wrapper are LLVM syntax, not substitutions.
        signature = f'define {ret} @"{name}"({args})'.replace("{", "{{").replace("}", "}}")
        code = fn.llvm_declares + "\n" + signature + " {{\n" + fn.llvm_body + "\n}}"
        literals = [
            self.get_llvm_type(lit.value)
            if not lit.is_static() and not lit.is_static_str()
            else lit.value
            for lit in fn.llvm_literals
        ]
        try:
            self.mod.parse_and_link(code.format(*literals))
        except (ValueError, IndexError, llvm.LLVMError) as error:
            raise llvm.LLVMError(f"{fn.name}: {error}") from error
        result = self.mod.function(name)
        assert result, f"inline LLVM function {name} not linked"
        result.set_linkage(llvm.Linkage.PRIVATE)
        result.add_attribute("alwaysinline")
        scope = self.get_di_subprogram(fn)
        result.set_subprogram(scope)
        src = self.source(fn)
        loc = self.db.location(src.line, src.col, scope)
        for block in result.blocks():
            for instr in block.instructions():
                if not instr.debug_location:
                    instr.set_debug_location(loc)
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:1828
    def visit_InternalFunc(self, fn):
        if fn.is_type_query():
            return
        self.fn = self.get_func(fn)
        self.set_debug_info_for_node(fn)
        self.fn.set_linkage(llvm.Linkage.PRIVATE)
        self.fn.add_attribute("alwaysinline")
        self.block = self.fn.append_block("entry")
        self.b.set_insert_point(self.block)
        result = self.codegen_intrinsic(fn, self.fn.args)
        if not self.b.insert_block.terminator:
            assert result, f"internal function {fn.name} did not produce a value"
            self.b.create_ret(result)

    # C++: codon/cir/llvm/llvisitor.cpp:1958
    def visit_BodiedFunc(self, fn):
        self.fn = self.funcs[fn.id]
        self.coro = None
        self.set_debug_info_for_node(fn)
        self.fn.set_linkage(
            llvm.Linkage.EXTERNAL if fn.jit or irtools.is_export(fn) else llvm.Linkage.PRIVATE
        )
        if fn.jit:
            self.fn.add_attribute("jit", "")
        if irtools.is_inline(fn):
            self.fn.add_attribute("alwaysinline")
        if irtools.is_no_inline(fn):
            self.fn.add_attribute("noinline")
        if irtools.is_gpu_kernel(fn):
            self.fn.set_linkage(llvm.Linkage.EXTERNAL)
            self.fn.add_attribute("noinline")
            self.fn.add_attribute("kernel", "")
        self.fn.set_personality(self.personality())

        entry = self.fn.append_block("entry")
        self.b.set_insert_point(entry)
        for idx, (var, arg) in enumerate(zip(fn.args, self.fn.args, strict=True), 1):
            typ = self.get_llvm_type(var.type)
            storage = self.b.create_alloca(typ)
            self.vars.setdefault(var.id, storage)
            if self.is_storable(typ):
                self.b.create_store(arg, storage)
            self.declare_local(var, storage, entry, idx)
        for var in fn:
            typ = self.get_llvm_type(var.type)
            if typ.is_void:
                self.vars.setdefault(var.id, self.dummy(typ))
            else:
                storage = self.b.create_alloca(typ)
                # C++ insertVar uses emplace. A dominated loop/catch variable
                # can already refer to a global or an earlier local slot.
                self.vars.setdefault(var.id, storage)
                self.declare_local(var, storage, entry)

        start = self.fn.append_block("start")
        if fn.generator or fn.async_:
            self.start_coroutine(fn, entry, start)
        else:
            self.b.create_br(start)
            self.block = start
        self.process(fn.body)
        self.b.set_insert_point(self.block)
        if self.coro:
            self.b.create_br(self.coro.exit)
        else:
            ret = self.get_llvm_type(fn.type.return_type)
            if ret.is_void:
                self.b.create_ret_void()
            else:
                self.b.create_ret(ret.null())

    # C++: codon/cir/llvm/llvisitor.cpp:2053
    def start_coroutine(self, fn, entry, start):
        b, m = self.b, self.mod
        if not fn.async_:
            self.fn.add_attribute("codon.generator", "")
        self.fn.add_attribute("presplitcoroutine")
        coro_id = m.intrinsic("llvm.coro.id")
        coro_begin = m.intrinsic("llvm.coro.begin")
        coro_size = m.intrinsic("llvm.coro.size", (self.i64,))
        coro_end = m.intrinsic("llvm.coro.end")
        coro_alloc = m.intrinsic("llvm.coro.alloc")
        coro_free = m.intrinsic("llvm.coro.free")
        coro = self.coro = SimpleNamespace(
            cleanup=self.fn.append_block("coro.cleanup"),
            suspend=self.fn.append_block("coro.suspend"),
            exit=self.fn.append_block("coro.exit"),
            async_=fn.async_,
        )
        alloc_block = self.fn.append_block("coro.alloc")
        free_block = self.fn.append_block("coro.free")
        null = self.ptr.null()
        coro.promise = b.create_alloca(
            self.get_llvm_type(fn.type.return_type.base), name="coro.promise"
        )
        idx = b.create_call(coro_id, (self.i32.null(), coro.promise, null, null), name="coro.id")
        b.create_cond_br(b.create_call(coro_alloc, (idx,)), alloc_block, start)

        b.set_insert_point(alloc_block)
        size = b.create_call(coro_size)
        alloc = b.create_call(self.alloc_func(False), (size,))
        b.create_br(start)

        b.set_insert_point(start)
        phi = b.create_phi(self.ptr)
        phi.add_incoming(((null, entry), (alloc, alloc_block)))
        coro.handle = b.create_call(coro_begin, (idx, phi), name="coro.handle")

        b.set_insert_point(coro.cleanup)
        mem = b.create_call(coro_free, (idx, coro.handle))
        b.create_cond_br(b.create_is_not_null(mem), free_block, coro.suspend)
        b.set_insert_point(free_block)  # GC will free automatically.
        b.create_br(coro.suspend)
        b.set_insert_point(coro.suspend)
        b.create_call(coro_end, (coro.handle, self.i1.null(), self.token_none))
        b.create_ret(coro.handle)

        self.block = coro.exit
        self.make_yield(final=True)
        b.set_insert_point(self.block)
        b.create_unreachable()
        self.block = start
        self.make_yield()

    # C++: codon/cir/llvm/llvisitor.cpp:1788
    def make_yield(self, value=None, *, final=False):
        self.b.set_insert_point(self.block)
        if value:
            self.b.create_store(value, self.coro.promise)
        suspend = self.mod.intrinsic("llvm.coro.suspend")
        result = self.b.create_call(suspend, (self.token_none, self.i1.const_int(final)))
        self.block = self.fn.append_block("yield.new")
        switch = self.b.create_switch(result, self.coro.suspend, 2)
        switch.add_case(self.i8.const_int(0), self.block)
        switch.add_case(self.i8.const_int(1), self.coro.cleanup)

    # C++: codon/cir/llvm/llvisitor.cpp:1387
    def alloc_func(self, atomic, uncollectable=False):
        name = "seq_alloc_atomic" if atomic else "seq_alloc"
        if uncollectable:
            name += "_uncollectable"
        fn = self.mod.get_or_insert_function(name, self.ptr.function((self.i64,)))
        fn.add_attribute("nounwind")
        fn.add_attribute("noalias", index=0)
        fn.add_attribute("noundef", index=0)
        fn.add_attribute("nonnull", index=0)
        fn.add_attribute("memory", 3 << 2)  # inaccessiblemem: readwrite
        fn.add_attribute("allockind", 1 | 8)  # alloc, uninitialized
        fn.add_attribute("allocsize", (1 << 32) - 1)  # argument 0; no count argument
        fn.add_attribute("alloc-family", "seq_alloc")
        return fn

    # C++: codon/cir/llvm/llvisitor.cpp:1406
    def realloc_func(self):
        # seq_realloc takes (ptr, new_size, old_size).
        fn = self.mod.get_or_insert_function(
            "seq_realloc", self.ptr.function((self.ptr, self.i64, self.i64))
        )
        fn.add_attribute("nounwind")
        fn.add_attribute("noundef", index=0)
        fn.add_attribute("nonnull", index=0)
        fn.add_attribute("allocptr", index=1)
        fn.add_attribute("allockind", 2 | 8)  # realloc, uninitialized
        fn.add_attribute("allocsize", (1 << 32) | ((1 << 32) - 1))
        fn.add_attribute("alloc-family", "seq_alloc")
        return fn

    # C++: codon/cir/llvm/llvisitor.cpp:1423
    def free_func(self):
        fn = self.mod.get_or_insert_function("seq_free", self.void.function((self.ptr,)))
        fn.add_attribute("nounwind")
        fn.add_attribute("allocptr", index=1)
        fn.add_attribute("allockind", 4)  # free
        fn.add_attribute("alloc-family", "seq_alloc")
        return fn

    # C++: codon/cir/llvm/llvisitor.cpp:1436
    def personality(self):
        return self.mod.get_or_insert_function(
            "seq_personality", self.i32.function((self.i32, self.i32, self.i64, self.ptr, self.ptr))
        )

    # C++: codon/cir/llvm/llvisitor.cpp:2096
    def visit_Module(self, node):
        self.db = self.mod.create_debug_builder()
        self.unit = self.db.compile_unit(
            self.debug_file(self.source(node)), f"codon version {self.version}", not self.debug
        )
        self.mod.add_warning_flag("Debug Info Version", self.i32.const_int(3))
        target = self.mod.target
        if "darwin" in target or "macos" in target:
            self.mod.add_warning_flag("Dwarf Version", self.i32.const_int(2))
        # Windows uses CodeView for the PDB that provides symbolized backtraces.
        if "windows" in target:
            self.mod.add_warning_flag("CodeView", self.i32.const_int(1))
        self.register_global(node.argv_var)
        self.register_global(node.argc_var)
        for var in node:
            self.register_global(var.get_actual())
        for var in node:
            if isinstance(var.get_actual(), ir.Func):
                self.process(var)

        main = node.main_func
        self.funcs[main.id] = real_main = self.make_llvm_function(main)
        self.process(main)
        self.set_debug_info_for_node(None)

        b, m = self.b, self.mod
        init = m.get_or_insert_function("seq_init", self.void.function((self.i32,)))
        strlen = m.get_or_insert_function("strlen", self.i64.function((self.ptr,)))
        canonical = m.add_function(
            ".main.unclash" if m.function("main") else "main",
            self.i32.function((self.i32, self.ptr)),
        )
        canonical.set_personality(self.personality())
        argc, argv = canonical.args
        argc.set_name("argc")
        argv.set_name("argv")
        entry, loop, body, exit_ = map(canonical.append_block, ("entry", "loop", "body", "exit"))
        b.set_insert_point(entry)
        alloc = self.alloc_func(False)
        length = b.create_zext(argc, self.i64)
        size = b.create_mul(length, self.i64.const_int(self.layout.alloc_size(self.str_type)))
        ptr = b.create_call(alloc, (size,))
        b.create_br(loop)

        b.set_insert_point(loop)
        i = b.create_phi(self.i32, name="i")
        next_ = b.create_add(i, self.i32.const_int(1), name="next")
        cond = b.create_icmp(llvm.Predicate.SLT, i, argc)
        i.add_incoming(((self.i32.null(), entry), (next_, body)))
        b.create_cond_br(cond, body, exit_)

        b.set_insert_point(body)
        arg = b.create_load(self.ptr, b.create_gep(self.ptr, argv, (i,)))
        arg_len = b.create_call(strlen, (arg,))
        string = b.create_insert_value(self.str_type.undef(), arg, 0)
        string = b.create_insert_value(string, arg_len, 1)
        b.create_store(string, b.create_gep(self.str_type, ptr, (i,)))
        b.create_br(loop)

        b.set_insert_point(exit_)
        b.create_store(ptr, self.vars[node.argv_var.id])
        b.create_store(length, self.vars[node.argc_var.id])
        flags = int(self.debug) | (4 if self.standalone else 0)
        b.create_call(init, (self.i32.const_int(flags),))

        # Put the entire program in a new function, catching uncaught exceptions.
        proxy = m.add_function("codon.proxy_main", self.void.function(()))
        proxy.set_linkage(llvm.Linkage.PRIVATE)
        proxy.set_personality(self.personality())
        entry, main, done = map(proxy.append_block, ("entry", "main", "exit"))
        b.set_insert_point(entry)
        b.create_cond_br(self.i1.null(), done, main)
        b.set_insert_point(done)
        b.create_ret_void()
        normal, unwind = map(proxy.append_block, ("normal", "unwind"))
        b.set_insert_point(main)
        b.create_invoke(real_main, normal, unwind)
        b.set_insert_point(unwind)
        pad = b.create_landing_pad(m.struct_type((self.ptr, self.i32)), self.personality(), 1)
        pad.set_cleanup()
        pad.add_clause(self.type_idx_var(None))
        exc = b.create_extract_value(pad, 0)
        terminate = m.get_or_insert_function("seq_terminate", self.void.function((self.ptr,)))
        terminate.add_attribute("noreturn")
        b.create_call(terminate, (exc,))
        b.create_unreachable()
        b.set_insert_point(normal)
        b.create_ret_void()
        b.set_insert_point(exit_)
        b.create_call(proxy)
        b.create_ret(self.i32.null())
        for atomic, uncollectable, name in (
            (False, False, "seq_alloc"),
            (True, False, "seq_alloc_atomic"),
            (False, True, "seq_alloc_uncollectable"),
            (True, True, "seq_alloc_atomic_uncollectable"),
        ):
            if m.function(name):
                self.alloc_func(atomic, uncollectable)
        if m.function("seq_realloc"):
            self.realloc_func()
        if m.function("seq_free"):
            self.free_func()

    # C++: codon/cir/llvm/llvisitor.cpp:1965
    def type_idx_var(self, typ):
        name = f"codon.typeidx.{typ.name if typ else '<all>'}"
        # Module::getGlobalVariable skips local linkage by default.
        if existing := self.mod.global_variable(name, allow_local=False):
            return existing
        idx = 0 if typ is None else self.type_idx(typ)
        result = self.mod.add_global(
            name,
            self.mod.struct_type((self.i32,)),
            self.mod.const_struct((self.i32.const_int(idx),)),
            constant=True,
            linkage=llvm.Linkage.PRIVATE,
        )
        result.set_unnamed_addr()
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:1472
    def type_idx(self, typ):
        if typ.runtime_info and typ.runtime_info.exception_base == typ:
            return -1
        return typ.module.cache.get_realization_id(typ.ast_type.require_cls)

    # C++: codon/cir/llvm/llvisitor.cpp:2150
    def visit_VarValue(self, node):
        var = node.var.get_actual()
        if isinstance(var, ir.Func):
            return self.get_func(var)
        typ = self.get_llvm_type(node.type)
        if not self.is_storable(typ):
            return self.dummy(typ)
        self.b.set_insert_point(self.block)
        return self.b.create_load(typ, self.var_address(var))

    def var_address(self, var):
        result = self.vars[var.id]
        if var.tls:
            fn = self.mod.intrinsic("llvm.threadlocal.address", (self.ptr,))
            return self.b.create_call(fn, (result,))
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:2169
    def visit_PointerValue(self, node):
        self.b.set_insert_point(self.block)
        address, typ = self.var_address(node.var), node.var.type
        for field in node.fields:
            if isinstance(typ, ir.RefType):
                obj = self.b.create_load(self.get_llvm_type(typ), address)
                address = self.field_address(obj, typ, field)
            else:
                assert isinstance(typ, ir.RecordType)
                idx = typ.get_member_index(field)
                assert idx >= 0
                address = self.b.create_struct_gep(self.get_llvm_type(typ), address, idx)
            typ = typ.get_member_type(field)
        return address

    # C++: codon/cir/llvm/llvisitor.cpp:2539
    def visit_IntConst(self, node):
        return self.i64.const_int(node.val)

    # C++: codon/cir/llvm/llvisitor.cpp:2544
    def visit_FloatConst(self, node):
        return self.mod.primitive_type("Double").const_real(node.val)

    # C++: codon/cir/llvm/llvisitor.cpp:2549
    def visit_BoolConst(self, node):
        return self.i8.const_int(node.val)

    # C++: codon/cir/llvm/llvisitor.cpp:2554
    def visit_StringConst(self, node):
        return self.string_literal(node.val)

    def string_literal(self, text):
        # The parser keeps the original UTF-8 bytes with surrogateescape.
        # Codon's str decoder accepts UTF-8 encodings of surrogate code points
        # (e.g. a source literal of "\\ud800"), as distinct from raw bytes.
        if not text.isascii():
            text = text.encode("utf-8", "surrogateescape").decode("utf-8", "surrogatepass")
        points = list(map(ord, text))
        max_char = max(points, default=0)
        kind, typ = (
            (3, self.i32)
            if max_char > 0xFFFF
            else (2, self.i16)
            if max_char > 0xFF
            else (int(max_char > 0x7F), self.i8)
        )
        assert len(points) < 1 << 56, "string constant too large"
        return self.literal_data(".str", typ, points, len(points) | (kind << 56))

    # C++: codon/cir/llvm/llvisitor.cpp:2635
    def visit_BytesConst(self, node):
        # The lexer decodes raw bytes with UTF-8 + surrogateescape.
        data = (
            node.val if isinstance(node.val, bytes) else node.val.encode("utf-8", "surrogateescape")
        )
        return self.literal_data(".bytes", self.i8, data, len(data))

    def literal_data(self, name, typ, data, meta):
        elements = [typ.const_int(point) for point in data]
        elements.append(typ.null())
        result = self.mod.add_global(
            name,
            typ.array(len(elements)),
            typ.const_array(elements),
            constant=True,
            linkage=llvm.Linkage.PRIVATE,
        )
        result.set_unnamed_addr()
        return self.mod.const_struct((result, self.i64.const_int(meta)))

    # C++: codon/cir/llvm/llvisitor.cpp:3383
    def visit_AssignInstr(self, node):
        result = self.process(node.rhs)
        if self.is_storable(self.get_llvm_type(node.lhs.type)):
            self.b.set_insert_point(self.block)
            self.b.create_store(result, self.var_address(node.lhs))

    # C++: codon/cir/llvm/llvisitor.cpp:3395
    def visit_ExtractInstr(self, node):
        typ = node.val.type
        value = self.process(node.val)
        self.b.set_insert_point(self.block)
        if isinstance(typ, ir.RefType):
            exact = node.get_attribute(ir.ExactTypeAttribute)
            addr = self.field_address(value, typ, node.field, exact.type if exact else None)
            return self.b.create_load(self.get_llvm_type(node.type), addr)
        idx = typ.get_member_index(node.field)
        assert idx >= 0
        return self.b.create_extract_value(value, idx)

    # C++: codon/cir/llvm/llvisitor.cpp:3413
    def visit_InsertInstr(self, node):
        lhs = self.process(node.lhs)
        rhs = self.process(node.rhs)
        self.b.set_insert_point(self.block)
        exact = node.get_attribute(ir.ExactTypeAttribute)
        addr = self.field_address(lhs, node.lhs.type, node.field, exact.type if exact else None)
        self.b.create_store(rhs, addr)
        return lhs

    # C++: codon/cir/llvm/llvisitor.cpp:4113
    def visit_CallInstr(self, node):
        self.b.set_insert_point(self.block)
        if isinstance(node.callee, ir.VarValue):
            fn = node.callee.var.get_actual()
            if isinstance(fn, ir.InternalFunc) and fn.is_type_query():
                assert not node.args
                return self.codegen_type_query(fn)
        # Direct calls use the logical signature, without lowering the callee as
        # a first-class function value.
        direct = irtools.get_func(node.callee)
        fn = self.get_func(direct) if direct else self.process(node.callee)
        typ = self.get_llvm_func_type(node.callee.type)
        fn, targets, dynamic_id = self.specialize_virtual_call(fn, typ)
        args = [self.process(arg) for arg in node]
        self.b.set_insert_point(self.block)
        if targets:
            return self.emit_virtual_call(node, typ, args, targets, dynamic_id)
        result = self.call(fn, args, typ=typ)
        ret = self.get_llvm_type(node.type)
        return result if self.is_storable(ret) else self.dummy(ret)

    # C++: codon/cir/llvm/llvisitor.cpp:1499
    def call(self, fn, args=(), *, typ=None):
        self.b.set_insert_point(self.block)
        region = self.innermost_try_catch()
        if region is None:
            return self.b.create_call(fn, args, typ=typ)
        normal = self.fn.append_block("invoke.normal")
        unwind = (
            region.exception_block
            if self.trycatch and region is self.trycatch[-1]
            else region.finally_exception_block
        )
        result = self.b.create_invoke(fn, normal, unwind, args, typ=typ)
        self.block = normal
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:1524
    def enter_region(self, stack, data):
        data = copy(data)
        data.sequence_number = self.next_sequence_number
        self.next_sequence_number += 1
        stack.append(data)

    # C++: codon/cir/llvm/llvisitor.cpp:1564
    def innermost_try_catch(self):
        if not self.trycatch:
            return self.finally_[-1] if self.finally_ else None
        if (
            not self.finally_
            or self.trycatch[-1].sequence_number > self.finally_[-1].sequence_number
        ):
            return self.trycatch[-1]
        return self.finally_[-1]

    # C++: codon/cir/llvm/llvisitor.cpp:1467
    def pad_type(self):
        return self.mod.struct_type((self.ptr, self.i32))

    # C++: codon/cir/llvm/llvisitor.cpp:1449
    def throw_func(self):
        fn = self.mod.get_or_insert_function("seq_throw", self.void.function((self.ptr,)))
        fn.add_attribute("noreturn")
        return fn

    # C++: codon/cir/llvm/llvisitor.cpp:2851
    def visit_TryCatchFlow(self, node):
        lower_try_catch(self, node)

    # C++: codon/cir/llvm/llvisitor.cpp:3510
    def specialize_virtual_call(self, fn, typ):
        if not fn.is_call or fn.call_arg_count != 2 or fn.has_uses:
            return fn, (), None
        callee = fn.called_function
        receiver = self.virtual_lookup_types.get(callee.name) if callee else None
        thunk = fn.operand(1).signed_int() if receiver else None
        if thunk is None:
            return fn, (), None
        key = (receiver.runtime_info.id, thunk)
        if key not in self.virtual_targets:
            self.virtual_targets[key] = self.find_virtual_targets(receiver, thunk, typ)
        targets = self.virtual_targets[key]
        if not targets:
            return fn, (), None
        if all(target == targets[0][1] for _, target in targets):
            # Different concrete classes can share one implementation.
            fn.erase_instruction()
            return targets[0][1], (), None
        # The receiver expression has already run. Validate before evaluating
        # arguments, then use a second switch to dispatch after evaluating them once.
        b = self.b
        b.set_insert_point(self.block)
        header = b.create_load(self.ptr, fn.operand(0))
        dynamic_id = self.load_runtime_field(header, 0, self.i64)
        valid, missing = map(self.fn.append_block, ("dispatch.valid", "dispatch.missing"))
        selection = b.create_switch(dynamic_id, missing, 0)
        for idx, _ in targets:
            selection.add_case(self.i64.const_int(idx), valid)
        b.set_insert_point(missing)
        b.create_call(self.mod.intrinsic("llvm.trap"))
        b.create_unreachable()
        b.set_insert_point(valid)
        self.block = valid
        fn.erase_instruction()
        return None, targets, dynamic_id

    # C++: codon/cir/llvm/llvisitor.cpp:3525
    def find_virtual_targets(self, receiver, thunk, typ):
        # Every compatible subtype must be covered and share the call ABI.
        candidates = []
        singleton = True
        for candidate in receiver.module.types:
            info = candidate.runtime_info
            if not info or receiver not in info.mro:
                continue
            target = info.methods.get(thunk)
            if not target:
                return ()
            fn = self.get_func(target)
            if fn.value_type != typ or len(candidates) == 64:
                return ()
            candidates.append((info.id, fn))
            singleton &= fn == candidates[0][1]
            if not singleton and len(candidates) > 4:
                return ()
        return candidates

    # C++: codon/cir/llvm/llvisitor.cpp:3606
    def emit_virtual_call(self, node, typ, args, targets, dynamic_id):
        b = self.b
        ret = self.get_llvm_type(node.type)
        done, missing = map(self.fn.append_block, ("dispatch.done", "dispatch.unreachable"))
        selection = b.create_switch(dynamic_id, missing, 0)
        incoming = []
        for idx, target in targets:
            dest = self.fn.append_block("dispatch.call")
            selection.add_case(self.i64.const_int(idx), dest)
            self.block = dest
            value = self.call(target, args, typ=typ)
            incoming.append((value, self.block))
            b.set_insert_point(self.block)
            b.create_br(done)
        b.set_insert_point(missing)
        b.create_unreachable()
        b.set_insert_point(done)
        self.block = done
        if not self.is_storable(ret):
            return self.dummy(ret)
        result = b.create_phi(ret)
        result.add_incoming(incoming)
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:3603
    def visit_StackAllocInstr(self, node):
        self.b.position_before(self.fn.entry_block.terminator)
        return self.b.create_array_alloca(
            self.get_llvm_type(node.type.base), self.i64.const_int(node.count)
        )

    # C++: codon/cir/llvm/llvisitor.cpp:3682
    def visit_ReturnInstr(self, node):
        value = self.process(node.value) if node.value else None
        self.b.set_insert_point(self.block)
        tc = self.innermost_try_catch()
        if self.coro:
            if self.coro.async_:
                self.b.create_store(value, self.coro.promise)
            if tc:
                self.b.create_store(self.i8.const_int(ExceptionState.RETURN), tc.exc_flag)
                self.b.create_br(tc.finally_block)
            else:
                self.b.create_br(self.coro.exit)
        elif tc:
            self.b.create_store(self.i8.const_int(ExceptionState.RETURN), tc.exc_flag)
            if tc.ret_store:
                assert value, "no return value storage"
                self.b.create_store(value, tc.ret_store)
            self.b.create_br(tc.finally_block)
        elif value:
            self.b.create_ret(value)
        else:
            self.b.create_ret_void()
        self.block = self.fn.append_block("return.new")

    # C++: codon/cir/llvm/llvisitor.cpp:3718
    def visit_YieldInstr(self, node):
        value = self.process(node.value) if node.value else None
        if node.final:
            self.b.set_insert_point(self.block)
            if value:
                self.b.create_store(value, self.coro.promise)
            if tc := self.innermost_try_catch():
                self.b.create_store(self.i8.const_int(ExceptionState.RETURN), tc.exc_flag)
                self.b.create_br(tc.finally_block)
            else:
                self.b.create_br(self.coro.exit)
            self.block = self.fn.append_block("yield.new")
        else:
            self.make_yield(value)

    # C++: codon/cir/llvm/llvisitor.cpp:3585
    def visit_YieldInInstr(self, node):
        self.b.set_insert_point(self.block)
        if node.suspend:
            suspend = self.mod.intrinsic("llvm.coro.suspend")
            result = self.b.create_call(suspend, (self.token_none, self.i1.null()))
            self.block = self.fn.append_block("yieldin.new")
            switch = self.b.create_switch(result, self.coro.suspend, 2)
            switch.add_case(self.i8.const_int(0), self.block)
            switch.add_case(self.i8.const_int(1), self.coro.cleanup)
            self.b.set_insert_point(self.block)
        return self.b.create_load(self.get_llvm_type(node.type), self.coro.promise)

    # C++: codon/cir/llvm/llvisitor.cpp:3749
    def visit_ThrowInstr(self, node):
        exc_alloc = self.mod.get_or_insert_function("seq_alloc_exc", self.ptr.function((self.ptr,)))
        exc_alloc.add_attribute("nounwind")
        throw = self.throw_func()
        if node.value:
            obj = self.process(node.value)
        else:
            obj = self.catches[-1].exception if self.catches else self.ptr.null()
            empty = self.fn.append_block("raise.empty")
            active = self.fn.append_block("raise.active")
            self.b.set_insert_point(self.block)
            self.b.create_cond_br(self.b.create_is_null(obj), empty, active)
            self.block = empty
            self.process(node.fallback)
            self.b.set_insert_point(self.block)
            self.b.create_unreachable()
            self.block = active
        self.b.set_insert_point(self.block)
        self.call(throw, (self.b.create_call(exc_alloc, (obj,)),))

    # C++: codon/cir/llvm/llvisitor.cpp:3785
    def visit_FlowInstr(self, node):
        self.process(node.flow)
        return self.process(node.value)

    # C++: codon/cir/llvm/llvisitor.cpp:2666
    def visit_SeriesFlow(self, node):
        for value in node:
            self.process(value)

    # C++: codon/cir/llvm/llvisitor.cpp:2672
    def visit_IfFlow(self, node):
        b = self.b
        yes, no, done = map(self.fn.append_block, ("if.true", "if.false", "if.exit"))
        cond = self.process(node.cond)
        b.set_insert_point(self.block)
        b.create_cond_br(b.create_trunc(cond, self.i1), yes, no)
        for block, branch in ((yes, node.true_branch), (no, node.false_branch)):
            self.block = block
            if branch:
                self.process(branch)
            b.set_insert_point(self.block)
            b.create_br(done)
        self.block = done

    # C++: codon/cir/llvm/llvisitor.cpp:3610
    def visit_TernaryInstr(self, node):
        b = self.b
        yes, no, done = map(self.fn.append_block, ("ternary.true", "ternary.false", "ternary.exit"))
        typ = self.get_llvm_type(node.type)
        cond = self.process(node.cond)
        b.set_insert_point(self.block)
        b.create_cond_br(b.create_trunc(cond, self.i1), yes, no)
        incoming = []
        for block, expr in ((yes, node.true_value), (no, node.false_value)):
            self.block = block
            value = self.process(expr)
            b.set_insert_point(self.block)
            incoming.append((value, self.block))
            b.create_br(done)
        b.set_insert_point(done)
        self.block = done
        phi = b.create_phi(typ)
        phi.add_incoming(incoming)
        return phi

    # C++: codon/cir/llvm/llvisitor.cpp:2700
    def visit_WhileFlow(self, node):
        b = self.b
        test, body, done = map(self.fn.append_block, ("while.cond", "while.body", "while.exit"))
        b.set_insert_point(self.block)
        b.create_br(test)
        self.block = test
        cond = self.process(node.cond)
        b.set_insert_point(self.block)
        b.create_cond_br(b.create_trunc(cond, self.i1), body, done)
        self.loop_body(node, body, test, done)
        self.block = done

    # C++: codon/cir/llvm/llvisitor.cpp:2726
    def visit_ForFlow(self, node):
        assert not node.schedule, "parallel for-loop not lowered"
        b, m = self.b, self.mod
        typ, var = self.get_llvm_type(node.var.type), self.vars[node.var.id]
        test, body, check, cleanup, done = map(
            self.fn.append_block, ("for.cond", "for.body", "for.check", "for.cleanup", "for.exit")
        )
        resume, is_done, promise, destroy = (
            m.intrinsic("llvm.coro." + name) for name in ("resume", "done", "promise", "destroy")
        )
        it = self.process(node.iter)
        b.set_insert_point(self.block)
        b.create_cond_br(b.create_call(is_done, (it,)), done, test)
        b.set_insert_point(check)
        b.create_cond_br(b.create_call(is_done, (it,)), done, test)
        self.block = test
        self.call(resume, (it,))
        b.set_insert_point(self.block)
        b.create_cond_br(b.create_call(is_done, (it,)), cleanup, body)
        if self.is_storable(typ):
            b.set_insert_point(body)
            alignment = self.i32.const_int(self.layout.preferred_alignment(typ))
            ptr = b.create_call(promise, (it, alignment, self.i1.null()))
            b.create_store(b.create_load(typ, ptr), var)
        self.loop_body(node, body, check, done)
        b.set_insert_point(cleanup)
        b.create_call(destroy, (it,))
        b.create_br(done)
        self.block = done

    # C++: codon/cir/llvm/llvisitor.cpp:2788
    def visit_ImperativeForFlow(self, node):
        assert not node.schedule and node.step, "unlowered parallel loop or zero step"
        b = self.b
        var = self.vars[node.var.id]
        test, body, update, done = map(
            self.fn.append_block, ("imp_for.cond", "imp_for.body", "imp_for.update", "imp_for.exit")
        )
        start = self.process(node.start)
        end = self.process(node.end)
        b.set_insert_point(self.block)
        b.create_br(test)
        b.set_insert_point(test)
        phi = b.create_phi(self.i64)
        phi.add_incoming(((start, self.block),))
        compare = llvm.Predicate.SGE if node.step > 0 else llvm.Predicate.SLE
        b.create_cond_br(b.create_icmp(compare, phi, end), done, body)
        b.set_insert_point(body)
        b.create_store(phi, var)
        self.loop_body(node, body, update, done)
        b.set_insert_point(update)
        phi.add_incoming(((b.create_add(phi, self.i64.const_int(node.step)), update),))
        b.create_br(test)
        self.block = done

    def loop_body(self, node, body, update, done):
        self.block = body
        self.enter_region(self.loops, SimpleNamespace(id=node.id, update=update, done=done))
        self.process(node.body)
        self.loops.pop()
        self.b.set_insert_point(self.block)
        self.b.create_br(update)

    # C++: codon/cir/llvm/llvisitor.cpp:2527
    def loop_data(self, node):
        if node.loop:
            return next(loop for loop in reversed(self.loops) if loop.id == node.loop.id)
        return self.loops[-1]

    # C++: codon/cir/llvm/llvisitor.cpp:3645
    def visit_BreakInstr(self, node):
        loop = self.loop_data(node)
        self.exit_loop(loop, loop.done, ExceptionState.BREAK)
        self.block = self.fn.append_block("break.new")

    # C++: codon/cir/llvm/llvisitor.cpp:3664
    def visit_ContinueInstr(self, node):
        loop = self.loop_data(node)
        self.exit_loop(loop, loop.update, ExceptionState.CONTINUE)
        self.block = self.fn.append_block("continue.new")

    def exit_loop(self, loop, target, state):
        self.b.set_insert_point(self.block)
        if not self.finally_ or self.finally_[-1].sequence_number < loop.sequence_number:
            self.b.create_br(target)
        else:
            tc = self.finally_[-1]
            self.b.create_store(self.i8.const_int(state), tc.exc_flag)
            self.b.create_store(self.i64.const_int(loop.sequence_number), tc.loop_sequence)
            self.b.create_br(tc.finally_block)

    # C++: codon/cir/llvm/llvisitor.cpp:3795
    def runtime_type_info_type(self):
        i, p, s = self.i64, self.ptr, self.str_type
        return self.mod.struct_type(
            (i, i, p, p, p, i, p, i, p, self.i8, s, s, s, i, p, i, self.i8, self.i8, i, i)
        )

    # C++: codon/cir/llvm/llvisitor.cpp:3806
    def own_layout(self, typ):
        return self.mod.struct_type(
            self.get_llvm_type(field.type)
            for field, owner in zip(typ, typ.runtime_info.field_owners, strict=True)
            if owner == typ
        )

    # C++: codon/cir/llvm/llvisitor.cpp:3818
    def object_layout(self, typ):
        if not typ.polymorphic:
            return self.get_llvm_type(typ.contents)
        # References keep pointing at the header; base-first segments follow
        # in reverse MRO order, with padding supplied by LLVM.
        return self.mod.struct_type(
            (self.ptr, *(self.own_layout(base) for base in reversed(typ.runtime_info.mro)))
        )

    # C++: codon/cir/llvm/llvisitor.cpp:3833
    def object_owner_offset(self, typ, owner):
        mro = typ.runtime_info.mro
        return self.layout.offset_of(self.object_layout(typ), len(mro) - mro.index(owner))

    # C++: codon/cir/llvm/llvisitor.cpp:3843
    def object_field_offset(self, typ, idx):
        if not typ.polymorphic:
            return self.layout.offset_of(self.object_layout(typ), idx)
        owners = typ.runtime_info.field_owners
        owner = owners[idx]
        local_idx = owners[:idx].count(owner)
        return self.object_owner_offset(typ, owner) + self.layout.offset_of(
            self.own_layout(owner), local_idx
        )

    # C++: codon/runtime/rtti.h:39
    @staticmethod
    def method_hash(idx):
        mixed = (idx * 0x9E3779B97F4A7C15) & ((1 << 64) - 1)
        return mixed ^ (mixed >> 32)

    # C++: codon/cir/llvm/llvisitor.cpp:3858
    def runtime_type_info(self, typ):
        info = typ.runtime_info
        assert info, f"missing runtime metadata for {typ.name}"
        name = f".codon.typeinfo.{info.id}"
        if existing := self.mod.global_variable(name):
            return existing
        m, i, p = self.mod, self.i64, self.ptr
        result = m.add_global(
            name, self.runtime_type_info_type(), constant=True, linkage=llvm.Linkage.PRIVATE
        )

        def array(element, values, suffix):
            if not values:
                return p.null()
            return m.add_global(
                name + suffix,
                element.array(len(values)),
                element.const_array(values),
                constant=True,
                linkage=llvm.Linkage.PRIVATE,
            )

        ref = isinstance(typ, ir.RefType)
        polymorphic = ref and typ.polymorphic
        exception_offset = system_exit_offset = -1
        mro, offsets = [], []
        for base in info.mro:
            mro.append(base.runtime_info.id)
            offset = self.object_owner_offset(typ, base) if polymorphic else 0
            offsets.append(offset)
            if base == info.exception_base:
                exception_offset = offset
            if base == info.system_exit_base:
                system_exit_offset = offset
        if polymorphic:
            # Preserve the public MRO-order prefix. Its immutable tail is the
            # mask followed by keyed {owner ID, offset} entries.
            capacity = 1
            while capacity < 2 * len(mro):
                capacity *= 2
            index = [0] * (2 * capacity)
            for idx, offset in zip(mro, offsets, strict=True):
                slot = self.method_hash(idx) & (capacity - 1)
                while index[2 * slot + 1]:
                    slot = (slot + 1) & (capacity - 1)
                index[2 * slot : 2 * slot + 2] = idx, offset
            offsets += [capacity - 1, *index]

        slot_type = m.struct_type((self.str_type, i, i))
        slots = []
        if isinstance(typ, (ir.RefType, ir.RecordType)):
            for idx, field in enumerate(typ):
                offset = (
                    self.object_field_offset(typ, idx)
                    if ref
                    else self.layout.offset_of(self.get_llvm_type(typ), idx)
                )
                slots.append(
                    m.const_struct(
                        (
                            self.string_literal(field.name),
                            i.const_int(field.type.runtime_info.id),
                            i.const_int(offset),
                        )
                    )
                )
        if isinstance(typ, ir.OptionalType):
            offset = (
                0
                if isinstance(typ.base, ir.RefType)
                else self.layout.offset_of(self.get_llvm_type(typ), 1)
            )
            slots.append(
                m.const_struct(
                    (
                        self.string_literal("value"),
                        i.const_int(typ.base.runtime_info.id),
                        i.const_int(offset),
                    )
                )
            )
        params = [i.const_int(param.runtime_info.id) for param in info.parameters]

        method_count = int(bool(info.methods))
        while method_count < 2 * len(info.methods):
            method_count *= 2
        method_type = m.struct_type((i, p))
        methods = [method_type.null()] * method_count
        occupied = set()
        for idx, target in sorted(info.methods.items()):
            fn = self.get_func(target)
            assert idx, "zero is reserved for empty method slots"
            pos = self.method_hash(idx) & (method_count - 1)
            while pos in occupied:
                pos = (pos + 1) & (method_count - 1)
            occupied.add(pos)
            methods[pos] = m.const_struct((i.const_int(idx), fn))
        size = self.layout.alloc_size(self.get_llvm_type(typ))
        allow_inline = (
            isinstance(
                typ,
                (
                    ir.IntType,
                    ir.BoolType,
                    ir.FloatType,
                    ir.Float32Type,
                    ir.Float16Type,
                    ir.BFloat16Type,
                ),
            )
            or info.base_name == "NoneType"
        )
        inline = self.layout.pointer_size() == 8 and size <= 8 and allow_inline
        result.set_initializer(
            m.const_struct(
                (
                    i.const_int(info.id),
                    i.const_int(len(mro)),
                    array(i, list(map(i.const_int, mro)), ".mro"),
                    array(i, list(map(i.const_int, offsets)), ".offsets"),
                    array(i, list(map(i.const_int, sorted(mro))), ".sorted"),
                    i.const_int(len(slots)),
                    array(slot_type, slots, ".slots"),
                    i.const_int(method_count),
                    array(method_type, methods, ".methods"),
                    self.i8.const_int(polymorphic),
                    self.string_literal(info.raw_name),
                    self.string_literal(info.nice_name),
                    self.string_literal(info.base_name),
                    i.const_int(len(params)),
                    array(i, params, ".params"),
                    i.const_int(size),
                    self.i8.const_int(ref),
                    self.i8.const_int(inline),
                    i.const_int(exception_offset),
                    i.const_int(system_exit_offset),
                )
            )
        )
        return result

    # C++: codon/cir/llvm/llvisitor.cpp:4096
    def load_runtime_field(self, info, idx, typ):
        return self.b.create_load(
            typ, self.b.create_struct_gep(self.runtime_type_info_type(), info, idx)
        )

    # C++: codon/cir/llvm/llvisitor.cpp:4102
    def field_address(self, obj, typ, field, exact=None):
        idx = typ.get_member_index(field)
        assert idx >= 0, f"unknown field {field} of {typ.name}"
        b, i = self.b, self.i64
        if not typ.polymorphic:
            return b.create_struct_gep(self.object_layout(typ), obj, idx)
        info = typ.runtime_info
        owner = info.field_owners[idx]
        local_idx = info.field_owners[:idx].count(owner)
        offset = self.layout.offset_of(self.own_layout(owner), local_idx)
        if exact is not None:
            return b.create_gep(
                self.i8, obj, (i.const_int(offset + self.object_owner_offset(exact, owner)),)
            )
        key = info.id, owner.runtime_info.id
        if key not in self.field_offset_resolvers:
            # AOT: the closed set of compatible types gives either a constant
            # segment offset or a small resolver shared between every field.
            offsets = [
                (candidate.runtime_info.id, self.object_owner_offset(candidate, owner))
                for candidate in typ.module.types
                if isinstance(candidate, ir.RefType)
                and candidate.polymorphic
                and candidate.runtime_info
                and typ in candidate.runtime_info.mro
            ]
            assert offsets, f"missing AOT layout for {typ.name}"
            first = offsets[0][1]
            self.field_offset_resolvers[key] = (
                first
                if all(value == first for _, value in offsets)
                else self.make_owner_offset_resolver(key, offsets)
            )
        resolver = self.field_offset_resolvers[key]
        if isinstance(resolver, int):
            return b.create_gep(self.i8, obj, (i.const_int(resolver + offset),))
        header = b.create_load(self.ptr, obj)
        idx = self.load_runtime_field(header, 0, i)
        immutable = self.mod.metadata_node()
        header.set_metadata("invariant.load", immutable)
        idx.set_metadata("invariant.load", immutable)
        segment = b.create_call(resolver, (idx,))
        return b.create_gep(self.i8, obj, (b.create_add(segment, i.const_int(offset)),))

    # C++: codon/cir/llvm/llvisitor.cpp:4145
    def make_owner_offset_resolver(self, key, offsets):
        b, m, i = self.b, self.mod, self.i64
        prev = b.insert_block
        fn = m.add_function(f".codon.field.offset.{key[0]}.{key[1]}", i.function((i,)))
        fn.set_linkage(llvm.Linkage.PRIVATE)
        fn.add_attribute("memory", 0)
        fn.add_attribute("nounwind")
        entry, missing = map(fn.append_block, ("entry", "missing"))
        b.set_insert_point(entry)
        selection = b.create_switch(fn.args[0], missing, 0)
        destinations = {}
        for idx, offset in offsets:
            if offset not in destinations:
                destinations[offset] = target = fn.append_block("found")
                b.set_insert_point(target)
                b.create_ret(i.const_int(offset))
            selection.add_case(i.const_int(idx), destinations[offset])
        b.set_insert_point(missing)
        b.create_call(m.intrinsic("llvm.trap"))
        b.create_unreachable()
        b.set_insert_point(prev)
        return fn

    # C++: codon/cir/llvm/llvisitor.cpp:4282
    def codegen_type_query(self, fn):
        typ = fn.intrinsic_type
        match fn.intrinsic:
            case ir.Intrinsic.SIZEOF:
                return self.i64.const_int(self.layout.alloc_size(self.get_llvm_type(typ)))
            case ir.Intrinsic.IS_ATOMIC:
                return self.i8.const_int(typ.is_atomic())
            case ir.Intrinsic.IS_CONTENT_ATOMIC:
                return self.i8.const_int(typ.is_content_atomic())
        raise AssertionError(f"not a type query: {fn.intrinsic}")

    # C++: codon/cir/llvm/llvisitor.cpp:4298
    def codegen_intrinsic(self, fn, args):
        b, m = self.b, self.mod
        typ = fn.intrinsic_type
        match fn.intrinsic:
            case ir.Intrinsic.RECORD_NEW:
                result = self.get_llvm_type(typ).undef()
                for idx, arg in enumerate(args):
                    result = b.create_insert_value(result, arg, idx)
                return result
            case ir.Intrinsic.REPRESENTATION_CAST:
                assert len(args) == 1 and args[0].type == self.get_llvm_type(typ)
                return args[0]
            case ir.Intrinsic.GENERATOR_PROMISE:
                base = self.get_llvm_type(typ.base)
                if base.is_void:
                    return self.ptr.null()
                alignment = self.i32.const_int(self.layout.preferred_alignment(base))
                return b.create_call(
                    m.intrinsic("llvm.coro.promise"), (args[0], alignment, self.i1.null())
                )
            case ir.Intrinsic.ALLOCATION_SIZE:
                return self.i64.const_int(self.layout.alloc_size(self.object_layout(typ)))
            case ir.Intrinsic.ALLOCATE:
                size = max(1, self.layout.alloc_size(self.object_layout(typ)))
                obj = b.create_call(
                    self.alloc_func(typ.is_content_atomic()), (self.i64.const_int(size),)
                )
                if typ.polymorphic:
                    b.create_store(self.runtime_type_info(typ), obj)
                return obj
            case ir.Intrinsic.TYPEINFO:
                return self.runtime_type_info(typ)
            case ir.Intrinsic.TYPEINFO_LOOKUP:
                missing = self.fn.append_block("typeinfo.missing")
                selection = b.create_switch(args[0], missing, 0)
                for candidate in fn.module.types:
                    if not candidate.runtime_info:
                        continue
                    found = self.fn.append_block("typeinfo.found")
                    selection.add_case(self.i64.const_int(candidate.runtime_info.id), found)
                    b.set_insert_point(found)
                    b.create_ret(self.runtime_type_info(candidate))
                b.set_insert_point(missing)
                self.block = missing
                b.create_call(m.intrinsic("llvm.trap"))
                b.create_unreachable()
            case ir.Intrinsic.VIRTUAL_LOOKUP | ir.Intrinsic.TYPEINFO_METHOD_LOOKUP:
                info, idx = args
                if fn.intrinsic == ir.Intrinsic.VIRTUAL_LOOKUP:
                    info = b.create_load(self.ptr, info)
                return self.lookup_method(info, idx)
            case _:
                raise NotImplementedError(f"LLVM intrinsic lowering for {fn.intrinsic}")

    # C++: codon/cir/llvm/llvisitor.cpp:4394
    def lookup_method(self, info, idx):
        b, i, p = self.b, self.i64, self.ptr
        count = self.load_runtime_field(info, 7, i)
        valid, probe, next_, missing, found, done = (
            self.fn.append_block("method." + name)
            for name in ("valid", "probe", "next", "missing", "found", "done")
        )
        b.create_cond_br(b.create_icmp(llvm.Predicate.NE, count, i.null()), valid, missing)
        b.set_insert_point(valid)
        methods = self.load_runtime_field(info, 8, p)
        mask = b.create_sub(count, i.const_int(1))
        mixed = b.create_mul(idx, i.const_int(0x9E3779B97F4A7C15))
        start = b.create_and(b.create_xor(mixed, b.create_lshr(mixed, i.const_int(32))), mask)
        slot_type = self.mod.struct_type((i, p))
        b.create_br(probe)
        b.set_insert_point(probe)
        pos = b.create_phi(i)
        pos.add_incoming(((start, valid),))
        slot = b.create_gep(slot_type, methods, (pos,))
        key = b.create_load(i, b.create_struct_gep(slot_type, slot, 0))
        b.create_cond_br(b.create_icmp(llvm.Predicate.EQ, key, idx), found, next_)
        b.set_insert_point(next_)
        following = b.create_and(b.create_add(pos, i.const_int(1)), mask)
        pos.add_incoming(((following, next_),))
        b.create_cond_br(
            b.create_and(
                b.create_icmp(llvm.Predicate.NE, key, i.null()),
                b.create_icmp(llvm.Predicate.NE, following, start),
            ),
            probe,
            missing,
        )
        b.set_insert_point(found)
        target = b.create_load(p, b.create_struct_gep(slot_type, slot, 1))
        b.create_cond_br(b.create_is_not_null(target), done, missing)
        b.set_insert_point(missing)
        b.create_call(self.mod.intrinsic("llvm.trap"))
        b.create_unreachable()
        b.set_insert_point(done)
        self.block = done
        return target
