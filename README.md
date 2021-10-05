<p align="center">
 <img src="docs/sphinx/logo.png?raw=true" width="200" alt="Codon"/>
</p>

<h1 align="center"> Codon</h1>

<p align="center">
  <a href="https://github.com/exaloop.io/codon/actions?query=branch%3Adevelop">
    <img src="https://github.com/exaloop.io/codon/workflows/Codon%20CI/badge.svg?branch=develop"
         alt="Build Status">
  </a>
  <a href="https://gitter.im/seq-lang/seq?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge">
    <img src="https://badges.gitter.im/Join%20Chat.svg"
         alt="Gitter">
  </a>
  <a href="https://github.com/exaloop.io/codon/releases/latest">
    <img src="https://img.shields.io/github/v/release/exaloop.io/codon?sort=semver"
         alt="Version">
  </a>
  <a href="https://github.com/exaloop.io/codon/blob/master/LICENSE">
    <img src="https://img.shields.io/github/license/exaloop.io/codon"
         alt="License">
  </a>
</p>

## Introduction

> **A strongly-typed and statically-compiled high-performance Pythonic language!**

Codon is a programming language for computationally intensive workloads. With a Python-compatible syntax and a host of domain-specific features and optimizations, Codon makes writing high-performance software as easy as writing Python code, and achieves performance comparable to (and in many cases better than) C/C++.

**Think of Codon as a strongly-typed and statically-compiled Python: all the bells and whistles of Python, boosted with a strong type system, without any performance overhead.**

Codon is able to outperform Python code by up to 160x. Codon can further beat equivalent C/C++ code by up to 2x without any manual interventions, and also natively supports parallelism out of the box. Implementation details and benchmarks are discussed [in our paper](https://dl.acm.org/citation.cfm?id=3360551).

Learn more by following the [tutorial](https://docs.exaloop.io/tutorial).

## Examples

Codon is a Python-compatible language, and many Python programs should work with few if any modifications:

```python
def fib(n):
    a, b = 0, 1
    while a < n:
        print(a, end=' ')
        a, b = b, a+b
    print()
fib(1000)
```

This prime counting example showcases Codon's [OpenMP](https://www.openmp.org/) support, enabled with the addition of one line. The `@par` annotation tells the compiler to parallelize the following for-loop, in this case using a dynamic schedule, chunk size of 100, and 16 threads.

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

Pre-built binaries for Linux and macOS on x86_64 are available alongside [each release](https://github.com/exaloop/codon/releases). We also have a script for downloading and installing pre-built versions:

```bash
/bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"
```

### Build from source

See [Building from Source](docs/sphinx/build.rst).

## Documentation

Please check [docs.exaloop.io](https://docs.exaloop.io) for in-depth documentation.
