# numpy.random.philox

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/philox.codon)

variable u128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#variable-u128 "Copy/link to this API element")

```python
u128 = UInt[128]
```

variable PHILOX\_BUFFER\_SIZE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#variable-philox_buffer_size "Copy/link to this API element")

```python
PHILOX_BUFFER_SIZE: Literal[int] = 4
```

variable philox4x64\_rounds[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#variable-philox4x64_rounds "Copy/link to this API element")

```python
philox4x64_rounds: Literal[int] = 10
```

function mulhilo64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#function-mulhilo64 "Copy/link to this API element")

```python
mulhilo64(a: u64, b: u64)
```

function philox4x64\_R[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#function-philox4x64_r "Copy/link to this API element")

```python
philox4x64_R(R: int, ctr: Tuple[u64, u64, u64, u64], key: Tuple[u64, u64])
```

class Philox[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox "Copy/link to this API element")

```python
class Philox
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/philox/Philox/)

Fields 5

`ctr` `Tuple[u64, u64, u64, u64]`

`key` `Tuple[u64, u64]`

`buffer_pos` `int`

`buffer` `Tuple[u64, u64, u64, u64]`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-__init__ "Copy/link to this API element")

```python
__init__(self: Self, ctr: Tuple[u64, u64, u64, u64], key: Tuple[u64, u64], seed: SeedSequence, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Methods 4

next64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

jump[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-jump "Copy/link to this API element")

```python
jump(self: Self, Self: type)
```

advance[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-advance "Copy/link to this API element")

```python
advance(self: Self, step: Tuple[u64, u64, u64, u64], Self: type)
```

jump\_inplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-jump_inplace "Copy/link to this API element")

```python
jump_inplace(self: Self, jumps: int, Self: type)
```

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/#class-philox-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/
