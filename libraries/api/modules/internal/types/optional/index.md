# internal.types.optional

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/optional.codon)

class Optional[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional "Copy/link to this API element")

@extend

```python
class Optional[T]
```

Extends [`internal.core.Optional`](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__new__ "Copy/link to this API element")

```python
__new__() -> Optional[T]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: T) -> Optional[T]
```

Special methods 10

\_\_has\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__has__ "Copy/link to this API element")

```python
__has__(self: Self) -> bool
```

\_\_val\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__val__ "Copy/link to this API element")

```python
__val__(self: Self) -> T
```

\_\_val\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__val_or__ "Copy/link to this API element")

```python
__val_or__(self: Self, default: T)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: T) -> bool
```

@overload

```python
__eq__(self: Self, other: Optional[T]) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: T) -> bool
```

@overload

```python
__ne__(self: Self, other: Optional[T]) -> bool
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__str__ "Copy/link to this API element")

```python
__str__(self: Self) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_is\_optional\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__is_optional__ "Copy/link to this API element")

```python
__is_optional__(self: Self, other: Optional[T]) -> bool
```

\_\_unwrap\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__unwrap__ "Copy/link to this API element")

```python
__unwrap__(self: Self) -> T
```

alias optional[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#alias-optional "Copy/link to this API element")

```python
optional = Optional
```

Alias of [`Optional`](https://docs.exaloop.io/libraries/api/classes/internal/core/Optional/)

function unwrap[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#function-unwrap "Copy/link to this API element")

```python
unwrap[T: type = NoneType](opt)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/optional/
