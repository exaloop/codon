# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Compile successive Codon inputs into the same ORC JIT session."""

from contextlib import contextmanager, nullcontext
from copy import copy
from ctypes import CFUNCTYPE, POINTER, PYFUNCTYPE, string_at
from pathlib import Path
from subprocess import CalledProcessError
import sys

from .. import ast, ir
from ..cache import MAIN_IMPORT, STDLIB_IMPORT
from ..error import ParserError, TypecheckError
from ..parser.pegen import CodonSyntaxError
from ..parser.tokenize import TokenError
from ..passes.translate import TranslateVisitor
from ..passes.typecheck import infer, special, typecheck_program, utils
from . import llvm
from .jit import Engine
from .jit_debug import JIT_ERROR_CALLBACK, format_frame, symbolize_frames, symbolizer_path
from .llvisitor import LLVMVisitor
from .transform import PassManager

JIT_FILENAME = "<jit>"
INVOKER = "codon.jit.invoke_input"
OUTPUT_CALLBACK = CFUNCTYPE(None, llvm.P, llvm.N)


def add_invoker(visitor):
    m, b = visitor.mod, visitor.b
    p, i64 = visitor.ptr, visitor.i64
    invoker = m.add_function(INVOKER, p.function((p,)))
    personality = visitor.personality()
    invoker.set_personality(personality)
    entry, normal, unwind, caught, foreign = map(
        invoker.append_block, ("entry", "normal", "unwind", "caught", "foreign")
    )
    b.set_insert_point(entry)
    b.create_invoke(invoker.args[0], normal, unwind, typ=visitor.void.function(()))
    b.set_insert_point(normal)
    b.create_ret(p.null())
    b.set_insert_point(unwind)
    pad = b.create_landing_pad(visitor.pad_type(), personality, 1)
    pad.set_cleanup()
    pad.add_clause(visitor.type_idx_var(None))
    exc = b.create_extract_value(pad, 0)
    kind = b.create_load(i64, exc)
    # codon/runtime/lib.h: SEQ_EXCEPTION_CLASS
    matches = b.create_icmp(llvm.Predicate.EQ, kind, i64.const_int(0x6F626A0073657100))
    b.create_cond_br(matches, caught, foreign)
    b.set_insert_point(caught)
    b.create_ret(exc)
    b.set_insert_point(foreign)
    b.create_resume(pad)
    b.clear_insert_point()


@contextmanager
def compilation_state(cache):
    # JITState copies the compiler tables and removes CIR nodes created while
    # typechecking a rejected input.
    module = cache.module
    contexts = (cache.type_ctx, cache.imports[MAIN_IMPORT].ctx, cache.imports[STDLIB_IMPORT].ctx)
    context_states = [copy(ctx) for ctx in contexts]

    def fields(obj):
        return {
            key: copy(value) if isinstance(value, (dict, list, set)) else value
            for key, value in vars(obj).items()
        }

    state = fields(cache)
    compiler_data = [
        (data, fields(data)) for data in (*cache.functions.values(), *cache.classes.values())
    ]
    for data in cache.classes.values():
        for realization in data.realizations.values():
            compiler_data.append((realization, fields(realization)))
            if realization.ir and (info := realization.ir.runtime_info):
                compiler_data.append((info, fields(info)))
    state["overloads"] = {key: copy(items) for key, items in cache.overloads.items()}
    state["generated_tuple_names"] = [copy(names) for names in cache.generated_tuple_names]
    intrinsics = module.intrinsics.copy()
    module.push_arena()
    try:
        yield
    except (ParserError, ast.NodeError, CodonSyntaxError, TokenError):
        module.pop_arena()
        module.intrinsics = intrinsics
        vars(cache).update(state)
        for ctx, previous in zip(contexts, context_states, strict=True):
            vars(ctx).update(vars(previous))
        for data, previous in compiler_data:
            vars(data).update(previous)
        raise
    else:
        module.arenas.pop()


def display_expression(node):
    while isinstance(node, ast.SuiteStmt) and node.items:
        last = node.items[-1]
        if isinstance(last, ast.ExprStmt):
            node.items[-1] = ast.ExprStmt(
                ast.CallExpr(
                    ast.IdExpr("_jit_display", info=last.info),
                    [last.expr, ast.StringExpr("", info=last.info)],
                    info=last.info,
                ),
                info=last.info,
            )
            break
        node = last


