<p align="center">
 <img src="docs/sphinx/logo.png?raw=true" width="200" alt="Seq"/>
</p>

<h1 align="center"> Seq — a language for bioinformatics</h1>

<p align="center">
  <a href="https://github.com/seq-lang/seq/actions?query=branch%3Adevelop">
    <img src="https://github.com/seq-lang/seq/workflows/Seq%20CI/badge.svg?branch=develop"
         alt="Build Status">
  </a>
  <a href="https://gitter.im/seq-lang/seq?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge">
    <img src="https://badges.gitter.im/Join%20Chat.svg"
         alt="Gitter">
  </a>
  <a href="https://github.com/seq-lang/seq/releases/latest">
    <img src="https://img.shields.io/github/v/release/seq-lang/seq?sort=semver"
         alt="Version">
  </a>
  <a href="https://github.com/seq-lang/seq/blob/master/LICENSE">
    <img src="https://img.shields.io/github/license/seq-lang/seq"
         alt="License">
  </a>
</p>

## Introduction

> **A strongly-typed and statically-compiled high-performance Pythonic language!**

Seq is a programming language for computational genomics and bioinformatics. With a Python-compatible syntax and a host of domain-specific features and optimizations, Seq makes writing high-performance genomics software as easy as writing Python code, and achieves performance comparable to (and in many cases better than) C/C++.

**Think of Seq as a strongly-typed and statically-compiled Python: all the bells and whistles of Python, boosted with a strong type system, without any performance overhead.**

Seq is able to outperform Python code by up to 160x. Seq can further beat equivalent C/C++ code by up to 2x without any manual interventions, and also natively supports parallelism out of the box. Implementation details and benchmarks are discussed [in our paper](https://dl.acm.org/citation.cfm?id=3360551).

Learn more by following the [tutorial](https://docs.seq-lang.org/tutorial) or from the [cookbook](https://docs.seq-lang.org/cookbook).

## Examples

Seq is a Python-compatible language, and many Python programs should work with few if any modifications:

```python
def fib(n):
    a, b = 0, 1
    while a < n:
        print(a, end=' ')
        a, b = b, a+b
    print()
fib(1000)
```

This prime counting example showcases Seq's [OpenMP](https://www.openmp.org/) support, enabled with the addition of one line. The `@par` annotation tells the compiler to parallelize the following for-loop, in this case using a dynamic schedule, chunk size of 100, and 16 threads.

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

Here is an example showcasing some of Seq's bioinformatics features, which include native sequence and k-mer types.

```python
from bio import *
s = s'ACGTACGT'     # sequence literal
print(s[2:5])       # subsequence
print(~s)           # reverse complement
kmer = Kmer[8](s)   # convert to k-mer

# iterate over length-3 subsequences
# with step 2
for sub in s.split(3, step=2):
    print(sub[-1])  # last base

    # iterate over 2-mers with step 1
    for kmer in sub.kmers(step=1, k=2):
        print(~kmer)  # '~' also works on k-mers
```

## Install

### Pre-built binaries

Pre-built binaries for Linux and macOS on x86_64 are available alongside [each release](https://github.com/seq-lang/seq/releases). We also have a script for downloading and installing pre-built versions:

```bash
/bin/bash -c "$(curl -fsSL https://seq-lang.org/install.sh)"
```

### Build from source

See [Building from Source](docs/sphinx/build.rst).

## Documentation

Please check [docs.seq-lang.org](https://docs.seq-lang.org) for in-depth documentation.

## Citing Seq

If you use Seq in your research, please cite:

> Ariya Shajii, Ibrahim Numanagić, Riyadh Baghdadi, Bonnie Berger, and Saman Amarasinghe. 2019. Seq: a high-performance language for bioinformatics. *Proc. ACM Program. Lang.* 3, OOPSLA, Article 125 (October 2019), 29 pages. DOI: https://doi.org/10.1145/3360551

BibTeX:

```
@article{Shajii:2019:SHL:3366395.3360551,
 author = {Shajii, Ariya and Numanagi\'{c}, Ibrahim and Baghdadi, Riyadh and Berger, Bonnie and Amarasinghe, Saman},
 title = {Seq: A High-performance Language for Bioinformatics},
 journal = {Proc. ACM Program. Lang.},
 issue_date = {October 2019},
 volume = {3},
 number = {OOPSLA},
 month = oct,
 year = {2019},
 issn = {2475-1421},
 pages = {125:1--125:29},
 articleno = {125},
 numpages = {29},
 url = {http://doi.acm.org/10.1145/3360551},
 doi = {10.1145/3360551},
 acmid = {3360551},
 publisher = {ACM},
 address = {New York, NY, USA},
 keywords = {Python, bioinformatics, computational biology, domain-specific language, optimization, programming language},
}
```
