# NoneType

Standard library class `internal.core.NoneType` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L105)

class NoneType[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype "Copy/link to this API element")

@tuple

```python
class NoneType
```

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__() -> NoneType
```

Added by [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
ret {} {}
```

Special methods 9

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: NoneType)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: NoneType)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: NoneType)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: NoneType)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/#class-nonetype-__from_py__ "Copy/link to this API element")

```python
__from_py__(x: cobj) -> None
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/
