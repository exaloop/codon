While Codon's syntax and semantics are virtually identical
to Python's, there are some notable differences that are
worth mentioning. Most of these design decisions were made
with the trade-off between performance and Python compatibility
in mind.

# Data types

- **Integers:** Codon's `int` is a 64-bit signed integer,
  whereas Python's (after version 3) can be arbitrarily large.
  However Codon does support larger integers via `Int[N]` where
  `N` is the bit width.

- **Strings:** Codon currently uses ASCII strings unlike
  Python's unicode strings.

- **Dictionaries:** Codon's dictionary type is not sorted
  internally, unlike Python's.

# Type checking

Since Codon performs static type checking ahead of time, a
few of Python's dynamic features are disallowed. For example,
monkey patching classes at runtime (although Codon supports a
form of this at compile time) or adding objects of different
types to a collection.

These few restrictions are ultimately what allow Codon to
compile to native code without any runtime performance overhead.
Future versions of Codon will lift some of these restrictions
by the introduction of e.g. implicit union types.

# Modules

While most of the commonly used builtin modules have Codon-native
implementations, a few are not yet implemented. However these can
still be used within Codon via `from python import`.
