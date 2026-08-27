### Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from pathlib import Path

from ..bridge import Callable, Dict, Enum, List, Set, dataclass
from . import ast

FILE_GENERATED: str = "<generated>"
MODULE_MAIN: str = "__main__"
STDLIB_INTERNAL_MODULE: str = "internal"
MAIN_IMPORT: str = ""
STDLIB_IMPORT: str = ":stdlib:"
FN_DISPATCH_SUFFIX: str = ":dispatch"
FN_SETTER_SUFFIX: str = ":set_"
VAR_CLASS_TOPLEVEL: str = ":toplevel"
VAR_USED_SUFFIX: str = ":used"
MAX_ERRORS: int = 5
MAX_TUPLE: int = 2048
MAX_INT_WIDTH: int = 10000
MAX_REALIZATION_DEPTH: int = 200
MAX_STATIC_ITER: int = 1024


@dataclass(init=False)
class Import:
    """Module (import) data."""

    @dataclass
    class File:
        class Status(Enum):
            StdLibrary = 1
            External = 2

        status: Import.File.Status = Status.StdLibrary
        # Absolute path of an import.
        path: str = ""
        # Module name (e.g. foo.bar.baz).
        module: str = ""


    # Relative module name (e.g., `foo.bar`)
    name: str = ""
    # Absolute filename of an import.
    filename: str = ""
    # Import typechecking context.
    ctx: object | None = None
    # Unique import variable for checking already loaded imports.
    import_var: str = ""
    # File content (line:col indexable)
    content: List[str]
    # Set if loaded at toplevel
    loaded_at_toplevel: bool = True

    def __init__(
        self,
        name: str = "",
        filename: str = "",
        ctx: object | None = None,
        import_var: str = "",
        content: List[str] | None = None,
        loaded_at_toplevel: bool = True,
    ):
        self.name = name
        self.filename = filename
        self.ctx = ctx
        self.import_var = import_var
        self.content = [] if content is None else content
        self.loaded_at_toplevel = loaded_at_toplevel


@dataclass(init=False)
class ClassData:
    """Stores class data for each class (type) in the source code."""

    # A class field (member).
    @dataclass
    class Field:
        # Field name.
        name: str
        # A corresponding generic field type.
        type: ast.types.Type
        # Base class name (if available)
        base_class: str = ""
        type_expr: ast.Expr | None = None

        def get_type(self):
            return self.type

    @dataclass(init=False)
    class Realization:
        # Realized class type.
        type: ast.types.Class
        # A list of field names and realization's realized field types.
        fields: List[tuple[str, ast.types.Type]]
        # IR type pointer.
        ir: object | None = None
        # Bases (in MRO order)
        bases: List[ast.types.Class]
        # Realization vtable (for each base class).
        # Maps {base, function signature} to {thunk realization, thunk ID}.
        # Base can be the realization itself.
        # Order is important so map is used instead of unordered_map.
        vtable: Dict[tuple[str, str], ast.types.Function]
        # Realization ID
        id: int = 0

        def __init__(
            self,
            type: ast.types.Class,
            fields: List[tuple[str, ast.types.Type]] | None = None,
            ir: object | None = None,
            bases: List[ast.types.Class] | None = None,
            vtable: Dict[tuple[str, str], ast.types.Function] | None = None,
            id: int = 0,
        ):
            self.type = type
            self.fields = [] if fields is None else fields
            self.ir = ir
            self.bases = [] if bases is None else bases
            self.vtable = {} if vtable is None else vtable
            self.id = id

    # Module information
    module: str = ""
    # Generic (unrealized) class template AST.
    ast: ast.ClassStmt | None = None
    # Non-simplified AST. Used for base class instantiation.
    original_ast: ast.ClassStmt | None = None
    # Class method lookup table. Each non-canonical name points
    # to a root function name of a corresponding method.
    methods: Dict[str, str]
    # A list of class' ClassData.Field instances. List is needed (instead of map) because
    # the order of the fields matters.
    fields: List[ClassData.Field]
    # Dictionary of class variables: a name maps to a canonical name.
    class_vars: Dict[str, str]
    # Realization lookup table that maps a realized class name to the corresponding
    # ClassRealization instance.
    realizations: Dict[str, ClassData.Realization]
    # Set if a class is polymorphic and has RTTI.
    rtti: bool = False
    # List of virtual method names
    virtuals: Set[str]
    # MRO
    mro: List[ast.types.Class]
    # Classes whose base is this class
    descendants: Set[str]
    jit_cell: int = 0

    def __init__(
        self,
        module: str = "",
        ast: ast.ClassStmt | None = None,
        original_ast: ast.ClassStmt | None = None,
        methods: Dict[str, str] | None = None,
        fields: List[ClassData.Field] | None = None,
        class_vars: Dict[str, str] | None = None,
        realizations: Dict[str, ClassData.Realization] | None = None,
        rtti: bool = False,
        virtuals: Set[str] | None = None,
        mro: List[ast.types.Class] | None = None,
        descendants: Set[str] | None = None,
        jit_cell: int = 0,
    ):
        self.module = module
        self.ast = ast
        self.original_ast = original_ast
        self.methods = {} if methods is None else methods
        self.fields = [] if fields is None else fields
        self.class_vars = {} if class_vars is None else class_vars
        self.realizations = {} if realizations is None else realizations
        self.rtti = rtti
        self.virtuals = set() if virtuals is None else virtuals
        self.mro = [] if mro is None else mro
        self.descendants = set() if descendants is None else descendants
        self.jit_cell = jit_cell


