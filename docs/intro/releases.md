Below you can find release notes for each major Codon release,
listing improvements, updates, optimizations and more for each
new version.

# v0.13

## Language

### Scoping

Scoping was changed to match Python scoping. For example:

``` python
if condition:
    x = 42

print(x)
```

If condition is `False`, referencing `x` causes a `NameError`
to be raised at runtime, much like what happens in Python.
There is zero new performance overhead for code using the old
scoping; code using the new scoping as above generates a flag to
indicate whether the given variable has been assigned.

Moreover, variables can now be assigned to different types:

``` python
x = 42
print(x)  # 42
x = 'hello'
print(x)  # hello
```

The same applies in Jupyter or JIT environments.

### Static methods

Added support for `@staticmethod` method decorator.
Class variables are also supported:

``` python
class Cls:
    a = 5  # or "a: ClassVar[int] = 5" (PEP 526)

    @staticmethod
    def method():
        print('hello world')

c = Cls()
Cls.a, Cls.method(), c.a, c.method()  # supported
```

### Tuple handling

Arbitrary classes can now be converted to tuples via the `tuple()`
function.

### Void type

The `void` type has been completely removed in favor of the new
and Pythonic `NoneType`, which compiles to an empty LLVM struct.
This does not affect C interoperability as the empty struct type
is replaced by `void` by LLVM.

### Standard library

The `re` module is now fully supported, and uses
[Google's `re2`](https://github.com/google/re2) as a backend. Future
versions of Codon will also include an additional regex optimization
pass to compile constant ("known at compile time") regular expressions
to native code.

## C variables

Global variables with C linkage can now be imported via `from C import`:

``` python
# assumes the C variable "long foo"
from C import foo: int
print(foo)
```

## Parallelism

Numerous improvements to the OpenMP backend, including the addition
of task-based reductions:

``` python
total = 0
@par
for a in some_arbitrary_generator():
    total += do_work(a)  # now converted to task reduction
```

## Python interoperability

Included revamped `codon` module for Python, with `@codon.jit` decorator
for compiling Python code in existing codebases. Further improved and
optimized the Python bridge. Please see the [docs](../interop/decorator.md)
for more information.

## Codon IR

New capture analysis pass for Codon IR for improving tasks such as dead
code elimination and side effect analysis. This allows Codon IR to deduce
whether arbitrary, compilable Python expressions have side effects, capture
variables, and more.

## Code generation and optimizations

A new dynamic allocation optimization pass is included, which 1)
removes unused allocations (e.g. instantiating a class but never
using it) and 2) demotes small heap allocations to stack (`alloca`)
allocations when possible. The latter optimization can frequently
remove any overhead associated with instantiating most classes.

## Command-line tool

The `codon` binary can now compile to shared libraries using the `-lib`
option to `codon build` (or it can be deduced from a `.so` or `.dylib`
extension on the output file name).

## Errors

Added support for multiple error reporting.
