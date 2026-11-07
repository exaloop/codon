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

## Refreshing the public results

`refresh.py` runs the existing 13 applications and 53 native NPBench kernels.
It stages the executed sources, inputs, compiler commands, executable hashes,
raw samples, validation outcomes, and runtime information in a work directory.
`export.py` converts completed reports into the documentation site's dataset
and downloadable source snapshots. It refuses incomplete runs and changed
artifacts as chart entries. Completed timings are included when outputs differ
or a diagnostic heap-growth threshold is exceeded; original diagnostics remain
in the raw report, with brief output-difference notes on affected benchmarks.

The November 2026 setup uses the locally built Codon 0.20.6, CPython 3.15.2,
PyPy 8.0.1 (Python 3.12.15), Clang 23.1.5, NumPy 2.5.4, SciPy 1.18.1, and
Numba 0.68.1. The machine is an Apple M1 Max with 64 GiB of RAM, running
macOS 26.6.2. These are native CPU comparisons with OpenMP and numerical-library
defaults set to one thread. Codon's Fannkuch explicitly requests four OpenMP
threads, overriding that default; the parallel Primes loop uses one thread.
These distinctions are noted on the corresponding benchmarks. The existing
CPU Mandelbrot program is used for Codon because this host has no NVIDIA GPU.
Clang uses the active macOS SDK's libc++ headers and system runtime, avoiding
an ABI mismatch between upstream LLVM headers and Apple's loaded libc++.

### Timing and validation

Applications retain their existing timing boundaries. Module-qualified Python
timers use `perf_counter`, and C++ uses `steady_clock`; existing directly imported
`time` calls retain their original wall timer. Each sample runs in a fresh process; the first process
is excluded, followed by three measured processes in rotating backend order.
Application allocation and any PyPy JIT work inside the original timer remain
included. The C++ comparison is available only where a C++ implementation
already exists. Output values are checked against CPython; the Chaos PPM uses
an exact image check. Completed timings with differing outputs are included
with a small note; the raw output-check result is not relabeled as a pass.

NPBench uses the upstream **L** preset for every kernel, not the historical
`paper` preset. Inputs are initialized once with upstream NumPy, saved with
shape, dtype, and SHA-256 metadata, and shared across all implementations.
Each worker holds one pristine and one working copy of every array. Working
arrays are reset in place with `np.copyto`; input loading, garbage collection,
reset, and output serialization are outside the kernel timer. Kernel-created
temporary allocations remain inside it. One warm-up precedes three samples
in the same process, excluding Numba specialization from recorded times.
Backend order rotates between kernels.

A separate native reset audit checks that 32 resets preserve buffer addresses,
allocate less than 64 KiB, and do not grow the GC heap. The measured process
also records heap size and live bytes after collection between full calls.
Live growth exceeding the larger of 16 MiB or 5% of input storage is recorded
as a diagnostic flag in the original measurement report. It does not exclude
completed timings from the published charts. Python and Numba record their actual BLAS
thread pools; each reported pool must use one thread.

NPBench compares every output element, rejects nonfinite values, and uses
upstream per-case tolerances (defaults: `rtol=1e-5`, `atol=1e-8`, with a
relative L2-error fallback below `1e-5`). Integer results are exact. Floating
dtype differences are recorded rather than hidden by casting. Mutated arrays
are checked even when the kernel returns `None`. The staged Numba MLP fixes
an upstream row-maximum axis error in softmax; all adaptations are recorded.
Native kernels link Apple Accelerate; NumPy and Numba use their installed BLAS
libraries. These are end-to-end software-stack comparisons, not isolated
compiler comparisons. Upstream Numba decorators, including `fastmath`, remain
unchanged.

### Reproduction

The current driver targets this macOS setup. Build the local compiler first,
then create an isolated work directory with these paths:

```text
build/benchmarks-20261106/
    venv/bin/python
    npbench/                         # https://github.com/spcl/npbench
    toolchains/LLVM-23.1.5-macOS-ARM64/bin/clang++
    toolchains/pypy3.12-v8.0.1-macos_arm64/bin/pypy3
```

The virtual environment requires the versions above plus `threadpoolctl==3.8.0`.
The saved report records the exact NPBench commit, local compiler commit,
source-diff fingerprint, runtime hashes, and SDK version. For exact
reproduction, check out that NPBench commit and use the recorded sources.
The TAQ input is the first 10 million lines of the supplied file. Word count
uses the first 16 MiB, ending at a complete line. Neither source dataset is
modified or redistributed; the report records the staged input hashes.

Compile and prepare both suites before measuring either:

```sh
WORK="$PWD/build/benchmarks-20261106"
PYTHON="$WORK/venv/bin/python"

"$PYTHON" bench/refresh.py --suite applications --prepare-only --samples 3 \
    --taq-source /path/to/taq-file --word-source /path/to/text-file \
    --output "$WORK/applications-final"
"$PYTHON" bench/refresh.py --suite npbench --preset L --prepare-only --samples 3 \
    --output "$WORK/npbench-final"

"$PYTHON" bench/refresh.py --suite applications --prepared --resume --samples 3 \
    --timeout 900 --output "$WORK/applications-final"
"$PYTHON" bench/refresh.py --suite npbench --preset L --prepared --resume --samples 3 \
    --timeout 900 --output "$WORK/npbench-final"

"$PYTHON" bench/export.py --work "$WORK"
```

Keep the machine free of builds and other benchmark runs during measurement.
The 900-second limit applies to a whole process, including warm-up and output
serialization, not to an individual recorded sample. Timed-out or incomplete
implementations are retained without a chart bar or speedup. NPBench array
artifacts are removed after each completed case to bound disk use; use
`--keep-arrays` for investigation. Reports and source snapshots are retained.
`--resume` skips completed cases. `--quick` for applications and `--preset S`
for NPBench are validation modes and are rejected by the publication exporter.
Downloadable raw reports replace machine-specific paths with `$CODON_ROOT`,
`$BENCH_WORK`, and `$HOME`; downloadable NPBench harnesses use the same path
redaction. Source downloads normalize a missing final newline; hashes of both
the original executed source and the download are retained. The exporter writes
tracked data under `docs/data/benchmarks`; the documentation asset preparation
step copies it to the generated static directory.

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
