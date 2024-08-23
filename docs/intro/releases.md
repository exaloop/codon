Below you can find release notes for each major Codon release,
listing improvements, updates, optimizations and more for each
new version.

These release notes generally do not include small bug fixes. See the
[closed issues](https://github.com/exaloop/codon/issues?q=is%3Aissue+is%3Aclosed)
for more information.

# v0.17

## LLVM upgrade

Upgraded to LLVM 17 (from 15).

## Standard library updates

- New floating-point types `float16`, `bfloat16` and `float128`.
- Updates to several existing functions, such as adding `key` and
  `default` arguments to `min()` and `max()`.
- Slice arguments can now be of any type, not just `int`.
- Added `input()` function.

## Other improvements

- Property setters are now supported.
- Updated import logic to match CPython's more closely.
- Several improvements to dynamic polymorphism to match CPython more
  closely.

## New compiler options

- `-disable-exceptions` will disable exceptions, potentially eliding
  various runtime checks (e.g. bounds checks for lists). This flag
  should only be used if you know that no exceptions will be raised
  in the given program.

# v0.16

## Python extensions

A new build mode is added to `codon` called `pyext` which compiles
to Python extension modules, allowing Codon code to be imported and
called directly from Python (similar to Cython). Please see the
[docs](../interop/pyext.md) for more information and usage examples.

## Standard library updates

- Various additions to the standard library, such as `math.fsum()` and
  the built-in `pow()`.

- Added `complex64`, which is a complex number with 32-bit float real and
  imaginary components.

- Better `Int[N]` and `UInt[N]` support: can now convert ints wider than
  64-bit to string; now supports more operators.

## More Python-specific optimizations

New optimizations for specific patterns including `any()`/`all()` and
multiple list concatenations. These patterns are now recognized and
optimized in Codon's IR.

## Static expressions

Codon now supports more compile-time static functions, such as `staticenumerate`.

# v0.15

## Union types

Codon adds support for union types (e.g., `Union[int, float]`):

```
def foo(cmd) -> Union:
    if cmd == 'int': return 1
    else: return "s"
foo('int')        # type is Union[int,str]
5 + foo('int')    # 6
'a' + foo('str')  # as
```

## Dynamic inheritance

Dynamic inheritance and polymorphism are now supported:

```
class A:
    def __repr__(): return 'A'
class B(A):
    def __repr__(): return 'B'
l = [A(), B(), A()]  # type of l is List[A]
print(l)  # [A, B, A]
```

This feature is still a work in progress.

## LLVM upgrade

Upgraded to LLVM 15 (from 12). Note that LLVM 15 now uses
[opaque pointers](https://llvm.org/docs/OpaquePointers.html),
e.g. `ptr` instead of `i8*` or `i64*`, which affects `@llvm`
functions written in Codon as well as LLVM IR output of
`codon build`.

## Standard library

`random` module now matches Python exactly for the same seed.

# v0.14

## GPU support

GPU kernels can now be written and called in Codon. Existing
loops can be parallelized on the GPU with the `@par(gpu=True)`
annotation. Please see the [docs](../advanced/gpu.md) for
more information and examples.

## Semantics

Added `-numerics` flag, which specifies semantics of various
numeric operations:

- `-numerics=c` (default): C semantics; best performance
- `-numerics=py`: Python semantics (checks for zero divisors
  and raises `ZeroDivisionError`, and adds domain checks to `math`
  functions); might slightly decrease performance.

## Types

Added `float32` type to represent 32-bit floats (equivalent to C's
`float`). All `math` functions now have `float32` overloads.

## Parallelism

Added `collapse` option to `@par`:

``` python
@par(collapse=2)  # parallelize entire iteration space of 2 loops
for i in range(N):
    for j in range(N):
        do_work(i, j)
```

## Standard library

Added `collections.defaultdict`.

## Python interoperability

Various Python interoperability improvements: can now use `isinstance`
on Python objects/types and can now catch Python exceptions by name.

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
