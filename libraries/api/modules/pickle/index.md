# pickle

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/pickle.codon)

function pickle[](https://docs.exaloop.io/libraries/api/modules/pickle/#function-pickle "Copy/link to this API element")

```python
pickle[T: type](x: T, jar: Jar)
```

function unpickle[](https://docs.exaloop.io/libraries/api/modules/pickle/#function-unpickle "Copy/link to this API element")

```python
unpickle[T: type](jar: Jar) -> T
```

function dump[](https://docs.exaloop.io/libraries/api/modules/pickle/#function-dump "Copy/link to this API element")

```python
dump[T: type](x: T, f)
```

function load[](https://docs.exaloop.io/libraries/api/modules/pickle/#function-load "Copy/link to this API element")

```python
load[T: type](f) -> T
```

class Int[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`internal.core.Int`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-int-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-int-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> Int[N]
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`internal.core.UInt`](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-uint-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-uint-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> UInt[N]
```

class float[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> float
```

class float32[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`internal.core.float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float32-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float32-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> float32
```

class float16[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`internal.core.float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float16-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float16-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> float16
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`internal.core.bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bfloat16-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bfloat16-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> bfloat16
```

class float128[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float128 "Copy/link to this API element")

@extend

```python
class float128
```

Extends [`internal.core.float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float128-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-float128-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> float128
```

class bool[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bool "Copy/link to this API element")

@extend

```python
class bool
```

Extends [`internal.core.bool`](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bool-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bool-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> bool
```

class str[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`internal.core.str`](https://docs.exaloop.io/libraries/api/classes/internal/core/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/str/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-str-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-str-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> str
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`internal.core.bytes`](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bytes-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-bytes-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> bytes
```

class List[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-list "Copy/link to this API element")

@extend

```python
class List[T]
```

Extends [`internal.types.collections.list.List`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-list-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-list-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> List[T]
```

class DynamicTuple[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-dynamictuple "Copy/link to this API element")

@extend

```python
class DynamicTuple[T]
```

Extends [`internal.types.collections.tuple.DynamicTuple`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/tuple/DynamicTuple/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/tuple/DynamicTuple/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-dynamictuple-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-dynamictuple-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> DynamicTuple[T]
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`internal.types.collections.dict.OrderedDict`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/dict/OrderedDict/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-ordereddict-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-ordereddict-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> Dict[(K, V)]
```

class Set[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-set "Copy/link to this API element")

@extend

```python
class Set[K]
```

Extends [`internal.types.collections.set.Set`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/set/Set/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/set/Set/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-set-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/pickle/#class-set-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> Set[K]
```

Source: https://docs.exaloop.io/libraries/api/modules/pickle/
