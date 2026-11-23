# internal.types.collections.dict

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/dict.codon)

class DictEntry[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-dictentry "Copy/link to this API element")

@tuple

```python
class DictEntry[K, V]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/DictEntry/)

Fields 5

`key` `K`

`value` `V`

`hash` `int`

`K` `type`

`V` `type`

Methods 1

with\_value[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-dictentry-with_value "Copy/link to this API element")

```python
with_value(self: Self, new_value: V)
```

Special methods 1

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-dictentry-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self)
```

class LookupResult[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-lookupresult "Copy/link to this API element")

@tuple

```python
class LookupResult
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/LookupResult/)

Fields 2

`slot` `int`

`found` `bool`

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict "Copy/link to this API element")

```python
class OrderedDict[K, V]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/)

Fields 2

`K` `type`

`V` `type`

Constructors 3

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, capacity: int = 8)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, items: Generator[Tuple[(K, V)]], /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, other: OrderedDict[(K, V)], /)
```

Methods 13

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, /)
```

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int)
```

get[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, /) -> Optional[V]
```

```python
get(self: Self, key: K, default: V, /)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, /) -> V
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, /) -> V
```

```python
pop(self: Self, key: K, missing, /)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, /)
```

keys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, /)
```

values[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-values "Copy/link to this API element")

```python
values(self: Self, /)
```

items[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-items "Copy/link to this API element")

```python
items(self: Self, /)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, /)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V, /) -> OrderedDict[(K, V)]
```

increment[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-increment "Copy/link to this API element")

```python
increment[T: type](self: Self, key: K, by: T = 1)
```

Special methods 17

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, /)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, /)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, /)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: OrderedDict[(K, V)], /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: OrderedDict[(K, V)], /) -> bool
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, /)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, /)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: K, other: Z, op: F)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: K, other: Z, dflt: V, op: F)
```

class UnorderedDict[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict "Copy/link to this API element")

```python
class UnorderedDict[K, V]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/UnorderedDict/)

Fields 2

`K` `type`

`V` `type`

Constructors 4

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, /, capacity: int)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, g: Generator[Tuple[(K, V)]], /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: UnorderedDict[(K, V)], /)
```

Methods 13

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, new_n_buckets: int)
```

get[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, default: V, /) -> V
```

```python
get(self: Self, key: K, /) -> Optional[V]
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, val: V, /) -> V
```

increment[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-increment "Copy/link to this API element")

```python
increment[T: type](self: Self, key: K, by: T = 1)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, /)
```

```python
update(self: Self, other: UnorderedDict[(K, V)], /)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, /) -> V
```

```python
pop(self: Self, key: K, missing, /)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, /) -> Tuple[(K, V)]
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

items[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-items "Copy/link to this API element")

```python
items(self: Self, /) -> Generator[Tuple[(K, V)]]
```

keys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, /) -> Generator[K]
```

values[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-values "Copy/link to this API element")

```python
values(self: Self, /) -> Generator[V]
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, /)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V, /) -> UnorderedDict[(K, V)]
```

Special methods 17

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, /) -> V
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, val: V, /)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, /)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /) -> bool
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: UnorderedDict[(K, V)], /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: UnorderedDict[(K, V)], /) -> bool
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[K]
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, /)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, /)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> UnorderedDict[(K, V)]
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: K, other: Z, op: F)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: K, other: Z, dflt: V, op: F)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/
