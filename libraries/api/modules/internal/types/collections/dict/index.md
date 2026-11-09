# internal.types.collections.dict

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/dict.codon)

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
with_value(self: Self, new_value: V, Self: type)
```

Special methods 1

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-dictentry-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
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

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/OrderedDict/)

Fields 2

`K` `type`

`V` `type`

Constructors 3

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Self, capacity: int = 8, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, items: Generator[Tuple[K, V]], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, other: OrderedDict[K, V], Self: type)
```

Methods 13

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int, Self: type)
```

get[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, Self: type)
```

```python
get(self: Self, key: K, default: V, Self: type)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, Self: type)
```

```python
pop(self: Self, key: K, missing, Self: type)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

keys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

values[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

items[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

increment[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-increment "Copy/link to this API element")

```python
increment(self: Self, key: K, by: T = 1, T: type, Self: type)
```

Special methods 17

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: OrderedDict[K, V], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: OrderedDict[K, V], Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: K, other: Z, op: F, F: type, Z: type, Self: type)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-ordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: K, other: Z, dflt: V, op: F, F: type, Z: type, Self: type)
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
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, capacity: int, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, g: Generator[Tuple[K, V]], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: UnorderedDict[K, V], Self: type)
```

Methods 13

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, new_n_buckets: int, Self: type)
```

get[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, default: V, Self: type)
```

```python
get(self: Self, key: K, Self: type)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, val: V, Self: type)
```

increment[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-increment "Copy/link to this API element")

```python
increment(self: Self, key: K, by: T = 1, T: type, Self: type)
```

update[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, Self: type)
```

```python
update(self: Self, other: UnorderedDict[K, V], Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, Self: type)
```

```python
pop(self: Self, key: K, missing, Self: type)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

items[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

keys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

values[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

Special methods 17

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, val: V, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: UnorderedDict[K, V], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: UnorderedDict[K, V], Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: K, other: Z, op: F, F: type, Z: type, Self: type)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: K, other: Z, dflt: V, op: F, F: type, Z: type, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/#class-unordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/
