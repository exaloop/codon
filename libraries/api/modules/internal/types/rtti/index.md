# internal.types.rtti

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/rtti.codon)

function force\_cast[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#function-force_cast "Copy/link to this API element")

@inline

```python
force_cast(obj, D: type)
```

class TypeInfo[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo "Copy/link to this API element")

```python
class TypeInfo
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/rtti/TypeInfo/)

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

value\_hash[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-value_hash "Copy/link to this API element")

```python
value_hash(self: Self, data: Ptr[byte], Self: type)
```

Hash an erased payload using this descriptor's registered adapter.

value\_equal[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-value_equal "Copy/link to this API element")

```python
value_equal(self: Self, left: Ptr[byte], right: Ptr[byte], Self: type)
```

Compare two erased payloads of this descriptor's type.

value\_dict[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-value_dict "Copy/link to this API element")

```python
value_dict(self: Self, data: Ptr[byte], Self: type)
```

Copy an erased dictionary through its typed items iterator.

cache[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-cache "Copy/link to this API element")

```python
cache(id: int)
```

```python
cache(T: type)
```

is\_parent[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-is_parent "Copy/link to this API element")

```python
is_parent(self: Self, T: type, Self: type)
```

wrap[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-wrap "Copy/link to this API element")

```python
wrap(arg: Ptr[u8], T: type)
```

Special methods 6

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: TypeInfo, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-typeinfo-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: TypeInfo, Self: type)
```

function get\_typeinfo[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#function-get_typeinfo "Copy/link to this API element")

@pure @llvm

```python
get_typeinfo(obj)
```

```llvm
%info = load ptr, ptr %obj
ret ptr %info
```

class RTTIType[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-rttitype "Copy/link to this API element")

@extend

```python
class RTTIType
```

Extends [`RTTIType`](https://docs.exaloop.io/libraries/api/classes/RTTIType/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/RTTIType/)

class Super[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-super "Copy/link to this API element")

@extend

```python
class Super[T]
```

Extends [`Super`](https://docs.exaloop.io/libraries/api/classes/Super/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Super/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-super-__new__ "Copy/link to this API element")

```python
__new__(B: type, obj: D, D: type)
```

Special methods 1

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/#class-super-__getattr__ "Copy/link to this API element")

```python
__getattr__(self: Self, attr: Literal[str], Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/rtti/
