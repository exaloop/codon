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
intrinsic metadata, and CIR text formatting. LLVM function bodies remain text
and metadata; `llvisitor` and LLVM emission are not ported. Release/test
optimization pipelines are outside this port and explicitly unsupported.

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

Compare typechecking, translation and every individual debug pass without
LLVM emission:

```sh
python -m meister.ir.compare scratch.codon --output /tmp/scratch-ir-stages
```

All ten stages are byte-identical for `scratch.codon`, including the full
stdlib. The comparison is strict, without filtering or ignoring dump fields.
Typechecked ASTs default to compact output (`--ast-indent 0`) to avoid enormous
repeated indentation; both indented and compact AST output have been checked.
No other Codon programs have been checked for IR equivalence. The existing
`meister/test.py` driver is unchanged.

Whole-module dumps use deterministic declaration order and dump-local graph
IDs, not compiler allocation IDs. Generated generator capture bindings are
displayed consistently at declarations, signatures and calls. Adjacent literal
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