class Session:
    def __init__(self, engine, cache, defines, definitions):
        self.engine, self.cache = engine, cache
        self.functions, self.globals = set(), set()
        self.type_methods = {}
        self.initialized = False
        options = cache.options
        self.manager = PassManager(options)
        self.version = ".".join(
            defines[f"__codon_version_{part}__"] for part in ("major", "minor", "micro")
        )
        self.get_output = engine.lib.runtime_function("seq_jit_get_output", None, OUTPUT_CALLBACK)
        self.terminate = engine.lib.runtime_function(
            "seq_jit_terminate", None, llvm.P, JIT_ERROR_CALLBACK
        )
        self.symbolizer = symbolizer_path(engine.lib) if options.debug and engine.manager else None

        cache.fs.set_module0(JIT_FILENAME)
        cache.plugins.load_runtime_libraries()
        checked = typecheck_program(
            cache, ast.SuiteStmt(), JIT_FILENAME, defines=definitions, early_defines=defines
        )
        # The first module still needs the canonical main entry point.
        options.jit = False
        try:
            TranslateVisitor.apply(cache, checked)
        finally:
            options.jit = True
        cache.module.info = ast.Node.SrcInfo(JIT_FILENAME)
        self.emit()
        self.invoke = PYFUNCTYPE(llvm.P, llvm.P)(engine.lookup(INVOKER))
        PYFUNCTYPE(llvm.I, llvm.I, POINTER(llvm.S))(engine.lookup("main"))(0, None)
        self.initialized = True

    def emit(self, fn=None):
        cache, engine = self.cache, self.engine
        options = cache.options
        cache.plugins.load_runtime_libraries()
        self.manager.results.clear()
        self.manager.run(cache.module)
        with llvm.Module(engine.lib, "codon", JIT_FILENAME, options=options) as module:
            visitor = LLVMVisitor(module, debug=options.debug, version=self.version, session=self)
            visitor.process(cache.module)
            if not self.initialized:
                add_invoker(visitor)
            module.debug_builder.finalize()
            module.verify()
            name = visitor.function_name(fn) if fn else None
            # Private inline bodies can be recreated in the next module.
            functions = {
                key for key, value in visitor.funcs.items()
                if value._call("GetLinkage", llvm.I, ()) != llvm.Linkage.PRIVATE
            }
            variables = {cache.module.argv_var.id, cache.module.argc_var.id}
            for var in cache.module:
                var = var.get_actual()
                if var.global_ and not isinstance(var, ir.Func):
                    variables.add(var.id)
            module.optimize(
                debug=options.debug, fast_math=options.fast_math, native=options.native,
                auto_free=options.auto_free,
            )
            engine.add_module(
                module, initialize_at=f".codon.typeinfo.initialize.{cache.jit_cell}"
            )
            self.functions.update(functions)
            self.globals.update(variables)
        return name

    def execute(self, source):
        cache = self.cache
        with compilation_state(cache):
            node = cache.parse(file=JIT_FILENAME, code=source)
            display_expression(node)
            ctx = cache.imports[MAIN_IMPORT].ctx
            cache.scope(node, ctx.global_shadows)
            preamble = ast.SuiteStmt()
            with ctx.substitute("preamble", preamble):
                checked = infer.infer_types(ctx, node, True)
                if checked is None:
                    raise TypecheckError(trace=utils.find_typecheck_errors(ctx, node))
                special.prepare_vtables(ctx)
            fn = TranslateVisitor.apply(cache, ast.SuiteStmt(preamble, checked))
        cache.jit_cell += 1
        name = self.emit(fn)
        exception = self.invoke(self.engine.lookup(name))
        output = []

        @OUTPUT_CALLBACK
        def capture(message, size):
            output.append(string_at(message, size))

        if not exception:
            self.get_output(capture)
        else:
            @JIT_ERROR_CALLBACK
            def report(message, size, pcs, count):
                output.append(string_at(message, size))
                output.append(b"\n\033[1mBacktrace:\033[0m\n")
                if not self.symbolizer or not count:
                    return
                addresses = pcs[:count]
                try:
                    frames = symbolize_frames(self.symbolizer, self.engine.manager.objects, addresses)
                except (OSError, ValueError, CalledProcessError):
                    return
                for pc in addresses:
                    if pc in frames:
                        output.append(format_frame(pc, frames[pc]).encode())

            self.terminate(exception, report)
        return b"".join(output).decode("utf-8", errors="surrogateescape")


def run(library, cache, defines, definitions, file):
    if sys.platform == "win32":
        raise llvm.LLVMError("incremental JIT requires the Codon Itanium exception runtime")
    with Engine(library, cache.options.debug) as engine:
        session = Session(engine, cache, defines, definitions)
        print(f">>> Codon JIT v{session.version} <<<", flush=True)

        def execute_cell(lines):
            try:
                print(session.execute("".join(lines)), end="")
            except ParserError as error:
                for group in error.errors:
                    for message in group:
                        print(message, file=sys.stderr)
            except (ast.NodeError, CodonSyntaxError, TokenError) as error:
                print(error, file=sys.stderr)
            print("[done]", flush=True)

        with (nullcontext(sys.stdin) if file == "-" else Path(file).open()) as stream:
            lines = []
            for line in stream:
                if line.rstrip("\n") == "#%%":
                    execute_cell(lines)
                    lines.clear()
                else:
                    lines.append(line)
            if lines:
                execute_cell(lines)
    return 0
