The `codon` command includes several subcommands for compiling
and executing code.

## `codon run`

`codon run` will compile and execute the provided program:

``` bash
codon run file.py           # compile and run (defaults to debug mode)
codon run -debug file.py    # compile and run in debug mode
codon run -release file.py  # compile and run with optimizations
```

If the file is given as `-`, then the program is read from standard
input:

``` bash
echo 'print("hello")' | build/codon run -release -
# hello
```

Program arguments can be provided after the file, as in:

``` bash
codon run -release file.py arg1 arg2 arg3
```

For example:

``` bash
echo 'import sys; print(sys.argv)' | build/codon run -release - arg1 arg2 arg3
# ['-', 'arg1', 'arg2', 'arg3']
```

## `codon build`

`codon build` will compile to a desired output type, be it an executable, object file,
shared library, LLVM IR or Python extension. The `-release` and `-debug` flags also apply
to `codon build` in the same way as described from `codon run` above.

The optional `-o <file>` parameter can be used to specify the output file. If no output
type is specified, the output type is determined from the file name provided to `-o`. If
no output file name is specified, the output file name is derived from the input file name,
in combination with the output type (e.g. compiling `foo.py` to an object file results in
output file `foo.o`).

### Compile to executable

The `-exe` flag can be used to generate an executable:

``` bash
# compile 'program.py' to executable 'program'
codon build -exe program.py

# compile 'program.py' to executable 'hello'
codon build -exe -o hello program.py

# compile 'program.py' to executable 'hello' with optimizations
codon build -exe -o hello -release program.py

# compile 'program.py' to executable 'hello' with optimizations
# '-exe' is inferred from `-o` argument
codon build -o hello -release program.py
```

Codon uses a C++ compiler to link the actual executable after compilation.
Extra linker flags can be passed with the `-linker-flags` argument. For example:

``` bash
# includes /foo/bar in the executable's rpath
codon build -release -linker-flags '-Wl,-rpath,/foo/bar' program.py
```

Multiple linker flags can be passed by separating them with a space in the argument
to `-linker-flags`.

### Compile to shared library

The `-lib` flag can be used to generate a shared library:

``` bash
# compile 'program.py' to shared library 'program.so'
codon build -lib program.py

# compile 'program.py' to shared library 'hello.so'
codon build -lib -o hello.so program.py

# compile 'program.py' to shared library 'hello.so' with optimizations
codon build -lib -o hello.so -release program.py

# compile 'program.py' to shared library 'hello.so' with optimizations
# '-lib' is inferred from `-o` argument
codon build -o hello.so -release program.py
```

The `-linker-flags` flag described above also applies when compiling to
a shared library.

!!! info

    When compiling to a shared library, the program's main code will be
    included and executed when the library is loaded as a *constructor*.
    This allows you to include any necessary initialization code in the
    main program.

