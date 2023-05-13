The main goal of Codon is to close the gap between it and Python as much as possible without sacrificing [its major objectives](https://github.com/exaloop/codon).
This is far from being trivial and will take some time (hey, it took Python 30+ years to get there!).
We are currently addressing the missing features on a need basis. The current roadmap roughly looks as follows:

# Language

- Fix any outstanding parsing and compilation bugs (a never-ending quest)!

- Type system improvements
  - `Any` type support
  - First-class types and compile-time metaclasses
  - Back-references to functions and classes
  - Better OOP support
  - Better handling of unions
  - Automatic boxing and unboxing of by-value objects
  - Dynamic (runtime) tuples and non-homogenous dictionaries
  - Better support for `Optional[T]` handling
  - Variardic type arguments (e.g., `Foo[Bar, ...]`)

- Parallelism
  - `async`/`await` support
  - More OpenMP hooks
  - Go-like channels
  - Automatic locking where needed
  - `multiprocessing` support

- Performance
  - Support code/model auto-tuning in Codon IR
  - Integrate `polyll`

- Compatibility with Python 3.10+:
  - Argument separators (`/` and `*`)
  - Constructor object matching in the `match` statement
  - Support accessing various object properties (`__dict__`, `__slots__` etc.) as much as possible in static context

- Automatic switching between Codon and CPython (i.e., compile only compatible functions and leave the rest to Python)

- Better error messages
  - Warning support
  - Explain performance considerations
  - Explain that a CPython feature is not supported

- Modules and incremental compilation
  - Cache compilation modules
  - Fast generics compilation in debug mode for quick turnaround

- Memory management
  - Auto-tune GC
  - Allow alternative GC backends
    - Use ARC instead of GC in performance-heavy loops
  - Fix GC-related performance issues in multi-threaded mode

- GPU support
  - Target Apple, AMD and Intel GPUs

- Interoperability with other languages
  - R interoperability
  - C++ interoperability via Clang

# Libraries

Currently, missing Python functionality can be easily accessed through `from python import xyz` statement. In most cases, that is good enough,
as many libraries are just thin wrappers around C calls, and their performance is not a major consideration.

In near future, we would like to support the following modules natively:

- Python's standard library
  - Complete builtins support
  - 1-to-1 compatibility with existing Python functions and modules
  - File modules: `os`, `sys`, `struct`, `pathlib` and so on
  - Pretty much everything else on a need basis
- Testing infrastructure
- Native Numpy support
- Native Pandas support

# Infrastructure / Tools

- A sane package manager akin to Rust's [Cargo](https://github.com/rust-lang/cargo)
  - Think of something better and try to avoid Python's packaging hell
- Windows support
  - In theory doable with some changes; developers with Windows experience are needed to facilitate this
- Auto-detection of Python libraries
- Improved `codon.jit` library support
  - Better error messages
  - Better installation flow
- Fully static binary support akin to Go
  - Remove `libcodonrt` dependency if needed
  - Remove `libcpp` dependency
- Better Jupyter support
  - Auto-complete and code inspection
  - Jupyter magic command support
- Plugins for Visual Studio Code, vim, Emacs and so on

# Documentation

- Completely document major differences with Python
- Document Codon IR API
- Document all major gotchas
- Document all supported modules
- Document everything else!

# Nice to have

- New parser
- Implement Codon in Codon
