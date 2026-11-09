# BaseException

Standard library class `internal.types.error.BaseException` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/error.codon)

class BaseException[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception "Copy/link to this API element")

```python
class BaseException
```

Fields 7

`message` `str`

`func` `str`

`file` `str`

`line` `int`

`col` `int`

`python_type` `cobj`

`cause` `Optional[BaseException]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

Properties 1

typename[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception-typename "Copy/link to this API element")

@property

```python
typename(self: Self, Self: type)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

Special methods 3

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception-__str__ "Copy/link to this API element")

```python
__str__(self: BaseException)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception-__repr__ "Copy/link to this API element")

```python
__repr__(self: BaseException)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

\_\_cause\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/#class-baseexception-__cause__ "Copy/link to this API element")

@property

```python
__cause__(self: Self, Self: type)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/error/BaseException/
