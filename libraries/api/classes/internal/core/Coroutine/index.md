# Coroutine

Standard library class `internal.core.Coroutine` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon)

class Coroutine[](https://docs.exaloop.io/libraries/api/classes/internal/core/Coroutine/#class-coroutine "Copy/link to this API element")

@tuple

```python
class Coroutine[T]
```

Fields 1

`T` `type`

Special methods 2

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Coroutine/#class-coroutine-__raw__ "Copy/link to this API element")

@pure @derives @llvm

```python
__raw__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
ret ptr %self
```

\_\_await\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Coroutine/#class-coroutine-__await__ "Copy/link to this API element")

@pure @derives @llvm

```python
__await__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
ret ptr %self
```

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Coroutine/
