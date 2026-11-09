# numpy.npdatetime

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npdatetime.codon)

class DatetimeMetaData[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata "Copy/link to this API element")

@tuple

```python
class DatetimeMetaData
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/)

Fields 2

`base` `i32`

`num` `i32`

class timedelta64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64 "Copy/link to this API element")

@tuple

```python
class timedelta64[base, num]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/)

Fields 3

`value` `int`

`base` `Literal[str]`

`num` `Literal[int]`

class datetime64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64 "Copy/link to this API element")

@tuple

```python
class datetime64[base, num]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/)

Fields 3

`value` `int`

`base` `Literal[str]`

`num` `Literal[int]`

class datetimestruct[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct "Copy/link to this API element")

```python
class datetimestruct
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/)

Fields 9

`year` `int`

`month` `i32`

`day` `i32`

`hour` `i32`

`min` `i32`

`sec` `i32`

`us` `i32`

`ps` `i32`

`as_` `i32`

class DatetimeMetaData[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata "Copy/link to this API element")

@extend

```python
class DatetimeMetaData
```

Extends [`numpy.npdatetime.DatetimeMetaData`](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-__new__ "Copy/link to this API element")

```python
__new__(base: int, num: int = 1)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-__new__-overload-2 "Copy/link to this API element")

```python
__new__(base: Literal[str], num: int = 1)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-__new__-overload-3 "Copy/link to this API element")

```python
__new__(dt: datetime64)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-__new__-overload-4 "Copy/link to this API element")

```python
__new__(td: timedelta64)
```

Methods 2

divisor\_to\_multiple[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-divisor_to_multiple "Copy/link to this API element")

```python
divisor_to_multiple(self: Self, den: int, metastr: str, Self: type)
```

conversion\_factor[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimemetadata-conversion_factor "Copy/link to this API element")

```python
conversion_factor(self: Self, dst: DatetimeMetaData, Self: type)
```

class timedelta64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64 "Copy/link to this API element")

@extend

```python
class timedelta64[base, num]
```

Extends [`numpy.npdatetime.timedelta64`](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/timedelta64/)

Constructors 8

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value: int, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-3 "Copy/link to this API element")

```python
__new__(td: timedelta64, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-4 "Copy/link to this API element")

```python
__new__(s: str, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-5 "Copy/link to this API element")

```python
__new__(value: int, unit: Literal[str])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-6 "Copy/link to this API element")

```python
__new__(td: timedelta64, unit: Literal[str] = "generic")
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-7 "Copy/link to this API element")

```python
__new__(s: str, unit: Literal[str] = "generic")
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__new__-overload-8 "Copy/link to this API element")

```python
__new__(td: datetime.timedelta, unit: Literal[str] = "us")
```

Methods 3

tolist[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

cdiv[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: timedelta64, Self: type)
```

cmod[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: timedelta64, Self: type)
```

Special methods 22

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: timedelta64, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: timedelta64, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: timedelta64, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: timedelta64, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: timedelta64, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: timedelta64, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

```python
__add__(self: Self, other: timedelta64, Self: type)
```

```python
__add__(self: Self, other: datetime64, Self: type)
```

```python
__add__(self: Self, other: int, Self: type)
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

```python
__radd__(self: Self, other: int, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

```python
__sub__(self: Self, other: timedelta64, Self: type)
```

```python
__sub__(self: Self, other: int, Self: type)
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other, Self: type)
```

```python
__rsub__(self: Self, other: int, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other, Self: type)
```

```python
__mul__(self: Self, other: float, Self: type)
```

```python
__mul__(self: Self, other: int, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other, Self: type)
```

```python
__rmul__(self: Self, other: float, Self: type)
```

```python
__rmul__(self: Self, other: int, Self: type)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other, Self: type)
```

```python
__floordiv__(self: Self, other: timedelta64, Self: type)
```

```python
__floordiv__(self: Self, other: float, Self: type)
```

```python
__floordiv__(self: Self, other: int, Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

```python
__truediv__(self: Self, other: timedelta64, Self: type)
```

```python
__truediv__(self: Self, other: float, Self: type)
```

```python
__truediv__(self: Self, other: int, Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: timedelta64, Self: type)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: timedelta64, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-timedelta64-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class datetime64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64 "Copy/link to this API element")

@extend

```python
class datetime64[base, num]
```

Extends [`numpy.npdatetime.datetime64`](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetime64/)

Constructors 9

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value: int, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-3 "Copy/link to this API element")

