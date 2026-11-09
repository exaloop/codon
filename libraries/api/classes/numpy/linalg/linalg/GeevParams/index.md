# GeevParams

Standard library class `numpy.linalg.linalg.GeevParams` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/linalg.codon)

class GeevParams[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/#class-geevparams "Copy/link to this API element")

```python
class GeevParams[T, B]
```

Fields 18

`A` `Ptr[T]`

`WR` `Ptr[B]`

`WI` `Ptr[T]`

`VLR` `Ptr[T]`

`VRR` `Ptr[T]`

`WORK` `Ptr[T]`

`W` `Ptr[T]`

`VL` `Ptr[T]`

`VR` `Ptr[T]`

`N` `fint`

`LDA` `fint`

`LDVL` `fint`

`LDVR` `fint`

`LWORK` `fint`

`JOBVL` `u8`

`JOBVR` `u8`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/#class-geevparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, jobvl: u8, jobvr: u8, n: fint, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Methods 3

call[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/#class-geevparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

release[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/#class-geevparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

process\_geev\_results[](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/#class-geevparams-process_geev_results "Copy/link to this API element")

```python
process_geev_results(self: Self, Self: type)
```

Defined in [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/
