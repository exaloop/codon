# internal.format

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/format.codon)

function seq\_locale\_decimal\_point[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-seq_locale_decimal_point "Copy/link to this API element")

@C

```python
seq_locale_decimal_point()
```

function seq\_locale\_grouping[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-seq_locale_grouping "Copy/link to this API element")

@C

```python
seq_locale_grouping()
```

function seq\_locale\_thousands\_sep[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-seq_locale_thousands_sep "Copy/link to this API element")

@C

```python
seq_locale_thousands_sep()
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`Int`](https://docs.exaloop.io/libraries/api/classes/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Int/)

Special methods 1

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-int-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`UInt`](https://docs.exaloop.io/libraries/api/classes/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/UInt/)

Special methods 1

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-uint-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`float`](https://docs.exaloop.io/libraries/api/classes/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float/)

Special methods 2

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-float-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-float-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

class str[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`str`](https://docs.exaloop.io/libraries/api/classes/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/str/)

Special methods 1

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

class Ptr[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-ptr "Copy/link to this API element")

@extend

```python
class Ptr[T]
```

Extends [`Ptr`](https://docs.exaloop.io/libraries/api/classes/Ptr/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Ptr/)

Special methods 1

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-ptr-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`Int`](https://docs.exaloop.io/libraries/api/classes/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Int/)

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-int-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`UInt`](https://docs.exaloop.io/libraries/api/classes/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/UInt/)

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-uint-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

variable FORMAT\_FLAG\_ALT[](https://docs.exaloop.io/libraries/api/modules/internal/format/#variable-format_flag_alt "Copy/link to this API element")

```python
FORMAT_FLAG_ALT: Literal[int] = 1
```

variable FORMAT\_FLAG\_ZERO[](https://docs.exaloop.io/libraries/api/modules/internal/format/#variable-format_flag_zero "Copy/link to this API element")

```python
FORMAT_FLAG_ZERO: Literal[int] = 2
```

variable FORMAT\_FLAG\_LEFT[](https://docs.exaloop.io/libraries/api/modules/internal/format/#variable-format_flag_left "Copy/link to this API element")

```python
FORMAT_FLAG_LEFT: Literal[int] = 4
```

variable FORMAT\_FLAG\_SPACE[](https://docs.exaloop.io/libraries/api/modules/internal/format/#variable-format_flag_space "Copy/link to this API element")

```python
FORMAT_FLAG_SPACE: Literal[int] = 8
```

variable FORMAT\_FLAG\_PLUS[](https://docs.exaloop.io/libraries/api/modules/internal/format/#variable-format_flag_plus "Copy/link to this API element")

```python
FORMAT_FLAG_PLUS: Literal[int] = 16
```

function float\_format\_write[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-float_format_write "Copy/link to this API element")

```python
float_format_write(value: float, conversion: int, precision: int, alternate: bool, output: Ptr[u8])
```

function float\_format\_write\_padded[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-float_format_write_padded "Copy/link to this API element")

```python
float_format_write_padded(value: float, conversion: int, precision: int, flags: int, width: int, output: Ptr[u8])
```

function integer\_format\_write\_padded[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-integer_format_write_padded "Copy/link to this API element")

```python
integer_format_write_padded(value, conversion: int, precision: int, flags: int, width: int, output: Ptr[u8])
```

function percent\_format[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-percent_format "Copy/link to this API element")

```python
percent_format(format: str, values)
```

function percent\_format\_mapping[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-percent_format_mapping "Copy/link to this API element")

```python
percent_format_mapping(format: str, values)
```

function bytes\_percent\_format[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-bytes_percent_format "Copy/link to this API element")

```python
bytes_percent_format(format: bytes, values)
```

function bytes\_percent\_format\_mapping[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-bytes_percent_format_mapping "Copy/link to this API element")

```python
bytes_percent_format_mapping(format: bytes, values)
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Special methods 1

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-bytes-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, values, Self: type)
```

class str[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`str`](https://docs.exaloop.io/libraries/api/classes/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/str/)

Methods 2

format[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str-format "Copy/link to this API element")

```python
format(self: Self, *values, Self: type)
```

format\_map[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str-format_map "Copy/link to this API element")

```python
format_map(self: Self, mapping, Self: type)
```

Special methods 1

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/format/#class-str-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, values, Self: type)
```

function brace\_format[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-brace_format "Copy/link to this API element")

```python
brace_format(format: str, values)
```

function brace\_format\_map[](https://docs.exaloop.io/libraries/api/modules/internal/format/#function-brace_format_map "Copy/link to this API element")

```python
brace_format_map(format: str, mapping)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/format/
