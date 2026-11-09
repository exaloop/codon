# threading

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/threading.codon)

class ThreadLocal[](https://docs.exaloop.io/libraries/api/modules/threading/#class-threadlocal "Copy/link to this API element")

@tuple

```python
class ThreadLocal[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/threading/ThreadLocal/)

Fields 1

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-threadlocal-__new__ "Copy/link to this API element")

```python
__new__(T: type)
```

class Lock[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock "Copy/link to this API element")

@tuple

```python
class Lock
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/threading/Lock/)

Fields 1

`p` `cobj`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock-__new__ "Copy/link to this API element")

```python
__new__()
```

Methods 2

acquire[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock-acquire "Copy/link to this API element")

```python
acquire(self: Self, block: bool = True, timeout: float = -1.0, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-lock-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

class RLock[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock "Copy/link to this API element")

@tuple

```python
class RLock
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/threading/RLock/)

Fields 1

`p` `cobj`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock-__new__ "Copy/link to this API element")

```python
__new__()
```

Methods 2

acquire[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock-acquire "Copy/link to this API element")

```python
acquire(self: Self, block: bool = True, timeout: float = -1.0, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/threading/#class-rlock-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

function active\_count[](https://docs.exaloop.io/libraries/api/modules/threading/#function-active_count "Copy/link to this API element")

```python
active_count()
```

function get\_native\_id[](https://docs.exaloop.io/libraries/api/modules/threading/#function-get_native_id "Copy/link to this API element")

```python
get_native_id()
```

function get\_ident[](https://docs.exaloop.io/libraries/api/modules/threading/#function-get_ident "Copy/link to this API element")

```python
get_ident()
```

Source: https://docs.exaloop.io/libraries/api/modules/threading/
