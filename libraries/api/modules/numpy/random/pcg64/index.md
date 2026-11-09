# numpy.random.pcg64

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/pcg64.codon)

variable u128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-u128 "Copy/link to this API element")

```python
u128 = UInt[128]
```

function pcg\_int[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#function-pcg_int "Copy/link to this API element")

```python
pcg_int(hi, lo)
```

variable PCG\_DEFAULT\_MULTIPLIER\_HIGH[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_default_multiplier_high "Copy/link to this API element")

```python
PCG_DEFAULT_MULTIPLIER_HIGH = u128(2549297995355413924)
```

variable PCG\_DEFAULT\_MULTIPLIER\_LOW[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_default_multiplier_low "Copy/link to this API element")

```python
PCG_DEFAULT_MULTIPLIER_LOW = u128(4865540595714422341)
```

variable PCG\_DEFAULT\_MULTIPLIER\_128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_default_multiplier_128 "Copy/link to this API element")

```python
PCG_DEFAULT_MULTIPLIER_128 = pcg_int(PCG_DEFAULT_MULTIPLIER_HIGH, PCG_DEFAULT_MULTIPLIER_LOW)
```

variable PCG\_DEFAULT\_INCREMENT\_128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_default_increment_128 "Copy/link to this API element")

```python
PCG_DEFAULT_INCREMENT_128 = pcg_int(6364136223846793005, 1442695040888963407)
```

variable PCG\_STATE\_SETSEQ\_128\_INITIALIZER[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_state_setseq_128_initializer "Copy/link to this API element")

```python
PCG_STATE_SETSEQ_128_INITIALIZER = (pcg_int(0x979c9a98d8462005, 0x7d3e9cb6cfe0549b), pcg_int(1, 0xda3e39cb94b95bdb))
```

variable PCG\_CHEAP\_MULTIPLIER\_128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg_cheap_multiplier_128 "Copy/link to this API element")

```python
PCG_CHEAP_MULTIPLIER_128 = ((u128(0xda942042) << u128(32)) | u128(0xe4dd58b5))
```

variable PCG64\_INITIALIZER[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#variable-pcg64_initializer "Copy/link to this API element")

```python
PCG64_INITIALIZER = PCG_STATE_SETSEQ_128_INITIALIZER
```

function rotr64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#function-rotr64 "Copy/link to this API element")

```python
rotr64(value: u64, rot: int)
```

class PCG64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64 "Copy/link to this API element")

```python
class PCG64
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64/)

Fields 3

`state` `u128`

`inc` `u128`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-__init__ "Copy/link to this API element")

```python
__init__(self: Self, initstate: u128, initseq: u128, seed: SeedSequence, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Methods 14

setseq\_128\_step\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-setseq_128_step_r "Copy/link to this API element")

```python
setseq_128_step_r(self: Self, Self: type)
```

output\_xsl\_rr\_128\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-output_xsl_rr_128_64 "Copy/link to this API element")

```python
output_xsl_rr_128_64(self: Self, Self: type)
```

setseq\_128\_xsl\_rr\_64\_random\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-setseq_128_xsl_rr_64_random_r "Copy/link to this API element")

```python
setseq_128_xsl_rr_64_random_r(self: Self, Self: type)
```

setseq\_128\_srandom\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-setseq_128_srandom_r "Copy/link to this API element")

```python
setseq_128_srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

advance\_lcg\_128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-advance_lcg_128 "Copy/link to this API element")

```python
advance_lcg_128(self: Self, delta: u128, cur_mult: u128, cur_plus: u128, Self: type)
```

setseq\_128\_advance\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-setseq_128_advance_r "Copy/link to this API element")

```python
setseq_128_advance_r(self: Self, delta: u128, Self: type)
```

random\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-random_r "Copy/link to this API element")

```python
random_r(self: Self, Self: type)
```

srandom\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-srandom_r "Copy/link to this API element")

```python
srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

advance\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-advance_r "Copy/link to this API element")

```python
advance_r(self: Self, delta: u128, Self: type)
```

next64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

pcg\_advance[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-pcg_advance "Copy/link to this API element")

```python
pcg_advance(self: Self, step: Tuple[u64, u64], Self: type)
```

advance[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-advance "Copy/link to this API element")

```python
advance(self: Self, step: int, Self: type)
```

jump\_inplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-jump_inplace "Copy/link to this API element")

```python
jump_inplace(self: Self, jumps: int, Self: type)
```

set\_seed[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-set_seed "Copy/link to this API element")

```python
set_seed(self: Self, seed: Tuple[u64, u64], inc: Tuple[u64, u64], Self: type)
```

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

class PCG64DXSM[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm "Copy/link to this API element")

```python
class PCG64DXSM
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/pcg64/PCG64DXSM/)

Fields 3

`state` `u128`

`inc` `u128`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-__init__ "Copy/link to this API element")

```python
__init__(self: Self, initstate: u128, initseq: u128, seed: SeedSequence, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Methods 14

cm\_step\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-cm_step_r "Copy/link to this API element")

```python
cm_step_r(self: Self, Self: type)
```

setseq\_128\_srandom\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-setseq_128_srandom_r "Copy/link to this API element")

```python
setseq_128_srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

srandom\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-srandom_r "Copy/link to this API element")

```python
srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

setseq\_128\_step\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-setseq_128_step_r "Copy/link to this API element")

```python
setseq_128_step_r(self: Self, Self: type)
```

output\_cm\_128\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-output_cm_128_64 "Copy/link to this API element")

```python
output_cm_128_64(self: Self, Self: type)
```

cm\_random\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-cm_random_r "Copy/link to this API element")

```python
cm_random_r(self: Self, Self: type)
```

cm\_srandom\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-cm_srandom_r "Copy/link to this API element")

```python
cm_srandom_r(self: Self, initstate: u128, initseq: u128, Self: type)
```

advance\_lcg\_128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-advance_lcg_128 "Copy/link to this API element")

```python
advance_lcg_128(self: Self, delta: u128, cur_mult: u128, cur_plus: u128, Self: type)
```

cm\_advance\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-cm_advance_r "Copy/link to this API element")

```python
cm_advance_r(self: Self, delta: u128, Self: type)
```

next64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

pcg\_advance[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-pcg_advance "Copy/link to this API element")

```python
pcg_advance(self: Self, step: Tuple[u64, u64], Self: type)
```

advance[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-advance "Copy/link to this API element")

```python
advance(self: Self, step: int, Self: type)
```

jump\_inplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-jump_inplace "Copy/link to this API element")

```python
jump_inplace(self: Self, jumps: int, Self: type)
```

set\_seed[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-set_seed "Copy/link to this API element")

```python
set_seed(self: Self, seed: Tuple[u64, u64], inc: Tuple[u64, u64], Self: type)
```

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/#class-pcg64dxsm-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/
