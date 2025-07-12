Codon implements much of Python's standard library natively.
Some built-in modules and some methods of certain modules are
not yet available natively in Codon; these
[can still be called through Python](/integrations/python/python-from-codon),
however:

``` python
import sys              # uses Codon's native 'sys' module
from python import sys  # uses Python's 'sys' module
```

## Built-in modules

The following built-in modules are supported either in full
or in part natively in Codon:

| Module        | Notes |
| ------------- | - |
| `copy`        ||
| `gzip`        ||
| `random`      | Matches CPython's `random` outputs for same seed. |
| `threading`   | Locks work with Codon's parallel programming features. |
| `bisect`      ||
| `datetime`    | `timedelta` are represented in microseconds. Time zones not supported. |
| `heapq`       ||
| `operator`    ||
| `re`          | Uses [Google's RE2 library](https://github.com/google/re2) internally. |
| `time`        ||
| `bz2`         ||
| `os`          ||
| `cmath`       ||
| `functools`   ||
| `itertools`   ||
| `statistics`  ||
| `typing`      | Contents are available by default in Codon. |
| `getopt`      ||
| `math`        ||
| `pickle`      | Codon uses its own pickle format, so generally not compatible with CPython pickling. |
| `string`      ||
| `collections` ||
| `sys`         ||

## Additional modules

Alongside the standard modules above, Codon provides several additional
modules that support various Codon-specific features.

- `openmp`: Contains [OpenMP](https://openmp.org) API, which can be used when
  writing multithreaded programs. See [multithreading](/parallel/multithreading) for
  more information.
- `gpu`: Contains GPU API (e.g. CUDA intrinsics), which can be used when writing
  GPU code. See [GPU](/parallel/gpu) for more information.
- `python`: Contains internal machinery for interfacing with CPython. Most users will
  not need to interact with this module directly.
- `experimental`: Contains experimental features that are available for use, but might
  not be stable nor complete.
