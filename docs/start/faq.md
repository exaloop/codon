---
title: Frequently asked questions
---

## Technical

### What is Codon?

Codon compiles Python-like programs to native machine code, with type inference
and specialization replacing much of the work normally performed by an
interpreter. This can substantially accelerate computation written in Python,
while native multithreading provides another way to use the available hardware.
The benefit depends on the workload: a loop implemented in Python and a call
to an already optimized external library offer different opportunities.

Codon still uses runtime services, including memory management and exception
handling. It is also extensible through plugins that add libraries, compiler
optimizations and language features.

### What isn't Codon?

While Codon supports nearly all of Python's syntax, it is not a drop-in replacement, and large
codebases might require modifications to be run through the Codon compiler. For example, some
of Python's modules are not yet implemented within Codon, and a few of Python's dynamic features
are disallowed. The Codon compiler produces detailed error messages to help identify and resolve
any incompatibilities. Codon supports seamless [Python interoperability](../integrations/python/python-from-codon.md) to
handle cases where specific Python libraries or dynamism are required, and also supports writing
[Python extension modules](../integrations/python/extensions.md) that can be imported and used
from larger Python codebases.

### Why Codon?

Python is arguably the world's most popular programming language, and is gradually becoming the
*lingua franca* particularly amongst non-technical or non-CS practitioners in numerous fields.
It provides a readable, clean syntax, is easy to learn, and has an unmatched ecosystem of libraries.
However, Python's achilles heel has always been performance: a typical codebase in pure Python is
orders of magnitude slower than its C/C++/Rust counterpart.

