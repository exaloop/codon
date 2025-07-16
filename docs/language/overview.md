Codon strives to be as close as possible to both official Python and the
[CPython](https://github.com/python/cpython/) implementation.

Outside of a few differences due to performance considerations and Codon's
static compilation paradigm, if you know Python, you already know 99% of Codon!

## Differences with Python

!!! tip

    Have you found a difference between Codon and Python that isn't mentioned
    below? Let us know [on GitHub](https://github.com/exaloop/codon/issues).

To facilitate low-level programming, parallel programming, compile-time
metaprogramming, and other features, Codon introduces the following elements
to Python.

!!! tip

    The [roadmap](../developers/roadmap.md) describes our plan to close some
    of these gaps.

### Data types

| Data Type  | Python                     | Codon                            |
|------------|----------------------------|----------------------------------|
| Integer    | Arbitrarily large integers | 64-bit signed                    |
| String     | Unicode                    | ASCII                            |
| Dictionary | Preserves insertion order  | Doesn't preserve insertion order |
| Tuple      | Arbitrary length           | Length fixed at compile time     |

### Type checking

Codon performs static type checking ahead of time, so some of Python's dynamic
features are disallowed, such as monkey patching classes at runtime or adding
objects of different types to a collection.

### Numerics

Some numeric operations use C semantics, including raising an exception when
dividing by zero and other checks done by `math` functions.  Nevertheless,
the `codon` CLI flag, `-numerics=py`, enforces Python semantics. 

### Modules

A few Python built-in modules are not yet implemented as Codon-native modules,
however, the Python built-in modules can be used within Codon via
[`from python import`](../integrations/python/python-from-codon.md).

## Differences with NumPy

Codon includes a native NumPy implementation with a corresponding `ndarray`
type. Codon's `ndarray` is parameterized by data type (`dtype`) and dimension
(`ndim`). This rarely affects NumPy code as Codon automatically determines
these parameters at compile time, but if the compile-time parameters might
differ from the run-time parameters, such as when reading an `ndarray` from
disk, the code must include the parameters. Learn more in the
[Codon-NumPy docs](../libraries/numpy.md).

