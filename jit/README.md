To install:

```bash
$ pip install .
```

If Codon is installed in non-standard directory, please set `CODON_DIR` accordingly.

To use:

```python
import codon

@codon.jit
def ...
```

## Compiler options

Configure the shared JIT before the first `@codon.jit`, `@codon.convert`, or
`codon.execute()` call:

```python
import codon

codon.set_options(fastmath=True, pynum=False)

@codon.jit
def divide(value, divisor):
	return value // divisor

assert divide(-3, 2) == -1
```

Option names match the fields in `codon/compiler/options.h`:

| Type | Options |
| --- | --- |
| `bool` | `debug`, `pmempty`, `capture`, `native`, `pynum`, `noexc`, `fastmath`, `autopy`, `autofree`, `unordereddict`, `cabi` |
| `str` | `libdevice`, `gpuName`, `gpuFeat`, `gpuOutput`, `log`, `march`, `mcpu` |
| List of `str` | `plugins`, `defines`, `disabled`, `mattrs` |

Defaults match `Options`, except the JIT compiles in release mode (`debug=False`).
For example, `pynum=False` selects C numerical semantics, `fastmath=True` relaxes
floating-point semantics, and `unordereddict=True` selects unordered dictionaries.
`defines=["SIZE=64", "LABEL=str:example"]` supplies compile-time constants using
the same syntax as the CLI's `-D` flag. Plugins are loaded before initialization.
Logging controls the compiler's process-wide logger.

Repeated `set_options()` calls merge settings; list-valued options are replaced,
not appended. Values are copied, and invalid names or values leave the previous
configuration intact. Unknown options and invalid field types raise `ValueError`
(non-JSON-serializable Python values raise `TypeError`). Initialization failures
raise `JITError`.

Options cannot change after the first JIT use: `set_options()` then raises
`RuntimeError`. This avoids invalidating compiled functions or changing semantics
partway through a session. Options are preserved across the JIT's automatic
error-recovery resets. Internal execution-mode fields (`argv0`, `jit`, `test`,
`standalone`, `pyext`, and `ctor`) are not configurable through this API.

`debug=True` here controls compiler optimization/debug information. The existing
`@codon.jit(debug=...)` argument and `CODON_JIT_DEBUG` control diagnostic verbosity.

Independent low-level wrappers accept the same keyword options and allow updates
until their first `execute()` or `run_wrapper()` call:

```python
wrapper = codon.JITWrapper(pynum=False)
wrapper.set_options(fastmath=True)
wrapper.execute("assert -3 // 2 == -1", "<example>", 1, False)
```

The C bridge exposes `jit_validate_options(json)` and
`jit_init_with_options(name, json)`, using a JSON object with the same field names.
Both return `CJITResult`; errors are allocated strings that the caller must
`free()`. Successful initialization returns the JIT handle in `result`, owned by
the caller and released with `jit_exit()`. The original `jit_init(name)` remains
available with its existing defaults.
