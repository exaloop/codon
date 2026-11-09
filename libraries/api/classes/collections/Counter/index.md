# Counter

Standard library class `collections.Counter` [View source](https://github.com/exaloop/codon/blob/master/stdlib/collections.codon)

class Counter[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter "Copy/link to this API element")

@tuple

```python
class Counter[T]
```

Fields 1

`T` `type`

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__new__ "Copy/link to this API element")

```python
__new__(capacity: int = 8)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__new__-overload-2 "Copy/link to this API element")

```python
__new__(elements: Generator[T])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__new__-overload-3 "Copy/link to this API element")

```python
__new__(other: Counter[T])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__new__-overload-4 "Copy/link to this API element")

```python
__new__(other: OrderedDict[T, int])
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__new__-overload-5 "Copy/link to this API element")

```python
__new__(other: defaultdict[T, int, S], S: type)
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Methods 17

keys[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

values[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

items[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

get[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-get "Copy/link to this API element")

```python
get(self: Self, key: T, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
get(self: Self, key: T, default: int, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

setdefault[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: T, value: int, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

pop[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-pop "Copy/link to this API element")

```python
pop(self: Self, key: T, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
pop(self: Self, key: T, missing, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

popitem[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

clear[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

resize[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

increment[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-increment "Copy/link to this API element")

```python
increment(self: Self, key: T, by: Z = 1, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

copy[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

fromkeys[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

elements[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-elements "Copy/link to this API element")

```python
elements(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

most\_common[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-most_common "Copy/link to this API element")

```python
most_common(self: Self, n: Optional[int] = None, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

subtract[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-subtract "Copy/link to this API element")

```python
subtract(self: Self, elements: Generator[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
subtract(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
subtract(self: Self, other: OrderedDict[T, int], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
subtract(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

update[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-update "Copy/link to this API element")

```python
update(self: Self, elements: Generator[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
update(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
update(self: Self, other: OrderedDict[T, int], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
update(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
update(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

total[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-total "Copy/link to this API element")

```python
total(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Special methods 25

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: T, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: T, value: int, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: T, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: T, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__eq__(self: Self, other: OrderedDict[T, int], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__eq__(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ne__(self: Self, other: OrderedDict[T, int], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ne__(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__or__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ior__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Counter[T], Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: T, other: Z, default: int, op: F, F: type, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/classes/collections/Counter/#class-counter-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: T, other: Z, op: F, F: type, Z: type, Self: type)
```

Defined in [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Source: https://docs.exaloop.io/libraries/api/classes/collections/Counter/
