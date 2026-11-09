# Language overview

Codon uses Python's syntax and familiar programming constructs, including functions, classes, comprehensions, generators and exceptions. The main difference is how a program runs: Codon determines native types before execution and compiles operations on those types to machine code, rather than evaluating them through the CPython interpreter.

Type annotations are often optional because the compiler can infer types from assignments and function calls. For example, the same unannotated function can be compiled for integer and floating-point inputs:

```python
def squared_total(values):
  return sum(value * value for value in values)

print(squared_total([1, 2, 3]))        # 14
print(squared_total([1.0, 2.0, 3.0]))  # 14.0
```

Both calls use the same source definition, but operate on different native element types. The [generics guide](https://docs.exaloop.io/language/generics/) explains this specialization model in more detail, including how to describe types explicitly when an API needs a particular representation.

The rest of this section covers [functions and generators](https://docs.exaloop.io/language/functions/), [classes](https://docs.exaloop.io/language/classes/), [exceptions and resources](https://docs.exaloop.io/language/errors/), [compile-time metaprogramming](https://docs.exaloop.io/language/meta/) and [low-level facilities](https://docs.exaloop.io/language/lowlevel/). Python libraries can also be used through [Python interoperability](https://docs.exaloop.io/integrations/python/python-from-codon/), which is particularly useful when a dependency relies on dynamic Python behavior.

## Differences with Python

tip

Found something that works differently in Codon than Python which isn't mentioned below? Let us know [on GitHub](https://github.com/exaloop/codon/issues/new).

Python source often needs little adaptation, but its data representations and runtime assumptions still matter. The differences below are useful to consider when moving an existing program to Codon, especially at file formats, numerical boundaries and calls into other libraries.

### Data types

-   **Integers:** Codon's `int` is a 64-bit signed integer, whereas Python's integers can grow to arbitrary precision. `Int[N]` and `UInt[N]` provide other fixed widths; choosing a larger `N` changes the available range, but does not make an integer grow automatically. See [low-level types](https://docs.exaloop.io/language/lowlevel/) for their use and C equivalents.
    
-   **Tuples:** Ordinary tuples have a fixed number of fields whose types are known at compile time. Python-style `tuple(iterable)` conversion is not generally supported; a list is a more suitable representation when a sequence's length is determined at runtime.
    
-   **Arrays**: Codon includes a native NumPy implementation with a corresponding `ndarray` type. Codon's `ndarray` is parameterized by the data type (`dtype`) and dimension (`ndim`). In practice, this almost never affects NumPy code as these parameters are determined automatically at compile time. In some cases, such as when reading array data from disk, they must be provided programmatically. Learn more in the [Codon-NumPy docs](https://docs.exaloop.io/libraries/numpy/).
    

### Type checking

Codon needs to determine the type of each expression before execution. A function can be generic, as in the example above, but each compiled specialization still has known argument and return types. An annotation can help express an interface or resolve an otherwise ambiguous empty collection:

```python
counts: dict[str, int] = {}
for name in ["oak", "pine", "oak"]:
  counts[name] = counts.get(name, 0) + 1

print(counts["oak"])  # 2
```

Runtime changes to a class's fields or methods do not fit this compilation model. When an application requires that kind of dynamism, the affected code can remain in Python and be called through the interoperability layer. For native code, [generic types](https://docs.exaloop.io/language/generics/) and compile-time specialization offer ways to share implementations without changing their layout at runtime.

### Numerics

Python-style numerical semantics are the default. For example, integer floor division rounds down, and division by zero raises an exception. Programs that can work with C-style behavior may opt into `-numerics=c`, which changes operations such as signed division and some numerical error checks in exchange for additional optimization opportunities.

This choice is independent of `-debug` and `-release`; enabling optimization does not by itself select C-style numerics. Neither numerical mode changes the fixed width of `int`. The [usage guide](https://docs.exaloop.io/start/usage/#numerical-semantics) shows examples of the differences.

### Modules

While most of the commonly used builtin modules have Codon-native implementations, a few are not yet implemented. However these can still be used within Codon [via `from python import`](https://docs.exaloop.io/integrations/python/python-from-codon/).

Source: https://docs.exaloop.io/language/overview/
