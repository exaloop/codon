# Philox

Standard library class `numpy.random.philox.Philox` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/philox.codon)

class Philox[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox "Copy/link to this API element")

```python
class Philox
```

Fields 5

`ctr` `Tuple[u64, u64, u64, u64]`

`key` `Tuple[u64, u64]`

`buffer_pos` `int`

`buffer` `Tuple[u64, u64, u64, u64]`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-__init__ "Copy/link to this API element")

```python
__init__(self: Self, ctr: Tuple[u64, u64, u64, u64], key: Tuple[u64, u64], seed: SeedSequence, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

Methods 4

next64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

jump[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-jump "Copy/link to this API element")

```python
jump(self: Self, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

advance[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-advance "Copy/link to this API element")

```python
advance(self: Self, step: Tuple[u64, u64, u64, u64], Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

jump\_inplace[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-jump_inplace "Copy/link to this API element")

```python
jump_inplace(self: Self, jumps: int, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/#class-philox-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

Defined in [`numpy.random.philox`](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/
