# FixedDict

Standard library class `internal.core.FixedDict` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L374)

class FixedDict[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict "Copy/link to this API element")

@tuple

```python
class FixedDict[_KeysId, T]
```

Fields 1

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__[_KeysId: Literal[int] = 0, T: type](values: T = ()) -> FixedDict[(_KeysId, T)]
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
%result = insertvalue { {=T} } undef, {=T} %values, 0
ret { {=T} } %result
```

Methods 4

keys[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-keys "Copy/link to this API element")

```python
keys(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

values[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-values "Copy/link to this API element")

```python
values(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

items[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-items "Copy/link to this API element")

```python
items(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

get[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-get "Copy/link to this API element")

```python
get(self: Self, key: str, default = None)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

```python
get[key: Literal[str]](self: Self, default = None)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

Special methods 7

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: str)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

```python
__getitem__[key: Literal[str]](self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: str)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

```python
__contains__[key: Literal[str]](self: Self) -> Literal[bool]
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/#class-fixeddict-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/
