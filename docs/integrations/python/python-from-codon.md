There are two ways to call Python from Codon:

- `from python import` allows importing and calling Python functions
  from existing Python modules.
- `@python` allows writing Python code directly in Codon.

In order to use these features, the `CODON_PYTHON` environment variable
must be set to the appropriate Python shared library:

``` bash
export CODON_PYTHON=/path/to/libpython.X.Y.so
```

For example, with a Homebrew-installed Python 3.9 on macOS, this might be

```
/usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/lib/libpython3.9.dylib
```

Note that only Python versions 3.6 and later are supported.

!!! tip "Tip: Finding `libpython`"

    You can use
    [this](https://raw.githubusercontent.com/exaloop/codon/refs/heads/develop/test/python/find-python-library.py)
    script to locate the Python shared library for the `CODON_PYTHON` environment variable. Simply run it as
    `python3 find-python-library.py` and it will print the library path.

!!! info "Info: Using virtual environments (`venv`)"

    If you are using a virtual environment created with `venv`, set `PYTHON_PATH` to the `site-packages`
    directory inside your virtual environment (e.g. `.venv/lib/python3.11/site-packages`).

!!! info "Info: Using virtual environments (`uv`)"

    If you using a virtual environment created with `uv`, you can use the following steps to set up the necessary
    environment variables. From the parent directory of your python project (where `pyproject.toml` is):

    1. Run `uv python find --system` and set `PYTHON_HOME` to the result.
    2. Set `CODON_PYTHON` to the `libpython.dylib` (or `.so`) file found in the folder from the previous
      step (e.g. `lib/python3.11.dylib`) as an absolute path.
    3. Set `PYTHON_PATH` to the `site-packages` folder inside your virtual environment (e.g `.venv/lib/python3.11/site-packages`).

## Import Python modules in Codon

Python modules can be imported and used in Codon-compiled programs through
a `from python import <module>` import statement. For example:

``` python
from python import sys  # imports Python's 'sys' module
print(sys.version)  # 3.11.12 (main, Apr  8 2025, 14:15:29) [Clang 17.0.0 (clang-1700.0.13.3)]
```

You can also import third-party libraries. Here is an example that imports
Matplotlib to create a simple plot:

``` python
from python import matplotlib.pyplot as plt

x = [1, 2, 3, 4, 5]
y = [2, 5, 3, 6, 4]

fig, ax = plt.subplots()
ax.plot(x, y)
plt.show()
```

Objects created from imported Python modules can be manipulated and operated on
from Codon. Internally, such operations are implemented by using CPython's C API.
For example, we can create a Pandas dataframe in Codon, and perform operations
on it:

``` python
from python import pandas as pd
df = pd.DataFrame({'A': [1, 2, 3], 'B': [4, 5, 6]})  # internally a Python object
print(df['B'].max())  # 6
```

## Run Python code directly in Codon

It is also possible to annotate functions with the `@python` decorator in order
to have them execute in Python, instead of being compiled by Codon:

``` python
@python
def version():
	# the following runs in plain Python
    import sys
    print(sys.version)

version()  # 3.11.12 (main, Apr  8 2025, 14:15:29) [Clang 17.0.0 (clang-1700.0.13.3)]
```

`@python` functions can specify return types, in which case returned values will
be checked and converted to native Codon types:

``` python
@python
def foo():
    return 2 + 2

@python
def bar() -> int:
    return 2 + 2

@python
def baz() -> int:
    return 'abc'


print(foo())  # 4 (Python object)
print(bar())  # 4 (native Codon int)
print(baz())  # error: Python object did not have type 'int'
```

Similarly, arguments can be type-annotated as well:

``` python
@python
def square(n: int) -> int:
    return n * n

print(square(4))  # 16
```

## Data conversions

Codon uses two new magic methods to transfer data to and from Python:

- `__to_py__`: Produces a Python object (`PyObject*` in C) given a Codon object.
- `__from_py__`: Produces a Codon object given a Python object.

For example:

``` python
import python  # needed to initialize the Python runtime

o = (42).__to_py__()  # type of 'o' is 'Ptr', equivalent to a pointer in C
print(o)  # 0x100e00610

n = int.__from_py__(o)  # converts Python object 'o' to native Codon integer
print(n)  # 42
```

Codon stores the results of `__to_py__` calls by wrapping them in an instance of a new
class called `pyobj`, which correctly handles the underlying Python object's reference
count. All operations on `pyobj`s then go through CPython's API.
