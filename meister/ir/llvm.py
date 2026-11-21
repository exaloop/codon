# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""The LLVM-C subset used by Meister, backed by a live Exaloop LLVM module."""

from ctypes import (
    CDLL,
    CFUNCTYPE,
    POINTER,
    RTLD_GLOBAL,
    PyDLL,
    byref,
    c_char_p,
    c_double,
    c_int,
    c_int64,
    c_size_t,
    c_uint,
    c_uint64,
    c_void_p,
    string_at,
)
from enum import IntEnum
from os import fsencode


class LLVMError(RuntimeError):
    pass


class Linkage(IntEnum):
    EXTERNAL = 0
    APPENDING = 7
    INTERNAL = 8
    PRIVATE = 9


class Predicate(IntEnum):
    EQ = 32
    NE = 33
    UGT = 34
    UGE = 35
    ULT = 36
    ULE = 37
    SGT = 38
    SGE = 39
    SLT = 40
    SLE = 41


# Ref types in llvm-c/Types.h are opaque pointers. Keep the C signatures in
# one place and convert strings/arrays at the Python API boundary.
P, A, S, U, Q, I, N, D = (
    c_void_p,
    POINTER(c_void_p),
    c_char_p,
    c_uint,
    c_uint64,
    c_int,
    c_size_t,
    c_double,
)


class Library:
    def __init__(self, path):
        self.path = path
        # ORC resolves runtime symbols through the current process.
        self._dll = CDLL(str(path), mode=RTLD_GLOBAL)
        self._functions = {}

    def function(self, name, result, *args):
        return self._bind("LLVM" + name, result, args)

    def codon_function(self, name, result, *args, keep_gil=False):
        return self._bind("codon_llvm_" + name, result, args, keep_gil=keep_gil)

    def exception_offset(self):
        return self._bind("seq_exc_offset", c_int64, ())()

    def _bind(self, full_name, result, args, *, keep_gil=False):
        key = full_name, keep_gil
        if key not in self._functions:
            dll = PyDLL(str(self.path), handle=self._dll._handle) if keep_gil else self._dll
            try:
                fn = getattr(dll, full_name)
            except AttributeError as error:
                raise LLVMError(f"{self.path} does not export {full_name}") from error
            fn.restype = result
            fn.argtypes = args
            self._functions[key] = fn
        return self._functions[key]

    def message(self, pointer):
        if not pointer:
            return ""
        try:
            return string_at(pointer).decode(errors="replace")
        finally:
            self.function("DisposeMessage", None, P)(pointer)


def _array(items):
    items = tuple(items)
    return (P * len(items))(*(item._as_parameter_ if item is not None else None for item in items))


class _Handle:
    def __init__(self, owner, pointer):
        self._owner = owner
        self._pointer = pointer

    @property
    def _as_parameter_(self):
        if not self._pointer or not self._owner._pointer:
            raise LLVMError("LLVM handle is no longer live")
        return self._pointer

    def _call(self, name, result, args, *values):
        return self._owner.lib.function(name, result, P, *args)(self, *values)

    def __eq__(self, other):
        if not isinstance(other, _Handle):
            return NotImplemented
        return self._as_parameter_ == other._as_parameter_


class Type(_Handle):
    @property
    def return_type(self):
        return Type(self._owner, self._call("GetReturnType", P, ()))

    def __str__(self):
        return self._owner.lib.message(self._call("PrintTypeToString", P, ()))

    @property
    def is_void(self):
        return self._call("GetTypeKind", I, ()) == 0

    @property
    def is_pointer(self):
        return self._call("GetTypeKind", I, ()) == 12

    def function(self, args, variadic=False):
        args = _array(args)
        return Type(
            self._owner, self._call("FunctionType", P, (A, U, I), args, len(args), variadic)
        )

    def array(self, count):
        return Type(self._owner, self._call("ArrayType2", P, (Q,), count))

    def vector(self, count):
        return Type(self._owner, self._call("VectorType", P, (U,), count))

    def const_int(self, value):
        return Value(self._owner, self._call("ConstInt", P, (Q, I), value, value < 0))

    def const_real(self, value):
        return Value(self._owner, self._call("ConstReal", P, (D,), value))

    def const_array(self, values):
        values = _array(values)
        return Value(self._owner, self._call("ConstArray2", P, (A, Q), values, len(values)))

    def null(self):
        return Value(self._owner, self._call("ConstNull", P, ()))

    def undef(self):
        return Value(self._owner, self._call("GetUndef", P, ()))


