<p align="center">
 <img src="docs/img/codon.png?raw=true" width="600" alt="Codon"/>
</p>

<h3 align="center">
  <a href="https://docs.exaloop.io/codon" target="_blank"><b>Docs</b></a>
  &nbsp;&vert;&nbsp;
  <a href="https://docs.exaloop.io/codon/general/faq" target="_blank"><b>FAQ</b></a>
  &nbsp;&vert;&nbsp;
  <a href="https://discourse.exaloop.io" target="_blank"><b>Discourse</b></a>
  &nbsp;&vert;&nbsp;
  <a href="https://blog.exaloop.io" target="_blank"><b>Blog</b></a>
  &nbsp;&vert;&nbsp;
  <a href="https://github.com/exaloop/codon/releases" target="_blank"><b>Releases</b></a>
</h3>

<a href="https://github.com/exaloop/codon/actions/workflows/ci.yml">
  <img src="https://github.com/exaloop/codon/actions/workflows/ci.yml/badge.svg"
       alt="Build Status">
</a>

## What is Codon?

Codon is a high-performance Python compiler that compiles Python code to native machine code without any runtime overhead.
Typical speedups over Python are on the order of 10-100x or more, on a single thread. Codon's performance is typically on par with
(and sometimes better than) that of C/C++. Unlike Python, Codon supports native multithreading, which can lead to speedups many
times higher still.

### What isn't Codon?

While Codon supports nearly all of Python's syntax, it is not a drop-in replacement, and large codebases might require modifications
to be run through the Codon compiler. For example, some of Python's modules are not yet implemented within Codon, and a few of Python's
dynamic features are disallowed. The Codon compiler produces detailed error messages to help identify and resolve any incompatibilities.

Codon can be used within larger Python codebases via the [`@codon.jit` decorator](https://docs.exaloop.io/codon/interoperability/decorator).
Plain Python functions and libraries can also be called from within Codon via
[Python interoperability](https://docs.exaloop.io/codon/interoperability/python).

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

## Install

### Pre-built binaries

Pre-built binaries for Linux and macOS on x86_64 are available alongside [each release](https://github.com/exaloop/codon/releases).
Download and install with:

```bash
/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"
```

### Build from source

Codon can be built using CMake. It is recommended to build Codon with Clang, which can be done by setting the `CC` and `CXX`
environment variables:

```bash
export CC=clang
export CXX=clang++
```

Most dependencies are built as part of the CMake build process, except for LLVM, which can be built as follows
(note that Codon uses a custom fork of LLVM 15):

```bash
git clone --depth 1 -b codon https://github.com/exaloop/llvm-project
mkdir -p llvm-project/llvm/build
cd llvm-project/llvm/build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_TERMINFO=OFF \
    -DLLVM_TARGETS_TO_BUILD=all
make
make install
```

Finally, build Codon itself with:

```bash
cd codon
mkdir build
(cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=$(llvm-config --cmakedir))
cmake --build build --config Release
```

## Documentation

Please see [docs.exaloop.io](https://docs.exaloop.io/codon) for in-depth documentation.