@dataclass(init=False)
class FunctionData:
    @dataclass(init=False)
    class Realization:
        # Realized function type.
        type: ast.types.Function
        # Realized function AST (stored here for later realization in code generations
        # stage).
        ast: ast.FunctionStmt | None = None
        # IR function pointer.
        ir: object | None = None
        # Resolved captures
        captures: List[str]

        def __init__(
            self,
            type: ast.types.Function,
            ast: ast.FunctionStmt | None = None,
            ir: object | None = None,
            captures: List[str] | None = None,
        ):
            self.type = type
            self.ast = ast
            self.ir = ir
            self.captures = [] if captures is None else captures

    # Module information
    module: str = ""
    root_name: str = ""
    # Generic (unrealized) function template AST.
    ast: ast.FunctionStmt | None = None
    # Unrealized function type.
    type: ast.types.Function | None = None
    # Non-simplified AST.
    orig_ast: ast.FunctionStmt | None = None
    is_toplevel: bool = False
    # Realization lookup table that maps a realized function name to the corresponding
    # FunctionRealization instance.
    realizations: Dict[str, FunctionData.Realization]
    captures: Set[str]

    def __init__(
        self,
        module: str = "",
        root_name: str = "",
        ast: ast.FunctionStmt | None = None,
        type: ast.types.Function | None = None,
        orig_ast: ast.FunctionStmt | None = None,
        is_toplevel: bool = False,
        realizations: Dict[str, FunctionData.Realization] | None = None,
        captures: Set[str] | None = None,
    ):
        self.module = module
        self.root_name = root_name
        self.ast = ast
        self.type = type
        self.orig_ast = orig_ast
        self.is_toplevel = is_toplevel
        self.realizations = {} if realizations is None else realizations
        self.captures = set() if captures is None else captures


@dataclass(init=False)
class PyModule:
    types: List[object]
    functions: List[object]

    def __init__(
        self,
        types: List[object] | None = None,
        functions: List[object] | None = None,
    ):
        self.types = [] if types is None else types
        self.functions = [] if functions is None else functions


