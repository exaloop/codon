Codon ships with a feature-complete, fully-compiled native NumPy implementation.
It uses the same API as NumPy, but re-implements everything in Codon itself,
allowing for a range of optimizations and performance improvements. Codon-NumPy
works with Codon's Python interoperability (you can transfer arrays to and from
regular Python seamlessly), parallel backend (you can do array operations in
parallel), and GPU backend (you can transfer arrays to and from the GPU seamlessly,
and operate on them on the GPU).

# Getting started
Importing `numpy` in Codon will use Codon-NumPy (as opposed to
`from python import numpy`, which would use standard NumPy):

``` python
import numpy as np
```

We can then create and manipulate arrays just like in standard NumPy:

``` python
x = np.arange(15, dtype=np.int64).reshape(3, 5)
print(x)
#   0   1   2   3   4
#   5   6   7   8   9
#  10  11  12  13  14

x[1:, ::2] = -99
#   0   1   2   3   4
# -99   6 -99   8 -99
# -99  11 -99  13 -99

y = x.max(axis=1)
print(y)
#   4   8  13
```

In Codon-NumPy, any Codon type can be used as the array type. The `numpy`
module has the same aliases that regular NumPy has, like `np.int64`,
`np.float32` etc., but these simply refer to the regular Codon types.

{% hint style="warning" %}
Using a string (e.g. `"i4"` or `"f8"`) for the dtype is not yet supported.
{% endhint %}

# Codon array type
The Codon array type is parameterized by the array data type ("`dtype`")
and the array dimension ("`ndim`"). That means that, in Codon-NumPy, the
array dimension is a property of the type, so a 1-d array is a different
type than a 2-d array and so on:

``` python
import numpy as np

arr = np.array([[1.1, 2.2], [3.3, 4.4]])
print(arr.__class__.__name__)  # ndarray[float,2]

arr = np.arange(10)
print(arr.__class__.__name__)  # ndarray[int,1]
```

The array dimension must also be known at compile-time. This allows the
compiler to perform a wider range of optimizations on array operations.
Usually, this has no impact on the code as the NumPy functions can
determine input and output dimensions automatically. However, the dimension
(and dtype) must be given when, for instance, reading arrays from disk:

``` python
# 'dtype' argument specifies array type
# 'ndim' argument specifies array dimension
arr = np.load('arr.npy', dtype=float, ndim=3)
```

A very limited number of NumPy functions return an array whose dimension
cannot be deduced from its inputs. One such example is `squeeze()`, which
removes axes of length 1; since the number of axes of length 1 is not
determinable at compile-time, this function requires an extra argument that
indicates which axes to remove.

# Python interoperability
Codon's `ndarray` type supports Codon's standard Python interoperability API
(i.e. `__to_py__` and `__from_py__` methods), so arrays can be transferred to
and from Python seamlessly.

## PyTorch integration
Because PyTorch tensors and NumPy arrays are interchangeable without copying
data, it is easy to use Codon to efficiently manipulate or operate on PyTorch
tensors. This can be achieved either via Codon's just-in-time (JIT) compilation
mode or via its Python extension mode.

### Using Codon JIT
Here is an example showing initializing a $$128 \times 128 \times 128$$ tensor
$$A$$ such that $$A_{i,j,k} = i + j + k$$:

``` python
import numpy as np
import time
import codon
import torch

@codon.jit
def initialize(arr):
    for i in range(128):
        for j in range(128):
            for k in range(128):
                arr[i, j, k] = i + j + k

# first call JIT-compiles; subsequent calls use cached JIT'd code
tensor = torch.empty(128, 128, 128)
initialize(tensor.numpy())

tensor = torch.empty(128, 128, 128)
t0 = time.time()
initialize(tensor.numpy())
t1 = time.time()

print(tensor)
print(t1 - t0, 'seconds')
```

Timings on an M1 MacBook Pro:

- Without `@codon.jit`: 0.1645 seconds
- With `@codon.jit`: 0.001485 seconds (*110x speedup*)

For more information, see the [Codon JIT docs](../interop/decorator.md).

### Using Codon Python extensions
Codon can compile directly to a Python extension module, similar to writing a C
extension for CPython or using Cython.

Taking the same example, we can create a file `init.py`:

