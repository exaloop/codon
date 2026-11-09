# JSONEncoder

Standard library class `json.JSONEncoder` [View source](https://github.com/exaloop/codon/blob/master/stdlib/json.codon)

class JSONEncoder[](https://docs.exaloop.io/libraries/api/classes/json/JSONEncoder/#class-jsonencoder "Copy/link to this API element")

```python
class JSONEncoder
```

Fields 8

`ensure_ascii` `bool`

`allow_nan` `bool`

`check_circular` `bool`

`sort_keys` `bool`

`pretty` `bool`

`indent` `str`

`separators` `Tuple[str, str]`

`max_depth` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSONEncoder/#class-jsonencoder-__init__ "Copy/link to this API element")

```python
__init__(self: Self, ensure_ascii: bool = True, check_circular: bool = True, allow_nan: bool = True, indent = None, separators: Optional[Tuple[str, str]] = None, sort_keys: bool = False, max_depth: int = 512, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Methods 1

encode[](https://docs.exaloop.io/libraries/api/classes/json/JSONEncoder/#class-jsonencoder-encode "Copy/link to this API element")

```python
encode(self: Self, value, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Source: https://docs.exaloop.io/libraries/api/classes/json/JSONEncoder/
