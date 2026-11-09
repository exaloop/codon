# SumRedux

Standard library class `numpy.reductions.SumRedux` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/reductions.codon)

class SumRedux[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux "Copy/link to this API element")

```python
class SumRedux[T]
```

Fields 2

`total` `T`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Methods 8

create[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-create "Copy/link to this API element")

```python
create(T: type, **kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

accept[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-accept "Copy/link to this API element")

```python
accept(self: Self, item, index: int, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

result[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-result "Copy/link to this API element")

```python
result(self: Self, count: int, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

empty[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

done[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-done "Copy/link to this API element")

```python
done(self: Self, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

gradual\_init[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-gradual_init "Copy/link to this API element")

```python
gradual_init(self: Self, **kwargs, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

gradual\_accept[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-gradual_accept "Copy/link to this API element")

```python
gradual_accept(self: Self, curr, item, index: int, **kwargs, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

loop[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/#class-sumredux-loop "Copy/link to this API element")

```python
loop(self: Self, a: Ptr[S], n: int, stride: int, partial: Literal[bool], S: type, Self: type)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/reductions/SumRedux/
