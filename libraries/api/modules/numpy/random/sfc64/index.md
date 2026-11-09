# numpy.random.sfc64

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/sfc64.codon)

function rotl[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#function-rotl "Copy/link to this API element")

```python
rotl(value: u64, rot: int)
```

class SFC64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64 "Copy/link to this API element")

```python
class SFC64
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/sfc64/SFC64/)

Fields 5

`s0` `u64`

`s1` `u64`

`s2` `u64`

`s3` `u64`

`seed` `SeedSequence`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64-__init__ "Copy/link to this API element")

```python
__init__(self: Self, s0: u64, s1: u64, s2: u64, seed: SeedSequence, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, seed, Self: type)
```

Methods 1

next64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

Special methods 2

\_\_get\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64-__get_state__ "Copy/link to this API element")

```python
__get_state__(self: Self, Self: type)
```

\_\_set\_state\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/#class-sfc64-__set_state__ "Copy/link to this API element")

```python
__set_state__(self: Self, state, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/