```python
__new__(value: datetime64, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-4 "Copy/link to this API element")

```python
__new__(s: str, base: Literal[str], num: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-5 "Copy/link to this API element")

```python
__new__(value: int, unit: Literal[str])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-6 "Copy/link to this API element")

```python
__new__(dt: datetime64, unit: Literal[str])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-7 "Copy/link to this API element")

```python
__new__(s: str, unit: Literal[str] = "us")
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-8 "Copy/link to this API element")

```python
__new__(dt: datetime.datetime, unit: Literal[str] = "us")
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__new__-overload-9 "Copy/link to this API element")

```python
__new__(dt: datetime.date, unit: Literal[str] = "D")
```

Methods 1

tolist[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

Special methods 12

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: datetime64, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: datetime64, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: datetime64, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: datetime64, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: datetime64, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: datetime64, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

```python
__add__(self: Self, other: timedelta64, Self: type)
```

```python
__add__(self: Self, other: int, Self: type)
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

```python
__radd__(self: Self, other: int, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

```python
__sub__(self: Self, other: timedelta64, Self: type)
```

```python
__sub__(self: Self, other: datetime64, Self: type)
```

```python
__sub__(self: Self, other: int, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetime64-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class datetimestruct[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct "Copy/link to this API element")

@extend

```python
class datetimestruct
```

Extends [`numpy.npdatetime.datetimestruct`](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/)

Constructors 3

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, s: str, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, dt: datetime.datetime, Self: type)
```

Properties 3

datetime[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-datetime "Copy/link to this API element")

@property

```python
datetime(self: Self, Self: type)
```

days[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-days "Copy/link to this API element")

@property

```python
days(self: Self, Self: type)
```

minutes[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-minutes "Copy/link to this API element")

@property

```python
minutes(self: Self, Self: type)
```

Methods 10

reset[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-reset "Copy/link to this API element")

```python
reset(self: Self, Self: type)
```

copy\_to[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-copy_to "Copy/link to this API element")

```python
copy_to(self: Self, other: datetimestruct, Self: type)
```

set\_days[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-set_days "Copy/link to this API element")

```python
set_days(self: Self, days: int, Self: type)
```

add\_minutes[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-add_minutes "Copy/link to this API element")

```python
add_minutes(self: Self, minutes: i32, Self: type)
```

to\_datetime64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-to_datetime64 "Copy/link to this API element")

```python
to_datetime64(self: Self, meta: DatetimeMetaData, Self: type)
```

from\_datetime64[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-from_datetime64 "Copy/link to this API element")

```python
from_datetime64(self: Self, meta: DatetimeMetaData, dt: int, Self: type)
```

utc\_to\_local[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-utc_to_local "Copy/link to this API element")

```python
utc_to_local(self: Self, out: datetimestruct, Self: type)
```

lossless\_unit[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-lossless_unit "Copy/link to this API element")

```python
lossless_unit(self: Self, Self: type)
```

parse\_iso8601[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-parse_iso8601 "Copy/link to this API element")

```python
parse_iso8601(self: Self, s: str, Self: type)
```

to\_str[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-to_str "Copy/link to this API element")

```python
to_str(self: Self, unit: int, local: bool = False, utc: bool = False, tzoffset: int = -1, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-datetimestruct-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class busdaycalendar[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-busdaycalendar "Copy/link to this API element")

```python
class busdaycalendar
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/busdaycalendar/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/#class-busdaycalendar-__init__ "Copy/link to this API element")

```python
__init__(self: Self, weekmask = "1111100", holidays = None, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/
