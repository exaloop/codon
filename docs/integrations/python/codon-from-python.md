---
title: Call Codon from Python
---

Codon can be used within larger Python projects or codebases
through its just-in-time (JIT) compilation mode. To use Codon's
Python JIT compiler, install the `codon-jit` Python package:

```bash
python3 -m pip install codon-jit
```

This library will attempt to use an installed version of Codon.
If Codon is installed at a non-standard path, set the `CODON_DIR`
environment variable to the installation path.

## Using `@codon.jit`

The `@codon.jit` decorator compiles the annotated function for the types of
arguments supplied by the caller, converting supported Python values to
native Codon representations. A useful starting point is a function that
does a complete batch of work, so one call crosses the language boundary
instead of making a separate call for every element:

```python
import codon
from time import perf_counter

@codon.jit
def squared_total(values):
    return sum(value * value for value in values)

values = list(range(100_000))
expected = sum(value * value for value in values)

assert squared_total(values) == expected  # compile and warm up this specialization
started = perf_counter()
result = squared_total(values)
elapsed = perf_counter() - started

assert result == expected
print(result)  # 333328333350000
print(f"Call took {elapsed:.6f} seconds")
```

The first call includes compilation; later calls with the same argument types
reuse compiled code. Calling the function with a different type, such as a
list of floats, may require another specialization. The timed call still
includes argument and return-value conversion, so it measures the cost seen
by the Python caller rather than only the native loop.

:::info


`@par` (to parallelize `for`-loops) can be used in annotated functions
via a leading underscore: `_@par`.


:::

:::warning


In general, changes made to objects in a JIT'd function will not be
reflected in the host Python application, since objects passed to Codon
are *converted* to Codon-native types. If objects need to be modified,
consider returning any necessary values and performing modifications
in Python. The one notable exception to this rule is NumPy arrays, which
are not copied but instead have their data pointer passed directly to
Codon, meaning array modifications *are* visible in the host Python
application.

:::

## Type conversions

`@codon.jit` will attempt to convert any Python types that it can
to native Codon types. The current conversion rules are as follows:

- Basic types like `int`, `float`, `bool`, `str` and `complex` are
  converted to the same type in Codon.

- Tuples are converted to Codon tuples (which are then compiled
  down to the equivalent of C structs).

- Collection types like `list`, `dict` and `set` are converted to
    the corresponding Codon collection type. Element types are inferred from
    sampled contents; homogeneous inputs can use native element types, while
    mixed or otherwise unresolved contents may require Python-object elements.
    An empty collection may not provide enough information to select a concrete
    native element type.

- Other types are passed to Codon directly as Python objects.
  Codon will then use its Python object API ("`pyobj`") to handle
  and operate on these objects. Internally, this consists of calling
  the appropriate CPython C API functions, e.g. `PyNumber_Add(a, b)`
  for `a + b`.

### Custom types

User-defined classes can be converted to Codon classes via `@codon.convert`:

```python
import codon

@codon.convert
class Foo:
    __slots__ = 'a', 'b', 'c'

    def __init__(self, n):
        self.a = n
        self.b = n**2
        self.c = n**3

    @codon.jit
    def total(self):
        return self.a + self.b + self.c

print(Foo(10).total())  # 1110
```

`@codon.convert` requires the annotated class to specify `__slots__`, which
it uses to construct a generic Codon class (specifically, a named tuple) to
store the class's converted fields.

## Passing globals to Codon

Global variables, functions or modules can be passed to JIT'd functions through
the `pyvars` argument to `@codon.jit`:

``` python
import codon

def foo(n):
    print(f'n is {n}')

@codon.jit(pyvars=['foo'])
def bar(n):
    foo(n)  # calls the Python function 'foo'
    return n ** 2

print(bar(9))  # 'n is 9' then '81'
```

This also allows imported Python modules to be accessed by Codon. All `pyvars`
are passed as Python objects. Note that JIT'd functions can call each other
by default.

:::info


`pyvars` takes in variable names as strings, not the variables themselves.

:::

## Debugging

`@codon.jit` takes an optional `debug` parameter that can be used to print debug
information such as generated Codon functions and data types:

``` python
import codon

@codon.jit(debug=True)
def sum_of_squares(v):
    return sum(i**2 for i in v)

print(sum_of_squares([1.4, 2.9, 3.14]))
```

outputs:

```
[codon::jit::execute] code:
def sum_of_squares(v):
    return sum(i**2 for i in v)
-----
[python] sum_of_squares(['List[float]'])
[codon::jit::executePython] wrapper:
@export
def __codon_wrapped__sum_of_squares_0(args: cobj) -> cobj:
    a0 = List[float].__from_py__(PyTuple_GetItem(args, 0))
    return sum_of_squares(a0).__to_py__()
-----
20.229599999999998
```

## Internals and performance tips

Under the hood, the `codon` module maintains an instance of the Codon JIT,
which it uses to dynamically compile annotated Python functions. These functions
are then wrapped in *another* generated function that performs the type conversions.
The JIT maintains a cache of native function pointers corresponding to annotated
Python functions with concrete input types. Hence, calling a JIT'd function
multiple times does not repeatedly invoke the entire Codon compiler pipeline,
but instead reuses the cached function pointer.

Conversion cost depends on the data: converting a large Python list traverses
its elements, whereas a supported NumPy array can share its data buffer.
Batching work into fewer calls is therefore useful even when each individual
native operation is fast. When measuring a candidate function, a warm-up call
for each input type and an output check help separate compilation, conversion
and execution costs without changing the work being compared.
