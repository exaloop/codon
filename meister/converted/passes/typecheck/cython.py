# TODO: review / fix

# Generate Python bindings for Cython-like access.
def populate_python_module(self: Cache):
    from .visitors.translate.translate import TranslateVisitor
    from .visitors.typecheck.typecheck import TypecheckVisitor

    cython_iter = "_PyWrap.IterWrap"
    if not self.python_ext:
        return
    if self.py_module is None:
        self.py_module = PyModule()
    visitor = TypecheckVisitor(self.type_ctx)

    # needs copy as below fns can mutate this
    classes = self.classes.copy()
    for class_name in classes:
        python_type = visitor.cythonize_class(class_name)
        if python_type.name:
            self.py_module.types.append(python_type)

    # Handle __iternext__ wrappers
    for class_name in self.classes[cython_iter].realizations:
        python_type = visitor.cythonize_iterator(class_name)
        self.py_module.types.append(python_type)

    # needs copy as below fns can mutate this
    functions = self.functions.copy()
    for function_name in functions:
        python_function = visitor.cythonize_function(function_name)
        if python_function.name:
            self.py_module.functions.append(python_function)

    # Handle pending realizations!
    # copy it as it might be modified
    pending = self.pending_realizations.copy()
    for key in pending:
        TranslateVisitor(self.codegen_ctx).translate_stmts(self.functions[key[0]].ast)


