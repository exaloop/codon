# Call C/C++ from Codon

C/C++ functions can be called from Codon via the `from C import` import statement. Unlike standard imports, `from C import` must specify the imported function's argument and return types. For example:

```python
# import the C standard library 'sqrt' function
from C import sqrt(float) -> float
print(sqrt(2.0))  # 1.4142135623730951
```

You can also rename C-imported functions:

```python
# cobj is a C pointer (void*, char*, etc.)
from C import puts(cobj) -> i32 as print_line
print_line("hello".c_str())  # prints "hello"; c_str() converts Codon str to C string
```

The declaration needs to match the C signature, even when the return value is unused: `puts()` returns a C `int`, represented by `i32`, whereas Codon's ordinary `int` is 64-bit. A function returning C `void` uses `-> None`.

You can also add [annotations](https://docs.exaloop.io/language/llvm/#annotations) such as `@pure` to C-imported functions by instead declaring them with the `@C` attribute:

```python
@C
@pure
def abs(value: i32) -> i32:
    pass

print(abs(i32(-42)))  # 42
```

These annotations are optimizer contracts rather than runtime checks. They are appropriate only when the imported implementation satisfies their requirements; an incorrect annotation can change the behavior of optimized code.

warning

If you're using C++, remember to declare any functions you want to call from Codon with `extern "C"` to enable C linkage, which Codon expects.

## Type conversions

The following table shows the conversions between Codon and C/C++ types:

| Codon | C/C++ |
| --- | --- |
| `int` | `int64_t` |
| `i32` | `int32_t` (also C `int` on supported platforms) |
| `float32` | `float` |
| `float` | `double` |
| `bool` | `bool` |
| `complex` | `{double, double}` (real and imag.) |
| `str` | `{char*, int64_t}` (data and meta) |
| `bytes` | `{char*, int64_t}` (data and length) |
| `tuple` | Struct of fields |
| `class` | Pointer to instance storage; layout depends on the class hierarchy |
| `Ptr[T]` | `T*` |

These are native representations, not a mapping from a Codon class to an arbitrary C++ class. For a stable external interface, scalar arguments and pointers to explicitly described buffers are usually easier to share than compiler-managed object layouts.

`str`'s metadata encodes its character count in the low 56 bits and its character-storage kind in bits 56 and 57. The data is not necessarily UTF-8 or NUL-terminated. Specifically:

-   `meta & ((1 << 56) - 1)` gives the string length
    
-   `(meta & (3 << 56)) >> 56` gives the string kind:
    
    -   `0`: ASCII (1-byte encoding)
    -   `1`: Latin-1 (1-byte encoding)
    -   `2`: UCS-2 (2-byte encoding)
    -   `3`: UCS-4 (4-byte encoding)

warning

Use caution when returning structures from C-imported functions, as C compilers might use an ABI that differs from Codon's ABI. It is recommended to instead pass a pointer as an argument that the callee can populate with the return value.

### Optionals

Codon also has an `Optional[T]` type for representing `None` values, which is represented in one of two ways:

-   If `T` is a reference type (i.e. a type defined with `class`), then `Optional[T]` is represented the same way as `T` (i.e. a pointer to dynamically-allocated member data) with null representing `None`.
    
-   Otherwise, `Optional[T]` is represented as a C structure `{bool, T}` where the boolean field indicates whether the value is present.
    

### NumPy arrays

NumPy array types are parameterized by the data type (`dtype`) and array dimension (`ndim`). They correspond to the following C structure definition:

```c
struct ndarray {
  int64_t shape[ndim];
  int64_t strides[ndim];
  dtype *data
};
```

Refer to the [NumPy documentation](https://docs.exaloop.io/libraries/numpy/#array-abi) for an explanation of these fields.

## Ownership and lifetime

An imported function's type describes how arguments cross the ABI, but does not establish who owns pointed-to memory. A pointer borrowed from a Codon array or buffer remains valid only while its storage remains alive; if a C library retains the pointer after the call, the owning allocation needs a correspondingly long lifetime in the Codon program.

Conversely, memory allocated by a C library follows that library's ownership contract, including any required release function. Codon's garbage-collected allocations should not be passed to C's `free()` merely because they are represented by pointers. The [pointer guide](https://docs.exaloop.io/language/lowlevel/#pointers) distinguishes allocated buffers from pointers to local variables.

For text APIs, `str.c_str()` provides a NUL-terminated UTF-8 buffer rather than exposing the native string layout. The pointer still needs to remain valid for the duration of its use, and an embedded NUL terminates a C string early. An API accepting a byte pointer and explicit length is more suitable for binary data or text that may contain NUL characters.

## Dynamic loading

Shared libraries can be loaded dynamically as follows:

```python
LIBRARY = "libhello.so"

# load dynamically from 'libhello.so'
from C import LIBRARY.foo(int, float) -> None
from C import LIBRARY.bar() -> int as baz

x = foo(1, 2.2)
y = baz()
```

Dynamic C imports are implemented by calling `dlopen()` on the given shared library, followed by `dlsym()` to obtain the required functions.

Source: https://docs.exaloop.io/integrations/cpp/cpp-from-codon/
