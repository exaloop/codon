# collections

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/collections.codon)

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
__init__(self: Self, /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, /, maxlen: int)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, /, it: Generator[T])
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, /, other: deque[T])
```

Properties 2

maxlen[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-maxlen "Copy/link to this API element")

@property

```python
maxlen(self: Self) -> int
```

left[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-left "Copy/link to this API element")

@property

```python
left(self: Self) -> T
```

Methods 6

appendleft[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-appendleft "Copy/link to this API element")

```python
appendleft(self: Self, x: T, /)
```

append[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-append "Copy/link to this API element")

```python
append(self: Self, x: T, /)
```

popleft[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-popleft "Copy/link to this API element")

```python
popleft(self: Self, /) -> T
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-pop "Copy/link to this API element")

```python
pop(self: Self, /) -> T
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-copy "Copy/link to this API element")

```python
copy(self: Self, /) -> deque[T]
```

Special methods 8

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[T]
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, x: T, /) -> bool
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> deque[T]
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, /) -> deque[T]
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-deque-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, /) -> T
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
__new__(capacity: int = 8) -> Counter[T]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-2 "Copy/link to this API element")

```python
__new__(elements: Generator[T], /) -> Counter[T]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-3 "Copy/link to this API element")

```python
__new__(other: Counter[T], /) -> Counter[T]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__new__-overload-4 "Copy/link to this API element")

```python
__new__(other: Dict[(T, int)], /) -> Counter[T]
```

Methods 17

keys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-keys "Copy/link to this API element")

```python
keys(self: Self, /)
```

values[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-values "Copy/link to this API element")

```python
values(self: Self, /)
```

items[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-items "Copy/link to this API element")

```python
items(self: Self, /)
```

get[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-get "Copy/link to this API element")

```python
get(self: Self, key: T, /)
```

```python
get(self: Self, key: T, default: int, /)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: T, value: int, /)
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-pop "Copy/link to this API element")

```python
pop(self: Self, key: T, /)
```

```python
pop(self: Self, key: T, missing, /)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-popitem "Copy/link to this API element")

```python
popitem(self: Self, /)
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

resize[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int)
```

increment[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-increment "Copy/link to this API element")

```python
increment[Z: type](self: Self, key: T, by: Z = 1)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-copy "Copy/link to this API element")

```python
copy(self: Self)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V)
```

elements[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-elements "Copy/link to this API element")

```python
elements(self: Self) -> Generator[T]
```

most\_common[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-most_common "Copy/link to this API element")

```python
most_common(self: Self, n: Optional[int] = None) -> List[Tuple[(T, int)]]
```

subtract[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-subtract "Copy/link to this API element")

```python
subtract(self: Self, elements: Generator[T], /)
```

```python
subtract(self: Self, other: Counter[T], /)
```

```python
subtract(self: Self, other: Dict[(T, int)], /)
```

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-update "Copy/link to this API element")

```python
update(self: Self, elements: Generator[T], /)
```

```python
update(self: Self, other: Counter[T], /)
```

```python
update(self: Self, other: Dict[(T, int)], /)
```

```python
update(self: Self, /)
```

total[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-total "Copy/link to this API element")

```python
total(self: Self) -> int
```

Special methods 25

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: T, /)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: T, value: int, /)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: T, /) -> int
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: T)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Counter[T]) -> bool
```

```python
__eq__(self: Self, other: Dict[(T, int)]) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: Counter[T]) -> bool
```

```python
__ne__(self: Self, other: Dict[(T, int)]) -> bool
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> Counter[T]
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other)
```

```python
__or__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other)
```

```python
__ior__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> Counter[T]
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self) -> Counter[T]
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Counter[T]) -> Counter[T]
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: T, other: Z, default: int, op: F)
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: T, other: Z, op: F)
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`internal.types.collections.dict.OrderedDict`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Dict[(K, int)], other: Counter[K])
```

Methods 1

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: Dict[(K, int)], other: Counter[K])
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Dict[(K, int)], other: Counter[K])
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Dict[(K, int)], other: Counter[K])
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

`S` `TypeTrait[CallableTrait[([], V)]]`

Constructors 8

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__ "Copy/link to this API element")

```python
__new__[VV: TypeTrait[V]]() -> defaultdict[(K, VV, Function[([], V)])]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-2 "Copy/link to this API element")

