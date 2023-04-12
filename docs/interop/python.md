Calling Python from Codon is possible in two ways:

-   `from python import` allows importing and calling Python functions
    from existing Python modules.
-   `@python` allows writing Python code directly in Codon.

In order to use these features, the `CODON_PYTHON` environment variable
must be set to the appropriate Python shared library:

``` bash
export CODON_PYTHON=/path/to/libpython.X.Y.so
```

For example, with a `brew`-installed Python 3.9 on macOS, this might be

``` bash
/usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/lib/libpython3.9.dylib
```

Note that only Python versions 3.6 and later are supported.

# `from python import`

Let\'s say we have a Python function defined in *mymodule.py*:

``` python
def multiply(a, b):
    return a * b
```

We can call this function in Codon using `from python import` and
indicating the appropriate call and return types:

``` python
from python import mymodule.multiply(int, int) -> int
print(multiply(3, 4))  # 12
```

(Be sure the `PYTHONPATH` environment variable includes the path of
*mymodule.py*!)

`from python import` does not need to specify explicit types, in which case
Codon will operate directly on the Python objects, and convert Codon types
to Python types as necessary:

``` python
from python import numpy as np  # Codon will call NumPy through CPython's API
x = np.array([1, 2, 3, 4]) * 10
print(x)  # [10 20 30 40]
```

# `@python`

Codon programs can contain functions that will be executed by Python via
`pydef`:

``` python
@python
def multiply(a: int, b: int) -> int:
    return a * b

print(multiply(3, 4))  # 12
```

This makes calling Python modules like NumPy very easy:

``` python
@python
def myrange(n: int) -> List[int]:
    from numpy import arange
    return list(arange(n))

print(myrange(5))  # [0, 1, 2, 3, 4]
```

# Data conversions

Codon uses two new magic methods to transfer data to and from Python:

- `__to_py__`: Produces a Python object (`PyObject*` in C) given a Codon object.
- `__from_py__`: Produces a Codon object given a Python object.

``` python
import python  # needed to initialize the Python runtime

o = (42).__to_py__()  # type of 'o' is 'cobj', equivalent to a pointer in C
print(o)  # 0x100e00610

n = int.__from_py__(o)
print(n)  # 42
```

Codon stores the results of `__to_py__` calls by wrapping them in an instance of
a new class called `pyobj`, which correctly handles the underlying Python object's
reference count. All operations on `pyobj`s then go through CPython's API.
