Codon supports a number of additional types that are not present
in plain Python.

# Arbitrary-width integers

Codon's `int` type is a 64-bit signed integer. However, Codon
supports arbitrary-width signed and unsigned integers:

``` python
a = Int[16](42)    # signed 16-bit integer 42
b = UInt[128](99)  # unsigned 128-bit integer 99
```

The Codon standard library provides shorthands for the common
variants:

- `i8`/`u8`: signed/unsigned 8-bit integer
- `i16`/`u16`: signed/unsigned 16-bit integer
- `i32`/`u32`: signed/unsigned 32-bit integer
- `i64`/`u64`: signed/unsigned 64-bit integer

# 32-bit float

Codon's `float` type is a 64-bit floating point value. Codon
also supports `float32` (or `f32` as a shorthand), representing
a 32-bit floating point value (like C's `float`).

# Pointers

Codon has a `Ptr[T]` type that represents a pointer to an object
of type `T`. Pointers can be useful when interfacing with C. The
`__ptr__` keyword can also be used to obtain a pointer to a variable:

``` python
p = Ptr[int](100)  # allocate a buffer of 100 ints
p = Ptr[int]()     # null pointer

x = 42
p = __ptr__(x)     # pointer to x, like "&x" in C

from C import foo(Ptr[int])
foo(p)             # pass pointer to C function
```

The `cobj` alias corresponds to `void*` in C and represents a generic
C or C++ object.

{% hint style="warning" %}
Using pointers directly circumvents any runtime checks, so dereferencing a
null pointer, for example, will cause a segmentation fault just like in C.
{% endhint %}

# Static arrays

The `__array__` keyword can be used to allocate static arrays on the stack:

``` python
def foo(n):
    arr = __array__[int](5)  # similar to "long arr[5]" in C
    arr[0] = 11
    arr[1] = arr[0] + 1
    ...
```
