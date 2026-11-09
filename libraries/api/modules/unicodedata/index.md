# unicodedata

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/unicodedata.codon)

variable unidata\_version[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#variable-unidata_version "Copy/link to this API element")

```python
unidata_version: str = UNICODEDATA_VERSION
```

function normalize[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-normalize "Copy/link to this API element")

```python
normalize(form: str, value: str)
```

function is\_normalized[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-is_normalized "Copy/link to this API element")

```python
is_normalized(form: str, value: str)
```

function decomposition[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-decomposition "Copy/link to this API element")

```python
decomposition(value: str)
```

function name[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-name "Copy/link to this API element")

```python
name(value: str)
```

@overload

```python
name(value: str, default)
```

function lookup[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-lookup "Copy/link to this API element")

```python
lookup(value: str)
```

function category[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-category "Copy/link to this API element")

```python
category(value: str)
```

function combining[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-combining "Copy/link to this API element")

```python
combining(value: str)
```

function bidirectional[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-bidirectional "Copy/link to this API element")

```python
bidirectional(value: str)
```

function east\_asian\_width[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-east_asian_width "Copy/link to this API element")

```python
east_asian_width(value: str)
```

function mirrored[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-mirrored "Copy/link to this API element")

```python
mirrored(value: str)
```

function decimal[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-decimal "Copy/link to this API element")

```python
decimal(value: str)
```

@overload

```python
decimal(value: str, default)
```

function digit[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-digit "Copy/link to this API element")

```python
digit(value: str)
```

@overload

```python
digit(value: str, default)
```

function numeric[](https://docs.exaloop.io/libraries/api/modules/unicodedata/#function-numeric "Copy/link to this API element")

```python
numeric(value: str)
```

@overload

```python
numeric(value: str, default)
```

Source: https://docs.exaloop.io/libraries/api/modules/unicodedata/
