# time

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/time.codon)

function time[](https://docs.exaloop.io/libraries/api/modules/time/#function-time "Copy/link to this API element")

```python
time()
```

function time\_ns[](https://docs.exaloop.io/libraries/api/modules/time/#function-time_ns "Copy/link to this API element")

```python
time_ns()
```

function monotonic[](https://docs.exaloop.io/libraries/api/modules/time/#function-monotonic "Copy/link to this API element")

```python
monotonic()
```

function monotonic\_ns[](https://docs.exaloop.io/libraries/api/modules/time/#function-monotonic_ns "Copy/link to this API element")

```python
monotonic_ns()
```

function perf\_counter[](https://docs.exaloop.io/libraries/api/modules/time/#function-perf_counter "Copy/link to this API element")

```python
perf_counter()
```

function perf\_counter\_ns[](https://docs.exaloop.io/libraries/api/modules/time/#function-perf_counter_ns "Copy/link to this API element")

```python
perf_counter_ns()
```

function sleep[](https://docs.exaloop.io/libraries/api/modules/time/#function-sleep "Copy/link to this API element")

```python
sleep(secs: float)
```

class TimeInterval[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval "Copy/link to this API element")

```python
class TimeInterval
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/time/TimeInterval/)

Utility class for timing Codon code

Fields 2

`start` `int`

`msg` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Methods 3

report[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-report "Copy/link to this API element")

```python
report(self: Self, msg = "", memory = False, Self: type)
```

elapsed[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-elapsed "Copy/link to this API element")

```python
elapsed(self: Self, Self: type)
```

tick[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-tick "Copy/link to this API element")

```python
tick(self: Self, msg, memory = False, Self: type)
```

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/time/#class-timeinterval-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

function timing[](https://docs.exaloop.io/libraries/api/modules/time/#function-timing "Copy/link to this API element")

```python
timing(msg: str = "")
```

Example usage:

```python
from time import timing
with timing('foo function'):
    foo()  # prints runtime of foo
```

class struct\_time[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time "Copy/link to this API element")

@tuple

```python
class struct_time
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/time/struct_time/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-__new__ "Copy/link to this API element")

```python
__new__(year: int = 0, mon: int = 0, mday: int = 0, hour: int = 0, min: int = 0, sec: int = 0, wday: int = 0, yday: int = 0, isdst: int = 0)
```

Properties 9

tm\_year[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_year "Copy/link to this API element")

@property

```python
tm_year(self: Self, Self: type)
```

tm\_yday[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_yday "Copy/link to this API element")

@property

```python
tm_yday(self: Self, Self: type)
```

tm\_sec[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_sec "Copy/link to this API element")

@property

```python
tm_sec(self: Self, Self: type)
```

tm\_min[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_min "Copy/link to this API element")

@property

```python
tm_min(self: Self, Self: type)
```

tm\_hour[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_hour "Copy/link to this API element")

@property

```python
tm_hour(self: Self, Self: type)
```

tm\_mday[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_mday "Copy/link to this API element")

@property

```python
tm_mday(self: Self, Self: type)
```

tm\_mon[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_mon "Copy/link to this API element")

@property

```python
tm_mon(self: Self, Self: type)
```

tm\_wday[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_wday "Copy/link to this API element")

@property

```python
tm_wday(self: Self, Self: type)
```

tm\_isdst[](https://docs.exaloop.io/libraries/api/modules/time/#class-struct_time-tm_isdst "Copy/link to this API element")

@property

```python
tm_isdst(self: Self, Self: type)
```

function localtime[](https://docs.exaloop.io/libraries/api/modules/time/#function-localtime "Copy/link to this API element")

```python
localtime(secs: int = -1)
```

function gmtime[](https://docs.exaloop.io/libraries/api/modules/time/#function-gmtime "Copy/link to this API element")

```python
gmtime(secs: int = -1)
```

function mktime[](https://docs.exaloop.io/libraries/api/modules/time/#function-mktime "Copy/link to this API element")

```python
mktime(t)
```

Source: https://docs.exaloop.io/libraries/api/modules/time/
