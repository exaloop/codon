# internal.types.range

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/range.codon)

class range[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range "Copy/link to this API element")

@tuple

```python
class range
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/range/range/)

Fields 3

`start` `int`

`stop` `int`

`step` `int`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__new__ "Copy/link to this API element")

@overload

```python
__new__(start: int, stop: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(stop: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(start: int, stop: int, step: int)
```

Methods 2

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-index "Copy/link to this API element")

```python
index(self: Self, n: int, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-count "Copy/link to this API element")

```python
count(self: Self, n: int, Self: type)
```

Special methods 7

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__getitem__ "Copy/link to this API element")

@overload

```python
__getitem__(self: Self, idx: int, Self: type)
```

```python
__getitem__(self: Self, s: Slice, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, idx: int, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__iter__ "Copy/link to this API element")

@overload

```python
__iter__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__len__ "Copy/link to this API element")

@overload

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/range/#class-range-__repr__ "Copy/link to this API element")

@overload

```python
__repr__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/range/
