# internal.unicode.normalization

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/unicode/normalization.codon)

variable HANGUL\_SBASE[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_sbase "Copy/link to this API element")

```python
HANGUL_SBASE: Literal[int] = 0xAC00
```

variable HANGUL\_LBASE[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_lbase "Copy/link to this API element")

```python
HANGUL_LBASE: Literal[int] = 0x1100
```

variable HANGUL\_VBASE[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_vbase "Copy/link to this API element")

```python
HANGUL_VBASE: Literal[int] = 0x1161
```

variable HANGUL\_TBASE[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_tbase "Copy/link to this API element")

```python
HANGUL_TBASE: Literal[int] = 0x11A7
```

variable HANGUL\_LCOUNT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_lcount "Copy/link to this API element")

```python
HANGUL_LCOUNT: Literal[int] = 19
```

variable HANGUL\_VCOUNT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_vcount "Copy/link to this API element")

```python
HANGUL_VCOUNT: Literal[int] = 21
```

variable HANGUL\_TCOUNT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_tcount "Copy/link to this API element")

```python
HANGUL_TCOUNT: Literal[int] = 28
```

variable HANGUL\_NCOUNT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_ncount "Copy/link to this API element")

```python
HANGUL_NCOUNT: Literal[int] = (HANGUL_VCOUNT * HANGUL_TCOUNT)
```

variable HANGUL\_SCOUNT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/#variable-hangul_scount "Copy/link to this API element")

```python
HANGUL_SCOUNT: Literal[int] = (HANGUL_LCOUNT * HANGUL_NCOUNT)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/unicode/normalization/
