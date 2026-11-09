# DatetimeMetaData

Standard library class `numpy.npdatetime.DatetimeMetaData` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npdatetime.codon)

class DatetimeMetaData[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata "Copy/link to this API element")

@tuple

```python
class DatetimeMetaData
```

Fields 2

`base` `i32`

`num` `i32`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-__new__ "Copy/link to this API element")

```python
__new__(base: int, num: int = 1)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-__new__-overload-2 "Copy/link to this API element")

```python
__new__(base: Literal[str], num: int = 1)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-__new__-overload-3 "Copy/link to this API element")

```python
__new__(dt: datetime64)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-__new__-overload-4 "Copy/link to this API element")

```python
__new__(td: timedelta64)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Methods 2

divisor\_to\_multiple[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-divisor_to_multiple "Copy/link to this API element")

```python
divisor_to_multiple(self: Self, den: int, metastr: str, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

conversion\_factor[](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/#class-datetimemetadata-conversion_factor "Copy/link to this API element")

```python
conversion_factor(self: Self, dst: DatetimeMetaData, Self: type)
```

Added by [`numpy.npdatetime`](https://docs.exaloop.io/libraries/api/modules/numpy/npdatetime/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/DatetimeMetaData/
