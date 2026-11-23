# internal.types.slice

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/slice.codon)

class Slice[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice "Copy/link to this API element")

@tuple

```python
class Slice[T, U, V]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/)

Fields 6

`start` `Optional[T]`

`stop` `Optional[U]`

`step` `Optional[V]`

`T` `type`

`U` `type`

`V` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__new__ "Copy/link to this API element")

```python
__new__[U: type = int](stop: Optional[U], /) -> Slice[(int, U, int)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__[T: type = int, U: type = int](start: Optional[T], stop: Optional[U], /) -> Slice[(T, U, int)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__[T: type = int, U: type = int, V: type = int](start: Optional[T], stop: Optional[U], step: Optional[V], /) -> Slice[(T, U, V)]
```

Methods 3

adjust\_indices[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-adjust_indices "Copy/link to this API element")

```python
adjust_indices(self: Self, length: int) -> Tuple[(int, int, int, int)]
```

adjust\_indices\_helper[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-adjust_indices_helper "Copy/link to this API element")

```python
adjust_indices_helper(length: int, start: int, stop: int, step: int) -> Tuple[(int, int, int, int)]
```

indices[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-indices "Copy/link to this API element")

```python
indices(self: Self, length: int, /)
```

Special methods 3

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__repr__ "Copy/link to this API element")

@overload

```python
__repr__(self: Self, /)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__eq__ "Copy/link to this API element")

@overload

```python
__eq__(self: Self, other: Slice, /)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#class-slice-__ne__ "Copy/link to this API element")

@overload

```python
__ne__(self: Self, other: Slice, /)
```

alias slice[](https://docs.exaloop.io/libraries/api/modules/internal/types/slice/#alias-slice "Copy/link to this API element")

```python
slice = Slice
```

Alias of [`Slice`](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/slice/
