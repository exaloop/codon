# numpy.ndgpu

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ndgpu.codon)

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-ndarray-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-ndarray-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: ndarray[dtype, ndim], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-ndarray-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: ndarray[dtype, ndim])
```

class UnaryUFunc[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc "Copy/link to this API element")

@extend

```python
class UnaryUFunc[__name__, F]
```

Extends [`numpy.ufunc.UnaryUFunc`](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UnaryUFunc, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UnaryUFunc)
```

class UnaryUFunc2[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc2 "Copy/link to this API element")

@extend

```python
class UnaryUFunc2[__name__, F]
```

Extends [`numpy.ufunc.UnaryUFunc2`](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc2-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc2-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UnaryUFunc2, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-unaryufunc2-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UnaryUFunc2)
```

class BinaryUFunc[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-binaryufunc "Copy/link to this API element")

@extend

```python
class BinaryUFunc[__name__, F, I]
```

Extends [`numpy.ufunc.BinaryUFunc`](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-binaryufunc-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-binaryufunc-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: BinaryUFunc, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/#class-binaryufunc-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: BinaryUFunc)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/
