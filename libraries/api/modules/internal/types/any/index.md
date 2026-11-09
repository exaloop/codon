# internal.types.any

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/any.codon)

class Any[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any "Copy/link to this API element")

@tuple

```python
class Any
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/any/Any/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__new__ "Copy/link to this API element")

```python
__new__(ti: TypeInfo, data: Ptr[byte])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__new__-overload-2 "Copy/link to this API element")

```python
__new__(obj)
```

Properties 1

members[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-members "Copy/link to this API element")

@property

```python
members(self: Self, Self: type)
```

Special methods 10

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, o, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, o, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_unwrap\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__unwrap__ "Copy/link to this API element")

```python
__unwrap__(self: Self, T: type, Self: type)
```

\_\_cast\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__cast__ "Copy/link to this API element")

```python
__cast__(self: Self, T: type, Self: type)
```

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__getattr__ "Copy/link to this API element")

```python
__getattr__(self: Self, attr: Literal[str], T: type = Any, Self: type)
```

\_\_setattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__setattr__ "Copy/link to this API element")

```python
__setattr__(self: Self, attr: Literal[str], value: T, T: type, Self: type)
```

\_\_name\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__name__ "Copy/link to this API element")

@property

```python
__name__(self: Self, Self: type)
```

class Capsule[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-capsule "Copy/link to this API element")

@extend

```python
class Capsule[T]
```

Extends [`internal.core.Capsule`](https://docs.exaloop.io/libraries/api/classes/internal/core/Capsule/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Capsule/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/any/
