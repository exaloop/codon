# internal.types.function

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/function.codon)

class Function[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function "Copy/link to this API element")

@extend

```python
class Function[T, TR]
```

Extends [`internal.core.Function`](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Function/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__new__ "Copy/link to this API element")

@pure @overload @llvm

```python
__new__(what: Ptr[u8]) -> Function[(T, TR)]
```

```llvm
ret ptr %what
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: Function[(T, TR)]) -> Function[(T, TR)]
```

Special methods 4

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__raw__ "Copy/link to this API element")

@pure @llvm

```python
__raw__(self: Self) -> Ptr[u8]
```

```llvm
ret ptr %self
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_call\_internal\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__call_internal__ "Copy/link to this API element")

@\_\_generated\_\_ @llvm\_c\_call @llvm

```python
__call_internal__(self: Function[(T, TR)], args: T) -> TR
```

```llvm
noop  # compiler will populate this one
```

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-function-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args) -> TR
```

class Callable[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable "Copy/link to this API element")

@extend

```python
class Callable[T, TR]
```

Extends [`internal.core.Callable`](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Callable/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable-__new__ "Copy/link to this API element")

```python
__new__(fn: Function[([Ptr[u8], T], TR)], data: Ptr[u8]) -> Callable[(T, TR)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__[T: type, TR: type, M: Literal[str], PT: type, F: type, K: type](fn: Function[([Ptr[u8], T], TR)], data: Partial[(M, PT, K, F)]) -> Callable[(T, TR)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(fn: Function[([Ptr[u8], T], TR)], data: Function[(T, TR)]) -> Callable[(T, TR)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(fn: Function[(T, TR)]) -> Callable[(T, TR)]
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-callable-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args)
```

class Partial[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-partial "Copy/link to this API element")

@extend

```python
class Partial[M, T, K, F]
```

Extends [`internal.core.Partial`](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Partial/)

Special methods 4

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-partial-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-partial-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs)
```

\_\_fn\_name\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-partial-__fn_name__ "Copy/link to this API element")

@property

```python
__fn_name__(self: Self)
```

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/function/#class-partial-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/function/
