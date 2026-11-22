# Meister LLVM backend

Meister loads Exaloop LLVM 20 through `ctypes`. The compiler process uses
`libLLVM` for LLVM IR construction, optimization, ORC linking, and native
emission. `libcodonrt` and its OpenMP dependency supply the Codon runtime ABI.
The runtime also exports compiler-rt helpers used by generated code. The build
command uses a system linker driver for executables and shared libraries. Generated native outputs have the same runtime dependencies as
the corresponding Codon outputs on Linux.

## Changes and differences from C++

| Area | Meister implementation and differences |
| --- | --- |
| Loading and configuration | The Python frontend supplies early definitions and stdlib paths. The LLVM and runtime locations can be selected independently. The four C ABI bridge functions formerly used by Meister were removed. `python -m meister test` runs pytest. The standalone AST and CIR comparison tools live under `test/` and load the native compiler only when explicitly invoked. The pytest native reference cases run `codon_test` in subprocesses. |
| Debug metadata | Meister creates an empty composite and replaces its elements through the LLVM C API, matching the construction order in C++ for recursive types. |
| JIT linking | On Linux x86-64, the LLVM C API exposes allocation and GDB registration through RTDyld. C++ uses an ObjectLinkingLayer with JITLink and its own plugins. Other hosts retain the default LLJIT layer. Meister calls the C entry point with a null-terminated argument vector while holding the Python GIL. |
| Garbage collection | The Linux RTDyld callbacks register entire writable section mappings as GC roots and unregister each mapping before release. C++ registers writable segments within a contiguous JITLink slab. Other hosts retain the per-global registration path. |
| Exceptions | Meister uses the existing runtime JITError path through a C callback on Linux x86-64. It keeps the loaded object and symbolizes its frames with the LLVM installation’s llvm-symbolizer. Older runtimes use the standalone handler. C++ uses an in-process symbolizer and JITLink section addresses. |
| LLVM passes | Meister ports coroutine resume inlining, coroutine unroll guards and branch simplification, OpenMP thread-ID reuse, allocation removal and stack demotion, loop allocation hoisting, and optional allocation autofree. The LLVM C API has no PassBuilder extension callbacks, so these transformations run at module boundaries around the two O3 invocations. C++ schedules them within each O3 pipeline. LLVM analyses for capture and nonzero values also have more information than the corresponding Meister checks. |
| Native targets and GPU | Linux x86-64 is the verified target. C++ has further CPU-feature selection for ARM and AArch64, target-selection flags, Windows-specific SEH registration and export handling, and a GPU module, PTX, and libdevice pipeline. These paths still require parity work and target-specific verification. |
| Native output | The build command emits CIR, LLVM IR, bitcode, assembly, objects, executables, shared libraries, and CPython extension objects. It supports native format detection, library dependencies, linker flags, PIE, and global constructors. Linker command ordering, optional toolchain flags, and platform integration can differ. |
| CPython extensions | Meister emits CPython module and type definitions, slot tables, and exception translation wrappers directly into the LLVM module. Linking the result into a loadable extension is a separate step, as in C++. |
| Native integrations | Compiler DSL plugins, native optimizer plugins, and the incremental `codon jit` session use C++ compiler APIs. This backend provides whole-program `run` through ORC. |

## LLVM optimization and JIT port verification

- The 64-worker sweep passed 120 focused cases. It covered the ported optimizer and JIT tests, 59 C++ optimizer assertions, and selected core, SIMD, OpenMP, Python bridge, NumPy, and stdlib cases. [64-worker log](../../build/meister-backend-check/llvm-port/pytest64.log)
- The final focused rerun passed 37 cases, including the ported allocation, coroutine, and OpenMP tests, both backend modes, GDB registration and teardown, source backtraces, and recursive debug types. [Ported-test log](../../build/meister-backend-check/llvm-port/ported-tests.log)
- The Python unit run passed 149 cases and 3,313 subtests. [Unit log](../../build/meister-backend-check/llvm-port/python-unit.log)
- The uncaught JIT exception matched C++ stderr and exit status in debug and release after normalizing program-counter addresses. The debug comparison checks frame names, files, lines, and columns. [Backtrace comparison](../../build/meister-backend-check/llvm-port/cpp-backtrace.log)

The existing runtime was relinked with the rebuilt exception object after the
configured Clang build aborted on this host. The C callback adds access to the
existing native JIT error handler. The runtime's ELF dependencies and runpath
match the previous build.

## Verification before the LLVM optimization and JIT port

The verification environment uses the existing uv Python 3.13.2 environment,
Exaloop LLVM 20.1.7 and the Codon runtime under `StdEnv/2023`.

- The unit and backend run passed 123 tests. The backend checks cover both modes, exercise the JIT, emit six native and LLVM formats, run the linked executable, and inspect the process library mappings. [Unit and backend log](../../build/meister-backend-check/unit.log)
- The focused runtime run passed 16 tests in both modes, including garbage collection, exceptions, arithmetic, the Python bridge and NumPy's Python bridge. [Focused runtime log](../../build/meister-backend-check/runtime.log)
- Hello world matched C++ at the typed AST, translated CIR, and final debug CIR stages. The release JIT CLI also matched C++ stdout using the same program arguments.
- The release executables for `test/core/exceptions.codon` matched C++ stdout across 146 lines, stderr and exit code. The JIT resolved the runtime library explicitly supplied with `-l`. The shared-library build matched the C++ constructor output. [Executable comparison](../../build/meister-backend-check/aot-reference.log) [JIT library test](../../build/meister-backend-check/jit-library.log) [Constructor comparison](../../build/meister-backend-check/library-reference.log)
- The CLI built and executed a PIE from stdin with a static definition supplied by `-D`. Linux ELF metadata confirmed the runtime links and runpath for the executable and the CPython extension.
- The repository’s CPython extension script passed against both Meister-built extension modules in debug and release. It exercises exception translation, dispatch, generated classes, NumPy and repeated parallel calls. [Release log](../../build/meister-backend-check/pyext.log) [Debug log](../../build/meister-backend-check/pyext-debug.log)
- All 2,001 collected pytest cases passed. The 64-worker sweep passed 1,999 cases and 3,166 subtests. The two NumPy linear algebra modes passed in a separate two-worker invocation running concurrently. [64-worker log](../../build/meister-backend-check/pytest64.log) [Linear algebra log](../../build/meister-backend-check/pytest-linalg.log)

| Suite | Passing cases |
| --- | ---: |
| Registered Codon source fixtures in debug and release | 1,630 |
| Python unit and LLVM backend smoke tests | 123 |
| Compiler-backed CIR and LLVM checks | 9 |
| Native GoogleTest reference assertions | 239 |
| **Total** | **2,001** |

The native reference assertions used the existing C++ build. The C++ LLVM
visitor passed a syntax check after removal of the bridge exports.
[C++ syntax log](../../build/meister-backend-check/cpp-syntax.log)
