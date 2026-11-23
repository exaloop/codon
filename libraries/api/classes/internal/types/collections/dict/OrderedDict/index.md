# OrderedDict

Standard library class `internal.types.collections.dict.OrderedDict` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/dict.codon#L61)

class OrderedDict[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict "Copy/link to this API element")

```python
class OrderedDict[K, V]
```

Fields 2

`K` `type`

`V` `type`

Constructors 5

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /, capacity: int = 8)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, items: Generator[Tuple[(K, V)]], /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, other: OrderedDict[(K, V)], /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Dict[(K, int)], other: Counter[K])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__init__-overload-5 "Copy/link to this API element")

```python
__init__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

Methods 13

copy[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-copy "Copy/link to this API element")

```python
copy(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

resize[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-resize "Copy/link to this API element")

```python
resize(self: Self, minused: int)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

get[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-get "Copy/link to this API element")

```python
get(self: Self, key: K, /) -> Optional[V]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
get(self: Self, key: K, default: V, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

setdefault[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-setdefault "Copy/link to this API element")

```python
setdefault(self: Self, key: K, value: V, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

pop[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-pop "Copy/link to this API element")

```python
pop(self: Self, key: K, /) -> V
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
pop(self: Self, key: K, missing, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

popitem[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-popitem "Copy/link to this API element")

```python
popitem(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

keys[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-keys "Copy/link to this API element")

```python
keys(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

values[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-values "Copy/link to this API element")

```python
values(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

items[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-items "Copy/link to this API element")

```python
items(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

update[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-update "Copy/link to this API element")

```python
update(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
update(self: Dict[(K, int)], other: Counter[K])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
update[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

clear[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

fromkeys[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-fromkeys "Copy/link to this API element")

```python
fromkeys[K: type, V: type](ks: Generator[K], v: V, /) -> OrderedDict[(K, V)]
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

increment[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-increment "Copy/link to this API element")

```python
increment[T: type](self: Self, key: K, by: T = 1)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

Special methods 24

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key: K, value: V, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key: K, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: K, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key: K, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: OrderedDict[(K, V)], /) -> bool
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
__eq__(self: Dict[(K, int)], other: Counter[K])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__eq__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: OrderedDict[(K, V)], /) -> bool
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

```python
__ne__(self: Dict[(K, int)], other: Counter[K])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

```python
__ne__[S: type](self: Dict[(K, V)], other: defaultdict[(K, V, S)])
```

Added by [`collections`](https://docs.exaloop.io/libraries/api/modules/collections/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_dict\_do\_op\_throws\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__dict_do_op_throws__ "Copy/link to this API element")

```python
__dict_do_op_throws__[F: type, Z: type](self: Self, key: K, other: Z, op: F)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_dict\_do\_op\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__dict_do_op__ "Copy/link to this API element")

```python
__dict_do_op__[F: type, Z: type](self: Self, key: K, other: Z, dflt: V, op: F)
```

Defined in [`internal.types.collections.dict`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/dict/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Dict[(K, V)])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Dict[(K, V)])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> Dict[(K, V)]
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/#class-ordereddict-__from_py__ "Copy/link to this API element")

```python
__from_py__(d: cobj) -> Dict[(K, V)]
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/
