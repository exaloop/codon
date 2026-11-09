# Call Python from Codon

There are two ways to call Python from Codon:

-   `from python import` allows importing and calling Python functions from existing Python modules.
-   `@python` allows writing Python code directly in Codon.

These features run code in an embedded CPython interpreter; they do not compile the imported Python library into native Codon code. This makes it possible to keep a library-dependent part of an application in Python while using native Codon for the surrounding computation.

## Configure the Python runtime

`CODON_PYTHON` identifies the Python shared library to load, rather than the `python3` executable. The library, its standard library and any binary Python packages need to come from a compatible Python installation and architecture. Using the interpreter from the intended environment helps keep these paths consistent.

The following command looks for the shared library using that interpreter's build configuration, including framework installations on macOS:

```bash
export CODON_PYTHON="$(python3 - <<'PY'
import sysconfig
from pathlib import Path

config = sysconfig.get_config_vars()
candidates = []
for setting in ("LIBDIR", "LIBPL"):
    directory = config.get(setting)
    library = config.get("LDLIBRARY")
    if directory and library:
        candidates.append(Path(directory) / library)

framework = config.get("PYTHONFRAMEWORK")
framework_prefix = config.get("PYTHONFRAMEWORKPREFIX")
if framework and framework_prefix:
    candidates.append(Path(framework_prefix) / f"{framework}.framework"
                      / "Versions" / config["VERSION"] / framework)

for candidate in candidates:
    if candidate.is_file() and candidate.suffix != ".a":
        print(candidate.resolve())
        break
else:
    raise SystemExit("No shared Python library found; use a shared-library Python installation.")
PY
)"
```

If a distribution does not provide a shared Python library, an installation that includes one is needed for this integration. Native Codon programs that do not call Python have no such requirement.

### Virtual environments

After activating a virtual environment, its package directories can be added to the embedded interpreter's search path:

```bash
export PYTHONPATH="$(python3 -c 'import os, site; print(os.pathsep.join(site.getsitepackages()))')${PYTHONPATH:+:$PYTHONPATH}"
```

When CPython cannot locate its standard library, `PYTHONHOME` can additionally identify the base installation:

```bash
export PYTHONHOME="$(python3 -c 'import sys; print(sys.base_prefix)')"
```

This is an installation directory, not an executable or a `site-packages` directory. The variable names are CPython's `PYTHONPATH` and `PYTHONHOME`, without an underscore between `PYTHON` and the rest of the name. For a `uv`\-managed project, the configuration commands can use `uv run python` in place of `python3` to select the project's interpreter.

## Import Python modules in Codon

Python modules can be imported and used in Codon-compiled programs through a `from python import <module>` import statement. For example:

```python
from python import sys  # imports Python's 'sys' module
print(sys.version)
```

The reported version is that of the loaded Python runtime, which is a useful first check when diagnosing an environment mismatch.

You can also import third-party libraries. Here is an example that imports Matplotlib to create a simple plot:

```python
from python import matplotlib.pyplot as plt

x = [1, 2, 3, 4, 5]
y = [2, 5, 3, 6, 4]

fig, ax = plt.subplots()
ax.plot(x, y)
plt.show()
```

Objects created from imported Python modules can be manipulated and operated on from Codon. Internally, such operations are implemented by using CPython's C API. For example, we can create a Pandas dataframe in Codon, and perform operations on it:

```python
from python import pandas as pd
df = pd.DataFrame({'A': [1, 2, 3], 'B': [4, 5, 6]})  # internally a Python object
print(df['B'].max())  # 6
```

## Run Python code directly in Codon

It is also possible to annotate functions with the `@python` decorator in order to have them execute in Python, instead of being compiled by Codon:

```python
@python
def version():
    import sys
    print(sys.version)

version()
```

`@python` functions can specify return types, in which case returned values will be checked and converted to native Codon types:

```python
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
```

Calling `baz()` would fail at the conversion boundary because it returns a string where a native integer is required. A return annotation therefore describes a conversion as well as the type used by subsequent Codon code.

Similarly, arguments can be type-annotated as well:

```python
@python
def square(n: int) -> int:
    return n * n

print(square(4))  # 16
```

## Data conversions

A value obtained from a Python module normally remains a Python object, represented by Codon's `pyobj` wrapper. Operations on that value use CPython's API. Converting at a deliberate boundary lets the following computation use native Codon types instead:

```python
from python import json as pyjson

python_values = pyjson.loads('[2, 3, 5]')
values = list[int](python_values)
print(sum(values))  # 10
```

The conversion produces a native list, rather than a shared view of the Python list. Changes to the converted list are therefore not a way to mutate the original Python object. For performance-sensitive code, converting a batch once and doing the subsequent work natively is often more effective than crossing the boundary for each element. NumPy arrays have a separate [array interoperability path](https://docs.exaloop.io/libraries/numpy/#python-interoperability) that can share data.

At the implementation level, `__to_py__()` creates a raw Python object pointer and `__from_py__()` converts from such a pointer. These hooks are useful when implementing conversions for custom types, but direct calls require attention to Python's reference-ownership rules. Ordinary Python imports, calls and conversions use managed `pyobj` wrappers to handle that ownership on the application's behalf.

## Troubleshooting imports

A shared-library loading failure usually points to `CODON_PYTHON`, an architecture mismatch, or a missing dependent library. By contrast, `ModuleNotFoundError` means the interpreter started but could not find the requested Python package. Checking the imported Python `sys.version` and `sys.path` helps distinguish these cases; packages should be installed with the same interpreter used to configure the runtime.

Source: https://docs.exaloop.io/integrations/python/python-from-codon/
