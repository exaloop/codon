# asyncio

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/asyncio.codon)

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
__init__(self: Self, work: WorkItem, Self: type)
```

Methods 1

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-worknode-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, Self: type)
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
__init__(self: Self, work: WorkItem, when: float, Self: type)
```

Methods 1

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timer-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, Self: type)
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
cancel(self: Self, Self: type)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-handle-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, Self: type)
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
cancel(self: Self, Self: type)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, Self: type)
```

when[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-timerhandle-when "Copy/link to this API element")

```python
when(self: Self, Self: type)
```

class Future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future "Copy/link to this API element")

```python
class Future[R]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/Future/)

Fields 1

`R` `type`

class Task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task "Copy/link to this API element")

```python
class Task[R]
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
cancelled(self: Self, Self: type)
```

class InvalidStateError[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-invalidstateerror "Copy/link to this API element")

```python
class InvalidStateError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/InvalidStateError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-invalidstateerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class CancelledError[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-cancellederror "Copy/link to this API element")

```python
class CancelledError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/asyncio/CancelledError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-cancellederror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
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
__init__(self: Self, Self: type)
```

Methods 13

close[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

time[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-time "Copy/link to this API element")

```python
time(self: Self, Self: type)
```

stop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-stop "Copy/link to this API element")

```python
stop(self: Self, Self: type)
```

is\_running[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-is_running "Copy/link to this API element")

```python
is_running(self: Self, Self: type)
```

is\_closed[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-is_closed "Copy/link to this API element")

```python
is_closed(self: Self, Self: type)
```

call\_soon[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_soon "Copy/link to this API element")

```python
call_soon(self: Self, callback, *args, Self: type)
```

call\_soon\_threadsafe[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_soon_threadsafe "Copy/link to this API element")

```python
call_soon_threadsafe(self: Self, callback, *args, Self: type)
```

call\_later[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_later "Copy/link to this API element")

```python
call_later(self: Self, delay: float, callback, *args, Self: type)
```

call\_at[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-call_at "Copy/link to this API element")

```python
call_at(self: Self, when: float, callback, *args, Self: type)
```

run\_forever[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-run_forever "Copy/link to this API element")

```python
run_forever(self: Self, Self: type)
```

run\_until\_complete[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-run_until_complete "Copy/link to this API element")

```python
run_until_complete(self: Self, future, Self: type)
```

create\_future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-create_future "Copy/link to this API element")

```python
create_future(self: Self, T: type = NoneType, Self: type)
```

create\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-eventloop-create_task "Copy/link to this API element")

```python
create_task(self: Self, coro: Coroutine, name: Optional[str] = None, Self: type)
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
__init__(self: Self, loop: Optional[EventLoop] = None, Self: type)
```

Methods 9

add\_done\_callback[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-add_done_callback "Copy/link to this API element")

```python
add_done_callback(self: Self, callback, Self: type)
```

result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-result "Copy/link to this API element")

```python
result(self: Self, Self: type)
```

set\_result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-set_result "Copy/link to this API element")

```python
set_result(self: Self, result: R, Self: type)
```

set\_exception[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-set_exception "Copy/link to this API element")

```python
set_exception(self: Self, exception: BaseException, Self: type)
```

cancelled[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-cancelled "Copy/link to this API element")

```python
cancelled(self: Self, Self: type)
```

done[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-done "Copy/link to this API element")

```python
done(self: Self, Self: type)
```

cancel[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-cancel "Copy/link to this API element")

```python
cancel(self: Self, msg: Optional[str] = None, Self: type)
```

get\_loop[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-get_loop "Copy/link to this API element")

```python
get_loop(self: Self, Self: type)
```

exception[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-exception "Copy/link to this API element")

```python
exception(self: Self, Self: type)
```

Special methods 1

\_\_await\_\_[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-future-__await__ "Copy/link to this API element")

```python
__await__(self: Self, Self: type)
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
__init__(self: Self, coro: Coroutine[R], loop: Optional[EventLoop] = None, name: Optional[str] = None, Self: type)
```

Methods 4

get\_name[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-get_name "Copy/link to this API element")

```python
get_name(self: Self, Self: type)
```

set\_name[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-set_name "Copy/link to this API element")

```python
set_name(self: Self, value: str, Self: type)
```

get\_coro[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-get_coro "Copy/link to this API element")

```python
get_coro(self: Self, T: type = NoneType, Self: type)
```

result[](https://docs.exaloop.io/libraries/api/modules/asyncio/#class-task-result "Copy/link to this API element")

```python
result(self: Self, Self: type)
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
get_running_loop()
```

function create\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-create_task "Copy/link to this API element")

```python
create_task(coro, name: Optional[str] = None)
```

function isfuture[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-isfuture "Copy/link to this API element")

```python
isfuture(obj)
```

function iscoroutine[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-iscoroutine "Copy/link to this API element")

```python
iscoroutine(obj)
```

function ensure\_future[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-ensure_future "Copy/link to this API element")

```python
ensure_future(obj, loop: Optional[EventLoop] = None)
```

function current\_task[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-current_task "Copy/link to this API element")

```python
current_task()
```

function run[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-run "Copy/link to this API element")

```python
run(coro, debug = None, loop_factory = None)
```

function sleep[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-sleep "Copy/link to this API element")

```python
sleep(delay: float, result = None)
```

function gather[](https://docs.exaloop.io/libraries/api/modules/asyncio/#function-gather "Copy/link to this API element")

```python
gather(*aws)
```

Source: https://docs.exaloop.io/libraries/api/modules/asyncio/
