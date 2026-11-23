# Task

Standard library class `asyncio.Task` [View source](https://github.com/exaloop/codon/blob/master/stdlib/asyncio.codon#L121)

class Task[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task "Copy/link to this API element")

```python
class Task[R](Future[R])
```

Fields 1

`R` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, coro: Coroutine[R], *, loop: Optional[EventLoop] = None, name: Optional[str] = None)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Methods 4

get\_name[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task-get_name "Copy/link to this API element")

```python
get_name(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

set\_name[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task-set_name "Copy/link to this API element")

```python
set_name(self: Self, value: str, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

get\_coro[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task-get_coro "Copy/link to this API element")

```python
get_coro[T: type = NoneType](self: Self, /) -> Coroutine[T]
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

result[](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/#class-task-result "Copy/link to this API element")

```python
result(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Source: https://docs.exaloop.io/libraries/api/classes/asyncio/Task/
