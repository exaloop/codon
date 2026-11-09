# SeedSequence

Standard library class `numpy.random.seed.SeedSequence` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/seed.codon)

class SeedSequence[](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/#class-seedsequence "Copy/link to this API element")

```python
class SeedSequence
```

Fields 4

`entropy` `ndarray[u32, ?]`

`pool_size` `int`

`n_children_spawned` `int`

`pool` `ndarray[u32, ?]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/#class-seedsequence-__init__ "Copy/link to this API element")

```python
__init__(self: Self, entropy = None, spawn_key: Optional[List[int]] = None, pool_size: int = DEFAULT_POOL_SIZE, n_children_spawned: int = 0, Self: type)
```

Defined in [`numpy.random.seed`](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/)

Properties 1

spawn\_key[](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/#class-seedsequence-spawn_key "Copy/link to this API element")

@property

```python
spawn_key(self: Self, Self: type)
```

Defined in [`numpy.random.seed`](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/)

Methods 2

generate\_state[](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/#class-seedsequence-generate_state "Copy/link to this API element")

```python
generate_state(self: Self, n_words: int, dtype: type = u32, Self: type)
```

Defined in [`numpy.random.seed`](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/)

spawn[](https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/#class-seedsequence-spawn "Copy/link to this API element")

```python
spawn(self: Self, n_children: int, Self: type)
```

Defined in [`numpy.random.seed`](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/seed/SeedSequence/
