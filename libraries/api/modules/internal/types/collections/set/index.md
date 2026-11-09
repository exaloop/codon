# internal.types.collections.set

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/set.codon)

class Set[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set "Copy/link to this API element")

```python
class Set[K]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/Set/)

Fields 1

`K` `type`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, g: Generator[K], Self: type)
```

Methods 18

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-resize "Copy/link to this API element")

```python
resize(self: Self, new_n_buckets: int, Self: type)
```

add[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-add "Copy/link to this API element")

```python
add(self: Self, key: K, Self: type)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-update "Copy/link to this API element")

```python
update(self: Self, other: Generator[K], Self: type)
```

remove[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-remove "Copy/link to this API element")

```python
remove(self: Self, key: K, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-pop "Copy/link to this API element")

```python
pop(self: Self, Self: type)
```

discard[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-discard "Copy/link to this API element")

```python
discard(self: Self, key: K, Self: type)
```

difference[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-difference "Copy/link to this API element")

```python
difference(self: Self, other: Set[K], Self: type)
```

difference\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-difference_update "Copy/link to this API element")

```python
difference_update(self: Self, other: Set[K], Self: type)
```

intersection[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-intersection "Copy/link to this API element")

```python
intersection(self: Self, other: Set[K], Self: type)
```

intersection\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-intersection_update "Copy/link to this API element")

```python
intersection_update(self: Self, other: Set[K], Self: type)
```

symmetric\_difference[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-symmetric_difference "Copy/link to this API element")

```python
symmetric_difference(self: Self, other: Set[K], Self: type)
```

symmetric\_difference\_update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-symmetric_difference_update "Copy/link to this API element")

```python
symmetric_difference_update(self: Self, other: Set[K], Self: type)
```

union[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-union "Copy/link to this API element")

```python
union(self: Self, other: Set[K], Self: type)
```

isdisjoint[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-isdisjoint "Copy/link to this API element")

```python
isdisjoint(self: Self, other: Set[K], Self: type)
```

issubset[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-issubset "Copy/link to this API element")

```python
issubset(self: Self, other: Set[K], Self: type)
```

issuperset[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-issuperset "Copy/link to this API element")

```python
issuperset(self: Self, other: Set[K], Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Special methods 21

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Set[K], Self: type)
```

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other: Set[K], Self: type)
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__and__ "Copy/link to this API element")

@commutative @associative

```python
__and__(self: Self, other: Set[K], Self: type)
```

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other: Set[K], Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__or__ "Copy/link to this API element")

@commutative @associative

```python
__or__(self: Self, other: Set[K], Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other: Set[K], Self: type)
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__xor__ "Copy/link to this API element")

@commutative @associative

```python
__xor__(self: Self, other: Set[K], Self: type)
```

\_\_ixor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ixor__ "Copy/link to this API element")

```python
__ixor__(self: Self, other: Set[K], Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Set[K], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Set[K], Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Set[K], Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Set[K], Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Set[K], Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Set[K], Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/#class-set-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/set/