``` python
import numpy as np
import numpy.pybridge

def initialize(arr: np.ndarray[np.float32, 3]):
    for i in range(128):
        for j in range(128):
            for k in range(128):
                arr[i, j, k] = i + j + k
```

Note that extension module functions need to specify argument types. In this case,
the argument is a 3-dimensional array of type `float32`, which is expressed as
`np.ndarray[np.float32, 3]` in Codon.

Now we can use a setup script `setup.py` to create the extension module as described
in the [Codon Python extension docs](../interop/pyext.md):

``` bash
python3 setup.py build_ext --inplace  # setup.py from docs linked above
```

Finally, we can call the function from Python:

``` python
from codon_initialize import initialize
import torch

tensor = torch.empty(128, 128, 128)
initialize(tensor.numpy())
print(tensor)
```

Note that there is no compilation happening at runtime with this approach. Instead,
everything is compiled ahead of time when creating the extension. The timing is the same
as the first approach.

You can also use any Codon compilation flags with this approach by adding them to the
`spawn` call in the setup script. For example, you can use the `-disable-exceptions`
flag to disable runtime exceptions, which can yield performance improvements and generate
more streamlined code.

# Parallel processing
Unlike Python, Codon has no global interpreter lock ("GIL") and supports full
multithreading, meaning NumPy code can be parallelized. For example:

``` python
import numpy as np
import numpy.random as rnd
import time

N = 100000000
n = 10

rng = rnd.default_rng(seed=0)
x = rng.normal(size=(N,n))
y = np.empty(n)

t0 = time.time()

@par(num_threads=n)
for i in range(n):
    y[i] = x[:,i].sum()

t1 = time.time()

print(y)
print(t1 - t0, 'seconds')
# no par - 1.4s
# w/ par - 0.4s
```

# GPU processing
Codon-NumPy supports seamless GPU processing: arrays can be passed to and
from the GPU, and array operations can be performed on the GPU using Codon's
GPU backend. Here's an example that computes the Mandelbrot set:

``` python
import numpy as np
import gpu

MAX    = 1000  # maximum Mandelbrot iterations
N      = 4096  # width and height of image
pixels = np.empty((N, N), int)

def scale(x, a, b):
    return a + (x/N)*(b - a)

@gpu.kernel
def mandelbrot(pixels):
    i = (gpu.block.x * gpu.block.dim.x) + gpu.thread.x
    j = (gpu.block.y * gpu.block.dim.y) + gpu.thread.y
    c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
    z = 0j
    iteration = 0

    while abs(z) <= 2 and iteration < MAX:
        z = z**2 + c
        iteration += 1

    pixels[i, j] = 255 * iteration/MAX

mandelbrot(pixels, grid=(N//32, N//32), block=(32, 32))
```

Here is the same code using GPU-parallelized `for`-loops:

``` python
import numpy as np
import gpu

MAX    = 1000  # maximum Mandelbrot iterations
N      = 4096  # width and height of image
pixels = np.empty((N, N), int)

def scale(x, a, b):
    return a + (x/N)*(b - a)

@par(gpu=True, collapse=2)  # <--
for i in range(N):
    for j in range(N):
        c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
        z = 0j
        iteration = 0

        while abs(z) <= 2 and iteration < MAX:
            z = z**2 + c
            iteration += 1

        pixels[i, j] = 255 * iteration/MAX
```

# Linear algebra
Codon-NumPy fully supports the NumPy linear algebra module which provides
a comprehensive set of functions for linear algebra operations. Importing
the linear algebra module, just like in standard NumPy:

``` python
import numpy.linalg as LA
```

For example, the `eig()` function computes the eigenvalues and eigenvectors
of a square matrix:

``` python
eigenvalues, eigenvectors = LA.eig(np.diag((1, 2, 3)))
print(eigenvalues)
# 1.+0.j 2.+0.j 3.+0.j

print(eigenvectors)
# [[1.+0.j 0.+0.j 0.+0.j]
#  [0.+0.j 1.+0.j 0.+0.j]
#  [0.+0.j 0.+0.j 1.+0.j]]
```

Just like standard NumPy, Codon will use an optimized BLAS library under the
hood to implement many linear algebra operations. This defaults to OpenBLAS
on Linux and Apple's Accelerate framework on macOS.

