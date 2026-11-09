# time

Standard library class `datetime.time` [View source](https://github.com/exaloop/codon/blob/master/stdlib/datetime.codon)

class time[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time "Copy/link to this API element")

@tuple

```python
class time
```

Fields 3

`min` `ClassVar[time]`

`max` `ClassVar[time]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-__new__ "Copy/link to this API element")

```python
__new__(hour: int = 0, minute: int = 0, second: int = 0, microsecond: int = 0)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Properties 4

hour[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-hour "Copy/link to this API element")

@property

```python
hour(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

minute[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-minute "Copy/link to this API element")

@property

```python
minute(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

second[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-second "Copy/link to this API element")

@property

```python
second(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

microsecond[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-microsecond "Copy/link to this API element")

@property

```python
microsecond(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Methods 3

fromisoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-fromisoformat "Copy/link to this API element")

```python
fromisoformat(time_string: str)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

replace[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-replace "Copy/link to this API element")

```python
replace(self: Self, hour: int = -1, minute: int = -1, second: int = -1, microsecond: int = -1, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-isoformat "Copy/link to this API element")

```python
isoformat(self: Self, timespec: Literal[str] = "auto", Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Special methods 3

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/time/#class-time-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Source: https://docs.exaloop.io/libraries/api/classes/datetime/time/
