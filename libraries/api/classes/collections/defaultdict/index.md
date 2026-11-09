# defaultdict

Standard library class `collections.defaultdict` [View source](https://github.com/exaloop/codon/blob/master/stdlib/collections.codon)

class defaultdict[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict "Copy/link to this API element")

@tuple

```python
class defaultdict[K, V, S]
```

Fields 4

`default_factory` `S`

`K` `type`

`V` `type`

`S` `TypeTrait[CallableTrait[V]]`

Constructors 8

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__ "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-2 "Copy/link to this API element")

```python
__new__(f: S)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-3 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: OrderedDict[K, V])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-4 "Copy/link to this API element")

```python
__new__(f: S, other: OrderedDict[K, V])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-5 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: defaultdict[K, V, F], F: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-6 "Copy/link to this API element")

```python
__new__(f: S, other: defaultdict[K, V, F], F: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-7 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: Counter[K])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__new__-overload-8 "Copy/link to this API element")

```python
__new__(f: S, other: Counter[K])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Methods 13

keys[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

values[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

items[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

get[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-get "Copy/link to this API element")

```python
get(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
get(self: Self, key: K, default: V, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

setdefault[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

pop[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
pop(self: Self, key: K, missing, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

popitem[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

clear[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

resize[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

increment[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-increment "Copy/link to this API element")

```python
increment(self: Self, key: K, by: Z = 1, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

update[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-update "Copy/link to this API element")

```python
update(self: Self, other, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

fromkeys[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

copy[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Special methods 18

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_missing\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__missing__ "Copy/link to this API element")

```python
__missing__(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: K, other: Z, op: F, F: type, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: K, other: Z, default: V, op: F, F: type, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: defaultdict[K, V, F], F: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__eq__(self: Self, other: OrderedDict[K, V], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__eq__(self: Self, other: Counter[K], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: defaultdict[K, V, F], F: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ne__(self: Self, other: OrderedDict[K, V], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ne__(self: Self, other: Counter[K], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/#class-defaultdict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Source: https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/
