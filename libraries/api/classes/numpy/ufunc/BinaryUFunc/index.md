# BinaryUFunc

Standard library class `numpy.ufunc.BinaryUFunc` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ufunc.codon#L539)

class BinaryUFunc[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc "Copy/link to this API element")

@tuple

```python
class BinaryUFunc[__name__, F, I]
```

Fields 3

`identity` `I`

`F` `type`

`I` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-__new__ "Copy/link to this API element")

```python
__new__[name: Literal[str], F: type, I: type](op: F, identity: I = None) -> BinaryUFunc[(name, F, I)]
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Properties 2

nin[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

nout[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Methods 4

reduce[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-reduce "Copy/link to this API element")

```python
reduce[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, array, axis = 0, out = None, initial = util._NoValue(), where = True)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

accumulate[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-accumulate "Copy/link to this API element")

```python
accumulate[dtype: type = NoneType](self: Self, array, axis: int = 0, out = None)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

at[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices, b)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

outer[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-outer "Copy/link to this API element")

```python
outer[dtype: type = NoneType](self: Self, A, B)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

Special methods 4

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-__call__ "Copy/link to this API element")

```python
__call__[dtype: type = NoneType](self: Self, x, y, out = None, where = True)
```

Defined in [`numpy.ufunc`](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: BinaryUFunc)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/#class-binaryufunc-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: BinaryUFunc)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/
