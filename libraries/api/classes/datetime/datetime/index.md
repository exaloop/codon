# datetime

Standard library class `datetime.datetime` [View source](https://github.com/exaloop/codon/blob/master/stdlib/datetime.codon#L880)

class datetime[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime "Copy/link to this API element")

@tuple

```python
class datetime
```

Fields 3

`min` `ClassVar[datetime]`

`max` `ClassVar[datetime]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__new__ "Copy/link to this API element")

```python
__new__(year: int, month: int, day: int, hour: int = 0, minute: int = 0, second: int = 0, microsecond: int = 0) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Properties 7

year[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-year "Copy/link to this API element")

@property

```python
year(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

month[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-month "Copy/link to this API element")

@property

```python
month(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

day[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-day "Copy/link to this API element")

@property

```python
day(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

hour[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-hour "Copy/link to this API element")

@property

```python
hour(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

minute[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-minute "Copy/link to this API element")

@property

```python
minute(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

second[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-second "Copy/link to this API element")

@property

```python
second(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

microsecond[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-microsecond "Copy/link to this API element")

@property

```python
microsecond(self: Self) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Methods 21

date[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-date "Copy/link to this API element")

```python
date(self: Self, /) -> date
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

time[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-time "Copy/link to this API element")

```python
time(self: Self, /) -> time
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

today[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-today "Copy/link to this API element")

```python
today() -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

now[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-now "Copy/link to this API element")

```python
now() -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

utcnow[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-utcnow "Copy/link to this API element")

```python
utcnow() -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromtimestamp[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-fromtimestamp "Copy/link to this API element")

```python
fromtimestamp(timestamp) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

utcfromtimestamp[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-utcfromtimestamp "Copy/link to this API element")

```python
utcfromtimestamp(timestamp, /) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromordinal[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-fromordinal "Copy/link to this API element")

```python
fromordinal(ordinal: int, /) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

combine[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-combine "Copy/link to this API element")

```python
combine(date: date, time: time) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromisoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-fromisoformat "Copy/link to this API element")

```python
fromisoformat(date_string: str, /) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromisocalendar[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-fromisocalendar "Copy/link to this API element")

```python
fromisocalendar(year: int, week: int, day: int) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

replace[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-replace "Copy/link to this API element")

```python
replace(self: Self, /, year: int = -1, month: int = -1, day: int = -1, hour: int = -1, minute: int = -1, second: int = -1, microsecond: int = -1) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

timetuple[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-timetuple "Copy/link to this API element")

```python
timetuple(self: Self, /) -> struct_time
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

utctimetuple[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-utctimetuple "Copy/link to this API element")

```python
utctimetuple(self: Self, /) -> struct_time
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

toordinal[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-toordinal "Copy/link to this API element")

```python
toordinal(self: Self, /) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

timestamp[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-timestamp "Copy/link to this API element")

```python
timestamp(self: Self, /) -> float
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

weekday[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-weekday "Copy/link to this API element")

```python
weekday(self: Self, /) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isoweekday[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-isoweekday "Copy/link to this API element")

```python
isoweekday(self: Self, /) -> int
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isocalendar[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-isocalendar "Copy/link to this API element")

```python
isocalendar(self: Self, /) -> IsoCalendarDate
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-isoformat "Copy/link to this API element")

```python
isoformat[timespec: Literal[str] = "auto"](self: Self, /, sep: str = "T") -> str
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

ctime[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-ctime "Copy/link to this API element")

```python
ctime(self: Self, /) -> str
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Special methods 11

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /) -> str
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, /) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, /) -> datetime
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__sub__(self: Self, other: datetime, /) -> timedelta
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: datetime, /) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/#class-datetime-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Source: https://docs.exaloop.io/libraries/api/classes/datetime/datetime/
