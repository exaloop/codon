# UnorderedDict

Standard library class `internal.types.collections.dict.UnorderedDict` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/dict.codon#L574)

class UnorderedDict[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict "Copy/link to this API element")

```python
class UnorderedDict[K, V]
```

Fields 2

`K` `type`

`V` `type`

Constructors 4

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, /, capacity: int)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, g: Generator[Tuple[(K, V)]], /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: UnorderedDict[(K, V)], /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

Methods 13

resize[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, new_n_buckets: int)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

get[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, default: V, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
get(self: Self, key: K, /) -> Optional[V]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

setdefault[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, val: V, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

increment[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-increment "Copy/link to this API element")

```python
increment[T: type](self: Self, key: K, by: T = 1)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

update[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
update(self: Self, other: UnorderedDict[(K, V)], /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

pop[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
pop(self: Self, key: K, missing, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

popitem[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, /) -> Tuple[(K, V)]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

clear[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

items[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-items "Copy/link to this API element")

```python
items(self: Self, /) -> Generator[Tuple[(K, V)]]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

keys[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, /) -> Generator[K]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

values[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-values "Copy/link to this API element")

```python
values(self: Self, /) -> Generator[V]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

copy[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

fromkeys[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V, /) -> UnorderedDict[(K, V)]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

Special methods 17

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, val: V, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /) -> bool
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: UnorderedDict[(K, V)], /) -> bool
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: UnorderedDict[(K, V)], /) -> bool
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[K]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> UnorderedDict[(K, V)]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: K, other: Z, op: F)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: K, other: Z, dflt: V, op: F)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/#class-unordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/
