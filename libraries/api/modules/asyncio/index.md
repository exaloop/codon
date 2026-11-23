# asyncio

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/asyncio.codon)

class WorkItem[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem "Copy/link to this API element")

@tuple

```python
class WorkItem
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/WorkItem/)

Fields 2

`coro` `cobj`

`task` `Optional[_FutureState]`

class WorkNode[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-worknode "Copy/link to this API element")

```python
class WorkNode
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/WorkNode/)

Fields 4

`work` `WorkItem`

`prev` `Optional[WorkNode]`

`next` `Optional[WorkNode]`

`queued` `bool`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-worknode-__init__ "Copy/link to this API element")

```python
__init__(self: Self, work: WorkItem)
```

Methods 1

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-worknode-cancelled "Copy/link to this API element")

```python
cancelled(self: Self)
```

class Timer[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timer "Copy/link to this API element")

```python
class Timer
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Timer/)

Fields 2

`work` `WorkItem`

`when` `float`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timer-__init__ "Copy/link to this API element")

```python
__init__(self: Self, work: WorkItem, when: float)
```

Methods 1

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timer-cancelled "Copy/link to this API element")

```python
cancelled(self: Self)
```

class EventLoop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop "Copy/link to this API element")

```python
class EventLoop
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/)

class Handle[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-handle "Copy/link to this API element")

@tuple

```python
class Handle
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Handle/)

Methods 2

cancel[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-handle-cancel "Copy/link to this API element")

```python
cancel(self: Self)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-handle-cancelled "Copy/link to this API element")

```python
cancelled(self: Self)
```

class TimerHandle[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle "Copy/link to this API element")

@tuple

```python
class TimerHandle
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/TimerHandle/)

Methods 3

cancel[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle-cancel "Copy/link to this API element")

```python
cancel(self: Self)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle-cancelled "Copy/link to this API element")

```python
cancelled(self: Self)
```

when[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle-when "Copy/link to this API element")

```python
when(self: Self)
```

class Future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future "Copy/link to this API element")

```python
class Future[R](object)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/)

Fields 1

`R` `type`

class Task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task "Copy/link to this API element")

```python
class Task[R](Future[R])
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/)

Fields 1

`R` `type`

class WorkItem[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem "Copy/link to this API element")

@extend

```python
class WorkItem
```

Extends [`asyncio.WorkItem`](https://docs.exaloop.io/libraries/api/classes/asyncio/WorkItem/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/WorkItem/)

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-__new__ "Copy/link to this API element")

```python
__new__(coro: cobj)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-__new__-overload-2 "Copy/link to this API element")

```python
__new__(coro: Coroutine)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-__new__-overload-3 "Copy/link to this API element")

```python
__new__(task: Task)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-__new__-overload-4 "Copy/link to this API element")

```python
__new__(state: _FutureState)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-__new__-overload-5 "Copy/link to this API element")

```python
__new__()
```

Methods 1

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-workitem-cancelled "Copy/link to this API element")

```python
cancelled(self: Self)
```

class InvalidStateError[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-invalidstateerror "Copy/link to this API element")

```python
class InvalidStateError(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/InvalidStateError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-invalidstateerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class CancelledError[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-cancellederror "Copy/link to this API element")

```python
class CancelledError(BaseException)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/CancelledError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-cancellederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", /)
```

class EventLoop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop "Copy/link to this API element")

@extend

```python
class EventLoop
```

Extends [`asyncio.EventLoop`](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/EventLoop/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-__init__ "Copy/link to this API element")

```python
__init__(self: Self)
```

Methods 13

close[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-close "Copy/link to this API element")

```python
close(self: Self)
```

time[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-time "Copy/link to this API element")

```python
time(self: Self)
```

stop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-stop "Copy/link to this API element")

```python
stop(self: Self)
```

is\_running[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-is_running "Copy/link to this API element")

```python
is_running(self: Self)
```

is\_closed[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-is_closed "Copy/link to this API element")

```python
is_closed(self: Self)
```

call\_soon[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_soon "Copy/link to this API element")

```python
call_soon(self: Self, callback, *args)
```

call\_soon\_threadsafe[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_soon_threadsafe "Copy/link to this API element")

```python
call_soon_threadsafe(self: Self, callback, *args)
```

call\_later[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_later "Copy/link to this API element")

```python
call_later(self: Self, delay: float, callback, *args)
```

call\_at[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_at "Copy/link to this API element")

```python
call_at(self: Self, when: float, callback, *args)
```

run\_forever[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-run_forever "Copy/link to this API element")

```python
run_forever(self: Self)
```

run\_until\_complete[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-run_until_complete "Copy/link to this API element")

```python
run_until_complete(self: Self, future)
```

create\_future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-create_future "Copy/link to this API element")

```python
create_future[T: type = NoneType](self: Self)
```

create\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-create_task "Copy/link to this API element")

```python
create_task(self: Self, coro: Coroutine, *, name: Optional[str] = None)
```

class Future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future "Copy/link to this API element")

@extend

```python
class Future[R]
```

Extends [`asyncio.Future`](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, *, loop: Optional[EventLoop] = None)
```

