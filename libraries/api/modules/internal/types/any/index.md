# internal.types.any

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/any.codon)

class Any[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any "Copy/link to this API element")

@tuple

```python
class Any
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/any/Any/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__new__ "Copy/link to this API element")

```python
__new__(ti: TypeInfo, data: Ptr[byte]) -> Any
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__new__-overload-2 "Copy/link to this API element")

```python
__new__(obj) -> Any
```

Properties 1

members[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-members "Copy/link to this API element")

@property

```python
members(self: Self)
```

Special methods 10

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__str__ "Copy/link to this API element")

```python
__str__(self: Self)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, o)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, o)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

\_\_unwrap\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__unwrap__ "Copy/link to this API element")

```python
__unwrap__[T: type](self: Self) -> T
```

\_\_cast\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__cast__ "Copy/link to this API element")

```python
__cast__[T: type](self: Self)
```

\_\_getattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__getattr__ "Copy/link to this API element")

```python
__getattr__[attr: Literal[str], T: type = Any](self: Self) -> T
```

\_\_setattr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__setattr__ "Copy/link to this API element")

```python
__setattr__[attr: Literal[str], T: type](self: Self, value: T)
```

\_\_name\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-any-__name__ "Copy/link to this API element")

@property

```python
__name__(self: Self) -> str
```

class Capsule[](https://docs.exaloop.io/libraries/api/modules/internal/types/any/#class-capsule "Copy/link to this API element")

@extend

```python
class Capsule[T]
```

Extends [`internal.core.Capsule`](https://docs.exaloop.io/libraries/api/classes/internal/core/Capsule/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Capsule/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/any/
