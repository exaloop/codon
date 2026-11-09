# cfftp

Standard library class `numpy.fft.pocketfft.cfftp` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/fft/pocketfft.codon)

class cfftp[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp "Copy/link to this API element")

```python
class cfftp[T, T0]
```

Fields 5

`length` `int`

`mem` `arr[T]`

`fact` `List[fctdata[T]]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Methods 15

add\_factor[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-add_factor "Copy/link to this API element")

```python
add_factor(self: Self, factor: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass2[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass2 "Copy/link to this API element")

```python
pass2(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass3[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass3 "Copy/link to this API element")

```python
pass3(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass4[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass4 "Copy/link to this API element")

```python
pass4(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass5[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass5 "Copy/link to this API element")

```python
pass5(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass7[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass7 "Copy/link to this API element")

```python
pass7(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass8[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass8 "Copy/link to this API element")

```python
pass8(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass11[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass11 "Copy/link to this API element")

```python
pass11(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

passg[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-passg "Copy/link to this API element")

```python
passg(ido: int, ip: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], csarr: Ptr[T], fwd: bool)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

pass\_all[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-pass_all "Copy/link to this API element")

```python
pass_all(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

exec[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

factorize[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-factorize "Copy/link to this API element")

```python
factorize(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

twsize[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-twsize "Copy/link to this API element")

```python
twsize(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

comp\_twiddle[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-comp_twiddle "Copy/link to this API element")

```python
comp_twiddle(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

dealloc[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/#class-cfftp-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/
