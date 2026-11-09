# datetime

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/datetime.codon)

variable MINYEAR[](https://docs.exaloop.io/libraries/api/modules/datetime/#variable-minyear "Copy/link to this API element")

```python
MINYEAR = 1
```

variable MAXYEAR[](https://docs.exaloop.io/libraries/api/modules/datetime/#variable-maxyear "Copy/link to this API element")

```python
MAXYEAR = 9999
```

variable MAXORDINAL[](https://docs.exaloop.io/libraries/api/modules/datetime/#variable-maxordinal "Copy/link to this API element")

```python
MAXORDINAL = 3652059
```

variable MAX\_DELTA\_DAYS[](https://docs.exaloop.io/libraries/api/modules/datetime/#variable-max_delta_days "Copy/link to this API element")

```python
MAX_DELTA_DAYS = 999999999
```

class timedelta[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta "Copy/link to this API element")

@tuple

```python
class timedelta
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/)

Fields 3

`min` `ClassVar[timedelta]`

`max` `ClassVar[timedelta]`

`resolution` `ClassVar[timedelta]`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__new__ "Copy/link to this API element")

```python
__new__(days: float = 0, seconds: float = 0, microseconds: float = 0, milliseconds: float = 0, minutes: float = 0, hours: float = 0, weeks: float = 0)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__new__-overload-2 "Copy/link to this API element")

```python
__new__(days: int)
```

Properties 3

days[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-days "Copy/link to this API element")

@property

```python
days(self: Self, Self: type)
```

seconds[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-seconds "Copy/link to this API element")

@property

```python
seconds(self: Self, Self: type)
```

microseconds[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-microseconds "Copy/link to this API element")

@property

```python
microseconds(self: Self, Self: type)
```

Methods 1

total\_seconds[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-total_seconds "Copy/link to this API element")

```python
total_seconds(self: Self, Self: type)
```

Special methods 20

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: int, Self: type)
```

```python
__mul__(self: Self, other: float, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: int, Self: type)
```

```python
__rmul__(self: Self, other: float, Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: timedelta, Self: type)
```

```python
__truediv__(self: Self, other: float, Self: type)
```

```python
__truediv__(self: Self, other: int, Self: type)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: timedelta, Self: type)
```

```python
__floordiv__(self: Self, other: int, Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: timedelta, Self: type)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: timedelta, Self: type)
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: timedelta, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: timedelta, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: timedelta, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: timedelta, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: timedelta, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: timedelta, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

class IsoCalendarDate[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-isocalendardate "Copy/link to this API element")

@tuple

```python
class IsoCalendarDate
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/IsoCalendarDate/)

Fields 3

`year` `int`

`week` `int`

`weekday` `int`

Special methods 1

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-isocalendardate-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

class date[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date "Copy/link to this API element")

@tuple

```python
class date
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/date/)

Fields 3

`min` `ClassVar[date]`

`max` `ClassVar[date]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__new__ "Copy/link to this API element")

```python
__new__(year: int, month: int, day: int)
```

Properties 3

year[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-year "Copy/link to this API element")

@property

```python
year(self: Self, Self: type)
```

month[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-month "Copy/link to this API element")

@property

```python
month(self: Self, Self: type)
```

day[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-day "Copy/link to this API element")

@property

```python
day(self: Self, Self: type)
```

Methods 13

today[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-today "Copy/link to this API element")

```python
today()
```

fromtimestamp[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-fromtimestamp "Copy/link to this API element")

```python
fromtimestamp(timestamp)
```

fromordinal[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-fromordinal "Copy/link to this API element")

```python
fromordinal(ordinal: int)
```

fromisoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-fromisoformat "Copy/link to this API element")

```python
fromisoformat(date_string: str)
```

fromisocalendar[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-fromisocalendar "Copy/link to this API element")

```python
fromisocalendar(year, week, day)
```

replace[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-replace "Copy/link to this API element")

```python
replace(self: Self, year: int = -1, month: int = -1, day: int = -1, Self: type)
```

timetuple[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-timetuple "Copy/link to this API element")

```python
timetuple(self: Self, Self: type)
```

toordinal[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-toordinal "Copy/link to this API element")

```python
toordinal(self: Self, Self: type)
```

weekday[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-weekday "Copy/link to this API element")

```python
weekday(self: Self, Self: type)
```

isoweekday[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-isoweekday "Copy/link to this API element")

```python
isoweekday(self: Self, Self: type)
```

isocalendar[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-isocalendar "Copy/link to this API element")

```python
isocalendar(self: Self, Self: type)
```

isoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-isoformat "Copy/link to this API element")

```python
isoformat(self: Self, Self: type)
```

ctime[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-ctime "Copy/link to this API element")

```python
ctime(self: Self, Self: type)
```

Special methods 11

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, Self: type)
```

```python
__sub__(self: Self, other: date, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: date, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: date, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: date, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: date, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: date, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: date, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-date-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class time[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time "Copy/link to this API element")

@tuple

```python
class time
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/time/)

Fields 3

`min` `ClassVar[time]`

