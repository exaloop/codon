# Function

Standard library class `internal.core.Function` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon)

class Function[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function "Copy/link to this API element")

@tuple

```python
class Function[T, TR]
```

Fields 2

`T` `type`

`TR` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__()
```

Added by [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
ret ptr null
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__new__-overload-2 "Copy/link to this API element")

@pure @overload @llvm

```python
__new__(what: Ptr[u8])
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

```llvm
ret ptr %what
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: Function[T, TR])
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

Special methods 4

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__raw__ "Copy/link to this API element")

@pure @llvm

```python
__raw__(self: Self, Self: type)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

```llvm
ret ptr %self
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

\_\_call\_internal\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__call_internal__ "Copy/link to this API element")

@\_\_generated\_\_ @llvm

```python
__call_internal__(self: Function[T, TR], args: T)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

```llvm
noop  # compiler will populate this one
```

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/#class-function-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, Self: type)
```

Added by [`internal.types.function`](https://docs.exaloop.io/libraries/api/modules/internal/types/function/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Function/
