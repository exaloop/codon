# Partial

Standard library class `internal.core.Partial` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L389)

class Partial[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial "Copy/link to this API element")

@tuple

```python
class Partial[M, T, K, F]
```

Fields 6

`args` `T`

`kwargs` `K`

`M` `Literal[str]`

`T` `type`

`K` `type`

`F` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial-__new__ "Copy/link to this API element")

@pure @derives @\_\_internal\_\_

```python
__new__[M: Literal[str], F: type, T: type, K: type](args: T, kwargs: K) -> Partial[(M, T, K, F)]
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

Special methods 4

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_fn\_name\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial-__fn_name__ "Copy/link to this API element")

@property

```python
__fn_name__(self: Self)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/#class-partial-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/
