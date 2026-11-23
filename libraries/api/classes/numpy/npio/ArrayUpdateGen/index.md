# ArrayUpdateGen

Standard library class `numpy.npio.ArrayUpdateGen` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npio.codon#L2058)

class ArrayUpdateGen[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen "Copy/link to this API element")

```python
class ArrayUpdateGen[A, C, F, M]
```

Fields 11

`arr` `A`

`cols` `C`

`conv` `F`

`filling_values` `M`

`autostrip` `bool`

`loose` `bool`

`last_row` `int`

`A` `type`

`C` `type`

`F` `type`

`M` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-__init__ "Copy/link to this API element")

```python
__init__(self: Self, arr: A, cols: C, conv: F, filling_values: M, autostrip: bool, loose: bool)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Properties 1

cap[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-cap "Copy/link to this API element")

@property

```python
cap(self: Self)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Methods 6

resize\_arrays[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-resize_arrays "Copy/link to this API element")

```python
resize_arrays(self: Self, new_cap: int)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

trim\_arrays[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-trim_arrays "Copy/link to this API element")

```python
trim_arrays(self: Self)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

fill\_value[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-fill_value "Copy/link to this API element")

```python
fill_value[dtype: type](self: Self, idx: int) -> dtype
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

fill\_value\_static[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-fill_value_static "Copy/link to this API element")

```python
fill_value_static[idx: Literal[int], dtype: type](self: Self) -> dtype
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

convert[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-convert "Copy/link to this API element")

```python
convert[dtype: type](self: Self, field: str, idx: int) -> dtype
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

convert\_static[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-convert_static "Copy/link to this API element")

```python
convert_static[idx: Literal[int], dtype: type](self: Self, field: str) -> dtype
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/#class-arrayupdategen-__call__ "Copy/link to this API element")

```python
__call__(self: Self, fields: Ptr[str], num_fields: int, row: int)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/
