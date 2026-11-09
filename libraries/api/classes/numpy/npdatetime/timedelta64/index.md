# timedelta64

Standard library class `numpy.npdatetime.timedelta64` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npdatetime.codon)

class timedelta64[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64 "Copy/link to this API element")

@tuple

```python
class timedelta64[base, num]
```

Fields 3

`value` `int`

`base` `Literal[str]`

`num` `Literal[int]`

Constructors 8

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__ "Copy/link to this API element")

```python
__new__()
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value: int, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-3 "Copy/link to this API element")

```python
__new__(td: timedelta64, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-4 "Copy/link to this API element")

```python
__new__(s: str, base: Literal[str], num: Literal[int])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-5 "Copy/link to this API element")

```python
__new__(value: int, unit: Literal[str])
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-6 "Copy/link to this API element")

```python
__new__(td: timedelta64, unit: Literal[str] = "generic")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-7 "Copy/link to this API element")

```python
__new__(s: str, unit: Literal[str] = "generic")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__new__-overload-8 "Copy/link to this API element")

```python
__new__(td: datetime.timedelta, unit: Literal[str] = "us")
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Methods 3

tolist[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

cdiv[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

cmod[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Special methods 22

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__add__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__add__(self: Self, other: datetime64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__add__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__radd__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__sub__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__sub__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__rsub__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__mul__(self: Self, other: float, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__mul__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__rmul__(self: Self, other: float, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__rmul__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__floordiv__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__floordiv__(self: Self, other: float, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__floordiv__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__truediv__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__truediv__(self: Self, other: float, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

```python
__truediv__(self: Self, other: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: timedelta64, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/#class-timedelta64-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/