def cythonize_class(tc: TypeVisitor, name: str) -> object:
    """*** Cython-like code generation ****"""
    cython_module = "std.internal.python"
    cython_wrap = "_PyWrap"
    cache_class = self.get_class(name)
    imported = self.get_import_module(cache_class.module)
    module = self.ctx.cache.module
    # 1. Replace to_py / from_py with _PyWrap.wrap_to_py/from_py
    if (
        imported.name
        or "__to_py__" not in cache_class.methods
        or "__from_py__" not in cache_class.methods
    ):
        return module.make_py_type("", "")
    python_type = module.make_py_type(self.get_unmangled_name(name), cache_class.ast.get_docstr())
    type_item = self.ctx.force_find(name)
    class_type = self.extract_type(type_item.get_type())
    if not class_type.can_realize():
        raise TypeError(f"cannot realize '{self.get_unmangled_name(name)}' for Python export")
    realized_type = self.realize(class_type)
    assert realized_type is not None, f"cannot realize '{name}'"
    for method_name, wrapper_name in (
        ("__to_py__", "wrap_to_py"),
        ("__from_py__", "wrap_from_py"),
    ):
        overload_root = cache_class.methods[method_name]
        # default first overload!
        # default first overload!
        function_name = self.get_overloads(overload_root)[0]
        function = self.get_function(function_name)
        wrapper_args = [ast.IdExpr(function.ast.items[0].name)]
        if method_name == "__from_py__":
            wrapper_args.append(ast.IdExpr(name))
        function.ast.suite = ast.SuiteStmt(
            ast.ReturnStmt(
                expr=ast.CallExpr(
                    ast.IdExpr(ast.types.mangle(cython_module, cython_wrap, wrapper_name)),
                    items=wrapper_args,
                )
            )
        )
    for method_name in ("__from_py__", "__to_py__"):
        function_name = self.get_overloads(cache_class.methods[method_name])[0]
        function = self.get_function(function_name)
        old_ir: object | None = None
        if function.realizations:
            old_ir = next(iter(function.realizations.values())).ir
        function.realizations.clear()
        transformed_type = self.realize(function.type)
        assert transformed_type is not None, f"cannot re-realize '{function_name}'"
        new_ir = next(iter(function.realizations.values())).ir
        if old_ir is not None:
            args = [module.new_var_value(arg) for arg in old_ir.args()]
            call = module.make_call(new_ir, args)
            old_ir.set_body(module.make_series([call]))
    type_hook = self.get_function(ast.types.mangle(cython_module, cython_wrap, "py_type"))
    for realization in type_hook.realizations.values():
        generic = self.extract_func_generic(realization.type)
        if generic.unify(realized_type, None) >= 0:
            python_type.type_ptr_hook = realization.ir
            break

    # 2. Handle methods
    methods = dict(cache_class.methods)
    special_attributes = {
        "__repr__": "repr",
        "__add__": "add",
        "__iadd__": "iadd",
        "__sub__": "sub",
        "__isub__": "isub",
        "__mul__": "mul",
        "__imul__": "imul",
        "__mod__": "mod",
        "__imod__": "imod",
        "__divmod__": "divmod",
        "__pow__": "pow",
        "__ipow__": "ipow",
        "__neg__": "neg",
        "__pos__": "pos",
        "__abs__": "abs",
        "__bool__": "bool_",
        "__invert__": "invert",
        "__lshift__": "lshift",
        "__ilshift__": "ilshift",
        "__rshift__": "rshift",
        "__irshift__": "irshift",
        "__and__": "and_",
        "__iand__": "iand",
        "__xor__": "xor_",
        "__ixor__": "ixor",
        "__or__": "or_",
        "__ior__": "ior",
        "__int__": "int_",
        "__float__": "float_",
        "__floordiv__": "floordiv",
        "__ifloordiv__": "ifloordiv",
        "__truediv__": "truediv",
        "__itruediv__": "itruediv",
        "__idx__": "index",
        "__matmul__": "matmul",
        "__imatmul__": "imatmul",
        "__len__": "len",
        "__getitem__": "getitem",
        "__setitem__": "setitem",
        "__contains__": "contains",
        "__hash__": "hash",
        "__call__": "call",
        "__str__": "str",
        "__iter__": "iter",
        "__del__": "del_",
    }
    for method_name, overload_root in methods.items():
        overloads = self.get_overloads(overload_root)
        canonical_name = overloads[-1]
        function = self.get_function(canonical_name)
        if len(overloads) == 1 and function.ast.has(ast.Attr.AutoGenerated):
            continue
        is_method = function.ast.has(ast.Attr.Method)
        is_property = function.ast.has(ast.Attr.Property)
        call_name = ast.types.mangle(cython_module, cython_wrap, "wrap_multiple")
        is_magic = False
        if method_name.startswith("__") and method_name.endswith("__"):
            # always use FASTCALL for now; works even for 0- or 1- arg methods
            magic_name = method_name[2:-2]
            if magic_name == "new" and cache_class.ast.has(ast.Attr.Tuple):
                magic_name = "init"
            wrapper_class = self.get_class(ast.types.mangle(cython_module, cython_wrap))
            if f"wrap_magic_{magic_name}" in wrapper_class.methods:
                call_name = ast.types.mangle(cython_module, cython_wrap, f"wrap_magic_{magic_name}")
                is_magic = True
        if is_property:
            call_name = ast.types.mangle(cython_module, cython_wrap, "wrap_get")
        generics = [realized_type]
        if is_property:
            generics.append(self.instantiate_static(self.get_unmangled_name(canonical_name)))
        elif not is_magic:
            generics.append(self.instantiate_static(method_name))
            generics.append(self.instantiate_static(int(is_method)))
        call_function = self.get_function(call_name)
        ir_function = self.realize_ir_func(call_function.type, generics)
        if ir_function is None:
            continue
        if is_property:
            python_type.getset.append(
                module.make_py_get_set(
                    self.get_unmangled_name(canonical_name), "", ir_function, None
                )
            )
        elif method_name in special_attributes:
            setattr(python_type, special_attributes[method_name], ir_function)
        elif method_name == "__init__" or (
            cache_class.ast.has(ast.Attr.Tuple) and method_name == "__new__"
        ):
            python_type.init = ir_function
        else:
            method_kind = module.PyMethod if is_method else module.PyClass
            python_function = module.make_py_function(
                method_name, function.ast.get_docstr(), ir_function, method_kind, 2
            )
            python_function.keywords = True
            python_type.methods.append(python_function)
    comparisons = {"__lt__", "__le__", "__eq__", "__ne__", "__gt__", "__ge__"}
    if any(method.name in comparisons for method in python_type.methods):
        compare_item = self.ctx.force_find(ast.types.mangle(cython_module, cython_wrap, "wrap_cmp"))
        compare_function = compare_item.type.get_func()
        python_type.cmp = self.realize_ir_func(compare_function, [realized_type])
    if len(cache_class.realizations) != 1:
        raise TypeError(f"cannot pythonize generic class '{name}'")
    class_realization = next(iter(cache_class.realizations.values()))
    python_type.type = class_realization.ir
    assert not class_realization.type.is_type(ast.types.Stdlib.Tuple), "tuples not yet done"
    for member_name, _ in class_realization.fields:
        # TODO: handle PyMember for tuples
        # Generate getters & setters
        generics = [realized_type, self.instantiate_static(member_name)]
        getter_item = self.get_function(ast.types.mangle(cython_module, cython_wrap, "wrap_get"))
        getter = self.realize_ir_func(getter_item.type, generics)
        setter: object | None = None
        if not cache_class.ast.has(ast.Attr.Tuple):
            setter_item = self.get_function(
                ast.types.mangle(cython_module, cython_wrap, "wrap_set")
            )
            setter = self.realize_ir_func(setter_item.type, generics)
        python_type.getset.append(module.make_py_get_set(member_name, "", getter, setter))
    return python_type


