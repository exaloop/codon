# collections

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/collections.codon)

class deque[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque "Copy/link to this API element")

```python
class deque[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/collections/deque/)

Fields 1

`T` `type`

Constructors 4

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, maxlen: int, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, it: Generator[T], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, other: deque[T], Self: type)
```

Properties 2

maxlen[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-maxlen "Copy/link to this API element")

@property

```python
maxlen(self: Self, Self: type)
```

left[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-left "Copy/link to this API element")

@property

```python
left(self: Self, Self: type)
```

Methods 6

appendleft[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-appendleft "Copy/link to this API element")

```python
appendleft(self: Self, x: T, Self: type)
```

append[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-append "Copy/link to this API element")

```python
append(self: Self, x: T, Self: type)
```

popleft[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-popleft "Copy/link to this API element")

```python
popleft(self: Self, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-pop "Copy/link to this API element")

```python
pop(self: Self, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Special methods 8

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, x: T, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

class Counter[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter "Copy/link to this API element")

@tuple

```python
class Counter[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/collections/Counter/)

Fields 1

`T` `type`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__ "Copy/link to this API element")

```python
__new__(capacity: int = 8)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-2 "Copy/link to this API element")

```python
__new__(elements: Generator[T])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-3 "Copy/link to this API element")

```python
__new__(other: Counter[T])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-4 "Copy/link to this API element")

```python
__new__(other: OrderedDict[T, int])
```

Methods 17

keys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

values[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

items[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

get[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-get "Copy/link to this API element")

```python
get(self: Self, key: T, Self: type)
```

```python
get(self: Self, key: T, default: int, Self: type)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: T, value: int, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-pop "Copy/link to this API element")

```python
pop(self: Self, key: T, Self: type)
```

```python
pop(self: Self, key: T, missing, Self: type)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

resize[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int, Self: type)
```

increment[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-increment "Copy/link to this API element")

```python
increment(self: Self, key: T, by: Z = 1, Z: type, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

elements[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-elements "Copy/link to this API element")

```python
elements(self: Self, Self: type)
```

most\_common[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-most_common "Copy/link to this API element")

```python
most_common(self: Self, n: Optional[int] = None, Self: type)
```

subtract[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-subtract "Copy/link to this API element")

```python
subtract(self: Self, elements: Generator[T], Self: type)
```

```python
subtract(self: Self, other: Counter[T], Self: type)
```

```python
subtract(self: Self, other: OrderedDict[T, int], Self: type)
```

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-update "Copy/link to this API element")

```python
update(self: Self, elements: Generator[T], Self: type)
```

```python
update(self: Self, other: Counter[T], Self: type)
```

```python
update(self: Self, other: OrderedDict[T, int], Self: type)
```

```python
update(self: Self, Self: type)
```

total[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-total "Copy/link to this API element")

```python
total(self: Self, Self: type)
```

Special methods 25

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: T, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: T, value: int, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: T, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: T, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Counter[T], Self: type)
```

```python
__eq__(self: Self, other: OrderedDict[T, int], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Counter[T], Self: type)
```

```python
__ne__(self: Self, other: OrderedDict[T, int], Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

```python
__or__(self: Self, other: Counter[T], Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

```python
__ior__(self: Self, other: Counter[T], Self: type)
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: Counter[T], Self: type)
```

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other: Counter[T], Self: type)
```

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other: Counter[T], Self: type)
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Counter[T], Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Counter[T], Self: type)
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Counter[T], Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: T, other: Z, default: int, op: F, F: type, Z: type, Self: type)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: T, other: Z, op: F, F: type, Z: type, Self: type)
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`OrderedDict`](https://docs.exaloop.io/libraries/api/classes/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/OrderedDict/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: OrderedDict[K, int], other: Counter[K])
```

Methods 1

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: OrderedDict[K, int], other: Counter[K])
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: OrderedDict[K, int], other: Counter[K])
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: OrderedDict[K, int], other: Counter[K])
```

class defaultdict[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict "Copy/link to this API element")

@tuple

```python
class defaultdict[K, V, S]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/collections/defaultdict/)

Fields 4

`default_factory` `S`

`K` `type`

`V` `type`

`S` `TypeTrait[CallableTrait[V]]`

Constructors 8

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__ "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-2 "Copy/link to this API element")

```python
__new__(f: S)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-3 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: OrderedDict[K, V])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-4 "Copy/link to this API element")

```python
__new__(f: S, other: OrderedDict[K, V])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-5 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: defaultdict[K, V, F], F: type)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-6 "Copy/link to this API element")

```python
__new__(f: S, other: defaultdict[K, V, F], F: type)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-7 "Copy/link to this API element")

```python
__new__(VV: TypeTrait[V], other: Counter[K])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-8 "Copy/link to this API element")

```python
__new__(f: S, other: Counter[K])
```

Methods 13

keys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

values[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

items[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

get[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-get "Copy/link to this API element")

```python
get(self: Self, key: K, Self: type)
```

```python
get(self: Self, key: K, default: V, Self: type)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, Self: type)
```

```python
pop(self: Self, key: K, missing, Self: type)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-popitem "Copy/link to this API element")

```python
popitem(self: Self, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

resize[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int, Self: type)
```

increment[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-increment "Copy/link to this API element")

```python
increment(self: Self, key: K, by: Z = 1, Z: type, Self: type)
```

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-update "Copy/link to this API element")

```python
update(self: Self, other, Self: type)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-fromkeys "Copy/link to this API element")

```python
fromkeys(ks: Generator[K], v: V, K: type, V: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Special methods 18

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

\_\_missing\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__missing__ "Copy/link to this API element")

```python
__missing__(self: Self, key: K, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, Self: type)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__(self: Self, key: K, other: Z, op: F, F: type, Z: type, Self: type)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__(self: Self, key: K, other: Z, default: V, op: F, F: type, Z: type, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: defaultdict[K, V, F], F: type, Self: type)
```

```python
__eq__(self: Self, other: OrderedDict[K, V], Self: type)
```

```python
__eq__(self: Self, other: Counter[K], Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: defaultdict[K, V, F], F: type, Self: type)
```

```python
__ne__(self: Self, other: OrderedDict[K, V], Self: type)
```

```python
__ne__(self: Self, other: Counter[K], Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`OrderedDict`](https://docs.exaloop.io/libraries/api/classes/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/OrderedDict/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: OrderedDict[K, V], other: defaultdict[K, V, S], S: type)
```

Methods 1

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: OrderedDict[K, V], other: defaultdict[K, V, S], S: type)
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: OrderedDict[K, V], other: defaultdict[K, V, S], S: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: OrderedDict[K, V], other: defaultdict[K, V, S], S: type)
```

class Counter[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter "Copy/link to this API element")

@extend

```python
class Counter[T]
```

Extends [`collections.Counter`](https://docs.exaloop.io/libraries/api/classes/collections/Counter/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/collections/Counter/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__ "Copy/link to this API element")

```python
__new__(other: defaultdict[T, int, S], S: type)
```

Methods 2

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-update "Copy/link to this API element")

```python
update(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

subtract[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-subtract "Copy/link to this API element")

```python
subtract(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: defaultdict[T, int, S], S: type, Self: type)
```

function namedtuple[](https://docs.exaloop.io/libraries/api/modules/collections/#function-namedtuple "Copy/link to this API element")

@\_\_generated\_\_

```python
namedtuple(name: Literal[str], args)
```

Source: https://docs.exaloop.io/libraries/api/modules/collections/
