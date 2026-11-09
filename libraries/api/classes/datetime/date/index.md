# date

Standard library class `datetime.date` [View source](https://github.com/exaloop/codon/blob/master/stdlib/datetime.codon)

class date[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date "Copy/link to this API element")

@tuple

```python
class date
```

Fields 3

`min` `ClassVar[date]`

`max` `ClassVar[date]`

`resolution` `ClassVar[timedelta]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__new__ "Copy/link to this API element")

```python
__new__(year: int, month: int, day: int)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Properties 3

year[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-year "Copy/link to this API element")

@property

```python
year(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

month[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-month "Copy/link to this API element")

@property

```python
month(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

day[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-day "Copy/link to this API element")

@property

```python
day(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Methods 13

today[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-today "Copy/link to this API element")

```python
today()
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromtimestamp[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-fromtimestamp "Copy/link to this API element")

```python
fromtimestamp(timestamp)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromordinal[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-fromordinal "Copy/link to this API element")

```python
fromordinal(ordinal: int)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromisoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-fromisoformat "Copy/link to this API element")

```python
fromisoformat(date_string: str)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

fromisocalendar[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-fromisocalendar "Copy/link to this API element")

```python
fromisocalendar(year, week, day)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

replace[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-replace "Copy/link to this API element")

```python
replace(self: Self, year: int = -1, month: int = -1, day: int = -1, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

timetuple[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-timetuple "Copy/link to this API element")

```python
timetuple(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

toordinal[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-toordinal "Copy/link to this API element")

```python
toordinal(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

weekday[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-weekday "Copy/link to this API element")

```python
weekday(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isoweekday[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-isoweekday "Copy/link to this API element")

```python
isoweekday(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isocalendar[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-isocalendar "Copy/link to this API element")

```python
isocalendar(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

isoformat[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-isoformat "Copy/link to this API element")

```python
isoformat(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

ctime[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-ctime "Copy/link to this API element")

```python
ctime(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Special methods 11

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: timedelta, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

```python
__sub__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: date, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/datetime/date/#class-date-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`datetime`](https://docs.exaloop.io/libraries/api/modules/datetime/)

Source: https://docs.exaloop.io/libraries/api/classes/datetime/date/
