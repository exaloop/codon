# PCG64

Standard library class `numpy.random.pcg64.PCG64` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/pcg64.codon)

class PCG64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64 "Copy/link to this API element")

```python
class PCG64
```

Fields 3

`state` `u128`

`inc` `u128`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-__init__ "Copy/link to this API element")

```python
__init__(self: Self, initstate: u128, initseq: u128, seed: SeedSequence, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

Methods 14

setseq\_128\_step\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-setseq_128_step_r "Copy/link to this API element")

```python
setseq_128_step_r(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

output\_xsl\_rr\_128\_64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-output_xsl_rr_128_64 "Copy/link to this API element")

```python
output_xsl_rr_128_64(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

setseq\_128\_xsl\_rr\_64\_random\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-setseq_128_xsl_rr_64_random_r "Copy/link to this API element")

```python
setseq_128_xsl_rr_64_random_r(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

setseq\_128\_srandom\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-setseq_128_srandom_r "Copy/link to this API element")

```python
setseq_128_srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

advance\_lcg\_128[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-advance_lcg_128 "Copy/link to this API element")

```python
advance_lcg_128(self: Self, delta: u128, cur_mult: u128, cur_plus: u128, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

setseq\_128\_advance\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-setseq_128_advance_r "Copy/link to this API element")

```python
setseq_128_advance_r(self: Self, delta: u128, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

random\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-random_r "Copy/link to this API element")

```python
random_r(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

srandom\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-srandom_r "Copy/link to this API element")

```python
srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

advance\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-advance_r "Copy/link to this API element")

```python
advance_r(self: Self, delta: u128, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

next64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

pcg\_advance[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-pcg_advance "Copy/link to this API element")

```python
pcg_advance(self: Self, step: Tuple[u64, u64], Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

advance[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-advance "Copy/link to this API element")

```python
advance(self: Self, step: int, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

jump\_inplace[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-jump_inplace "Copy/link to this API element")

```python
jump_inplace(self: Self, jumps: int, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

set\_seed[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-set_seed "Copy/link to this API element")

```python
set_seed(self: Self, seed: Tuple[u64, u64], inc: Tuple[u64, u64], Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/#class-pcg64-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

Defined in [`numpy.random.pcg64`](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/
