# Standard library

Codon implements much of Python's standard library natively, so ordinary imports become part of the compiled program. A Python import uses the CPython implementation instead, which can be helpful when a required module or API is not available natively:

```python
import sys                       # Codon's native module
from python import sys as pysys  # CPython's module
```

The two imports can coexist under different names. Python imports require the [Python runtime setup](https://docs.exaloop.io/integrations/python/python-from-codon/); an ordinary native import does not make the program depend on CPython.

## Native modules

The table below summarizes the standard-library modules available natively. Coverage varies by module, so the [API reference](https://docs.exaloop.io/libraries/api/) is the place to check individual functions, signatures and compatibility notes.

| Module | Coverage and notable differences |
| --- | --- |
| `argparse` | Command-line parsing with tagged namespace values. |
| `asyncio` | Coroutines, tasks, futures and event-loop scheduling. See the example below. |
| `bisect` | Binary searches and insertion into sorted sequences. |
| `bz2`, `gzip` | Compressed-file I/O. |
| `cmath`, `math` | Complex and real-valued mathematical functions. |
| `collections` | Container types and helpers. |
| `copy` | Shallow and deep copies. |
| `datetime` | Dates, times and microsecond-resolution timedeltas; timezone-aware objects are not supported. |
| `enum` | Class-based enumerations; see [enumerations](https://docs.exaloop.io/language/classes/#enumerations) for supported declarations. |
| `functools` | Function-oriented helpers. |
| `getopt` | Command-line option parsing. |
| `heapq` | Heap operations and priority queues. |
| `itertools` | Iterator construction and composition. |
| `json` | Parsing and encoding with tagged native `JSON` values. |
| `operator` | Function forms of operators. |
| `os` | Environment, filesystem, process and descriptor operations. |
| `pathlib` | `Path` objects, filesystem operations, globbing and traversal. |
| `pickle` | Codon's own serialization format, rather than a CPython-compatible interchange format. |
| `random` | Pseudorandom generators and sampling. |
| `re` | Uses [RE2](https://github.com/google/re2); constructs such as backreferences and lookaround are not supported. |
| `statistics` | Descriptive statistics. |
| `string` | String constants and utilities. |
| `sys` | Native streams, encoding queries and Codon implementation metadata. |
| `threading` | Synchronization primitives that work with Codon's parallel programming features. |
| `time` | Clocks, sleeping and time conversion. |
| `typing` | Type annotations, many of which are available without an explicit import. |
| `unicodedata` | Unicode properties and normalization. |
| `unittest` | Test cases and assertions; consult the API reference for supported helpers. |

## Reading JSON data

JSON documents can contain values of different types, even within a single array. Codon's `json.loads()` represents them with a common `JSON` type and provides accessors for extracting native values:

```python
import json

document = json.loads('{"name": "run-1", "samples": [2, 3, 5]}')
name = document["name"].as_str()
samples = [item.as_int() for item in document["samples"].values()]

print(name, sum(samples))  # run-1 10
document["total"] = sum(samples)
print(document["total"].as_int())  # 10
```

Here, `samples` is a native `list[int]`, so subsequent calculations use native integer operations. An accessor raises `TypeError` if the value has an incompatible kind; when a document permits several kinds, `is_null()`, `is_number()` or the `kind` property can help select the appropriate conversion. Arrays and objects expose `values()`, `keys()` and `items()` as applicable, rather than using direct iteration over a `JSON` value.

`json.dumps()` accepts these values as well as supported native scalars and containers. Applications that depend on CPython-specific decoder hooks can use `from python import json` for that part of the program.

## Scheduling asynchronous work

Native coroutines use the familiar `async def` and `await` syntax. An event loop coordinates them, allowing one coroutine to make progress while another is suspended:

```python
import asyncio

async def delayed_label(label: str, delay: float):
  await asyncio.sleep(delay)
  return label

async def main():
  labels = await asyncio.gather(
    delayed_label("first", 0.01),
    delayed_label("second", 0.0),
  )
  for label in labels:
    print(label)

asyncio.run(main())
# first
# second
```

Although the second coroutine can finish first, `gather()` returns results in argument order. `asyncio.run()` manages the event loop for the top-level coroutine. This is cooperative concurrency: a long-running synchronous calculation still occupies the event-loop thread. For CPU work that should run across cores, [multithreading](https://docs.exaloop.io/parallel/multithreading/) is a better fit.

## Additional modules

Alongside the standard modules above, Codon provides several additional modules that support various Codon-specific features.

-   `openmp`: Contains [OpenMP](https://openmp.org/) API, which can be used when writing multithreaded programs. See [multithreading](https://docs.exaloop.io/parallel/multithreading/) for more information.
-   `gpu`: Contains GPU API (e.g. CUDA intrinsics), which can be used when writing GPU code. See [GPU](https://docs.exaloop.io/parallel/gpu/) for more information.
-   `python`: Contains internal machinery for interfacing with CPython. Most users will not need to interact with this module directly.
-   `codon`: Provides Codon-specific facilities, including [`codon.static`](https://docs.exaloop.io/language/meta/) for compile-time operations.
-   `simd`: Provides [SIMD vector types and operations](https://docs.exaloop.io/parallel/simd/).
-   `sortedlist`: Provides a sorted-list container.

Source: https://docs.exaloop.io/libraries/stdlib/
