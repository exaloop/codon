# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Lower CPython module definitions and slots to LLVM globals and functions."""

from .. import ir
from ..ast.ir import PyFunction
from .llvm import A, I, N, P, S, U, Linkage, LLVMError, Predicate, Type, Value, _array

# CPython Include/methodobject.h and Include/modsupport.h.
METH_KEYWORDS = 0x0002
METH_NOARGS = 0x0004
METH_O = 0x0008
METH_CLASS = 0x0010
METH_STATIC = 0x0020
METH_COEXIST = 0x0040
METH_FASTCALL = 0x0080
PYTHON_ABI_VERSION = 1013

NUMBER_SLOTS = (
    "add", "sub", "mul", "mod", "divmod", "pow", "neg", "pos", "abs", "bool_", "invert",
    "lshift", "rshift", "and_", "xor_", "or_", "int_", None, "float_",
    "iadd", "isub", "imul", "imod", "ipow", "ilshift", "irshift", "iand", "ixor", "ior",
    "floordiv", "truediv", "ifloordiv", "itruediv", "index", "matmul", "imatmul",
)
SEQUENCE_SLOTS = ("len", None, None, None, None, None, None, "contains", None, None)
MAPPING_SLOTS = (None, "getitem", "setitem")


class PythonExtension:
    # C++: codon/cir/llvm/llvisitor.cpp, writeToPythonExtension.
    def __init__(self, visitor, module_info):
        self.visitor = visitor
        self.info = module_info
        self.module, self.b = visitor.mod, visitor.b
        self.i8, self.i32, self.i64, self.ptr, self.void = (
            visitor.i8, visitor.i32, visitor.i64, visitor.ptr, visitor.void,
        )
        p, i32, i64 = self.ptr, self.i32, self.i64
        self.null, self.zero32, self.zero64 = p.null(), i32.null(), i64.null()
        self.windows = "msvc" in self.module.target
        self.object = self.struct("PyObject", (i64, p))
        self.var_object = self.struct("PyVarObject", (self.object, i64))
        self.method = self.struct("PyMethodDef", (p, p, i32, p))
        self.member = self.struct("PyMemberDef", (p, i32, i64, i32, p))
        self.getset = self.struct("PyGetSetDef", (p, p, p, p, p))
        self.module_base = self.struct("PyMethodDefBase", (self.object, p, i64, p))
        self.module_def = self.struct("PyModuleDef", (self.module_base, p, p, i64, p, p, p, p, p))
        self.number = self.struct("PyNumberMethods", (p,) * len(NUMBER_SLOTS))
        self.sequence = self.struct("PySequenceMethods", (p,) * len(SEQUENCE_SLOTS))
        self.mapping = self.struct("PyMappingMethods", (p,) * len(MAPPING_SLOTS))
        self.type_object = self.struct("PyTypeObject", (
            self.var_object, p, i64, i64, p, i64, p, p, p, p, p, p, p, p, p, p, p, p, p,
            i64, p, p, p, p, i64, p, p, p, p, p, p, p, p, p, i64, p, p, p, p, p, p,
            p, p, p, p, p, i32, p, p, self.i8,
        ))
        self.object_header = self.constant(self.object, (i64.const_int(1), self.null))

    def struct(self, name, fields):
        lib = self.module.lib
        typ = Type(self.module, lib.function("StructCreateNamed", P, P, S)(
            self.module.context, name.encode()
        ))
        typ._call("StructSetBody", None, (A, U, I), _array(fields), len(fields), False)
        return typ

    def constant(self, typ, fields):
        return Value(self.module, typ._call("ConstNamedStruct", P, (A, U),
                                          _array(fields), len(fields)))

    def global_var(self, name, initializer, *, constant=False):
        return self.module.add_global(
            ".pyext_" + name, initializer.type, initializer, constant=constant,
            linkage=Linkage.PRIVATE,
        )

    def terminated_array(self, name, typ, values):
        if not values:
            return self.null
        return self.global_var(name, typ.const_array([*values, typ.null()]))

    def string(self, value):
        if not value:
            return self.null
        data = value.encode()
        initializer = Value(self.module, self.module.lib.function(
            "ConstStringInContext2", P, P, S, N, I
        )(self.module.context, data, len(data), False))
        variable = self.global_var("str", initializer, constant=True)
        variable.set_unnamed_addr()
        return variable

    def extern(self, name, result, *args, nounwind=False):
        fn = self.module.get_or_insert_function(name, result.function(args))
        if nounwind:
            fn.add_attribute("nounwind")
        return fn

    def function(self, function, *, wrap=True):
        if function is None:
            return self.null
        fn = self.module.function(self.visitor.function_name(function))
        if fn is None:
            raise LLVMError(f"Python extension wrapper {function.name} is missing")
        if not wrap:
            return fn
        return self.catch_wrapper(fn)

    def catch_wrapper(self, function):
        name = function.name + ".tc_wrap"
        if wrapper := self.module.function(name):
            return wrapper
        wrapper = self.module.add_function(name, function.value_type)
        wrapper.set_personality(self.visitor.personality())
        entry, normal, unwind = map(wrapper.append_block, ("entry", "normal", "unwind"))
        b = self.b
        b.set_insert_point(entry)
        result = b.create_invoke(function, normal, unwind, wrapper.args)
        b.set_insert_point(normal)
        b.create_ret(result)
        b.set_insert_point(unwind)
        caught = b.create_landing_pad(self.visitor.pad_type(), self.visitor.personality(), 1)
        caught.set_cleanup()
        caught.add_clause(self.visitor.type_idx_var(None))
        exception = b.create_extract_value(caught, 0)
        offset = self.i64.const_int(self.module.lib.exception_offset())
        obj = b.create_load(self.ptr, b.create_gep(self.i8, exception, (offset,)))
        b.create_call(self.extern("seq_set_python_exception", self.void, self.ptr), (obj,))
        ret = wrapper.value_type.return_type
        b.create_ret(self.i32.const_int(-1) if ret == self.i32 else ret.null())
        return wrapper

    def methods(self, functions):
        entries = []
        for function in functions:
            if function.keywords:
                flags = METH_FASTCALL | METH_KEYWORDS
            else:
                flags = (METH_NOARGS, METH_O)[function.nargs] if function.nargs < 2 else METH_FASTCALL
            if function.kind == PyFunction.CLASS:
                flags |= METH_CLASS
            elif function.kind == PyFunction.STATIC:
                flags |= METH_STATIC
            if function.coexist:
                flags |= METH_COEXIST
            entries.append(self.constant(self.method, (
                self.string(function.name), self.function(function.func),
                self.i32.const_int(flags), self.string(function.doc),
            )))
        return self.terminated_array("methods", self.method, entries)

    def members(self, members, typ):
        entries = []
        for member in members:
            indexes = _array((self.zero64, self.i32.const_int(1),
                              *(self.i32.const_int(index) for index in member.indexes)))
            address = Value(self.module, typ._call(
                "ConstGEP2", P, (P, A, U), self.null, indexes, len(indexes)
            ))
            offset = Value(self.module, address._call("ConstPtrToInt", P, (P,), self.i64))
            entries.append(self.constant(self.member, (
                self.string(member.name), self.i32.const_int(member.type), offset,
                self.i32.const_int(member.readonly), self.string(member.doc),
            )))
        return self.terminated_array("members", self.member, entries)

    def properties(self, getset):
        entries = [
            self.constant(self.getset, (
                self.string(entry.name), self.function(entry.get), self.function(entry.set),
                self.string(entry.doc), self.null,
            ))
            for entry in getset
        ]
        return self.terminated_array("getset", self.getset, entries)

    def slots(self, pytype, name, typ, names):
        values = [pytype.slots.get(slot) for slot in names]
        if not any(values):
            return self.null
        return self.global_var(f"{name}_slots.{pytype.name}",
                               self.constant(typ, [self.function(fn) for fn in values]))

    def object_functions(self, pytype, object_type, reference, size):
        alloc = self.extern(pytype.name + ".py_alloc", self.ptr, self.ptr, self.i64)
        b = self.b
        b.set_insert_point(alloc.append_block("entry"))
        memory = b.create_call(self.visitor.alloc_func(False, uncollectable=True), (size,))
        header = b.create_insert_value(self.object_header, alloc.args[0], 1)
        b.create_store(header, memory)
        if reference:
            contents = self.visitor.get_llvm_type(reference.contents)
            length = self.i64.const_int(self.visitor.layout.alloc_size(contents))
            allocation = self.visitor.alloc_func(reference.contents.is_atomic())
            obj = b.create_call(allocation, (length,))
            b.create_store(obj, b.create_struct_gep(object_type, memory, 1))
        b.create_ret(memory)

        free = self.visitor.free_func()
        dealloc = self.extern(pytype.name + ".py_dealloc", self.void, self.ptr)
        destructor = pytype.slots.get("del_")
        b.set_insert_point(dealloc.append_block("entry"))
        if destructor:
            b.create_call(self.function(destructor, wrap=False), dealloc.args)
        b.create_call(free, dealloc.args)
        b.create_ret_void()
        return alloc, dealloc, free

    def make_type(self, pytype, type_type):
        typ = pytype.type
        reference = ir.cast(ir.RefType, typ)
        if reference and reference.polymorphic:
            raise LLVMError("Python extension types cannot be polymorphic")
        object_type = self.module.struct_type((self.object, self.visitor.get_llvm_type(typ)))
        size = self.i64.const_int(self.visitor.layout.alloc_size(object_type))
        alloc, dealloc, free = self.object_functions(pytype, object_type, reference, size)
        py_new = self.extern("PyType_GenericNew", self.ptr, self.ptr, self.ptr, self.ptr)
        header = self.global_header(type_type)
        null, zero64 = self.null, self.zero64

        def slot(name):
            return self.function(pytype.slots.get(name))

        initializer = self.constant(self.type_object, (
            header,                                             # PyObject_VAR_HEAD
            self.string(self.info.name + "." + pytype.name),    # tp_name
            size, zero64, dealloc, zero64, null, null, null,     # tp_basicsize ... tp_as_async
            slot("repr"),                                       # tp_repr
            self.slots(pytype, "number", self.number, NUMBER_SLOTS),
            self.slots(pytype, "sequence", self.sequence, SEQUENCE_SLOTS),
            self.slots(pytype, "mapping", self.mapping, MAPPING_SLOTS),
            slot("hash"), slot("call"), slot("str"),
            null, null, null, zero64, self.string(pytype.doc),  # tp_getattro ... tp_doc
            null, null, slot("cmp"), zero64, slot("iter"), slot("iternext"),
            self.methods(pytype.methods), self.members(pytype.members, object_type),
            self.properties(pytype.getset),
            null, null, null, null, zero64,                     # tp_base ... tp_dictoffset
            slot("init"), alloc, py_new, free,
            null, null, null, null, null, null, null,            # tp_is_gc ... tp_del
            self.zero32, free, null, self.i8.null(),              # tp_version_tag ... tp_watched
        ))
        variable = self.global_var("type." + pytype.name, initializer)
        if pytype.type_ptr_hook:
            hook = self.function(pytype.type_ptr_hook, wrap=False)
            for block in hook.blocks():
                for inst in block.instructions():
                    if inst._call("IsAReturnInst", P, ()):
                        inst._call("SetOperand", None, (U, P), 0, variable)
        return variable

    def global_header(self, type_type):
        header = self.constant(self.object, (
            self.i64.const_int(1), self.null if self.windows else type_type,
        ))
        return self.constant(self.var_object, (header, self.zero64))

    def emit(self):
        b, p, module = self.b, self.ptr, self.module
        base = self.constant(self.module_base, (self.object_header, self.null, self.zero64, self.null))
        module_def = self.global_var("module", self.constant(self.module_def, (
            base, self.string(self.info.name), self.string(self.info.doc), self.i64.const_int(-1),
            self.methods(self.info.functions), self.null, self.null, self.null, self.null,
        )))
        type_type = module.add_global("PyType_Type", p)
        if self.windows:
            type_type._call("SetDLLStorageClass", None, (I,), 1)  # DLL import
        types = [(item, self.make_type(item, type_type)) for item in self.info.types]
        by_type = {item.type: value for item, value in types}

        incref = self.extern("Py_IncRef", self.void, p, nounwind=True)
        decref = self.extern("Py_DecRef", self.void, p, nounwind=True)
        module_create = self.extern("PyModule_Create2", p, p, self.i32, nounwind=True)
        type_ready = self.extern("PyType_Ready", self.i32, p, nounwind=True)
        module_add = self.extern("PyModule_AddObject", self.i32, p, p, p, nounwind=True)
        init = self.extern("PyInit_" + self.info.name, p)
        if self.windows:
            init._call("SetDLLStorageClass", None, (I,), 2)  # DLL export
        b.set_insert_point(init.append_block("entry"))
        if main := module.function("main"):
            main.set_name(".main.unclash")
            b.create_call(main, (self.zero32, self.null))

        for item, value in types:
            if self.windows:
                b.create_store(type_type, b.create_struct_gep(self.object, value, 1))
            if item.base:
                b.create_store(by_type[item.base.type], b.create_struct_gep(self.type_object, value, 30))
        for _, value in types:
            status = b.create_call(type_ready, (value,))
            self.check(init, b.create_icmp(Predicate.SLT, status, self.zero32))

        result = b.create_call(module_create, (module_def, self.i32.const_int(PYTHON_ABI_VERSION)))
        self.check(init, b.create_icmp(Predicate.EQ, result, self.null))
        for item, value in types:
            b.create_call(incref, (value,))
            status = b.create_call(module_add, (result, self.string(item.name), value))
            self.check(init, b.create_icmp(Predicate.SLT, status, self.zero32),
                       cleanup=(decref, (value, result)))
        b.create_ret(result)

    def check(self, init, failed, *, cleanup=None):
        fail, success = map(init.append_block, ("failure", "success"))
        self.b.create_cond_br(failed, fail, success)
        self.b.set_insert_point(fail)
        if cleanup:
            function, objects = cleanup
            for obj in objects:
                self.b.create_call(function, (obj,))
        self.b.create_ret(self.null)
        self.b.set_insert_point(success)