Methods 9

add\_done\_callback[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-add_done_callback "Copy/link to this API element")

```python
add_done_callback(self: Self, callback, /)
```

result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-result "Copy/link to this API element")

```python
result(self: Self, /) -> R
```

set\_result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-set_result "Copy/link to this API element")

```python
set_result(self: Self, result: R, /)
```

set\_exception[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-set_exception "Copy/link to this API element")

```python
set_exception(self: Self, exception: BaseException, /)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, /)
```

done[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-done "Copy/link to this API element")

```python
done(self: Self, /)
```

cancel[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-cancel "Copy/link to this API element")

```python
cancel(self: Self, /, msg: Optional[str] = None)
```

get\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-get_loop "Copy/link to this API element")

```python
get_loop(self: Self, /)
```

exception[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-exception "Copy/link to this API element")

```python
exception(self: Self, /) -> Optional[BaseException]
```

Special methods 1

\_\_await\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-__await__ "Copy/link to this API element")

@async

```python
__await__(self: Self, /)
```

class Task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task "Copy/link to this API element")

@extend

```python
class Task[R]
```

Extends [`asyncio.Task`](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Task/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, coro: Coroutine[R], *, loop: Optional[EventLoop] = None, name: Optional[str] = None)
```

Methods 4

get\_name[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-get_name "Copy/link to this API element")

```python
get_name(self: Self, /)
```

set\_name[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-set_name "Copy/link to this API element")

```python
set_name(self: Self, value: str, /)
```

get\_coro[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-get_coro "Copy/link to this API element")

```python
get_coro[T: type = NoneType](self: Self, /) -> Coroutine[T]
```

result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-result "Copy/link to this API element")

```python
result(self: Self, /)
```

function new\_event\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-new_event_loop "Copy/link to this API element")

```python
new_event_loop()
```

function set\_event\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-set_event_loop "Copy/link to this API element")

```python
set_event_loop(loop: EventLoop)
```

function get\_event\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-get_event_loop "Copy/link to this API element")

```python
get_event_loop()
```

function get\_running\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-get_running_loop "Copy/link to this API element")

```python
get_running_loop() -> EventLoop
```

function create\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-create_task "Copy/link to this API element")

```python
create_task(coro, *, name: Optional[str] = None)
```

function isfuture[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-isfuture "Copy/link to this API element")

```python
isfuture(obj) -> Literal[bool]
```

function iscoroutine[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-iscoroutine "Copy/link to this API element")

```python
iscoroutine(obj) -> Literal[bool]
```

function ensure\_future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-ensure_future "Copy/link to this API element")

```python
ensure_future(obj, *, loop: Optional[EventLoop] = None)
```

function current\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-current_task "Copy/link to this API element")

```python
current_task() -> Optional[Task[None]]
```

function run[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-run "Copy/link to this API element")

```python
run(coro, *, debug = None, loop_factory = None)
```

function sleep[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-sleep "Copy/link to this API element")

@async

```python
sleep(delay: float, result = None)
```

function gather[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-gather "Copy/link to this API element")

```python
gather(*aws)
```

Source: https://docs.exaloop.io/libraries/api/modules/asyncio/
