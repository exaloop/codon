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
| Debug metadata | The LLVM C API replaces a temporary composite type with the final structure after its members are lowered. C++ updates the member array in place. The Python approach supports recursive debug types through LLVM metadata replacement. |
| JIT linking | LLVM-C creates and owns LLJIT and its thread-safe module. The default LLJIT configuration picks JITLink on the verified Linux x86-64 platform. C++ configures its own linking layer, debugger plugins, and platform registration. Meister calls the normal C entry point with a null-terminated argument vector while holding the Python GIL. |
| Garbage collection | Meister registers address ranges for writable, non-TLS LLVM globals after ORC resolves them and unregisters the ranges before destroying the JIT. C++ registers whole JIT allocation slabs through its memory manager. The Meister table omits JIT linker allocations and TLS storage. |
| Exceptions | Both paths use the Codon runtime ABI for caught exceptions. CLI JIT programs use the runtime's standalone unhandled-exception handler. The C++ JIT uses a callback with JIT symbol lookup. Debug backtraces from unhandled Meister JIT exceptions can differ. |
| LLVM passes | The LLVM C API runs the default O0 pipeline once or the default O3 pipeline twice. Meister ports the debug settings, scalar fast math, host attributes, and OpenMP thread-ID rewrite. The rewrite runs before each O3 pipeline. C++ installs it at a PassBuilder extension point. The native coroutine, allocation-removal, allocation-hoisting, and allocation-autofree passes still differ. This can change optimized IR, assembly, and performance. |
| Native targets and GPU | Linux x86-64 is the verified target. C++ has further CPU-feature selection for ARM and AArch64, target-selection flags, Windows-specific SEH registration and export handling, and a GPU module, PTX, and libdevice pipeline. These paths still require parity work and target-specific verification. |
| Native output | The build command emits CIR, LLVM IR, bitcode, assembly, objects, executables, shared libraries, and CPython extension objects. It supports native format detection, library dependencies, linker flags, PIE, and global constructors. Linker command ordering, optional toolchain flags, and platform integration can differ. |
| CPython extensions | Meister emits CPython module and type definitions, slot tables, and exception translation wrappers directly into the LLVM module. Linking the result into a loadable extension is a separate step, as in C++. |
| Native integrations | Compiler DSL plugins, native optimizer plugins, and the incremental `codon jit` session use C++ compiler APIs. This backend provides whole-program `run` through ORC. |

## Verification

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
