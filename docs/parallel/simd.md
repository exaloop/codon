Codon's `simd` module and `Vec[T, N]` type provide direct, ergonomic access to
[SIMD](https://en.wikipedia.org/wiki/Single_instruction,_multiple_data)
instructions, including:

- Explicit control over vector width and element type
- Portable syntax for arithmetic, logic, comparisons, and math intrinsics
- Reductions, masking, and overflow-aware operations


## Vector types

`simd.Vec[T, N]` represents an LLVM vector `<N x T>`:

- `T` is a scalar numeric type (e.g. `float32`, `int`, `u32`, etc.)
- `N` is an integeral [literal](../language/meta.md)

Conceptually:

* `Vec[float32, 8]` ≈ *"8-wide float32 SIMD register"*
* `Vec[u8, 16]` ≈ *"16 byte-wide lanes"*

You would typically use `Vec` in hot loops where:

- Work is *data-parallel* (same operation applied to many elements)
- The data is laid out in contiguous memory (arrays, lists, strings)
- You care about *predictable* vectorization (not relying on the auto-vectorizer)


## Creating vectors

### Broadcasting a scalar

The simplest way to create a vector is to broadcast a scalar into all lanes:

``` python
from simd import Vec

# 8-lane vector of all ones
v = Vec[float, 8](1.0)

# 4-lane vector of all -3
w = Vec[int, 4](-3)
```

### Loading from pointers and arrays

`Vec` can also load data from a pointer, such as the underlying buffer of a
NumPy array. Here is an example that implements hand-vectorized addition of
two arrays:

``` python
import numpy as np
from simd import Vec

def add_arrays(a: np.ndarray[float32, 1],
               b: np.ndarray[float32, 1],
               out: np.ndarray[float32, 1]):
    W: Literal[int] = 8  # vector width
    i = 0
    while i + W <= len(a):
        va = Vec[float32, W](a.data + i)            # load 8 elements from a[i..i+7]
        vb = Vec[float32, W](b.data + i)            # load 8 elements from b[i..i+7]
        vc = va + vb                                # SIMD add
        Ptr[Vec[float32, W]](out.data + i)[0] = vc  # store back
        i += W

    # handle remaining tail elements (scalar)
    while i < len(a):
        out[i] = a[i] + b[i]
        i += 1
```

Note that:

- `Vec[T, N](ptr)` treats `ptr` as a `Ptr[Vec[T, N]]` and loads one SIMD vector
- You can store by casting the output pointer to `Ptr[Vec[T, N]]`

You can also construct vectors from lists:

``` python
data = [1.0, 2.0, 3.0, 4.0]
v = Vec[float, 4](data)  # load from data[0..3]
```

### Loading from strings (bytes)

For byte-sized element types (`i8`, `u8`, `byte`), you can read directly from a string buffer:

``` python
# Load 16 bytes from a string
s = "ABCDEFGHIJKLMNOP"
v = Vec[u8, 16](s)

# Skip first 4 bytes
v2 = Vec[u8, 16](s, 4)
```


## SIMD arithmetic

All basic arithmetic operations are *lane-wise*:

- `+`, `-`, `*` on integer and float vectors
- `/` on float vectors (true division)
- `//` and `%` on integer vectors
- and so on...

Example: fused multiply-add style accumulation for a dot product:

``` python
import numpy as np
from simd import Vec

def dot(a: Ptr[float32], b: Ptr[float32], n: int) -> float32:
    W: Literal[int] = 8
    i = 0
    acc = Vec[float32, W](0.0f32)

    while i + W <= n:
        va = Vec[float32, W](a + i)
        vb = Vec[float32, W](b + i)
        acc = acc + va * vb   # lane-wise multiply + add
        i += W

    # horizontal reduce SIMD accumulator
    total = acc.sum()

    # handle tail scalars
    while i < n:
        total += a[i] * b[i]
        i += 1

    return total
```

Note that:

- `va * vb` multiplies lanes
- `acc.sum()` adds all lanes, resulting in a scalar


## Masks, comparisons and branchless code

Comparisons between vectors (or between a vector and a scalar) produce *mask vectors*:

``` python
v: Vec[float, 8] = ...
mask_negative = v < Vec[float, 8](0.0)  # Vec[u1, 8]
```

You can then use `mask` to select between two vectors, *without branches*:

``` python
def relu_vec(v: Vec[float, 8]) -> Vec[float, 8]:
    zero = Vec[float, 8](0.0)
    m = v > zero            # positive lanes
    return v.mask(zero, m)  # if m: v else zero
```

The general pattern is:

- Build a mask via comparisons (`<`, `<=`, `>`, `>=`, `==`, `!=`)
- Use `mask(self, other, mask)` to do: `mask ? self : other` lane-wise

This is useful to turn control-flow into data-flow, which is conducive to SIMD programming:

- Clamping (`min`/`max`/conditionals)
- Thresholding (e.g. `x > t ? x : 0`)
- Selectively updating subset of lanes


## Reductions and horizontal operations

`Vec` includes a reduction over addition, both for integers and floats:

``` python
v = Vec[float32, 8](...)
s = v.sum()  # returns float32
```

Internally this uses LLVM's vector reduction intrinsics and is typically much faster than
manually scattering and summing.

You can combine this with loops to build aggregate operations. Below is an example that
implements L2 norm:

``` python
def l2_norm(xs: Ptr[float32], n: int) -> float32:
    W: Literal[int] = 8
    i = 0
    acc = Vec[float32, W](0.0f32)

    while i + W <= n:
        v = Vec[float32, W](xs + i)
        acc = acc + v * v
        i += W

    total = acc.sum()
    while i < n:
        total += xs[i] * xs[i]
        i += 1

    return np.sqrt(total)
```


## Integer-specific operations

Integer types support additional operations, such as:

- bitwise `&`, `|`, `^`
- shifts `<<`, `>>`
- `min`, `max`
- overflow-aware add/sub

### Saturating add example

Suppose you want to add two `u8` images but clamp to 255 on overflow.
You can use the overflow-aware addition and a mask:

``` python
def saturating_add_u8(a: Ptr[u8], b: Ptr[u8],
                      out: Ptr[u8], n: int):
    W: Literal[int] = 16
    i = 0
    max_val = Vec[u8, W](255u8)

    while i + W <= n:
        va = Vec[u8, W](a + i)
        vb = Vec[u8, W](b + i)

        (sum_vec, overflow) = va.add(vb, overflow=True)
        # where overflow[i] == 1, clamp to 255
        clamped = max_val.mask(sum_vec, overflow)

        Ptr[Vec[u8, W]](out + i)[0] = clamped
        i += W

    while i < n:
        s = int(a[i]) + int(b[i])
        out[i] = u8(255 if s > 255 else s)
        i += 1
```

Note:

- `va.add(vb, overflow=True)` returns `(result, overflow_mask)`
- Use `mask` to blend between a "safe" value and the raw result


## Debugging

### Scatter to list

Vectors can be transformed into lists:

``` python
v = Vec[int, 4](...)
lst = v.scatter()  # List[int] of length 4
print(lst)         # e.g. [1, 1, 1, 1]
```

### Printing vectors

Vectors can be printed directly:

``` python
print(v)  # e.g. "<1,1,1,1>"
```
