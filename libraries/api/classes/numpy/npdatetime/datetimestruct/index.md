# datetimestruct

Standard library class `numpy.npdatetime.datetimestruct` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npdatetime.codon)

class datetimestruct[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct "Copy/link to this API element")

```python
class datetimestruct
```

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

Constructors 3

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, s: str, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, dt: datetime.datetime, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Properties 3

datetime[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-datetime "Copy/link to this API element")

@property

```python
datetime(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

days[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-days "Copy/link to this API element")

@property

```python
days(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

minutes[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-minutes "Copy/link to this API element")

@property

```python
minutes(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Methods 10

reset[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-reset "Copy/link to this API element")

```python
reset(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

copy\_to[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-copy_to "Copy/link to this API element")

```python
copy_to(self: Self, other: datetimestruct, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

set\_days[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-set_days "Copy/link to this API element")

```python
set_days(self: Self, days: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

add\_minutes[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-add_minutes "Copy/link to this API element")

```python
add_minutes(self: Self, minutes: i32, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

to\_datetime64[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-to_datetime64 "Copy/link to this API element")

```python
to_datetime64(self: Self, meta: DatetimeMetaData, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

from\_datetime64[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-from_datetime64 "Copy/link to this API element")

```python
from_datetime64(self: Self, meta: DatetimeMetaData, dt: int, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

utc\_to\_local[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-utc_to_local "Copy/link to this API element")

```python
utc_to_local(self: Self, out: datetimestruct, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

lossless\_unit[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-lossless_unit "Copy/link to this API element")

```python
lossless_unit(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

parse\_iso8601[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-parse_iso8601 "Copy/link to this API element")

```python
parse_iso8601(self: Self, s: str, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

to\_str[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-to_str "Copy/link to this API element")

```python
to_str(self: Self, unit: int, local: bool = False, utc: bool = False, tzoffset: int = -1, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/#class-datetimestruct-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/datetimestruct/
