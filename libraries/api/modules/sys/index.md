# sys

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/sys.codon)

Native process streams, encoding queries and Codon implementation metadata.

Version information describes Codon, not a CPython language compatibility level. The compiled runtime does not expose interpreter frames or reference counts.

`exit()` accepts integers, booleans, strings and `None`, including optional values. An uncaught string exit writes the message to `stderr` and exits with status `1`; `None` maps to status `0`. Unlike Python, other objects are rejected at compile time.

variable platform[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-platform "Copy/link to this API element")

```python
platform: Literal[str] = static.platform()
```

variable argv[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-argv "Copy/link to this API element")

```python
argv = list(__argv__, __argc__)
```

variable stdin[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-stdin "Copy/link to this API element")

```python
stdin = File[str](_C.seq_stdin(), mode="r")
```

variable stdout[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-stdout "Copy/link to this API element")

```python
stdout = File[str](_C.seq_stdout(), mode="w")
```

variable stderr[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-stderr "Copy/link to this API element")

```python
stderr = File[str](_C.seq_stderr(), mode="w")
```

function exit[](https://docs.exaloop.io/libraries/api/modules/sys/#function-exit "Copy/link to this API element")

```python
exit(status = 0)
```

variable maxsize[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-maxsize "Copy/link to this API element")

```python
maxsize: Literal[int] = 0x7FFFFFFFFFFFFFFF
```

variable maxunicode[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-maxunicode "Copy/link to this API element")

```python
maxunicode: Literal[int] = 0x10FFFF
```

variable byteorder[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-byteorder "Copy/link to this API element")

```python
byteorder = _byteorder()
```

variable version\_info[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-version_info "Copy/link to this API element")

```python
version_info = _version_info(__codon_version_major__, __codon_version_minor__, __codon_version_micro__, "final", 0)
```

variable version[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-version "Copy/link to this API element")

```python
version: Literal[str] = (((((static.int_to_string(__codon_version_major__) + ".") + static.int_to_string(__codon_version_minor__)) + ".") + static.int_to_string(__codon_version_micro__)) + " (Codon)")
```

variable hexversion[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-hexversion "Copy/link to this API element")

```python
hexversion: Literal[int] = ((((__codon_version_major__ << 24) | (__codon_version_minor__ << 16)) | (__codon_version_micro__ << 8)) | 0xF0)
```

variable implementation[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-implementation "Copy/link to this API element")

```python
implementation = _implementation("codon", version_info, hexversion, None)
```

variable float\_info[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-float_info "Copy/link to this API element")

```python
float_info = _float_info(1.7976931348623157e308, 1024, 308, 2.2250738585072014e-308, -1021, -307, 15, 53, 2.220446049250313e-16, 2, 1)
```

variable float\_repr\_style[](https://docs.exaloop.io/libraries/api/modules/sys/#variable-float_repr_style "Copy/link to this API element")

```python
float_repr_style: Literal[str] = "short"
```

function getdefaultencoding[](https://docs.exaloop.io/libraries/api/modules/sys/#function-getdefaultencoding "Copy/link to this API element")

```python
getdefaultencoding()
```

function getfilesystemencoding[](https://docs.exaloop.io/libraries/api/modules/sys/#function-getfilesystemencoding "Copy/link to this API element")

```python
getfilesystemencoding()
```

function getfilesystemencodeerrors[](https://docs.exaloop.io/libraries/api/modules/sys/#function-getfilesystemencodeerrors "Copy/link to this API element")

```python
getfilesystemencodeerrors()
```

Source: https://docs.exaloop.io/libraries/api/modules/sys/
