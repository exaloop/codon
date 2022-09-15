Codon supports GPU programming through a native GPU backend.
Currently, only Nvidia devices are supported.
Here is a simple example:

``` python
import gpu

@gpu.kernel
def hello(a, b, c):
    i = gpu.thread.x
    c[i] = a[i] + b[i]

a = [i for i in range(16)]
b = [2*i for i in range(16)]
c = [0 for _ in range(16)]

hello(a, b, c, grid=1, block=16)
print(c)
```

which outputs:

```
[0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45]
```

The same code can be written using Codon's `@par` syntax:

``` python
a = [i for i in range(16)]
b = [2*i for i in range(16)]
c = [0 for _ in range(16)]

@par(gpu=True)
for i in range(16):
    c[i] = a[i] + b[i]

print(c)
```

Below is a more comprehensive example for computing the [Mandelbrot
set](https://en.wikipedia.org/wiki/Mandelbrot_set), and plotting it
using NumPy/Matplotlib:

``` python
from python import numpy as np
from python import matplotlib.pyplot as plt
import gpu

MAX    = 1000  # maximum Mandelbrot iterations
N      = 4096  # width and height of image
pixels = [0 for _ in range(N * N)]

def scale(x, a, b):
    return a + (x/N)*(b - a)

@gpu.kernel
def mandelbrot(pixels):
    idx = (gpu.block.x * gpu.block.dim.x) + gpu.thread.x
    i, j = divmod(idx, N)
    c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
    z = 0j
    iteration = 0

    while abs(z) <= 2 and iteration < MAX:
        z = z**2 + c
        iteration += 1

    pixels[idx] = int(255 * iteration/MAX)

mandelbrot(pixels, grid=(N*N)//1024, block=1024)
plt.imshow(np.array(pixels).reshape(N, N))
plt.show()
```

The GPU version of the Mandelbrot code is about 450 times faster
than an equivalent CPU version.

GPU kernels are marked with the `@gpu.kernel` annotation, and
compiled specially in Codon's backend. Kernel functions can
use the vast majority of features supported in Codon, with a
couple notable exceptions:

- Exception handling is not supported inside the kernel, meaning
  kernel code should not throw or catch exceptions. `raise`
  statements inside the kernel are marked as unreachable and
  optimized out.

- Functionality related to I/O is not supported (e.g. you can't
  open a file in the kernel).

- A few other modules and functions are not allowed, such as the
  `re` module (which uses an external regex library) or the `os`
  module.

{% hint style="warning" %}
The GPU module is under active development. APIs and semantics
might change between Codon releases.
{% endhint %}

# Invoking the kernel

The kernel can be invoked via a simple call with added `grid` and
`block` parameters. These parameters define the grid and block
dimensions, respectively. Recall that GPU execution involves a *grid*
of (`X` x `Y` x `Z`) *blocks* where each block contains (`x` x `y` x `z`)
executing threads. Device-specific restrictions on grid and block sizes
apply.

The `grid` and `block` parameters can be one of:

- Single integer `x`, giving dimensions `(x, 1, 1)`
- Tuple of two integers `(x, y)`, giving dimensions `(x, y, 1)`
- Tuple of three integers `(x, y, z)`, giving dimensions `(x, y, z)`
- Instance of `gpu.Dim3` as in `Dim3(x, y, z)`, specifying the three dimensions

# GPU intrinsics

Codon's GPU module provides many of the same intrinsics that CUDA does:

| Codon             | Description                             | CUDA equivalent |
|-------------------|-----------------------------------------|-----------------|
| `gpu.thread.x`    | x-coordinate of current thread in block | `threadId.x`    |
| `gpu.block.x`     | x-coordinate of current block in grid   | `blockIdx.x`    |
| `gpu.block.dim.x` | x-dimension of block                    | `blockDim.x`    |
| `gpu.grid.dim.x`  | x-dimension of grid                     | `gridDim.x`     |

The same applies for the `y` and `z` coordinates. The `*.dim` objects are instances
of `gpu.Dim3`.

# Math functions

All the functions in the `math` module are supported in kernel functions, and
are automatically replaced with GPU-optimized versions:

``` python
import math
import gpu

@gpu.kernel
def hello(x):
    i = gpu.thread.x
    x[i] = math.sqrt(x[i])  # uses __nv_sqrt from libdevice

x = [float(i) for i in range(10)]
hello(x, grid=1, block=10)
print(x)
```

gives:

```
[0, 1, 1.41421, 1.73205, 2, 2.23607, 2.44949, 2.64575, 2.82843, 3]
```

# Libdevice

Codon uses [libdevice](https://docs.nvidia.com/cuda/libdevice-users-guide/index.html)
for GPU-optimized math functions. The default libdevice path is
`/usr/local/cuda/nvvm/libdevice/libdevice.10.bc`. An alternative path can be specified
via the `-libdevice` compiler flag.

# Working with raw pointers

By default, objects are converted entirely to their GPU counterparts, which have
the same data layout as the original objects (although the Codon compiler might perform
optimizations by swapping a CPU implementation of a data type with a GPU-optimized
implementation that exposes the same API). This preserves all of Codon/Python's
standard semantics within the kernel.

It is possible to use a kernel with raw pointers via `gpu.raw`, which corresponds
to how the kernel would be written in C++/CUDA:

``` python
import gpu

@gpu.kernel
def hello(a, b, c):
    i = gpu.thread.x
    c[i] = a[i] + b[i]

a = [i for i in range(16)]
b = [2*i for i in range(16)]
c = [0 for _ in range(16)]

# call the kernel with three int-pointer arguments:
hello(gpu.raw(a), gpu.raw(b), gpu.raw(c), grid=1, block=16)
print(c)  # output same as first snippet's
```

`gpu.raw` can avoid an extra pointer indirection, but outputs a Codon `Ptr` object,
meaning the corresponding kernel parameters will not have the full list API, instead
having the more limited `Ptr` API (which primarily just supports indexing/assignment).

# Object conversions

A hidden API is used to copy objects to and from the GPU device. This API consists of
two new *magic methods*:

- `__to_gpu__(self)`: Allocates the necessary GPU memory and copies the object `self` to
  the device.

- `__from_gpu__(self, gpu_object)`: Copies the GPU memory of `gpu_object` (which is
  a value returned by `__to_gpu__`) back to the CPU object `self`.

For primitive types like `int` and `float`, `__to_gpu__` simply returns `self` and
`__from_gpu__` does nothing. These methods are defined for all the built-in types *and*
are automatically generated for user-defined classes, so most objects can be transferred
back and forth from the GPU seamlessly. A user-defined class that makes use of raw pointers
or other low-level constructs will have to define these methods for GPU use. Please refer
to the `gpu` module for implementation examples.

# `@par(gpu=True)`

Codon's `@par` syntax can be used to seamlessly parallelize existing loops on the GPU,
without needing to explicitly write them as kernels. For loop nests, the `collapse` argument
can be used to cover the entire iteration space on the GPU. For example, here is the Mandelbrot
code above written using `@par`:

``` python
MAX    = 1000  # maximum Mandelbrot iterations
N      = 4096  # width and height of image
pixels = [0 for _ in range(N * N)]

def scale(x, a, b):
    return a + (x/N)*(b - a)

@par(gpu=True, collapse=2)
for i in range(N):
    for j in range(N):
        c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
        z = 0j
        iteration = 0

        while abs(z) <= 2 and iteration < MAX:
            z = z**2 + c
            iteration += 1

        pixels[i*N + j] = int(255 * iteration/MAX)
```

Note that the `gpu=True` option disallows shared variables (i.e. assigning out-of-loop
variables in the loop body) as well as reductions. The other GPU-specific restrictions
described here apply as well.

# Troubleshooting

CUDA errors resulting in kernel abortion are printed, and typically arise from invalid
code in the kernel, either via using exceptions or using unsupported modules/objects.
