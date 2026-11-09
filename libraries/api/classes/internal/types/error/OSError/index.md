# OSError

Standard library class `internal.types.error.OSError` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/error.codon)

class OSError[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/#class-oserror "Copy/link to this API element")

```python
class OSError
```

Fields 5

`errno` `Optional[int]`

`strerror` `Optional[str]`

`filename` `Optional[str]`

`filename2` `Optional[str]`

`winerror` `Optional[int]`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/#class-oserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/#class-oserror-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, errno: Optional[int], strerror: str, filename: Optional[str] = None, winerror: Optional[int] = None, filename2: Optional[str] = None, Self: type)
```

Defined in [`internal.types.error`](https://docs.exaloop.io/libraries/api/modules/internal/types/error/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/error/OSError/
