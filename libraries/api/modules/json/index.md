# json

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/json.codon)

Native JSON parsing and encoding with a tagged `JSON` value.

Provides `load`, `loads`, `dump`, `dumps`, `JSONDecoder`, `JSONEncoder`, and `JSONDecodeError`. Standard formatting options and non-finite number defaults follow Python.

```text
import json
value = json.loads('{"items": [1, true, null]}')
number = value["items"][0].as_int()
value["items"].append("text")
print(json.dumps(value, indent=2))
```

**Differences from Python:**

-   Decoding returns `JSON`. Use `kind`, `is_null()`, and typed accessors `as_bool()`, `as_int()`, `as_float()`, `as_str()`; wrong kinds raise `TypeError` (`as_float()` also accepts integers). `JSON()` is null; `JSON(value)` converts supported native scalars, optionals, lists, tuples, and string-keyed dicts. `JSON.array()` and `JSON.object()` create empty containers.
-   Use `values()`, `keys()`, or `items()` instead of direct iteration. Containers support indexing and mutation; copies share storage. Scalars are immutable. Equality is structural and kind-sensitive (`JSON(1) != JSON(1.0)`); values are unhashable. `str(value)` serializes JSON, including quotes around strings.
-   Integers are signed 64-bit; oversized literals raise `OverflowError`. Fractional and exponent-form numbers use binary64.
-   Keys must be strings. `skipkeys`, `cls`, `default`, `object_hook`, `object_pairs_hook`, `parse_int`, `parse_float`, and `parse_constant` are unsupported; unsupported native types, including nested members, are compile-time errors.
-   Parsing/encoding use `max_depth=512`, raising `RecursionError` when exceeded, even with `check_circular=False`. Structural comparison also has a 512 limit.
-   I/O is not streaming: `load()` reads the whole stream; `dump()` writes a complete string to a text stream. `iterencode()` is not provided.
-   Decode-error locations use Unicode code points; trailing-comma diagnostics follow CPython 3.14.

class JSON[](https://docs.exaloop.io/libraries/api/modules/json/#class-json "Copy/link to this API element")

@tuple

```python
class JSON
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/json/JSON/)

Two-word value with a packed tag and string metadata, plus scalar bits or a pointer.

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value)
```

Properties 1

kind[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-kind "Copy/link to this API element")

@property

```python
kind(self: Self) -> str
```

Methods 17

array[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-array "Copy/link to this API element")

```python
array()
```

object[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-object "Copy/link to this API element")

```python
object()
```

is\_null[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_null "Copy/link to this API element")

```python
is_null(self: Self) -> bool
```

is\_number[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_number "Copy/link to this API element")

```python
is_number(self: Self) -> bool
```

is\_bool[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_bool "Copy/link to this API element")

```python
is_bool(self: Self) -> bool
```

is\_str[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_str "Copy/link to this API element")

```python
is_str(self: Self) -> bool
```

is\_array[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_array "Copy/link to this API element")

```python
is_array(self: Self) -> bool
```

is\_object[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-is_object "Copy/link to this API element")

```python
is_object(self: Self) -> bool
```

as\_bool[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-as_bool "Copy/link to this API element")

```python
as_bool(self: Self) -> bool
```

as\_int[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-as_int "Copy/link to this API element")

```python
as_int(self: Self) -> int
```

as\_float[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-as_float "Copy/link to this API element")

```python
as_float(self: Self) -> float
```

as\_str[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-as_str "Copy/link to this API element")

```python
as_str(self: Self) -> str
```

append[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-append "Copy/link to this API element")

```python
append(self: Self, value)
```

get[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-get "Copy/link to this API element")

```python
get(self: Self, key: str, default = None) -> JSON
```

values[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-values "Copy/link to this API element")

```python
values(self: Self) -> Generator[JSON]
```

keys[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-keys "Copy/link to this API element")

```python
keys(self: Self) -> Generator[str]
```

items[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-items "Copy/link to this API element")

```python
items(self: Self) -> Generator[Tuple[(str, JSON)]]
```

Special methods 13

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__int__ "Copy/link to this API element")

```python
__int__(self: Self) -> int
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__float__ "Copy/link to this API element")

```python
__float__(self: Self) -> float
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__len__ "Copy/link to this API element")

```python
__len__(self: Self) -> int
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int) -> JSON
```

```python
__getitem__(self: Self, key: str) -> JSON
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, index: int, value)
```

```python
__setitem__(self: Self, key: str, value)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, index: int)
```

```python
__delitem__(self: Self, key: str)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: str) -> bool
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__str__ "Copy/link to this API element")

```python
__str__(self: Self) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: JSON) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: JSON) -> bool
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-json-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

class JSONDecodeError[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecodeerror "Copy/link to this API element")

```python
class JSONDecodeError(ValueError)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/json/JSONDecodeError/)

Fields 5

`msg` `str`

`doc` `str`

`pos` `int`

`lineno` `int`

`colno` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecodeerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, msg: str, doc: str, pos: int)
```

class JSONDecoder[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecoder "Copy/link to this API element")

```python
class JSONDecoder
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/json/JSONDecoder/)

Fields 2

`strict` `bool`

`max_depth` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecoder-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *, strict: bool = True, max_depth: int = 512)
```

Methods 2

raw\_decode[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecoder-raw_decode "Copy/link to this API element")

```python
raw_decode(self: Self, source: str, idx: int = 0) -> Tuple[(JSON, int)]
```

decode[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsondecoder-decode "Copy/link to this API element")

```python
decode(self: Self, source: str) -> JSON
```

function loads[](https://docs.exaloop.io/libraries/api/modules/json/#function-loads "Copy/link to this API element")

```python
loads(source, *, strict: bool = True, max_depth: int = 512) -> JSON
```

function load[](https://docs.exaloop.io/libraries/api/modules/json/#function-load "Copy/link to this API element")

```python
load(fp, *, strict: bool = True, max_depth: int = 512) -> JSON
```

class JSONEncoder[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsonencoder "Copy/link to this API element")

```python
class JSONEncoder
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/json/JSONEncoder/)

Fields 8

`ensure_ascii` `bool`

`allow_nan` `bool`

`check_circular` `bool`

`sort_keys` `bool`

`pretty` `bool`

`indent` `str`

`separators` `Tuple[(str, str)]`

`max_depth` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsonencoder-__init__ "Copy/link to this API element")

```python
__init__(self: Self, *, ensure_ascii: bool = True, check_circular: bool = True, allow_nan: bool = True, indent = None, separators: Optional[Tuple[(str, str)]] = None, sort_keys: bool = False, max_depth: int = 512)
```

Methods 1

encode[](https://docs.exaloop.io/libraries/api/modules/json/#class-jsonencoder-encode "Copy/link to this API element")

```python
encode(self: Self, value) -> str
```

function dumps[](https://docs.exaloop.io/libraries/api/modules/json/#function-dumps "Copy/link to this API element")

```python
dumps(value, *, ensure_ascii: bool = True, check_circular: bool = True, allow_nan: bool = True, indent = None, separators: Optional[Tuple[(str, str)]] = None, sort_keys: bool = False, max_depth: int = 512) -> str
```

function dump[](https://docs.exaloop.io/libraries/api/modules/json/#function-dump "Copy/link to this API element")

```python
dump(value, fp, *, ensure_ascii: bool = True, check_circular: bool = True, allow_nan: bool = True, indent = None, separators: Optional[Tuple[(str, str)]] = None, sort_keys: bool = False, max_depth: int = 512)
```

Source: https://docs.exaloop.io/libraries/api/modules/json/
