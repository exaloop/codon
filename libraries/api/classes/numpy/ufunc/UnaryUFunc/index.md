# UnaryUFunc

Standard library class `numpy.ufunc.UnaryUFunc` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ufunc.codon)

class UnaryUFunc[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc "Copy/link to this API element")

@tuple

```python
class UnaryUFunc[__name__, F]
```

Fields 1

`F` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-__new__ "Copy/link to this API element")

```python
__new__(op: F, name: Literal[str], F: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Properties 2

nin[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

nout[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Methods 1

at[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Special methods 4

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, out = None, where = True, dtype: type = NoneType, Self: type)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UnaryUFunc, Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/#class-unaryufunc-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UnaryUFunc)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/
