Codon provides various low-level programming features that can
be used in performance-sensitive settings or when interfacing
with external APIs.

## Integer types

While Codon's standard `int` type is a 64-bit signed integer,
different integer types are made available through the `Int`
and `UInt` types:

- `Int[N]`: Signed integer with `N` bits
- `UInt[N]`: Unsigned integer with `N` bits

For example:

``` python
a = Int[16](42)    # signed 16-bit integer 42
b = UInt[128](99)  # unsigned 128-bit integer 99
```

The Codon standard library provides shorthands for the common variants:

- `i8`/`u8`: signed/unsigned 8-bit integer
- `i16`/`u16`: signed/unsigned 16-bit integer
- `i32`/`u32`: signed/unsigned 32-bit integer
- `i64`/`u64`: signed/unsigned 64-bit integer

You can cast between different integer types freely:

``` python
a = 42
b = i16(a)
c = u32(b)
```

Similarly, you can perform arithmetic operations on integers of the
same types:

``` python
x = i32(10)
y = i32(20)
print(x + y)  # 30
```

## Floating-point types

Codon's standard `float` type represents a 64-bit floating-point value
(IEEE 754 `binary64`). Codon supports several alternative floating-point
types:

- `float32`: 32-bit floating-point value (IEEE 754 `binary32`)
- `float16`: 16-bit floating-point value (IEEE 754 `binary16`)
- `float128`: 128-bit floating-point value (IEEE 754 `binary128`)
- `bfloat16`: 16-bit "brain" floating-point value (7-bit significand).
  Provides the same number of exponent bits as float, so that it matches
  its dynamic range, but with greatly reduced precision.

Each of these float types can be constructed from a standard `float`:

``` python
x = float32(3.14)
y = float128(-1.0)
z = bfloat16(0.5)
```

They all also support the usual arithmetic operators:

``` python
x = bfloat16(2) ** bfloat16(0.5)
print(x)  # 1.41406
```

## Pointers

Codon supports raw pointers natively via the `Ptr` type, which is
parameterized by the type of the object being pointed to (i.e.
`Ptr[int]` is an `int` pointer, `Ptr[float]` is a `float` pointer,
and so on).

!!! danger

    Pointer operations are not bounds-checked, meaning dereferencing
    an invalid pointer can cause a segmentation fault.

Buffers of a specific type can be dynamically allocated by constructing
a `Ptr` type with an integer argument, representing the number of elements
of the given type to allocate:

``` python
buffer = Ptr[int](10)  # equivalent to 'malloc(10 * sizeof(int64_t))' in C
```

Pointers can be dereferenced and indexed:

``` python
buffer = Ptr[int](10)
buffer[0] = 10  # equivalent to '(*buffer) = 10' in C
buffer[5] = 42  # equivalent to 'buffer[5] = 42' in C
print(buffer[0])  # 10
print(buffer[5])  # 42
```

Constructing a pointer without any arguments results in a null pointer:

``` python
null = Ptr[int]()  # equivalent to 'NULL' in C or 'nullptr' in C++
```

You can cast between different pointer types:

``` python
x = Ptr[float](1)
x[0] = 3.14

y = Ptr[int](x)  # treat 'x' as an integer pointer
print(y[0])      # 4614253070214989087 - same bits as '3.14' float
```

Pointers support various arithmetic and comparison operators:

``` python
p = Ptr[int](1)
q = p + 1  # equivalent to '&p[1]' in C

print(p == q)  # False
print(q - p)   # 1
print(p < q)   # True
```

### Pointers to variables

It is possible to obtain a pointer to a variable via the `__ptr__` intrinsic
function:

``` python
x = 42
p = __ptr__(x)  # 'p' is a 'Ptr[int]'; equivalent to '&x' in C
p[0] = 99
print(x)  # 99
```

Pointers to variables can be useful when interfacing with C APIs. For example,
consider the C standard library function [`frexp()`](https://en.cppreference.com/w/cpp/numeric/math/frexp.html)
which stores one of its outputs in an argument pointer. We can call this
function from Codon as follows:

``` python
from C import frexp(float, Ptr[i32]) -> float

x = 16.4
exponent = i32()
mantissa = frexp(x, __ptr__(exponent))  # equivalent to 'frexp(x, &exponent)' in C

print(mantissa, exponent)  # 0.5125 5
```

Refer to [C/C++ integration](integrations/cpp) for more information about
calling C/C++ functions from Codon.

### Pointers to fields

`__ptr__` can also be used to obtain pointers to fields of tuple classes:

``` python
@tuple
class Point:
    x: int
    y: int

r = Point(3, 4)
p = __ptr__(r.y)  # 'p' is a 'Ptr[int]'; equivalent to '&r.x' in C
p[0] = 99
print(r.x, r.y)  # 3 99
```

Recall that tuple class instances are immutable and passed by value, so
assignments create new instances:

``` python
@tuple
class Point:
    x: int
    y: int

r = Point(3, 4)
s = r  # creates a copy of 'r'
p = __ptr__(r.y)
p[0] = 99

print(s.x, s.y)  # 3 4 (not changed by pointer modification)
```

### Software prefetching

Pointers have several methods to facilitate software prefetching. These
methods all have the form `__prefetch_[rw][0123]__`, where `[rw]` indicates
whether the prefetch is made for a "read" (`r`) or a "write" (`w`), and
the `[0123]` is a temporal locality specifier, with higher values indicating
more locality (i.e. that the value should be kept in cache).

For example:

``` python
p = Ptr[int](1)
p.__prefetch_w3__()  # prefetch for write, high temporal locality
```

Refer to [LLVM's prefetch intrinsic](https://llvm.org/docs/LangRef.html#llvm-prefetch-intrinsic)
for additional information.

!!! warning

    Not all targets support software prefetching. Consult the relevant documentation
    for your instruction set architecture for more information.

## Static arrays

Arrays can be allocated on the stack via the `__array__` intrinsic function:

``` python
def f():
    arr = __array__[int](10)  # array of 10 integers; equivalent to 'int64_t arr[10]' in C
    arr[0] = 42
    print(arr[0])  # 42
```

Arrays created with `__array__` have two fields: `ptr` (pointer to array data) and `len`
(length of array). The argument of `__array__` must be a [literal](/language/meta#literals) integer.
