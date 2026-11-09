# Union

Standard library class `internal.core.Union` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon)

class Union[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union "Copy/link to this API element")

@tuple

```python
class Union[TU]
```

Fields 1

`TU` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__new__ "Copy/link to this API element")

@\_\_generated\_\_

```python
__new__(val)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

Special methods 5

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__getattr__ "Copy/link to this API element")

```python
__getattr__(self: Self, member: Literal[str], Self: type)
```

Added by [`internal.types.union`](https://docs.exaloop.io/libraries/api/modules/internal/types/union/)

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs, Self: type)
```

Added by [`internal.types.union`](https://docs.exaloop.io/libraries/api/modules/internal/types/union/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__iter__ "Copy/link to this API element")

@\_\_generated\_\_

```python
__iter__(union)
```

Added by [`internal.types.union`](https://docs.exaloop.io/libraries/api/modules/internal/types/union/)

Acquire the active iterator before lazily adapting its elements.

```text
    The concrete __iter__ runs at iter(union), not at the first next(), preserving
    acquisition side effects and exceptions. A non-iterable alternative raises
    only when its tag is active; it does not invalidate other alternatives.
    Yield the shared element type, or a union of element types when they differ,
    so heterogeneous iterable alternatives remain usable in ordinary loops.
```

\_\_repr\_default\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__repr_default__ "Copy/link to this API element")

```python
__repr_default__(self: Self, Self: type)
```

Added by [`internal.types.union`](https://docs.exaloop.io/libraries/api/modules/internal/types/union/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/#class-union-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.types.union`](https://docs.exaloop.io/libraries/api/modules/internal/types/union/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Union/
