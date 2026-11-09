# datetime64

Standard library class `numpy.npdatetime.datetime64` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npdatetime.codon)

class datetime64[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64 "Copy/link to this API element")

@tuple

```python
class datetime64[base, num]
```

Fields 3

`value` `int`

`base` `Literal[str]`

`num` `Literal[int]`

Constructors 9

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__ "Copy/link to this API element")

```python
__new__()
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value: int, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-3 "Copy/link to this API element")

```python
__new__(value: datetime64, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-4 "Copy/link to this API element")

```python
__new__(s: str, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-5 "Copy/link to this API element")

```python
__new__(value: int, unit: Literal[str])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-6 "Copy/link to this API element")

```python
__new__(dt: datetime64, unit: Literal[str])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-7 "Copy/link to this API element")

```python
__new__(s: str, unit: Literal[str] = "us")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-8 "Copy/link to this API element")

```python
__new__(dt: datetime.datetime, unit: Literal[str] = "us")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__new__-overload-9 "Copy/link to this API element")

```python
__new__(dt: datetime.date, unit: Literal[str] = "D")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Methods 1

tolist[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Special methods 12

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__add__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__add__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__radd__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__sub__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__sub__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__sub__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/#class-datetime64-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/