Because Codon supports full multithreading, it's possible to use outer-loop
parallelism to perform linear algebra operations in parallel. Here's an example
that multiplies several matrices in parallel:

``` python
import numpy as np
import numpy.random as rnd
import time

N = 5000
n = 10
rng = rnd.default_rng(seed=0)
a = rng.normal(size=(n, N, N))
b = rng.normal(size=(n, N, N))
y = np.empty((n, N, N))
t0 = time.time()

@par(num_threads=n)
for i in range(n):
    y[i, :, :] = a[i, :, :] @ b[i, :, :]

t1 = time.time()
print(y.sum())
print(t1 - t0, 'seconds')  # Python - 53s
                           # Codon  -  6s
```

{% hint style="warning" %}
When using Codon's outer-loop parallelism, make sure to set the environment
variable `OPENBLAS_NUM_THREADS` to 1 (i.e. `export OPENBLAS_NUM_THREADS=1`)
to avoid conflicts with OpenBLAS multithreading.
{% endhint %}

# NumPy-specific compiler optimizations
Codon includes compiler passes that optimize NumPy code through methods like
operator fusion, which combine distinct operations so that they can be executed
during a single pass through the argument arrays, saving both execution time and
memory (since intermediate arrays no longer need to be allocated).

To showcase this, here's a simple NumPy program that approximates $$\pi$$.
The code below generates two random vectors $$x$$ and $$y$$ with entries in the
range $$[0, 1)$$ and computes the fraction of pairs of points that lie in the
circle of radius $$0.5$$ centered at $$(0.5, 0.5)$$, which is approximately
$$\pi \over 4$$.

``` python
import time
import numpy as np

rng = np.random.default_rng(seed=0)
x = rng.random(500_000_000)
y = rng.random(500_000_000)

t0 = time.time()
# pi ~= 4 x (fraction of points in circle)
pi = ((x-1)**2 + (y-1)**2 < 1).sum() * (4 / len(x))
t1 = time.time()

print(pi)
print(t1 - t0, 'seconds')
```

The expression `(x-1)**2 + (y-1)**2 < 1` gets fused by Codon so that it is
executed in just a single pass over the `x` and `y` arrays, rather than in
multiple passes for each sub-expression `x-1`, `y-1` etc. as is the case with
standard NumPy.

Here are the resulting timings on an M1 MacBook Pro:

- Python / standard NumPy: 2.4 seconds
- Codon: 0.42 seconds (*6x speedup*)

You can display information about fused expressions by using the `-npfuse-verbose`
flag of `codon`, as in `codon run -release -npfuse-verbose pi.py`. Here's the
output for the program above:

```
Optimizing expression at pi.py:10:7
lt <array[bool, 1]> [cost=6]
  add <array[f64, 1]> [cost=5]
    pow <array[f64, 1]> [cost=2]
      sub <array[f64, 1]> [cost=1]
        a0 <array[f64, 1]>
        a1 <i64>
      a2 <i64>
    pow <array[f64, 1]> [cost=2]
      sub <array[f64, 1]> [cost=1]
        a3 <array[f64, 1]>
        a4 <i64>
      a5 <i64>
  a6 <i64>

-> static fuse:
lt <array[bool, 1]> [cost=6]
  add <array[f64, 1]> [cost=5]
    pow <array[f64, 1]> [cost=2]
      sub <array[f64, 1]> [cost=1]
        a0 <array[f64, 1]>
        a1 <i64>
      a2 <i64>
    pow <array[f64, 1]> [cost=2]
      sub <array[f64, 1]> [cost=1]
        a3 <array[f64, 1]>
        a4 <i64>
      a5 <i64>
  a6 <i64>
```

As shown, the optimization pass employs a cost model to decide how to best handle
a given expression, be it by fusing or evaluating sequentially. You can adjust the
fusion cost thresholds via the following flags:

- `-npfuse-always <cost1>`: Expression cost below which to always fuse a given
  expression (default: `10`).
- `-npfuse-never <cost2>`: Expression cost above which (>) to never fuse a given
  expression (default: `50`).

Given an expression cost `C`, the logic implemented in the pass is to:

- Always fuse expressions where `C <= cost1`.
- Fuse expressions where `cost1 < C <= cost2` if there is no broadcasting involved.
- Never fuse expressions where `C > cost2` and instead evaluate them sequentially.

