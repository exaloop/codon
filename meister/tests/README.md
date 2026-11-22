# Meister tests

From the repository root, use the Python 3.13 uv environment and a CMake build
configured with shared Exaloop LLVM.

```sh
uv sync --locked --python 3.13
cmake --build build --target codonrt codon_test --parallel 8
uv run --locked python -m pytest -n 64 meister
```

With an existing environment such as `.venv/bin/python`, use
`uv run --locked --python .venv/bin/python python -m pytest -n 64 meister`.
The test dependency group includes NumPy with the version used by native
Python 3.13 CI. Meister fixtures use the shared LLVM and Codon runtime described in the
[backend instructions](../ir/README.md#llvm-emission-and-execution). Use
`--codon-build /path/to/build` for a separate runtime build and
`--llvm-library /path/to/libLLVM.so` to select LLVM explicitly. Pytest finds
`codon_test` separately for the optional native reference cases. Use the same linker toolchain on `PATH` that you used to build Codon
for native tests that link executables. The runner derives `CODON_PYTHON`
from the active interpreter unless it is set explicitly.

```sh
uv run python -m pytest -n 64 meister -m numpy
uv run python -m pytest meister -k 'test_access and new_scoping and release'
uv run python -m pytest meister -m 'not compiler and not native'
uv run python -m pytest meister -m native
```

[Test fixture registrations](../../test/fixtures.tsv) are shared with
GoogleTest. Each `#%%` section in the typechecking suite has its own pytest ID
with the source line and mode. `__ignore__` sections are excluded and
`barebones` sections load the same
reduced standard library as GoogleTest. Other files are collected as whole
programs. Both modes use the optimized CIR pipeline in compiler fixture mode.
Each uses its own `__debug__` define and LLVM optimization settings. GPU
fixtures stop after compilation, as in GoogleTest. Runtime fixtures enable
the standalone runtime flag, as the native runner does.

Each source case runs in a separate process and temporary working directory.
`# EXPECT:` and `#:` lines specify stdout, and `#!` lines specify an expected
compiler rejection with matching diagnostic prefixes. The
[Meister diagnostic expectations](diagnostics.tsv) give the exact prefixes
for named cases where the PEG parser or typechecker reports a different
diagnostic. Cases without output markers still check exit status and the
`TEST FAILED` sentinel. Large NumPy fixtures can take substantially longer
than other cases. Set an optional per-case limit with `--codon-timeout SECONDS`.

The CIR tests exercise Meister's node, analysis, DSL extension, and transform
APIs directly. The `native` cases execute individual GoogleTest assertions for
the C++ implementation, including native LLVM optimizer, dynamic DSL plugin,
and incremental JIT tests. They are reported separately from the Meister
compiler and CIR cases. Native tests can skip for unsupported target features.
