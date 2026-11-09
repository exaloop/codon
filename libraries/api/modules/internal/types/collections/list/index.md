# internal.types.collections.list

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/list.codon)

class List[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list "Copy/link to this API element")

```python
class List[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/List/)

Fields 1

`T` `type`

Constructors 5

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, capacity: int, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, it: Generator[T], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: List[T], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__init__-overload-5 "Copy/link to this API element")

```python
__init__(self: Self, p: Ptr[T], n: int, Self: type)
```

Methods 11

append[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-append "Copy/link to this API element")

```python
append(self: Self, x: T, Self: type)
```

extend[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-extend "Copy/link to this API element")

```python
extend(self: Self, itr: Generator[T], Self: type)
```

insert[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-insert "Copy/link to this API element")

```python
insert(self: Self, idx: int, x: T, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-pop "Copy/link to this API element")

```python
pop(self: Self, idx: int = -1, Self: type)
```

remove[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-remove "Copy/link to this API element")

```python
remove(self: Self, x: T, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-index "Copy/link to this API element")

```python
index(self: Self, x: T, Self: type)
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-find "Copy/link to this API element")

```python
find(self: Self, x: T, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-count "Copy/link to this API element")

```python
count(self: Self, x: T, Self: type)
```

reverse[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-reverse "Copy/link to this API element")

```python
reverse(self: Self, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Special methods 23

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

```python
__getitem__(self: Self, s: Slice, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, idx: int, val: T, Self: type)
```

```python
__setitem__(self: Self, s: Slice, other: Generator[T], Self: type)
```

```python
__setitem__(self: Self, s: Slice, other: List[T], Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, idx: int, Self: type)
```

```python
__delitem__(self: Self, s: Slice, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: List[T], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: List[T], Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, x: T, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: List[T], Self: type)
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: List[T], Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, n: int, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, n: int, Self: type)
```

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, n: int, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: List[T], Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: List[T], Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: List[T], Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/#class-list-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: List[T], Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/list/
