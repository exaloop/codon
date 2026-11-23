# range

Standard library class `internal.types.range.range` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/range.codon#L4)

class range[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range "Copy/link to this API element")

@tuple

```python
class range
```

Fields 3

`start` `int`

`stop` `int`

`step` `int`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__new__ "Copy/link to this API element")

@overload

```python
__new__(start: int, stop: int, /) -> range
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(stop: int, /) -> range
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(start: int, stop: int, step: int, /) -> range
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

Methods 2

index[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-index "Copy/link to this API element")

```python
index(self: Self, n: int, /) -> int
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

count[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-count "Copy/link to this API element")

```python
count(self: Self, n: int, /) -> int
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

Special methods 7

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__getitem__ "Copy/link to this API element")

@overload

```python
__getitem__(self: Self, idx: int, /) -> int
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

```python
__getitem__(self: Self, s: Slice, /) -> range
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, idx: int, /) -> bool
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__iter__ "Copy/link to this API element")

@overload

```python
__iter__(self: Self, /) -> Generator[int]
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__len__ "Copy/link to this API element")

@overload

```python
__len__(self: Self, /) -> int
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, /) -> bool
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /) -> Generator[int]
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/#class-range-__repr__ "Copy/link to this API element")

@overload

```python
__repr__(self: Self, /) -> str
```

Defined in [`internal.types.range`](https://docs.exaloop.io/libraries/api/modules/internal/types/range/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/
