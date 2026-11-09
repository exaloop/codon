# internal.types.union

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/union.codon)

class Union[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union "Copy/link to this API element")

@extend

```python
class Union[TU]
```

Extends [`internal.core.Union`](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Union/)

Special methods 5

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union-__getattr__ "Copy/link to this API element")

```python
__getattr__(self: Self, member: Literal[str], Self: type)
```

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union-__iter__ "Copy/link to this API element")

@\_\_generated\_\_

```python
__iter__(union)
```

Acquire the active iterator before lazily adapting its elements.

```text
    The concrete __iter__ runs at iter(union), not at the first next(), preserving
    acquisition side effects and exceptions. A non-iterable alternative raises
    only when its tag is active; it does not invalidate other alternatives.
    Yield the shared element type, or a union of element types when they differ,
    so heterogeneous iterable alternatives remain usable in ordinary loops.
```

\_\_repr\_default\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union-__repr_default__ "Copy/link to this API element")

```python
__repr_default__(self: Self, Self: type)
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/union/#class-union-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/union/