class Value(_Handle):
    @property
    def type(self):
        return Type(self._owner, self._call("TypeOf", P, ()))

    @property
    def value_type(self):
        return Type(self._owner, self._call("GlobalGetValueType", P, ()))

    def set_name(self, name):
        name = name.encode()
        self._call("SetValueName2", None, (S, N), name, len(name))

    @property
    def name(self):
        length = N()
        pointer = self._call("GetValueName2", P, (POINTER(N),), byref(length))
        return string_at(pointer, length.value).decode()

    @property
    def is_call(self):
        return bool(self._call("IsACallInst", P, ()))

    @property
    def has_uses(self):
        return bool(self._call("GetFirstUse", P, ()))

    @property
    def called_function(self):
        callee = self._call("GetCalledValue", P, ())
        if self._owner.lib.function("IsAFunction", P, P)(callee):
            return Function(self._owner, callee)
        return None

    @property
    def call_arg_count(self):
        return self._call("GetNumArgOperands", U, ())

    def operand(self, idx):
        return Value(self._owner, self._call("GetOperand", P, (U,), idx))

    def signed_int(self):
        if self._call("IsAConstantInt", P, ()):
            return self._call("ConstIntGetSExtValue", c_int64, ())
        return None

    def erase_instruction(self):
        self._call("InstructionEraseFromParent", None, ())
        self._pointer = None

    def set_alignment(self, alignment):
        self._call("SetAlignment", None, (U,), alignment)

    def set_initializer(self, value):
        self._call("SetInitializer", None, (P,), value)

    def set_linkage(self, linkage):
        self._call("SetLinkage", None, (I,), linkage)

    def set_unnamed_addr(self, *, local=False):
        self._call("SetUnnamedAddress", None, (I,), 1 if local else 2)

    def set_thread_local(self):
        self._call("SetThreadLocal", None, (I,), True)

    def add_incoming(self, pairs):
        values, blocks = zip(*pairs, strict=True)
        self._call("AddIncoming", None, (A, A, U), _array(values), _array(blocks), len(values))

    def add_case(self, value, block):
        self._call("AddCase", None, (P, P), value, block)

    def add_clause(self, clause):
        self._call("AddClause", None, (P,), clause)

    def set_cleanup(self):
        self._call("SetCleanup", None, (I,), True)

    @property
    def debug_location(self):
        pointer = self._call("InstructionGetDebugLoc", P, ())
        return Metadata(self._owner, pointer) if pointer else None

    def set_debug_location(self, loc):
        self._call("InstructionSetDebugLoc", None, (P,), loc)

    def set_metadata(self, name, metadata):
        idx = self._owner.metadata_kind(name)
        value = self._owner.lib.function("MetadataAsValue", P, P, P)(self._owner.context, metadata)
        self._call("SetMetadata", None, (U, P), idx, value)

    def set_global_metadata(self, name, metadata):
        self._call("GlobalSetMetadata", None, (U, P), self._owner.metadata_kind(name), metadata)


class Function(Value):
    @property
    def args(self):
        count = self._call("CountParams", U, ())
        args = (P * count)()
        self._call("GetParams", None, (A,), args)
        return [Value(self._owner, arg) for arg in args]

    @property
    def entry_block(self):
        return BasicBlock(self._owner, self._call("GetEntryBasicBlock", P, ()))

    def append_block(self, name):
        pointer = self._owner.lib.function("AppendBasicBlockInContext", P, P, P, S)(
            self._owner.context, self, name.encode()
        )
        return BasicBlock(self._owner, pointer)

    def set_personality(self, fn):
        self._call("SetPersonalityFn", None, (P,), fn)

    @property
    def subprogram(self):
        pointer = self._call("GetSubprogram", P, ())
        return Metadata(self._owner, pointer) if pointer else None

    def set_subprogram(self, subprogram):
        self._call("SetSubprogram", None, (P,), subprogram)

    def blocks(self):
        pointer = self._call("GetFirstBasicBlock", P, ())
        while pointer:
            block = BasicBlock(self._owner, pointer)
            yield block
            pointer = block._call("GetNextBasicBlock", P, ())

    def add_attribute(self, name, value=0, index=-1):
        name = name.encode()
        lib, ctx = self._owner.lib, self._owner.context
        if isinstance(value, str):
            value = value.encode()
            attr = lib.function("CreateStringAttribute", P, P, S, U, S, U)(
                ctx, name, len(name), value, len(value)
            )
        else:
            kind = lib.function("GetEnumAttributeKindForName", U, S, N)(name, len(name))
            if not kind:
                raise LLVMError(f"unknown LLVM attribute: {name.decode()}")
            attr = lib.function("CreateEnumAttribute", P, P, U, Q)(ctx, kind, value)
        self._call("AddAttributeAtIndex", None, (U, P), index, attr)


