# JSON

Standard library class `json.JSON` [View source](https://github.com/exaloop/codon/blob/master/stdlib/json.codon)

class JSON[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json "Copy/link to this API element")

@tuple

```python
class JSON
```

Two-word value with a packed tag and string metadata, plus scalar bits or a pointer.

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__new__ "Copy/link to this API element")

```python
__new__()
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__new__-overload-2 "Copy/link to this API element")

```python
__new__(value)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Properties 1

kind[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-kind "Copy/link to this API element")

@property

```python
kind(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Methods 17

array[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-array "Copy/link to this API element")

```python
array()
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

object[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-object "Copy/link to this API element")

```python
object()
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_null[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_null "Copy/link to this API element")

```python
is_null(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_number[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_number "Copy/link to this API element")

```python
is_number(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_bool[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_bool "Copy/link to this API element")

```python
is_bool(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_str[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_str "Copy/link to this API element")

```python
is_str(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_array[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_array "Copy/link to this API element")

```python
is_array(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

is\_object[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-is_object "Copy/link to this API element")

```python
is_object(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

as\_bool[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-as_bool "Copy/link to this API element")

```python
as_bool(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

as\_int[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-as_int "Copy/link to this API element")

```python
as_int(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

as\_float[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-as_float "Copy/link to this API element")

```python
as_float(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

as\_str[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-as_str "Copy/link to this API element")

```python
as_str(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

append[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-append "Copy/link to this API element")

```python
append(self: Self, value, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

get[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-get "Copy/link to this API element")

```python
get(self: Self, key: str, default = None, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

values[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-values "Copy/link to this API element")

```python
values(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

keys[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-keys "Copy/link to this API element")

```python
keys(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

items[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-items "Copy/link to this API element")

```python
items(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Special methods 13

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

```python
__getitem__(self: Self, key: str, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, index: int, value, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

```python
__setitem__(self: Self, key: str, value, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, index: int, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

```python
__delitem__(self: Self, key: str, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, key: str, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: JSON, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: JSON, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/json/JSON/#class-json-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Defined in [`json`](https://docs.exaloop.io/libraries/api/modules/json/)

Source: https://docs.exaloop.io/libraries/api/classes/json/JSON/
