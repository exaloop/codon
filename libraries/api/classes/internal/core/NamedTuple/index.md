# NamedTuple

Standard library class `internal.core.NamedTuple` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon)

class NamedTuple[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple "Copy/link to this API element")

@tuple

```python
class NamedTuple[N, T]
```

Fields 3

`args` `T`

`N` `Literal[int]`

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-__new__ "Copy/link to this API element")

@pure @derives @llvm

```python
__new__(args: T = (), N: Literal[int] = 0, T: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
%0 = insertvalue { {=T} } undef, {=T} %args, 0
ret { {=T} } %0
```

Methods 1

get[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-get "Copy/link to this API element")

```python
get(self: Self, key: Literal[str], default = None, Self: type)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

Special methods 4

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: Literal[str], Self: type)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: Literal[str], Self: type)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_keys\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-__keys__ "Copy/link to this API element")

```python
__keys__(self: Self, Self: type)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/#class-namedtuple-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.tuple`](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/NamedTuple/