@dataclass(init=False)
class Cache:
    """
    Cache encapsulation that holds data structures shared across various transformation
    stages (AST transformation, type checking etc.). The subsequent stages (e.g. type
    checking) assumes that previous stages populated this structure correctly.
    Implemented to avoid a bunch of global objects.
    """

    argv0: str = ""
    # Filesystem object used for accessing files.
    fs: Filesystem
    # Stores a count for each identifier (name) seen in the code.
    # Used to generate unique identifier for each name in the code (e.g. Foo -> Foo.2).
    identifier_count: Dict[str, int]
    # Maps a unique identifier back to the original name in the code
    # (e.g. Foo.2 -> Foo).
    reverse_identifier_lookup: Dict[str, str]
    # Number of code-generated source code positions. Used to generate the next unique
    # source-code position information.
    generated_info_count: int = 0
    # Number of unbound variables so far. Used to generate the next unique unbound
    # identifier.
    unbound_count: int = 256
    # Number of auto-generated variables so far. Used to generate the next unique
    # variable name in getTemporaryVar() below.
    var_count: int = 0
    # Scope counter. Each conditional block gets a new scope ID.
    block_count: int = 1
    # Compiler
    compiler: object | None = None
    # IR module.
    module: object | None = None
    # Table of imported files that maps an absolute filename to an Import structure.
    # By convention, the key of the Codon's standard library is ":stdlib:",
    # and the main module is "".
    imports: Dict[str, Import]
    # Set of unique (canonical) global identifiers for marking such variables as global
    # in code-generation step and in JIT.
    globals: Dict[str, object]
    # Class lookup table that maps a canonical class identifier to the corresponding
    # Class instance.
    classes: Dict[str, ClassData]
    class_realization_cnt: int = 0
    thunk_ids: Dict[tuple[str, str], int]
    # Function lookup table that maps a canonical function identifier to the
    # corresponding Function instance.
    functions: Dict[str, FunctionData]
    # Maps a "root" name of each function to the list of names of the function
    # overloads (canonical names).
    overloads: Dict[str, List[str]]
    # Pointer to the later contexts needed for IR API access.
    type_ctx: object | None = None
    codegen_ctx: object | None = None
    # Set of function realizations that are to be translated to IR.
    pending_realizations: Set[tuple[str, str]]
    custom_block_stmts: Dict[str, tuple[bool, Callable[[object, object], object]]]
    custom_expr_stmts: Dict[str, Callable[[object, object], object]]
    # Set if the Codon is running in JIT mode.
    is_jit: bool = False
    jit_cell: int = 0
    generated_tuples: Set[int]
    generated_kw_tuples: Dict[str, int]
    generated_tuple_names: List[List[str]]
    # Set if Codon operates in Python compatibility mode (e.g., with Python numerics)
    python_compat: bool = False
    # Set if Codon operates in Python extension mode
    python_ext: bool = False
    py_module: PyModule | None = None
    _timings: Dict[str, float]

    def __init__(
        self,
        argv0: str,
        fs: Filesystem | None = None,
        identifier_count: Dict[str, int] | None = None,
        reverse_identifier_lookup: Dict[str, str] | None = None,
        generated_info_count: int = 0,
        unbound_count: int = 256,
        var_count: int = 0,
        block_count: int = 1,
        compiler: object | None = None,
        module: object | None = None,
        imports: Dict[str, Import] | None = None,
        globals: Dict[str, object] | None = None,
        classes: Dict[str, ClassData] | None = None,
        class_realization_cnt: int = 0,
        thunk_ids: Dict[tuple[str, str], int] | None = None,
        functions: Dict[str, FunctionData] | None = None,
        overloads: Dict[str, List[str]] | None = None,
        type_ctx: object | None = None,
        codegen_ctx: object | None = None,
        pending_realizations: Set[tuple[str, str]] | None = None,
        custom_block_stmts: Dict[str, tuple[bool, Callable[[object, object], object]]]
        | None = None,
        custom_expr_stmts: Dict[str, Callable[[object, object], object]] | None = None,
        is_jit: bool = False,
        jit_cell: int = 0,
        generated_tuples: Set[int] | None = None,
        generated_kw_tuples: Dict[str, int] | None = None,
        generated_tuple_names: List[List[str]] | None = None,
        python_compat: bool = False,
        python_ext: bool = False,
        py_module: PyModule | None = None,
    ):
        self.argv0 = argv0
        self.fs = fs or Filesystem(argv0=self.argv0)
        self.identifier_count = {} if identifier_count is None else identifier_count
        self.reverse_identifier_lookup = (
            {} if reverse_identifier_lookup is None else reverse_identifier_lookup
        )
        self.generated_info_count = generated_info_count
        self.unbound_count = unbound_count
        self.var_count = var_count
        self.block_count = block_count
        self.compiler = compiler
        self.module = module
        self.imports = {} if imports is None else imports
        self.globals = {} if globals is None else globals
        self.classes = {} if classes is None else classes
        self.class_realization_cnt = class_realization_cnt
        self.thunk_ids = {} if thunk_ids is None else thunk_ids
        self.functions = {} if functions is None else functions
        self.overloads = {} if overloads is None else overloads
        self.type_ctx = type_ctx  # or TypeContext(cache=self, filename=".root")
        self.codegen_ctx = codegen_ctx
        self.pending_realizations = set() if pending_realizations is None else pending_realizations
        self.custom_block_stmts = {} if custom_block_stmts is None else custom_block_stmts
        self.custom_expr_stmts = {} if custom_expr_stmts is None else custom_expr_stmts
        self.is_jit = is_jit
        self.jit_cell = jit_cell
        self.generated_tuples = set() if generated_tuples is None else generated_tuples
        self.generated_kw_tuples = {} if generated_kw_tuples is None else generated_kw_tuples
        self.generated_tuple_names = (
            [[]] if generated_tuple_names is None else generated_tuple_names
        )
        self.python_compat = python_compat
        self.python_ext = python_ext
        self.py_module = py_module
        self._timings = {}

    # Return a uniquely named temporary variable of a format
    # "{sigil}_{prefix}{counter}". A sigil should be a non-lexable symbol.
    def get_temporary_var(self, prefix: str = "", sigil: str = "%") -> str:
        self.var_count += 1
        name = f"{f'{sigil}_' if sigil else ''}{prefix}_{self.var_count}"
        return name

    # Get the non-canonical version of a canonical name.
    def rev(self, value: str) -> str:
        if item := self.reverse_identifier_lookup.get(value):
            return item
        assert False, f"'{value}' has no non-canonical name"

    # Generate a unique SrcInfo for internally generated AST nodes.
    def generate_src_info(self, file="<generated>") -> ast.Node.SrcInfo:
        result = ast.Node.SrcInfo(
            file,
            self.generated_info_count,
            self.generated_info_count,
            0,
        )
        self.generated_info_count += 1
        return result

    # Get file contents at the given location.
    def get_content(self, info: ast.Node.SrcInfo) -> str:
        if not (imported := self.imports.get(info.file, None)):
            return ""
        content = imported.content[
            max(0, info.line - 1) : min(info.end_line, len(imported.content))
        ]
        content[0] = content[0][info.col - 1 :]
        content[-1] = content[-1][: info.end_col]
        return "".join(content)

    def get_class(self, typ: ast.types.Class) -> ClassData:
        return self.classes[typ.name]

    # Find the canonical name of a class method.
    def get_method(self, typ: ast.types.Class, member: str) -> str:
        if cls := self.get_class(typ):
            return cls.methods[member]
        assert False, f"cannot find '{member}' in '{typ.name}'"

    # Realization API.
    # Find a class with a given canonical name and return a matching types::ast.types.Type pointer
    # or a nullptr if a class is not found.
    # Returns an _uninstantiated_ type.
    def find_class(self, name: str) -> ast.types.Class | None:
        if self.type_ctx is None:
            return None
        if (item := self.type_ctx.find(name)) and item.is_type():
            if isinstance(item.type, ast.types.Class) and item.type.generics:
                return item.type[0]
        return None

    # Find a function with a given canonical name and return a matching types::ast.types.Type
    # pointer or a nullptr if a function is not found.
    # Returns an _uninstantiated_ type.
    def find_function(self, name: str) -> ast.types.Function | None:
        if self.type_ctx is None:
            return None
        if self.type_ctx is None:
            return None
        for n in (name, f"{name}:0"):
            if (item := self.type_ctx.find(name)) and item.is_type():
                if isinstance(item.type, ast.types.Function):
                    return item.type
        return None

    # Find the class method in a given class type that best matches the given arguments.
    # Returns an _uninstantiated_ type.
    def find_method(
        self, typ: ast.types.Class, member: str, args: List[ast.types.Type]
    ) -> ast.types.Function | None:
        with self.typecheck() as tc:
            return tc.find_best_method(typ, member, args)

    # Given a class type and the matching generic vector, instantiate the type and
    # realize it.
    def realize_type(
        self, typ: ast.types.Class, generics: List[ast.types.Type] = []
    ) -> object | None:
        with self.typecheck() as tc:
            if realized := tc.realize(tc.instantiate_type(typ, generics)):
                if isinstance(realized, ast.types.Class):
                    return self.classes[realized.name].realizations[realized.realized_name()].ir
        return None

    # Given a function type and function arguments, instantiate the type and
    # realize it. The first argument is the function return type.
    # You can also pass function generics if a function has one (e.g. T in def
    # foo[T](...)). If a generic is used as an argument, it will be auto-deduced. Pass
    # only if a generic cannot be deduced from the provided args.
    def realize_function(
        self,
        typ: ast.types.Function,
        args: List[ast.types.Type],
        generics: List[ast.types.Type] = [],
        parent_class: ast.types.Class | None = None,
    ) -> object | None:
        function = None
        with self.typecheck() as tc:
            function_type = tc.instantiate_type(typ, parent_class)
            if (
                not isinstance(function_type, ast.types.Function)
                or len(args) != len(function_type) + 1
            ):
                return None
            undo = ast.types.Type.UnifyContext()
            return_type = function_type.get_ret_type()
            if return_type.unify(args[0], undo) < 0:
                undo.undo()
                return None
            for generic_index in range(1, len(args)):
                undo = ast.types.Type.UnifyContext()
                if function_type[generic_index - 1].unify(args[generic_index], undo) < 0:
                    undo.undo()
                    return None
            if generics:
                if len(generics) != len(function_type.func_generics):
                    return None
                for generic_index in range(len(generics)):
                    undo = ast.types.Type.UnifyContext()
                    if (
                        function_type.func_generics[generic_index].type.unify(
                            generics[generic_index], undo
                        )
                        < 0
                    ):
                        undo.undo()
                        return None
            if realized := tc.realize(function_type):
                pending = self.pending_realizations.copy()
                for key in pending:
                    template = self.functions[key[0]].ast
                    with self.translate() as ts:
                        ts.translate_stmts(template.clone())
                if isinstance(realized, ast.types.Function) and realized.ast:
                    function = (
                        self.functions[realized.ast.name].realizations[realized.realized_name()].ir
                    )
        return function

    def make_tuple(self, types: List[ast.types.Type]) -> object | None:
        with self.typecheck() as tc:
            tuple_type = tc.instantiate_type(tc.generate_tuple(len(types)), types)
            return self.realize_type(tuple_type, types)

    def make_function(self, types: List[ast.types.Type]) -> object | None:
        with self.typecheck() as tc:
            assert types, "types must have at least one argument"
            return_type = types[0]
            arguments_type = tc.instantiate_type(tc.generate_tuple(len(types) - 1), types[1:])
            return self.realize_type(ast.types.Stdlib.Function, [arguments_type, return_type])

    def make_union(self, types: List[ast.types.Type]) -> object | None:
        with self.typecheck() as tc:
            arguments_type = tc.instantiate_type(tc.generate_tuple(len(types)), types)
            return self.realize_type(tc.get_stdlib_type(ast.types.Stdlib.Union), [arguments_type])

    def get_realization_id(self, typ: ast.types.Class) -> int:
        with self.typecheck() as tc:
            realization = tc.get_class_realization(typ)
            return realization.id

    def get_base_realization_ids(self, typ: ast.types.Class) -> List[int]:
        with self.typecheck() as tc:
            realization = tc.get_class_realization(typ)
            return [self.get_realization_id(base) for base in realization.bases]

    def get_child_realization_ids(self, typ: ast.types.Class) -> List[int]:
        with self.typecheck() as tc:
            class_value = tc.get_class_realization(typ)
        parent_id = class_value.id
        child_ids = []
        for class_data in self.classes.values():
            for realization in class_data.realizations.values():
                for base in realization.bases:
                    if self.get_realization_id(base) == parent_id:
                        child_ids.append(realization.id)
                        break
        return child_ids

    def parse_code(self, code: str) -> ast.Node:
        raise NotImplementedError()

        # try:
        #     # startLine=
        #     node = parse_code(self, "<internal>", code, 0)
        # except ParserException:
        #     raise
        # context = self.imports[MAIN_IMPORT].ctx
        # checked = TypecheckVisitor.apply(context, node)
        # old = list(self.codegen_ctx.series)
        # self.codegen_ctx.series.clear()
        # TranslateVisitor(self.codegen_ctx).initialize_globals()
        # TranslateVisitor(self.codegen_ctx).translate_stmts(checked)
        # current = list(self.codegen_ctx.series)
        # self.codegen_ctx.series = old
        # return current

    def scope(
        self, node: ast.Stmt, globals: Dict[str, int] | None = None, dominate_all: bool = False
    ):
        from .passes import scope

        # Count number of shadowed names to know which names change or not later on
        ctx = scope.ScopeContext(self, scope=[])
        visitor = scope.ScopingVisitor(ctx)
        with visitor.conditional(node, block_id=0):
            visitor.visit(node)
            visitor.process_child_captures()
            if dominate_all:
                for name in ctx.map:
                    visitor.dominate(name)
            if globals:
                for name, values in ctx.map.items():
                    if (count := sum(1 for v in values if not v.ignore)) > 1:
                        globals[name] = count

        return node

    def typecheck(self):
        return TypecheckVisitor(self.type_ctx)

    def translate(self):
        return TranslateVisitor(self.codegen_ctx)

    @staticmethod
    def merge_c3(seqs: List[List[ast.types.Type]]) -> List[ast.types.Class]:
        # Reference: https://www.python.org/download/releases/2.3/mro/
        result = []
        index = 0
        while True:
            found = False
            candidate = None
            for sequence in seqs:
                if not sequence:
                    continue
                found = True
                not_head = False
                for other in seqs:
                    if other:
                        present = False
                        for item_index in range(1, len(other)):
                            if isinstance(other[item_index], ast.types.Class):
                                present = present or sequence[0].is_type(other[item_index].name)
                            if present:
                                break
                        if present:
                            not_head = True
                            break
                if not not_head:
                    candidate = sequence[0]
                    break
            if not found:
                return result
            if candidate is None:
                return []
            result.append(candidate)
            for sequence in seqs:
                if sequence:
                    if isinstance(sequence[0], ast.types.Class) and candidate.is_type(
                        sequence[0].name
                    ):
                        del sequence[0]
            index += 1

    # Generate Python bindings for Cython-like access.
    def populate_python_module(self):
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

    def get_import_file(
        self, what: str, relative_to: str, force_stdlib: bool = False
    ) -> Import.File | None:
        """
        Find an import file what given an executable path (argv0) either in the standard
        library or relative to a file relativeTo. Set forceStdlib for searching only the
        standard library.
        """
        paths: List[Path] = []
        parent_relative_to = Path(relative_to).parent
        if what != "<jit>":
            if not force_stdlib:
                path = (parent_relative_to / what).with_suffix(".codon")
                if self.fs.exists(path):
                    paths.append(self.fs.canonical(path))
                path = parent_relative_to / what / "__init__.codon"
                if self.fs.exists(path):
                    paths.append(self.fs.canonical(path))
                path = (parent_relative_to / what).with_suffix(".py")
                if self.fs.exists(path):
                    paths.append(self.fs.canonical(path))
                path = parent_relative_to / what / "__init__.py"
                if self.fs.exists(path):
                    paths.append(self.fs.canonical(path))

        def check_plugin(path: Path, requested: str):
            # C++ source: codon/parser/common.cpp:385
            plugin = path / requested
            init = plugin / "stdlib" / requested / "__init__.codon"
            if self.fs.exists(plugin / "plugin.toml") and self.fs.exists(init):
                failed = False
                if self.compiler and self.compiler.is_plugin_loaded(plugin):
                    try:
                        self.compiler.load(plugin)
                    except Exception:
                        # TODO-CONV: Needs to print an error message
                        raise NotImplementedError
                        failed = True
                if not failed:
                    paths.append(self.fs.canonical(init))

        if not paths:
            # Load a plugin maybe
            check_plugin(parent_relative_to, what)
        for stdlib_path in self.fs.get_stdlib_paths():
            path = (stdlib_path / what).with_suffix(".codon")
            if self.fs.exists(path):
                paths.append(self.fs.canonical(path))
            path = stdlib_path / what / "__init__.codon"
            if self.fs.exists(path):
                paths.append(self.fs.canonical(path))
            # Load a plugin maybe
            check_plugin(stdlib_path, what)
        if not paths:
            return None
        return self.fs.get_root(paths[0])


