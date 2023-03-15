# Codon benchmark suite

This folder contains a number of Codon benchmarks. Some are taken
from the [pyperformance suite](https://github.com/python/pyperformance)
while others are adaptations of applications we've encountered in the
wild. Further, some of the benchmarks are identical in both Python and
Codon, some are changed slightly to work with Codon's type system, and
some use Codon-specific features like parallelism or GPU.

Some of the pyperformance benchmarks can be made (much) faster in Codon
by using various Codon-specific features, but their adaptations here are
virtually identical to the original implementations (mainly just the use
of the `pyperf` module is removed).

## Setup

The `bench.sh` script can be used to run all the benchmarks and output a
CSV file with the results. The benchmark script looks at the following
environment variables:

- `EXE_PYTHON`: Python command (default: `python3`)
- `EXE_PYPY`: PyPy command (default: `pypy3`)
- `EXE_CPP`: C++ compiler command (default: `clang++`; run with `-std=c++17 -O3`)
- `EXE_CODON`: Codon command (default: `build/codon`; run with `-release`)

Some benchmarks also require specific environment variables to be set
for accessing data (details below).

## Benchmarks

- `chaos`: [Pyperformance's `chaos` benchmark](https://github.com/python/pyperformance/blob/main/pyperformance/data-files/benchmarks/bm_chaos/run_benchmark.py).
- `float`: [Pyperformance's `float` benchmark](https://github.com/python/pyperformance/blob/main/pyperformance/data-files/benchmarks/bm_float/run_benchmark.py).
- `go`: [Pyperformance's `go` benchmark](https://github.com/python/pyperformance/blob/main/pyperformance/data-files/benchmarks/bm_go/run_benchmark.py).
- `nbody`: [Pyperformance's `nbody` benchmark](https://github.com/python/pyperformance/blob/main/pyperformance/data-files/benchmarks/bm_nbody/run_benchmark.py).
- `spectral_norm`: [Pyperformance's `spectral_norm` benchmark](https://github.com/python/pyperformance/blob/main/pyperformance/data-files/benchmarks/bm_spectral_norm/run_benchmark.py).
- `mandelbrot`: Generates an image of the Mandelbrot set. Codon version uses GPU via one additional `@par(gpu=True, collapse=2)` line.
- `set_partition`: Calculates set partitions. Code taken from [this Stack Overflow answer](https://stackoverflow.com/a/73549333).
- `sum`: Computes sum of integers from 1 to 50000000 with a loop. Code taken from [this article](https://towardsdatascience.com/getting-started-with-pypy-ef4ba5cb431c).
- `taq`: Performs volume peak detection on an NYSE TAQ file. Sample TAQ files can be downloaded and uncompressed from [here](https://ftp.nyse.com/Historical%20Data%20Samples/DAILY%20TAQ/)
         (e.g. `EQY_US_ALL_NBBO_20220705.gz`). We recommend using the first 10M lines for benchmarking purposes. The TAQ file path should be passed to the benchmark script
         through the `DATA_TAQ` environment variable.
- `binary_trees`: [Boehm's binary trees benchmark](https://hboehm.info/gc/gc_bench.html).
- `fannkuch`: See [*Performing Lisp analysis of the FANNKUCH benchmark*](https://dl.acm.org/doi/10.1145/382109.382124) by Kenneth R. Anderson and Duane Rettig. Benchmark
              involves generating permutations and repeatedly reversing elements of a list. Codon version is multithreaded with a dynamic schedule via one additional
              `@par(schedule='dynamic')` line.
- `word_count`: Counts occurrences of words in a file using a dictionary. The file should be passed to the benchmark script through the `DATA_WORD_COUNT` environment variable.
- `primes`: Counts the number of prime numbers below a threshold. Codon version is multithreaded with a dynamic schedule via one additional `@par(schedule='dynamic')` line.
