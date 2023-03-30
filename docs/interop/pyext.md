Codon includes a build mode for generating
[Python extensions](https://docs.python.org/3/extending/extending.html)
(which are traditionally written in C, C++ or Cython):

``` bash
codon build -pyext extension.codon  # add -release to enable optimizations
```

`codon build -pyext` accepts the following options:

- `-o <output object>`: Writes the compilation result to the specified file.
- `-module <module name>`: Specifies the generated Python module's name.

# Functions

Extension functions written in Codon should generally be fully typed:

``` python
def foo(a: int, b: float, c: str):  # return type will be deduced
	return a * b + float(c)
```

The `pyext` build mode will automatically generate all the necessary wrappers
and hooks for converting a function written in Codon into a function that's
callable from Python.

Function arguments that are not explicitly typed will be treated as generic
Python objects, and operated on through the CPython API.

# Types


