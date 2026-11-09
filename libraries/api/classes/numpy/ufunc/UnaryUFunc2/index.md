# UnaryUFunc2

Standard library class `numpy.ufunc.UnaryUFunc2` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ufunc.codon)

class UnaryUFunc2[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2 "Copy/link to this API element")

@tuple

```python
class UnaryUFunc2[__name__, F]
```

Fields 1

`F` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-__new__ "Copy/link to this API element")

```python
__new__(op: F, name: Literal[str], F: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Properties 2

nin[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-nin "Copy/link to this API element")

@property

```python
nin(self: Self, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

nout[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-nout "Copy/link to this API element")

@property

```python
nout(self: Self, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Special methods 4

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, out1 = None, out2 = None, out = None, where = True, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UnaryUFunc2, Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/#class-unaryufunc2-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UnaryUFunc2)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/
