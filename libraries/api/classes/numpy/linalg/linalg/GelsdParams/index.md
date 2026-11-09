# GelsdParams

Standard library class `numpy.linalg.linalg.GelsdParams` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/linalg.codon)

class GelsdParams[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/#class-gelsdparams "Copy/link to this API element")

```python
class GelsdParams[T, B]
```

Fields 16

`M` `fint`

`N` `fint`

`NRHS` `fint`

`A` `Ptr[T]`

`LDA` `fint`

`B_` `Ptr[T]`

`LDB` `fint`

`S` `Ptr[B]`

`RCOND` `B`

`RANK` `fint`

`WORK` `Ptr[T]`

`LWORK` `fint`

`RWORK` `Ptr[B]`

`IWORK` `Ptr[fint]`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/#class-gelsdparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, m: fint, n: fint, nrhs: fint, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Methods 2

call[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/#class-gelsdparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

release[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/#class-gelsdparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/