@dataclass(init=False)
class Filesystem:
    search_paths: List[Path]
    argv0: str = ""
    module0: Path
    extra_paths: List[Path]

    def __init__(self, argv0: str, module0: str = ""):
        import os

        self.search_paths = []
        self.argv0 = argv0
        self.module0 = Path(module0)
        self.extra_paths = []
        if codon_path := os.getenv("CODON_PATH"):
            self.add_search_path(codon_path)
        if self.argv0:
            root = self.executable_path(self.argv0).parent
            for location in ("../lib/codon/stdlib", "../stdlib", "stdlib"):
                self.add_search_path(str(root / location))
            for location in ("../lib/codon/plugins", "../plugins"):
                self.add_search_path(str(root / location))

    def canonical(self, path: Path) -> Path:
        return path.resolve(strict=False)

    def add_search_path(self, value: str):
        path = Path(value)
        if path.exists:
            self.search_paths.append(self.canonical(path))

    def get_stdlib_paths(self):
        return self.search_paths

    def read_lines(self, path: Path) -> List[str]:
        import sys

        if str(path) == "-":
            return [line for line in sys.stdin]
        else:
            with open(path, "r") as file:
                return [line for line in file]
        return []

    def set_module0(self, value: str):
        self.module0 = self.canonical(Path(value))

    def get_module0(self) -> Path:
        return Path("") if str(self.module0) in ("", ".") else self.canonical(self.module0)

    @staticmethod
    def executable_path(argv0: str) -> Path:
        return Path(argv0).resolve(strict=False)

    def exists(self, path: Path):
        return path.exists()

    def get_root(self, source_path: Path):
        is_stdlib = False
        source = str(source_path)
        root = ""
        for path in self.get_stdlib_paths():
            if source.startswith(str(path)):
                root = str(path)
                is_stdlib = True
                break
        module0 = self.get_module0().parent
        module0_string = "" if str(module0) == "." else str(module0)
        if not is_stdlib and module0_string and source.startswith(module0_string):
            root = module0_string
        extension = ".codon"
        if not ((not root or source.startswith(root)) and source.endswith(extension)):
            extension = ".py"
        assert (not root or source.startswith(root)) and source.endswith(extension), (
            f"bad path substitution: {source}, {root}"
        )
        module = source[len(root) + 1 : len(source) - len(extension)]
        module = module.replace("/", ".")
        return Import.File(
            Import.File.Status.External
            if not is_stdlib and root == module0_string
            else Import.File.Status.StdLibrary,
            source,
            module,
        )


@dataclass(init=False)
class ResourceFilesystem(Filesystem):
    allow_external: bool = True
    resources: Dict[str, str]

    def __init__(self, argv0: str, module0: str = "", allow_external: bool = True):
        super().__init__(argv0, module0)
        self.allow_external = allow_external
        self.resources = {}
        self.search_paths = [Path("/stdlib")]

    def read_lines(self, path: Path) -> List[str]:
        if str(path) not in self.resources and self.allow_external:
            return super().read_lines(path)
        lines: List[str] = []
        if path == "-":
            raise FileNotFoundError("<stdin>")
        try:
            lines = self.resources[str(path)].split("\n")
        except KeyError as error:
            raise FileNotFoundError(path) from error
        return lines

    def exists(self, path: Path):
        if str(path) in self.resources:
            return True
        if self.allow_external:
            return super().exists(path)
        return False
