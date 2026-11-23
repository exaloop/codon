# Future

Standard library class `asyncio.Future` [View source](https://github.com/exaloop/codon/blob/master/stdlib/asyncio.codon#L116)

class Future[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future "Copy/link to this API element")

```python
class Future[R](object)
```

Fields 1

`R` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *, loop: Optional[EventLoop] = None)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Methods 9

add\_done\_callback[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-add_done_callback "Copy/link to this API element")

```python
add_done_callback(self: Self, callback, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

result[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-result "Copy/link to this API element")

```python
result(self: Self, /) -> R
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

set\_result[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-set_result "Copy/link to this API element")

```python
set_result(self: Self, result: R, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

set\_exception[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-set_exception "Copy/link to this API element")

```python
set_exception(self: Self, exception: BaseException, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

cancelled[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

done[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-done "Copy/link to this API element")

```python
done(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

cancel[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-cancel "Copy/link to this API element")

```python
cancel(self: Self, /, msg: Optional[str] = None)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

get\_loop[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-get_loop "Copy/link to this API element")

```python
get_loop(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

exception[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-exception "Copy/link to this API element")

```python
exception(self: Self, /) -> Optional[BaseException]
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Special methods 1

\_\_await\_\_[](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/#class-future-__await__ "Copy/link to this API element")

@async

```python
__await__(self: Self, /)
```

Added by [`asyncio`](https://docs.exaloop.io/libraries/api/modules/asyncio/)

Source: https://docs.exaloop.io/libraries/api/classes/asyncio/Future/
