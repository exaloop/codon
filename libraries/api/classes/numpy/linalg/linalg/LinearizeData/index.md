# LinearizeData

Standard library class `numpy.linalg.linalg.LinearizeData` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/linalg.codon)

class LinearizeData[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata "Copy/link to this API element")

@tuple

```python
class LinearizeData
```

Fields 5

`rows` `int`

`cols` `int`

`row_strides` `int`

`col_strides` `int`

`out_lead_dim` `int`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-__new__ "Copy/link to this API element")

```python
__new__(rows: int, cols: int, row_strides: int, col_strides: int)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Methods 5

linearize[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-linearize "Copy/link to this API element")

```python
linearize(self: Self, dst: Ptr[T], src: Ptr[T], T: type, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

delinearize[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-delinearize "Copy/link to this API element")

```python
delinearize(self: Self, dst: Ptr[T], src: Ptr[T], T: type, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

nan\_matrix[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-nan_matrix "Copy/link to this API element")

```python
nan_matrix(self: Self, dst: Ptr[T], T: type, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

zero\_matrix[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-zero_matrix "Copy/link to this API element")

```python
zero_matrix(self: Self, dst: Ptr[T], T: type, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

identity\_matrix[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/#class-linearizedata-identity_matrix "Copy/link to this API element")

```python
identity_matrix(dst: Ptr[T], n: int, T: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/