This logic is applied recursively to a given expression to determine the optimal
evaluation strategy.

You can disable these optimizations altogether by disabling the corresponding
compiler pass via the flag `-disable-opt core-numpy-fusion`.

# I/O
Codon-NumPy supports most of NumPy's I/O API. One important difference, however,
is that I/O functions must specify the dtype and dimension of arrays being read,
since Codon-NumPy array types are parameterized by dtype and dimension:

``` python
import numpy as np

a = np.arange(27, dtype=np.int16).reshape(3, 3, 3)
np.save('arr.npy', a)

# Notice the 'dtype' and 'ndim' arguments:
b = np.load('arr.npy', dtype=np.int16, ndim=3)
```

Writing arrays has no such requirement.

# Datetimes
Codon-NumPy fully supports NumPy's datetime types: `datetime64` and `timedelta64`.
One difference from standard NumPy is how these types are specified. Here's an example:

``` python
# datetime64 type with units of "1 day"
# same as "dtype='datetime64[D]'" in standard NumPy
dt = np.array(['2020-01-02', '2021-09-15', '2022-07-01'],
              dtype=np.datetime64['D', 1])

# timedelta64 type with units of "15 minutes"
# same as "dtype='timedelta64[15m]'" in standard NumPy
td = np.array([100, 200, 300], dtype=np.timedelta64['m', 15])
```

# Passing array data to C/C++
You can pass an `ndarray`'s underlying data pointer to a C/C++ function by using
the `data` attribute of the array. For example:

``` python
from C import foo(p: Ptr[float], n: int)

arr = np.ndarray([1.0, 2.0, 3.0])
foo(arr.data, arr.size)
```

Of course, it's the caller's responsibility to make sure the array is contiguous
as needed and/or pass additional shape or stride information. See the
[C interoperability](../interop/cpp.md) docs for more information.

## Array ABI

The `ndarray[dtype, ndim]` data structure has three fields, in the following order:

- `shape`: length-`ndim` tuple of non-negative 64-bit integers representing the array
  shape
- `strides`: length-`ndim` tuple of 64-bit integers representing the stride in bytes
  along each axis of the array
- `data`: pointer of type `dtype` to the array's data

For example, `ndarray[np.float32, 3]` would correspond to the following C structure:

``` c
struct ndarray_float32_3 {
  int64_t shape[3];
  int64_t strides[3];
  float *data;
};
```

This can be used to pass an entire `ndarray` object to a C function without breaking
it up into its constituent components.

# Performance tips

## Array layouts
As with standard NumPy, Codon-NumPy performs best when array data is contiguous in
memory, ideally in row-major order (also called "C order"). Most NumPy functions will
return C-order arrays, but operations like slicing and transposing arrays can alter
contiguity. You can use `numpy.ascontiguousarray()` to create a contiguous array from
an arbitrary array.

## Linux huge pages
When working with large arrays on Linux, enabling
[transparent hugepages](https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html)
can result in significant performance improvements.

You can check if transparent hugepages are enabled via

``` bash
cat /sys/kernel/mm/transparent_hugepage/enabled
```

and you can enable them via

``` bash
echo "always" | sudo tee /sys/kernel/mm/transparent_hugepage/enabled
```

## Disabling exceptions
By default, Codon performs various validation checks at runtime (e.g. bounds checks when
indexing an array) just like standard NumPy, and raises an exception if they fail. If
you know your program will not raise or catch any exceptions, you can disable these
checks through the `-disable-exceptions` compiler flag.

Note that when using this flag, raising an exception will terminate the process with a
`SIGTRAP`.

## Fast-math
You can enable "fast-math" optimizations via the `-fast-math` compiler flag. It is
advisable to **use this flag with caution** as it changes floating point semantics and
makes assumptions regarding `inf` and `nan` values. For more information, consult LLVM's
documentation on [fast-math flags](https://llvm.org/docs/LangRef.html#fast-math-flags).

# Not-yet-supported
The following features of NumPy are not yet supported, but are planned for the future:
- String operations
- Masked arrays
- Polynomials

A few miscellaneous Python-specific functions like `get_include()` are also not
supported, as they are not applicable in Codon.
