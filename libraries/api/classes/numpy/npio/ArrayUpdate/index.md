# ArrayUpdate

Standard library class `numpy.npio.ArrayUpdate` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npio.codon)

class ArrayUpdate[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate "Copy/link to this API element")

```python
class ArrayUpdate[A, C, F]
```

Fields 7

`arr` `A`

`cols` `C`

`conv` `F`

`last_row` `int`

`A` `type`

`C` `type`

`F` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-__init__ "Copy/link to this API element")

```python
__init__(self: Self, arr: A, cols: C, conv: F, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Properties 1

cap[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-cap "Copy/link to this API element")

@property

```python
cap(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Methods 4

resize\_arrays[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-resize_arrays "Copy/link to this API element")

```python
resize_arrays(self: Self, new_cap: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

trim\_arrays[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-trim_arrays "Copy/link to this API element")

```python
trim_arrays(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

convert[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-convert "Copy/link to this API element")

```python
convert(self: Self, field: str, idx: int, dtype: type, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

convert\_static[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-convert_static "Copy/link to this API element")

```python
convert_static(self: Self, field: str, idx: Literal[int], dtype: type, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/#class-arrayupdate-__call__ "Copy/link to this API element")

```python
__call__(self: Self, fields: Ptr[str], num_fields: int, row: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/
