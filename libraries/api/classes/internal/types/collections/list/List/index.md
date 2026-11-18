# List

Standard library class `internal.types.collections.list.List` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/list.codon#L23)

class List[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list "Copy/link to this API element")

```python
class List[T]
```

Fields 1

`T` `type`

Constructors 5

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, /, capacity: int)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, it: Generator[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__init__-overload-5 "Copy/link to this API element")

```python
__init__(self: Self, /, p: Ptr[T], n: int)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

Methods 12

append[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-append "Copy/link to this API element")

```python
append(self: Self, x: T, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

extend[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-extend "Copy/link to this API element")

```python
extend(self: Self, itr: Generator[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

insert[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-insert "Copy/link to this API element")

```python
insert(self: Self, idx: int, x: T, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

pop[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-pop "Copy/link to this API element")

```python
pop(self: Self, idx: int = -1, /) -> T
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

remove[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-remove "Copy/link to this API element")

```python
remove(self: Self, x: T, /) -> bool
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

clear[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

index[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-index "Copy/link to this API element")

```python
index(self: Self, x: T, /) -> int
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

find[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-find "Copy/link to this API element")

```python
find(self: Self, x: T) -> int
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

count[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-count "Copy/link to this API element")

```python
count(self: Self, x: T, /) -> int
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

reverse[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-reverse "Copy/link to this API element")

```python
reverse(self: Self, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

copy[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-copy "Copy/link to this API element")

```python
copy(self: Self, /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

sort[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-sort "Copy/link to this API element")

```python
sort[algorithm: Literal[str] = "auto"](self: Self, /, *, key = None, reverse: bool = False)
```

Added by [`internal.sort`](https://docs.exaloop.io/libraries/api/modules/internal/sort/)

Special methods 30

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, /) -> T
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

```python
__getitem__(self: Self, s: Slice, /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, idx: int, val: T, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

```python
__setitem__(self: Self, s: Slice, other: Generator[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

```python
__setitem__(self: Self, s: Slice, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, idx: int, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

```python
__delitem__(self: Self, s: Slice, /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: List[T], /) -> bool
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: List[T], /) -> bool
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, x: T, /) -> bool
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /) -> Generator[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: List[T], /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: List[T], /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, n: int, /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, n: int, /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, n: int, /) -> List[T]
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: List[T], /)
```

Defined in [`internal.types.collections.list`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__to_gpu__ "Copy/link to this API element")

@inline

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__from_gpu__ "Copy/link to this API element")

@inline

```python
__from_gpu__(self: Self, other: List[T])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__from_gpu_new__ "Copy/link to this API element")

@inline

```python
__from_gpu_new__(other: List[T])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> List[T]
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/#class-list-__from_py__ "Copy/link to this API element")

```python
__from_py__(v: cobj) -> List[T]
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/
