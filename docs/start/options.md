## `-debug`

Turns off compiler optimizations and enables backtraces.

Use this when debugging compiler output or runtime failures.

This is the default optimization mode.


## `-release`

Turns on compiler optimizations and disables debug information.

Use this for production builds or performance benchmarking.


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