class BasicBlock(_Handle):
    @property
    def terminator(self):
        pointer = self._call("GetBasicBlockTerminator", P, ())
        return Value(self._owner, pointer) if pointer else None

    def instructions(self):
        pointer = self._call("GetFirstInstruction", P, ())
        while pointer:
            inst = Value(self._owner, pointer)
            yield inst
            pointer = inst._call("GetNextInstruction", P, ())


class Context:
    def __init__(self, library):
        self.lib = library
        self._pointer = library.function("ContextCreate", P)()

    @property
    def _as_parameter_(self):
        if not self._pointer:
            raise LLVMError("LLVM context is closed")
        return self._pointer

    def close(self):
        if self._pointer:
            self.lib.function("ContextDispose", None, P)(self)
            self._pointer = None


class Module:
    """Own the context, module and builder together; all other handles borrow them."""

    def __init__(self, library, name, source):
        self.lib = library
        self.context = Context(library)
        self._pointer = None
        self.builder = None
        self.debug_builder = None
        try:
            self._pointer = library.function("ModuleCreateWithNameInContext", P, S, P)(
                name.encode(), self.context
            )
            source = source.encode()
            library.function("SetSourceFileName", None, P, S, N)(self, source, len(source))
            err = library.message(library.codon_function("set_host_target", P, P)(self))
            if err:
                raise LLVMError(err)
            self.builder = IRBuilder(self)
            self.data_layout = DataLayout(self, library.function("GetModuleDataLayout", P, P)(self))
        except BaseException:
            self.close()
            raise

    @property
    def _as_parameter_(self):
        if not self._pointer:
            raise LLVMError("LLVM module is closed")
        return self._pointer

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    def close(self):
        if self._pointer:
            if self.debug_builder:
                self.debug_builder.close()
            if self.builder:
                self.lib.function("DisposeBuilder", None, P)(self.builder)
            self.lib.function("DisposeModule", None, P)(self)
            self._pointer = None
        self.context.close()

    def run(self, args, *, debug, on_stage=None):
        """Run through Codon's optimizer and ORC, transferring LLVM ownership."""
        argv = (S * len(args))(*map(fsencode, args))
        # Codon can import Python through the runtime. If it opens the host
        # interpreter, Py_Initialize is a no-op and the current thread must
        # still hold the GIL when generated code calls the Python C API.
        callback = None
        if on_stage is None:
            run = self.lib.codon_function("run", P, P, P, I, I, POINTER(S), keep_gil=True)
        else:
            callback_type = CFUNCTYPE(None, I)
            callback = callback_type(on_stage)
            run = self.lib.codon_function(
                "run_timed", P, P, P, I, I, POINTER(S), callback_type, keep_gil=True
            )
        mod, ctx = self._as_parameter_, self.context._as_parameter_
        if self.debug_builder:
            self.debug_builder.finalize()
            self.debug_builder.close()
        self.lib.function("DisposeBuilder", None, P)(self.builder)
        self._pointer = self.context._pointer = None
        arguments = (ctx, mod, debug, len(argv), argv)
        error = self.lib.message(run(*arguments, callback) if callback else run(*arguments))
        if error:
            raise LLVMError(error)

    def int_type(self, width):
        return Type(self, self.lib.function("IntTypeInContext", P, P, U)(self.context, width))

    def primitive_type(self, name):
        return Type(self, self.lib.function(name + "TypeInContext", P, P)(self.context))

    def pointer_type(self):
        return Type(self, self.lib.function("PointerTypeInContext", P, P, U)(self.context, 0))

    def struct_type(self, fields):
        fields = _array(fields)
        return Type(
            self,
            self.lib.function("StructTypeInContext", P, P, A, U, I)(
                self.context, fields, len(fields), False
            ),
        )

    def const_struct(self, values):
        values = _array(values)
        return Value(
            self,
            self.lib.function("ConstStructInContext", P, P, A, U, I)(
                self.context, values, len(values), False
            ),
        )

    @property
    def target(self):
        return self.lib.function("GetTarget", S, P)(self).decode()

    def metadata_kind(self, name):
        name = name.encode()
        return self.lib.function("GetMDKindIDInContext", U, P, S, U)(self.context, name, len(name))

    def metadata_node(self, elements=()):
        elements = _array(elements)
        pointer = self.lib.function("MDNodeInContext2", P, P, A, N)(
            self.context, elements, len(elements)
        )
        return Metadata(self, pointer)

    def add_warning_flag(self, name, value):
        metadata = self.lib.function("ValueAsMetadata", P, P)(value)
        name = name.encode()
        # LLVMModuleFlagBehaviorWarning
        self.lib.function("AddModuleFlag", None, P, I, S, N, P)(self, 1, name, len(name), metadata)

    def create_debug_builder(self):
        assert self.debug_builder is None
        self.debug_builder = DebugBuilder(self)
        return self.debug_builder

    def function(self, name):
        pointer = self.lib.function("GetNamedFunction", P, P, S)(self, name.encode())
        return Function(self, pointer) if pointer else None

    def add_function(self, name, typ):
        return Function(
            self, self.lib.function("AddFunction", P, P, S, P)(self, name.encode(), typ)
        )

    def get_or_insert_function(self, name, typ):
        return self.function(name) or self.add_function(name, typ)

    def intrinsic(self, name, types=()):
        name, types = name.encode(), _array(types)
        idx = self.lib.function("LookupIntrinsicID", U, S, N)(name, len(name))
        if not idx:
            raise LLVMError(f"unknown LLVM intrinsic {name.decode()}")
        return Function(
            self,
            self.lib.function("GetIntrinsicDeclaration", P, P, U, A, N)(
                self, idx, types, len(types)
            ),
        )

    def global_variable(self, name, *, allow_local=True):
        pointer = self.lib.function("GetNamedGlobal", P, P, S)(self, name.encode())
        if not pointer:
            return None
        if not allow_local:
            linkage = self.lib.function("GetLinkage", I, P)(pointer)
            if linkage in (Linkage.INTERNAL, Linkage.PRIVATE):
                return None
        return Value(self, pointer)

    def add_global(self, name, typ, initializer=None, *, constant=False, linkage=Linkage.EXTERNAL):
        value = Value(self, self.lib.function("AddGlobal", P, P, P, S)(self, typ, name.encode()))
        value.set_linkage(linkage)
        if initializer:
            value.set_initializer(initializer)
        if constant:
            value._call("SetGlobalConstant", None, (I,), True)
        return value

    def parse_and_link(self, source):
        source = source.encode()
        buffer = self.lib.function("CreateMemoryBufferWithMemoryRangeCopy", P, S, N, S)(
            source, len(source), b"<inline LLVM>"
        )
        module, message = P(), P()
        # On both success and failure LLVMParseIRInContext consumes the buffer.
        if self.lib.function("ParseIRInContext", I, P, P, A, A)(
            self.context, buffer, byref(module), byref(message)
        ):
            raise LLVMError(self.lib.message(message))
        layout = self.lib.function("GetDataLayoutStr", S, P)(self)
        self.lib.function("SetDataLayout", None, P, S)(module, layout)
        if self.lib.function("LinkModules2", I, P, P)(self, module):
            raise LLVMError("cannot link inline LLVM module")

    def verify(self):
        message = P()
        failed = self.lib.function("VerifyModule", I, P, I, A)(self, 2, byref(message))
        error = self.lib.message(message)
        if failed:
            raise LLVMError(error)

    def __str__(self):
        return self.lib.message(self.lib.function("PrintModuleToString", P, P)(self))


