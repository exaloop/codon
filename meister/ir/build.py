# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Emit and link the build outputs supported by Meister's LLVM backend."""

import os
import shlex
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from .llvm import Linkage, LLVMError
from .toolchain import RUNTIME_NAME, SUFFIX

EXTENSIONS = {
    "llvm": ".ll", "bc": ".bc", "obj": ".o", "asm": ".s",
    "exe": "", "lib": SUFFIX, "cir": ".cir", "pyext": ".o",
}
CTOR = ".main.ctor"


def output_kind(path):
    for kind, suffixes in (
        ("llvm", (".ll",)), ("bc", (".bc",)), ("obj", (".o", ".obj")),
        ("asm", (".s", ".S", ".asm")), ("lib", (".so", ".dylib", ".dll")),
    ):
        if path.suffix in suffixes or kind == "lib" and path.suffix.lower() == ".dll":
            return kind
    return "exe"


def output_path(source, kind):
    path = Path(source)
    stem = path.stem if path.suffix in (".codon", ".seq", ".py") else path.name
    return path.with_name(stem + EXTENSIONS.get(kind, ""))


def setup_global_ctor(module):
    if module.global_variable("llvm.global_ctors"):
        return
    main = module.function(".main.unclash")
    if main is None:
        main = module.function("main")
        if main is None:
            return
        main.set_name(".main.unclash")
        main.set_linkage(Linkage.PRIVATE)

    i32, ptr = module.int_type(32), module.pointer_type()
    body = module.add_function(CTOR, module.primitive_type("Void").function(()))
    body.set_linkage(Linkage.PRIVATE)
    builder = module.builder
    builder.set_insert_point(body.append_block("entry"))
    builder.create_call(main, (i32.null(), ptr.null()))
    builder.create_ret_void()

    entry = module.const_struct((i32.const_int(65535), body, ptr.null()))
    entry_type = entry.type
    module.add_global(
        "llvm.global_ctors", entry_type.array(1), entry_type.const_array((entry,)),
        constant=True, linkage=Linkage.APPENDING,
    )


def write(
    module, path, kind, options, *, libraries=(), linker_flags="", pie=False,
    ctor="auto", relocation=None, plugins=None,
):
    use_ctor = (ctor == "yes" and kind != "bc") or (ctor == "auto" and kind == "lib")
    if use_ctor:
        setup_global_ctor(module)
    module.optimize(
        debug=options.debug, ctor=CTOR, fast_math=options.fast_math, native=options.native,
        auto_free=options.auto_free,
    )
    if kind in {"exe", "lib"}:
        link(module, path, shared=kind == "lib", libraries=libraries, flags=linker_flags,
             pie=pie, debug=options.debug, relocation=relocation, plugins=plugins)
    else:
        module.write(path, "obj" if kind == "pyext" else kind, pic=relocation == "pic")


def link(module, path, *, shared, libraries, flags, pie, debug, relocation, plugins=None):
    flags = ["-pie" if flag == "--pie" else flag for flag in shlex.split(flags)]
    if pie:
        flags.append("-pie")
    pie_flags = [flag for flag in flags if flag in {"-pie", "-no-pie"}]
    pie = bool(pie_flags and pie_flags[-1] == "-pie")
    if shared and pie:
        raise LLVMError("PIE is only supported for executables")

    runtime = module.lib.runtime_path
    runtime_dir = runtime.parent
    runtime_arg = "-lcodonrt" if runtime.name == RUNTIME_NAME else str(runtime)
    windows = sys.platform == "win32"
    driver = shlex.split(os.environ.get("CXX", "clang" if windows else "g++"))
    if windows:
        driver += [
            "-fuse-ld=lld", "-fms-runtime-lib=dll", "-Wl,/nodefaultlib:libcmt",
            "-lmsvcrt", "-lvcruntime", "-lucrt",
        ]
    with TemporaryDirectory(prefix="meister-link-") as temporary:
        obj = Path(temporary) / "program.o"
        output = path.absolute()
        module.write(obj, "obj", pic=shared or pie or relocation == "pic")
        command = [*driver, str(obj)]
        if shared:
            command.append("-shared")
        command.append(f"-L{runtime_dir}")
        if not windows:
            command.append(f"-Wl,-rpath,{runtime_dir}")
        if plugins:
            plugins.add_link_paths(command)
        command.extend(f"-l{name}" for name in libraries)
        if plugins:
            plugins.add_link_libraries(command)
        if windows:
            command.extend(("-lcodonrt", "-llibomp", "-llibopenblas"))
            if debug and not shared:
                command += ["-g", "-Wl,/debug", f"-Wl,/pdb:{output.with_suffix('.pdb')}"]
        else:
            command.extend((runtime_arg, "-lomp", "-lpthread", "-ldl", "-lz", "-lm", "-lc"))
            if not shared:
                command.append("-pie" if pie else "-no-pie")
        command.extend(("-o", str(output)))
        subprocess.run([*command, *flags], check=True)
    if debug and sys.platform == "darwin":
        subprocess.run(["dsymutil", str(output)], check=True)
