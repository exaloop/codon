# timedelta

Standard library class `datetime.timedelta` [View source](https://github.com/exaloop/codon/blob/master/stdlib/datetime.codon)

class timedelta[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta "Copy/link to this API element")

@tuple

```python
class timedelta
```

Fields 3

`min` `ClassVar[timedelta]`

`max` `ClassVar[timedelta]`

`resolution` `ClassVar[timedelta]`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__new__ "Copy/link to this API element")

```python
__new__(days: float = 0, seconds: float = 0, microseconds: float = 0, milliseconds: float = 0, minutes: float = 0, hours: float = 0, weeks: float = 0)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__new__-overload-2 "Copy/link to this API element")

```python
__new__(days: int)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Properties 3

days[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-days "Copy/link to this API element")

@property

```python
days(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

seconds[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-seconds "Copy/link to this API element")

@property

```python
seconds(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

microseconds[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-microseconds "Copy/link to this API element")

@property

```python
microseconds(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Methods 1

total\_seconds[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-total_seconds "Copy/link to this API element")

```python
total_seconds(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Special methods 20

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__add__(self: Self, other: date, Self: type)
```

Added by [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__add__(self: Self, other: datetime, Self: type)
```

Added by [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: int, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__mul__(self: Self, other: float, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: int, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__rmul__(self: Self, other: float, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__truediv__(self: Self, other: float, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__truediv__(self: Self, other: int, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__floordiv__(self: Self, other: int, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/#class-timedelta-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Source: https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/
