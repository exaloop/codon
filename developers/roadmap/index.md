# Roadmap

Codon's development balances Python compatibility with native compilation and the opportunities it creates for optimization and parallel execution. This page outlines areas for further work, rather than a release schedule or a commitment to a particular implementation.

Several capabilities from earlier roadmaps are now available, including Unicode strings, native NumPy, `async`/`await`, and native `os`, `sys`, `pathlib` and `unittest` modules. The [language overview](https://docs.exaloop.io/language/overview/) and [standard-library guide](https://docs.exaloop.io/libraries/stdlib/) describe current behavior; the [release notes](https://docs.exaloop.io/start/changelog/) record changes between versions.

## Core features

-   Parallelism
    
    -   `multiprocessing` support
    -   Automatic locking in parallel code (e.g. if mutating a data structure shared between threads)
    -   Race detection
-   Broader Python syntax and object-model compatibility:
    
    -   Extend pattern-matching coverage
    -   Support accessing various object properties (`__dict__`, `__slots__` etc.) as much as possible in Codon's static context
-   Optional automatic switching between Codon and CPython (i.e. compile only compatible functions and leave the rest to Python)
    
-   Better error messages
    
    -   Warning support
    -   Explain performance considerations
    -   Explain Codon-specific optimizations (e.g. Codon IR passes)
    -   Explain that a CPython feature is not supported
-   Modules and incremental compilation
    
    -   Cache compilation modules
    -   Fast generics compilation in debug mode for quick turnarounds
-   Memory management
    
    -   Auto-tune GC
    -   Optional alternative memory management modes like reference counting
-   GPU support
    
    -   Target Apple, AMD and Intel GPUs
    -   GPU-specific compiler optimizations (e.g. for using various Python constructs on the GPU)
-   Interoperability with other languages
    
    -   Direct C++ interoperability via Clang
    -   R interoperability

## Libraries

Python interoperability provides access to libraries that are not implemented natively, while native implementations can participate more directly in Codon's optimizations and parallel execution. Further library work includes:

-   Python's standard library
    
    -   Complete builtins support
    -   1-to-1 compatibility with existing Python functions and modules
        -   Remaining modules such as `struct`, and missing APIs within existing modules
        -   Additional coverage driven by application requirements
-   Further NumPy compatibility and optimization, building on the existing native implementation
    
-   Native implementations of additional third-party libraries, including dataframe-oriented functionality
    
-   Broader compatibility with Python's testing infrastructure
    

## Infrastructure & Tools

-   Windows support
    
-   A package-management workflow similar to Rust's [Cargo](https://github.com/rust-lang/cargo)
    
-   Auto-detection of installed Python libraries
    
-   Improved `codon.jit` library support
    
    -   Better error messages
    -   Better installation flow
-   Fully static binary support like Go
    
    -   Remove `libcodonrt` (runtime library) dependency if needed
    -   Remove `libcpp` dependency
-   Improved Jupyter support
    
    -   Auto-completion and code inspection
    -   Jupyter magic command support
-   Improved editor integration, including diagnostics and code navigation
    

## Nice to have

-   Implement Codon in Codon

Source: https://docs.exaloop.io/developers/roadmap/
