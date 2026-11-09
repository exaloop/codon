# internal.numeric.float\_dtoa

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/numeric/float_dtoa.codon)

variable FLOAT64\_SIGN\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_sign_mask "Copy/link to this API element")

```python
FLOAT64_SIGN_MASK: Literal[int] = 0x8000000000000000
```

variable FLOAT64\_EXPONENT\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_exponent_mask "Copy/link to this API element")

```python
FLOAT64_EXPONENT_MASK: Literal[int] = 0x7FF0000000000000
```

variable FLOAT64\_FRACTION\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_fraction_mask "Copy/link to this API element")

```python
FLOAT64_FRACTION_MASK: Literal[int] = 0x000FFFFFFFFFFFFF
```

variable FLOAT64\_HIDDEN\_BIT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_hidden_bit "Copy/link to this API element")

```python
FLOAT64_HIDDEN_BIT: Literal[int] = 0x0010000000000000
```

variable FLOAT64\_EXPONENT\_SHIFT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_exponent_shift "Copy/link to this API element")

```python
FLOAT64_EXPONENT_SHIFT: Literal[int] = 52
```

variable FLOAT64\_EXPONENT\_INF\_NAN[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_exponent_inf_nan "Copy/link to this API element")

```python
FLOAT64_EXPONENT_INF_NAN: Literal[int] = 0x7FF
```

variable FLOAT64\_SUBNORMAL\_EXPONENT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_subnormal_exponent "Copy/link to this API element")

```python
FLOAT64_SUBNORMAL_EXPONENT: Literal[int] = -1074
```

variable FLOAT64\_NORMAL\_EXPONENT\_OFFSET[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float64_normal_exponent_offset "Copy/link to this API element")

```python
FLOAT64_NORMAL_EXPONENT_OFFSET: Literal[int] = -1075
```

variable FLOAT32\_SIGN\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_sign_mask "Copy/link to this API element")

```python
FLOAT32_SIGN_MASK: Literal[int] = 0x80000000
```

variable FLOAT32\_EXPONENT\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_exponent_mask "Copy/link to this API element")

```python
FLOAT32_EXPONENT_MASK: Literal[int] = 0x7F800000
```

variable FLOAT32\_FRACTION\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_fraction_mask "Copy/link to this API element")

```python
FLOAT32_FRACTION_MASK: Literal[int] = 0x007FFFFF
```

variable FLOAT32\_HIDDEN\_BIT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_hidden_bit "Copy/link to this API element")

```python
FLOAT32_HIDDEN_BIT: Literal[int] = 0x00800000
```

variable FLOAT32\_EXPONENT\_SHIFT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_exponent_shift "Copy/link to this API element")

```python
FLOAT32_EXPONENT_SHIFT: Literal[int] = 23
```

variable FLOAT32\_EXPONENT\_INF\_NAN[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_exponent_inf_nan "Copy/link to this API element")

```python
FLOAT32_EXPONENT_INF_NAN: Literal[int] = 0xFF
```

variable FLOAT32\_SUBNORMAL\_EXPONENT[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_subnormal_exponent "Copy/link to this API element")

```python
FLOAT32_SUBNORMAL_EXPONENT: Literal[int] = -149
```

variable FLOAT32\_NORMAL\_EXPONENT\_OFFSET[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#variable-float32_normal_exponent_offset "Copy/link to this API element")

```python
FLOAT32_NORMAL_EXPONENT_OFFSET: Literal[int] = -150
```

function float\_bits[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#function-float_bits "Copy/link to this API element")

@pure @llvm

```python
float_bits(value: float)
```

```llvm
%bits = bitcast double %value to i64
ret i64 %bits
```

function float32\_bits[](https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/#function-float32_bits "Copy/link to this API element")

@pure @llvm

```python
float32_bits(value: float32)
```

```llvm
%bits = bitcast float %value to i32
ret i32 %bits
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/numeric/float_dtoa/
