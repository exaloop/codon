Codon's goal is to be as close to CPython as possible while still
being fully statically compilable. While Codon already supports
much of Python, there is still much to be done to fully realize
its potential. Here is a high-level roadmap of the things we want
to add, implement or explore.

# Core features

- Type system improvements:
  - First-class types and compile-time metaclasses
  - Full class member deduction
  - Implicit union types to support mixed-type collections
  - Variadic type arguments (e.g. `Foo[Bar, ...]`)

- Parallelism
  - `async`/`await` support
  - `multiprocessing` support
  - Automatic locking in parallel code (e.g. if mutating a
    data structure shared between threads)
  - Race detection

- Compatibility with Python 3.10+:
  - Argument separators (`/` and `*`)
  - Constructor object matching in the `match` statement
  - Support accessing various object properties (`__dict__`, `__slots__`
    etc.) as much as possible in a static context

- Optional automatic switching between Codon and CPython (i.e.
  compile only compatible functions and leave the rest to Python)

- Better error messages
  - Warning support
  - Explain performance considerations
  - Explain that a CPython feature is not supported

- Modules and incremental compilation
  - Cache compilation modules
  - Fast generics compilation in debug mode for quick turnarounds

- Memory management
  - Auto-tune GC
  - Optional alternative memory management modes like reference
    counting

- GPU support
  - Target Apple, AMD and Intel GPUs
  - GPU-specific compiler optimizations (e.g. for using various
    Python constructs on the GPU)

- Interoperability with other languages
  - Direct C++ interoperability via Clang
  - R interoperability

# Libraries

Currently, missing Python functionality can be easily accessed via a
`from python import foo` statement, which is sufficient in most cases
as many libraries are just thin wrappers around a C library and/or not
performance-sensitive.

However, in the near future, we would like to support the following
modules natively:

- Python's standard library
  - Complete builtins support
  - 1-to-1 compatibility with existing Python functions and modules
  - File modules: `os`, `sys`, `struct`, `pathlib` and so on
  - Pretty much everything else on an as-needed basis

- Native NumPy, Pandas, etc.: Having Codon-native versions of the most
  popular 3rd-party libraries would allow them to work with Codon's
  other features like multithreading and GPU. We're currently prioritizing
  NumPy and Pandas but aim to later target other popular libraries as well.
  - **As of Codon 0.18, NumPy is natively supported!**

- Unicode support

- Python's testing infrastructure

# Infrastructure & Tools

- Windows support

- A sane package manager similar to Rust's
  [Cargo](https://github.com/rust-lang/cargo)

- Auto-detection of installed Python libraries

- Improved `codon.jit` library support
  - Better error messages
  - Better installation flow

- Fully static binary support like Go
  - Remove `libcodonrt` (runtime library) dependency if needed
  - Remove `libcpp` dependency

- Improved Jupyter support
  - Auto-completion and code inspection
  - Jupyter magic command support

- Plugins for Visual Studio Code, Vim, Emacs and so on

# Documentation

- Fully document major differences with CPython
- Document Codon IR API, with guides and tutorials
- Document all supported modules

# Nice to have

- Implement Codon in Codon
