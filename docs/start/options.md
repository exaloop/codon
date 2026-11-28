---
title: Compiler options
---

## `-debug`

Turns off compiler optimizations and enables backtraces.

Use this when debugging compiler output or runtime failures.

This is the default optimization mode.


## `-release`

Turns on compiler optimizations and disables debug information.

Use this for production builds or performance benchmarking.


## `-release-quick`

Use this for edit/run cycles when you want release semantics, including plugin
optimizations, without the full cost of a production build. It runs each CIR
folding group once and uses one host LLVM O2 pipeline instead of two O3
pipelines. Generated code may be slower. Use `-release` for final benchmarks.


## `-compile-trace` (build only)

`codon build -release -compile-trace -o app main.py` writes `app.trace.json`
alongside the normal output. Open the JSON in Perfetto or a Chrome trace viewer.
`Codon Import` shows why a module was loaded; `Codon Parse File` isolates the
file read and parse. `Codon Realize Function` shows which functions asked the
type checker to specialize others, labeled by function family. Within each
`Codon LLVM` round, `PassManager<Function>` identifies optimization work on
individual specializations. `OptFunction` is separate code-generation time.

Event times are inclusive: a parent's duration already includes its children.
Individual spans shorter than 0.5 ms are omitted. Use untraced builds for
wall-clock benchmarks.


## `-cir-analysis-threads <n>`

Maximum compiler workers for independent reaching-definition and dominator analyses. Large
modules use up to four physical cores by default; small modules stay serial.
Use `1` for serial compilation or `0` to allow all available physical cores.
This does not change the thread count of the compiled program.
Embedded compilers can set `Options::threads` per instance.
In Python, use `codon.set_options(threads=1)` before the first JIT call.


## `-disable-exceptions`

Disables exception handling. Attempting to raise an exception will
trap. Enables further optimization by eliminating exception code paths.

**Default:** `false`


## `-fast-math`

Enables fast-math optimizations.

This may improve performance but can change floating-point behavior.

**Default:** `false`


## `-disable-native`

Disables architecture-specific optimizations.

By default, Codon may optimize for the host CPU architecture.

**Default:** `false`


## `-auto-python`

Automatically falls back to Python when importing modules that Codon
does not support natively.

**Default:** `false`


## `-D<name>=<value>`

Adds a literal variable definition. For example `-Dfoo=42` will include variable
`foo` with value `42` as a `Literal[int]` during compilation.


## `-disable-opt <name>`

Disables a specific Codon IR optimization pass. Can be specified multiple times.


## `-plugin <path>`

Loads the specified Codon plugin. The provided path should refer to the directory containing
the plugin's `plugin.toml` configuration file. Can be specified multiple times.


## `-log <streams>`

Enables the specified compiler log streams. See [Logging](usage.md#logging) for more information.


## `-numerics=<py|c>`

- `py`: Uses Python-style numerical semantics. Closely matches CPython behavior, but may
        disable certain optimizations such as vectorization.
- `c`:  Uses C-style numerical semantics. Provides the best performance, but behavior
        may differ from Python in areas such as integer division and division by zero.

Note that Codon `int`s are 64-bit signed integers regardless of this setting.

**Default:** `py`


## `-libdevice <path>`

Path to the NVIDIA `libdevice` bitcode library used when compiling GPU kernels.

**Default:** `/usr/local/cuda/nvvm/libdevice/libdevice.10.bc`


## `-gpu-name <architecture>`

Target GPU architecture or compute capability.

**Default:** `sm_30`


## `-gpu-features <features>`

Additional GPU feature flags passed to the backend.

**Default:** `+ptx42`


## `-ptx <file>`

Writes generated PTX assembly to the specified file.


## `-unordered-dict`

Uses the unordered dictionary implementation.

**Default:** `false`


## `-global-ctor=<yes|no|auto>`

- `yes`: insert a call to top-level program code in a global constructor
- `no`: do not generate global constructors
- `auto`: `yes` if generating a shared library, `no` otherwise

**Default:** `auto`
