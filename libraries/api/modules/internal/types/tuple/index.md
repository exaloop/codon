# internal.types.tuple

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/tuple.codon)

class Tuple[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-tuple "Copy/link to this API element")

@extend

```python
class Tuple
```

Extends [`internal.core.Tuple`](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/)

class FixedDict[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict "Copy/link to this API element")

@extend

```python
class FixedDict[_KeysId, T]
```

Extends [`internal.core.FixedDict`](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/FixedDict/)

Methods 4

keys[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-keys "Copy/link to this API element")

```python
keys(self: Self)
```

values[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-values "Copy/link to this API element")

```python
values(self: Self)
```

items[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-items "Copy/link to this API element")

```python
items(self: Self)
```

get[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-get "Copy/link to this API element")

```python
get(self: Self, key: str, default = None)
```

```python
get[key: Literal[str]](self: Self, default = None)
```

Special methods 6

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: str)
```

```python
__getitem__[key: Literal[str]](self: Self)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: str)
```

```python
__contains__[key: Literal[str]](self: Self) -> Literal[bool]
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/#class-fixeddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/tuple/