`max` `ClassVar[time]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-__new__ "Copy/link to this API element")

```python
__new__(hour: int = 0, minute: int = 0, second: int = 0, microsecond: int = 0)
```

Properties 4

hour[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-hour "Copy/link to this API element")

@property

```python
hour(self: Self, Self: type)
```

minute[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-minute "Copy/link to this API element")

@property

```python
minute(self: Self, Self: type)
```

second[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-second "Copy/link to this API element")

@property

```python
second(self: Self, Self: type)
```

microsecond[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-microsecond "Copy/link to this API element")

@property

```python
microsecond(self: Self, Self: type)
```

Methods 3

fromisoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-fromisoformat "Copy/link to this API element")

```python
fromisoformat(time_string: str)
```

replace[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-replace "Copy/link to this API element")

```python
replace(self: Self, hour: int = -1, minute: int = -1, second: int = -1, microsecond: int = -1, Self: type)
```

isoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-isoformat "Copy/link to this API element")

```python
isoformat(self: Self, timespec: Literal[str] = "auto", Self: type)
```

Special methods 3

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-time-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

class datetime[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime "Copy/link to this API element")

@tuple

```python
class datetime
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/datetime/)

Fields 3

`min` `ClassVar[datetime]`

`max` `ClassVar[datetime]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__new__ "Copy/link to this API element")

```python
__new__(year: int, month: int, day: int, hour: int = 0, minute: int = 0, second: int = 0, microsecond: int = 0)
```

Properties 7

year[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-year "Copy/link to this API element")

@property

```python
year(self: Self, Self: type)
```

month[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-month "Copy/link to this API element")

@property

```python
month(self: Self, Self: type)
```

day[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-day "Copy/link to this API element")

@property

```python
day(self: Self, Self: type)
```

hour[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-hour "Copy/link to this API element")

@property

```python
hour(self: Self, Self: type)
```

minute[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-minute "Copy/link to this API element")

@property

```python
minute(self: Self, Self: type)
```

second[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-second "Copy/link to this API element")

@property

```python
second(self: Self, Self: type)
```

microsecond[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-microsecond "Copy/link to this API element")

@property

```python
microsecond(self: Self, Self: type)
```

Methods 21

date[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-date "Copy/link to this API element")

```python
date(self: Self, Self: type)
```

time[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-time "Copy/link to this API element")

```python
time(self: Self, Self: type)
```

today[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-today "Copy/link to this API element")

```python
today()
```

now[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-now "Copy/link to this API element")

```python
now()
```

utcnow[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-utcnow "Copy/link to this API element")

```python
utcnow()
```

fromtimestamp[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-fromtimestamp "Copy/link to this API element")

```python
fromtimestamp(timestamp)
```

utcfromtimestamp[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-utcfromtimestamp "Copy/link to this API element")

```python
utcfromtimestamp(timestamp)
```

fromordinal[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-fromordinal "Copy/link to this API element")

```python
fromordinal(ordinal: int)
```

combine[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-combine "Copy/link to this API element")

```python
combine(date: date, time: time)
```

fromisoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-fromisoformat "Copy/link to this API element")

```python
fromisoformat(date_string: str)
```

fromisocalendar[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-fromisocalendar "Copy/link to this API element")

```python
fromisocalendar(year: int, week: int, day: int)
```

replace[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-replace "Copy/link to this API element")

```python
replace(self: Self, year: int = -1, month: int = -1, day: int = -1, hour: int = -1, minute: int = -1, second: int = -1, microsecond: int = -1, Self: type)
```

timetuple[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-timetuple "Copy/link to this API element")

```python
timetuple(self: Self, Self: type)
```

utctimetuple[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-utctimetuple "Copy/link to this API element")

```python
utctimetuple(self: Self, Self: type)
```

toordinal[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-toordinal "Copy/link to this API element")

```python
toordinal(self: Self, Self: type)
```

timestamp[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-timestamp "Copy/link to this API element")

```python
timestamp(self: Self, Self: type)
```

weekday[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-weekday "Copy/link to this API element")

```python
weekday(self: Self, Self: type)
```

isoweekday[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-isoweekday "Copy/link to this API element")

```python
isoweekday(self: Self, Self: type)
```

isocalendar[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-isocalendar "Copy/link to this API element")

```python
isocalendar(self: Self, Self: type)
```

isoformat[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-isoformat "Copy/link to this API element")

```python
isoformat(self: Self, sep: str = "T", timespec: Literal[str] = "auto", Self: type)
```

ctime[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-ctime "Copy/link to this API element")

```python
ctime(self: Self, Self: type)
```

Special methods 11

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, Self: type)
```

```python
__sub__(self: Self, other: datetime, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: datetime, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: datetime, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: datetime, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: datetime, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: datetime, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: datetime, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-datetime-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

class timedelta[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta "Copy/link to this API element")

@extend

```python
class timedelta
```

Extends [`datetime.timedelta`](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/datetime/timedelta/)

Special methods 1

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/datetime/#class-timedelta-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: date, Self: type)
```

```python
__add__(self: Self, other: datetime, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/datetime/
