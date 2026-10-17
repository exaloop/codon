Codon's `simd` module provides explicit vector operations through the `Vec[T, N]`
type. Using [SIMD](https://en.wikipedia.org/wiki/Single_instruction,_multiple_data)
(single instruction, multiple data), a vector operation applies the same
computation to several values at once. Each value occupies a *lane*, whose type
and width are specified by `T` and `N`.

The module supports arithmetic, bitwise operations, comparisons, mathematical
functions, and reductions, along with masks for selecting values and controlling
memory access. It is useful in performance-critical loops that process contiguous
data and require explicit control over vectorization.

## Vector types

`simd.Vec[T, N]` represents an LLVM vector `<N x T>`, where:

- `T` is an `Int`/`UInt` type, `float16`, `bfloat16`, `float32`, `float`, or `float128`.
- `N` is a positive integer [literal](../language/meta.md) that fits in 32 bits.

For example, `Vec[float32, 8]` contains eight 32-bit floating-point lanes, while
`Vec[u8, 16]` contains sixteen unsigned byte lanes. In either case, `len(v)`
returns the number of lanes, `N`.

Individual lanes can be accessed with literal or runtime integer indices, with
negative indices counting from the end. Out-of-range literal indices are rejected
at compile time; out-of-range runtime indices raise `IndexError` in both numeric
modes. The `v.extract(index)` method provides an explicit lane read with runtime
bounds checking.

Vectors are immutable. To replace a lane, use `v.insert(index, value)`, which
returns a new vector containing the replacement.

## Creating vectors

### Broadcasting a scalar

Passing a scalar to the constructor broadcasts it to every lane:

``` python
from simd import Vec

# 8-lane vector of all ones
v = Vec[float, 8](1.0)

# 4-lane vector of all -3
w = Vec[int, 4](-3)
```

### Loading from pointers and arrays

The `Vec` constructor can load consecutive elements from a pointer, including the
underlying buffer of a NumPy array. The following function adds two arrays in
groups of eight elements, then processes any remaining elements with a scalar
loop. It assumes that the arrays are contiguous and have matching lengths.

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
        vc.store(out.data, offset=i)               # store back
        i += W

    # handle remaining tail elements (scalar)
    while i < len(a):
        out[i] = a[i] + b[i]
        i += 1
```

`Vec[T, N](ptr)` reads `N` scalar elements using `T`'s memory layout. Unaligned
addresses are supported, but the caller must ensure that all `N` elements are
readable: raw pointer accesses are not bounds-checked.

Loads and stores through `Ptr[Vec[T, N]]` also support unaligned addresses, but
use LLVM's vector allocation stride, which can include padding. Non-power-of-two
lane counts can differ from scalar-array storage even for ordinary element widths:
for example, `Vec[int, 3]` can occupy 32 bytes rather than 24. Layout can also
differ for types such as `u1` and `UInt[24]`. Use `Vec[T, N].load(ptr, offset)` and
`v.store(ptr, offset)` when accessing scalar-array storage.

Vectors can also be constructed from lists:

``` python
data = [1.0, 2.0, 3.0, 4.0]
v = Vec[float, 4](data)  # load from data[0..3]
```

The list's element type must match `T`. To change types, convert the elements
explicitly or use the vector's `cast()` method. Constructors that read from lists,
strings, or byte buffers check the requested range and raise `IndexError` if the
offset is negative or fewer than `N` elements remain.
When the caller has already established these bounds, the constructor accepts
`check=False` to omit validation; see [Controlling runtime checks](#controlling-runtime-checks).

### Explicit loads and stores

`Vec[T, N].load(ptr, offset=0)` and `v.store(ptr, offset=0)` read and write scalar
arrays of `T`, including types with unusual integer widths. Offsets are measured
in elements.

Both operations default to `alignment=1`, which permits unaligned access. A
larger literal alignment, such as `alignment=16`, promises that the address
*after applying the offset* is aligned to that many bytes. The value must be a
power of two from 1 through `2**31`. Because this promise is not checked at
runtime, specify a larger alignment only when both the allocation and the offset
guarantee it.

A `Vec[u1, N]` mask restricts memory access to active lanes, allowing a partial
vector to be processed without accessing elements beyond the end of a buffer.
Inactive load lanes receive zero unless a scalar or vector `passthrough` value is
provided; inactive store lanes leave memory unchanged.

``` python
data = [10.0, 20.0, 30.0]
output = [0.0] * len(data)
active = Vec[int, 4]([0, 1, 2, 3]) < len(data)
values = Vec[float, 4].load(data._ptr, mask=active, passthrough=0.0)
(values + 1.0).store(output._ptr, mask=active)
assert output == [11.0, 21.0, 31.0]
```

Only active lanes require valid storage, so an all-false mask may be used with a
null pointer. Without a mask, the caller must provide storage for all `N` lanes.

### Indexed gather and scatter

For noncontiguous access, `Vec[T, N].gather(ptr, indices)` loads elements at the
offsets specified by a `Vec[int, N]`, while `v.scatter(ptr, indices)` writes lanes
to those offsets. Both operations accept `mask` and `alignment` arguments, and
gather also accepts `passthrough` for inactive lanes.

``` python
data = [10.0, 20.0, 30.0, 40.0]
output = [0.0] * len(data)
indices = Vec[int, 4]([3, 0, 2, 1])
values = Vec[float, 4].gather(data._ptr, indices)
values.scatter(output._ptr, indices)
assert output == data
```

Indices are scalar element offsets relative to the supplied pointer and are not
bounds-checked. Every active address must refer to valid storage and satisfy the
specified alignment. Active scatter destinations must also be distinct, because
the API does not define an order for conflicting writes.

The no-argument form, `v.scatter()`, returns the lanes as a list rather than
writing to memory.

### Loading from strings and bytes

For byte-sized element types (`i8`, `u8`, and `byte`), the constructor accepts
`bytes` or a string's UTF-8 encoding. Both the offset and the vector width count
bytes, so they do not necessarily correspond to Unicode character positions.
ASCII strings can be loaded directly; non-ASCII strings are encoded first. The
result retains the requested signed or unsigned element type.

``` python
# Load 16 bytes from a string
s = "ABCDEFGHIJKLMNOPQRST"
v = Vec[u8, 16](s)

# Skip first 4 bytes
v2 = Vec[u8, 16](s, 4)
```

## SIMD arithmetic

Arithmetic operates lane by lane: integer and floating-point vectors support
`+`, `-`, and `*`, floating-point vectors support true division with `/`, and
integer vectors support `//` and `%`.

A scalar operand on either side of an operator is broadcast to all lanes. Thus,
`2 * v`, `10 - v`, and `12 // v` preserve operand order and follow the same lane
type and numeric-mode rules as the corresponding vector-vector operations.

Integer division follows the selected numeric mode, as it does for scalar
integers:

- With `-numerics=py`, `//` rounds down and `%` produces a remainder with the
  divisor's sign. A zero divisor in any lane raises `ZeroDivisionError`. Both
  operators raise `OverflowError` for the minimum representable signed integer
  paired with a divisor of `-1`.
- With `-numerics=c`, division truncates toward zero and the remainder has the
  dividend's sign. Divisors must be nonzero, and the minimum representable signed
  integer must not be divided by `-1`; violating either precondition has
  undefined behavior.

Other integer arithmetic wraps at the lane width. Floating-point equality uses
ordered comparisons (`NaN == NaN` is false), while inequality uses unordered
comparisons (`NaN != NaN` is true). Unary negation preserves IEEE signed-zero
behavior.

For floating-point multiply-accumulate operations, `va.fma(vb, acc)` computes
`va * vb + acc` with a single rounding step per lane. The following dot product
accumulates eight products at a time, then uses `sum()` to reduce the accumulator
to a scalar before processing the remaining elements:

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
        acc = va.fma(vb, acc)
        i += W

    # horizontal reduce SIMD accumulator
    total = acc.sum()

    # handle tail scalars
    while i < n:
        total += a[i] * b[i]
        i += 1

    return total
```

### Floating-point math

Floating-point vectors provide `sqrt()`, `exp()`, `exp2()`, `log()`, `log2()`,
`log10()`, `sin()`, and `cos()` methods. The rounding methods `floor()`, `ceil()`,
`trunc()`, and `round()` preserve the floating-point lane type, with `round()`
using ties-to-even rounding.

The `exp()`, `exp2()`, `log2()`, `log10()`, `floor()`, `ceil()`, `trunc()`, and
`round()` methods, along with `fma()`, support `float16`, `bfloat16`, `float32`,
and `float`. Their `float128` specializations are rejected at compile time
because portable backend and runtime support is unavailable. The unary methods
listed in this paragraph evaluate `float16` and `bfloat16` lanes through `float32`
and convert back. The `sqrt()`, `log()`, `sin()`, and `cos()` methods instead use
intrinsics of the lane type; their lowering is backend-dependent.

Classification methods—`isnan()`, `isinf()`, `isfinite()`, and `signbit()`—support
all floating-point lane types and return `Vec[u1, N]` masks. `signbit()` tests the
sign bit, including that of negative zero and NaNs.

The lane-wise methods `v.min(other)` and `v.max(other)` accept either a scalar or
a vector. For floating-point lanes, they propagate NaNs and order negative zero
below positive zero. Both lane-wise and reducing min/max operations reject
`float128`, whose LLVM lowering is not reliably supported by the current backend.
Sum and product reductions and classification methods do support `float128`.

## Masks, comparisons, and branchless selection

Comparing two vectors, or a vector and a scalar, produces a *mask vector* with
one `u1` lane per comparison result:

``` python
v: Vec[float, 8] = ...
mask_negative = v < Vec[float, 8](0.0)  # Vec[u1, 8]
```

Use `v.mask(other, mask)` to select values lane by lane: a true mask lane selects
the corresponding value from `v`, and a false lane selects from `other`. This
expresses conditional updates as data selection without branches, which is useful
for thresholding, clamping, and selectively updating lanes. For example, a ReLU
operation retains positive values and replaces the remaining lanes with zero:

``` python
def relu_vec(v: Vec[float, 8]) -> Vec[float, 8]:
    zero = Vec[float, 8](0.0)
    m = v > zero            # positive lanes
    return v.mask(zero, m)  # if m: v else zero
```

### Inspecting masks

To inspect a mask as a whole, use `any()` or `all()`, which return scalar
booleans, or `count()`, which returns the number of nonzero lanes as an `int`.
These methods also accept numeric vectors, treating every nonzero value,
including NaN, as true. Use them explicitly instead of converting an entire
mask to `bool`.

``` python
mask = Vec[int, 4]([0, 5, 0, -2]) != 0
assert mask.any() and not mask.all()
assert mask.count() == 2
bits = mask.to_bits()
assert bits == UInt[4](0b1010)
assert Vec[u1, 4].from_bits(bits).scatter() == mask.scatter()
```

For a compact bit representation, `to_bits()` returns a `UInt[N]` with lane zero
in the least significant bit, independent of machine endianness.
`Vec[u1, N].from_bits()` reverses this mapping; neither operation is limited to
64 lanes.

## Reductions and horizontal operations

Reductions combine all lanes into a scalar of the lane type. The methods
`sum()`, `prod()`, `min()`, and `max()` compute the sum, product, minimum, and
maximum, respectively. For `min()` and `max()`, omitting the argument requests a
reduction; supplying an argument requests a lane-wise operation.

Integer reductions wrap at the lane width, while floating-point min/max
reductions follow the same NaN and signed-zero rules as their lane-wise
counterparts.

``` python
v = Vec[float32, 8](...)
s = v.sum()  # returns float32
product = v.prod()
smallest = v.min()
largest = v.max()
```

These operations use LLVM's vector reduction intrinsics, avoiding the need to
convert the vector to a list before combining its elements. They can also finish
an accumulation performed across multiple loop iterations, as in the following
L2 norm calculation:

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

### Rearranging lanes

`shuffle(indices)` constructs a vector by selecting lanes from the input. The
indices may be a nonempty tuple of `int` values or a `Vec[int, M]`; either form
produces a `Vec[T, M]`, allowing the output width to differ from the input width.
Each index must be in `[0, N)`. Unlike ordinary lane indexing, shuffle indices do
not accept negative values; invalid indices raise `IndexError` in both numeric
modes.

The two-vector form, `shuffle(indices, other)`, selects from the concatenated
lanes of both vectors: indices in `[0, N)` refer to the first vector, and those
in `[N, 2*N)` refer to `other`. Both vectors must have the same type and width.
For the common case of alternating lanes from two vectors, `interleave(other)`
returns a `Vec[T, 2*N]` directly.

``` python
left = Vec[int, 4]([10, 11, 12, 13])
right = Vec[int, 4]([20, 21, 22, 23])
assert left.shuffle((3, 0)).scatter() == [13, 10]
assert left.shuffle((0, 4, 1, 5), right).scatter() == [10, 20, 11, 21]
assert left.interleave(right).scatter() == [10, 20, 11, 21, 12, 22, 13, 23]
```

## Integer-specific operations

Integer vectors support bitwise operators (`&`, `|`, `^`, and `~`), bit shifts
(`<<` and `>>`), min/max operations, and overflow-aware addition and subtraction.
Comparisons, right shifts, min/max operations, and overflow flags respect the
signedness of the lane type. Absolute value leaves unsigned lanes unchanged;
for signed lanes, the absolute value of the minimum representable value wraps
to that same value.

Binary bitwise operators accept a scalar or vector on either side. Bit shifts
accept either a vector of per-lane counts or a scalar count applied to every
lane; reflected shifts such as `1 << counts` broadcast the left operand. Each
count must be between zero and the lane bit width minus one. Python numeric mode
checks this range and raises `ValueError` for invalid counts, while C numeric
mode requires the caller to satisfy the precondition.

The `lsh(k)` and `rsh(k)` methods instead shift whole lanes, filling vacated
lanes with zero. Their argument must be a literal between zero and `N`, inclusive:
zero leaves the vector unchanged, and `N` produces an all-zero vector.

Floating-point vectors also support `&`, `|`, and `^`, which operate on the lanes'
bit representations without numerically converting them to integers.

### Saturating addition

Overflow-aware arithmetic returns both the result and a mask identifying lanes
that overflowed. For example, `va.add(vb, overflow=True)` returns
`(result, overflow_mask)`, allowing the result to be replaced selectively with a
limit value.

The following function uses this pattern to add unsigned byte values with
saturation at 255, as needed when combining image buffers:

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

        clamped.store(out, offset=i)
        i += W

    while i < n:
        s = int(a[i]) + int(b[i])
        out[i] = u8(255 if s > 255 else s)
        i += 1
```

Here, `max_val.mask(sum_vec, overflow)` selects 255 for lanes that overflowed
and retains the computed sum for all other lanes.

## Controlling runtime checks

Checked operations accept `check: Literal[bool] = True`. Passing `check=False`
removes their runtime validation at compile time, which can avoid repeated checks
inside a loop whose bounds or operands are already known to be valid. The flag
must be a literal, not a runtime boolean.

The following interfaces expose this control:

| Operation | Unchecked form | Caller precondition |
| --- | --- | --- |
| List, byte, or string load | `Vec[T, N](data, offset=0, check=False)` | Nonnegative offset with at least `N` elements remaining; string offsets count UTF-8 bytes |
| Lane extraction | `v.extract(index, check=False)` | `-N <= index < N` |
| Lane replacement | `v.insert(index, value, check=False)` | `-N <= index < N` |
| Lane shuffle | `v.shuffle(indices, other=None, check=False)` | Every selector is in `[0, N)`, or `[0, 2*N)` when `other` is supplied |
| Bit shifts | `v.lshift(counts, check=False)`, `v.rshift(counts, check=False)` | Each count is nonnegative and less than the lane bit width |
| Integer division/remainder | `v.floordiv(divisors, check=False)`, `v.mod(divisors, check=False)` | No zero divisor; no signed minimum-value / `-1` pair |

The named integer methods accept either a scalar or a vector operand. `lshift()`
and `rshift()` shift bits within lanes, unlike `lsh()` and `rsh()`, which move
whole lanes. Operators such as `v[index]`, `v << counts`, `v // divisors`, and
`v % divisors` retain their existing default checks.

``` python
data = [4, 8, 12, 16]
vector = Vec[int, 4](data, check=False)
assert vector.extract(-1, check=False) == 16
counts = Vec[int, 4]([0, 1, 2, 3])
assert vector.rshift(counts, check=False).scatter() == [4, 4, 3, 2]
assert vector.floordiv(3, check=False).scatter() == [1, 2, 4, 5]
```

Only validation is removed. Negative lane indices still count from the end,
string loads still use UTF-8, and division/remainder retain the selected numeric
mode's rounding and sign rules. `check=True` preserves existing numeric-mode
behavior: integer division and shift checks apply in Python mode, while C mode
already requires valid operands. Buffer and lane checks apply in both modes
unless explicitly disabled.

Unchecked inputs must satisfy the preconditions even when the underlying storage
is larger than the logical buffer. Violating them can cause invalid memory access
or undefined behavior instead of an exception. Static element-type, vector-width,
mask-type, and alignment validation cannot be disabled with this flag.

Raw-pointer `load`, `store`, `gather`, and `scatter` already rely on caller-provided
bounds and do not accept a `check` argument. Masked-off lanes remain unaccessed;
their masks are memory-access semantics, not validation to be switched off.

## Debugging

### Converting to a list

Call `v.scatter()` without arguments to obtain a list of lane values for
inspection:

``` python
v = Vec[int, 4](...)
lst = v.scatter()  # List[int] of length 4
print(lst)         # e.g. [1, 1, 1, 1]
```

### Printing vectors

Vectors can also be printed directly, with their lane values enclosed in angle
brackets:

``` python
v = Vec[int, 4](1)
print(v)  # e.g. "<1,1,1,1>"
```
