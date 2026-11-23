# unicodedata

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/unicodedata.codon)

variable unidata\_version[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#variable-unidata_version "Copy/link to this API element")

```python
unidata_version: str = UNICODEDATA_VERSION
```

function normalize[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-normalize "Copy/link to this API element")

```python
normalize(form: str, value: str, /) -> str
```

function is\_normalized[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-is_normalized "Copy/link to this API element")

```python
is_normalized(form: str, value: str, /) -> bool
```

function decomposition[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-decomposition "Copy/link to this API element")

```python
decomposition(value: str, /) -> str
```

function name[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-name "Copy/link to this API element")

```python
name(value: str, /) -> str
```

@overload

```python
name(value: str, default, /)
```

function lookup[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-lookup "Copy/link to this API element")

```python
lookup(value: str, /) -> str
```

function category[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-category "Copy/link to this API element")

```python
category(value: str, /) -> str
```

function combining[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-combining "Copy/link to this API element")

```python
combining(value: str, /) -> int
```

function bidirectional[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-bidirectional "Copy/link to this API element")

```python
bidirectional(value: str, /) -> str
```

function east\_asian\_width[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-east_asian_width "Copy/link to this API element")

```python
east_asian_width(value: str, /) -> str
```

function mirrored[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-mirrored "Copy/link to this API element")

```python
mirrored(value: str, /) -> bool
```

function decimal[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-decimal "Copy/link to this API element")

```python
decimal(value: str, /) -> int
```

@overload

```python
decimal(value: str, default, /)
```

function digit[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-digit "Copy/link to this API element")

```python
digit(value: str, /) -> int
```

@overload

```python
digit(value: str, default, /)
```

function numeric[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-numeric "Copy/link to this API element")

```python
numeric(value: str, /) -> float
```

@overload

```python
numeric(value: str, default, /)
```

Source: https://docs.exaloop.io/libraries/api/modules/unicodedata/
