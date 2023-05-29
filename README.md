<p align="center">
 <img src="docs/img/codon.png?raw=true" width="600" alt="Codon"/>
</p>

<h3 align="center">
  <a href="https://docs.exaloop.io/codon" target="_blank"><b>Docs</b></a>
  &nbsp;&#65372;&nbsp;
  <a href="https://docs.exaloop.io/codon/general/faq" target="_blank"><b>FAQ</b></a>
  &nbsp;&#65372;&nbsp;
  <a href="https://blog.exaloop.io" target="_blank"><b>Blog</b></a>
  &nbsp;&#65372;&nbsp;
  <a href="https://github.com/exaloop/codon/discussions" target="_blank"><b>Forum</b></a>
  &nbsp;&#65372;&nbsp;
  <a href="https://join.slack.com/t/exaloop/shared_invite/zt-1jusa4kc0-T3rRWrrHDk_iZ1dMS8s0JQ" target="_blank">Chat</a>
  &nbsp;&#65372;&nbsp;
  <a href="https://exaloop.io/benchmarks" target="_blank">Benchmarks</a>
</h3>

<a href="https://github.com/exaloop/codon/actions/workflows/ci.yml">
  <img src="https://github.com/exaloop/codon/actions/workflows/ci.yml/badge.svg"
       alt="Build Status">
</a>

## What is Codon?

Codon is a high-performance compiler that compiles Python and Python-like code to native machine code with minimal overhead.

Typical speedups over Python are on the order of 10-100x or more, on a single thread. Codon's performance is typically on par with
(and sometimes better than) that of C/C++. Unlike Python, Codon supports native multithreading, which can lead to speedups many
times higher still.

### Goals

Think of Codon as a Python reimagined for _statical compilation_ completely from scratch. The goals of Codon are:

- Complete support of Python's syntax (extensions are allowed)
- Semantics as close as Python's (not 100\% identical, but close enough for most common use-cases)
- Top-notch performance and easy implementation of compile-time optimizations for new domains
     (something that libraries in any mainstream language cannot do).
- Seamless interoperability with CPython, C/C++ and/or other languages.

The perfomance is achieved by the following design choices:

- Static ahead-of-time type-checking with minimal reliance on type annotations.
- Static instantiation of types and functions.
- Compile-time expressions, statements and branches.
- Lightweight object representation (as close to C as possible).
- Compile-time elision of any metadata that will not be needed.
- Aggressive compile-time optimizations whenever possible.

Codon stems from the scientific computing environment: its percursor was [Seq project](https://github.com/seq-lang/seq),
a DSL for bioinformatics where every saved CPU cycle counts.

For more information, please consult [Differences with Python]().

### Where are you at right now?

Long answer—please see [Roadmap]() for details.

### Why?

Python is arguably the world's programming language: it is [most widely taught](), [used]() and is
widely popular among non-CS oriented communities. It provides clean (and analyzable) syntax, [simple semantics],
and has unmatched library coverage. However, its Achilee's heel was (and still is) the performance: typicall pure Python
code is many orders of magnitude slower than its C/C++/Rust counterpart.

Most of the performance hit comes from the extreme flexibility of its semantics, as well from legacy considerations.
However, this flexiblity is often not needed and is typically not used (or even known) in many contexts. Thus, we can
often get rid of it to achieve large performance gains. When and how? That's where Codon kicks in! We aim to combine
[modern compiler techniques](cite) with [Python syntax and semantics]() to get the best of both worlds whenever possible.

So, TL;DR:

    - We want Python's syntax, semantics and ease of use (we don't want you to learn yet another language)
    - We want to be as close to bare metal as possible (speeeeed!)
    - We want compiler to help us optimize and detect as many bugs as possible ahead-of-time

While there are many amazing attempts to imrpove Python's performance (e.g., [PyPy], new CPython, Numba, Mojo, to name a few), nearly all of them are limited by either legacy constraints, limited scope, or a commitment to the absolute 100\%
semantical compatibility with (C)Python. Codon took a different approach: we started with a small compiler that targeted
limited subset of Python, and will keep expanding it until the gap is small enough not to matter.

## Install

Pre-built binaries for Linux (x86_64) and macOS (x86_64 and arm64) are available alongside [each release](https://github.com/exaloop/codon/releases).
Download and install with:

```bash
/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
```

Or you can [build from source](https://docs.exaloop.io/codon/advanced/build).

## Examples

Codon is a Python-compatible language, and many Python programs will work with few if any modifications:

```python
def fib(n):
    a, b = 0, 1
    while a < n:
        print(a, end=' ')
        a, b = b, a+b
    print()
fib(1000)
```

The `codon` compiler has a number of options and modes:

```bash
# compile and run the program
codon run fib.py
# 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987

# compile and run the program with optimizations enabled
codon run -release fib.py
# 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987

# compile to executable with optimizations enabled
codon build -release -exe fib.py
./fib
# 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987

# compile to LLVM IR file with optimizations enabled
codon build -release -llvm fib.py
# outputs file fib.ll
```

See [the docs](https://docs.exaloop.io/codon/general/intro) for more options and examples.

This prime counting example showcases Codon's [OpenMP](https://www.openmp.org/) support, enabled with the addition of one line.
The `@par` annotation tells the compiler to parallelize the following `for`-loop, in this case using a dynamic schedule, chunk size
of 100, and 16 threads.

```python
from sys import argv

def is_prime(n):
    factors = 0
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0

limit = int(argv[1])
total = 0

@par(schedule='dynamic', chunk_size=100, num_threads=16)
for i in range(2, limit):
    if is_prime(i):
        total += 1

print(total)
```

Codon supports writing and executing GPU kernels. Here's an example that computes the
[Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set):

```python
import gpu

MAX    = 1000  # maximum Mandelbrot iterations
N      = 4096  # width and height of image
pixels = [0 for _ in range(N * N)]

def scale(x, a, b):
    return a + (x/N)*(b - a)

@gpu.kernel
def mandelbrot(pixels):
    idx = (gpu.block.x * gpu.block.dim.x) + gpu.thread.x
    i, j = divmod(idx, N)
    c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
    z = 0j
    iteration = 0

    while abs(z) <= 2 and iteration < MAX:
        z = z**2 + c
        iteration += 1

    pixels[idx] = int(255 * iteration/MAX)

mandelbrot(pixels, grid=(N*N)//1024, block=1024)
```

GPU programming can also be done using the `@par` syntax with `@par(gpu=True)`.

## What isn't Codon?

While Codon supports nearly all of Python's syntax, it is not a drop-in replacement, and large codebases might require modifications
to be run through the Codon compiler. For example, some of Python's modules are not yet implemented within Codon, and a few of Python's
dynamic features are disallowed. The Codon compiler produces detailed error messages to help identify and resolve any incompatibilities.

Codon can be used within larger Python codebases via the [`@codon.jit` decorator](https://docs.exaloop.io/codon/interoperability/decorator).
Plain Python functions and libraries can also be called from within Codon via
[Python interoperability](https://docs.exaloop.io/codon/interoperability/python).

## Documentation

Please see [docs.exaloop.io](https://docs.exaloop.io/codon) for in-depth documentation.
