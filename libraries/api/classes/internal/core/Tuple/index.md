# Tuple

Standard library class `internal.core.Tuple` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon)

class Tuple[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple "Copy/link to this API element")

@tuple

```python
class Tuple
```

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__new__ "Copy/link to this API element")

@llvm

```python
__new__()
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

```llvm
ret {} {}
```

Special methods 23

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__add__ "Copy/link to this API element")

```python
__add__(self: Self, obj, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, n: Literal[int], Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, obj, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__le__ "Copy/link to this API element")

```python
__le__(self: Self, obj: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, dest: Ptr[u8], Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(src: Ptr[u8], Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__from_py__ "Copy/link to this API element")

```python
__from_py__(src: Ptr[u8], Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_tuplesize\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__tuplesize__ "Copy/link to this API element")

```python
__tuplesize__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/#class-tuple-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`internal.core`](https://docs.exaloop.io/libraries/api/modules/internal/core/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/Tuple/