!!! info

    If you intend to call a Codon-generated shared library from C or C++,
    be sure to mark relevant functions with `@export` to ensure they are
    made visible by the linker. Exported functions can be called as regular
    C functions (i.e. they follow the C ABI).
    [Learn more &#x2192;](/integrations/cpp/codon-from-cpp)

### Compile to object file

The `-obj` flag can be used to generate an object file:

``` bash
# compile 'program.py' to object file 'program.o'
codon build -obj program.py

# compile 'program.py' to object file 'hello.o'
codon build -obj -o hello.o program.py

# compile 'program.py' to object file 'hello.o' with optimizations
codon build -obj -o hello.o -release program.py

# compile 'program.py' to object file 'hello.o' with optimizations
# '-obj' is inferred from `-o` argument
codon build -o hello.o -release program.py
```

### Compile to LLVM IR

The `-llvm` flag can be used to generate LLVM IR:

``` bash
# compile 'program.py' to LLVM IR file 'program.ll'
codon build -llvm program.py

# compile 'program.py' to LLVM IR file 'hello.ll'
codon build -llvm -o hello.ll program.py

# compile 'program.py' to LLVM IR file 'hello.ll' with optimizations
codon build -llvm -o hello.ll -release program.py

# compile 'program.py' to LLVM IR file 'hello.ll' with optimizations
# '-llvm' is inferred from `-o` argument
codon build -o hello.ll -release program.py
```

### Compile to Python extension

The `-pyext` flag can be used to generate a Python extension:

``` bash
# compile 'program.py' to Python extension 'program.o'
codon build -pyext program.py

# compile 'program.py' to Python extension 'hello.o'
codon build -pyext -o hello.o program.py

# compile 'program.py' to Python extension 'hello.o' with optimizations
codon build -pyext -o hello.o -release program.py

# compile 'program.py' to Python extension 'hello.o' with module
# name 'mymodule' and optimizations enabled
codon build -pyext -o hello.o -release -module mymodule program.py
```

!!! info

    When using `-pyext`, you will also often want to use the `--relocation-model=pic`
    flag to generate position-independent code.

## `codon jit`

Codon provides a debugging interface for its JIT compilation capabilities through the
`codon jit` subcommand. This subcommand uses the same JIT engine internally as used by
Codon's [Python JIT decorator](/integrations/python/codon-from-python) and [Jupyter kernel](/integrations/jupyter).
However, it is intended to be used as a debugging utility rather than as a general usage mode.

!!! warning

    `codon jit` is intended to be used as a debugging utility for Codon's JIT compilation
    capabilities. The interface may change between Codon versions.

`codon jit` can be passed a file name to read from, or `-` to read from standard input.
For example:

```bash
echo 'print("hello world")' | build/codon jit -
# >>> Codon JIT v0.19.0 <<<
# hello world
# [done]
```

It can also be used as a REPL if no file is provided. JIT inputs can be separated with the string `#%%`.

## Using Codon in an existing Python codebase

Codon provides a Python package called `codon-jit` that can be installed with `pip`. This package
supports JIT compilation on a per-function basis within an existing Python codebase.

Learn more in the [Python JIT docs](/integrations/python/codon-from-python).

## Additional options

### Disabling exceptions

By default, Codon does exception handling to match Python's semantics and behavior. If you
know your program does not raise or catch exceptions, they can be disabled altogether with
the `-disable-exceptions` flag.

`-disable-exceptions` can lead to (sometimes substantial) performance improvements by enabling
the compiler to deduce additional information about the semantics of the program. For example,
if the compiler can deduce that there will be no index errors in a loop that iterates over an
array, it can potentially use that information to perform vectorization or other optimizations.

In some contexts, exceptions are disabled automatically by Codon, such as when compiling for
GPU execution.

### Numerical semantics

For performance reasons, certain numerical operations in Codon follow C semantics by default.
For example, integer division rounds towards zero in Codon whereas it rounds down in Python:

``` python
print((-3) // 2)
# Codon: -1
# Python: -2
```

Similarly, floating-point division by zero returns `inf` in Codon whereas it raises an exception
in Python:

``` python
print(1.0 / 0.0)
# Codon: inf
# Python: 'ZeroDivisionError: float division by zero'
```

The `-numerics=<mode>` flag can be used to control this behavior:

- `-numerics=c` (default): C semantics, as above
- `-numerics=py`: Python semantics, matching Python behavior at the cost of performance

### Fast-math

[Fast-math optimizations](https://llvm.org/docs/LangRef.html#fast-math-flags) can be enabled with
the `-fast-math` flag. Note that this flag makes various assumptions about `nan` and `inf` values,
so it is best to use it with caution.

### Disabling optimization passes

The `-disable-opt <pass>` flag can be used to disable specific optimization passes. For example:

``` bash
# compile & run with optimizations, but don't perform NumPy fusion optimization
build/codon run -release -disable-opt core-numpy-fusion program.py
```

## Compile-time definitions

Literal variables can be passed on the command-line via the `-D` flag. These variables are
treated as compile-time constants and can be used for [compile-time metaprogramming](/language/meta).

For example, the following code:

``` python
n = Int[N](42)
print(n * n)
```

can be executed with:

``` bash
codon run -DN=16 program.py
```

to use a 16-bit integer as the type of variable `n`.

## Logging

Codon can display logging information and also output intermediate compilation results via the
`-log <streams>` command. The argument to `-log` can contain any of the following characters:

- `t` (time): Displays timings for various stages of the compilation process.
- `T` (typecheck): Enables logging during type checking
- `i` (IR): Enables logging during Codon IR passes
- `l` (dump): Dumps intermediate compilation results, including AST, Codon IR and LLVM IR