```python
__new__(f: S, /) -> defaultdict[(K, V, S)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-3 "Copy/link to this API element")

```python
__new__[VV: TypeTrait[V]](other: Dict[(K, V)], /) -> defaultdict[(K, VV, Function[([], V)])]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-4 "Copy/link to this API element")

```python
__new__(f: S, other: Dict[(K, V)], /) -> defaultdict[(K, V, S)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-5 "Copy/link to this API element")

```python
__new__[VV: TypeTrait[V], F: type](other: defaultdict[(K, V, F)], /) -> defaultdict[(K, VV, Function[([], V)])]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-6 "Copy/link to this API element")

```python
__new__[F: type](f: S, other: defaultdict[(K, V, F)], /) -> defaultdict[(K, V, S)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-7 "Copy/link to this API element")

```python
__new__[VV: TypeTrait[V]](other: Counter[K], /) -> defaultdict[(K, VV, Function[([], V)])]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__new__-overload-8 "Copy/link to this API element")

```python
__new__(f: S, other: Counter[K], /) -> defaultdict[(K, V, S)]
```

Methods 13

keys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-keys "Copy/link to this API element")

```python
keys(self: Self, /)
```

values[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-values "Copy/link to this API element")

```python
values(self: Self, /)
```

items[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-items "Copy/link to this API element")

```python
items(self: Self, /)
```

get[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-get "Copy/link to this API element")

```python
get(self: Self, key: K, /)
```

```python
get(self: Self, key: K, default: V, /)
```

setdefault[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, /)
```

pop[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, /)
```

```python
pop(self: Self, key: K, missing, /)
```

popitem[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-popitem "Copy/link to this API element")

```python
popitem(self: Self, /)
```

clear[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

resize[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int)
```

increment[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-increment "Copy/link to this API element")

```python
increment[Z: type](self: Self, key: K, by: Z = 1)
```

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-update "Copy/link to this API element")

```python
update(self: Self, other, /)
```

fromkeys[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V, /)
```

copy[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-copy "Copy/link to this API element")

```python
copy(self: Self, /)
```

Special methods 18

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, /)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, /)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, /)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, /)
```

\_\_missing\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__missing__ "Copy/link to this API element")

```python
__missing__(self: Self, key: K, /)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, /) -> V
```

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: K, other: Z, op: F)
```

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: K, other: Z, default: V, op: F)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, /)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__eq__ "Copy/link to this API element")

```python
__eq__[F: type](self: Self, other: defaultdict[(K, V, F)], /) -> bool
```

```python
__eq__(self: Self, other: Dict[(K, V)], /) -> bool
```

```python
__eq__(self: Self, other: Counter[K], /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__ne__ "Copy/link to this API element")

```python
__ne__[F: type](self: Self, other: defaultdict[(K, V, F)], /) -> bool
```

```python
__ne__(self: Self, other: Dict[(K, V)], /) -> bool
```

```python
__ne__(self: Self, other: Counter[K], /) -> bool
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-defaultdict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`internal.types.collections.dict.OrderedDict`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Methods 1

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-update "Copy/link to this API element")

```python
update[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
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
__new__[S: type](other: defaultdict[(T, int, S)], /) -> Counter[T]
```

Methods 2

update[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-update "Copy/link to this API element")

```python
update[S: type](self: Self, other: defaultdict[(T, int, S)], /)
```

subtract[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-subtract "Copy/link to this API element")

```python
subtract[S: type](self: Self, other: defaultdict[(T, int, S)], /)
```

Special methods 2

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__eq__ "Copy/link to this API element")

```python
__eq__[S: type](self: Self, other: defaultdict[(T, int, S)])
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/collections/#class-counter-__ne__ "Copy/link to this API element")

```python
__ne__[S: type](self: Self, other: defaultdict[(T, int, S)])
```

function namedtuple[](https://docs.exaloop.io/libraries/api/modules/collections/#function-namedtuple "Copy/link to this API element")

@\_\_generated\_\_

```python
namedtuple[name: Literal[str]](args)
```

Source: https://docs.exaloop.io/libraries/api/modules/collections/
