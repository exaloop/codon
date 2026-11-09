# TypeInfo

Standard library class `internal.types.rtti.TypeInfo` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/rtti.codon)

class TypeInfo[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo "Copy/link to this API element")

```python
class TypeInfo
```

Fields 20

`id` `int`

`n_mro` `int`

`mro` `Ptr[int]`

`offsets` `Ptr[int]`

`sorted_mro` `Ptr[int]`

`n_slots` `int`

`slots` `Ptr[Tuple[str, int, int]]`

`n_methods` `int`

`methods` `Ptr[_MethodSlot]`

`rtti` `bool`

`raw_name` `str`

`nice_name` `str`

`base_name` `str`

`n_params` `int`

`params` `Ptr[int]`

`size` `int`

`reference` `bool`

`inline_value` `bool`

`exception_offset` `int`

`system_exit_offset` `int`

Methods 6

value\_hash[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-value_hash "Copy/link to this API element")

```python
value_hash(self: Self, data: Ptr[byte], Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

Hash an erased payload using this descriptor's registered adapter.

value\_equal[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-value_equal "Copy/link to this API element")

```python
value_equal(self: Self, left: Ptr[byte], right: Ptr[byte], Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

Compare two erased payloads of this descriptor's type.

value\_dict[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-value_dict "Copy/link to this API element")

```python
value_dict(self: Self, data: Ptr[byte], Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

Copy an erased dictionary through its typed items iterator.

cache[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-cache "Copy/link to this API element")

```python
cache(id: int)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

```python
cache(T: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

is\_parent[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-is_parent "Copy/link to this API element")

```python
is_parent(self: Self, T: type, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

wrap[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-wrap "Copy/link to this API element")

```python
wrap(arg: Ptr[u8], T: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

Special methods 6

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: TypeInfo, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/#class-typeinfo-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: TypeInfo, Self: type)
```

Defined in [`internal.types.rtti`](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/
