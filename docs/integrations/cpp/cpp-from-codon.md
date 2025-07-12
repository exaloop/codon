C/C++ functions can be called from Codon via the `from C import`
import statement. Unlike standard imports, `from C import` must
specify the imported function's argument and return types. For
example:

``` python
# import the C standard library 'sqrt' function
from C import sqrt(float) -> float
print(sqrt(2.0))  # 1.41421
```

You can also rename C-imported functions:

``` python
# cobj is a C pointer (void*, char*, etc.)
# None can be used to represent C's void
from C import puts(cobj) -> None as print_line
print_line("hello".c_str())  # prints "hello"; c_str() converts Codon str to C string
```

You can also add [annotations](/language/llvm#annotations) such as
`@pure` to C-imported functions by instead declaring them with the `@C`
attribute:

``` python
@C
@pure
def sqrt(x: float) -> float:
    pass

print(sqrt(2.0))  # 1.41421
```

!!! warning

    If you're using C++, remember to declare any functions you want to call from
    Codon with `extern "C"` to enable C linkage, which Codon expects.

## Type conversions

The following table shows the conversions between Codon and C/C++ types:

| Codon         | C/C++                                |
| ------------- | ------------------------------------ |
| `int`         | `int64_t`                            |
| `float`       | `double`                             |
| `bool`        | `bool`                               |
| `complex`     | `{double, double}` (real and imag.)  |
| `str`         | `{int64_t, char*}` (length and data) |
| `tuple`       | Struct of fields                     |
| `class`       | Pointer to corresponding tuple       |
| `Ptr[T]`      | `T*`                                 |

!!! warning

    Use caution when returning structures from C-imported functions, as C compilers
    might use an ABI that differs from Codon's ABI. It is recommended to instead
    pass a pointer as an argument that the callee can populate with the return value.

### Optionals

Codon also has an `Optional[T]` type for representing `None` values, which
is represented in one of two ways:

- If `T` is a reference type (i.e. a type defined with `class`), then `Optional[T]`
  is represented the same way as `T` (i.e. a pointer to dynamically-allocated member
  data) with null representing `None`.

- Otherwise, `Optional[T]` is represented as a C structure `{bool, T}` where the
  boolean field indicates whether the value is present.

### NumPy arrays

NumPy array types are parameterized by the data type (`dtype`) and array dimension
(`ndim`). They correspond to the following C structure definition:

``` c
struct ndarray {
  int64_t shape[ndim];
  int64_t strides[ndim];
  dtype *data
};
```

Refer to the [NumPy documentation](/libraries/numpy#array-abi) for an explanation of these fields.

## Dynamic loading

Shared libraries can be loaded dynamically as follows:

``` python
LIBRARY = "libhello.so"

# load dynamically from 'libhello.so'
from C import LIBRARY.foo(int, float) -> None
from C import LIBRARY.bar() -> int as baz

x = foo(1, 2.2)
y = baz()
```

Dynamic C imports are implemented by calling `dlopen()` on the given shared library,
followed by `dlsym()` to obtain the required functions.
