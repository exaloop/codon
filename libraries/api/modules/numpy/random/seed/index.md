# numpy.random.seed

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/seed.codon)

variable DEFAULT\_POOL\_SIZE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-default_pool_size "Copy/link to this API element")

```python
DEFAULT_POOL_SIZE = 4
```

variable INIT\_A[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-init_a "Copy/link to this API element")

```python
INIT_A = u32(0x43b0d7e5)
```

variable MULT\_A[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-mult_a "Copy/link to this API element")

```python
MULT_A = u32(0x931e8875)
```

variable INIT\_B[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-init_b "Copy/link to this API element")

```python
INIT_B = u32(0x8b51f9dd)
```

variable MULT\_B[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-mult_b "Copy/link to this API element")

```python
MULT_B = u32(0x58f38ded)
```

variable MIX\_MULT\_L[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-mix_mult_l "Copy/link to this API element")

```python
MIX_MULT_L = u32(0xca01f9dd)
```

variable MIX\_MULT\_R[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-mix_mult_r "Copy/link to this API element")

```python
MIX_MULT_R = u32(0x4973f715)
```

variable XSHIFT[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-xshift "Copy/link to this API element")

```python
XSHIFT = u32(((4 * 8) // 2))
```

variable MASK32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#variable-mask32 "Copy/link to this API element")

```python
MASK32 = u32(0xFFFFFFFF)
```

function hashmix[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#function-hashmix "Copy/link to this API element")

```python
hashmix(value: u32, hash_const: u32)
```

function mix[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#function-mix "Copy/link to this API element")

```python
mix(x: u32, y: u32)
```

function random\_seed\_time\_pid[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#function-random_seed_time_pid "Copy/link to this API element")

```python
random_seed_time_pid()
```

class SeedSequence[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#class-seedsequence "Copy/link to this API element")

```python
class SeedSequence
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/)

Fields 4

`entropy` `ndarray[u32, ?]`

`pool_size` `int`

`n_children_spawned` `int`

`pool` `ndarray[u32, ?]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#class-seedsequence-__init__ "Copy/link to this API element")

```python
__init__(self: Self, entropy = None, spawn_key: Optional[List[int]] = None, pool_size: int = DEFAULT_POOL_SIZE, n_children_spawned: int = 0, Self: type)
```

Properties 1

spawn\_key[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#class-seedsequence-spawn_key "Copy/link to this API element")

@property

```python
spawn_key(self: Self, Self: type)
```

Methods 2

generate\_state[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#class-seedsequence-generate_state "Copy/link to this API element")

```python
generate_state(self: Self, n_words: int, dtype: type = u32, Self: type)
```

spawn[](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/#class-seedsequence-spawn "Copy/link to this API element")

```python
spawn(self: Self, n_children: int, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/