def cythonize_iterator(tc: TypeVisitor, name: str) -> object:
    cython_module = "std.internal.python"
    cython_wrap = "_PyWrap"
    cython_iterator = "_PyWrap.IterWrap"
    module = self.ctx.cache.module
    python_type = module.make_py_type(name, "")
    iterator_class = self.ctx.cache.classes[cython_iterator]
    class_realization = iterator_class.realizations[name]
    iterator_type = class_realization.type
    type_hook = self.get_function(ast.types.mangle(cython_module, cython_wrap, "py_type"))
    for realization in type_hook.realizations.values():
        if self.extract_func_generic(realization.type).unify(iterator_type, None) >= 0:
            python_type.type_ptr_hook = realization.ir
            break
    for method_name in ("_iter", "_iternext"):
        overload = self.get_overloads(iterator_class.methods[method_name])[0]
        function = self.get_function(overload)
        if method_name == "_iter":
            instantiated = self.instantiate(function.type, iterator_type)
        else:
            underlying = self.extract_class_generic(iterator_type).get_class()
            if underlying is None:
                continue
            iter_methods = self.find_method(underlying, "__iter__", False)
            matches = self.matching_methods(
                underlying,
                iter_methods,
                [self._typed(ast.NoneExpr(), underlying)],
            )
            if not matches:
                continue
            found_type = self.instantiate(matches[0], underlying)
            realized_found = self.realize(found_type)
            if realized_found is None:
                continue
            instantiated = self.instantiate(function.type, iterator_type)
            instantiated_function = instantiated.get_func()
            self.unify(
                self.extract_func_generic(instantiated_function),
                realized_found.get_func().get_ret_type(),
            )
        realized = self.realize(instantiated)
        if realized is not None:
            realized_function = realized.get_func()
            cache_function = self.get_function(realized_function.get_func_name())
            ir_function = cache_function.realizations[realized.realized_name()].ir
            setattr(
                python_type,
                "iter" if method_name == "_iter" else "iternext",
                ir_function,
            )
    python_type.type = class_realization.ir
    return python_type


def cythonize_function(tc: TypeVisitor, name: str) -> object:
    cython_module = "std.internal.python"
    cython_wrap = "_PyWrap"
    module = self.ctx.cache.module
    function = self.get_function(name)
    if function.is_toplevel:
        wrapper_name = ast.types.mangle(cython_module, cython_wrap, "wrap_multiple")
        generics = [
            self.get_stdlib_type(ast.types.Stdlib.NoneType),
            self.instantiate_static(function.ast.name),
            self.instantiate_static(0),
        ]
        wrapper = self.get_function(wrapper_name)
        ir_function = self.realize_ir_func(wrapper.type, generics)
        if ir_function is not None:
            python_function = module.make_py_function(
                self.get_unmangled_name(name),
                function.ast.get_docstr(),
                ir_function,
                module.PyToplevel,
                len(function.ast.items),
            )
            python_function.keywords = True
            return python_function
    return module.make_py_function("", "")


def realize_ir_func(
    self: TypeVisitor,
    function: ast.types.Function,
    generics: List[ast.types.Type] | None = None,
) -> object | None:
    if not generics:
        generics = []
    # TODO: used by cytonization. Probably needs refactoring.
    instantiated = utils.instantiate(self.ctx, function)
    instantiated_function = instantiated.get_func()
    undo = ast.types.Type.UnifyContext()
    for idx, generic in enumerate(generics):
        instantiated_function.func_generics[idx].type.unify(generic, undo)
    if not realize(instantiated):
        return None
    # copy it as it might be modified
    pending = set(self.ctx.cache.pending_realizations)
    from ..translate.translate import TranslateVisitor

    for key, _ in pending:
        pending_function = utils.get_function(self.ctx, key)
        cloned = pending_function.ast.clone()
        TranslateVisitor(self.ctx.cache.codegen_ctx).translate_stmts(cloned)
    source_function = utils.get_function(self.ctx, function.ast.name)
    realized = source_function.realizations[instantiated.realized_name()]
    return realized.ir
