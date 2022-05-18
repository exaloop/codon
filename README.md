<p align="center">
 <img src="docs/sphinx/codon.png?raw=true" width="600" alt="Codon"/>
</p>

<p align="center">
  <a href="https://github.com/exaloop/codon/actions/workflows/ci.yml">
    <img src="https://github.com/exaloop/codon/actions/workflows/ci.yml/badge.svg"
         alt="Build Status">
  </a>
  <a href="https://discord.com/invite/8aKr6HEN?utm_source=Discord%20Widget&utm_medium=Connect">
    <img src="https://img.shields.io/discord/895056805846192139.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2"
         alt="Discord">
  </a>
</p>

## What is Codon?

> *Think of Codon as a strongly-typed and statically-compiled Python: all the bells and whistles of Python,
   boosted with a strong type system, without any performance overhead.*

Codon is a high-performance Python compiler that compiles Python code to native machine code without any runtime overhead.
Typical speedups over Python are on the order of 10-100x or more, on a single thread. Codon's performance is typically on par with
(and in many cases better than) that of C/C++. Unlike Python, Codon supports native multithreading, which can lead to speedups many
times higher still.

Codon is extensible via a plugin infrastructure. For example, [Seq](https://github.com/seq-lang/seq) is a domain-specific
language for genomics and bioinformatics, built on Codon, that can outperform hand-optimized C code by 2-10x (more details in
the [Seq paper](https://www.nature.com/articles/s41587-021-00985-6)).

## What isn't Codon?

While Codon supports nearly all of Python's syntax, it is not a drop-in replacement, and large codebases might require modifications
to be run through the Codon compiler. For example, some of Python's modules are not yet implemented within Codon, and a few of Python's
dynamic features are disallowed. The Codon compiler produces detailed error messages to help identify and resolve any incompatibilities.

Codon supports seamless Python interoperability to handle cases where specific Python libraries or dynamism are required:

```python
@python
def hello():
    import sys
    print('Hello from Python!')
    print('The version is', sys.version)

hello()
# Hello from Python!
# The version is 3.9.6 (default, Jun 29 2021, 06:20:32)
# [Clang 12.0.0 (clang-1200.0.32.29)]
```

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

## Install

### Pre-built binaries

Pre-built binaries for Linux and macOS on x86_64 are available alongside [each release](https://github.com/exaloop/codon/releases).
We also have a script for downloading and installing pre-built versions:

```bash
/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
```

### Build from source

See [Building from Source](docs/advanced/build.md).

## Documentation

Please see [docs.exaloop.io](https://docs.exaloop.io/codon) for in-depth documentation.
