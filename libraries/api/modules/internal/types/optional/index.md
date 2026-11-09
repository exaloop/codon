# internal.types.optional

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/optional.codon)

class Optional[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional "Copy/link to this API element")

@extend

```python
class Optional[T]
```

Extends [`Optional`](https://docs.exaloop.io/libraries/api/classes/Optional/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Optional/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: T)
```

Special methods 10

\_\_has\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__has__ "Copy/link to this API element")

```python
__has__(self: Self, Self: type)
```

\_\_val\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__val__ "Copy/link to this API element")

```python
__val__(self: Self, Self: type)
```

\_\_val\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__val_or__ "Copy/link to this API element")

```python
__val_or__(self: Self, default: T, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: T, Self: type)
```

@overload

```python
__eq__(self: Self, other: Optional[T], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: T, Self: type)
```

@overload

```python
__ne__(self: Self, other: Optional[T], Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_is\_optional\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__is_optional__ "Copy/link to this API element")

```python
__is_optional__(self: Self, other: Optional[T], Self: type)
```

\_\_unwrap\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#class-optional-__unwrap__ "Copy/link to this API element")

```python
__unwrap__(self: Self, Self: type)
```

function unwrap[](https://docs.exaloop.io/libraries/api/modules/internal/types/optional/#function-unwrap "Copy/link to this API element")

```python
unwrap(opt, T: type = NoneType)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/optional/
