# internal.types.collections.set

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/set.codon)

class Set[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set "Copy/link to this API element")

```python
class Set[K]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/set/Set/)

Fields 1

`K` `type`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, g: Generator[K], /)
```

Methods 18

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-resize "Copy/link to this API element")

```python
resize(self: Self, new_n_buckets: int)
```

add[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-add "Copy/link to this API element")

```python
add(self: Self, key: K, /)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-update "Copy/link to this API element")

```python
update(self: Self, other: Generator[K], /)
```

remove[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-remove "Copy/link to this API element")

```python
remove(self: Self, key: K, /)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-pop "Copy/link to this API element")

```python
pop(self: Self, /) -> K
```

discard[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-discard "Copy/link to this API element")

```python
discard(self: Self, key: K, /)
```

difference[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-difference "Copy/link to this API element")

```python
difference(self: Self, other: Set[K], /) -> Set[K]
```

difference\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-difference_update "Copy/link to this API element")

```python
difference_update(self: Self, other: Set[K], /)
```

intersection[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-intersection "Copy/link to this API element")

```python
intersection(self: Self, other: Set[K], /) -> Set[K]
```

intersection\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-intersection_update "Copy/link to this API element")

```python
intersection_update(self: Self, other: Set[K], /)
```

symmetric\_difference[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-symmetric_difference "Copy/link to this API element")

```python
symmetric_difference(self: Self, other: Set[K], /) -> Set[K]
```

symmetric\_difference\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-symmetric_difference_update "Copy/link to this API element")

```python
symmetric_difference_update(self: Self, other: Set[K], /)
```

union[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-union "Copy/link to this API element")

```python
union(self: Self, other: Set[K], /) -> Set[K]
```

isdisjoint[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-isdisjoint "Copy/link to this API element")

```python
isdisjoint(self: Self, other: Set[K], /) -> bool
```

issubset[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-issubset "Copy/link to this API element")

```python
issubset(self: Self, other: Set[K], /) -> bool
```

issuperset[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-issuperset "Copy/link to this API element")

```python
issuperset(self: Self, other: Set[K], /) -> bool
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-copy "Copy/link to this API element")

```python
copy(self: Self, /) -> Set[K]
```

Special methods 21

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__and__ "Copy/link to this API element")

@commutative @associative

```python
__and__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__or__ "Copy/link to this API element")

@commutative @associative

```python
__or__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__xor__ "Copy/link to this API element")

@commutative @associative

```python
__xor__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_ixor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ixor__ "Copy/link to this API element")

```python
__ixor__(self: Self, other: Set[K], /) -> Set[K]
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /) -> bool
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Set[K], /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Set[K], /) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Set[K], /) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Set[K], /) -> bool
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Set[K], /) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Set[K], /) -> bool
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[K]
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> Set[K]
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> Set[K]
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

alias set[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#alias-set "Copy/link to this API element")

```python
set = Set
```

Alias of [`Set`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/set/Set/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/
