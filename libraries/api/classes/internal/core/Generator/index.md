# Generator

Standard library class `internal.core.Generator` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L124)

class Generator[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator "Copy/link to this API element")

@tuple

```python
class Generator[T]
```

Fields 1

`T` `type`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__(ptr: cobj) -> Generator[T]
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
ret ptr %ptr
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__() -> Generator[T]
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Methods 1

send[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-send "Copy/link to this API element")

```python
send(self: Self, what: T, /) -> T
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Special methods 7

\_\_promise\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__promise__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__promise__(self: Self) -> Ptr[T]
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_next\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__next__ "Copy/link to this API element")

```python
__next__(self: Generator[T], /) -> T
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[T]
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__raw__ "Copy/link to this API element")

@pure @derives @llvm

```python
__raw__(self: Self) -> Ptr[u8]
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
ret ptr %self
```

\_\_done\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__done__ "Copy/link to this API element")

@pure @llvm

```python
__done__(self: Self) -> bool
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
declare i1 @llvm.coro.done(ptr nocapture readonly)
%0 = call i1 @llvm.coro.done(ptr %self)
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_resume\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__resume__ "Copy/link to this API element")

@nocapture @llvm

```python
__resume__(self: Self) -> None
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
declare void @llvm.coro.resume(ptr)
call void @llvm.coro.resume(ptr %self)
ret {} {}
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/#class-generator-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Generator/