class DataLayout(_Handle):
    def alloc_size(self, typ):
        return self._call("ABISizeOfType", Q, (P,), typ)

    def abi_alignment(self, typ):
        return self._call("ABIAlignmentOfType", U, (P,), typ)

    def preferred_alignment(self, typ):
        return self._call("PreferredAlignmentOfType", U, (P,), typ)

    def offset_of(self, typ, idx):
        return self._call("OffsetOfElement", Q, (P, U), typ, idx)

    def pointer_size(self):
        return self._call("PointerSize", U, ())


def _instruction(operation, *types, named=True):
    """Adapt the common LLVMBuild* signatures (optional trailing name)."""

    def create(self, *args, name=""):
        if named:
            return self._emit(operation, (*types, S), *args, name.encode())
        return self._emit(operation, types, *args)

    return create


class IRBuilder(_Handle):
    def __init__(self, module):
        super().__init__(
            module, module.lib.function("CreateBuilderInContext", P, P)(module.context)
        )

    def _emit(self, operation, types, *args):
        return Value(self._owner, self._call("Build" + operation, P, types, *args))

    def set_insert_point(self, block):
        self._call("PositionBuilderAtEnd", None, (P,), block)

    def position_before(self, instr):
        self._call("PositionBuilderBefore", None, (P,), instr)

    def set_debug_location(self, loc):
        self._call("SetCurrentDebugLocation2", None, (P,), loc)

    @property
    def insert_block(self):
        pointer = self._call("GetInsertBlock", P, ())
        return BasicBlock(self._owner, pointer) if pointer else None

    def clear_insert_point(self):
        self._call("ClearInsertionPosition", None, ())

    def create_call(self, fn, args=(), *, typ=None, name=""):
        if typ is None:
            typ = fn.value_type
        args = _array(args)
        return self._emit("Call2", (P, P, A, U, S), typ, fn, args, len(args), name.encode())

    def create_invoke(self, fn, normal, unwind, args=(), *, typ=None, name=""):
        if typ is None:
            typ = fn.value_type
        args = _array(args)
        return self._emit(
            "Invoke2",
            (P, P, A, U, P, P, S),
            typ,
            fn,
            args,
            len(args),
            normal,
            unwind,
            name.encode(),
        )

    def create_gep(self, typ, ptr, indexes, *, inbounds=False, name=""):
        indexes = _array(indexes)
        return self._emit(
            "InBoundsGEP2" if inbounds else "GEP2",
            (P, P, A, U, S),
            typ,
            ptr,
            indexes,
            len(indexes),
            name.encode(),
        )

    def create_landing_pad(self, typ, personality, count):
        return self._emit("LandingPad", (P, P, U, S), typ, personality, count, b"")

    create_alloca = _instruction("Alloca", P)
    create_array_alloca = _instruction("ArrayAlloca", P, P)
    create_load = _instruction("Load2", P, P)
    create_store = _instruction("Store", P, P, named=False)
    create_struct_gep = _instruction("StructGEP2", P, P, U)
    create_insert_value = _instruction("InsertValue", P, P, U)
    create_extract_value = _instruction("ExtractValue", P, U)
    create_br = _instruction("Br", P, named=False)
    create_cond_br = _instruction("CondBr", P, P, P, named=False)
    create_ret = _instruction("Ret", P, named=False)
    create_ret_void = _instruction("RetVoid", named=False)
    create_unreachable = _instruction("Unreachable", named=False)
    create_resume = _instruction("Resume", P, named=False)
    create_switch = _instruction("Switch", P, P, U, named=False)
    create_phi = _instruction("Phi", P)
    create_select = _instruction("Select", P, P, P)
    create_add = _instruction("Add", P, P)
    create_sub = _instruction("Sub", P, P)
    create_mul = _instruction("Mul", P, P)
    create_and = _instruction("And", P, P)
    create_or = _instruction("Or", P, P)
    create_xor = _instruction("Xor", P, P)
    create_lshr = _instruction("LShr", P, P)
    create_trunc = _instruction("Trunc", P, P)
    create_zext = _instruction("ZExt", P, P)
    create_ptr_to_int = _instruction("PtrToInt", P, P)
    create_int_to_ptr = _instruction("IntToPtr", P, P)
    create_icmp = _instruction("ICmp", I, P, P)
    create_is_null = _instruction("IsNull", P)
    create_is_not_null = _instruction("IsNotNull", P)