Codon bridges the gap between Python's simplicity and ease-of-use, and the performance of low-level
languages like C++ or Rust, by using [novel compiler and type checking techniques](https://dl.acm.org/doi/abs/10.1145/3578360.3580275)
to specialize and compile code ahead of time. This makes it possible to retain
a familiar programming style while reducing interpreter overhead in native
portions of a program.

### How does Codon compare to...

- **CPython?** Codon tries to follow CPython's syntax, semantics and APIs as
  closely as possible, aside from a few cases where Codon differs from CPython for
  performance reasons (one example being Codon's 64-bit `int` vs. CPython's arbitrary-
  width `int`). Performance gains depend on how much work is performed in native
  Codon code rather than in I/O or external libraries.

- **Numba?** While Codon does offer a JIT decorator similar to Numba's, Codon is in
  general an ahead-of-time compiler that compiles end-to-end programs to native code.
  It also supports compilation of a much broader set of Python constructs and libraries.

- **PyPy?** PyPy strives to effectively be a drop-in replacement for CPython, whereas
  Codon makes different compatibility trade-offs to compile native types ahead
  of time. The more suitable choice depends on the application's dynamic
  features, dependencies and performance characteristics.

- **Cython?** Like Cython, Codon has a [Python-extension build mode](../integrations/python/extensions.md) that
  compiles to Python extension modules, allowing Codon-compiled code to be imported and called
  from plain Python.

- **C++?** Codon and C++ can produce similar native code for equivalent
  operations. Codon can optimize across application code and native library
  implementations, including Python-specific container and array operations.
  Differences in algorithms, data layout and external libraries still matter
  when comparing complete applications.

- **Julia?** Codon's compilation process is actually much closer to C++ than to Julia. Julia
  is a dynamically-typed language that performs type inference as an optimization, whereas
  Codon type checks the entire program ahead of time. Codon also tries to circumvent the learning
  curve of a new language by adopting Python's syntax and semantics.

- **Mojo?** Mojo strives to add low-level programming support/features to the Python language,
  while also supporting the rest of Python by relying on CPython. By contrast, Codon aims to
  make Python itself more performant by using new type checking and compilation techniques,
  without trying to be a superset or drop-in replacement. Codon tries to minimize new syntax
  and language features with respect to Python.

You can see results from [Codon's benchmark suite](https://github.com/exaloop/codon/tree/develop/bench)
at [exaloop.io/#benchmarks](https://exaloop.io/#benchmarks).
More benchmarks can be found in the [2019 paper](https://dl.acm.org/doi/10.1145/3360551)
on bioinformatics-specific use cases (note that the name used in that paper is that of Codon's predecessor,
"Seq").

### I want to use Codon, but I have a large Python codebase I don't want to port.

You can use Codon on a per-function basis via the [`@codon.jit` decorator](../integrations/python/codon-from-python.md),
which can be used within Python codebases. This will compile only the annotated functions
and automatically handle data conversions to and from Codon. It also allows for
the use of any Codon-specific modules or extensions, such as multithreading.

Codon can also [compile to Python extension modules](../integrations/python/extensions.md) that can be
imported and used from Python.

### What about interoperability with other languages and frameworks?

Interoperability is and will continue to be a priority for Codon.
We don't want using Codon to render you unable to use all the other great frameworks and
libraries that exist. Codon supports full interoperability with Python and C/C++.

### Does Codon use garbage collection?

Yes, Codon uses the [Boehm garbage collector](https://github.com/ivmai/bdwgc)
for native managed allocations. Files and other external resources still need
their normal cleanup; a [context manager](../language/errors.md#working-with-files)
keeps that cleanup tied to the code using the resource. Memory borrowed from a
C library follows the library's own ownership rules.

### Codon doesn't support Python module X or function Y.

While Codon covers a sizeable subset of Python's standard library, it does not yet cover
every function from every module. The [standard-library guide](../libraries/stdlib.md)
and API reference describe native coverage. Missing functionality can often be
called through Python via `from python import`, without moving the rest of the
application back to Python.

### Codon is no faster than Python for my application.

Applications that spend most of their time in C-implemented library code generally do not
see substantial performance improvements in Codon. Similarly, applications that are I/O or
network-bound will have the same bottlenecks in Codon.

### Codon is slower than Python for my application.

It is worth first checking that the measurement uses release mode, excludes
compilation when appropriate, and includes comparable inputs and results.
The [performance guidance](usage.md#measuring-performance) discusses common
measurement boundaries. If a representative workload remains unexpectedly
slower, a small reproducer, version information and timings on the
[issue tracker](https://github.com/exaloop/codon/issues) help us investigate.

## Usage

### Is Codon free and open source?

Yes, Codon is free and open source under the [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0).
Exaloop offers enterprise and custom solutions on top of Codon for a variety of applications, use cases and
industries; please email [info@exaloop.io](mailto:info@exaloop.io) to learn more.

## Contributing

### Does Codon accept outside contributions?

Absolutely, we'd be delighted to accept any contributions in the form of issues, bug reports,
feature requests or pull requests.

### I want to contribute. Where do I start?

If you have a specific feature or use case in mind, here is a quick breakdown of the codebase
to help provide a sense of where to look first:

- [`codon/`](https://github.com/exaloop/codon/tree/develop/codon): compiler code
  - [`codon/parser/`](https://github.com/exaloop/codon/tree/develop/codon/parser):
    parser and type checker code: this is the first step of compilation
  - [`codon/cir/`](https://github.com/exaloop/codon/tree/develop/codon/cir):
    Codon IR and optimizations: the second step of compilation
  - [`codon/cir/llvm/`](https://github.com/exaloop/codon/tree/develop/codon/cir/llvm):
    conversion from Codon IR to LLVM IR and machine code: the last step of compilation
  - [`codon/runtime/`](https://github.com/exaloop/codon/tree/develop/codon/runtime):
    runtime library: used during execution
- [`stdlib/`](https://github.com/exaloop/codon/tree/develop/stdlib): standard library code

You can also take a look at some of the [open issues](https://github.com/exaloop/codon/issues). If you
have any question or suggestions, please feel free to ask in [the forum](https://github.com/exaloop/codon/discussions).

### Is there a Contributor License Agreement (CLA)?

Yes, there is a CLA that is required to be agreed to before any pull requests are merged.
Please see [exaloop.io/legal/cla](https://exaloop.io/legal/cla) for more information. To agree to
the CLA, send an email with your GitHub username to [info@exaloop.io](mailto:info@exaloop.io).
