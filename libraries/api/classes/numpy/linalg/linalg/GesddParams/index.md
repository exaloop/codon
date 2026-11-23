# GesddParams

Standard library class `numpy.linalg.linalg.GesddParams` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/linalg.codon#L1239)

class GesddParams[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/#class-gesddparams "Copy/link to this API element")

```python
class GesddParams[T, B]
```

Fields 16

`A` `Ptr[T]`

`S` `Ptr[B]`

`U` `Ptr[T]`

`VT` `Ptr[T]`

`WORK` `Ptr[T]`

`RWORK` `Ptr[B]`

`IWORK` `Ptr[fint]`

`M` `fint`

`N` `fint`

`LDA` `fint`

`LDU` `fint`

`LDVT` `fint`

`LWORK` `fint`

`JOBZ` `u8`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/#class-gesddparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, jobz: u8, m: fint, n: fint)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Methods 2

call[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/#class-gesddparams-call "Copy/link to this API element")

```python
call(self: Self)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

release[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/#class-gesddparams-release "Copy/link to this API element")

```python
release(self: Self)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/
