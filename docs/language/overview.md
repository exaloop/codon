Codon strives to be as close to Python (specifically, CPython) as possible,
outside of a few differences that stem from performance considerations or
incompatibilities with Codon's static compilation paradigm.

As a result, if you know Python, you already know 99% of Codon!

Codon also introduces several new elements to Python to facilitate low-level
programming, parallel programming and compile-time metaprogramming, among
other features. These elements are described in this section.

## Differences with Python

!!! tip

    Found something that works differently in Codon than Python which
    isn't mentioned below? Let us know
    [on GitHub](https://github.com/exaloop/codon/issues/new).

While Codon's syntax and semantics are nearly identical
to Python's, there are some notable differences that are
worth considering. Most of these design decisions were made
with the trade-off between performance and Python compatibility
in mind.

Please see our [roadmap](/developers/roadmap) for more information
about how we plan to close some of these gaps in the future.

### Data types

- **Integers:** Codon's `int` is a 64-bit signed integer,
  whereas Python's (after version 3) can be arbitrarily large.
  However Codon does support larger integers via `Int[N]` where
  `N` is the bit width.

- **Strings:** Codon currently uses ASCII strings unlike
  Python's unicode strings. Unicode strings are planned for
  Codon 1.0.

- **Dictionaries:** Codon's dictionary type does not preserve
  insertion order, unlike Python's as of 3.6.

- **Tuples**: Since tuples compile down to structs, tuple lengths
  must be known at compile time, meaning you can't convert an
  arbitrarily-sized list to a tuple, for instance.

- **Arrays**: Codon includes a native NumPy implementation with
  a corresponding `ndarray` type. Codon's `ndarray` is parameterized
  by the data type (`dtype`) and dimension (`ndim`). In practice,
  this almost never affects NumPy code as these parameters are
  determined automatically at compile time. In some cases, such
  as when reading array data from disk, they must be provided
  programmatically. Learn more in the [Codon-NumPy docs](/libraries/numpy).

### Type checking

Since Codon performs static type checking ahead of time, a
few of Python's dynamic features are disallowed. For example,
monkey patching classes at runtime (although Codon supports a
form of this at compile time) or adding objects of different
types to a collection.

These few restrictions are ultimately what allow Codon to
compile to native code without any runtime performance overhead.
Future versions of Codon will lift some of these restrictions
by the introduction of e.g. implicit union types.

### Numerics

For performance reasons, some numeric operations use C semantics
rather than Python semantics. This includes, for example, raising
an exception when dividing by zero, or other checks done by `math`
functions. Strict adherence to Python semantics can be enforced by
using the `-numerics=py` flag of the `codon` CLI. Note that this
does *not* change `int`s from 64-bit.

### Modules

While most of the commonly used builtin modules have Codon-native
implementations, a few are not yet implemented. However these can
still be used within Codon
[via `from python import`](/integrations/python/python-from-codon).
