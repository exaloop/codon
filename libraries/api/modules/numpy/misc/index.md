# numpy.misc

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/misc.codon)

variable MAXDIMS[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#variable-maxdims "Copy/link to this API element")

```python
MAXDIMS: Literal[int] = 32
```

variable MAY\_SHARE\_BOUNDS[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#variable-may_share_bounds "Copy/link to this API element")

```python
MAY_SHARE_BOUNDS: Literal[int] = 0
```

variable MAY\_SHARE\_EXACT[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#variable-may_share_exact "Copy/link to this API element")

```python
MAY_SHARE_EXACT: Literal[int] = -1
```

alias i128[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#alias-i128 "Copy/link to this API element")

```python
i128 = Int[128]
```

Alias of [`Int[128]`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

class DiophantineTerm[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#class-diophantineterm "Copy/link to this API element")

@tuple

```python
class DiophantineTerm
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/misc/DiophantineTerm/)

Fields 2

`a` `int`

`ub` `int`

Methods 2

with\_a[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#class-diophantineterm-with_a "Copy/link to this API element")

```python
with_a(self: Self, a: int)
```

with\_ub[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#class-diophantineterm-with_ub "Copy/link to this API element")

```python
with_ub(self: Self, ub: int)
```

Special methods 1

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#class-diophantineterm-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: DiophantineTerm)
```

function shares\_memory[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#function-shares_memory "Copy/link to this API element")

```python
shares_memory(a: ndarray, b: ndarray, max_work: Optional[int] = None)
```

function may\_share\_memory[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#function-may_share_memory "Copy/link to this API element")

```python
may_share_memory(a: ndarray, b: ndarray, max_work: Optional[int] = None)
```

function setbufsize[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#function-setbufsize "Copy/link to this API element")

```python
setbufsize(size: int)
```

function getbufsize[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#function-getbufsize "Copy/link to this API element")

```python
getbufsize()
```

function byte\_bounds[](https://docs.exaloop.io/libraries/api/modules/numpy/misc/#function-byte_bounds "Copy/link to this API element")

```python
byte_bounds(a: ndarray)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/misc/
