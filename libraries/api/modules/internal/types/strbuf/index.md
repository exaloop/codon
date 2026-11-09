# internal.types.strbuf

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/strbuf.codon)

class strbuf[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf "Copy/link to this API element")

```python
class strbuf
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/strbuf/strbuf/)

Fields 4

`data` `Ptr[u8]`

`n` `int`

`m` `int`

`kind` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf-__init__ "Copy/link to this API element")

```python
__init__(self: Self, capacity: int = 16, kind: int = 0, Self: type)
```

Methods 3

append[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf-append "Copy/link to this API element")

```python
append(self: Self, s: str, Self: type)
```

reverse[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf-reverse "Copy/link to this API element")

```python
reverse(self: Self, Self: type)
```

trim[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf-trim "Copy/link to this API element")

```python
trim(self: Self, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/#class-strbuf-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/strbuf/
