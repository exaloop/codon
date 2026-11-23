# Callable

Standard library class `internal.core.Callable` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L407)

class Callable[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable "Copy/link to this API element")

@tuple

```python
class Callable[T, TR]
```

Fields 4

`fn` `Function[([Ptr[u8], T], TR)]`

`data` `Ptr[u8]`

`T` `type`

`TR` `type`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable-__new__ "Copy/link to this API element")

```python
__new__(fn: Function[([Ptr[u8], T], TR)], data: Ptr[u8]) -> Callable[(T, TR)]
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__[T: type, TR: type, M: Literal[str], PT: type, F: type, K: type](fn: Function[([Ptr[u8], T], TR)], data: Partial[(M, PT, K, F)]) -> Callable[(T, TR)]
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(fn: Function[([Ptr[u8], T], TR)], data: Function[(T, TR)]) -> Callable[(T, TR)]
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(fn: Function[(T, TR)]) -> Callable[(T, TR)]
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/#class-callable-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/
