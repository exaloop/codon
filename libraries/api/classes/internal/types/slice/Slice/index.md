# Slice

Standard library class `internal.types.slice.Slice` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/slice.codon)

class Slice[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice "Copy/link to this API element")

@tuple

```python
class Slice[T, U, V]
```

Fields 6

`start` `Optional[T]`

`stop` `Optional[U]`

`step` `Optional[V]`

`T` `type`

`U` `type`

`V` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__new__ "Copy/link to this API element")

```python
__new__(stop: Optional[U], U: type = int)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(start: Optional[T], stop: Optional[U], T: type = int, U: type = int)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(start: Optional[T], stop: Optional[U], step: Optional[V], T: type = int, U: type = int, V: type = int)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

Methods 3

adjust\_indices[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-adjust_indices "Copy/link to this API element")

```python
adjust_indices(self: Self, length: int, Self: type)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

adjust\_indices\_helper[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-adjust_indices_helper "Copy/link to this API element")

```python
adjust_indices_helper(length: int, start: int, stop: int, step: int)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

indices[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-indices "Copy/link to this API element")

```python
indices(self: Self, length: int, Self: type)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

Special methods 5

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__repr__ "Copy/link to this API element")

@overload

```python
__repr__(self: Self, Self: type)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__eq__ "Copy/link to this API element")

@overload

```python
__eq__(self: Self, other: Slice, Self: type)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__ne__ "Copy/link to this API element")

@overload

```python
__ne__(self: Self, other: Slice, Self: type)
```

Defined in [`internal.types.slice`](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/#class-slice-__from_py__ "Copy/link to this API element")

```python
__from_py__(s: cobj)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/
