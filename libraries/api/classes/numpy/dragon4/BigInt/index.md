# BigInt

Standard library class `numpy.dragon4.BigInt` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/dragon4.codon)

class BigInt[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint "Copy/link to this API element")

```python
class BigInt
```

Fields 2

`length` `int`

`blocks` `Ptr[u32]`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, blocks: Ptr[u32], Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

Properties 1

even[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-even "Copy/link to this API element")

@property

```python
even(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

Methods 13

make\_pow10[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-make_pow10 "Copy/link to this API element")

```python
make_pow10(i: u32)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

copy[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-copy "Copy/link to this API element")

```python
copy(self: Self, dst: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

set[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-set "Copy/link to this API element")

```python
set(self: Self, val: u64, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

```python
set(self: Self, hi: u64, lo: u64, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

```python
set(self: Self, val: u32, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

compare[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-compare "Copy/link to this API element")

```python
compare(self: Self, rhs: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

add[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-add "Copy/link to this API element")

```python
add(self: Self, lhs: BigInt, rhs: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

mul[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-mul "Copy/link to this API element")

```python
mul(self: Self, lhs: BigInt, rhs: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

```python
mul(self: Self, lhs: BigInt, rhs: u32, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

mul2[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-mul2 "Copy/link to this API element")

```python
mul2(self: Self, in_: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

```python
mul2(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

mul10[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-mul10 "Copy/link to this API element")

```python
mul10(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

pow10[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-pow10 "Copy/link to this API element")

```python
pow10(self: Self, exponent: u32, temp: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

mul\_pow10[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-mul_pow10 "Copy/link to this API element")

```python
mul_pow10(self: Self, exponent: u32, temp: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

pow2[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-pow2 "Copy/link to this API element")

```python
pow2(self: Self, exponent: u32, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

div\_with\_rem\_maxq9[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-div_with_rem_maxq9 "Copy/link to this API element")

```python
div_with_rem_maxq9(self: Self, divisor: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

lshift[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-lshift "Copy/link to this API element")

```python
lshift(self: Self, shift: u32, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

Special methods 11

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, idx: int, val: u32, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

```python
__setitem__(self: Self, idx: int, val: u64, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/#class-bigint-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: BigInt, Self: type)
```

Defined in [`numpy.dragon4`](https://docs.exaloop.io/libraries/api/modules/numpy/dragon4/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/dragon4/BigInt/