class Metadata(_Handle):
    pass


class DebugBuilder(_Handle):
    def __init__(self, module):
        super().__init__(module, module.lib.function("CreateDIBuilder", P, P)(module))
        self._finalized = False

    def _make(self, operation, types, *args):
        return Metadata(self._owner, self._call("DIBuilder" + operation, P, types, *args))

    def file(self, filename, directory):
        filename, directory = filename.encode(), directory.encode()
        return self._make(
            "CreateFile", (S, N, S, N), filename, len(filename), directory, len(directory)
        )

    def compile_unit(self, file, producer, optimized):
        producer = producer.encode()
        # LLVMDWARFSourceLanguageC, LLVMDWARFEmissionFull; C++ defaults otherwise.
        return self._make(
            "CreateCompileUnit",
            (I, P, S, N, I, S, N, U, S, N, I, U, I, I, S, N, S, N),
            1,
            file,
            producer,
            len(producer),
            optimized,
            b"",
            0,
            0,
            b"",
            0,
            1,
            0,
            True,
            False,
            b"",
            0,
            b"",
            0,
        )

    def basic_type(self, name, size, encoding):
        name = name.encode()
        return self._make("CreateBasicType", (S, N, Q, U, I), name, len(name), size, encoding, 0)

    def pointer_type(self, base, size):
        return self._make("CreatePointerType", (P, Q, U, U, S, N), base, size, 0, 0, b"", 0)

    def reference_type(self, base):
        return self._make("CreateReferenceType", (U, P), 0x10, base)  # DW_TAG_reference_type

    def struct_type(self, name, file, line, size):
        name = name.encode()
        return self._make(
            "CreateStructType",
            (P, S, N, P, U, Q, U, I, P, A, U, U, P, S, N),
            file,
            name,
            len(name),
            file,
            line,
            size,
            0,
            0,
            None,
            None,
            0,
            0,
            None,
            b"",
            0,
        )

    def member_type(self, scope, name, file, line, size, offset, typ, flags=0):
        name = name.encode()
        return self._make(
            "CreateMemberType",
            (P, S, N, P, U, Q, U, Q, I, P),
            scope,
            name,
            len(name),
            file,
            line,
            size,
            0,
            offset,
            flags,
            typ,
        )

    def replace_members(self, typ, members):
        members = _array(members)
        replace = self._owner.lib.codon_function("replace_debug_members", P, P, P, A, N)
        typ._pointer = replace(self, typ, members, len(members))

    def subroutine_type(self, types):
        types = _array(types)
        return self._make("CreateSubroutineType", (P, A, U, I), None, types, len(types), 0)

    def function(self, file, name, linkage, line, typ, optimized):
        name, linkage = name.encode(), linkage.encode()
        return self._make(
            "CreateFunction",
            (P, S, N, S, N, P, U, P, I, I, U, I, I),
            file,
            name,
            len(name),
            linkage,
            len(linkage),
            file,
            line,
            typ,
            True,
            True,
            0,
            0,
            optimized,
        )

    def location(self, line, col, scope):
        location = self._owner.lib.function("DIBuilderCreateDebugLocation", P, P, U, U, P, P)
        return Metadata(self._owner, location(self._owner.context, line, col, scope, None))

    def expression(self):
        return self._make("CreateExpression", (POINTER(Q), N), None, 0)

    def global_variable(self, scope, name, linkage, file, line, typ, local):
        name, linkage = name.encode(), linkage.encode()
        return self._make(
            "CreateGlobalVariableExpression",
            (P, S, N, S, N, P, U, P, I, P, P, U),
            scope,
            name,
            len(name),
            linkage,
            len(linkage),
            file,
            line,
            typ,
            local,
            None,
            None,
            0,
        )

    def local_variable(self, scope, name, file, line, typ, preserve, arg=0):
        name = name.encode()
        if arg:
            return self._make(
                "CreateParameterVariable",
                (P, S, N, U, P, U, P, I, I),
                scope,
                name,
                len(name),
                arg,
                file,
                line,
                typ,
                preserve,
                0,
            )
        return self._make(
            "CreateAutoVariable",
            (P, S, N, P, U, P, I, I, U),
            scope,
            name,
            len(name),
            file,
            line,
            typ,
            preserve,
            0,
            0,
        )

    def insert_declare(self, storage, var, loc, block):
        self._call(
            "DIBuilderInsertDeclareRecordAtEnd",
            P,
            (P, P, P, P, P),
            storage,
            var,
            self.expression(),
            loc,
            block,
        )

    def finalize(self):
        if not self._finalized:
            self._call("DIBuilderFinalize", None, ())
            self._finalized = True

    def close(self):
        if self._pointer:
            self._call("DisposeDIBuilder", None, ())
            self._pointer = None
