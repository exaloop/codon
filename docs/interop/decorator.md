Codon includes a Python package called `codon` that allows
functions or methods within Python codebases to be compiled and
executed by Codon's JIT. The `codon` library can be installed
with `pip`:

```bash
pip install codon-jit
```

This library will attempt to use an installed version of Codon.
If Codon is installed at a non-standard path, set the `CODON_DIR`
environment variable to the installation path.

# Using `@codon.jit`

The `@codon.jit` decorator causes the annotated function to be
compiled by Codon, and automatically converts standard Python
objects to native Codon objects. For example:

```python
import codon
from time import time

def is_prime_python(n):
    if n <= 1:
        return False
    for i in range(2, n):
        if n % i == 0:
            return False
    return True

@codon.jit
def is_prime_codon(n):
    if n <= 1:
        return False
    for i in range(2, n):
        if n % i == 0:
            return False
    return True

t0 = time()
ans = sum(1 for i in range(100000, 200000) if is_prime_python(i))
t1 = time()
print(f'[python] {ans} | took {t1 - t0} seconds')

t0 = time()
ans = sum(1 for i in range(100000, 200000) if is_prime_codon(i))
t1 = time()
print(f'[codon]  {ans} | took {t1 - t0} seconds')
```

outputs:

```
[python] 8392 | took 39.6610209941864 seconds
[codon]  8392 | took 0.998633861541748 seconds
```

{% hint style="info" %}
`@par` (to parallelize `for`-loops) can be used in annotated functions
via a leading underscore: `_@par`.
{% endhint %}

{% hint style="warning" %}
Changes made to objects in a JIT'd function will not be reflected
in the host Python application, since objects passed to Codon are
*converted* to Codon-native types. If objects need to be modified,
consider returning any necessary values and performing modifications
in Python.
{% endhint %}

{% hint style="warning" %}
Polymorphism and inheritance are not yet supported in JIT mode.
{% endhint %}


# Type conversions

`@codon.jit` will attempt to convert any Python types that it can
to native Codon types. The current conversion rules are as follows:

- Basic types like `int`, `float`, `bool`, `str` and `complex` are
  converted to the same type in Codon.

- Tuples are converted to Codon tuples (which are then compiled
  down to the equivalent of C structs).

- Collection types like `list`, `dict` and `set` are converted to
  the corresponding Codon collection type, with the restriction
  that all elements in the collection must have the same type.

- Other types are passed to Codon directly as Python objects.
  Codon will then use its Python object API ("`pyobj`") to handle
  and operate on these objects. Internally, this consists of calling
  the appropriate CPython C API functions, e.g. `PyNumber_Add(a, b)`
  for `a + b`.

## Custom types

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

# Passing globals to Codon

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

{% hint style="info" %}
`pyvars` takes in variable names as strings, not the variables themselves.
{% endhint %}

# Debugging

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

# Internals and performance tips

Under the hood, the `codon` module maintains an instance of the Codon JIT,
which it uses to dynamically compile annotated Python functions. These functions
are then wrapped in *another* generated function that performs the type conversions.
The JIT maintains a cache of native function pointers corresponding to annotated
Python functions with concrete input types. Hence, calling a JIT'd function
multiple times does not repeatedly invoke the entire Codon compiler pipeline,
but instead reuses the cached function pointer.

Although object conversions from Python to Codon are generally cheap, they do
impose a small overhead, meaning **`@codon.jit` will work best on expensive and/or
long-running operations** rather than short-lived operations. By the same token,
**the more work can be done in Codon, the better,** as opposed to repeatedly
transferring back and forth.
