# EventLoop

Standard library class `asyncio.EventLoop` [View source](https://github.com/exaloop/codon/blob/master/stdlib/asyncio.codon)

class EventLoop[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop "Copy/link to this API element")

```python
class EventLoop
```

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Methods 13

close[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

time[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-time "Copy/link to this API element")

```python
time(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

stop[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-stop "Copy/link to this API element")

```python
stop(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

is\_running[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-is_running "Copy/link to this API element")

```python
is_running(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

is\_closed[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-is_closed "Copy/link to this API element")

```python
is_closed(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

call\_soon[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-call_soon "Copy/link to this API element")

```python
call_soon(self: Self, callback, *args, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

call\_soon\_threadsafe[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-call_soon_threadsafe "Copy/link to this API element")

```python
call_soon_threadsafe(self: Self, callback, *args, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

call\_later[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-call_later "Copy/link to this API element")

```python
call_later(self: Self, delay: float, callback, *args, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

call\_at[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-call_at "Copy/link to this API element")

```python
call_at(self: Self, when: float, callback, *args, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

run\_forever[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-run_forever "Copy/link to this API element")

```python
run_forever(self: Self, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

run\_until\_complete[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-run_until_complete "Copy/link to this API element")

```python
run_until_complete(self: Self, future, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

create\_future[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-create_future "Copy/link to this API element")

```python
create_future(self: Self, T: type = NoneType, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

create\_task[](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/#class-eventloop-create_task "Copy/link to this API element")

```python
create_task(self: Self, coro: Coroutine, name: Optional[str] = None, Self: type)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Source: https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/
