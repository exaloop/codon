# PTPRedux

Standard library class `numpy.reductions.PTPRedux` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/reductions.codon#L1501)

class PTPRedux[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux "Copy/link to this API element")

```python
class PTPRedux[T]
```

Fields 3

`hi` `Optional[T]`

`lo` `Optional[T]`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-__init__ "Copy/link to this API element")

```python
__init__(self: Self, **kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Methods 6

create[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-create "Copy/link to this API element")

```python
create[T: type](**kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

accept[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-accept "Copy/link to this API element")

```python
accept(self: Self, item: T, index: int)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

result[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-result "Copy/link to this API element")

```python
result(self: Self, count: int) -> T
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

empty[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-empty "Copy/link to this API element")

```python
empty(**kwargs)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

done[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-done "Copy/link to this API element")

```python
done(self: Self)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

loop[](https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/#class-ptpredux-loop "Copy/link to this API element")

```python
loop[partial: Literal[bool], S: type](self: Self, a: Ptr[S], n: int, stride: int)
```

Defined in [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/reductions/PTPRedux/
