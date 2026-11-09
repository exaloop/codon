# fftblue

Standard library class `numpy.fft.pocketfft.fftblue` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/fft/pocketfft.codon)

class fftblue[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue "Copy/link to this API element")

```python
class fftblue[T, T0]
```

Fields 8

`n` `int`

`n2` `int`

`plan` `cfftp[T, T0]`

`mem` `arr[T]`

`bk` `Ptr[T]`

`bkf` `Ptr[T]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Methods 4

fft[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue-fft "Copy/link to this API element")

```python
fft(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

exec[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

exec\_r[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue-exec_r "Copy/link to this API element")

```python
exec_r(self: Self, c: Ptr[T0], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

dealloc[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/#class-fftblue-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/
