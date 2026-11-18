# Optional

Standard library class `internal.core.Optional` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L130)

class Optional[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional "Copy/link to this API element")

@tuple

```python
class Optional[T]
```

Fields 1

`T` `type`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__new__ "Copy/link to this API element")

```python
__new__() -> Optional[T]
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: T) -> Optional[T]
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

Special methods 15

\_\_has\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__has__ "Copy/link to this API element")

```python
__has__(self: Self) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_val\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__val__ "Copy/link to this API element")

```python
__val__(self: Self) -> T
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_val\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__val_or__ "Copy/link to this API element")

```python
__val_or__(self: Self, default: T)
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: T) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

@overload

```python
__eq__(self: Self, other: Optional[T]) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: T) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

@overload

```python
__ne__(self: Self, other: Optional[T]) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__str__ "Copy/link to this API element")

```python
__str__(self: Self) -> str
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_is\_optional\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__is_optional__ "Copy/link to this API element")

```python
__is_optional__(self: Self, other: Optional[T]) -> bool
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_unwrap\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__unwrap__ "Copy/link to this API element")

```python
__unwrap__(self: Self) -> T
```

Added by [`internal.types.optional`](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Optional[T])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Optional[T])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/#class-optional-__from_py__ "Copy/link to this API element")

```python
__from_py__(o: cobj) -> Optional[T]
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/
