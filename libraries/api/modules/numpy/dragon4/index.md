# numpy.dragon4

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/dragon4.codon)

class BigInt[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint "Copy/link to this API element")

```python
class BigInt
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/)

Fields 2

`length` `int`

`blocks` `Ptr[u32]`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, blocks: Ptr[u32], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Properties 1

even[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-even "Copy/link to this API element")

@property

```python
even(self: Self, Self: type)
```

Methods 13

make\_pow10[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-make_pow10 "Copy/link to this API element")

```python
make_pow10(i: u32)
```

copy[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-copy "Copy/link to this API element")

```python
copy(self: Self, dst: BigInt, Self: type)
```

set[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-set "Copy/link to this API element")

```python
set(self: Self, val: u64, Self: type)
```

```python
set(self: Self, hi: u64, lo: u64, Self: type)
```

```python
set(self: Self, val: u32, Self: type)
```

compare[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-compare "Copy/link to this API element")

```python
compare(self: Self, rhs: BigInt, Self: type)
```

add[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-add "Copy/link to this API element")

```python
add(self: Self, lhs: BigInt, rhs: BigInt, Self: type)
```

mul[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-mul "Copy/link to this API element")

```python
mul(self: Self, lhs: BigInt, rhs: BigInt, Self: type)
```

```python
mul(self: Self, lhs: BigInt, rhs: u32, Self: type)
```

mul2[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-mul2 "Copy/link to this API element")

```python
mul2(self: Self, in_: BigInt, Self: type)
```

```python
mul2(self: Self, Self: type)
```

mul10[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-mul10 "Copy/link to this API element")

```python
mul10(self: Self, Self: type)
```

pow10[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-pow10 "Copy/link to this API element")

```python
pow10(self: Self, exponent: u32, temp: BigInt, Self: type)
```

mul\_pow10[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-mul_pow10 "Copy/link to this API element")

```python
mul_pow10(self: Self, exponent: u32, temp: BigInt, Self: type)
```

pow2[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-pow2 "Copy/link to this API element")

```python
pow2(self: Self, exponent: u32, Self: type)
```

div\_with\_rem\_maxq9[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-div_with_rem_maxq9 "Copy/link to this API element")

```python
div_with_rem_maxq9(self: Self, divisor: BigInt, Self: type)
```

lshift[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-lshift "Copy/link to this API element")

```python
lshift(self: Self, shift: u32, Self: type)
```

Special methods 11

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, idx: int, val: u32, Self: type)
```

```python
__setitem__(self: Self, idx: int, val: u64, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: BigInt, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: BigInt, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: BigInt, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: BigInt, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: BigInt, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#class-bigint-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: BigInt, Self: type)
```

function dragon4[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-dragon4 "Copy/link to this API element")

```python
dragon4(bigints, exponent: i32, mantissa_bit: u32, has_unequal_margins: bool, digit_mode: int, cutoff_mode: int, cutoff_max: int, cutoff_min: int, out_buffer: cobj, buffer_size: int, out_exponent: Ptr[i32])
```

function format\_positional[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_positional "Copy/link to this API element")

```python
format_positional(buffer: cobj, buffer_size: int, mantissa, exponent: i32, signbit: u8, mantissa_bit: u32, has_unequal_margins: bool, digit_mode: int, cutoff_mode: int, precision: i32, min_digits: i32, trim_mode: int, digits_left: int, digits_right: int)
```

function format\_scientific[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_scientific "Copy/link to this API element")

```python
format_scientific(buffer: cobj, buffer_size: int, mantissa, exponent: i32, signbit: u8, mantissa_bit: u32, has_unequal_margins: bool, digit_mode: int, precision: i32, min_digits: i32, trim_mode: int, digits_left: int, exp_digits: int)
```

function print\_inf\_nan[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-print_inf_nan "Copy/link to this API element")

```python
print_inf_nan(buffer: cobj, buffer_size: int, mantissa: u64, mantissa_hex_width: int, signbit: u8)
```

function format\_float\_bits[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_float_bits "Copy/link to this API element")

```python
format_float_bits(buffer: cobj, buffer_size: int, mantissa, exponent: i32, signbit: u8, mantissa_bit: u32, has_unequal_margins: bool, scientific: bool, digit_mode: int, cutoff_mode: int, precision: i32, min_digits: i32, sign: bool, trim_mode: int, digits_left: int, digits_right: int, exp_digits: int)
```

function format\_ieee16[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_ieee16 "Copy/link to this API element")

```python
format_ieee16(value: float16, scientific: bool = False, unique: bool = True, fractional: bool = True, precision: int = -1, min_digits: int = -1, sign: bool = False, trim_mode: str = "k", digits_left: int = -1, digits_right: int = -1, exp_digits: int = -1)
```

function format\_ieee32[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_ieee32 "Copy/link to this API element")

```python
format_ieee32(value: float32, scientific: bool = False, unique: bool = True, fractional: bool = True, precision: int = -1, min_digits: int = -1, sign: bool = False, trim_mode: str = "k", digits_left: int = -1, digits_right: int = -1, exp_digits: int = -1)
```

function format\_ieee64[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_ieee64 "Copy/link to this API element")

```python
format_ieee64(value: float, scientific: bool = False, unique: bool = True, fractional: bool = True, precision: int = -1, min_digits: int = -1, sign: bool = False, trim_mode: str = "k", digits_left: int = -1, digits_right: int = -1, exp_digits: int = -1)
```

function format\_ieee128[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_ieee128 "Copy/link to this API element")

```python
format_ieee128(value: float128, scientific: bool = False, unique: bool = True, fractional: bool = True, precision: int = -1, min_digits: int = -1, sign: bool = False, trim_mode: str = "k", digits_left: int = -1, digits_right: int = -1, exp_digits: int = -1)
```

function format\_float[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_float "Copy/link to this API element")

```python
format_float(value, scientific: bool = False, unique: bool = True, fractional: bool = True, precision: int = -1, min_digits: int = -1, sign: bool = False, trim_mode: str = "k", digits_left: int = -1, digits_right: int = -1, exp_digits: int = -1)
```

function format\_float\_positional[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_float_positional "Copy/link to this API element")

```python
format_float_positional(x, precision: Optional[int] = None, unique: bool = True, fractional: bool = True, trim: str = "k", sign: bool = False, pad_left: Optional[int] = None, pad_right: Optional[int] = None, min_digits: Optional[int] = None)
```

function format\_float\_scientific[](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/#function-format_float_scientific "Copy/link to this API element")

```python
format_float_scientific(x, precision: Optional[int] = None, unique: bool = True, trim: str = "k", sign: bool = False, pad_left: Optional[int] = None, exp_digits: Optional[int] = None, min_digits: Optional[int] = None)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/
