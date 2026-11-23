# NanSumRedux

Standard library class `numpy.reductions.NanSumRedux` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/reductions.codon#L1165)

class NanSumRedux[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux "Copy/link to this API element")

```python
class NanSumRedux[T]
```

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Methods 8

create[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

accept[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

result[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-result "Copy/link to this API element")

```python
result(self: Self, count: int)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

empty[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

done[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-done "Copy/link to this API element")

```python
done(self: Self)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

gradual\_init[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

gradual\_accept[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

loop[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/#class-nansumredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/reductions/NanSumRedux/
