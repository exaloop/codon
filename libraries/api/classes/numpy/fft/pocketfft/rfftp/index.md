# rfftp

Standard library class `numpy.fft.pocketfft.rfftp` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/fft/pocketfft.codon)

class rfftp[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp "Copy/link to this API element")

```python
class rfftp[T, T0]
```

Fields 5

`length` `int`

`mem` `arr[T0]`

`fact` `List[fctdata[T0]]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Methods 17

add\_factor[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-add_factor "Copy/link to this API element")

```python
add_factor(self: Self, factor: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radf2[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radf2 "Copy/link to this API element")

```python
radf2(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radf3[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radf3 "Copy/link to this API element")

```python
radf3(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radf4[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radf4 "Copy/link to this API element")

```python
radf4(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radf5[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radf5 "Copy/link to this API element")

```python
radf5(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radfg[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radfg "Copy/link to this API element")

```python
radfg(ido: int, ip: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0], csarr: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radb2[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radb2 "Copy/link to this API element")

```python
radb2(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radb3[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radb3 "Copy/link to this API element")

```python
radb3(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radb4[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radb4 "Copy/link to this API element")

```python
radb4(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radb5[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radb5 "Copy/link to this API element")

```python
radb5(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

radbg[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-radbg "Copy/link to this API element")

```python
radbg(ido: int, ip: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0], csarr: Ptr[T0])
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

copy\_and\_norm[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-copy_and_norm "Copy/link to this API element")

```python
copy_and_norm(self: Self, c: Ptr[T0], p1: Ptr[T0], fct: T0, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

exec[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T0], fct: T0, r2hc: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

factorize[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-factorize "Copy/link to this API element")

```python
factorize(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

twsize[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-twsize "Copy/link to this API element")

```python
twsize(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

comp\_twiddle[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-comp_twiddle "Copy/link to this API element")

```python
comp_twiddle(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

dealloc[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/#class-rfftp-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/
