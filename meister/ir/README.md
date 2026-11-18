# Meister CIR port

The Python modules mirror the C++ sources in `codon/cir/`. AST translation is
in `meister/passes/translate/`, corresponding to
`codon/parser/visitors/translate/`. Source comments and C++ source references
are kept alongside the ported implementations.

The normal debug pipeline registers the same passes in the same order as
`PassManager::registerStandardPasses()`:

1. Pipeline lowering
2. Imperative-for lowering
3. Missing-return lowering
4. Throw lowering
5. Async-for lowering
6. Await lowering
7. OpenMP parallel lowering
8. Pythonic formatting optimization

The supporting utilities include traversal, cloning, outlining, scheduling,
intrinsic metadata, and CIR text formatting. Release/test CIR optimization
pipelines are outside this port and explicitly unsupported.

## LLVM emission and execution

Build Codon against Exaloop LLVM first. Meister uses `ctypes` to load the
LLVM-C symbols from that build's `libcodonc`. It does not use a stock LLVM,
and does not need a separate shared LLVM installation or Python extension.

```sh
cmake --build build --target codonc --parallel 1
python -m meister llvm -o /tmp/meister-scratch.ll scratch.codon
python -m meister run scratch.codon
```

`llvm` prints verified, unoptimized LLVM IR (stdout if no output file is
specified). `run` transfers that same module to Codon's native optimizer and
ORC JIT. Both accept `--library /path/to/libcodonc` before the source filename,
or use `CODON_LIBRARY`. `run` passes arguments after the filename to the program.
For Codon programs that import Python, set `CODON_PYTHON` to the CPython shared
library, as with native Codon.

`llvm.py` wraps native handles; `llvisitor.py` implements CIR lowering. Inline
`@llvm` bodies are parsed and linked into the live module in the same LLVM
context. Try/except/else/finally lowering also uses Codon's native unwinder.
Residual pipeline nodes, DSL plugins, incremental JIT cells, and standalone
or Python-extension emission are not covered.

## CIR dumps

To dump CIR after the debug pipeline:

```sh
python -m meister ir scratch.codon
```

Add `--no-passes` to dump directly after translation. The command locates local
build-generated stdlib tables as well as the source stdlib.

`meister.ir.dump` has the same options and layout as `meister.ast.dump`:
`annotate_fields`, `include_attributes`, `indent`, and `types`. Dumps use
`NodeName(field=value, ...)`, AST-style lists, quoting, booleans and omission
of empty fields. `TypeRef`, `VarRef` and `ValueRef` preserve shared/cyclic IR
edges. Source locations and ownership/cache back-pointers are omitted as in
the AST dumper. Attributes use the shared AST symbolic key lookup, or concrete
IR attribute names, and include typed payloads. Runtime type descriptors,
function flags, scheduling, instruction flags and literal metadata are included.
Null positional slots in pipelines and partial functions are printed as `None`
so that formatting cannot silently change their argument layout.

The CLI defaults to `--indent 2` with attributes and type information enabled.
Use `--indent 0`, `--no-field-names`, `--no-attributes` or `--no-types` to adjust it.

Compare the post-pass dump directly with the C++ CLI:

```sh
build/codon build --debug --cir scratch.codon -o /tmp/cpp-scratch.cir
python -m meister ir scratch.codon > /tmp/python-scratch.cir
cmp /tmp/cpp-scratch.cir /tmp/python-scratch.cir
```

Compare typechecking, translation and the final debug pipeline output without
LLVM emission (intermediate passes are not dumped or compared):

```sh
python -m meister.ir.compare scratch.codon --output /tmp/scratch-ir-stages
python -m meister.ir.compare test/parser/typecheck --jobs 6 --timeout 300 \
  --output /tmp/typecheck-ir-suite
python -m meister.ir.compare test/parser --jobs 4 --timeout 1200 \
  --output /tmp/parser-ir-suite
python -m meister.ir.compare test/core --jobs 3 --timeout 1200 \
  --output /tmp/core-ir-suite
```

Directory runs include every immediate `*.codon` file and every `#%%` case, with the
fixture's original filename, line offset and barebones setting. Each case runs
in an isolated worker. Negative cases rejected by both frontends are reported
separately; internal failures, crashes, timeouts, and translation/pass failures
are mismatches, not compiler rejections. The existing `meister/test.py` driver
is unchanged.

The typechecker suite has been checked without skipping cases: 380 cases have
matching typed ASTs, translated IR, and final debug IR; another 246 are rejected
by both compilers. Focused reruns after fixes are included in those 626 cases.
The remaining immediate parser files add 13 matching cases. Core adds 18 matching
files; `sort.codon` is rejected by both compilers because it supplies the `sorted`
key function positionally, although that parameter is keyword-only. No fixtures
are excluded or changed.

The comparison is strict: byte counts and ordered 64-MiB SHA-256 chunks cover the entire dump,
without filtering or ignoring fields. Only failing dumps are saved, gzip
compressed, alongside bounded first-difference excerpts, per-case diagnostics,
and `summary.json`. Typechecked ASTs default to compact output (`--ast-indent 0`)
to avoid enormous repeated indentation. `--filter` selects case labels for
focused debugging; omit it to run the complete suite.

Typed AST forests are hashed one realized function at a time. Compact IR is
emitted one declaration at a time, with byte-identical text and shared graph
numbering. This avoids retaining multi-gigabyte closure dumps in memory.
Native and Python frontends run in separate processes so native allocator arenas
are released before Python serializes the same large graphs.
Chunk boundaries are independent of formatter emissions. This also avoids an
LLVM 20 SHA-256 length-counter bug for messages larger than 512 MiB.

Whole-module dumps use deterministic declaration order and dump-local graph
IDs, not compiler allocation IDs. Generated generator names use their semantic
owner and occurrence within its body rather than global creation ordinals.
Capture bindings are displayed consistently at declarations, signatures and calls.
Adjacent literal
stores to distinct variables are displayed in name order (early compile-time
defines originate in an unordered C++ map). Other statement and positional
argument order is preserved. These are read-only formatting operations; they
do not change compiler counters, stored IR or executable code.

Python-only checks:

```sh
python -m unittest meister.ir.test_python meister.ir.test_format -v
python -m compileall -q meister/ir meister/passes/translate
ruff check meister/ir meister/passes/translate
```

The checks cover imports, annotations, class state declarations, Python call
signatures, constructors, frontend handle types, and debug pass registration.
The plumbing tests do not run transformations. Formatting regressions check
allocation/declaration/capture-order independence, preserve observable value
and positional-argument differences, and verify that dumping does not mutate IR.
Additional regressions compare the layout directly with `ast.dump` and cover
attribute enum lookup and replacement-aware graph numbering.
Generated function naming, unsigned integer widths, and byte-string quoting
also have regressions.
Chunked output is checked against the ordinary formatter, including indentation,
field names, and attributes; printable long-name quoting has an equivalent fast path.
